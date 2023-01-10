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

#define IJ_META_DATA_BYTE_COUNTS 50838
#define IJ_META_DATA 50839

/* TIFF tags can be read from a tif tile or written to a tif file
 * using a ttag */
typedef struct{
    char * imagedescription;
    char * software;

    uint16_t resolutionunit; // Default RESUNIT_CENTIMETER
    float xresolution;
    float yresolution;
    float zresolution;

    /* Image size in pixels */
    int64_t M;
    int64_t N;
    int64_t P;
    int composite; /* Is a composite image or not */
    int nchannel; /* Number of channels, only used if composite is set */
} ttags;

/* Create new tags with default values */
ttags * ttags_new();

/* Print out the ttag */
void ttags_show(FILE *, ttags *);
/* Transfer tags to a tif file */
void ttags_set(TIFF *, ttags *);
void ttags_set_software(ttags * , char *);
void ttags_set_imagesize(ttags *, int M, int N, int P);
void ttags_set_pixelsize_nm(ttags *, double, double, double);
void ttags_free(ttags **);

void ttags_set_composite(ttags *, int nchannel);

typedef struct{
    int64_t M;
    int64_t N;
    int64_t P;
    int64_t dd; // Slice to write
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
