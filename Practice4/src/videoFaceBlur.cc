#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "opencv4/opencv2/opencv.hpp"
#include "chrono"
#include "iostream"
#include "mpi.h"

#include "filter/filter.h"
#include "video_processor/video_processor.h"

using namespace cv;
using namespace std;
using namespace std::chrono;

// Used to distord the face
int convolution_matrix_size_m = 21;

// Execution time (Applying the filter)
double total_execution_time_ms = 0.0;

// Nodes info
int node_id;
int num_of_nodes;

// Frame info
int pixels_per_frame;
short *r, *g, *b;

void process_frame(Mat frame)
{
    vector<Rect> faces = detect_faces(frame);

    for (Rect face : faces)
    {
        apply_filter(frame, face);
    }
}

vector<Mat> distribute_charge(vector<Mat> frames)
{
    // Next node that a frame must be assigned
    int frame_id = 0;
    // Frames assigned to the master node
    vector<Mat> master_frames;

    // Flag that indicates to the nodes that one more frame should be read
    short next_frame_flag = 1;
    // Distribute the frames among the nodes
    for ( Mat frame : frames )
    {
        // If the current node is the master node store the frame into master_frames vector
        if (frame_id == 0) master_frames.push_back(frame);
        // If the current node is a slave node, then send it the next_frame_flag as 1
        // and the RGB factor of the frame
        else
        {
            // Send the next_frame_flag
            MPI_Send(&next_frame_flag, 1, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD);
            // Copy the frame from Mat to RGB pointer and send them
            mat_to_pointers(frame, r, g, b);
            MPI_Send(r, pixels_per_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD);
            MPI_Send(g, pixels_per_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD);
            MPI_Send(b, pixels_per_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD);
        }
        // Continue to the next node
        frame_id = (frame_id + 1) % num_of_nodes;
    }

    // Send the next_frame_flag as 0 the the slave nodes
    // to indicate there is no more frames
    next_frame_flag = 0;
    for (int i = 1; i < num_of_nodes; i++)
    {
        MPI_Send(&next_frame_flag, 1, MPI_SHORT, i, 0, MPI_COMM_WORLD);
    }

    // Return the frames assigned to master node
    return master_frames;
}

void master_nodes_process(String input_video_path, String output_video_path)
{
    // Open the given video
    VideoCapture input_video = open_video_capture(input_video_path);

    // Open the video writer to save the result
    VideoWriter output_video = open_video_writer(output_video_path, input_video);

    // Read the video and store all the frames
    vector<Mat> frames;
    while (true)
    {
        // Read the frame from the input video
        Mat current_frame;
        bool has_next_frame = next_frame(input_video, &current_frame);
        // If there are not more frames break the loop
        if (!has_next_frame) break;

        frames.push_back(current_frame);
    }
    // Store the amount of frames of the video
    int total_frames = frames.size();

    // Calculate the pixels_per_frame
    pixels_per_frame = frames[0].rows * frames[0].cols;

    // Allocate the memory required for RGB factor of one frame
    r = (short *) malloc(sizeof(short) * pixels_per_frame);
    g = (short *) malloc(sizeof(short) * pixels_per_frame);
    b = (short *) malloc(sizeof(short) * pixels_per_frame);
    if (r == NULL || g == NULL || b == NULL)
    {
        perror("Error allocating memory");
        exit(EXIT_FAILURE);
    }

    // Send the num of columns and rows of the frame
    for (int i = 1; i < num_of_nodes; i++) {
        MPI_Send(&frames[0].rows, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&frames[0].cols, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    // Distribute the frames among the nodes
    frames = distribute_charge(frames);

    // Iterate over the frames, dectect the faces and distord them
    for (Mat frame : frames)
    {
        process_frame(frame);
    }

    // Current frame id
    int frame_id = 0;
    // Iterate over the total frames, read them when necesaary and save the output video
    Mat current_frame = frames[0];
    for (int i = 0; i < total_frames; i++)
    {
        // If the current correspond correspond to the master node get it from the vector
        if (frame_id == 0)
        {
            current_frame = frames[0];
            frames.erase(frames.begin());
        }
        // If the current frame correspond correspond to a slave node get the frame reading it
        else
        {
            MPI_Status recv_status;

            // Read the frame RGB fractor from the slave node
            MPI_Recv(r, pixels_per_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD, &recv_status);
            MPI_Recv(g, pixels_per_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD, &recv_status);
            MPI_Recv(b, pixels_per_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD, &recv_status);

            // Convert from RGB factor to Mat object
            pointers_to_mat(current_frame, r, g, b);
        }
        // Save the result in the output video
        output_video.write(current_frame);
        // Continue to the next node
        frame_id = (frame_id + 1) % num_of_nodes;
    }

    // Free memory
    free(r);
    free(g);
    free(b);

    // Close the input and output files
    input_video.release();
    output_video.release();
}

void slave_nodes_process()
{
    // Receive the number of pixels per frame
    MPI_Status recv_status;
    int cols, rows;
    MPI_Recv(&rows, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &recv_status);
    MPI_Recv(&cols, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &recv_status);
    pixels_per_frame = cols * rows;

    // Allocate the memory for RGB factor
    r = (short *) malloc(sizeof(short) * pixels_per_frame);
    g = (short *) malloc(sizeof(short) * pixels_per_frame);
    b = (short *) malloc(sizeof(short) * pixels_per_frame);
    if (r == NULL || g == NULL || b == NULL)
    {
        perror("Error allocating memory");
        exit(EXIT_FAILURE);
    }

    // Flag that indicates if there is a frame to read
    short next_frame_flag = 0;
    // Read the frames and store them on frames vector
    vector<Mat> frames;
    while (true)
    {  
        // Read the next_frame_flag
        MPI_Recv(&next_frame_flag, 1, MPI_SHORT, 0, 0, MPI_COMM_WORLD, &recv_status);
        // If there are no more frames break the loop
        if (next_frame_flag == 0) break;
        // Create the mat object to Store the RGB factor
        Mat current_frame = Mat(rows, cols, CV_8UC3);
        // Read the RGB factor
        MPI_Recv(r, pixels_per_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(g, pixels_per_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(b, pixels_per_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD, &recv_status);

        // Convert the RGB factor to Mat object and push it to the frames vector
        pointers_to_mat(current_frame, r, g, b);
        frames.push_back(current_frame);
    }

    // Iterate over the frames and distord the faces
    for (Mat current_frame : frames)
    {   
        // Dectect the faces and distord them
        process_frame(current_frame);
    }

    // Iterate over the frames and send them back to the master node
    for (Mat current_frame : frames)
    {
        // Convert the Mat object to RGB factor
        mat_to_pointers(current_frame, r, g, b);

        // Send the RGB factor back to the master node
        MPI_Send(r, pixels_per_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(g, pixels_per_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(b, pixels_per_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD);
    }

    // Free memory
    free(r);
    free(g);
    free(b);
}

int main(int argc, char **argv)
{
    // Set up the command line arguments
    const String keys =
        "{@video_in             |       | Input video path}"
        "{@video_out            |       | Output video path}"
        "{m                     |       | Convolution matrix size}";
    CommandLineParser parser(argc, argv, keys);

    // Check if the input and output video's paths were given
    if (!parser.has("@video_in"))
    {
        cout << "Error: Input video path is required" << endl;
        return -1;
    }
    else if (!parser.has("@video_out"))
    {
        cout << "Error: Output video path is required" << endl;
        return -1;
    }

    // Check if the number of pixels to group was given and calculate
    // the new values for the corresponding variables
    if (parser.has("m"))
    {
        convolution_matrix_size_m = parser.get<int>("m");
    }
    setup_filter(convolution_matrix_size_m);

    // Load OpenCV cascade  classifier
    load_cascade_classifier();

    // Store the paths for video_in and video_out
    String input_video_path = samples::findFile(parser.get<String>("@video_in"));
    String output_video_path = parser.get<String>("@video_out");

    // MPI Section
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &node_id );
    MPI_Comm_size( MPI_COMM_WORLD, &num_of_nodes );

    if (node_id == 0)
    {
        auto start_time = high_resolution_clock::now();
        cout << "Processing video..." << endl;

        master_nodes_process(input_video_path, output_video_path);

        auto finish_time = high_resolution_clock::now();
        total_execution_time_ms = duration_cast<microseconds>(finish_time - start_time).count();
        cout << "The video was processed in " << total_execution_time_ms / 1e6 << " seconds" << endl;
    }
    else slave_nodes_process();

    MPI_Finalize( );

    return 0;
}