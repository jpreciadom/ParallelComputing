OMP = g++
CUDA = nvcc
OMPI = mpic++

work_dir = src/

open_mp: $(work_dir)/openmp/openmp_index.cc
	@ $(OMP) $(work_dir)/openmp/openmp_index.cc \
		-o $(work_dir)/openmp/openmp_index.exe \
		-fopenmp \
		-I$(work_dir)/utils/matrix/matrix \
		$(work_dir)/utils/matrix/matrix.cc
	@ echo "OpenMP code have been compiled!"
