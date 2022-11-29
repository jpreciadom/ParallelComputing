#!/bin/bash

let total_iterations=$1

function take_time()
{
    local program=$1
    local threads=$2
    local matrix_size=$3
    local command=''

    if [[ $program = "openmp" ]]
    then
      command='src/openmp/openmp.exe '$threads' '$matrix_size
    elif [[ $program = "cuda" ]]
    then
      command='src/cuda/cuda.exe '$threads' '$threads' '$matrix_size
    else
      command='mpirun -np '$threads' src/openmpi/openmpi.exe '$matrix_size
    fi

    let total_time=0
    for (( i=0; i<$total_iterations; i++ ))
    do
        result=$($command)
        result=$(echo ${result##*multiplied in })
        result=$(echo ${result%% seconds*})
        
        total_time=$(echo "scale = 5; $total_time+$result" | bc)
    done
    average_time=$(echo "scale = 5; $total_time/$total_iterations" | bc)
    echo $average_time
}

# Compile the code
echo 'Compiling programs...'
make open_mp
make cuda
make open_mpi
echo "Programs copiled!"
echo

# Set up .csv out file
result_file_name="execution.result.csv"

# Compare by varying the matrix size
echo "Comparing algorithms performance by varying the matrix size..."
echo 'Matrix size;OpenMP;Cuda;OpenMPI' > $result_file_name
N=(1 2 4 8 16 32 64 128 256 512 1024 1536)
for matrix_size in ${N[@]}
do
    result_file_line=$matrix_size
    time_result=''
    
    # Execute open_mp code
    echo "Processing OpenMP:"
    echo "Threads: 16"
    echo "Matrix size: $matrix_size"
    echo
    time_result=$(take_time openmp 16 $matrix_size)
    result_file_line+=';'$time_result

    # Execute cuda code
    echo "Processing CUDA:"
    echo "Threads: 1024"
    echo "Matrix size: $matrix_size"
    echo
    time_result=$(take_time cuda 1024 $matrix_size)
    result_file_line+=';'$time_result

    # Execute open_mpi
    echo "Processing OpenMPI:"
    echo "Threads: 8"
    echo "Matrix size: $matrix_size"
    echo
    time_result=$(take_time openmpi 8 $matrix_size)
    result_file_line+=';'$time_result

    # Save the result into the .csv file
    echo $result_file_line >> $result_file_name
done
echo

echo >> $result_file_name

# Get Algorithms times
echo "Taking algorithms performance by varying the number of threads..."
echo
declare -a openmp_threads=(1 2 4 8 16 32 64)
declare -a cuda_threads=(1 16 64 128 256 512 1024)
declare -a openmpi_threads=(1 2 3 4 5 6 7 8)
declare -a threads_groups=("openmp_threads" "cuda_threads" "openmpi_threads")
N=(1024 1024 1024)
for i in ${!N[@]}
do
  matrix_size=${N[$i]}
  threads_group="${threads_groups[$i]}"
  lst="$threads_group[@]"

  program=''

  if (( i == 0 ))
  then
    program='openmp'
  elif (( i == 1 ))
  then
    program='cuda'
  else
    program='openmpi'
  fi

  echo $program >> $result_file_name
  echo 'Num of threads;Time' >> $result_file_name

  for threads in "${!lst}"
  do
    echo "Executing $program using $threads threads and $matrix_size for matrix size"
    result_file_line=$threads
    time_result=''

    time_result=$(take_time $program $threads $matrix_size)
    result_file_line+=';'$time_result

    echo $result_file_line >> $result_file_name
  done
  echo >> $result_file_name
  echo
done
