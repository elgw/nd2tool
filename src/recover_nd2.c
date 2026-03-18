// Assumptions:
//
// The ImageDataSeq|#! tags are ordered, i.e. we will find
// ImageDataSeq|0!, then ImageDataSeq|1!, etc...
//
// These tags are repeated at the end of the file in the chunk map, so
// as soon as see a duplicate we are done.
//
// It is also assumed that the ImageDataSeq chunks are about the same size
// this is used to quick jump to a new expected location.
//
// Tested on an nd2 file with the signature:
//
// .... ...@.......ND2 FILE SIGNATURE CHUNK NAME01!Ver3.0......
//
// It would be quite simple to also allow for some bit-rot,
// by  using some fuzzy distance metric, for example the Hamming
// distance, to find the "ImageDataSeq" tags.
//
// Todo:
// - Be aware of missing/undetected ImageDataSeq's.
// - Test on some more images ...
// - Include in nd2tool
// - Export files without the python step
// - Adopt so that it can be built on mac... ?
// - ...
//
// Version 1.0.0, 2026-03-18
// Erik Wernersson

#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string.h>
#include <getopt.h>

typedef uint8_t u8;
typedef int64_t i64;

typedef struct {
    int width; // width of image planes
    int height; // height of image planes
    int nchan; // number of channels/colors
    int nz; // number of z planes
    int bps; // bits per sample (and color)
    int write_to_disk;
    int optpos;
    int overwrite;
} conf;

static void conf_show(conf * s)
{
    printf("Input values\n");
    printf("::: Image size (X x Y x Z): %d x %d x %d\n", s->width, s->height, s->nz);
    printf("::: channels: %d\n", s->nchan);
    return;
}
conf * conf_new(void)
{
    conf * s = calloc(1, sizeof(conf));
    s->width = 1024;
    s->height = 1024;
    s->nchan = 5;
    s->nz = 87;
    s->bps = 2;
    s->write_to_disk = 1;
    return s;
}

void conf_free(conf * s)
{
    free(s);
}

static int parse_im_id(const char * _str)
{
    char str[32];
    memcpy(str, _str, 32);
    str[31] = '\0';
    int start_pos = 0;
    int end_pos = 0;
    for(int kk = 0; kk < 32; kk++)
    {
        if(str[kk] == '|')
        {
            start_pos = kk;
        }
        if(str[kk] == '!')
        {
            end_pos = kk;
        }
    }
    if(start_pos == 0 || end_pos == 0)
    {
        printf("Unable to parse %s\n", str);
        exit(EXIT_FAILURE);
    }
    str[start_pos] = '\0';
    str[end_pos] = '\0';
    return atoi(str+(start_pos+1));
}

static void usage(void)
{
    printf("recover_nd2 can possibly be used to recover image data "
           "from an nd2 file which is missing the the chunk map "
           "which is stored at the end of the file.\n"
           "Output: Creates <input>.recover_locations.csv"
           "which contains the positions of the image data chunks"
           "\n"
           "The output file can be used with the accompanying python script\n");
    printf("\n");
    printf("Usage: recover_nd2 [arguments] file.nd2\n");
    printf("\n");
    printf("Arguments\n");
    printf("--dry"
           "\n\t don't write anything to disk, dump results to terminal\n");
    printf("--overwrite"
           "\n\t overwrite any existing output file\n");
    printf("\n");
#if 0
    printf("Useless arguments (at the moment)\n");
    printf("--width w"
           "\n\t set the image width\n");
    printf("--height h"
           "\n\t set the image height\n");
    printf("--ncolor c"
           "\n\t set number of colors/channels\n");
    printf("--nz z"
           "\n\t set number of z planes per image\n");
#endif
    return;
}

static int parse_cli(int argc, char ** argv, conf * s)
{

    struct option longopts[] = {
        //{"width", required_argument, NULL, 'w'},
        //{"height", required_argument, NULL, 'h'},
        //{"ncolor", required_argument, NULL, 'c'},
        //{"nz",     required_argument, NULL, 'z'},
        //{"dry",    no_argument,       NULL, 'W'},
        {"overwrite", no_argument,    NULL, 'o'},
        {NULL, 0, NULL, 0}};

    int ch;
    while((ch = getopt_long(argc, argv, "w:h:c:z:Wo", longopts, NULL)) != -1)
    {
        switch(ch){
        case 'w':
            s->width = atoi(optarg);
            break;
        case 'h':
            s->height = atoi(optarg);
            break;
        case 'c':
            s->nchan = atoi(optarg);
            break;
        case 'z':
            s->nz = atoi(optarg);
            break;
        case 'W':
            s->write_to_disk = 0;
            break;
        case 'o':
            s->overwrite = 1;
        default:
            usage();
            exit(EXIT_FAILURE);
        }
    }
    s->optpos = optind;
    return 0;
}


static void check_image_size(conf * s, size_t * ids_pos, int n_ids)
{
    // Figure out how many bytes the images can be

    if(n_ids < 2) // if there is just one data chunk we skip this
    {
        return;
    }

    i64 max_image_bytes = ids_pos[1]-ids_pos[0];
    for(int kk = 0; kk+1 < n_ids; kk++)
    {
        i64 size = ids_pos[kk+1] - ids_pos[kk];
        size -= 4096;
        size += 16;
        if(size < max_image_bytes)
        {
            max_image_bytes = size;
        }
    }

    printf("Each ImageDataSeq can have at most %ld bytes\n", max_image_bytes);
    i64 expected_image_bytes = s->width*s->height*s->nchan*s->bps;
    if(expected_image_bytes <= max_image_bytes)
    {
        printf("Good! That is compatible with the expected sizes: %ld\n", expected_image_bytes);
    } else {
        printf("Problematic. Expected them to be %ld bytes\n", expected_image_bytes);
    }

    return;
}

static void print_signature(conf * s, u8 * data)
{
    for(int kk = 0; kk < 60; kk++)
    {
        char c = (char) data[kk];
        if(isprint(c))
        {
            printf("%c", c);
        } else {
            printf(".");
        }
    }
    printf("\n");
}

int file_exists(const char * fname)
{
    FILE * fid = fopen(fname, "r");
    if(fid == NULL)
    {
        return 0;
    }
    fclose(fid);
    return 1;
}

int main(int argc, char ** argv)
{
    if(argc < 1)
    {
        printf("Usage: %s file.nd2\n", argv[0]);
    }

    conf * s = conf_new();

    if(parse_cli(argc, argv, s))
    {
        printf("Invalid command line arguments\n");
        exit(EXIT_FAILURE);
    }

    //conf_show(s);

    if(s->optpos >= argc)
    {
        printf("No file name given\n");
        exit(EXIT_FAILURE);
    }
    const char * infile = argv[s->optpos];

    int fd = open(infile, O_RDONLY);
    if(fd < 0)
    {
        printf("Unable to open %s\n", infile);
        exit(EXIT_FAILURE);
    }
    errno = 0;

    struct stat finfo = {};
    if(fstat(fd, &finfo))
    {
        printf("stat failed\n");
        exit(EXIT_FAILURE);
    }

    size_t length = finfo.st_size;
    if(length < 2000)
    {
        printf("File size %zu bytes is too small\n", length);
        exit(EXIT_FAILURE);
    }
    printf("%s is %zu b\n", infile, length);
    u8 * data = (u8*) mmap(NULL,
                           length,
                           PROT_READ, // prot
                           MAP_PRIVATE | MAP_NORESERVE, // flags
                           fd,
                           0); // offset
    assert(errno == 0);

    close(fd);

    char * outfile = NULL;

    if(s->write_to_disk)
    {
        outfile = calloc(strlen(infile) + 64, 1);
        sprintf(outfile, "%s.recover_locations.csv", infile);
        printf("Will write to %s\n", outfile);
        if(file_exists(outfile))
        {
            if(s->overwrite == 0)
            {
                printf("Output file does already exist, either delete it or use --overwrite "
                       "if you want it re-generated\n");
                free(outfile);
                exit(EXIT_SUCCESS);
            }
        }
    }

    print_signature(s, data);

    // look for ImageDataSeq|0!, ImageDataSeq|1!, ...
    size_t * ids_pos = calloc(65536, sizeof(size_t));
    int n_images = 0;

    size_t nI = 0;
    for(size_t kk = 0; kk < length-32; kk++)
    {
        if(data[kk] == 'I')
        {
            nI++;
            if( strncmp((const char*) &data[kk], "ImageDataSeq|", 5+4+3+1) == 0)
            {
                // printf("%8s\n", (char*) data+kk);
                // parse image id
                // we can speed it up a lot by jumping forward the expected size ...
                int im_id = parse_im_id((const char*) &data[kk]);
                if(im_id == n_images)
                {
                    ids_pos[n_images++] = kk;
                    if(im_id > 1)
                    {
                        i64 jump = ids_pos[1]-ids_pos[0] - 2048;
                        kk += jump;
                    }
                }
            }
        }
    }
    munmap(data, length);
    printf("Found %d ImageDataSeq\n", n_images);

    check_image_size(s, ids_pos, n_images);


    FILE * fout = stdout;
    if(s->write_to_disk)
    {
        fout = fopen(outfile, "w");
    }

    assert(fout != NULL);
    fprintf(fout, "image_id,header_position,image_position\n");
    for(int im_id = 0; im_id < n_images; im_id++)
    {
        fprintf(fout, "%d, %zu, %zu\n",
                im_id,
                ids_pos[im_id],
                ids_pos[im_id] + 4096 - 16);
        // + 4096 - 16 skips past the header to find the image data
        //             it is a quite unexpected value! Why not simply 4096?
    }

    if(s->write_to_disk)
    {
        fclose(fout);
    }

    free(outfile);
    conf_free(s);
    free(ids_pos);
    return EXIT_SUCCESS;
}
