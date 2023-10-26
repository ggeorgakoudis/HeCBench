#-----------------------------------------------------------------------
# This file compiles for OpenMP and MPI hybrid operations using the GNU
# compile chain.

MINIFE_TYPES =  \
        -DMINIFE_SCALAR=double   \
        -DMINIFE_LOCAL_ORDINAL=int      \
        -DMINIFE_GLOBAL_ORDINAL=int \
	-DMINIFE_RESTRICT=__restrict__

MINIFE_MATRIX_TYPE = -DMINIFE_CSR_MATRIX
#MINIFE_MATRIX_TYPE = -DMINIFE_ELL_MATRIX

#-----------------------------------------------------------------------

ARCH   = sm_70
CFLAGS = -v -O3 -g
#	-S -emit-llvm
#\
#	-S -emit-llvm
#-mllvm -regalloc=pbqp
#CFLAGS = -qreport -O3 -fopenmp -mcpu=power8 -mtune=power8

CXXFLAGS = -I. -I../utils -I../fem $(MINIFE_TYPES) -O3 -x cu \
	--offload-arch=$(ARCH) \
	$(MINIFE_MATRIX_TYPE) \
	-g
#            -DMINIFE_DEBUG \
#	-DHAVE_MPI -DMPICH_IGNORE_CXX_SEEK \
#	-DMINIFE_REPORT_RUSAGE

LDFLAGS=-O3
LIBS=-L /usr/tce/packages/cuda/cuda-10.1.243/lib64 -lcudart

# The MPICH_IGNORE_CXX_SEEK macro is required for some mpich versions,
# such as the one on my cygwin machine.

#CXX=mpiicpc
#CC=mpiicc

#CXX=g++
#CC=g++

#CXX=icpc
#CC=icc

CXX=clang++
CC=clang

#CXX=xlC
#CC=xlc

include make_targets
