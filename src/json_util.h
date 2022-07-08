#ifndef __json_util_h__
#define __json_util_h__

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

/* Get a number and place it in store, returns 0 on success */
int get_json_int(const cJSON * j, const char * item, int * store);
int get_json_double(const cJSON * j, const char * item, double * store);

/* Returns A newly allocated string or NULL on failure */
char * get_json_string(const cJSON * j, const char * item);

#endif
