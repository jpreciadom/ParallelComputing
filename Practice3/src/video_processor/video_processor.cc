#include "opencv4/opencv2/objdetect.hpp"
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#include "opencv4/opencv2/videoio.hpp"
#include "opencv4/opencv2/core/utility.hpp"
#include "opencv4/opencv2/opencv.hpp"

using namespace std;
using namespace cv;

CascadeClassifier face_cascade;

// Load the haarcascade file to detect the faces
void load_cascade_classifier()
{
  if (!face_cascade.load("src/models/haarcascade_frontalface_alt.xml"))
  {
    perror("Error loading face cascade");
    exit(-1);
  }
}

// Open a video using the given path
VideoCapture open_video_capture(String video_path)
{
  VideoCapture video;
  video.open(video_path);

  // Finish the program if there is an error
  // and return the video reader in other case
  if (!video.isOpened())
  {
    perror("Error opening the video");
    exit(-1);
  }
  else
  {
    return video;
  }
}

// Open a OpenCV Video Writer used to stored the result
VideoWriter open_video_writer(String out_path, VideoCapture input_video)
{
  VideoWriter writer;
  int codec = VideoWriter::fourcc('a', 'v', 'c', '1');
  double fps = input_video.get(CAP_PROP_FPS);
  int frame_width = input_video.get(CAP_PROP_FRAME_WIDTH);
  int frame_height = input_video.get(CAP_PROP_FRAME_HEIGHT);
  writer.open(out_path, codec, fps, Size(frame_width, frame_height), true);

  // Finish the program if there is an error
  // and return the video writer in other case
  if (!writer.isOpened())
  {
    perror("Error opening the video writer");
    exit(-1);
  }
  else
  {
    return writer;
  }
}

// Read the next frame of a input video
bool next_frame(VideoCapture input_video, Mat *frame)
{
  input_video.read(*frame);

  // Return FALSE if there are not more frames
  // and TRUE in other case
  return !(*frame).empty();
}

// Detect and return the faces in a given frame
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