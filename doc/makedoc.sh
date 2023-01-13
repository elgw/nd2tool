#!/bin/bash
set -e

srcdir=../src/
ver_major=`sed -rn 's/^#define.*ND2TOOL_VERSION_MAJOR.*([0-9]+).*$/\1/p' < $srcdir/nd2tool.h`
ver_minor=`sed -rn 's/^#define.*ND2TOOL_VERSION_MINOR.*([0-9]+).*$/\1/p' < $srcdir/nd2tool.h`
ver_patch=`sed -rn 's/^#define.*ND2TOOL_VERSION_PATCH.*([0-9]+).*$/\1/p' < $srcdir/nd2tool.h`
ND2TOOL_VERSION="${ver_major}.${ver_minor}.${ver_patch}"

tempfile=$(mktemp)
sed "s/ND2TOOL_VERSION/${ND2TOOL_VERSION}/g" nd2tool.md > ${tempfile}

pandoc ${tempfile} --standalone --from markdown --to man --output nd2tool.1
rm -f ${tempfile}

man ./nd2tool.1 | col -b > nd2tool.txt
