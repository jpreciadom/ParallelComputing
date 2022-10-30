CC = g++
work_dir = src/

all: $(work_dir)videoFaceBlur.cc
			@ $(CC) $(work_dir)videoFaceBlur.cc -o videoFaceBlur.exe \
				-lopencv_objdetect \
				-lopencv_highgui \
				-lopencv_imgproc \
				-lopencv_videoio \
				-lopencv_core \
				-fopenmp \
				-I$(work_dir)/video_processor \
				$(work_dir)/video_processor/video_processor.cc \
				-I$(work_dir)/filter \
				$(work_dir)/filter/filter.cc
			@ echo "The code has been compiled successfully"

clean:
			@ rm -rf out_videos
			@ mkdir out_videos