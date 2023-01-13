# ROADMAP

In general nd2tool tries to keep the number of dependencies at a
minimum and will not try to compete in features with other existing
tools.

## To do

- [ ] Collect various nd2 files for testing, at the moment not tested
on time series at all (feel free to share your data).
- [ ] Indicate where the most in focus slice is.
- [ ] Support MACOS
- [ ] Warn/indicate of images are overlapping.

## Maybe some day
- [ ] Enable shake detection by default.
- [ ] check that the channel names are valid file names?
- [ ] As alternative to the Nikon library, consider adopting from
[Open-Science-Tools/nd2reader](https://github.com/Open-Science-Tools/nd2reader).
- [ ] Write tiff files without the tiff library like on
[fTIFFw](https://github.com/elgw/fTIFFw) for some extra speed.
- [ ] bash auto-completion.
- [ ] Option to export one file per FOV, channel and z-pos.
- [ ] Custom color maps for multi channel images.

## Done
- [x] Option to export as multi-color tif files. -- Requested in [issue
#1](https://github.com/elgw/nd2tool/issues/1) This was implemented in
version 0.1.3 and is enabled with the **--composite** flag.
- [x] Check for inconsistent dz values between image planes (shake
      detection with **--shake**).
- [x] Export position of images as csv file (**--coord**).
- [x] Supports writing BigTIFF images, i.e., > 2 Gb (at least one
image of size 14607 x 14645 x 17 worked out fine).
- [x] Include Nikon's nd2-library in the repo.
- [x] Metadata about resolution is transferred from nd2 files to tif
files so that the correct resolution is found by ImageJ.
- [x] It is checked that the image data is stored as 16-bit,
otherwise the program quits.
- [x] One log file is written per nd2 image that is converted.
- [x] Only keeps one image plane in RAM at the same time in order to
keep the memory usage low.
- [x] Safe writing: Using a temporary file (using `mkstemp`) for
writing. Renaming the temporary file to the final name only
after the writing is done. Prevents corrupt file being
written. Will leave files like `file.tif_tmp_XXXXXX` upon
failure that has to be removed manually.
