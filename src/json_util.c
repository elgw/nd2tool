#include "json_util.h"

int get_json_int(const cJSON * j, const char * item, int * store)
{
    const cJSON * q = cJSON_GetObjectItemCaseSensitive(j, item);
    if (cJSON_IsNumber(q))
    {
        *store = q->valueint;
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "ERROR: cJSON_IsNumber failed when trying to parse %s in %s at line %d\n",
               item, __FILE__, __LINE__);
        *store = -1;
        return EXIT_FAILURE;
    }
}

int get_json_double(const cJSON * j, const char * item, double * store)
{
    const cJSON * q = cJSON_GetObjectItemCaseSensitive(j, item);
    if (cJSON_IsNumber(q))
    {
        *store = q->valuedouble;
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "ERROR: cJSON_IsNumber failed when trying to parse %s in %s at line %d\n",
                item, __FILE__, __LINE__);
        *store = NAN;
        return EXIT_FAILURE;
    }
}

char * get_json_string(const cJSON * j, const char * item)
{
    const cJSON * q = cJSON_GetObjectItemCaseSensitive(j, item);
    if(cJSON_IsString(q) && (q->valuestring != NULL))
    {
        char * ret = strdup(q->valuestring);
        return ret;
    }
    fprintf(stderr, "ERROR: Failed to parse %s in %s on line %d\n",
            item, __FILE__,__LINE__);
    return NULL;
}
