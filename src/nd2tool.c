#include "nd2tool.h"
#include "nd2tool_util.h"

/* The stripped version of the header file from www.nd2sdk.com
 * where comments are removed.
 */
#include "Nd2ReadSdk_stripped.h"

#include "tiff_util.h"
#include "json_util.h"
#include "srgb_from_lambda.h"

typedef int64_t i64;

typedef enum {
    CONVERT_TO_TIF,
    SHOW_METADATA
} nt_purpose;

/* General settings */
typedef struct{
    int verbose;
    int convert;
    int showinfo;
    int showcoords;
    int overwrite;
    int composite;

    /* One file per z-plane according to SpaceTx,
       see https://github.com/elgw/nd2tool/issues/4 */
    int save_individual_planes;

    nt_purpose purpose;
    char * fov_string; /* Specifying what fov to use */
    int meta_file;
    int meta_coord;
    int meta_frame;
    int meta_text;
    int meta_exp;
    /* Index of first argument not consumed by getopt_long */
    int optind;
    int shake;
    int dry; /* Dry run -- don't write anything */
    int deconwolf; /* Write deconwolf script? */
    int deconwolfx; /* Write deconwolf script and as for arguments */
    int deconwolf_dots; /* Write script for dot detection */

    /* If only extracting a subset of the slices */
    int use_range;
    int range_from;
    int range_to;

    char * dwargs; /* Extra arguments to dw */
} ntconf_t;


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

typedef struct{
    double * stagePositionUm;
} meta_frame_t;

typedef struct
{
    channel_attrib_t ** channels;
    int nchannels;
} metadata_t;

typedef struct
{
    ntconf_t * conf;
    char * filename;
    metadata_t * meta_att;
    file_attrib_t * file_att;
    meta_frame_t * meta_frame;
    char * error;
    int nFOV;
    char * loopstring;
    char * outfolder;
    char * logfile;
    FILE * log;
    char * camera_name;
    char * microscope_name;
} nd2info_t;

/*
 * Forward declarations
 */

static ntconf_t * ntconf_new(void);
static void ntconf_free(ntconf_t * );

/* Main interface for querying nd2 metadata */
static nd2info_t * nd2info(ntconf_t *, const char * file);
static void nd2info_free(nd2info_t * n);
static void nd2info_print(const ntconf_t *, FILE *, const nd2info_t *);
static nd2info_t * nd2info_new(ntconf_t * conf);

/* Utility functions */
static void file_attrib_free(file_attrib_t * f);
static void metadata_free(metadata_t * m);
static metadata_t * parse_metadata(const char * str);
static file_attrib_t * parse_file_attrib(const char * str);
/* Parse the frame metadata (given as text), say what number of
 * channels that we expect (nchannels) and where to put the coordinates (pos) */
static void parse_stagePosition(const char * frameMeta, int nchannels, double * pos);

static void check_stage_position(nd2info_t * info, int fov, int channel);

/* RAW metadata extraction without JSON parsing  */
static void showmeta(ntconf_t * conf, char * file);
static void showmeta_file(char *);
static void showmeta_coord(char *);
static void showmeta_frame(char *);
static void showmeta_text(char *);
static void showmeta_exp(char *);

/* Convert to tif and place in the outfolder. The outfolder has to
   exist. */
static int nd2_to_tiff(ntconf_t *, nd2info_t *);

/* Show XYZ coordinates of all images in csv format */
static void nd2_show_coordinates(nd2info_t * info);

/* Write some initial information to the log file, info->log */
static void hello_log(ntconf_t * conf, nd2info_t * info, int argc, char ** argv);

/* Misc */

static void show_help(char * binary_name);
static void print_version(FILE * fid);
static void print_web(FILE * fid);

/*
 * End of forward declarations
 * Start of code section
 */

#define NOT_NULL(x) {                                           \
        if(x == NULL){                                          \
            fprintf(stderr, "Got a NULL pointer at %s:%d\n",    \
                    __FILE__, __LINE__);                        \
            fprintf(stderr, "Can't continue :(\n");             \
            exit(EXIT_FAILURE);                                 \
        }                                                       \
    }                                                           \


static int
cli_get_int(const char * prompt,
            const int default_value,
            const int min_value,
            const int max_value)
{
    printf("%s (default=%d, min=%d, max=%d)\n",
           prompt,
           default_value,
           min_value, max_value);


    int niter = default_value;
    int accepted = 0;
    int invalid = 0;

    char *line = NULL;
    size_t len = 0;
    while(accepted == 0)
    {
        invalid = 0;
        printf("> ");
        ssize_t lineSize = getline(&line, &len, stdin);
        if(lineSize == -1)
        {
            exit(EXIT_FAILURE);
        }

        if(lineSize == 1) // i.e. '\n'
        {
            printf("Using default (%d)\n", default_value);
            niter = default_value;
            accepted = 1;
        } else {
            errno = 0;
            char * endptr = line;
            int ret = strtol(line, &endptr, 10);
            if( (errno == 0) & (endptr != line) )
            {
                niter = ret;
            } else {
                invalid = 1;
            }
        }

        if(niter < min_value || niter > max_value)
        {
            invalid = 1;
        }

        if(invalid == 1)
        {
            printf(" ! Invalid input, please provide a value in the range [%d, %d]\n",
                   min_value, max_value);
            accepted = 0;
        } else {
            accepted = 1;
        }
    }

    return niter;
}

char * cli_get_string(const char * prompt, const char * standard)
{
    if(standard)
    {
        printf("%s (default: %s)\n", prompt, standard);
    } else {
        printf("%s\n", prompt);
    }
    printf("> ");
    char *line = NULL;
    size_t len = 0;
    ssize_t lineSize = getline(&line, &len, stdin);
    if(lineSize == -1)
    {
        exit(EXIT_FAILURE);
    }

    if( (len == 0) | // no line allocated/returned
        (lineSize == 1) ) // just '\n'
    {
        free(line);
        if(standard)
        {
            printf("Using the defaults\n");
            line = strdup(standard);
            return line;
        } else {
            printf("No extra parameters given\n");
            line = strdup("");
            return line;
        }
    }
    // use provided line after stripping
    if(line[strlen(line)-1] == '\n')
    {
        line[strlen(line)-1] = '\0';
    }
    assert(line != NULL);
    return line;
}


/** @brief Print to the log file  */
static void
nd2info_log(nd2info_t * info, const char *fmt, ...)
{
    if(info->conf->dry)
    {
        return;
    }
    va_list argp;
    va_start(argp, fmt);
    vfprintf(info->log, fmt, argp);
    va_end(argp);
    return;
}


/** @brief Checked allocation */
static void *
ckcalloc(size_t nmemb, size_t size)
{
    void * p = calloc(nmemb, size);
    if(p == NULL)
    {
        char errstr[] = "nd2tool error: calloc returned NULL\n";

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        write(STDERR_FILENO, errstr, strlen(errstr));
#pragma GCC diagnostic pop

        exit(EXIT_FAILURE);
    }
    return p;
}


static void file_attrib_free(file_attrib_t * f)
{
    free(f);
}


static void metadata_free(metadata_t * m)
{
    if(m == NULL)
        return;

    for(int kk = 0; kk < m->nchannels; kk++)
    {
        channel_attrib_t * c = m->channels[kk];
        free(c->name);
        free(c->objectiveName);

        free(c);
    }
    free(m->channels);
    free(m);
}

static void nd2info_free(nd2info_t * n)
{
    free(n->filename);
    metadata_free(n->meta_att);
    file_attrib_free(n->file_att);
    free(n->loopstring);
    free(n->logfile);
    free(n->outfolder);

    if(n->log != NULL)
    {
        fclose(n->log);
    }
    free(n->error);

    if(n->meta_frame != NULL)
    {
        if(n->meta_frame->stagePositionUm != NULL)
        {
            free(n->meta_frame->stagePositionUm);
        }
        free(n->meta_frame);
    }
    free(n->camera_name);
    free(n->microscope_name);
    free(n);
}


static nd2info_t * nd2info_new(ntconf_t * conf)
{
    nd2info_t * info = ckcalloc(1, sizeof(nd2info_t));
    info->meta_frame = ckcalloc(1, sizeof(meta_frame_t));
    info->conf = conf;
    return info;
}


/** @breif Parse the result from Lim_FileGetMetadata */
static metadata_t * parse_metadata(const char * str)
{
    NOT_NULL(str);
    cJSON *j = cJSON_Parse(str);

    if (j == NULL)
    {
        fprintf(stderr, "Error parsing metadata (%s, line %d)\n", __FILE__, __LINE__);
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        fprintf(stderr, "Unable to continue\n");
        exit(EXIT_FAILURE);
    }

    metadata_t * m = ckcalloc(1, sizeof(metadata_t));

    const cJSON * j_contents = cJSON_GetObjectItemCaseSensitive(j, "contents");
    NOT_NULL(m);

    if(get_json_int(j_contents, "channelCount", &m->nchannels) != 0)
    {
        fprintf(stderr, "Unable to continue (%s, line %d)\n",
                __FILE__, __LINE__);
        free(m);
        exit(EXIT_FAILURE);
    }
    assert(m->nchannels > 0);

    m->channels = ckcalloc(m->nchannels, sizeof(channel_attrib_t*));

    for(int cc = 0; cc<m->nchannels; cc++)
    {
        m->channels[cc] = ckcalloc(1, sizeof(channel_attrib_t));

        m->channels[cc]->name = NULL;
    }

    cJSON * j_channel = NULL;
    cJSON * j_channels = cJSON_GetObjectItemCaseSensitive(j, "channels");
    if(j_channels == NULL)
    {
        fprintf(stderr, "Unable to find any channels in the nd2 file\n");
        exit(EXIT_FAILURE);
    }

    int cc = 0;
    cJSON_ArrayForEach(j_channel, j_channels)
    {
        if(cc >= m->nchannels)
        {
            fprintf(stderr, "Got conflicting number of channels\n");
            exit(EXIT_FAILURE);
        }

        cJSON * j_chan = cJSON_GetObjectItemCaseSensitive(j_channel, "channel");
        if(j_chan == NULL)
        {
            fprintf(stderr, "Unable to parse the JSON data. \n"
                    "Expected an object with name \"channel\" which was "
                    "not found.\n");
            exit(EXIT_FAILURE);
        }


        m->channels[cc]->name = get_json_string(j_chan, "name");
        NOT_NULL(m->channels[cc]->name);

        get_json_double(j_chan,
                        "emissionLambdaNm",
                        &m->channels[cc]->emissionLambdaNm);

        { /* Pixel size and image size */
            cJSON * j_vol = cJSON_GetObjectItemCaseSensitive(j_channel,
                                                             "volume");

            cJSON * j_ax = cJSON_GetObjectItemCaseSensitive(j_vol,
                                                            "axesCalibration");
            cJSON * j_ax_0 = cJSON_GetArrayItem(j_ax, 0);
            assert(cJSON_IsNumber(j_ax_0));
            m->channels[cc]->dx_nm = 1000.0*j_ax_0->valuedouble;
            cJSON * j_ax_1 = cJSON_GetArrayItem(j_ax, 1);
            assert(cJSON_IsNumber(j_ax_1));
            m->channels[cc]->dy_nm = 1000.0*j_ax_1->valuedouble;
            cJSON * j_ax_2 = cJSON_GetArrayItem(j_ax, 2);
            assert(cJSON_IsNumber(j_ax_2));
            m->channels[cc]->dz_nm = 1000.0*j_ax_2->valuedouble;

            cJSON * j_vox = cJSON_GetObjectItemCaseSensitive(j_vol,
                                                             "voxelCount");
            cJSON * j_temp = cJSON_GetArrayItem(j_vox, 0);
            m->channels[cc]->M = j_temp->valueint;
            j_temp = cJSON_GetArrayItem(j_vox, 1);
            m->channels[cc]->N = j_temp->valueint;
            j_temp = cJSON_GetArrayItem(j_vox, 2);
            m->channels[cc]->P = j_temp->valueint;

        }
        { /* Optical configuration*/
            cJSON * j_mic = cJSON_GetObjectItemCaseSensitive(j_channel,
                                                             "microscope");
            if(j_mic == NULL)
            {
                printf("Warning: Could not find channel/microscope\n");
                goto done_optical_configuration;
            }
            cJSON * j_tmp = cJSON_GetObjectItemCaseSensitive(j_mic,
                                                             "immersionRefractiveIndex");

            if(j_tmp == NULL)
            {
                printf("Warning: Could not find channel/microscope/immersionRefractiveIndex\n");
            } else {
                m->channels[cc]->immersionRefractiveIndex = j_tmp->valuedouble;
            }

            j_tmp = cJSON_GetObjectItemCaseSensitive(j_mic,
                                                     "objectiveNumericalAperture");
            if(j_tmp == NULL)
            {
                printf("Warning: Could not find channel/microscope/objectiveNumericalAperture\n");
            } else {
                m->channels[cc]->objectiveNumericalAperture = j_tmp->valuedouble;
            }

            j_tmp = cJSON_GetObjectItemCaseSensitive(j_mic,
                                                     "objectiveMagnification");
            if(j_tmp == NULL)
            {
                printf("Warning: Could not find channel/microscope/objectiveMagnification\n");
            } else {
                m->channels[cc]->objectiveMagnification = j_tmp->valuedouble;
            }

            j_tmp = cJSON_GetObjectItemCaseSensitive(j_mic,
                                                     "objectiveName");

            if(j_tmp == NULL)
            {
                printf("Warning: Could not find channel/microscope/objectiveName\n");
            } else {
                m->channels[cc]->objectiveName = strdup(j_tmp->valuestring);
            }

        done_optical_configuration: ;

        }
        cc++;
    }

    cJSON_Delete(j);
    return m;
}


/** @brief Parse the result from Lim_FileGetMetadata */
static file_attrib_t * parse_file_attrib(const char * str)
{
    cJSON *json = cJSON_Parse(str);

    if (json == NULL)
    {
        fprintf(stderr, "Error parsing file attributes\n");
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        exit(EXIT_FAILURE);
    }

    file_attrib_t * attrib = ckcalloc(1, sizeof(file_attrib_t));

    get_json_int(json, "heightPx",
                 &attrib->heightPx);
    get_json_int(json, "widthPx",
                 &attrib->widthPx);
    get_json_int(json, "sequenceCount",
                 &attrib->sequenceCount);
    get_json_int(json, "componentCount",
                 &attrib->componentCount);
    get_json_int(json, "bitsPerComponentInMemory",
                 &attrib->bitsPerComponentInMemory);
    get_json_int(json, "bitsPerComponentSignificant",
                 &attrib->bitsPerComponentSignificant);

    cJSON_Delete(json);
    return attrib;
}


static void check_cmd_line(int argc, char ** argv)
{
    if(argc < 2)
    {
        show_help(argv[0]);
        exit(EXIT_FAILURE);
    }
    return;
}


static void nd2info_set_outfolder(nd2info_t * info)
{
    free(info->outfolder);
    info->outfolder = strdup(info->filename);
    char * outfolder = strdup(basename(info->outfolder));
    free(info->outfolder);
    info->outfolder = outfolder;
    info->outfolder = basename(info->outfolder);
    remove_file_ext(info->outfolder);
    return;
}


static nd2info_t * nd2info(ntconf_t * conf, const char * file)
{
    nd2info_t * info = nd2info_new(conf);
    NOT_NULL(info);
    info->filename = strdup(file);
    NOT_NULL(info->filename);

    struct stat stats;
    if(stat(info->filename, &stats) != 0)
    {
        size_t slen = strlen(file) + 128;
        info->error = ckcalloc(slen, 1);
        snprintf(info->error, slen, "Can't open %s\n", file);
        return info;
    }

    void * nd2 = Lim_FileOpenForReadUtf8(file);
    if(nd2 == NULL)
    {
        size_t slen = strlen(file) + 128;
        info->error = ckcalloc(slen, 1);
        snprintf(info->error, slen,
                 "%s is not a valid nd2 file\n", file);
        return info;
    }

    //LIMFILEAPI LIMSIZE         Lim_FileGetCoordSize(LIMFILEHANDLE hFile);
    int csize = Lim_FileGetCoordSize(nd2);
    if(conf->verbose > 2)
    {
        printf("# Lim_FileGetCoordSize\n");
        printf("%d\n", csize);
    }

    if(csize < 1)
    {
        size_t slen = strlen(file) + 128;
        info->error = ckcalloc(slen, 1);
        snprintf(info->error, slen,
                 "Error: Can't find coordinates in %s\n", file);
        Lim_FileClose(nd2);
        return info;
    }

    int nFOV = 1;
    int nPlanes = 0;

    for(int kk = 0; kk<csize; kk++)
    {
        int buffsize = 1024;
        char * buffer = ckcalloc(buffsize, 1);

        int dsize = Lim_FileGetCoordInfo(nd2, kk, buffer, buffsize);

        if(conf->verbose > 2)
        {
            printf("# Lim_FileGetCoordInfo for dim %d\n", kk);
            printf("%d\n %s\n", dsize, buffer);
        }

        if(strcmp(buffer, "XYPosLoop") == 0)
        {
            nFOV = dsize;
        }

        if(strcmp(buffer, "ZStackLoop") == 0)
        {
            nPlanes = dsize;
        }

        if(kk+1 == csize)
        {

            if(strcmp(buffer, "ZStackLoop") == 0)
            {
                if(conf->verbose > 2)
                {
                    printf("Last loop is of type ZStackLoop -- good!\n");
                }
            } else {
                if(conf->verbose > 0)
                {
                    printf("WARNING: last loop is %s, expected ZStackLoop\n", buffer);
                }
            }
        }
        free(buffer);
    }

    if(nPlanes < 1)
    {
        size_t slen = strlen(file) + 128;
        info->error = ckcalloc(slen, 1);
        snprintf(info->error, slen,
                 "Error: Can't find any image planes %s\n", file);
        Lim_FileClose(nd2);
        return info;
    }

    info->nFOV = nFOV;

    int seqCount = Lim_FileGetSeqCount(nd2);

    char * fileAttributes = Lim_FileGetAttributes(nd2);
    info->file_att = parse_file_attrib(fileAttributes);
    Lim_FileFreeString(fileAttributes);

    char * fileMeta = Lim_FileGetMetadata(nd2);
    info->meta_att = parse_metadata(fileMeta);
    Lim_FileFreeString(fileMeta);

    int nchannel = info->meta_att->nchannels;
    if(conf->shake)
    {
        info->meta_frame->stagePositionUm = ckcalloc(3*seqCount*nchannel,
                                                     sizeof(double));
    }

    /* Information per xyz (all channels) */
    for(int kk = 0; kk<seqCount; kk++)
    {
        char * frameMeta = Lim_FileGetFrameMetadata(nd2, kk);
        if(conf->verbose > 2)
        {
            printf("# FileGetFrameMetadata for image %d\n", kk);
            printf("%s\n", frameMeta);
        }

        if(conf->shake)
        {
            parse_stagePosition(frameMeta, nchannel,
                                info->meta_frame->stagePositionUm + 3*kk*nchannel); // offset
        }
        Lim_FileFreeString(frameMeta);
    }

    /* Lim_FileGetTextinfo does not return JSON. It contains
     * information about sensor, camera, scope, tempertures etc.  Also
     * a line like "Dimensions: XY(11) x λ(2) x Z(51)" -- the loop
     * order. Can that be determined from the other metadata?  Note
     * that the line like "- Step: 0.3 µm" is rounded to 1 decimal and
     * should not be used.  Could be done: Initial parsing as JSON,
     * then we don't need the filter_textinfo function. See how it is
     * done for camera name etc below.
     */
    char * textinfo = Lim_FileGetTextinfo(nd2);
    filter_textinfo(textinfo); /* Output filled with '\r\n\' written out as text */
    if(conf->verbose > 2)
    {
        printf("# Lim_FileGetTextinfo\n");
        printf("%s\n", textinfo);
    }

    /* The loopstring is not always available
     * that information should be available in meta-exp? */
    char * token = strtok(textinfo, "\n");
    while( token != NULL ) {
        if(strncmp(token, "Dimensions:", 11) == 0)
        {
            info->loopstring = strdup(token);
        }
        token = strtok(NULL, "\n");
    }
    free(token);

    if(info->loopstring == NULL)
    {
        info->loopstring = strdup("Not available");
    }
    Lim_FileFreeString(textinfo);

    /* Look for camera name and microscope name This part is fragile
     * to how the meta data was written since it isn't structured
     * beyond the root nodes. Here we perform basic string search in the "description" object.
     *
     * */

    char * textinfo2 = Lim_FileGetTextinfo(nd2);
    cJSON *j2 = cJSON_Parse(textinfo2);
    if(j2 != NULL)
    {
        char * j_desc = get_json_string(j2, "description");
        // Now loop over the lines and print matching...
        // Camera Name: Andor Sona CSC-00633
        // Microscope Settings:   Microscope: Ti2 Microscope
        char * saveptr;
        char * line = strtok_r(j_desc, "\n", &saveptr);
        while(line != NULL)
        {
            if(strlen(line) > 15)
            {
                if(strncmp(line, "Camera Name:", 11) == 0)
                {
                    free(info->camera_name);
                    info->camera_name = strdup(line+13);
                }
            }
            if(strlen(line) > 40)
            {
                if(strncmp(line,
                           " Microscope Settings:   Microscope: ",
                           36) == 0)
                {
                    free(info->microscope_name);
                    info->microscope_name = strdup(line+36);
                }
            }
            line = strtok_r(NULL, "\n", &saveptr);
        }
        free(line);
        free(j_desc);
        cJSON_Delete(j2);
    }
    Lim_FileFreeString(textinfo2);


    /* This is interesting. The order probably plays a role here.
     * relevant to us is probably only the case where there are two items,
     * 1) For looping over pre-set start positions (type=XYPosLoop)
     * 2) For looping in z (type=ZStackLoop)
     * Is it possible to see where the looping over colors occur based on this?
     */

    char * expinfo = Lim_FileGetExperiment(nd2);
    Lim_FileFreeString(expinfo);

    Lim_FileClose(nd2);

    nd2info_set_outfolder(info);
    return info;
}

/** @brief Open and return an ND2 file
 * @return NULL on failure
 */
static void *
open_nd2(ntconf_t * conf, char * filename)
{
    /* Check that the nd2 file exists */
    {
        struct stat stats;
        if(stat(filename, &stats) != 0)
        {
            if(conf->verbose > 0)
            {
                fprintf(stderr, "Can't open %s\n", filename);
            }
            return NULL;
        }
    }

    /* Please note that this reports true also for tif files */
    void * nd2 = Lim_FileOpenForReadUtf8(filename);
    if(nd2 == NULL)
    {
        if(conf->verbose > 0)
        {
            fprintf(stderr, "%s is not a valid nd2 file\n", filename);
        }
        return NULL;
    }
    return nd2;
}


/* Create set info->outfolder and create that folder in the file system.
 * sets info->outfolder to NULL upon failure.  */
static void ensure_output_folder(ntconf_t * conf, nd2info_t * info)
{
    nd2info_set_outfolder(info);
    if(conf->verbose > 2)
    {
        printf("Will create folder '%s'\n", info->outfolder);
    }

    if(conf->dry)
    {
        return;
    }

    if (mkdir(info->outfolder, 0777) == -1)
    {
        if(errno != EEXIST)
        {
            fprintf(stdout,
                    "Error: Unable to create the folder %s\n",
                    info->outfolder);
            free(info->outfolder);
            info->outfolder = NULL;
        }
    }

    return;
}

/** @brief Write an ND2 file as one file per FOV and channel. Default option. */
static void nd2_to_tiff_splitC(void * nd2, ntconf_t * conf, nd2info_t * info)
{

    /* Prepare metadata for the tiff files */
    ttags * tags = ttags_new();
    {
        size_t slen = 1024;
        char * sw_string = ckcalloc(slen, 1);
        snprintf(sw_string, slen,
                 "github.com/elgw/nd2tool source image: %s",
                 info->filename);
        ttags_set_software(tags , sw_string);
        free(sw_string);
    }

    int nchan = info->meta_att->nchannels;
    int M = info->meta_att->channels[0]->M;
    int N = info->meta_att->channels[0]->N;
    int P = info->meta_att->channels[0]->P;

    ttags_set_imagesize(tags, M, N, P);
    ttags_set_pixelsize_nm(tags,
                           info->meta_att->channels[0]->dx_nm,
                           info->meta_att->channels[0]->dy_nm,
                           info->meta_att->channels[0]->dz_nm);


    /* We choose to extract the image data multiple times and only
     * collect pixels from one channel at a time. This is of course
     * slightly slower than extracting all channels for a given FOV at
     * a time but gives more predictable memory usage. */

    /* Buffer for one slice and one color */
    uint16_t * S = ckcalloc(M*N, sizeof(uint16_t));
    LIMPICTURE * pic = ckcalloc(1, sizeof(LIMPICTURE));
    Lim_InitPicture(pic, M, N, 16, nchan);

    for(i64 ff = 0; ff<info->nFOV; ff++) /* For each FOV */
    {
        if(conf->fov_string != NULL)
        {
            if(atoi(conf->fov_string) != (ff+1))
            {
                continue;
            }
        }

        for(i64 cc = 0; cc<nchan; cc++) /* For each channel */
        {
            /* Write out to disk */

            size_t slen = 1024;
            char * outname = ckcalloc(slen, 1);
            snprintf(outname, slen,
                     "%s/%s_%03" PRId64 ".tif", info->outfolder,
                     info->meta_att->channels[cc]->name, ff+1);

	    if(conf->verbose > 1)
            {
		printf("outfolder: %s\n", info->outfolder);
		printf("channel name: %s\n", info->meta_att->channels[cc]->name);
            }
            printf("%s ", outname);
            nd2info_log(info, "%s ", outname);

            if(conf->shake)
            {
                check_stage_position(info, ff, cc);
            }

            if(conf->overwrite == 0)
            {
                if(isfile(outname))
                {
                    printf("-- skipping, file exists\n");
                    nd2info_log(info, "-- skipping, file exists\n");

                    goto next_file;
                }
            }
            if(conf->verbose > 0)
            {
                printf("... writing ... "); fflush(stdout);
            }

            if(conf->dry)
            {
                printf(" (--dry, not writing)\n");
                goto next_file;
            }

            /* Create temporary file */
            slen = strlen(outname) + 16;
            char * outname_tmp = ckcalloc(slen, 1);

            snprintf(outname_tmp, slen,
                     "%s_tmp_XXXXXX", outname);
            int tfid = 0;
            if((tfid = mkstemp(outname_tmp)) == -1)
            {
                fprintf(stderr, "Failed to create a temporary file based on pattern: %s\n", outname_tmp);
                exit(EXIT_FAILURE);
            }
            close(tfid);

            tiff_writer_t * tw = tiff_writer_init(outname_tmp, tags, M, N, P);

            i64 p0 = 0;
            i64 p1 = P;
            /* For each plane */
            if(conf->use_range)
            {
                p0 = conf->range_from-1;
                p1 = conf->range_to;
                if(p0 < 0)
                {
                    printf("Invalid slice range\n");
                    exit(EXIT_FAILURE);
                }
                if(p1 > P)
                {
                    printf("Invalid slice range\n");
                    exit(EXIT_FAILURE);
                }
            }

            for(i64 kk = p0; kk < p1; kk++)
            {
                /* Returns interlaced data */
                int res = Lim_FileGetImageData(nd2,
                                               kk + ff*P, //uiSeqIndex,
                                               pic);
                if(res != 0)
                {
                    fprintf(stderr, "Failed to read from %s. At line %d\n",
                            info->filename, __LINE__);
                }

                if( (pic->pImageData == NULL) || (pic->uiSize == 0) )
                {
                    fprintf(stderr, "Failed to retrieve image data\n");
                }
                uint16_t * pixels = (uint16_t *) pic->pImageData;
                if(pixels == NULL)
                {
                    fprintf(stderr, "No pixel data could be found in the image\n");
                    exit(EXIT_FAILURE);
                }

                for(i64 pp = 0; pp<M*N; pp++)
                {
                    S[pp] = pixels[pp*nchan+cc];
                }
                tiff_writer_write(tw, S);
            } // kk

            /* Finish this image */
            tiff_writer_finish(tw);
            rename(outname_tmp, outname);
            if(conf->verbose > 0)
            {
                printf("done\n");
            }
            nd2info_log(info, "\n");
            free(outname_tmp);
        next_file: ;
            free(outname);

        } // cc
    }// ff

    Lim_DestroyPicture(pic);
    free(pic);

    free(S);
    ttags_free(&tags);
}


/** @brief Write an ND2 file as one file per FOV and channel */
static void nd2_to_tiff_splitC_splitZ(void * nd2, ntconf_t * conf, nd2info_t * info)
{

    /* Prepare metadata for the tiff files */
    ttags * tags = ttags_new();
    {
        size_t slen = 1024;
        char * sw_string = ckcalloc(slen, 1);
        snprintf(sw_string, slen,
                "github.com/elgw/nd2tool source image: %s",
                info->filename);
        ttags_set_software(tags , sw_string);
        free(sw_string);
    }

    int nchan = info->meta_att->nchannels;
    int M = info->meta_att->channels[0]->M;
    int N = info->meta_att->channels[0]->N;
    int P = info->meta_att->channels[0]->P;

    ttags_set_imagesize(tags, M, N, 1);
    ttags_set_pixelsize_nm(tags,
                           info->meta_att->channels[0]->dx_nm,
                           info->meta_att->channels[0]->dy_nm,
                           info->meta_att->channels[0]->dz_nm);


    /* We choose to extract the image data multiple times and only
     * collect pixels from one channel at a time. This is of course
     * slightly slower than extracting all channels for a given FOV at
     * a time but gives more predictable memory usage. */

    /* Buffer for one slice and one color */
    uint16_t * S = ckcalloc(M*N, sizeof(uint16_t));
    LIMPICTURE * pic = ckcalloc(1, sizeof(LIMPICTURE));
    Lim_InitPicture(pic, M, N, 16, nchan);

    for(i64 ff = 0; ff<info->nFOV; ff++) /* For each FOV */
    {
        if(conf->fov_string != NULL)
        {
            if(atoi(conf->fov_string) != (ff+1))
            {
                continue;
            }
        }

        for(i64 cc = 0; cc<nchan; cc++) /* For each channel */
        {
            for(i64 kk = 0; kk<P; kk++) /* For each plane */
            {

                /* Write out to disk */
                size_t slen = 1024;
                char * outname = ckcalloc(slen, 1);
                /* SpaceTx
                 * <image_type>-f<fov_id>-r<round_label>-c<ch_label>-z<zplane_label>.
                 * Example: nuclei-f0-r2-c3-z33.tiff
                 */
                snprintf(outname, slen,
                         "%s/%s_f%" PRId64 "-r%d-c%" PRIu64 "-z%" PRIu64 ".tif",
                         info->outfolder,
                         info->outfolder, /* <image_type> */
                         ff, /* <fov_id> */
                         0, /* <round_label> */
                         cc, /* <ch_label> */
                         kk); /* <zplane_label> */

                printf("%s ", outname);
                nd2info_log(info, "%s ", outname);

                if(conf->shake)
                {
                    check_stage_position(info, ff, cc);
                }

                if(conf->overwrite == 0)
                {
                    if(isfile(outname))
                    {
                        printf("-- skipping, file exists\n");
                        nd2info_log(info, "-- skipping, file exists\n");

                        goto next_file;
                    }
                }
                if(conf->verbose > 0)
                {
                    printf("... writing ... "); fflush(stdout);
                }

                if(conf->dry)
                {
                    printf(" (--dry, not writing)\n");
                    goto next_file;
                }

                /* Create temporary file */
                slen = strlen(outname) + 32;
                char * outname_tmp = ckcalloc(slen, 1);

                snprintf(outname_tmp, slen,
                         "%s_tmp_XXXXXX", outname);
                int tfid = 0;
                if((tfid = mkstemp(outname_tmp)) == -1)
                {
                    fprintf(stderr, "Failed to create a temporary file based on pattern: %s\n", outname_tmp);
                    exit(EXIT_FAILURE);
                }
                close(tfid);

                tiff_writer_t * tw = tiff_writer_init(outname_tmp, tags, M, N, 1);

                /* Returns interlaced data */
                int res = Lim_FileGetImageData(nd2,
                                               kk + ff*P, //uiSeqIndex,
                                               pic);
                if(res != 0)
                {
                    fprintf(stderr, "Failed to read from %s. At line %d\n",
                            info->filename, __LINE__);
                }

                if( (pic->pImageData == NULL) || (pic->uiSize == 0) )
                {
                    fprintf(stderr, "Failed to retrieve image data\n");
                }
                uint16_t * pixels = (uint16_t *) pic->pImageData;

                for(i64 pp = 0; pp<M*N; pp++)
                {
                    S[pp] = pixels[pp*nchan+cc];
                }
                tiff_writer_write(tw, S);


                /* Finish this image */
                tiff_writer_finish(tw);
                rename(outname_tmp, outname);
                if(conf->verbose > 0)
                {
                    printf("done\n");
                }
                nd2info_log(info, "\n");
                free(outname_tmp);
            next_file: ;
                free(outname);
            } // kk
        } // cc
    }// ff

    Lim_DestroyPicture(pic);
    free(pic);

    free(S);
    ttags_free(&tags);
}


/** @brief Write an ND2 file as composite tif files
 *
 * One file per FOV
 */
static void
nd2_to_tiff_composite(void * nd2, ntconf_t * conf, nd2info_t * info)
{

    /* Prepare metadata for the tiff files */
    ttags * tags = ttags_new();
    {
        size_t slen = 1024;
        char * sw_string = ckcalloc(slen, 1);
        snprintf(sw_string, slen,
                 "github.com/elgw/nd2tool source image: %s",
                 info->filename);
        ttags_set_software(tags , sw_string);
        free(sw_string);
    }
    int nchan = info->meta_att->nchannels;
    int M = info->meta_att->channels[0]->M;
    int N = info->meta_att->channels[0]->N;
    int P = info->meta_att->channels[0]->P;

    ttags_set_imagesize(tags, M, N, P);
    ttags_set_pixelsize_nm(tags,
                           info->meta_att->channels[0]->dx_nm,
                           info->meta_att->channels[0]->dy_nm,
                           info->meta_att->channels[0]->dz_nm);

    ttags_set_composite(tags, nchan);

    // TODO: Set the extra tags needed for composite images.

    /* We choose to extract the image data multiple times and only
     * collect pixels from one channel at a time. This is of course
     * slightly slower than extracting all channels for a given FOV at
     * a time but gives more predictable memory usage. */

    /* Buffer for one slice and one color */
    uint16_t * S = ckcalloc(M*N, sizeof(uint16_t));

    LIMPICTURE * pic = ckcalloc(sizeof(LIMPICTURE), 1);
    Lim_InitPicture(pic, M, N, 16, nchan);



    for(i64 ff = 0; ff<info->nFOV; ff++) /* For each FOV */
    {
        if(conf->fov_string != NULL)
        {
            if(atoi(conf->fov_string) != (ff+1))
            {
                continue;
            }
        }

        /* Write out to disk */
        size_t slen = 1024;
        char * outname = ckcalloc(slen, 1);
        snprintf(outname, slen,
                 "%s/%s_%03" PRId64 ".tif", info->outfolder,
                 "composite", ff+1);

        printf("%s ", outname);
        nd2info_log(info, "%s ", outname);

        if(conf->overwrite == 0)
        {
            if(isfile(outname))
            {
                printf("-- skipping, file exists\n");
                nd2info_log(info, "-- skipping, file exists\n");
                goto next_file;
            }
        }
        if(conf->verbose > 0)
        {
            printf("... writing ... "); fflush(stdout);
        }

        /* Create temporary file */
        slen = strlen(outname) + 16;
        char * outname_tmp = ckcalloc(slen, 1);
        snprintf(outname_tmp, slen,
                 "%s_tmp_XXXXXX", outname);
        int tfid = 0;
        if((tfid = mkstemp(outname_tmp)) == -1)
        {
            fprintf(stderr, "Failed to create a temporary file based on pattern: %s\n", outname_tmp);
            exit(EXIT_FAILURE);
        }
        close(tfid);

        tiff_writer_t * tw = tiff_writer_init(outname_tmp, tags, M, N, P*nchan);

        for(i64 kk = 0; kk<P; kk++) /* For each plane */
        {
            for(i64 cc = 0; cc<nchan; cc++) /* For each channel */
            {

                /* Returns interlaced data */
                int res = Lim_FileGetImageData(nd2,
                                               kk + ff*P, //uiSeqIndex,
                                               pic);
                if(res != 0)
                {
                    fprintf(stderr, "Failed to read from %s. At line %d\n",
                            info->filename, __LINE__);
                }

                if( (pic->pImageData == NULL) || (pic->uiSize == 0) )
                {
                    fprintf(stderr, "Failed to retrieve image data\n");
                }
                uint16_t * pixels = (uint16_t *) pic->pImageData;
                if(pixels == NULL)
                {
                    fprintf(stderr, "Failed to get pixels from the image\n");
                    exit(EXIT_FAILURE);
                }

                for(i64 pp = 0; pp<M*N; pp++)
                {
                    S[pp] = pixels[pp*nchan+cc];
                }
                tiff_writer_write(tw, S);
            } // cc

        } // kk
        /* Finish this image */
        tiff_writer_finish(tw);
        rename(outname_tmp, outname);
        if(conf->verbose > 0)
        {
            printf("done\n");
        }
        nd2info_log(info, "\n");
        free(outname_tmp);
    next_file: ;
        free(outname);

    }// ff

    Lim_DestroyPicture(pic);
    free(pic);

    free(S);
    ttags_free(&tags);
}


/** @brief Try to convert an ND2 file to tif
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int
nd2_to_tiff(ntconf_t * conf, nd2info_t * info)
{
    void * nd2 = open_nd2(conf, info->filename);
    if(nd2 == NULL)
    {
        fprintf(stderr, "Failed to read from %s\n", info->filename);
        return EXIT_FAILURE;
    }

    ensure_output_folder(conf, info);
    if(info->outfolder == NULL)
    {
        fprintf(stderr, "Failed to create the output folder\n");
        return EXIT_FAILURE;
    }

    if(info->file_att->bitsPerComponentInMemory != 16)
    {
        fprintf(stderr, "Can only convert files with 16 bit per pixel.\n"
                "This file has %d\n",
                info->file_att->bitsPerComponentInMemory);
        return EXIT_FAILURE;
    }
    size_t slen = strlen(info->outfolder) + 128;
    info->logfile = ckcalloc(slen, 1);
    snprintf(info->logfile, slen,
             "%s/nd2tool.log.txt",
             info->outfolder);

    if(conf->verbose > 1)
    {
        printf("Log file %s\n", info->logfile);
    }

    if(conf->dry)
    {
        printf("Not creating log file (%s) (--dry)\n", info->logfile);
    } else {
        info->log = fopen(info->logfile, "a");
        if(info->log == NULL)
        {
            fprintf(stderr, "Unable to create the log file: %s\n",
                    info->logfile);
            exit(EXIT_FAILURE);
        }
        /* A newline will separate what is written to the log this time to
           what was already written. */
        nd2info_log(info, "\n");
    }

    if(conf->composite)
    {
        nd2_to_tiff_composite(nd2, conf, info);
    } else {
        if(conf->save_individual_planes)
        {
            nd2_to_tiff_splitC_splitZ(nd2, conf, info);
        } else {
            nd2_to_tiff_splitC(nd2, conf, info);
        }
    }

    Lim_FileClose(nd2);
    return EXIT_SUCCESS;
}

static void
parse_stagePosition(const char * frameMeta, int nchannels, double * pos)
{

    cJSON *json = cJSON_Parse(frameMeta);
    if (json == NULL)
    {
        fprintf(stderr, "Error parsing stagePosition\n");
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        exit(EXIT_FAILURE);
    }
    const cJSON * j_channels = cJSON_GetObjectItemCaseSensitive(json,
                                                                "channels");
    /* TODO: Validate that this agrees with the information about the
       number of channels that we got from somewhere else  */
    int nchannels_validation = cJSON_GetArraySize(j_channels);

    if(nchannels != nchannels_validation)
    {
        fprintf(stderr, "Inconsistent metadata. Are there %d or %d channels?",
                nchannels, nchannels_validation);
        exit(EXIT_FAILURE);
    }

    for(int cc = 0; cc<nchannels; cc++)
    {
        const cJSON * j_chan = cJSON_GetArrayItem(j_channels, cc);
        assert(j_channels != NULL);
        const cJSON * j_position = cJSON_GetObjectItemCaseSensitive(j_chan,
                                                                    "position");
        assert(j_position != NULL);
        const cJSON * j_stagePositionUm = cJSON_GetObjectItemCaseSensitive(j_position,
                                                                           "stagePositionUm");
        assert(j_stagePositionUm != NULL);

        const cJSON * j_x = cJSON_GetArrayItem(j_stagePositionUm, 0);
        assert(j_x != NULL);
        assert(cJSON_IsNumber(j_x));

        const cJSON * j_y = cJSON_GetArrayItem(j_stagePositionUm, 1);
        assert(j_y != NULL);
        assert(cJSON_IsNumber(j_y));

        const cJSON * j_z = cJSON_GetArrayItem(j_stagePositionUm, 2);
        assert(j_z != NULL);
        assert(cJSON_IsNumber(j_z));
        pos[3*cc] = j_x->valuedouble;
        pos[3*cc+1] = j_y->valuedouble;
        pos[3*cc+2] = j_z->valuedouble;

        //printf("%f, %f ,%f,\n", j_x->valuedouble, j_y->valuedouble, j_z->valuedouble);
    }
    cJSON_Delete(json);
    return;
}


static void
nd2info_print(const ntconf_t * conf, FILE * fid, const nd2info_t * info)
{
    if(conf->dry)
    {
        if(fid == NULL)
        {
            return;
        }
    }
    metadata_t * meta = info->meta_att;
    int nFOV = info->nFOV;
    fprintf(fid, "%d FOV in %d channels:\n", nFOV, meta->nchannels);

    int max_chan_chars = 3;
    for(int cc = 0; cc < meta->nchannels; cc++)
    {
        if(meta->channels[cc]->name != NULL)
        {
            int len = strlen(meta->channels[cc]->name);
            len > max_chan_chars ? max_chan_chars = len : 0;
        }
    }

    for(int cc = 0; cc < meta->nchannels; cc++)
    {
        double lambda = meta->channels[cc]->emissionLambdaNm;
        double RGB[3] = {0,0,0};
        double l = lambda;
        l > 650 ? l = 650 : 0;
        l < 425 ? l = 425 : 0;
        srgb_from_lambda(l, RGB);


        fprintf(fid, "   #%d '%*s', λ_em=%.1f",
                cc+1,
                max_chan_chars,
                meta->channels[cc]->name,
                meta->channels[cc]->emissionLambdaNm);

        fprintf(fid, " #%02X%02X%02X ",
                (int) round(255.0*RGB[0]),
                (int) round(255.0*RGB[1]),
                (int) round(255.0*RGB[2]));
        show_color(fid, RGB, lambda);
        fprintf(fid, "\n");

    }

    int cc = 0;
    fprintf(fid, "Bits per pixel: %d, significant: %d\n",
            info->file_att->bitsPerComponentInMemory,
            info->file_att->bitsPerComponentSignificant);
    fprintf(fid, "dx=%.1f nm, dy=%.1f nm, dz=%.1f nm\n",
            meta->channels[cc]->dx_nm,
            meta->channels[cc]->dy_nm,
            meta->channels[cc]->dz_nm);
    fprintf(fid, "NA=%.3f, ni=%.3f\n",
            meta->channels[cc]->objectiveNumericalAperture,
            meta->channels[cc]->immersionRefractiveIndex);
    fprintf(fid, "Objective Name: %s\nObjective Magnification: %.1fX\n",
            meta->channels[cc]->objectiveName,
            meta->channels[cc]->objectiveMagnification);
    fprintf(fid, "Volume size: %d x %d x %d\n",
            meta->channels[cc]->M,
            meta->channels[cc]->N,
            meta->channels[cc]->P);
    fprintf(fid, "Looping: %s\n", info->loopstring);
    if(info->camera_name)
    {
        fprintf(fid, "Camera: %s\n", info->camera_name);
    }
    if(info->microscope_name)
    {
        fprintf(fid, "Microscope: %s\n", info->microscope_name);
    }
    return;
}


static void print_version(FILE * fid)
{
    fprintf(fid, "nd2tool v.%s.%s.%s",
            ND2TOOL_VERSION_MAJOR,
            ND2TOOL_VERSION_MINOR,
            ND2TOOL_VERSION_PATCH);
#ifdef ND2TOOL_GIT_VERSION
    fprintf(fid, " git:%s\n", ND2TOOL_GIT_VERSION);
#endif
    fprintf(fid, "\n");
    fprintf(fid, "TIFF: '%s'\n", TIFFGetVersion());
    return;
}


static void print_web(FILE * fid)
{
    fprintf(fid, "Web page: <https://www.github.com/elgw/nd2tool>\n");
    return;
}


static void show_version()
{
    print_version(stdout);
    print_web(stdout);
    return;
}


static void show_help(char * name)
{
    ntconf_t * conf = ntconf_new();

    printf("Usage: ");
    printf("%s [--info] [--coords] [--help] file1.nd2 file2.nd2 ...\n",
           name);
    printf("Convert Nikon nd2 file(s) to tif file(s) or just show some metadata.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -i, --info \n\t Just show brief info about the file(s) and then quit.\n");
    printf("  -v, --verbose l\n\t Set verbosity level l. Default: %d.\n",
           conf->verbose);
    printf("  -V, --version\n\t Show version info and quit.\n");
    printf("  -h, --help\n\t Show this message and quit\n");
    printf("  -o, --overwrite\n\t Overwrite existing tif files. Default: %d.\n",
           conf->overwrite);
    printf("  -c, --coord\n\t Show coordinates in csv format for all z-planes\n");
    printf("  -s, --shake\n\t Enable experimental shake detection\n");
    printf("  --fov n\n\t Only extract Field Of View #n\n");
    printf("  --slice '[a, b]'\n\t"
           "Where range is a json array, for example [2, 10]\n\t"
           "Only extract slices in the 1-indexed range [a, b]\n");
    printf("  -C, --composite\n\t Don't split by channel\n");
    printf("  --deconwolf\n\t"
           "for each file, generate a script to run deconwolf\n");
    printf("  --deconwolfx\n\t"
           "like --deconwolf but will ask for some parameters interactively\n");
    printf(" --deconwolf_dots\n\t"
           "write a script for dot detection with `dw dots`\n");
    printf("  --dry\n\t"
           "Perform a dry run, i.e. do not write files or create folders\n");
    printf("  --SpaceTx\n\t"
           "Save one image per z-plane according to the SpaceTx convention.\n\t"
           "<image_type>-f<fov_id>-r<round_label>-c<ch_label>-z<zplane_label>\n\t"
           "<image_type> will be the name of the nd2file (without extension)\n\t"
           "<round_label> will always be 0.\n\t");
    printf("\n");
    printf("Raw meta data extraction to stdout:\n");
    printf("  --meta\n\t all metadata.\n");
    printf("  --meta-file\n\t Lim_FileGetMetadata JSON.\n");
    printf("  --meta-coord\n\t Lim_FileGetCoordInfo text.\n");
    printf("  --meta-frame\n\t Lim_FileGetFrameMetadata JSON.\n");
    printf("  --meta-text\n\t Lim_FileGetTextinfo text.\n");
    printf("  --meta-exp\n\t Lim_FileGetExperiment JSON.\n");
    printf("\n");
    printf("See the man page for more information or the\n");
    printf("web page <https://www.github.com/elgw/nd2tool>\n");
    ntconf_free(conf);
    return;
}


static ntconf_t * ntconf_new(void)
{
    ntconf_t * conf = ckcalloc(1, sizeof(ntconf_t));
    conf->verbose = 1;
    conf->convert = 1;
    conf->showinfo = 1;
    conf->purpose = CONVERT_TO_TIF;
    return conf;
}


static void ntconf_free(ntconf_t * conf)
{
    if(conf != NULL)
    {
        free(conf->fov_string);
        free(conf->dwargs);
    }
    free(conf);
}

static int parse_slice_range(const char * str, int * a, int *b)
{
    /* Parse a json formatted range from the string, for example
       [2, 10] sets a to 2 and b to 10
    */
    cJSON *j = cJSON_Parse(str);
    if(j == NULL)
    {
        return EXIT_FAILURE;
    }
    if(cJSON_GetArraySize(j) != 2)
    {
        printf("Array has wrong size\n");
        goto fail;
    }

    cJSON * ja = cJSON_GetArrayItem(j, 0);
    if (cJSON_IsNumber(ja))
    {
        *a = ja->valueint;
    } else {
        goto fail;
    }

    cJSON * jb = cJSON_GetArrayItem(j, 1);
    if (cJSON_IsNumber(jb))
    {
        *b = jb->valueint;
    } else {
        goto fail;
    }

    cJSON_Delete(j);
    return EXIT_SUCCESS;

 fail:
    cJSON_Delete(j);
    return EXIT_FAILURE;
}

static int argparse(ntconf_t * conf, int argc, char ** argv)
{
    struct option longopts[] = {
        { "coord",      no_argument, NULL, 'c'},
        { "composite",  no_argument, NULL, 'C'},
        { "dry",        no_argument, NULL, 'd'},
        { "deconwolf",  no_argument, NULL, 'D'},
        { "deconwolfx", no_argument, NULL, 'E'},
        { "deconwolf_dots", no_argument, NULL, 'G'},
        { "help",       no_argument, NULL, 'h'},
        { "info",       no_argument, NULL, 'i'},
        { "overwrite",  no_argument, NULL, 'o'},
        { "slice",      required_argument, NULL, 'r'},
        { "shake",      no_argument, NULL, 's'},
        { "SpaceTx",    no_argument, NULL, 'S'},
        { "test",       no_argument, NULL, 't'},
        { "verbose",    required_argument, NULL, 'v'},
        { "version",    no_argument, NULL, 'V'},
        { "fov",        required_argument, NULL, 'F'},
        { "meta",       no_argument, NULL, '1'},
        { "meta-file",  no_argument, NULL, '2'},
        { "meta-coord", no_argument, NULL, '3'},
        { "meta-frame", no_argument, NULL, '4'},
        { "meta-text",  no_argument, NULL, '5'},
        { "meta-exp",   no_argument, NULL, '6'},
        { NULL, 0, NULL, 0 }
    };
    int ch;

    while((ch = getopt_long(argc, argv, "123456CDEFGSVcdhior:sv:t",
                            longopts, NULL)) != -1)
    {
        switch(ch) {
        case '1':
            conf->purpose = SHOW_METADATA;
            conf->meta_file = 1;
            conf->meta_coord = 1;
            conf->meta_frame = 1;
            conf->meta_text = 1;
            conf->meta_exp = 1;
            break;
        case '2':
            conf->purpose = SHOW_METADATA;
            conf->meta_file = 1;
            break;
        case '3':
            conf->purpose = SHOW_METADATA;
            conf->meta_coord = 1;
            break;
        case '4':
            conf->purpose = SHOW_METADATA;
            conf->meta_frame = 1;
            break;
        case '5':
            conf->purpose = SHOW_METADATA;
            conf->meta_text = 1;
            break;
        case '6':
            conf->purpose = SHOW_METADATA;
            conf->meta_exp = 1;
            break;
        case 'c':
            conf->showcoords = 1;
            conf->shake = 1;
            conf->verbose = 0;
            break;
        case 'C':
            conf->composite = 1;
            break;
        case 'd':
            conf->dry = 1;
            break;
        case 'D':
            conf->deconwolf = 1;
            break;
        case 'E':
            conf->deconwolf = 1;
            conf->deconwolfx = 1;
            break;
        case 'F':
            free(conf->fov_string);
            conf->fov_string = strdup(optarg);
            break;
        case 'G':
            conf->deconwolf_dots = 1;
            break;
        case 'h':
            show_help(argv[0]);
            exit(EXIT_SUCCESS);
            break;
        case 'o':
            conf->overwrite = 1;
            break;
        case 'r':
            {
                int a, b;
                if(parse_slice_range(optarg, &a, &b) == 0)
                {
                    conf->use_range = 1;
                    conf->range_from = a;
                    conf->range_to = b;
                } else {
                    printf("Unable to parse range from %s\n", optarg);
                    printf("When using --slice make sure to quote the range, for example:\n"
                           "nd2tool --slice '[2, 20]' ...\n");
                }
            }
            break;
        case 's':
            conf->shake = 1;
            break;
        case 'S':
            conf->save_individual_planes = 1;
            break;
        case 't':
            nd2tool_util_ut();
            exit(EXIT_SUCCESS);
            break;
        case 'v':
            conf->verbose = atoi(optarg);
            break;
        case 'i':
            conf->convert = 0;
            break;
        case 'V':
            show_version();
            exit(EXIT_SUCCESS);
        default:
            exit(EXIT_FAILURE);
        }
    }
    conf->optind = optind;
    return EXIT_SUCCESS;
}


static void showmeta_file(char * file)
{
    void * nd2 = Lim_FileOpenForReadUtf8(file);
    if(nd2 == NULL)
    {
        fprintf(stderr, "%s is not a valid nd2 file\n", file);
        exit(EXIT_FAILURE);
    }
    char * fileMeta = Lim_FileGetMetadata(nd2);
    printf("%s\n", fileMeta);
    Lim_FileFreeString(fileMeta);
    Lim_FileClose(nd2);
    return;
}


static void showmeta_coord(char * file)
{
    void * nd2 = Lim_FileOpenForReadUtf8(file);
    if(nd2 == NULL)
    {
        fprintf(stderr, "%s is not a valid nd2 file\n", file);
        exit(EXIT_FAILURE);
    }
    int csize = Lim_FileGetCoordSize(nd2);
    if(csize == 0)
    {
        printf("%s contains only one frame (not an ND document).\n",
               file);
    }

    int buffsize = 1024;
    char * buffer = ckcalloc(buffsize, 1);

    for(int kk = 0; kk<csize; kk++)
    {
        int dsize = Lim_FileGetCoordInfo(nd2, kk, buffer, buffsize);
        printf("%s (loop size: %d)\n", buffer, dsize);
    }
    free(buffer);

    Lim_FileClose(nd2);
    return;
}


static void showmeta_frame(char * file)
{
    void * nd2 = Lim_FileOpenForReadUtf8(file);
    if(nd2 == NULL)
    {
        fprintf(stderr, "%s is not a valid nd2 file\n", file);
        exit(EXIT_FAILURE);
    }

    int seqCount = Lim_FileGetSeqCount(nd2);

    for(int kk = 0; kk<seqCount; kk++)
    {
        char * info = Lim_FileGetFrameMetadata(nd2, kk);
        printf("%s\n", info);
        Lim_FileFreeString(info);
    }

    Lim_FileClose(nd2);
    return;
}


static void showmeta_exp(char * file)
{
    void * nd2 = Lim_FileOpenForReadUtf8(file);
    if(nd2 == NULL)
    {
        fprintf(stderr, "%s is not a valid nd2 file\n", file);
        exit(EXIT_FAILURE);
    }
    char * expinfo = Lim_FileGetExperiment(nd2);

    printf("%s\n", expinfo);
    Lim_FileFreeString(expinfo);

    Lim_FileClose(nd2);
    return;
}

/* It is JSON data, with elements
 * - capturing
 * - date
 * - description
 * - optics
 * The elements are all text.
 */
static void showmeta_text(char * file)
{
    void * nd2 = Lim_FileOpenForReadUtf8(file);
    if(nd2 == NULL)
    {
        fprintf(stderr, "%s is not a valid nd2 file\n", file);
        exit(EXIT_FAILURE);
    }
    char * textinfo = Lim_FileGetTextinfo(nd2);
    filter_textinfo(textinfo); /* Output filled with '\r\n\' written out as text */

    printf("%s\n", textinfo);
    Lim_FileFreeString(textinfo);

    Lim_FileClose(nd2);
    return;
}


/** @brief Shake detection in z */
static void check_stage_position(nd2info_t * info, int fov, int channel)
{
    int nchannel = info->meta_att->nchannels;
    /* Assuming equal number of planes in all channels */
    int nplane = info->meta_att->channels[0]->P;
    double * XYZ = info->meta_frame->stagePositionUm;
    size_t stride = nchannel*3;
    size_t offset = fov*stride*nplane + 3*channel;
    XYZ = XYZ + offset;

    /* check dz */
    double dz_min = 0;
    double dz_max = 0;
    for(int zz = 1; zz < nplane; zz++)
    {
        double dz = XYZ[stride*zz + 2] - XYZ[stride*(zz-1) + 2];
        if(zz == 1)
        {
            dz_min = dz;
            dz_max = dz;
        }
        dz < dz_min ? dz_min = dz : 0;
        dz > dz_max ? dz_max = dz : 0;
    }

    if( fabs(1000.0*(dz_max - dz_min)) > 1 )
    {
        printf("WARNING: dz_nm [%.0f, %.0f] ", dz_min*1000, dz_max*1000);
    }

    nd2info_log(info, "dz_nm [%.f, %.0f] ", dz_min*1000, dz_max*1000);
    return;
}


static void showmeta(ntconf_t * conf, char * file)
{
    if(conf->meta_file)
    {
        showmeta_file(file);
    }
    if(conf->meta_coord)
    {
        showmeta_coord(file);
    }
    if(conf->meta_frame)
    {
        showmeta_frame(file);
    }
    if(conf->meta_text)
    {
        showmeta_text(file);
    }
    if(conf->meta_exp)
    {
        showmeta_exp(file);
    }
    return;
}


static void hello_log(__attribute__((unused)) ntconf_t * conf,
                      nd2info_t * info, int argc, char ** argv)
{
    if(conf->dry)
    {
        return;
    }

    fprintf(info->log, "CMD: ");
    for(int kk = 0; kk<argc; kk++)
    {
        fprintf(info->log, "%s ", argv[kk]);
    }
    fprintf(info->log, "\n");

    char * hname = ckcalloc(1024, sizeof(char));
    if(gethostname(hname, 1023) == 0)
    {
        fprintf(info->log, "HOSTNAME: '%s'\n", hname);
    }
    free(hname);

    char * user = getenv("USER");
    if(user != NULL)
    {
        fprintf(info->log, "USER: '%s'\n", user);
    }
    return;
}


static void nd2_show_coordinates(nd2info_t * info)
{
    /* see check_stage_position */
    int nchan = info->meta_att->nchannels;
    int nfov = info->nFOV;
    int nplane = info->meta_att->channels[0]->P;
    double * _XYZ = info->meta_frame->stagePositionUm;
    assert(_XYZ != NULL);

    printf("FOV, Channel, Z, X_um, Y_um, Z_um\n");
    for(int fov = 0; fov < nfov; fov++)
    {
        for(int cc = 0; cc < nchan; cc++)
        {
            for(int zz = 0; zz < nplane; zz++)
            {
                size_t offset = fov*nchan*nplane*3; /* select fov */
                offset += zz*3*nchan; /* select plane */
                offset += cc*3; /* select channel */
                double * XYZ = _XYZ + offset;
                double x = XYZ[0];
                double y = XYZ[1];
                double z = XYZ[2];
                printf("%d, %d, %d, %f, %f, %f\n",
                       fov+1, cc+1, zz+1, x, y, z);
            }
        }
    }
    return;
}


/** @brief Write a script that will run deconwolf
 */
static void
nd2info_show_deconwolf(ntconf_t * conf, const nd2info_t * info, FILE * fid)
{
    metadata_t * meta = info->meta_att;
    fprintf(fid, "#!/bin/env bash\n");
    fprintf(fid, "set -e # abort on errors\n");
    fprintf(fid, "\n");
    fprintf(fid, "# PSF Generation\n");
    for(int cc = 0; cc < meta->nchannels; cc++)
    {
        fprintf(fid, "dw_bw --lambda %f --resxy %f --resz %f --NA %f --ni %f '%s/PSF_%s.tif'\n",
                meta->channels[cc]->emissionLambdaNm,
                meta->channels[cc]->dx_nm,
                meta->channels[cc]->dz_nm,
                meta->channels[cc]->objectiveNumericalAperture,
                meta->channels[cc]->immersionRefractiveIndex,
                info->outfolder,
                meta->channels[cc]->name);
    }

    int dw_iter = 50;

    if(conf->deconwolfx)
    {
        nd2info_print(conf, stdout, info);
        printf("Enter the number of iterations to use for each channel\n");
        printf("To skip this channel, set the number of iterations to 0\n");
    }
    assert(meta->nchannels < 1000);
    int use_channel[meta->nchannels];
    for(int cc = 0; cc < meta->nchannels; cc++)
    {
        if(conf->deconwolfx)
        {
            dw_iter = cli_get_int(meta->channels[cc]->name, dw_iter, 0, 1000);
        }
        fprintf(fid, "iter_%s=%d\n", meta->channels[cc]->name, dw_iter);
        if(dw_iter > 0)
        {
            use_channel[cc] = 1;
        } else {
            use_channel[cc] = 0;
        }
    }

    char * xargs = strdup("");
    if(conf->deconwolfx)
    {
        free(xargs);
        xargs = cli_get_string("Enter any extra arguments to deconwolf", conf->dwargs);
        free(conf->dwargs);
        conf->dwargs = strdup(xargs);
    }

    fprintf(fid, "xargs=\"%s\"\n", xargs);
    free(xargs);

    for(int ff = 0; ff  < info->nFOV; ff++)
    {
        for(int cc = 0; cc < meta->nchannels; cc++)
        {
            if(use_channel[cc] == 0)
            {
                continue;
            }
            /* Write out to disk */
            size_t slen = 1024;
            char * outname = ckcalloc(slen, 1);
            snprintf(outname, slen,
                     "%s/%s_%03d.tif", info->outfolder,
                     info->meta_att->channels[cc]->name, ff+1);

            fprintf(fid, "dw ${xargs} --iter $iter_%s '%s' '%s/PSF_%s.tif'\n",
                    meta->channels[cc]->name,
                    outname,
                    info->outfolder,
                    meta->channels[cc]->name);

            free(outname);
        }
    }

    return;
}

/* Print out how to detect the dots using `dw dots` */
static void
nd2info_show_deconwolf_dots(const ntconf_t * conf,
                            const nd2info_t * info,
                            FILE * fid)
{
    metadata_t * meta = info->meta_att;
    fprintf(fid, "#!/bin/env bash\n");
    fprintf(fid, "set -e # abort on errors\n");
    fprintf(fid, "\n");
    fprintf(fid, "# dot detection with dw dots\n\n");

    char * xargs = strdup("--fitting");

    fprintf(fid, "# --overwrite\n");
    fprintf(fid, "xargs=\"%s\"\n", xargs);
    free(xargs);

    fprintf(fid, "\n");
    fprintf(fid, "# set to \"\" to run on non-deconvolved images\n");
    fprintf(fid, "prefix=\"dw_\"\n");
    fprintf(fid, "\n");

    fprintf(fid, "# Set to 0 to skip a channel\n");
    for(int cc = 0; cc < meta->nchannels; cc++)
    {
        fprintf(fid, "use_%s=1\n", info->meta_att->channels[cc]->name);
    }
    fprintf(fid, "\n");

    for(int ff = 0; ff  < info->nFOV; ff++)
    {
        for(int cc = 0; cc < meta->nchannels; cc++)
        {
            /* Write out to disk */
            size_t slen = 1024;
            char * outname = ckcalloc(slen, 1);
            snprintf(outname, slen,
                     "%s/${prefix}%s_%03d.tif", info->outfolder,
                    info->meta_att->channels[cc]->name, ff+1);
            fprintf(fid, "if [[ $use_%s -eq 1 ]]\n", info->meta_att->channels[cc]->name);

            fprintf(fid, "then\n");

            fprintf(fid, "   dw dots ${xargs} %s --NA %.2f --ni %.2f --dx %.1f --dz %.1f --lambda %.0f\n",
                    outname,
                    info->meta_att->channels[cc]->objectiveNumericalAperture,
                    info->meta_att->channels[cc]->immersionRefractiveIndex,
                    info->meta_att->channels[cc]->dx_nm,
                    info->meta_att->channels[cc]->dz_nm,
                    info->meta_att->channels[cc]->emissionLambdaNm
                    );
            fprintf(fid, "fi\n");
            free(outname);
        }
    }

    if(conf->verbose > 0)
    {
        printf("Please set xargs and prefix before running the script\n");
    }
    return;
}


/** @brief Command line interface to nd2tool */
int nd2tool_cli(int argc, char ** argv)
{
    check_cmd_line(argc, argv);

    ntconf_t * conf = ntconf_new();
    if(argparse(conf, argc, argv) != EXIT_SUCCESS)
    {
        exit(EXIT_FAILURE);
    }

    /* Process each file */

    /* Show some metadata and exit */
    if(conf->purpose == SHOW_METADATA)
    {
        for(int ff = optind; ff<argc; ff++)
        {
            showmeta(conf, argv[ff]);
        }
        goto done;
    }

    /* Convert to tif */
    int nfiles = argc-optind;
    if(nfiles == 0)
    {
        printf("error: No file(s) given\n");
        exit(EXIT_FAILURE);
    }

    for(int ff = optind; ff<argc; ff++)
    {
        if(nfiles > 1 && conf->verbose > 0)
        {
            printf(" -> %s (%d/%d)\n",
                   argv[ff], ff-optind+1, nfiles);
        }
        /* Parse information */
        nd2info_t * info = nd2info(conf, argv[ff]);

        if(info->error != NULL)
        {
            fprintf(stderr, "%s", info->error);
            goto cleanup_file;
        }

        if(conf->deconwolf)
        {
            char * script_name0 = prefix_filename(info->filename, "deconwolf_");
            char * script_name = postfix_filename(script_name0, ".sh");
            free(script_name0);

            FILE * fid_dw_script = fopen(script_name, "w");
            if(fid_dw_script == NULL)
            {
                fprintf(stderr, "Unable to open %s for writing\n", script_name);
                free(script_name);
                exit(EXIT_FAILURE);
            }
            if(conf->verbose > 0)
            {
                fprintf(stdout, "Writing to %s\n", script_name);
            }
            nd2info_show_deconwolf(conf, info, fid_dw_script);
            fclose(fid_dw_script);
            make_file_executable(script_name);

            free(script_name);
            goto cleanup_file;
        }

        if(conf->deconwolf_dots)
        {
            char * script_name0 = prefix_filename(info->filename, "deconwolf_dots_");
            char * script_name = postfix_filename(script_name0, ".sh");
            free(script_name0);

            FILE * fid_dw_script = fopen(script_name, "w");
            if(fid_dw_script == NULL)
            {
                fprintf(stderr, "Unable to open %s for writing\n", script_name);
                free(script_name);
                exit(EXIT_FAILURE);
            }
            if(conf->verbose > 0)
            {
                fprintf(stdout, "Writing to %s\n", script_name);
            }
            nd2info_show_deconwolf_dots(conf, info, fid_dw_script);
            fclose(fid_dw_script);
            make_file_executable(script_name);

            free(script_name);
            goto cleanup_file;
        }

        if(conf->showinfo == 1 && conf->verbose > 0)
        {
            /* Show brief summary */
            nd2info_print(conf, stdout, info);
        }

        if(conf->showcoords)
        {
            nd2_show_coordinates(info);
            goto cleanup_file;
        }

        if(conf->convert)
        {
            /* Create output folder and export tiff files */
            if(nd2_to_tiff(conf, info) == EXIT_SUCCESS)
            {
                /* Write some basic information to the log */
                hello_log(conf, info, argc, argv);
                nd2info_print(conf, info->log, info);
                nd2info_log(info, "done\n");

            } else {
                fprintf(stderr,
                        "Conversion failed for %s, please make a bug report at "
                        "https://github.com/elgw/nd2tool/issues in order to "
                        "improve the program.\n", argv[ff]);
            }
        }
        /* Might jump directly here if an error occurred  */
    cleanup_file: ;
        /* Clean up */
        nd2info_free(info);
    }

    /* Print out peak memory usage */
    size_t mem = get_peakMemoryKB();

    if(mem > 0)
    {
        if(conf->verbose > 1)
        {
            printf("Done! Used at most %zu kb of RAM\n", mem);
        }
    } else {
        if(conf->verbose > 1)
        {
            printf("Done! But failed to measure RAM usage\n");
        }
    }


    /* Final cleanup */
 done: ;
    ntconf_free(conf);
    return EXIT_SUCCESS;
}
