#include "nd2tool_util.h"

static void * ckcalloc(size_t nmemb, size_t size)
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


/** @breif Replace '\\r\\n' by '   \n'
*/
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

/* Remove .nd2 from file.nd2 */
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

/** @brief How much memory was allocated?
 *
 * Get the peak memory used by the process from /proc/<PID>/status
*/
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
    char * statfile = ckcalloc(100, sizeof(char));

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
                free(peakline);
                peakline = strdup(line);
            }
        }
    }
    free(line);
    fclose(sf);
    free(statfile);

    if(peakline == NULL)
    {
        return 0;
    }

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

/** @brief Check if file exists
*/
int isfile(char * filename)
{
    FILE *file = fopen(filename, "r");
    if (file != NULL)
    {
        fclose(file);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/** @brief make file executable by user (chmod u+x file)*/
void make_file_executable(const char * filename)
{
    struct stat statbuf = {0};
    stat(filename, &statbuf);
    // printf("File mask = (%3o)\n", statbuf.st_mode);
    if(chmod(filename, statbuf.st_mode | S_IXUSR))
    {
        fprintf(stderr, "Unable to chmod u+x %s\n", filename);
    }
    return;
}

/** @brief Show four blank spaces coloured by RGB
 * if fid != stdout, writes "    ".
 * if lambda < 400, show " uv ", if lambda > 700, show " ir "
 */
void show_color(FILE * fid, const double * RGB, double lambda)
{
    char ir_type[] = "ir";
    char uv_type[] = "uv";
    char vis_type[] = "  ";

    char * type_str = vis_type;

    if(lambda < 400)
    {
        type_str = uv_type;
    }
    if(lambda > 700)
    {
        type_str = ir_type;
    }

    if(fid == stdout)
    {
        fprintf(fid, "\e[38;2;%d;%d;%dm\e[48;2;%d;%d;%dm %s \e[0m",
                (int) round(255.0 - 255.0*RGB[0]),
                (int) round(255.0 - 255.0*RGB[1]),
                (int) round(255.0 - 255.0*RGB[2]),
                (int) round(255.0*RGB[0]),
                (int) round(255.0*RGB[1]),
                (int) round(255.0*RGB[2]),
                type_str);
    } else {
        fprintf(fid, " %s ", type_str);
    }
}

/** @brief Add a prefix to a file name
 *
 * Example:
 * "file.tif", "dw_" -> "dw_file.tif"
 * "/data/file.tif", "dw_" -> "/data/dw_file.tif"
*/
char *
prefix_filename(const char * filename, const char * prefix)
{

    const size_t px_size = strlen(filename) + strlen(prefix)+1;
    char * px_name = ckcalloc(px_size, 1);

    ssize_t last_filesep = 0;
    for(size_t kk = 0; kk < strlen(filename); kk++)
    {
        if(filename[kk] == '/')
        {
            last_filesep = kk+1;
        }
    }

    strncat(px_name,
            filename, last_filesep);

    strcat(px_name,
            prefix);

    strncat(px_name,
            filename+last_filesep,
            strlen(filename)-last_filesep);

    return px_name;
}

char *
postfix_filename(const char * filename, const char * postfix)
{
    char * px_name = ckcalloc(
        strlen(filename) + strlen(postfix) + 1, 1);
    strcat(px_name, filename);
    strcat(px_name, postfix);
    return px_name;
}

static void prefix_filename_test(const char * file,
                                 const char * prefix,
                                 const char * ref)
{
    char * result = prefix_filename(file, prefix);

    if(strcmp(result, ref))
    {
        fprintf(stderr, "prefix_filename_test failed\n");
        fprintf(stderr, "('%s', '%s') -> '%s' != '%s'\n",
                file, prefix, result, ref);
        exit(EXIT_FAILURE);
    } else {
        printf("ok: ('%s', '%s') -> '%s'\n", file, prefix, result);
    }
    free(result);
}

static void prefix_filename_ut(void)
{
    printf("-> testing prefix_filename\n");
    prefix_filename_test("file.tif", "a_", "a_file.tif");
    prefix_filename_test("./f.tif", "a_", "./a_f.tif");
    prefix_filename_test("/a/b/c/q.x", "_", "/a/b/c/_q.x");
    prefix_filename_test("", "", "");
    prefix_filename_test("", "a", "a");
    prefix_filename_test("a", "", "a");
    prefix_filename_test("", "", "");
}

/** @brief Unit tests */
void nd2tool_util_ut()
{
    prefix_filename_ut();
}
