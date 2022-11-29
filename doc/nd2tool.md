% ND2TOOL(1) nd2tool
% Erik Wernersson
% 2022

# NAME
**nd2tool** is a tool converting Nikon nd2 files to the tif image
format. It can also be used to extract metadata from nd2 files without
conversion.

# SYNOPSIS

**nd2tool** [OPTIONS] file1.nd2 file2.nd2 ...

# OPTIONS
**-i, \--info**
: Display basic information from the metadata but do no conversion

**-v l**, **\--verbose l**
: Set verbosity level to l. 0=quite, 1=normal, >1 show memory usage
  etc >2 also dump meta data as it is read.

**-o**, **\--overwrite**
: Overwrite existing tif files.

**-V**, **\--version**
: Show version information and quit.

**-h**, **\--help**
: Show a short help message and quit.

**\--meta**
: Extract all metadata and write to stdout. This is seldom useful,
  please see the following options.

**\--meta-file**
: Print the JSON metadata returned by the API call Lim_FileGetMetadata.

**\--meta-coord**
: Print the text metadata returned by the API call Lim_FileGetCoordInfo.

**\--meta-frame**
: Print the JSON metadata returned by Lim_FileGetFrameMetadata.

**\--meta-text**
: Print the text metadata returned by the API call Lim_FileGetTextinfo.

**\--meta-exp**
Print the JSON metadata returned by Lim_FileGetExperiment.

# INPUT
**nd2tool** should be capable to convert nd2 files where the image
data is stored as 16-bit unsigned integers. It can currently not
handle images with time loops.

# OUTPUT
If nd2tool is run by

``` shell
nd2tool iiQV015_20220630_001.nd2
```

it will create the following files:

``` shell
iiQV015_20220630_001
├── A488_001.tif
├── A488_002.tif
├── A488_003.tif
├── A647_001.tif
├── A647_002.tif
├── A647_003.tif
├── dapi_001.tif
├── dapi_002.tif
├── dapi_003.tif
├── nd2tool.log.txt
├── SpGold_001.tif
├── SpGold_002.tif
└── SpGold_003.tif
```
i.e. a new folder with the same name as the input file excluding the file
extension `.nd2`. Then each Field of View (FOV) and channel will be saved as a
separate file with the scheme `CHANNEL_FOV.tif` were FOV is padded with 0s to
always be three digits.


# NOTES
The meta data extraction should work in most cases even if the
conversion does not. If meta data can't be parsed from a file it is
probably corrupt or not a nd2 file. Meta data should be appended to
all bug reports.


# WEB PAGE
[https://github.com/elgw/nd2tool/](https://github.com/elgw/nd2tool/)

# REPORTING BUGS
Please report bugs at
[https://github.com/elgw/nd2tool/issues/](https://github.com/elgw/nd2tool/issues/)
