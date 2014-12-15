all: bmpi bshmem

bshmem: shmem.c option.c
	oshcc shmem.c option.c -o bshmem -O0
	
bmpi: mpi.c option.c
	mpicc mpi.c option.c -o bmpi -O0

clean:
	rm -rf bshmem bmpi
