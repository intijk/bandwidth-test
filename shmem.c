#include <shmem.h>
#include <stdio.h>
#include <regex.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
static struct option
long_options[]=
{
	{ "debug",    optional_argument, NULL, 'd'},
	{ "size",     required_argument, NULL, 's'},
	{ "topology", required_argument, NULL, 't'},
	{ "help",     optional_argument, NULL, 'h'},
	{ NULL,       no_argument,       NULL,  0 },
};

size_t parse_size(char *optarg){
	regex_t regex;
	int reti;
	reti=regcomp(&regex, "[0-9][0-9]*[kmgKMG]", 0);
	size_t size;
	if(reti){
		fprintf(stderr, "Could not compile regex\n");
		return -1;
	}

	reti=regexec(&regex, optarg, 0, NULL, 0);
	if(reti==0){
		/* match the format */
		sscanf(optarg, "%lu", &size);
		int i=0;
		while(optarg[i]>='0' && optarg[i]<='9'){
			i++;
		}
		switch(optarg[i]){
			case 'k':
			case 'K':
				size*=1024;
				break;
			case 'm':
			case 'M':
				size*=1024*1024;
				break;
			case 'g':
			case 'G':
				size*=1024*1024*1024;
				break;
		}
	}else if(reti==REG_NOMATCH){
		/* format not match */
		printf("not matach\n");
		return -1;
	}else{
		/* error happen */
		return -1;
	}
	return size;
}

int main(int argc, char *argv[])
{

	

	/* 
	   -d --debug default turned off
	   -s --size size of block for testing default 128M
	   -t --topology topology of the communication, default ring
	   -h --print this help
	*/
	int debug=0;
	size_t block_size=128*1024*1024; /* default 128M */
	char *topo;
	topo="ring";		
	while(1){
		int oidx;
		const int c=getopt_long(argc, argv, "d::h::s:t:",long_options,&oidx);
		if(c==-1){
			break;
		}
		switch(c){
			case 'h':
				printf("-d --debug default turned off\n-s --size size of block for testing default 128M\n-t --topology topology of the communication, default ring\n-h --print this help\n");
				return 0;
				break;
			case 'd':
				debug=1;
				break;
			case 's':
				block_size=parse_size(optarg);
				if(block_size<0){
					fprintf(stderr, "Block size format must be [0-9]+[KMGkmg]");
					exit(-1);
				}
				break;
			case 't':
				topo=optarg;
				break;
			default:
				break;
		}
	}
	
	printf("Config:\n");
	printf("Block_size=%lu\n", block_size);	
	printf("Topo=%s\n", topo);
	printf("Debug=%s\n", debug==1?"on":"off");
	puts("");

	start_pes(0);		
	int npes=shmem_n_pes();
	int me=shmem_my_pe();

	
	void *block=shmalloc(block_size);
	memset(block,0,block_size);
	
	/* pre process of the block content */	

	
	struct timeval start_time, end_time;
	gettimeofday(&start_time, NULL);	

	shmem_barrier_all();		

	int target;
	if(strcmp(topo,"ring")==0){
		target=(me+1)%npes;
	}else if(strcmp(topo,"pair")==0){
		target=(me%2==0?(me+npes+1)%npes:(me+npes-1)%npes);
	}
	shmem_putmem(block,block,block_size,target);

	gettimeofday(&end_time, NULL);	
	shmem_barrier_all();

	double tu=end_time.tv_sec*1e6+end_time.tv_usec-(start_time.tv_sec*1e6+start_time.tv_usec);
	double ttime=tu/1e6;
	printf("%f\n", ttime);
	printf("Bandwidth=%f\n",npes*block_size/ttime);


	return 0;
}
