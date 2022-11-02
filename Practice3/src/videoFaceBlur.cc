#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "opencv4/opencv2/opencv.hpp"
#include "omp.h"
#include "iostream"

#include "filter/filter.h"
#include "video_processor/video_processor.h"

using namespace cv;
using namespace std;

// Structure to grouping a frame and the faces detected on it
struct Frame
{
    Mat frame;
    vector<Rect> faces;
};

// Used to distord the face
int convolution_matrix_size_m = 21;

// Num of block and threads per block to use in the GPU
int num_of_blocks_m, threads_per_block_m;

// Queue for frames and faces and its syncronizers
bool are_remaining_frames = true;
queue<Frame> frames;
omp_lock_t queue_locker;

// Iterate frame by frame, dected the faces on it and
// and push both into the pipe
void process_frames(VideoCapture input_video)
{
    while (true)
    {
        // Read the frame from the input video
        Mat current_frame;
        bool has_next_frame = next_frame(input_video, &current_frame);
        // If there are not more frames break the loop
        if (!has_next_frame) break;

        // Detect the faces on the frame
        vector<Rect> faces = detect_faces(current_frame);

        // Push the frame and the faces into the queue
        omp_set_lock(&queue_locker);
        frames.push({ current_frame.clone(), faces });
        omp_unset_lock(&queue_locker);
    }

    are_remaining_frames = false;
}

// Read the frame and its faces from the pipe, setup the environment
// and lauch the process in the GPU
void launch_gpu_process(VideoWriter output_video)
{
    setup_filter(convolution_matrix_size_m, threads_per_block_m);
    while (true)
    {
        // Verify if there are frames pending for being processed
        bool are_pending_frames = frames.size() != 0;
        if (!are_remaining_frames && !are_pending_frames) break;
        else if (!are_pending_frames) continue;

        omp_set_lock(&queue_locker);
        struct Frame current_frame = frames.back();
        frames.pop();
        omp_unset_lock(&queue_locker);

        // Apply the filer
        for (Rect face: current_frame.faces)
        {
            apply_filter(current_frame.frame, face);
        }

        // Write the result
        output_video.write(current_frame.frame);
    }
}

int main(int argc, const char **argv)
{
    // Init the queue locker
    omp_init_lock(&queue_locker);

    // Set up the command line arguments
    const String keys =
        "{@video_in             |      | Input video path}"
        "{@video_out            |      | Output video path}"
        "{@threads_per_block    |      | Number of threads to be launched on each block}"
        "{m                     |      | Convolution matrix size}";
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
    else if(!parser.has("@threads_per_block"))
    {
        cout << "Error: The number of threads per block is required" << endl;
        return -1;
    }

    // Check if the number of pixels to group was given and calculate
    // the new values for the corresponding variables
    if (parser.has("m"))
    {
        convolution_matrix_size_m = parser.get<int>("m");
    }

    threads_per_block_m = parser.get<int>("@threads_per_block");

    load_cascade_classifier();

    // Store the paths for video_in and video_out
    String input_video_path = samples::findFile(parser.get<String>("@video_in"));
    String output_video_path = parser.get<String>("@video_out");

    // Open the given video
    VideoCapture input_video = open_video_capture(input_video_path);

    // Open the video writer to save the result
    VideoWriter output_video = open_video_writer(output_video_path, input_video);

    #pragma omp parallel num_threads(2)
    {
        if (omp_get_thread_num() == 0)
        {
            // This thread read from the pipe
            launch_gpu_process(output_video);
        }
        else
        {
            // This thread write in the pipe.
            process_frames(input_video);
        }
    }

    // Close the input and output files
    input_video.release();
    output_video.release();

    return 0;
}