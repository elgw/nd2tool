#ifndef tiff_util_h
#define tiff_util_h

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <tiffio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define INLINED inline __attribute__((always_inline))
#define XTAG_IJIJUNKNOWN 50838
#define XTAG_IJIJINFO 50839


/* Tiff tags -- for simple transfer from one image to another */
typedef struct{
    float xresolution;
    float yresolution;
    float zresolution;
    char * imagedescription;
    char * software;
    uint16_t resolutionunit;
    char * IJIJinfo; // Tag 50839 contains a string, used by Imagej.
    uint32_t nIJIJinfo;
    // Image size
    int M;
    int N;
    int P;
} ttags;

// new with everything set to defaults
ttags * ttags_new();
void ttags_get(TIFF *, ttags *);
void ttags_show(FILE *, ttags *);
void ttags_set(TIFF *, ttags *);
void ttags_set_software(ttags * , char *);
void ttags_set_imagesize(ttags *, int M, int N, int P);
void ttags_set_pixelsize_nm(ttags *, double, double, double);
void ttags_free(ttags **);

typedef struct{
    int M;
    int N;
    int P;
    int dd; // Slice to write
    TIFF * out;
} tiff_writer_t;

/* These three functions enables writing a tif image slice by slice */

/* State what you intend to do */
tiff_writer_t * tiff_writer_init(const char * fName,
                                 ttags * T,
                                 int64_t N, int64_t M, int64_t P);
/* Write a slice */
int tiff_writer_write(tiff_writer_t * tw, uint16_t * slice);
/* Close file and free memory */
int tiff_writer_finish(tiff_writer_t * tw);

int u16_to_tiff(const char * fName, uint16_t * V,
                ttags * T,
                int64_t M, int64_t N, int64_t P);

#endif
