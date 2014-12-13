#include <shmem.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include "option.h"

#define MAX_NODENAME_LENGTH 1024
struct peinfo{
	char nodename[MAX_NODENAME_LENGTH];
	int pe;
	int noderank;
};

int compare(const void * a, const void *b){
	int ret=strcmp(a,b);
	if(ret!=0)return ret;
	return ((struct peinfo*)(a))->pe-((struct peinfo*)(b))->pe;
}
int main(int argc, char *argv[])
{
	size_t block_size,delay;
	char *topo;
	int debug;


	int ret=parse_option(argc,argv,&block_size,&topo,&debug,&delay);
	if(ret!=0) return ret;

	start_pes(0);
	int npes=shmem_n_pes();
	int me=shmem_my_pe();

	struct peinfo *pl=(struct peinfo*)shmalloc(sizeof(struct peinfo)*npes);	
	int i;
	struct utsname unameData;
	uname(&unameData);
	if(debug){
		printf("%d: %s\n", me, unameData.nodename);
	}
	strcpy(pl[me].nodename, unameData.nodename);
	pl[me].pe=me;
	for(i=0;i<npes;i++){
		if(i!=me){
			shmem_putmem(&pl[me], &pl[me], sizeof(struct peinfo), i);	
		}
	}
	shmem_barrier_all();
	qsort(pl, npes, sizeof(struct peinfo), compare);

	int selfnode;
	int selfind;
	int noderank=-1;
	char *prevname="";
	for(i=0;i<npes;i++){
		if(strcmp(pl[i].nodename,prevname)!=0){
			noderank++;
			pl[i].noderank=noderank;
			prevname=pl[i].nodename;
		}else{
			pl[i].noderank=noderank;
		}
		if(pl[i].pe==me){
			selfnode=noderank;
			selfind=i;
		}
	}

	int nodesum=noderank+1;

	int selfsum=1;
	for(i=selfind+1;i<npes;i++){
		if(pl[i].noderank==selfnode)
			selfsum++;
	}	
	for(i=selfind-1;i>=0;i--){
		if(pl[i].noderank==selfnode)
			selfsum++;
	}


	
	void *block=shmalloc(block_size);
	memset(block,0,block_size);

	/* pre process of the block content */	

	
	struct timeval start_time, end_time;

	int target,source;	
	if(strcmp(topo,"ring")==0){
		target=pl[(selfind+1)%npes].pe;
		source=pl[(selfind-1+npes)%npes].pe;
	}else if(strcmp(topo,"pair")==0){
		int targetdirect=1;
		if(selfnode%2==1){
			targetdirect=-1;
		}

		// Specific Judge for single node case
		if(nodesum==1){
			if(me%2==1){
				targetdirect=-1;
			}
		   	selfsum=1;
		}
		//printf("%d: selfind=%d selfnode=%d targetdirect=%d selfsum=%d \n", me, selfind, selfnode, targetdirect, selfsum);

		//printf("%d: ind %d\n", rank, selfind+targetdirect*selfsum);
		//printf("%d: targetd %d\n", rank, targetdirect);
		target=pl[(selfind+targetdirect*selfsum)].pe;
		source=target;
	}else{
		fprintf(stderr, "Unknow topology %s\n", topo);
		exit(-1);
	}

	if(debug){
		printf("%d -> (%d) -> %d\n",source, me, target);
	}

	shmem_barrier_all();		
	gettimeofday(&start_time, NULL);	
	if(delay){
		printf("delay=%lu\n", delay);
		double r=rand();	
		double p=r/RAND_MAX;
		double d=(double)(delay)*p;
		long long dd=(long long)d;
		usleep(dd);
	}
	
	shmem_putmem(block,block,block_size,target);

	shmem_barrier_all();
	gettimeofday(&end_time, NULL);	

	double tu=end_time.tv_sec*1e6+end_time.tv_usec-(start_time.tv_sec*1e6+start_time.tv_usec);
	double ttime=tu/1e6;
	printf("Transmission time=%.6fs\n", ttime);		

	char r[100],r1[100];	
	parse_readable_size(npes*block_size/ttime, r);
	parse_readable_size(block_size/ttime, r1);
	printf("Total Bandwidth=%s/s Average Bandwidth=%s/s\n",r,r1);


	return 0;
}
