OMP = g++
CUDA = nvcc
OMPI = mpic++

work_dir = src/

all:
	@ make open_mp
	@ make cuda
	@ make open_mpi
	@ echo "All code have been compiled!"

open_mpi: $(work_dir)/openmpi/openmpi_index.cc
	@ $(OMPI) $(work_dir)/openmpi/openmpi_index.cc \
		-o $(work_dir)/openmpi/openmpi.exe \
		-fopenmp \
		-I$(work_dir)/utils/matrix/matrix \
		$(work_dir)/utils/matrix/matrix.cc
	@ echo "OpenMPI code have been compiled!"

cuda: $(work_dir)/cuda/cuda_index.cu
	@ $(CUDA) $(work_dir)/cuda/cuda_index.cu \
		-o $(work_dir)/cuda/cuda.exe \
		-Xcompiler -fopenmp \
		-I$(work_dir)/utils/matrix/matrix \
		$(work_dir)/utils/matrix/matrix.cc
	@ echo "Cuda code have been compiled!"

open_mp: $(work_dir)/openmp/openmp_index.cc
	@ $(OMP) $(work_dir)/openmp/openmp_index.cc \
		-o $(work_dir)/openmp/openmp.exe \
		-fopenmp \
		-I$(work_dir)/utils/matrix/matrix \
		$(work_dir)/utils/matrix/matrix.cc
	@ echo "OpenMP code have been compiled!"
