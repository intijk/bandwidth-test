#include <mpi.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>
#include "option.h"

#define MAX_NODENAME_LENGTH 1024
struct procinfo{
	char nodename[MAX_NODENAME_LENGTH];
	int rank;
	int noderank;
};

int compare(const void *a, const void *b)
{
	int ret=strcmp(a,b);
	if(ret!=0)return ret;
	return ((struct procinfo*)(a))->rank-((struct procinfo*)(b))->rank;
}
int main (int argc, char *argv[])
{
	size_t block_size,delay;
	char *topo;
	int debug=0;

	int ret=parse_option(argc,argv,&block_size,&topo,&debug,&delay);
	if(ret!=0) return ret;


	int rank, size;
	MPI_Init (&argc, &argv);	/* starts MPI */
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* get current process id */
	MPI_Comm_size (MPI_COMM_WORLD, &size);	/* get number of processes */
	struct utsname unameData;
	uname(&unameData);
	if(debug){
		printf("%d: %s\n", rank, unameData.nodename);
	}

	struct procinfo * pl=
		(struct procinfo *)malloc(sizeof(struct procinfo)*size);
	
	strcpy(pl[rank].nodename, unameData.nodename);
	pl[rank].rank=rank;
	int i;
	//broadcast info to all others.
	int pinfos=sizeof(struct procinfo);
	for(i=0;i<size;i++){
		MPI_Bcast(&pl[i], pinfos, MPI_BYTE, i, MPI_COMM_WORLD);
	}

	//sort on node name and merge nodes on each node.
	qsort(pl, size, sizeof(struct procinfo), compare);
	//according to the config , set the performance test target.
	
	int selfnode;
	int selfind;
	int noderank=-1;
	char *prevname="";
	for(i=0;i<size;i++){
		if(strcmp(pl[i].nodename,prevname)!=0){
			noderank++;
			pl[i].noderank=noderank;
			prevname=pl[i].nodename;
		}else{
			pl[i].noderank=noderank;
		}
		if(pl[i].rank==rank){
			selfnode=noderank;
			selfind=i;
		}
	}
	int nodesum=noderank+1;

	int selfsum=1;
	for(i=selfind+1;i<size;i++){
		if(pl[i].noderank==selfnode)
			selfsum++;
	}	
	for(i=selfind-1;i>=0;i--){
		if(pl[i].noderank==selfnode)
			selfsum++;
	}

	/*
	if(rank==2){
		for(i=0;i<size;i++){
			printf("rank 0 receive %s %d\n", pl[i].nodename, pl[i].rank);
		}
		printf("selfind %d selfnode %d selfsum %d\n", selfind,selfnode,selfsum);
	}
	*/
	
	// Asumption -- number of node is even number, each node with same number of processes.
	

	int target,source;	
	if(strcmp(topo,"ring")==0){
		target=pl[(selfind+1)%size].rank;
		source=pl[(selfind-1+size)%size].rank;
	}else if(strcmp(topo,"pair")==0){
		int targetdirect=1;
		if(selfnode%2==1){
			targetdirect=-1;
		}

		// Specific Judge for single node case
		if(nodesum==1){
			if(rank%2==1){
				targetdirect=-1;
			}
		   	selfsum=1;
		}

		//printf("%d: ind %d\n", rank, selfind+targetdirect*selfsum);
		//printf("%d: targetd %d\n", rank, targetdirect);
		target=pl[(selfind+targetdirect*selfsum)].rank;
		source=target;
	}else{
		fprintf(stderr, "Unknow topology %s\n", topo);
		exit(-1);
	}

	if(debug){
		printf("%d -> (%d) -> %d\n",source, rank, target);
	}
		
	void *block=malloc(block_size);
	memset(block,0,block_size);

	long long dd;
	if(delay){
		srand(rank);
		double r=rand();	
		double p=r/RAND_MAX;
		double d=(double)(delay)*p;
		dd=(long long)d;
		if(debug){
			printf("Real delay=%lld us\n", dd);
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);
	double st=MPI_Wtime();
	if(delay){
		usleep(dd);
	}



	MPI_Request r1,r2;
	MPI_Status status;
	MPI_Isend(block, block_size, MPI_BYTE, target, 0, MPI_COMM_WORLD, &r1);
	MPI_Irecv(block, block_size, MPI_BYTE, source, 0, MPI_COMM_WORLD, &r2);
	//MPI_Sendrecv(block, block_size, MPI_BYTE, target, 0, 
	//		     block, block_size, MPI_BYTE, target, 0, MPI_COMM_WORLD, NULL);
	MPI_Wait(&r1, &status);
	MPI_Wait(&r2, &status);	
	MPI_Barrier(MPI_COMM_WORLD);	
	double et=MPI_Wtime();

	printf("Transmission time=%.6fs\n", et-st);		
	char r[100],ra[100];
	parse_readable_size(size*block_size/(et-st), r);
	parse_readable_size(block_size/(et-st), ra);
	printf("Total Bandwidth=%s/s Average Bandwidth=%s/s\n",r,ra);

	MPI_Finalize();
	
	return 0;
}
