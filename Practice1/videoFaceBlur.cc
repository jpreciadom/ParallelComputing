#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "opencv4/opencv2/opencv.hpp"
#include "iostream"

using namespace std;
using namespace cv;

int matrix_size = 15;
int num_of_pixels_to_group = matrix_size * matrix_size;

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

// void distort_face(Mat frame, Rect face)
// {
//     int num_of_pixels = (face.width + 1) * (face.height + 1);
//     struct pixel_to_replace *pixels_to_replace = (struct pixel_to_replace *)malloc(sizeof(struct pixel_to_replace) * num_of_pixels);
//     if (pixels_to_replace == NULL)
//     {
//         perror("Could not allocate memory to store the pixels");
//         exit(EXIT_FAILURE);
//     }

//     int max_x = face.x + face.width;
//     int max_y = face.y + face.height;
//     int index = 0;
//     for (int x = face.x; x <= max_x; x++)
//     {
//         for (int y = face.y; y <= max_y; y++)
//         {
//             int new_b = 0;
//             int new_g = 0;
//             int new_r = 0;
//             for (int i = 0; i < distort_size; i++)
//             {
//                 int x_i = x - fix_position + (i % matrix_size), y_i = y - fix_position + (int)(i / matrix_size);
//                 Vec3b pixel = frame.at<Vec3b>(y_i, x_i);
//                 new_b += pixel.val[0];
//                 new_g += pixel.val[1];
//                 new_r += pixel.val[2];
//             }
//             Vec3b new_pixel = Vec3b(new_b / distort_size, new_g / distort_size, new_r / distort_size);
//             *(pixels_to_replace + index) = { Point2d(x, y), new_pixel };
//             index++;
//         }
//     }

//     for (index = 0; index < num_of_pixels; index++)
//     {
//         struct pixel_to_replace to_replace = *(pixels_to_replace + index);
//         Point2d position = to_replace.position;
//         Vec3b new_pixel = to_replace.pixel;

//         Vec3b &old_pixel = frame.at<Vec3b>(position.y, position.x);
//         old_pixel.val[0] = new_pixel.val[0];
//         old_pixel.val[1] = new_pixel.val[1];
//         old_pixel.val[2] = new_pixel.val[2];
//     }
// }

void distort_face(Mat frame, Rect face)
{
    int max_x = face.x + face.width;
    int max_y = face.y + face.height;
    for (int x = face.x; x <= max_x; x+=matrix_size)
    {
        for (int y = face.y; y <= max_y; y+=matrix_size)
        {
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

    return 0;
}