# nd2tool v0.1.7

## Introduction
Provides the command line tool, [**nd2tool**](doc/nd2tool.txt), that
can be used to:
1. make a short summary of the meta data in a nd2 file (**--info**),
2. extract all metadata (**--meta**) from nd2 file or specific
portions (**--meta-file**, **--meta-coord**, **--meta-frame**,
**--meta-text**, **--meta-exp**), and,
3. export the image data to tif file(s). Either one per Field of View
(FOV) (**--composite**), or one file per FOV and color.

The RAM usage is low. Only one image plane is loaded into RAM at the
same time. As an example, a 23 GB ND2 image can typically be converted
to tiff files using less
than 100 Mb of RAM.

**nd2tool** should be considered as **experimental** since it is only
tested on a few images. If it does not work for your images, please
submit a bug report, or simply find a tool that suits you better; some
alternatives are listed in the [references](#references). At the
moment it only supports loops over XY, Color and Z, i.e., not over
time. It is furthermore limited to nd2 files where the image data is
stored as 16-bit unsigned int.

Supported platforms: Linux. If you want to have this running under
macOS or Windows, let me know.

## Usage

See the [man page](doc/nd2tool.txt) for the full documentation
(i.e. `man nd2tool`) or use `nd2tool --help` for a quick recap.

### Basic usage -- conversion
This will convert an nd2 file to a set of tif files. The output will
be a folder named after the nd2 file, but without the `.nd2`
extension.

```
$ nd2tool iiQV015_20220630_001.nd2
3 FOV in 4 channels:
   #1 '  A647', λ_em=710.0 #E10000  ir
   #2 'SpGold', λ_em=572.0 #FFFF00
   #3 '  A488', λ_em=543.0 #00FF00
   #4 '  dapi', λ_em=385.0 #8900FF  uv
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

### Export the coordinates from multi-FOV images

Coordinates are useful for stitching, detection of overlapping
regions, detection of mechanical instabilities etc. The example
command below gives output in a comma separated format (CSV).


``` shell
$ nd2tool --coord iMS441_20191016_001.nd2
FOV, Channel, Z, X_um, Y_um, Z_um
1, 1, 1, -3523.100000, 4802.500000, 2120.857777
1, 1, 2, -3523.100000, 4802.500000, 2121.457777
1, 1, 3, -3523.100000, 4802.500000, 2122.057777
...
```

### Generation of deconvolution script
nd2tool can generate scripts to use with
[deconwolf](https://www.github.com/elgw/deconwolf/). Either it creates
a script that has to be edited manually **--deconwolf** or it asks for
the key parameters interactively with **--deconwolfx** as in the
example below:

``` shell
$ nd2tool --deconwolfx iMS441_20191016_001.nd2
Writing to deconwolf_iMS441_20191016_001.nd2.sh
Enter the number of iterations to use
dapi (default=50)
> 50
A647 (default=50)
> 75
Enter any extra arguments to deconwolf
> --gpu
```

For this file the generated file was:

``` shell
cat deconwolf_iMS441_20191016_001.nd2.sh
#!/bin/env bash
set -e # abort on errors

# PSF Generation
dw_bw --lambda 385.000000 --resxy 108.333333 --resz 600.000000 --NA 1.400000 --ni 1.515000 'iMS441_20191016_001/PSF_dapi.tif'
dw_bw --lambda 710.000000 --resxy 108.333333 --resz 600.000000 --NA 1.400000 --ni 1.515000 'iMS441_20191016_001/PSF_A647.tif'
iter_dapi=50
iter_A647=75
xargs="--gpu"
dw "$xargs" --iter $iter_dapi 'iMS441_20191016_001/dapi_001.tif' 'iMS441_20191016_001/PSF_dapi.tif'
dw "$xargs" --iter $iter_A647 'iMS441_20191016_001/A647_001.tif' 'iMS441_20191016_001/PSF_A647.tif'
```

## Installation

The standard procedure: get required libraries, compile and then
install. These instructions are for Ubuntu 22.04 LTS (might work on
WSL for Windows as well).

### get dependencies
```
sudo apt-get update
sudo apt-get install libcjson1 libcjson-dev libtiff5-dev build-essential
```

### compile
``` shell
mkdir build
cd build
cmake ..
cmake build
```

### install
Either bypass the system package manager
``` shell
make install
```

or do it the proper way

``` shell
make package
sudo apt-get install ./nd2tool-0.1.7-Linux.deb
```

of course, you will need to adjust the last line to fit the system
package manager unless you are on Ubuntu.

## Reporting bugs
Please use [the issue tracking system on
github](https://github.com/elgw/nd2tool/issues) to report bugs or to
get in touch. Have a look on the [roadmap](ROADMAP.md) before
submitting suggestions or making pull requests.

## References
Alternative command line tools for nd2 files:
- [ggirelli/radiantkit](https://github.com/ggirelli/radiantkit) - Command line
tool for conversion from nd2 to tiff as well as lots of other
stuff. Unfortunately the repository has been archived but the code
is still useful. Uses the Python package nd2reader.

GUI tools for nd2 files:
- [Nikon NIS Elements
(Viewer)](https://www.microscope.healthcare.nikon.com/products/software/nis-elements/viewer) unfortunately not for Linux.
- [FIJI/ImageJ](https://imagej.net/software/fiji/)

Python libraries/tools for nd2 files:
- [Open-Science-Tools/nd2reader](https://github.com/Open-Science-Tools/nd2reader) a pure
Python package for reading nd2 files. I.e. not using the Nikon library.
- [tlambert03/nd2](https://github.com/tlambert03/nd2) a Python
package that uses and ships Nikons shared objects.

Libraries used by **nd2tool**:
- [cJSON](https://github.com/DaveGamble/cJSON) for parsing JSON data.
- [libTIFF](http://www.libtiff.org) for writing tif files.
- [Nikon's nd2 library](https://www.nd2sdk.com/) for reading nd2
files (with permission to redistribute the shared objects).
- [the GNU C library](https://www.gnu.org/software/libc/)

Related/useful:
- [Interactively explore JSON
data](https://jsonformatter.org/json-viewer) Copy and paste the
JSON output from nd2tool (**--meta-file**, **--meta-frame**,
**--meta-exp**) here for a quick overview. See also [JSON
specification](https://www.json.org/) if you are new to this.
- [deconwolf](https://www.github.com/elgw/deconwolf/) for deconvolution of wide
  field images (and possibly also other modalities).
