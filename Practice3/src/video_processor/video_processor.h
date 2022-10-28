#ifndef VIDEOPROCESSORH
#define VIDEOPROCESSORH

#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "opencv4/opencv2/opencv.hpp"

using namespace std;
using namespace cv;

void load_cascade_classifier();
VideoCapture open_video_capture(String video_path);
VideoWriter open_video_writer(String out_path, VideoCapture input_video);
bool next_frame(VideoCapture input_video, Mat *frame);
vector<Rect> detect_faces(Mat frame);

#endif