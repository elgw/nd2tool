# 0.1.6
- Parsing Camera name and Microscope name from the text section of the
  metadata. This part of the metadata isn't very well structured so I
  wouldn't be surprised if it doesn't work for all files. If it
  doesn't work no info will be printed and that's all.

# 0.1.5
- Added the **--SpaceTx** option to save one image per plane according
  to the SpaceTx convention. This closes
  [https://github.com/elgw/nd2tool/issues/4]. Please note that for
  **fov_id** the name of the nd2 file is used (which can contain
  underscores) and 0 is used for **round_label**. In other words, you
  might need to rename the files (for example with **rename** in
  linux) before processing them with starfish.

# 0.1.4
-  Added the **--dry** option for dry runs which tells the program not
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
