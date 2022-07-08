# nd2tool

 - [Introduction](#introduction)
 - [Usage](#usage)
   - [Installation](#installation)
   - [Example usage](#example-usage)
 - [Development](#development)
   - [Todo](#todo)
   - [Done](#done)
   - [Reporting bugs](#reporting-bugs)
 - [References](#references)

## Introduction
Provides the command line tool, [**nd2tool**](doc/nd2tool.txt), that
can be used to:
 1. Make a short summary of the meta data in a nd2 file (**-i**).
 2. extract all metadata (**--meta**) from nd2 file or specific
    portions (**--meta-file**, **--meta-coord**, **--meta-frame**,
    **--meta-text**, **--meta-exp**), and,
 3. export the image data to tif files, one per Field of View (FOV)
    and color, in a memory efficient way. Internally **nd2tool** only
    read and write one image plane at a time can convert large files
    using little RAM, for example a 23 GB ND2 image can typically be
    converted to till files using less than 100 Mb of RAM.

Current Limitations:
 - To be considered as **experimental** since it is only tested on a
    few images.
 - Only supports loops over XY, Color and Z, i.e., not over time.
 - Only supports data stored as 16-bit unsigned int.
 - Linux only.
 - Does not cover all edge cases, have a look at the
[references](#references) to find the best tool for you.

Project goals
 1. Provide a small (at the moment < 3 Mb including the Nikon
    libraries) and stable command line tool for the basic things that
    I need to do with nd2 files.
 2. Keep the number of dependencies small and only use standard
    packages.
 3. Be flexible and support the needs for your processing.


## Usage

### Installation
The standard procedure, get libraries, compile, make available.
```
sudo apt-get update
sudo apt-get install libcjson1 libcjson-dev libtiff5-dev build-essential
# Get Nikon libraries and header from https://github.com/tlambert03/nd2
make getsdk
# install Nikon's libraries so that they can be found duing build and run
sudo cp lib/*.so /usr/local/lib/
make
# Install binary and man page
sudo make install
```

### Example usage
```
nd2tool iiQV015_20220630_001.nd2
3 FOV in 4 channels:
   #1 'A647', λ_em=710.0
   #2 'SpGold', λ_em=572.0
   #3 'A488', λ_em=543.0
   #4 'dapi', λ_em=385.0
Bits per pixel: 16, significant: 16
dx=324.5 nm, dy=324.5 nm, dz=300.0 nm
NA=0.750, ni=1.000
Objective Name: Plan Apo VC 20x DIC N2
Objective Magnification: 20.0X
Volume size: 2048 x 2048 x 241
Looping: Dimensions: XY(3) x λ(4) x Z(241)
Writing to iiQV015_20220630_001/A647_001.tif
Writing to iiQV015_20220630_001/SpGold_001.tif
...
Writing to iiQV015_20220630_001/dapi_003.tif
```

See the [man page](doc/nd2tool.txt) for the full documentation.


## Development

**nd2tool** uses [Nikon's nd2 library](https://www.nd2sdk.com/) to
extract metadata. Until I get permissions to include it here it can be
found at [tlambert03/nd2](https://github.com/tlambert03/nd2).

The JSON metadata is parsed by
[cJSON](https://github.com/DaveGamble/cJSON) and images are saved as
tiff using [libTIFF](http://www.libtiff.org).


### Todo

 - [ ] Safe writing: Write to temporary file name, and rename to final name when
       writing is done.
 - [ ] Collect various nd2 files for testing.
 - [ ] Check for inconsistent dz values between image planes (shake detection).
 - [ ] Get permission from Nikon to distribute nd2sdk together
       with this repo. Request sent 20220701.
 - [ ] check that the channel names are valid file names and possibly
       convert spaces to underscores. Use toupper or tolower?
 - [ ] Support time series.

Maybe some day:

 - [ ] As an option to the Nikon library, consider adopting from
       [Open-Science-Tools/nd2reader](https://github.com/Open-Science-Tools/nd2reader).
 - [ ] Write tiff files without the tiff library like on
       [fTIFFw](https://github.com/elgw/fTIFFw) for some extra speed.

### Done

 - [x] Correct resolution set when opening in ImageJ.
 - [x] Double check that the image data is 16-bit (will be the only supported).
 - [x] One log file per nd2 image.

### Reporting bugs
Please use [the issue tracking system on
github](https://github.com/elgw/nd2tool/issues) to report bugs.

## References
 - [Interactively explore JSON
   data](https://jsonformatter.org/json-viewer) Copy and paste the
   JSON output from nd2tool (**--meta-file**, **--meta-frame**,
   **--meta-exp**) here for a quick overview. See also [JSON
   specification](https://www.json.org/) if you are new to this.

Alternative command line tools:
 - [ggirelli/radiantkit](https://github.com/ggirelli/radiantkit) - Command line
   tool for conversion from nd2 to tiff as well as lots of other
   stuff. Unfortunately the repository has been archived but the code
   is still useful. Uses the Python package nd2reader.

Python libraries/tools:
 - [Open-Science-Tools/nd2reader](https://github.com/Open-Science-Tools/nd2reader) a pure
   Python package for reading nd2 files. I.e. not using the Nikon library.
 - [tlambert03/nd2](https://github.com/tlambert03/nd2) a Python
   package that uses and ships Nikons shared objects.

Libraries used by **nd2tool**:
 - [cJSON](https://github.com/DaveGamble/cJSON) for parsing JSON data.
 - [libTIFF](http://www.libtiff.org) for writing tif files.
 - [Nikon's nd2 library](https://www.nd2sdk.com/) for reading nd2 files.
 - [the GNU C library](https://www.gnu.org/software/libc/)
