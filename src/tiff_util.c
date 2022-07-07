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
        fprintf(stdout, "tim_tiff: File is > 2 GB, using BigTIFF format\n");
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
        uint16_t * line = slice + tw->M*kk;
        int ok = TIFFWriteScanline(tw->out, // TIFF
                                   line, //buf,
                                   kk, // row
                                   0); //sample
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

int u16_to_tiff(const char * fName, uint16_t * V,
                ttags * T,
                int64_t N, int64_t M, int64_t P)
{

    //    size_t bytesPerSample = sizeof(uint16_t);
    char formatString[4] = "w";
    if(M*N*P*sizeof(uint16_t) >= pow(2, 32))
    {
        sprintf(formatString, "w8\n");
        fprintf(stdout, "tim_tiff: File is > 2 GB, using BigTIFF format\n");
    }

    TIFF* out = TIFFOpen(fName, formatString);
    assert(out != NULL);
    ttags_set(out, T);

    //size_t linbytes = (M+N)*bytesPerSample;
    //uint16_t * buf = _TIFFmalloc(linbytes);
    //    memset(buf, 0, linbytes);

    for(size_t dd = 0; dd < (size_t) P; dd++)
    {


        TIFFSetField(out, TIFFTAG_IMAGEWIDTH, N);  // set the width of the image
        TIFFSetField(out, TIFFTAG_IMAGELENGTH, M);    // set the height of the image
        TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);   // set number of channels per pixel
        TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 16);    // set the size of the channels
        TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);    // set the origin of the image.
        //   Some other essential fields to set that you do not have to understand for now.
        TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
        // TODO TIFFSSetFieldTIFFTAG_SOFTWARE

        /* We are writing single page of the multipage file */
        TIFFSetField(out, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
        /* Set the page number */
        TIFFSetField(out, TIFFTAG_PAGENUMBER, dd, P);


        for(size_t kk = 0; kk < (size_t) M; kk++)
        {
            uint16_t * line = V + M*N*dd + kk*N;
            //buf = V + M*N*dd + kk*N;
            /*
              for(size_t ll = 0; ll < (size_t) N; ll++)
              {
              buf[ll] = V[M*N*dd + kk*N + ll];
              }
            */
            int ok = TIFFWriteScanline(out, // TIFF
                                       line, //buf,
                                       kk, // row
                                       0); //sample
            if(ok != 1)
            {
                fprintf(stderr, "fim_tiff ERROR: TIFFWriteScanline failed\n");
                exit(EXIT_FAILURE);
            }
        }

        TIFFWriteDirectory(out);
    }


    TIFFClose(out);
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
    // Image size MxNxP
    T->M = 0;
    T->N = 0;
    T->P = 0;
    return T;
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
            "ImageJ=1.52r\nimages=%d\nslices=%d\nunit=nm\nspacing=%.1f\nloop=false.",
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

void ttags_get(TIFF * tfile, ttags * T)
{
    // https://docs.openmicroscopy.org/ome-model/5.6.3/ome-tiff/specification.html
    // a string of OME-XML metadata embedded in the ImageDescription tag of the first IFD (Image File Directory) of each file. The XML string must be UTF-8 encoded.

    T->xresolution = 1;
    T->yresolution = 1;
    T->imagedescription = NULL;
    T->software = NULL;
    T->resolutionunit = RESUNIT_NONE;


    uint16_t runit;
    if(TIFFGetField(tfile, TIFFTAG_RESOLUTIONUNIT, &runit))
    {
        T->resolutionunit = runit;
    }

    float xres = 0, yres = 0;;
    if(TIFFGetField(tfile, TIFFTAG_XRESOLUTION, &xres))
    {
        T->xresolution = xres;
    }

    if(TIFFGetField(tfile, TIFFTAG_XRESOLUTION, &yres))
    {
        T->yresolution = yres;
    }

    char * desc = NULL;
    if(TIFFGetField(tfile, TIFFTAG_IMAGEDESCRIPTION, &desc) == 1)
    {
        T->imagedescription = malloc(strlen(desc)+2);
        strcpy(T->imagedescription, desc);
    }

    char * software = NULL;
    if(TIFFGetField(tfile, TIFFTAG_SOFTWARE, &software) == 1)
    {
        T->software = malloc(strlen(software)+2);
        strcpy(T->software, software);
        //    printf("! Got software tag: %s\n", T->software);
    }

#if 0
    /*
     * From https://github.com/imagej/ImageJA/blob/master/src/main/java/ij/io/TiffDecoder.java
     * 	public static final int META_DATA_BYTE_COUNTS = 50838; // private tag registered with Adobe
     *      public static final int META_DATA = 50839; // private tag registered with Adobe
     */

    uint32_t count;
    void * data;
    if(TIFFGetField(tfile, XTAG_IJIJUNKNOWN, &count, &data))
    {
        uint32_t * dvalue  = (uint32_t*) data;

        for(int kk = 0; kk<count; kk++)
        {
            fprintf(fim_tiff_log, "Tag %d: %d, count %d\n", XTAG_IJIJUNKNOWN, dvalue[kk], count);
        }
    }

    T->nIJIJinfo = 0;
    T->IJIJinfo = NULL;
    if(TIFFGetField(tfile, XTAG_IJIJINFO, &count, &data))
    {
        T->nIJIJinfo = count;
        T->IJIJinfo = malloc(count);
        memcpy(T->IJIJinfo, data, count);
        uint8_t * udata = (uint8_t*) data;
        for(int kk = 0; kk<count; kk++)
        { fprintf(fim_tiff_log, "%02d %c %u\n ", kk, udata[kk], udata[kk]);};
    }
#endif
}

void ttags_set(TIFF * tfile, ttags * T)
{
    //ttags_show(stdout, T);
    TIFFSetDirectory(tfile, 0);
    if(T->software != NULL)
    {
        TIFFSetField(tfile, TIFFTAG_SOFTWARE, T->software);
    }

    if(T->imagedescription != NULL)
    {
        TIFFSetField(tfile, TIFFTAG_IMAGEDESCRIPTION, T->imagedescription);
    }

    TIFFSetField(tfile, TIFFTAG_XRESOLUTION, T->xresolution);
    TIFFSetField(tfile, TIFFTAG_YRESOLUTION, T->yresolution);
    TIFFSetField(tfile, TIFFTAG_RESOLUTIONUNIT, T->resolutionunit);
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
