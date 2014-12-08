#include "option.h"
struct option
long_options[]=
{
	{ "debug",    optional_argument, NULL, 'd'},
	{ "size",     required_argument, NULL, 's'},
	{ "topology", required_argument, NULL, 't'},
	{ "help",     optional_argument, NULL, 'h'},
	{ NULL,       no_argument,       NULL,  0 },
};

void parse_readable_size(size_t s, char* p){
	double k,m,g,b;	
	b=s;
	k=b/1024;
	m=k/1024;
	g=m/1024;
	
	if(g>=1){
		sprintf(p, "%.2fG\n", g);
	}else if(m>=1){
		sprintf(p, "%.2fM\n", m);
	}else if(k>=1){
		sprintf(p, "%.2fK\n", k);
	}else
		sprintf(p, "%.0fB\n", b);

}
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

int parse_option(int argc, char *argv[], size_t* block_size, char** topo, int* debug){
	/* 
	   -d --debug default turned off
	   -s --size size of block for testing default 128M
	   -t --topology topology of the communication, default ring
	   -h --print this help
	*/
	*debug=0;
	*block_size=128*1024*1024; /* default 128M */
	*topo="ring";
	while(1){
		int oidx;
		const int c=getopt_long(argc, argv, "d::h::s:t:",long_options,&oidx);
		if(c==-1){
			break;
		}
		switch(c){
			case 'h':
				printf("-d --debug default turned off\n-s --size size of block for testing default 128M\n-t --topology topology of the communication, default ring\n-h --print this help\n");
				return 1;
				break;
			case 'd':
				*debug=1;
				break;
			case 's':
				*block_size=parse_size(optarg);
				if((*block_size)<0){
					fprintf(stderr, "Block size format must be [0-9]+[KMGkmg]");
					exit(-1);
				}
				break;
			case 't':
				*topo=optarg;
				break;
			default:
				break;
		}
	}
	
	printf("Config:\n");
	char r[100];
	parse_readable_size(*block_size, r);
	printf("Block_size=%s\n", r);	
	printf("Topo=%s\n", *topo);
	printf("Debug=%s\n", (*debug==1)?"on":"off");
	puts("");

	return 0;
}
