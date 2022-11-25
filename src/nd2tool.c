#include "nd2tool.h"

void remove_file_ext(char * str)
{
    size_t lastpos = strlen(str);
    for(size_t kk = 0; kk<strlen(str); kk++)
    {
        if(str[kk] == '.')
        {
            lastpos = kk;
        }
    }

    if(lastpos < strlen(str))
    {
        str[lastpos] = '\0';
    }
    if(lastpos == 0)
    {
        fprintf(stdout, "Not a valid file name\n");
        exit(EXIT_FAILURE);
    }
}

#ifdef __APPLE__
size_t get_peakMemoryKB(void)
{
    struct rusage r_usage;
    getrusage(RUSAGE_SELF, &r_usage);
    return (size_t) round((double) r_usage.ru_maxrss/1024.0);
}
#endif

#ifndef __APPLE__
size_t get_peakMemoryKB(void)
{
    char * statfile = malloc(100*sizeof(char));
    sprintf(statfile, "/proc/%d/status", getpid());
    FILE * sf = fopen(statfile, "r");
    if(sf == NULL)
    {
        fprintf(stderr,
                "ERROR in %s at line %d: Failed to open %s\n",
                __FILE__, __LINE__, statfile);
        free(statfile);
        return 0;
    }

    char * peakline = NULL;

    char * line = NULL;
    size_t len = 0;

    while( getline(&line, &len, sf) > 0)
    {
        if(strlen(line) > 6)
        {
            if(strncmp(line, "VmPeak", 6) == 0)
            {
                peakline = strdup(line);
            }
        }
    }
    free(line);
    fclose(sf);
    free(statfile);

    /* Parse the line starting with "VmPeak" Seems like it is always
     * in kB (reference: fs/proc/task_mmu.c) actually in kiB i.e.,
     * 1024 bytes since the last three characters are ' kb' we can
     * skip them and parse in-between */
    size_t peakMemoryKB = 0;
    if(strlen(peakline) > 11)
    {
        peakline[strlen(peakline) -4] = '\0';
        peakMemoryKB = (size_t) atol(peakline+7);
    }

    free(peakline);
    return peakMemoryKB;
}
#endif



void file_attrib_free(file_attrib_t * f)
{
    if(f != NULL)
    {
        free(f);
    }
}


void metadata_free(metadata_t * m)
{
    if(m == NULL)
        return;

    for(int kk = 0; kk < m->nchannels; kk++)
    {
        channel_attrib_t * c = m->channels[kk];
        if(c->name != NULL)
        {
            free(c->name);
        }
        if(c->objectiveName != NULL)
        {
            free(c->objectiveName);
        }
        free(c);
    }
    free(m->channels);
    free(m);
}

void nd2info_free(nd2info_t * n)
{
    if(n->filename != NULL)
    {
        free(n->filename);
    }
    if(n->meta_att != NULL)
    {
        metadata_free(n->meta_att);
    }
    if(n->file_att != NULL)
    {
        file_attrib_free(n->file_att);
    }
    if(n->loopstring != NULL)
    {
        free(n->loopstring);
        n->loopstring = NULL;
    }
    if(n->logfile != NULL)
    {
        free(n->logfile);
        n->logfile = NULL;
    }
    if(n->outfolder != NULL)
    {
        free(n->outfolder);
        n->outfolder = NULL;
    }
    if(n->log != NULL)
    {
        fclose(n->log);
    }
    if(n->error != NULL)
    {
        free(n->error);
        n->error = NULL;
    }
    free(n);
}

nd2info_t * nd2info_new()
{
    nd2info_t * info = calloc(1, sizeof(nd2info_t));
    return info;
}

metadata_t * parse_metadata(const char * str)
{
    /* Parse the result from Lim_FileGetMetadata */
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

    metadata_t * m = malloc(sizeof(metadata_t));

    const cJSON * j_contents = cJSON_GetObjectItemCaseSensitive(j, "contents");
    if(get_json_int(j_contents, "channelCount", &m->nchannels) != 0)
    {
        fprintf(stderr, "Unable to continue (%s, line %d)\n",
                __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    assert(m->nchannels > 0);

    m->channels = malloc(m->nchannels*sizeof(channel_attrib_t*));
    for(int cc = 0; cc<m->nchannels; cc++)
    {
        m->channels[cc] = malloc(sizeof(channel_attrib_t));
        m->channels[cc]->name = NULL;
    }

    cJSON * j_channel = NULL;
    cJSON * j_channels = cJSON_GetObjectItemCaseSensitive(j, "channels");
    int cc = 0;
    cJSON_ArrayForEach(j_channel, j_channels)
    {
        cJSON * j_chan = cJSON_GetObjectItemCaseSensitive(j_channel, "channel");
        m->channels[cc]->name = get_json_string(j_chan, "name");
        assert(m->channels[cc] != NULL);
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
            cJSON * j_tmp = cJSON_GetObjectItemCaseSensitive(j_mic,
                                                             "immersionRefractiveIndex");
            m->channels[cc]->immersionRefractiveIndex = j_tmp->valuedouble;

            j_tmp = cJSON_GetObjectItemCaseSensitive(j_mic,
                                                     "objectiveNumericalAperture");
            m->channels[cc]->objectiveNumericalAperture = j_tmp->valuedouble;

            j_tmp = cJSON_GetObjectItemCaseSensitive(j_mic,
                                                     "objectiveMagnification");
            m->channels[cc]->objectiveMagnification = j_tmp->valuedouble;

            j_tmp = cJSON_GetObjectItemCaseSensitive(j_mic,
                                                     "objectiveName");
            m->channels[cc]->objectiveName = strdup(j_tmp->valuestring);
        }
        cc++;
    }



    cJSON_Delete(j);

    return m;
}


int isfile(char * filename)
{
    FILE *file = fopen(filename, "r");
    if (file != NULL)
    {
        fclose(file);
        return 1;
    }

    return 0;
}

file_attrib_t * parse_file_attrib(const char * str)
{
    /* Parse the result from Lim_FileGetMetadata */
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

    file_attrib_t * attrib = malloc(sizeof(file_attrib_t));

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

void check_cmd_line(int argc, char ** argv)
{
    if(argc < 2)
    {
        show_help(argv[0]);
        exit(EXIT_FAILURE);
    }
    return;
}

void filter_textinfo(char * s)
{
    size_t n = strlen(s);
    for(size_t kk = 0; kk+3<n; kk++)
    {
        if(s[kk] == '\\')
        {
            if(s[kk+1] == 'r')
            {
                if(s[kk+2] == '\\')
                {
                    if(s[kk+3] == 'n')
                    {
                        s[kk] = ' ';
                        s[kk+1] = ' ';
                        s[kk+2] = ' ';
                        s[kk+3] = '\n';
                    }
                }
            }
        }
    }
}


nd2info_t * nd2info(ntconf_t * conf, char * file)
{
    nd2info_t * info = nd2info_new();
    info->filename = strdup(file);

    struct stat stats;
    if(stat(info->filename, &stats) != 0)
    {
        info->error = malloc(strlen(file) + 128);
        sprintf(info->error, "Can't open %s\n", file);
        return info;
    }

    void * nd2 = Lim_FileOpenForReadUtf8(file);
    if(nd2 == NULL)
    {
        info->error = malloc(strlen(file) + 128);
        sprintf(info->error, "%s is not a valid nd2 file\n", file);
        return info;
    }

    //LIMFILEAPI LIMSIZE         Lim_FileGetCoordSize(LIMFILEHANDLE hFile);
    int csize = Lim_FileGetCoordSize(nd2);
    if(conf->verbose > 1)
    {
        printf("# Lim_FileGetCoordSize\n");
        printf("%d\n", csize);
    }

    if(csize < 1)
    {
        info->error = malloc(strlen(file) + 128);
        sprintf(info->error, "Error: Can't find coordinates in %s\n", file);
        Lim_FileClose(nd2);
        return info;
    }

    int nFOV = 1;
    int nPlanes = 0;

    for(int kk = 0; kk<csize; kk++)
    {
        int buffsize = 1024;
        char * buffer = malloc(buffsize);
        int dsize = Lim_FileGetCoordInfo(nd2, kk, buffer, buffsize);

        if(conf->verbose > 1)
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
                if(conf->verbose > 1)
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
        info->error = malloc(strlen(file) + 128);
        sprintf(info->error, "Error: Can't find any image planes %s\n", file);
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


    /* Information per 2D image */
    for(int kk = 0; kk<seqCount; kk++)
    {
        char * info = Lim_FileGetFrameMetadata(nd2, kk);
        if(conf->verbose > 1)
        {
            printf("# FileGetFrameMetadata for image %d\n", kk);
            printf("%s\n", info);
        }
        Lim_FileFreeString(info);
    }

    /* Lim_FileGetTextinfo does not return JSON. It contains
     * information about sensor, camera, scope, tempertures etc.  Also a
     * line like "Dimensions: XY(11) x λ(2) x Z(51)" -- the loop
     * order. Can that be determined from the other metadata?  Note that
     * the line like "- Step: 0,3 µm" is rounded to 1 decimal and should
     * not be used.
     */
    char * textinfo = Lim_FileGetTextinfo(nd2);
    filter_textinfo(textinfo); /* Output filled with '\r\n\' written out as text */
    if(conf->verbose > 2)
    {
        printf("# Lim_FileGetTextinfo\n");
        printf("%s\n", textinfo);
    }
    char * token = strtok(textinfo, "\n");
    while( token != NULL ) {
        if(strncmp(token, "Dimensions:", 11) == 0)
        {
            info->loopstring = strdup(token);
        }
        token = strtok(NULL, "\n");
    }
    free(token);

    Lim_FileFreeString(textinfo);

    /* This is interesting. The order probably plays a role here.
     * relevant to us is probably only the case where there are two items,
     * 1) For looping over pre-set start positions (type=XYPosLoop)
     * 2) For looping in z (type=ZStackLoop)
     * Is it possible to see where the looping over colors occur based on this?
     */

    char * expinfo = Lim_FileGetExperiment(nd2);
    Lim_FileFreeString(expinfo);

    Lim_FileClose(nd2);
    return info;
}

int nd2_to_tiff(ntconf_t * conf, nd2info_t * info)
{
    /* Check that the nd2 file exists */
    struct stat stats;
    if(stat(info->filename, &stats) != 0)
    {
        if(conf->verbose > 0)
        {
            fprintf(stderr, "Can't open %s\n", info->filename);
        }
        return EXIT_FAILURE;
    }

    /* Please note that this reports true also for tif files */
    void * nd2 = Lim_FileOpenForReadUtf8(info->filename);
    if(nd2 == NULL)
    {
        if(conf->verbose > 0)
        {
            fprintf(stderr, "%s is not a valid nd2 file\n", info->filename);
        }
        return EXIT_FAILURE;
    }


    info->outfolder = strdup(info->filename);
    info->outfolder = basename(info->outfolder);
    remove_file_ext(info->outfolder);
    if(conf->verbose > 1)
    {
        printf("Will create %s\n", info->outfolder);
    }


    if (mkdir(info->outfolder, 0777) == -1)
    {
        if(errno != EEXIST)
        {
            fprintf(stdout,
                    "Error: Unable to create the folder %s\n",
                    info->outfolder);
        }
    }

    if(info->file_att->bitsPerComponentInMemory != 16)
    {
        fprintf(stderr, "Can only convert files with 16 bit per pixel.\n"
                "This file has %d\n",
                info->file_att->bitsPerComponentInMemory);
        return EXIT_FAILURE;
    }

    info->logfile = malloc(strlen(info->outfolder) + 128);
    sprintf(info->logfile, "%s/nd2tool.log.txt",
            info->outfolder);

    if(conf->verbose > 1)
    {
        printf("Log file %s\n", info->logfile);
    }

    info->log = fopen(info->logfile, "a");
    if(info->log == NULL)
    {
        fprintf(stderr, "Unable to create the log file: %s\n",
                info->logfile);
        exit(EXIT_FAILURE);
    }
    fprintf(info->log, "\n");
    fprintf(info->log, "Input file: %s\n", info->filename);


    /* Prepare metadata for the tiff files */
    ttags * tags = ttags_new();
    {
        char * sw_string = malloc(1024);
        sprintf(sw_string, "github.com/elgw/nd2tool source image: %s",
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

    /* Buffer for one slice and one color */
    uint16_t * S = malloc(sizeof(uint16_t)*M*N);


    LIMPICTURE * pic = malloc(sizeof(LIMPICTURE));
    Lim_InitPicture(pic, M, N, 16, nchan);

    /* We choose to extract the image data multiple times and only
     * collect pixels from one channel at a time. This is of course
     * slightly slower than extracting all channels for a given FOV at
     * a time but gives more predictable memory usage. */

    for(int ff = 0; ff<info->nFOV; ff++) /* For each FOV */
    {
        for(int cc = 0; cc<nchan; cc++) /* For each channel */
        {
            /* Write out to disk */
            char * outname = malloc(1024);
            sprintf(outname, "%s/%s_%03d.tif", info->outfolder,
                    info->meta_att->channels[cc]->name, ff+1);

            if(conf->overwrite == 0)
            {
                if(isfile(outname))
                {
                    printf("Skipping %s (file exists)\n", outname);
                    fprintf(info->log, "Skipping %s (file exists)\n", outname);
                    goto next_file;
                }
            }
            if(conf->verbose > 0)
            {
                printf("Writing to %s ... ", outname); fflush(stdout);
            }

            /* Create temporary file */
            char * outname_tmp = malloc(strlen(outname) + 16);
            sprintf(outname_tmp, "%s_tmp_XXXXXX", outname);
            int tfid = 0;
            if((tfid = mkstemp(outname_tmp)) == -1)
            {
                fprintf(stderr, "Failed to create a temporary file based on pattern: %s\n", outname_tmp);
                exit(EXIT_FAILURE);
            }
            close(tfid);

            tiff_writer_t * tw = tiff_writer_init(outname_tmp, tags, M, N, P);

            for(int kk = 0; kk<P; kk++) /* For each plane */
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

                for(int pp = 0; pp<M*N; pp++)
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
            free(outname_tmp);
        next_file: ;
            free(outname);

        } // cc
    }// ff


    Lim_DestroyPicture(pic);
    free(pic);

    free(S);
    ttags_free(&tags);
    Lim_FileClose(nd2);
    return EXIT_SUCCESS;
}

void nd2info_print(FILE * fid, const nd2info_t * info)
{
    metadata_t * meta = info->meta_att;
    int nFOV = info->nFOV;
    fprintf(fid, "%d FOV in %d channels:\n", nFOV, meta->nchannels);
    for(int cc = 0; cc < meta->nchannels; cc++)
    {
        double l = meta->channels[cc]->emissionLambdaNm;
        l < 425 ? l = 425 : 0;
        l > 650 ? l = 650 : 0;
        double RGB[3] = {0,0,0};
        srgb_from_lambda(l, RGB);

        if(fid == stdout)
        {
            fprintf(fid, "\e[38;2;%d;%d;%dm\e[48;2;%d;%d;%dm   \e[0m",
                   (int) round(255.0 - 255.0*RGB[0]),
                   (int) round(255.0 - 255.0*RGB[1]),
                   (int) round(255.0 - 255.0*RGB[2]),
                   (int) round(255.0*RGB[0]),
                   (int) round(255.0*RGB[1]),
                   (int) round(255.0*RGB[2]));
        } else {
            fprintf(fid, "   ");
        }
        fprintf(fid, "#%d '%s', λ_em=%.1f",
                cc+1,
                meta->channels[cc]->name,
                meta->channels[cc]->emissionLambdaNm);

        fprintf(fid, " approx. RGB %.2f, %.2f, %.2f or #%02X%02X%02X",
                RGB[0], RGB[1], RGB[2],
                (int) round(255.0*RGB[0]),
                (int) round(255.0*RGB[1]),
                (int) round(255.0*RGB[2]));
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
    return;
}

void print_version(FILE * fid)
{
    fprintf(fid, "nd2tool v.%s.%s.%s\n",
            ND2TOOL_VERSION_MAJOR,
            ND2TOOL_VERSION_MINOR,
            ND2TOOL_VERSION_PATCH);
    return;
}

void print_web(FILE * fid)
{
    fprintf(fid, "Web page: <https://www.github.com/elgw/nd2tool>\n");
    return;
}

void show_version()
{
    print_version(stdout);
    print_web(stdout);
    return;
}

void show_help(char * name)
{
    ntconf_t * conf = ntconf_new();

    printf("Usage: ");
    printf("%s [--info] [--help] file1.nd2 file2.nd2 ...\n",
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

ntconf_t * ntconf_new(void)
{
    ntconf_t * conf = calloc(1, sizeof(ntconf_t));
    conf->verbose = 1;
    conf->convert = 1;
    conf->overwrite = 0;
    conf->showinfo = 1;
    conf->purpose = CONVERT_TO_TIF;
    conf->meta_file = 0;
    conf->meta_coord = 0;
    conf->meta_frame = 0;
    conf->meta_text = 0;
    conf->meta_exp = 0;

    return conf;
}

void ntconf_free(ntconf_t * conf)
{
    free(conf);
}

int argparse(ntconf_t * conf, int argc, char ** argv)
{
    struct option longopts[] = {
        { "help",       no_argument, NULL, 'h'},
        { "info",       no_argument, NULL, 'i'},
        { "overwrite",  no_argument, NULL, 'o'},
        { "verbose",    required_argument, NULL, 'v'},
        { "version",    no_argument, NULL, 'V'},
        { "meta",       no_argument, NULL, '1'},
        { "meta-file",  no_argument, NULL, '2'},
        { "meta-coord", no_argument, NULL, '3'},
        { "meta-frame", no_argument, NULL, '4'},
        { "meta-text",  no_argument, NULL, '5'},
        { "meta-exp",   no_argument, NULL, '6'},
        { NULL, 0, NULL, 0 }
    };
    int ch;

    while((ch = getopt_long(argc, argv, "123456hiov:V", longopts, NULL)) != -1)
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
        case 'h':
            show_help(argv[0]);
            exit(EXIT_SUCCESS);
            break;
        case 'o':
            conf->overwrite = 1;
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

void showmeta_file(char * file)
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

void showmeta_coord(char * file)
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
    char * buffer = malloc(buffsize);
    for(int kk = 0; kk<csize; kk++)
    {
        int dsize = Lim_FileGetCoordInfo(nd2, kk, buffer, buffsize);
        printf("%s (loop size: %d)\n", buffer, dsize);
    }
    free(buffer);

    Lim_FileClose(nd2);
    return;
}

void showmeta_frame(char * file)
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

void showmeta_exp(char * file)
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

void showmeta_text(char * file)
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


void showmeta(ntconf_t * conf, char * file)
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

void hello_log(__attribute__((unused)) ntconf_t * conf,
               nd2info_t * info, int argc, char ** argv)
{
    print_version(info->log);

    fprintf(info->log, "CMD: ");
    for(int kk = 0; kk<argc; kk++)
    {
        fprintf(info->log, "%s ", argv[kk]);
    }
    fprintf(info->log, "\n");
    char * hname = malloc(1024*sizeof(char));
    if(gethostname(hname, 1023) == 0)
    {
        fprintf(info->log, "HOSTNAME: '%s'\n", hname);
        free(hname);
    }
    char * user = getenv("USER");
    if(user != NULL)
    {
        fprintf(info->log, "USER: '%s'\n", user);
    }
    return;
}

int main(int argc, char ** argv)
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

        if(conf->showinfo == 1 && conf->verbose > 0)
        {
            /* Show brief summary */
            nd2info_print(stdout, info);
        }

        if(conf->convert)
        {
            /* Create output folder and export tiff files */
            if(nd2_to_tiff(conf, info) == EXIT_SUCCESS)
            {
                /* Write some basic information to the log */
                hello_log(conf, info, argc, argv);
                nd2info_print(info->log, info);
                fprintf(info->log, "done\n");
            } else {
                fprintf(stderr,
                        "Conversion failed for %s, please make a bug report at "
                        "https://github.com/elgw/nd2tool/issues in order to "
                        "improve the program.\n", argv[ff]);
            }
        }

    cleanup_file: ;
        /* Clean up */
        nd2info_free(info);
    }

 done: ;
    size_t mem = get_peakMemoryKB();
    if(conf->verbose > 1)
    {
        if(mem > 0)
        {
            printf("Done! Used %zu kb of RAM\n", mem);
        } else {
            printf("Done! But failed to measure RAM usage\n");
        }
    }

    ntconf_free(conf);
    return EXIT_SUCCESS;
}
