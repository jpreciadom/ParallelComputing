#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "opencv4/opencv2/opencv.hpp"
#include "chrono"
#include "cmath"

using namespace std;
using namespace cv;
using namespace std::chrono;

// Used to distord the face
int convolution_matrix_size;
int pixels_in_convolution_matrix;

void setup_filter(int convolution_matrix_size_p)
{
  convolution_matrix_size = convolution_matrix_size_p;
  pixels_in_convolution_matrix = convolution_matrix_size_p * convolution_matrix_size_p;
}

// Get the data from the Frame and copy it into the r, g, b arrays
void mat_to_pointers(Mat frame, short *r, short *g, short *b)
{
  int pointer = 0;
  for (int y = 0; y < frame.rows; y++)
  {
    for (int x = 0; x < frame.cols; x++)
    {
      Vec3b pixel = frame.at<Vec3b>(y, x);
      *(r + pointer) = pixel[0];
      *(g + pointer) = pixel[1];
      *(b + pointer) = pixel[2];
      pointer++;
    }
  }
}

// Get the data from the r, g, b arrays and copy it into the Frame
void pointers_to_mat(Mat frame, short *r, short *g, short *b)
{
  int pointer = 0;
  for (int y = 0; y < frame.rows; y++)
  {
    for (int x = 0; x < frame.cols; x++)
    {
      Vec3b &pixel = frame.at<Vec3b>(y, x);
      pixel[0] = *(r + pointer);
      pixel[1] = *(g + pointer);
      pixel[2] = *(b + pointer);
      pointer++;
    }
  }
}

double apply_filter(Mat frame, Rect face)
{

}