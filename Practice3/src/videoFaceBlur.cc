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

// Used to distord the face, by default it takes a matrix of 3x3 pixels and
// replaces their values by the their average value
int convolution_matrix_size = 3;

// Num of threads to apply the filter
int num_of_threads;

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
        frames.push({ current_frame, faces });
        omp_unset_lock(&queue_locker);
    }

    are_remaining_frames = false;
}

// Read the frame and its faces from the pipe, setup the environment
// and lauch the process in the GPU
void launch_gpu_process(VideoWriter output_video)
{
    setup_filter(pixels_in_convolution_matrix);
    while (are_remaining_frames)
    {
        // Verify if there are frames pending for being processed
        if (frames.size() == 0) continue;

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
        "{@video_in       |      | Input video path}"
        "{@video_out      |      | Output video path}"
        "{@num_threads    |      | Number of threads to apply the filter}"
        "{m               |      | Convolution matrix size}";
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
    else if (!parser.has("@num_threads"))
    {
        cout << "Error: The number of threads is required" << endl;
        return -1;
    }

    // Get the num of threads from parameters
    num_of_threads = parser.get<int>("@num_threads");

    // Check if the number of pixels to group was given and calculate
    // the new values for the corresponding variables
    if (parser.has("m"))
    {
        convolution_matrix_size = parser.get<int>("m");
    }

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