#!/bin/bash

result_file_name="execution_result.csv"
logs_file_name="logs"
rm $result_file_name $logs_file_name

echo 'Compiling program...' >> $logs_file_name
gcc pi.c -o pi.exe -pthread
echo 'Program copiled!' >> $logs_file_name
echo >> $logs_file_name

num_of_threads=(1 2 4 8 16 24 32 40 48 56 64 72 80 128 256 512 1024)
let total_iterations=$1
echo 'The program will be excecuted' $total_iterations 'times' >> $logs_file_name
echo >> $logs_file_name

# Write in result file
echo 'Threads;Time' >> $result_file_name

for thread in ${num_of_threads[@]}
do
  echo 'Excecuting for '$thread' threads:' >> $logs_file_name
  let total_time=0
  for (( i=0; i<$total_iterations; i++ ))
  do
    result=$((time ./pi.exe $thread) 2>&1)
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
    echo $result >> $logs_file_name
  done
  average=$(echo "scale = 3; $total_time/$total_iterations" | bc)
  echo 'For '$thread' threads the average time was '$average's' >> $logs_file_name
  echo >> $logs_file_name

  # Write in result file
  echo $thread';'${average/./,} >> $result_file_name
done
