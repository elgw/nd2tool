#pragma once

#define ND2TOOL_VERSION_MAJOR "0"
#define ND2TOOL_VERSION_MINOR "1"
#define ND2TOOL_VERSION_PATCH "6"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/** @brief command line interface for the nd2tool */
int nd2tool_cli(int argc, char ** argv);
