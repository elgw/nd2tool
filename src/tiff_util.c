#include "tiff_util.h"


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
    tiff_writer_t * tw = malloc(sizeof(tiff_writer_t));
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
                "%ld slices were expected and %ld has already been written\n",
                tw->P, tw->dd);
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
    ttags * T  = malloc(sizeof(ttags));
    T->xresolution = 1;
    T->yresolution = 1;
    T->zresolution = 1;
    T->imagedescription = NULL;
    T->software = NULL;
    T-> resolutionunit = RESUNIT_CENTIMETER;
    T->IJIJinfo = NULL;
    T->nIJIJinfo = 0;
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
    T->imagedescription = malloc(1024);

    sprintf(T->imagedescription,
            "ImageJ=1.52r\nimages=%ld\nslices=%ld\nunit=nm\nspacing=%.1f\nloop=false.",
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
    T->software = malloc(strlen(sw)+2);
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
        /* A little more meta data is required to produce an image
           that ImageJ can read.

           Useful references:
           https://stackoverflow.com/questions/24059421/adding-custom-tags-to-a-tiff-file
           http://www.simplesystems.org/libtiff/addingtags.html Also
           compare to what ImageJ writes to composite images, i.e. use
           tiffinfo on a composite image.
        */

        if(T->imagedescription)
        {
            free(T->imagedescription);
        }
        T->imagedescription = malloc(1024);

        sprintf(T->imagedescription,
                "ImageJ=1.52r\n"
                "images=%ld\n"
                "slices=%ld\n"
                "unit=nm\n"
                "spacing=%.1f\n"
                "loop=false\n"
                "channels=%d\n"
                "mode=composite\n",
                T->P*T->nchannel,
                T->P,
                T->zresolution,
                T->nchannel);

        /* In order to write to the custom tags they have to be registered first
         */
        assert(TIFFDataWidth(TIFF_BYTE) == 1);
        assert(TIFFDataWidth(TIFF_LONG) == 4);

        static const TIFFFieldInfo xtiffFieldInfo[] = {
            { IJ_META_DATA_BYTE_COUNTS, // tag
              TIFF_VARIABLE, // read count
              TIFF_VARIABLE, // write count
              TIFF_LONG, // data type
              FIELD_CUSTOM, //
              1, // can be updated
              1, // count must be passed
              "MetaDataByteCounts" }, // name
            { IJ_META_DATA,
              -1,
              -1,
              TIFF_BYTE,
              FIELD_CUSTOM,
              1,
              1,
              "MetaData" }
        };

        TIFFMergeFieldInfo(tfile, xtiffFieldInfo, 2);

        /* Now we can write */
        uint32_t count = (T->nchannel + 2);
        uint32_t * value = malloc(count*sizeof(uint32_t));
        value[0] = 20;
        value[1] = 64;
        for(uint32_t kk = 2; kk<count; kk++)
        {
            value[kk] = 768; // size of RGB data
        }
        TIFFSetField(tfile, IJ_META_DATA_BYTE_COUNTS, count, (void*) value);

        count = T->nchannel*768 + 20 + 64;
        free(value);

        /* First 20 numbers, don't know what these are except for the
         * last one which is the number of channels.  */
        uint8_t * metadata = calloc(count, 1);
        // 73,74,73,74,114,97,110,103,0,0,0,1,108,117,116,115,0,0,0,
        metadata[0] = 73;
        metadata[1] = 74;
        metadata[2] = 73;
        metadata[3] = 74;
        metadata[4] = 114;
        metadata[5] = 97;
        metadata[6] = 110;
        metadata[7] = 103;
        metadata[8] = 0;
        metadata[9] = 0;
        metadata[10] = 0;
        metadata[11] = 1;
        metadata[12] = 108;
        metadata[13] = 117;
        metadata[14] = 116;
        metadata[15] = 117;
        metadata[16] = 0;
        metadata[17] = 0;
        metadata[18] = 0;
        metadata[19] = T->nchannel;

        /* TODO: Write the next 64 numbers */

        /* TODO: Use some meaningful colormaps. Seems like ImageJ
         * picks something automatically if they are all black as it
         * is left here. */

        TIFFSetField(tfile, IJ_META_DATA, count, (void*) metadata);
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
