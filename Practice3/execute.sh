#!/bin/bash

let total_iterations=$1
video_in_path=$2
video_out_path=$3

function take_time() {
  num_of_threads=$1
  logs_file_name=$2

  let total_time=0
  for (( i=0; i<$total_iterations; i++ ))
  do
    result=$((time ./videoFaceBlur.exe $video_in_path $video_out_path $thread) 2>&1)
    result=$(echo ${result##*processed in})
    result=$(echo ${result%%seconds*})

    total_time=$(echo "scale = 3; $total_time+$result" | bc)
    echo $result's'
  done
  average=$(echo "scale = 3; $total_time/$total_iterations" | bc)

  echo 'For '$thread' threads the average time was '$average's'
  echo ''
}

# Compile the code
echo 'Compiling programs...'
make all
echo "Programs copiled!"
echo

# Clean out files
echo 'Cleaning outputs programs...'
make clean
echo "Outputs cleaned!"
echo

# Set up .csv out file
result_file_name="execution.result.csv"
echo 'Num of threads;Average time' > $result_file_name

num_of_threads=(1 2 3 4 5 6 7 8 16 24 32 64 68 128 256 512 1024)
for thread in ${num_of_threads[@]}
do
  echo 'Running for' $thread ' threads...'
  result_file_line=$thread
  take_time $thread $result_file_name
  result_file_line+=';'${average//./,}

  # Write in result file
  echo $result_file_line >> $result_file_name
done
