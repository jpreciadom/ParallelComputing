#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "iostream"

using namespace std;
using namespace cv;

CascadeClassifier face_cascade;

vector<Rect> detect_faces( Mat frame )
{
    Mat frame_gray;
    cvtColor( frame, frame_gray, COLOR_BGR2GRAY );
    equalizeHist( frame_gray, frame_gray );
    //-- Detect faces
    std::vector<Rect> faces;
    face_cascade.detectMultiScale( frame_gray, faces );

    return faces;
}

int main(int argc, const char **argv)
{
    const String keys =
        "{@video_in |      | Input video path}"
        "{@video_out|      | Output video path}";
    CommandLineParser parser(argc, argv, keys);

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

    if (!face_cascade.load("data/haarcascade_frontalface_alt.xml"))
    {
        cout << "Error loading face cascade" << endl;
        return -1;
    }

    String video_in = samples::findFile(parser.get<String>("@video_in"));
    VideoCapture video;
    video.open(video_in);
    if (!video.isOpened())
    {
        cout << "Error opening the video" << endl;
        return -1;
    }

    Mat frame;
    while (video.read(frame))
    {
        if (frame.empty())
        {
            break;
        }
    }
}