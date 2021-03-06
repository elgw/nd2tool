#ifndef __nd2tool_h__
#define __nd2tool_h__

#define ND2TOOL_VERSION_MAJOR "0"
#define ND2TOOL_VERSION_MINOR "0"
#define ND2TOOL_VERSION_PATCH "2"


#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <sys/stat.h>

#include "../include/Nd2ReadSdk.h"
#include "tiff_util.h"
#include "json_util.h"


/* General settings */
typedef struct{
    int verbose;
    int convert;
    int showinfo;
    int overwrite;
/* If metadata is to be dumped to stdout */
    int metamode;
    int meta_file;
    int meta_coord;
    int meta_frame;
    int meta_text;
    int meta_exp;
    /* Index of first argument not consumed by getopt_long */
    int optind;
} ntconf_t;

ntconf_t * ntconf_new(void);
void ntconf_free(ntconf_t * );

/* Structs holding a subset of the information that can be found in
   the nd2 files */

/* Data from Lim_FileGetAttributes */
typedef struct{
    int bitsPerComponentInMemory; // "bitsPerComponentInMemory": 16,
    int bitsPerComponentSignificant; // "bitsPerComponentSignificant": 16,
    int componentCount; //"componentCount": 4,
    int heightPx; //"heightPx": 2048,
    int pixelDataType; // "pixelDataType": "unsigned",
    int sequenceCount; // "sequenceCount": 61,
    int widthBytes; // "widthBytes": 16384,
    int widthPx; //"widthPx": 2048
} file_attrib_t;

/* Data from Lim_FileGetMetadata */
typedef struct{
    char * name; /* A647, DAPI, etc */
    double emissionLambdaNm;
    double objectiveMagnification;
    char *  objectiveName;
    double objectiveNumericalAperture;
    double immersionRefractiveIndex;
    int index; /* Channel number */
    int M; /* Volume size from "voxelCount" */
    int N;
    int P;
    double dx_nm;
    double dy_nm;
    double dz_nm;
} channel_attrib_t;

typedef struct
{
    channel_attrib_t ** channels;
    int nchannels;
} metadata_t;

typedef struct
{
    char * filename;
    metadata_t * meta_att;
    file_attrib_t * file_att;
    char * error;
    int nFOV;
    char * loopstring;
    char * outfolder;
    char * logfile;
    FILE * log;
} nd2info_t;


/* Main interface for querying nd2 metadata */
nd2info_t * nd2info(ntconf_t *, char * file);
void nd2info_free(nd2info_t * n);
void nd2info_print(FILE *, const nd2info_t *);
nd2info_t * nd2info_new();

/* Utility functions */
void file_attrib_free(file_attrib_t * f);
void metadata_free(metadata_t * m);
metadata_t * parse_metadata(const char * str);
file_attrib_t * parse_file_attrib(const char * str);

/* RAW metadata extraction without JSON parsing */
void showmeta(ntconf_t * conf, char * file);
void showmeta_file(char *);
void showmeta_coord(char *);
void showmeta_frame(char *);
void showmeta_text(char *);
void showmeta_exp(char *);

/* Convert to tif and place in the outfolder. The outfolder has to
   exist. */
int nd2_to_tiff(ntconf_t *, nd2info_t *);

/* Write some initial information to the log file, info->log */
void hello_log(ntconf_t * conf, nd2info_t * info, int argc, char ** argv);

/*
 * Misc
 */

/* Replace '\\r\\n' by '   \n' */
void filter_textinfo(char * s);
/* Remove .nd2 from file.nd2 */
void remove_file_ext(char * str);
/* Get the peak memory used by the process from /proc/<PID>/status */
size_t get_peakMemoryKB(void);
/* Check if file exists */
int isfile(char *);

void show_help(char * binary_name);
void print_version(FILE * fid);
void print_web(FILE * fid);

#endif
