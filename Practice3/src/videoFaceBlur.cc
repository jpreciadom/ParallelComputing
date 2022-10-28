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

// Used to distord the face, by default it takes a matrix of 3x3 pixels and
// replaces their values by the their average value
int convolution_matrix_size = 3;
int pixels_in_convolution_matrix = convolution_matrix_size * convolution_matrix_size;

// Num of threads to apply the filter
int num_of_threads;

void process_frame(Mat frame)
{
    // Use the frame to detect the facen on it
    vector<Rect> faces = detect_faces(frame);

    // Iterate over the detected faces and distort them
    for (Rect face : faces)
    {
        apply_filter(frame, face);
    }
}

int main(int argc, const char **argv)
{
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
        pixels_in_convolution_matrix = convolution_matrix_size * convolution_matrix_size;
    }

    load_cascade_classifier();

    // Store the paths for video_in and video_out
    String input_video_path = samples::findFile(parser.get<String>("@video_in"));
    String output_video_path = parser.get<String>("@video_out");

    // Open the given video
    VideoCapture input_video = open_video_capture(input_video_path);

    // Open the video writer to save the result
    VideoWriter output_video = open_video_writer(output_video_path, input_video);

    // Iterate frame by frame over the original video, process them and write the result
    bool has_next_frame = false;
    do
    {
        Mat current_frame;
        has_next_frame = next_frame(input_video, &current_frame);
        process_frame(current_frame);
        output_video.write(current_frame);
    } while (has_next_frame);

    // Close the input and output files
    input_video.release();
    output_video.release();

    return 0;
}