#!/bin/bash

let total_iterations=$1

function take_time()
{
    program_execution_command=$1
    
    let total_time=0
    for (( i=0; i<$total_iterations; i++ ))
    do
        result=$($program_execution_command)
        result=$(echo ${result##*processed in})
        result=$(echo ${result%%seconds*})
        
        total_time=$(echo "scale = 3; $total_time+$result" | bc)
        echo $result's'
    done
    average_time=$(echo "scale = 3; $total_time/$total_iterations" | bc)
    echo $average_time
}

function create_openmp_execution()
{
    local threads=$1
    local matrix_size=$2
    
    echo 'src/openmp/openmp_index.exe' $threads $matrix_size
}

function create_cuda_execution()
{
    local threads=$1
    local matrix_size=$2
    
    echo 'src/cuda/cuda_index.exe' $threads $threads $matrix_size
}

function create_openmpi_execution()
{
    local nodes=$1
    local matrix_size=$2
    
    echo 'mpirun -np' $nodes 'src/openmpi/openmpi_index.exe' $matrix_size
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
echo 'Num of threads;Average time' > $result_file_name
local N=(1 2 4 8 16 32 64 128 256 512 1024 2048)
for $matrix_size in ${N[@]}
do
    local result_file_line=$matrix_size
    local execution_command
    local time_result
    
    # Execute open_mp code
    execution_command=$(create_openmp_execution 16 $N)
    time_result=$(take_time $execution_command)
    result_file_line+=';'$time_result

    # Execute cuda code
    execution_command=$(create_cuda_execution 2048 $N)
    time_result=$(take_time $execution_command)
    result_file_line+=';'$time_result

    # Execute open_mpi
    execution_command=$(create_openmpi_execution 8 $N)
    time_result=$(take_time $execution_command)
    result_file_line+=';'$time_result

    # Save the result into the .csv file
    echo $result_file_line >> $result_file_name
done

echo >> $result_file_name

# Get Algorithms times
N=(1024 2048 1024)
local threads_group=((1 2 4 8 16 32 64) (1 2 4 8 16 64 128 256 512 1024 2048) (1 2 3 4 5 6 7 8))
for i in ${!threads_group[@]}
do
  echo 'Num of threads;Time' >> $result_file_name
  local matrix_size=${N[$i]}
  local execution_command

  for $threads in ${threads_group[$i]}
  do
    local result_file_line=$threads
    local time_result

    if (( i == 0 ))
    then
      execution_command=$(create_openmp_execution $threads $matrix_size)
    elif (( i == 1 ))
    then
      execution_command=$(create_cuda_execution $threads $matrix_size)
    else
      execution_command=$(create_openmpi_execution $threads $matrix_size)
    fi

    time_result=$(take_time $execution_command)
    result_file_line+=';'$time_result

    echo $time_result >> result_file_name
  done
  echo >> result_file_name
done
