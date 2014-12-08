#include <getopt.h>
#include <stdlib.h>
#include <regex.h>
#include <stdio.h>

extern struct option long_options[];
size_t parse_size(char *optarg);
int parse_option(int argc, char *argv[], size_t* block_size, char** topo, int *debug);
void parse_readable_size(size_t s, char *p);
