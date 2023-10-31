#pragma once

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void filter_textinfo(char * s);

void remove_file_ext(char * str);

size_t get_peakMemoryKB(void);

int isfile(char *);

void make_file_executable(const char * filename);

void show_color(FILE * fid, const double * RGB, double lambda);
