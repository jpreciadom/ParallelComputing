#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "opencv4/opencv2/opencv.hpp"

using namespace std;
using namespace cv;

// Used to distord the face
int convolution_matrix_size;
int pixels_in_convolution_matrix

void setup_filter(int matrix_size)
{
  convolution_matrix_size = matrix_size;
  pixels_in_convolution_matrix = matrix_size * matrix_size;
}

void apply_filter(Mat frame, Rect face)
{
  
}