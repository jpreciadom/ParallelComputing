CC = g++

all: videoFaceBlur.cc
			@ $(CC) videoFaceBlur.cc -o videoFaceBlur.exe -lopencv_objdetect -lopencv_highgui -lopencv_imgproc -lopencv_videoio -lopencv_core
			@ echo "The code has been compiled successfully"

clean:
			@ rm -rf out_videos
			@ mkdir out_videos