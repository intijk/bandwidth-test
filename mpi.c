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
	size_t block_size;
	char *topo;
	int debug;

	int ret=parse_option(argc,argv,&block_size,&topo,&debug);
	if(ret!=0) return ret;


	int rank, size;
	MPI_Init (&argc, &argv);	/* starts MPI */
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* get current process id */
	MPI_Comm_size (MPI_COMM_WORLD, &size);	/* get number of processes */
	struct utsname unameData;
	uname(&unameData);
	printf("%d: %s\n", rank, unameData.nodename);

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

	int selfsum=1;
	for(i=selfind+1;i<size;i++){
		if(pl[i].noderank==selfnode)
			selfsum++;
	}	
	for(i=selfind-1;i>=0;i--){
		if(pl[i].noderank==selfnode)
			selfsum++;
	}

	if(rank==2){
		for(i=0;i<size;i++){
			printf("rank 0 receive %s %d\n", pl[i].nodename, pl[i].rank);
		}
		printf("selfind %d selfnode %d selfsum %d\n", selfind,selfsum,selfnode);
	}
	
	// Asumption -- number of node is even number, each node with same number of processes.
	

	int target;	
	if(strcmp(topo,"ring")==0){
		target=pl[(selfind+1)%size].rank;
	}else if(strcmp(topo,"pair")==0){
		int targetdirect=1;
		if(selfnode%2==1){
			targetdirect=-1;
		}
		target=pl[(selfind+targetdirect*selfsum)].rank;
	}
		
	void *block=shmalloc(block_size);
	memset(block,0,block_size);
	
	
	

	MPI_Finalize();
	
	return 0;
}
