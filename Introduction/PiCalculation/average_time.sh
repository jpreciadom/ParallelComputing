#!/bin/bash

function take_time() {
  program_name=$1
  num_of_threads=$2

  logs_file=$program_name'.logs'
  echo 'Running for '$num_of_threads' threads' >> $logs_file

  let total_time=0
  for (( i=0; i<$total_iterations; i++ ))
  do
    result=$((time ./$program_name.exe $thread) 2>&1)
    result=$(echo ${result##*real})
    result=$(echo ${result%%user*})
    
    result_length=$(echo ${#result})
    m_index=$(expr index $result "m")
    s_index=$(expr index $result "s")

    minutes=${result:0:m_index-1}
    seconds=${result:m_index:result_length-m_index-1}
    seconds=${seconds/,/.}

    total_time=$(echo "scale = 3; $total_time+($minutes*60)" | bc)
    total_time=$(echo "scale = 3; $total_time+$seconds" | bc)

    echo $result >> $logs_file
  done
  average=$(echo "scale = 3; $total_time/$total_iterations" | bc)

  echo 'For '$thread' threads the average time was '$average's' >> $logs_file
  echo $average
}

result_file_name="execution.result.csv"
rm $result_file_name

# Compile the .c code files
echo 'Compiling programs...'
gcc -pthread pi_posix_fs.c -o pi_posix_fs.exe
gcc -pthread pi_posix.c -o pi_posix.exe
gcc -fopenmp pi_omp_fs.c -o pi_omp_fs.exe
gcc -fopenmp pi_omp.c -o pi_omp.exe
echo "Programs copiled!"\n

num_of_threads=(1 2 4 8 16 24 32 40 48 56 64 72 80 128 256 512 1024)
programs=(pi_posix_fs pi_posix pi_omp_fs pi_omp)

let total_iterations=$1

# Write in result file
result_file_line='Num of threads'
for program in ${programs[@]}
do
  result_file_line+=';'$program

  echo 'The program will be executed '$total_iterations' times' > $program.logs
  echo '' >> $program.logs
done
echo $result_file_line >> $result_file_name

for thread in ${num_of_threads[@]}
do
  echo 'Running for' $thread ' threads...'
  result_file_line=$thread
  for program in ${programs[@]}
  do
    echo '  Running '$program'...'
    average=$(take_time $program $thread)
    result_file_line+=';'$average
  done

  # Write in result file
  echo $result_file_line >> $result_file_name
done
