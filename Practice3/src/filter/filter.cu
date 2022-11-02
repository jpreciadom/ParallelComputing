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
int pixels_in_convolution_matrix;

// Num of block and threads per block to use in the GPU
int threads_per_block;

// Blocks of the image than will be processed on each MP
int extended_factor;
int image_block_width = 8, image_block_height = 8, image_block_area = 64;
int extended_image_block_width, extended_image_block_height, extended_image_block_area;

void setup_filter(int convolution_matrix_size_p, int threads_per_block_p)
{
  convolution_matrix_size = convolution_matrix_size_p;
  pixels_in_convolution_matrix = convolution_matrix_size_p * convolution_matrix_size_p;
  extended_factor = convolution_matrix_size / 2;

  // --------------------------------------------------------------------- //
  threads_per_block = threads_per_block_p;

  // --------------------------------------------------------------------- //
  extended_image_block_width = image_block_width + convolution_matrix_size - 1;
  extended_image_block_height = image_block_height + convolution_matrix_size - 1;
  extended_image_block_area = extended_image_block_width * extended_image_block_height;
}

// Available local memory 24 bytes
// Available shared memory
__global__ void distort_face(short *r, short *g, short *b, int *additional_data)
{
  __shared__ int
    blocks_per_row,
    pixels_in_convolution_matrix,
    extended_factor,
    image_block_width,
    image_block_height,
    extended_image_width;

  if (threadIdx.x == 0)
  {
    blocks_per_row = *(additional_data);
    pixels_in_convolution_matrix = *(additional_data + 1);
    extended_factor = *(additional_data + 2);
    image_block_width = *(additional_data + 6);
    image_block_height = *(additional_data + 7);
    extended_image_width = *(additional_data + 12);
  }
  __syncthreads();

  int x = ((blockIdx.x % blocks_per_row) * image_block_width) + (threadIdx.x % image_block_width);
  int y = ((blockIdx.x / blocks_per_row) * image_block_height) + (threadIdx.x / image_block_height);

  int reg_r = 0, reg_g = 0, reg_b = 0, index = 0;

  for (int reg_y = y + extended_factor * 2; reg_y >= y; reg_y--)
  {
    for (int reg_x = x + extended_factor * 2; reg_x >= x; reg_x--)
    {
      index = reg_y * extended_image_width + reg_x;
      reg_r += *(r + index);
      reg_g += *(g + index);
      reg_b += *(b + index);
    }
  }

  x += extended_factor;
  y += extended_factor;
  index = y * extended_image_width + x;

  *(r + index) = reg_r / pixels_in_convolution_matrix;
  *(g + index) = reg_g / pixels_in_convolution_matrix;
  *(b + index) = reg_b / pixels_in_convolution_matrix;
}

// Get the data from the Frame and copy it into the r, g, b arrays
void mat_to_pointers(Mat frame, Rect extended_face, short *r, short *g, short *b)
{
  int pointer = 0;
  for (int y = 0; y < extended_face.height; y++)
  {
    for (int x = 0; x < extended_face.width; x++)
    {
      Vec3b pixel = frame.at<Vec3b>(y + extended_face.y, x + extended_face.x);
      *(r + pointer) = pixel[0];
      *(g + pointer) = pixel[1];
      *(b + pointer) = pixel[2];
      pointer++;
    }
  }
}

// Get the data from the r, g, b arrays and copy it into the Frame
void pointers_to_mat(Mat frame, Rect extended_face, short *r, short *g, short *b)
{
  int pointer = 0;
  for (int y = 0; y < extended_face.height; y++)
  {
    for (int x = 0; x < extended_face.width; x++)
    {
      Vec3b &pixel = frame.at<Vec3b>(y + extended_face.y, x + extended_face.x);
      pixel[0] = *(r + pointer);
      pixel[1] = *(g + pointer);
      pixel[2] = *(b + pointer);
      pointer++;
    }
  }
}

void apply_filter(Mat frame, Rect face)
{
  cudaError_t cuda_err = cudaSuccess;

  // Calculate the face dimenssions according to detected and returned in face
  Rect fixed_face = Rect(
    face.x,
    face.y,
    face.width + (image_block_width - (face.width % image_block_width)),
    face.height + (image_block_height - (face.height % image_block_height))
  );
     
  // Calculate the extended face dimensions having into accound the pixels
  // required to apply the convolution matrix
  Rect extended_face = Rect(
    fixed_face.x - extended_factor,
    fixed_face.y - extended_factor,
    fixed_face.width + convolution_matrix_size - 1,
    fixed_face.height + convolution_matrix_size - 1
  );

  int blocks_per_row = fixed_face.width / image_block_width;

  // Size of memory in bytes to allocate in both device and main memmories
  int memory_to_allocate = extended_face.area() * sizeof(short);
  int additional_data_to_allocate = sizeof(int) * 15;

  // --------------------------------------------------------------------- //
  // Data setup in main memory
  short *r, *g, *b;
  int *additional_data;

  r = (short *)malloc(memory_to_allocate);
  g = (short *)malloc(memory_to_allocate);
  b = (short *)malloc(memory_to_allocate);
  additional_data = (int *)malloc(additional_data_to_allocate);

  // Check the result of malloc call
  if (r == NULL || g == NULL || b == NULL || additional_data == NULL)
  {
    perror("Error allocationg memory for rbg in the main memory");
    exit(-1);
  }

  // Copy the data from the Frame into the r, g, b arrays
  mat_to_pointers(frame, extended_face, r, g, b);

  // Copy the data into Block dimensions
  *additional_data = blocks_per_row;
  *(additional_data + 1) = pixels_in_convolution_matrix;
  *(additional_data + 2) = extended_factor;

  *(additional_data + 3) = extended_image_block_width;
  *(additional_data + 4) = extended_image_block_height;
  *(additional_data + 5) = extended_image_block_area;

  *(additional_data + 6) = image_block_width;
  *(additional_data + 7) = image_block_height;
  *(additional_data + 8) = image_block_area;

  *(additional_data + 9) = fixed_face.width;
  *(additional_data + 10) = fixed_face.height;
  *(additional_data + 11) = fixed_face.area();

  *(additional_data + 12) = extended_face.width;
  *(additional_data + 13) = extended_face.height;
  *(additional_data + 14) = extended_face.area();

  // --------------------------------------------------------------------- //
  // Data setup in device memory
  short *device_r, *device_g, *device_b;
  int *device_additional_data;

  cuda_err = cudaMalloc((void**)&device_r, memory_to_allocate);
  cuda_err = cudaMalloc((void**)&device_g, memory_to_allocate);
  cuda_err = cudaMalloc((void**)&device_b, memory_to_allocate);
  cuda_err = cudaMalloc((void**)&device_additional_data, additional_data_to_allocate);

  // Copy the data to the device memory
  cuda_err = cudaMemcpy(device_r, r, memory_to_allocate, cudaMemcpyHostToDevice);
  cuda_err = cudaMemcpy(device_g, g, memory_to_allocate, cudaMemcpyHostToDevice);
  cuda_err = cudaMemcpy(device_b, b, memory_to_allocate, cudaMemcpyHostToDevice);
  cuda_err = cudaMemcpy(device_additional_data, additional_data, additional_data_to_allocate, cudaMemcpyHostToDevice);

  // --------------------------------------------------------------------- //
  // Lauch the kernel (Apply the filter)
  distort_face<<<fixed_face.area() / image_block_area, threads_per_block>>>(
    device_r,
    device_g,
    device_b,
    device_additional_data
  );

  // Verify the execution
  cuda_err = cudaGetLastError();
  if (cuda_err)
  {
    perror("Failed to launch distort face kernel");
    exit(EXIT_FAILURE);
  }
  cudaDeviceSynchronize();

  // --------------------------------------------------------------------- //
  // Copy the results into the main memory
  cudaMemcpy(r, device_r, memory_to_allocate, cudaMemcpyDeviceToHost);
  cudaMemcpy(g, device_g, memory_to_allocate, cudaMemcpyDeviceToHost);
  cudaMemcpy(b, device_b, memory_to_allocate, cudaMemcpyDeviceToHost);
  cudaMemcpy(additional_data, device_additional_data, additional_data_to_allocate, cudaMemcpyDeviceToHost);

  // Free the device memory
  cudaFree(device_r);
  cudaFree(device_g);
  cudaFree(device_b);
  cudaFree(device_additional_data);

  // --------------------------------------------------------------------- //
  // Copy the results into the frame
  pointers_to_mat(frame, extended_face, r, g, b);

  // Free the main memory
  free(r);
  free(g);
  free(b);
  free(additional_data);
}