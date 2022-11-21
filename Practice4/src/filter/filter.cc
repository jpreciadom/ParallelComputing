#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "opencv4/opencv2/opencv.hpp"
#include "omp.h"
#include "cmath"

using namespace std;
using namespace cv;
using namespace std::chrono;

// Used to distord the face
int convolution_matrix_size;
int pixels_in_convolution_matrix;
int displacement;

void setup_filter(int convolution_matrix_size_p)
{
  convolution_matrix_size = convolution_matrix_size_p;
  pixels_in_convolution_matrix = convolution_matrix_size_p * convolution_matrix_size_p;
  displacement = convolution_matrix_size / 2;
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

// Apply the filter to the given frame at the given face
void apply_filter(Mat frame, Rect face)
{
  Mat reg_frame = Mat(face.height, face.width, CV_8UC3);

  #pragma omp parallel num_threads(1)
  {
    // Get the current thread Id and the total amoung of threads
    int thread_id = omp_get_thread_num();
    int num_of_threads = omp_get_num_threads();
    // Calculate the amoung of columns each thread must process
    double cols_by_thread = (double)(face.width / num_of_threads);

    // Calculate the thread operation limits
    double lower_bound = thread_id * cols_by_thread;
    double upper_bound = lower_bound + cols_by_thread;

    // Iterate over the face columns
    for ( int y = (int)lower_bound; y < upper_bound; y++ )
    {
      // Iterate over the face rows
      for ( int x = 0; x < face.width; x++)
      {
        // Store the average RGB factor using the convolution matrix
        int average_r = 0, average_g = 0, average_b = 0;
        // Iterate over the pixels around the selected one and calculate the sum of their RGB factors
        for (int dy = y - displacement; dy <= displacement; dy++)
        {
          for (int dx = x - displacement; dx <= displacement; dx++)
          {
            Vec3b pixel = frame.at<Vec3b>(face.y + dy, face.x + dx);
            average_r += pixel[0];
            average_g += pixel[1];
            average_b += pixel[2];
          }
        }

        // Asigng the average RGB factor to the selected pixel into the register Mat
        Vec3b &pixel = frame.at<Vec3b>(y, x);
        pixel[0] = average_r / pixels_in_convolution_matrix;
        pixel[1] = average_g / pixels_in_convolution_matrix;
        pixel[2] = average_b / pixels_in_convolution_matrix;
      }
    }

    #pragma omp barrier

    // Copy the values from register matriz to the original frame
    for ( int y = (int)lower_bound; y < upper_bound; y++ )
    {
      for ( int x = 0; x < face.width; x++)
      {
        Vec3b &pixel = frame.at<Vec3b>(face.y + y, face.x + x);
        Vec3b reg_pixel = reg_frame.at<Vec3b>(y, x);

        pixel[0] = reg_pixel[0];
        pixel[1] = reg_pixel[1];
        pixel[2] = reg_pixel[2];
      }
    }
  }
}