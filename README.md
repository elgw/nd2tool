# nd2tool

## Introduction
Provides the command line tool, [**nd2tool**](doc/nd2tool.txt), that
can be used to:
1. make a short summary of the meta data in a nd2 file (**--info**),
2. extract all metadata (**--meta**) from nd2 file or specific
portions (**--meta-file**, **--meta-coord**, **--meta-frame**,
**--meta-text**, **--meta-exp**), and,
3. export the image data to tif files, one per Field of View (FOV)
and color, in a memory efficient way. Internally **nd2tool** only
read and write one image plane at a time can convert large files
using little RAM, for example a 23 GB ND2 image can typically be
converted to till files using less than 100 Mb of RAM.
**nd2tool** ...
- should be considered as **experimental** since it is only tested on
a few images. You could help by sharing your data or find a tool
that suits you better in the [references](#references).
- only supports loops over XY, Color and Z, i.e., not over time.
- only supports data stored as 16-bit unsigned int.

## Example usage
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

See the [man page](doc/nd2tool.txt) for the full documentation
(i.e. `man nd2tool`) or use `nd2tool --help` for a quick recap.

## Installation

The standard procedure: get required libraries, compile and then
install. These instructions are for Ubuntu 22.04 LTS (might work on
WSL for Windows as well).

```
# get dependencies
sudo apt-get update
sudo apt-get install libcjson1 libcjson-dev libtiff5-dev build-essential
make release  # compile
```

After building, the preferred way to install is to first make a .deb
file like this:

```
./makedeb_ubuntu_2204.sh
sudo apt-get install ./nd2tool_*.deb
# Then you can uninstall with
# sudo apt-get remove nd2tool
```

Alternatively this could also be used, please check the `makefile` so
that the install paths makes sense on your machine.
```
sudo make install   # Install binary and man page
```

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
