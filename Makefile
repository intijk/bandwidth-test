all: bmpi bshmem

bshmem: shmem.c option.c
	oshcc shmem.c option.c -o bshmem
	
bmpi: mpi.c option.c
	mpicc mpi.c option.c -o bmpi
