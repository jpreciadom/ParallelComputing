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

void setup_filter(int convolution_matrix_size_p);
void mat_to_pointers(Mat frame, short *r, short *g, short *b);
void pointers_to_mat(Mat frame, short *r, short *g, short *b);
void apply_filter(Mat frame, Rect face);

#endif