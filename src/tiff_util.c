#include "tiff_util.h"

#define NOT_NULL(x) {                                           \
        if(x == NULL){                                          \
            fprintf(stderr, "Got a NULL pointer at %s:%d\n",    \
                    __FILE__, __LINE__);                        \
            fprintf(stderr, "Can't continue :(\n");             \
            exit(EXIT_FAILURE);                                 \
        }                                                       \
    }                                                           \


/* see man 3 tifflib
 *
 * From: https://www.cs.rochester.edu/u/nelson/courses/vision/resources/tiff/libtiff.html#Errors
 * Finally, note that the last strip of data in an image may have fewer rows in it than specified by the
 * RowsPerStrip tag. A reader should not assume that each decoded strip contains a full set of rows in it.
 */



/* Used to redirect errors from libtiff */
void tiffErrHandler(const char* module, const char* fmt, va_list ap)
{
    fprintf(stderr, "libtiff: Module: %s\n", module);
    fprintf(stdout, "libtiff: ");
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
}


tiff_writer_t * tiff_writer_init(const char * fName,
                                 ttags * T,
                                 int64_t N, int64_t M, int64_t P)
{
    tiff_writer_t * tw = calloc(1, sizeof(tiff_writer_t));
    NOT_NULL(tw);

    tw->M = M;
    tw->N = N;
    tw->P = P;
    tw->dd = 0;

    char formatString[4] = "w";
    if(M*N*P*sizeof(uint16_t) >= pow(2, 32))
    {
        sprintf(formatString, "w8\n");
        // fprintf(stdout, "tim_tiff: File is > 2 GB, using BigTIFF format\n");
    }

    tw->out = TIFFOpen(fName, formatString);
    assert(tw->out != NULL);
    ttags_set(tw->out, T);

    return tw;
}

int tiff_writer_write(tiff_writer_t * tw, uint16_t * slice)
{
    if(tw->dd == tw->P)
    {
        fprintf(stderr, "Error: Trying to write too many slices\n"
                "%d slices were expected and %d has already been written\n",
                (int) tw->P, (int) tw->dd);
        fprintf(stderr, "In %s, line %d\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    TIFFSetField(tw->out, TIFFTAG_IMAGEWIDTH, tw->N);  // set the width of the image
    TIFFSetField(tw->out, TIFFTAG_IMAGELENGTH, tw->M);    // set the height of the image
    TIFFSetField(tw->out, TIFFTAG_SAMPLESPERPIXEL, 1);   // set number of channels per pixel
    TIFFSetField(tw->out, TIFFTAG_BITSPERSAMPLE, 16);    // set the size of the channels
    TIFFSetField(tw->out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);    // set the origin of the image.

    TIFFSetField(tw->out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tw->out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tw->out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);

    /* We are writing single page of the multipage file */
    TIFFSetField(tw->out, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
    /* Set the page number */
    TIFFSetField(tw->out, TIFFTAG_PAGENUMBER, tw->dd, tw->P);


    for(size_t kk = 0; kk < (size_t) tw->M; kk++)
    {
        //printf("kk = %zu\n", kk); fflush(stdout);
        uint16_t * line = slice + tw->N*kk;
        int ok = TIFFWriteScanline(tw->out, // TIFF
                                   line, //buf,
                                   kk, // row
                                   0); //sample
        //usleep(1000);
        if(ok != 1)
        {
            fprintf(stderr, "fim_tiff ERROR: TIFFWriteScanline failed\n");
            exit(EXIT_FAILURE);
        }
    }
    TIFFWriteDirectory(tw->out);
    tw->dd++;
    return 0;
}

int tiff_writer_finish(tiff_writer_t * tw)
{
    TIFFClose(tw->out);
    free(tw);
    return 0;
}

ttags * ttags_new()
{
    ttags * T  = calloc(1, sizeof(ttags));
    NOT_NULL(T);
    T->xresolution = 1;
    T->yresolution = 1;
    T->zresolution = 1;
    T->imagedescription = NULL;
    T->software = NULL;
    T-> resolutionunit = RESUNIT_CENTIMETER;
    T->composite = 0;
    T->nchannel = 1;
    // Image size MxNxP
    T->M = 0;
    T->N = 0;
    T->P = 0;
    return T;
}

void ttags_set_composite(ttags * T, int nchannel)
{
    T->composite = 1;
    T->nchannel = nchannel;
}

void ttags_set_imagesize(ttags * T, int M, int N, int P)
{
    T->M = M;
    T->N = N;
    T->P = P;
}

void ttags_set_pixelsize_nm(ttags * T, double xres, double yres, double zres)
{
    if(T->M == 0)
    {
        fprintf(stderr, "use ttags_set_imagesize before ttags_set_pixelsize");
        exit(EXIT_FAILURE);
    }
    if(xres != yres)
    {
        fprintf(stderr, "Only supports isotropic pixels in x-y\n");
        exit(EXIT_FAILURE);
    }
    T->resolutionunit = RESUNIT_NONE;
    T->xresolution = 1.0/xres; // Pixels per nm
    T->yresolution = 1.0/yres;
    T->zresolution = zres; // nm

    if(T->imagedescription)
    {
        free(T->imagedescription);
    }
    T->imagedescription = calloc(1024, 1);
    NOT_NULL(T->imagedescription);

    sprintf(T->imagedescription,
            "ImageJ=1.52r\nimages=%" PRId64 "\nslices=%" PRId64 "\nunit=nm\nspacing=%.1f\nloop=false.",
            T->P, T->P, T->zresolution);
}

void ttags_free(ttags ** Tp)
{
    ttags * T = Tp[0];
    if(T == NULL)
    {
        fprintf(stderr, "fim_tiff: T = NULL, on line %d in %s\n", __LINE__, __FILE__);
        exit(EXIT_FAILURE);
    }
    if(T->software != NULL)
    {
        free(T->software);
    }

    if(T->imagedescription != NULL)
    {
        free(T->imagedescription);
    }
    free(T);
}

void ttags_set_software(ttags * T, char * sw)
{
    if(T->software != NULL)
    {
        free(T->software);
    }
    T->software = calloc(strlen(sw)+2, 1);
    NOT_NULL(T->software);
    sprintf(T->software, "%s", sw);
}

void ttags_set(TIFF * tfile, ttags * T)
{
    //ttags_show(stdout, T);
    TIFFSetDirectory(tfile, 0);
    if(T->software != NULL)
    {
        TIFFSetField(tfile, TIFFTAG_SOFTWARE, T->software);
    }

    TIFFSetField(tfile, TIFFTAG_XRESOLUTION, T->xresolution);
    TIFFSetField(tfile, TIFFTAG_YRESOLUTION, T->yresolution);
    TIFFSetField(tfile, TIFFTAG_RESOLUTIONUNIT, T->resolutionunit);

    if(T->composite)
    {
        if(T->imagedescription)
        {
            free(T->imagedescription);
        }
        T->imagedescription = calloc(1024, 1);
        NOT_NULL(T->imagedescription)

        sprintf(T->imagedescription,
                "ImageJ=1.52r\n"
                "images=%" PRId64 "\n"
                "slices=%" PRId64 "\n"
                "unit=nm\n"
                "spacing=%.1f\n"
                "loop=false\n"
                "channels=%d\n"
                "mode=composite\n"
                "hyperstack=true\n",
                T->P*T->nchannel,
                T->P,
                T->zresolution,
                T->nchannel);
    }

    if(T->imagedescription != NULL)
    {
        TIFFSetField(tfile, TIFFTAG_IMAGEDESCRIPTION, T->imagedescription);
    }

    return;
}

void ttags_show(FILE * fout, ttags* T)
{

    fprintf(fout, "Resolution unit: ");
    switch(T->resolutionunit)
    {
    case RESUNIT_NONE:
        fprintf(fout, "RESUNIT_NONE\n");
        break;
    case RESUNIT_INCH:
        fprintf(fout, "RESUNIT_INCH\n");
        break;
    case RESUNIT_CENTIMETER:
        fprintf(fout, "RESUNIT_CENTIMETER\n");
        break;
    default:
        fprintf(fout, "UNKNOWN\n");
        break;
    }

    fprintf(fout, "RESOLUTION: %f %f (pixels per <unit>)\n",
            T->xresolution, T->yresolution);

    if(T->imagedescription != NULL)
    {
        fprintf(fout, "Image description: '%s'\n", T->imagedescription);
    }

    if(T->software != NULL)
    {
        fprintf(fout, "Software: '%s'\n", T->software);
    }

    return;
}
