#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "opencv4/opencv2/opencv.hpp"
#include "iostream"
#include "mpi.h"

#include "filter/filter.h"
#include "video_processor/video_processor.h"

using namespace cv;
using namespace std;

// Used to distord the face
int convolution_matrix_size_m = 21;

// Number of nodes to deploy
int num_of_nodes;

// Execution time (Applying the filter)
double total_execution_time_ms = 0.0;

// Node info
int node_id;
int num_of_nodes;

// Frame info
int pixels_in_frame;
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
    int frame_id = 0;
    vector<Mat> master_frames;

    short flag = 1;
    for ( Mat frame : frames )
    {
        if (frame_id == 0)
        {
            master_frames.push_back(frame);
        }
        else
        {
            mat_to_pointers(frame, r, g, b);
            MPI_Send(&flag, 1, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD);
            MPI_Send(r, pixels_in_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD);
            MPI_Send(g, pixels_in_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD);
            MPI_Send(b, pixels_in_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD);
        }
        frame_id = (frame_id + 1) % num_of_nodes;
    }

    flag = 0;
    for (int i = 1; i < num_of_nodes; i++)
    {
        MPI_Send(&flag, 1, MPI_SHORT, i, 0, MPI_COMM_WORLD);
    }

    return master_frames;
}

void master_nodes_process(VideoCapture input_video, VideoWriter output_video)
{
    vector<Mat> frames;
    int total_frames = frames.size();
    while (true)
    {
        // Read the frame from the input video
        Mat current_frame;
        bool has_next_frame = next_frame(input_video, &current_frame);
        // If there are not more frames break the loop
        if (!has_next_frame) break;

        frames.push_back(current_frame);
    }

    pixels_in_frame = frames[0].rows * frames[0].cols;
    short *r, *g, *b;
    r = (short *) malloc(sizeof(short *) * pixels_in_frame);
    g = (short *) malloc(sizeof(short *) * pixels_in_frame);
    b = (short *) malloc(sizeof(short *) * pixels_in_frame);
    if (r == NULL || g == NULL || b == NULL)
    {
        perror("Error allocating memory");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < num_of_nodes; i++) {
        MPI_Send(&pixels_in_frame, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    frames = distribute_charge(frames);

    for (Mat frame : frames)
    {
        process_frame(frame);
    }

    int frame_id = 0;
    for (int i = 0; i < total_frames; i++)
    {
        Mat frame;
        if (frame_id == 0)
        {
            frame = frames[0];
            frames.erase(frames.begin());
        }
        else
        {
            short flag;
            MPI_Status recv_status;

            MPI_Recv(&flag, 1, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD, &recv_status);
            if (flag == 0) break;
            MPI_Recv(r, pixels_in_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD, &recv_status);
            MPI_Recv(g, pixels_in_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD, &recv_status);
            MPI_Recv(b, pixels_in_frame, MPI_SHORT, frame_id, 0, MPI_COMM_WORLD, &recv_status);

            pointers_to_mat(frame, r, g, b);
        }
        output_video.write(frame);
        frame_id = (frame_id + 1) % num_of_nodes;
    }

    free(r);
    free(g);
    free(b);
}

void slave_nodes_process()
{
    MPI_Status recv_status;
    MPI_Recv(&pixels_in_frame, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &recv_status);

    r = (short *) malloc(sizeof(short *) * pixels_in_frame);
    g = (short *) malloc(sizeof(short *) * pixels_in_frame);
    b = (short *) malloc(sizeof(short *) * pixels_in_frame);
    if (r == NULL || g == NULL || b == NULL)
    {
        perror("Error allocating memory");
        exit(EXIT_FAILURE);
    }

    short flag = 0;
    while (true)
    {
        MPI_Recv(&flag, 1, MPI_SHORT, 0, 0, MPI_COMM_WORLD, &recv_status);
        if (flag == 0) break;
        MPI_Recv(r, pixels_in_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(g, pixels_in_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(b, pixels_in_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD, &recv_status);

        Mat frame;
        pointers_to_mat(frame, r, g, b);
        process_frame(frame);
        mat_to_pointers(frame, r, g, b);

        MPI_Send(&flag, 1, MPI_SHORT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(r, pixels_in_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(g, pixels_in_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(b, pixels_in_frame, MPI_SHORT, 0, 0, MPI_COMM_WORLD);
    }
    MPI_Send(&flag, 1, MPI_SHORT, 0, 0, MPI_COMM_WORLD);

    free(r);
    free(g);
    free(b);
}

int main(int argc, char **argv)
{
    cout << "Processing video..." << endl;

    // Set up the command line arguments
    const String keys =
        "{@video_in             |       | Input video path}"
        "{@video_out            |       | Output video path}"
        "{@num_of_nodes         |       | The number of nodes to deploy to process the video"
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
    else if (!parser.has("@num_of_nodes"))
    {
        cout << "Error: The number of nodes is required" << endl;
        return -1;
    }

    // Load the number of nodes into the variable
    num_of_nodes = parser.get<int>("@num_of_nodes");

    // Check if the number of pixels to group was given and calculate
    // the new values for the corresponding variables
    if (parser.has("m"))
    {
        convolution_matrix_size_m = parser.get<int>("m");
    }

    load_cascade_classifier();

    // Store the paths for video_in and video_out
    String input_video_path = samples::findFile(parser.get<String>("@video_in"));
    String output_video_path = parser.get<String>("@video_out");

    // Open the given video
    VideoCapture input_video = open_video_capture(input_video_path);

    // Open the video writer to save the result
    VideoWriter output_video = open_video_writer(output_video_path, input_video);

    // MPI Section
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &node_id );
    MPI_Comm_size( MPI_COMM_WORLD, &num_of_nodes );

    if (node_id == 0) master_nodes_process(input_video, output_video);
    slave_nodes_process();

    MPI_Finalize( );

    // Close the input and output files
    input_video.release();
    output_video.release();

    cout << "The video was processed in " << total_execution_time_ms / 1e6 << " seconds" << endl;
    return 0;
}