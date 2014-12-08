#include <shmem.h>
#include <stdio.h>
#include <string.h>
#include "option.h"

int main(int argc, char *argv[])
{
	size_t block_size;
	char *topo;
	int debug;

	int ret=parse_option(argc,argv,&block_size,&topo,&debug);
	if(ret!=0) return ret;

	start_pes(0);
	int npes=shmem_n_pes();
	int me=shmem_my_pe();

	
	void *block=shmalloc(block_size);
	memset(block,0,block_size);
	
	/* pre process of the block content */	

	
	struct timeval start_time, end_time;

	int target;
	if(strcmp(topo,"ring")==0){
		target=(me+1)%npes;
	}else if(strcmp(topo,"pair")==0){
		target=(me%2==0?(me+npes+1)%npes:(me+npes-1)%npes);
	}else{
		fprintf(stderr, "Unknow topology %s\n", topo);
		exit(-1);
	}


	shmem_barrier_all();		
	gettimeofday(&start_time, NULL);	

	shmem_putmem(block,block,block_size,target);

	shmem_barrier_all();
	gettimeofday(&end_time, NULL);	

	double tu=end_time.tv_sec*1e6+end_time.tv_usec-(start_time.tv_sec*1e6+start_time.tv_usec);
	double ttime=tu/1e6;
	printf("%f\n", ttime);

	char r[100];	
	parse_readable_size(npes*block_size/ttime, r);
	printf("Bandwidth=%s\n",r);


	return 0;
}
