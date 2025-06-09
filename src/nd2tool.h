#pragma once

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "version.h"

/** @brief command line interface for the nd2tool */
int nd2tool_cli(int argc, char ** argv);
