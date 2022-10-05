#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "opencv4/opencv2/opencv.hpp"
#include "chrono"
#include "omp.h"
#include "iostream"

using namespace cv;
using namespace std;
using namespace std::chrono;

struct thread_organization {
    int first_group;
    int last_group;
};

// Used to distord the face, by default it takes a matrix of 15x15 pixels and
// replaces their values by the their average value
int matrix_size = 15;
int num_of_pixels_to_group = matrix_size * matrix_size;

// Num of threads to apply the filter
int num_of_threads;

// Total execution time expressed in milliseconds
double total_execution_time_ms = 0.0;

CascadeClassifier face_cascade;

// Detect the faces in the frame
vector<Rect> detect_faces(Mat frame)
{
    // Convert the frame to gray scale
    Mat frame_gray;
    cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
    equalizeHist(frame_gray, frame_gray);
    // Detect faces
    vector<Rect> faces;
    face_cascade.detectMultiScale(frame_gray, faces);

    return faces;
}

 struct thread_organization *organize_threads(int num_of_groups) {
    struct thread_organization *threads_organization = (struct thread_organization *)malloc(sizeof(struct thread_organization) * num_of_threads);
    if (threads_organization == NULL)
    {
        perror("Error allocating memory for threads data");
        exit(EXIT_FAILURE);
    }

    int groups_per_thread = num_of_groups / num_of_threads;
    int res = num_of_groups % num_of_threads;
    for (int i = 0; i < num_of_threads; i++)
    {
        int first_group, last_group;
        first_group = i == 0 ? 0 : (threads_organization + i - 1)->last_group + 1;

        last_group = first_group + groups_per_thread - 1;
        last_group += i < res ? 1 : 0;

        (threads_organization + i)->first_group = first_group;
        (threads_organization + i)->last_group = last_group;
    }
    return threads_organization;
 }

void distort_face(Mat frame, Rect face)
{
    auto start_time = high_resolution_clock::now();
    int num_of_groups_x = (face.width + matrix_size - 1) / matrix_size;
    int num_of_groups_y = (face.height + matrix_size - 1) / matrix_size;
    int num_of_groups = num_of_groups_x * num_of_groups_y;

    struct thread_organization *threads_organization = organize_threads(num_of_groups);

    #pragma omp parallel num_threads(num_of_threads)
    {
        int thread_id = omp_get_thread_num();
        int first_group = (threads_organization + thread_id)->first_group;
        int last_group = (threads_organization + thread_id)->last_group;

        for (int group = first_group; group <= last_group; group++) {
            int x = face.x + ((group % num_of_groups_x) * matrix_size);
            int y = face.y + ((group / num_of_groups_y) * matrix_size);

            // Allocate memmory for store the positions of the pixels in the group
            Point2d *pixels_position = (Point2d *)malloc(sizeof(Point2d) * num_of_pixels_to_group);
            if (pixels_position == NULL)
            {
                perror("Error allocating memory for pixels positions");
                exit(EXIT_FAILURE);
            }

            // Get the positions of all pixels in the group
            for (int i = 0; i < num_of_pixels_to_group; i++)
            {
                *(pixels_position + i) = Point(x + (i % matrix_size), y + (int)(i / matrix_size));
            }

            // Calculate the average value of the grouped pixels
            int new_pixels_value[3] = {0, 0, 0};
            for (int i = 0; i < num_of_pixels_to_group; i++)
            {
                Point2d *pixel_position = pixels_position + i;
                Vec3b pixel = frame.at<Vec3b>(pixel_position->y, pixel_position->x);

                new_pixels_value[0] += pixel[0];
                new_pixels_value[1] += pixel[1];
                new_pixels_value[2] += pixel[2];
            }
            new_pixels_value[0] /= num_of_pixels_to_group;
            new_pixels_value[1] /= num_of_pixels_to_group;
            new_pixels_value[2] /= num_of_pixels_to_group;

            // Replace the value of all pixels in the group for the previous one calculated
            for (int i = 0; i < num_of_pixels_to_group; i++)
            {
                Point2d *pixel_position = pixels_position + i;
                Vec3b &pixel = frame.at<Vec3b>(pixel_position->y, pixel_position->x);

                pixel.val[0] = new_pixels_value[0];
                pixel.val[1] = new_pixels_value[1];
                pixel.val[2] = new_pixels_value[2];
            }
        }
    }

    auto finish_time = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(finish_time - start_time);
    total_execution_time_ms += duration.count();
}

void process_frame(Mat frame)
{
    // Use the frame to detect the facen on it
    vector<Rect> faces = detect_faces(frame);

    // Iterate over the detected faces and distort them
    for (Rect face : faces)
    {
        distort_face( frame, face );
    }
}

int main(int argc, const char **argv)
{
    // Set up the command line arguments
    const String keys =
        "{@video_in       |      | Input video path}"
        "{@video_out      |      | Output video path}"
        "{@num_threads    |      | Number of threads to apply the filter}"
        "{pixels-to-group |      | Number of pixels to distort the face}";
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
    if (parser.has("pixels-to-group"))
    {
        matrix_size = parser.get<int>("pixels-to-group");
        num_of_pixels_to_group = matrix_size * matrix_size;
    }

    // Load the haarcascade file to detect the faces
    if (!face_cascade.load("data/haarcascade_frontalface_alt.xml"))
    {
        cout << "Error loading face cascade" << endl;
        return -1;
    }

    // Store the paths for video_in and video_out
    String video_in = samples::findFile(parser.get<String>("@video_in"));
    String video_out = parser.get<String>("@video_out");

    // Open the given video
    VideoCapture video;
    video.open(video_in);
    if (!video.isOpened())
    {
        cout << "Error opening the video" << endl;
        return -1;
    }

    // Open the video writer to save the result
    VideoWriter writer;
    int codec = VideoWriter::fourcc('a', 'v', 'c', '1');
    double fps = video.get(CAP_PROP_FPS);
    int frame_width = video.get(cv::CAP_PROP_FRAME_WIDTH);
    int frame_height = video.get(cv::CAP_PROP_FRAME_HEIGHT);
    writer.open(video_out, codec, fps, Size(frame_width, frame_height), true);
    if (!writer.isOpened())
    {
        cout << "Error opening the video writer" << endl;
        return -1;
    }

    // Iterate frame by frame over the original video, process them and write the result
    Mat frame;
    while (video.read(frame))
    {
        if (frame.empty())
        {
            break;
        }
        else
        {
            process_frame(frame);
            writer.write(frame);
        }
    }

    // Close the input and output files
    video.release();
    writer.release();

    cout << "The video was processed in " << total_execution_time_ms / 1000 << " seconds" << endl;
    return 0;
}