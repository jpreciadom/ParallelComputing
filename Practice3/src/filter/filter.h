#ifndef FILTERH
#define FILTERH

#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "opencv4/opencv2/opencv.hpp"

using namespace std;
using namespace cv;

void setup_filter(int convolution_matrix_size_p, int threads_per_block_p);
double apply_filter(Mat frame, Rect face);

#endif