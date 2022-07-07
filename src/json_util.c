#include "json_util.h"

int get_json_int(const cJSON * j, const char * item, int * store)
{
    const cJSON * q = cJSON_GetObjectItemCaseSensitive(j, item);
    if (cJSON_IsNumber(q))
    {
        *store = q->valueint;
    } else {
        printf("cJSON_IsNumber failed? %d\n", __LINE__);
    }
    return 0;
}

int get_json_double(const cJSON * j, const char * item, double * store)
{
    const cJSON * q = cJSON_GetObjectItemCaseSensitive(j, item);
    if (cJSON_IsNumber(q))
    {
        *store = q->valuedouble;
    } else {
        printf("cJSON_IsNumber failed %d\n", __LINE__);
    }
    return 0;
}

char * get_json_string(const cJSON * j, const char * item)
{
    const cJSON * q = cJSON_GetObjectItemCaseSensitive(j, item);
    if(cJSON_IsString(q) && (q->valuestring != NULL))
    {
        char * ret = strdup(q->valuestring);
        return ret;
    }
    fprintf(stderr, "ERROR: Failed to parse %s on line %d\n", item, __LINE__);
    return NULL;
}
