# 0.1.5
- Added checks for some unlikely scenarios. Builds cleanly with clang,
  i.e., with `scan-build make -B CC=clang DEBUG=1` as well as gcc
  `make -B DEBUG=1`.

# 0.1.4
- Added the **--dry** option for dry runs which tells the program not
  to create or modify any folders or files.
- Added the **--deconwolf** option to generate a script with commands
  to run deconwolf on the tif files (unless converted with the
  **--composite** flag).
- Factored out some code into `nd2tool_util.c`

# 0.1.3
- Experimental support for writing composite images with the
   **--composite** flag. It remains to choose color maps that makes
   sense.

# 0.1.2
- Added the **--coord** command line option that will print the
  position of all images in csv format to the standard output. To be
  validated together with **--shake**.

# 0.1.1
- Experimental (still to be validated and hence not enabled by
  default) shake detection in the axial direction, enable with the
  **--shake** command line argument.

# 0.1.0
- Slightly improved log file messages.

# 0.0.6
- Fixed problem with non quadratic image sizes.
- Not saying when it is using the BigTIFF format any more.

# 0.0.5
- added approximate conversion from emission wavelength to color.
