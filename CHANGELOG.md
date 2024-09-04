# Changelog for nd2tool

## 0.1.8

- Bug fix, avoids crashing if the metadata for the objective is
  missing. This happened in an image from a microscope with a broken
  objective turret.

  After the fix, this is what nd2tool reports:

  ```
  nd2tool iiJPC526_20240828_001.nd2 --info
  Warning: Could not find channel/microscope/immersionRefractiveIndex
  Warning: Could not find channel/microscope/objectiveNumericalAperture
  5 FOV in 3 channels:
     #1 'a594', λ_em=590.0 #FFB300
     #2 ' tmr', λ_em=542.0 #00FF00
     #3 'dapi', λ_em=432.0 #9900FF
  Bits per pixel: 16, significant: 16
  dx=1000.0 nm, dy=1000.0 nm, dz=300.0 nm
  NA=0.000, ni=0.000
  Objective Name: Uncalibrated
  Objective Magnification: 1.0X
  Volume size: 1024 x 1024 x 41
  Looping: Dimensions: XY(5) x λ(3) x Z(41)
  Camera: Andor DU-888 X-9877
  ```

  If it wasn't obvious that the information was wrong here, we could
  make a special case when the objective name is set to `Uncalibrated`.

  In that case it does not make sense to report these values (found in
  the metadata!)

  ```
  Objective Magnification: 1.0X
  dx=1000.0 nm, dy=1000.0 nm
  ```

  Furthermore we should also prevent nd2tool from reporting these
  default values:

  ```
  NA=0.000, ni=0.000
  ```


## 0.1.7
- Updating to Nikon libraries v 1.7.6.
- Total switch to cmake for building and package creation.

## 0.1.6
- Parsing Camera name and Microscope name from the text section of the
  metadata. This part of the metadata isn't very well structured so I
  wouldn't be surprised if it doesn't work for all files. If it
  doesn't work no info will be printed and that's all.

## 0.1.5
- Added checks for some unlikely scenarios. Builds cleanly with clang,
  i.e., with `scan-build make -B CC=clang DEBUG=1` as well as gcc
  `make -B DEBUG=1`.
- Added the **--SpaceTx** option to save one image per plane according
  to the SpaceTx convention. This closes
  [https://github.com/elgw/nd2tool/issues/4]. Please note that for
  **fov_id** the name of the nd2 file is used (which can contain
  underscores) and 0 is used for **round_label**. In other words, you
  might need to rename the files (for example with **rename** in
  linux) before processing them with starfish.


## 0.1.4
- Added the **--dry** option for dry runs which tells the program not
  to create or modify any folders or files.
- Added the **--deconwolf** option to generate a script with commands
  to run deconwolf on the tif files (unless converted with the
  **--composite** flag).
- Factored out some code into `nd2tool_util.c`

## 0.1.3
- Experimental support for writing composite images with the
   **--composite** flag. It remains to choose color maps that makes
   sense.

## 0.1.2
- Added the **--coord** command line option that will print the
  position of all images in csv format to the standard output. To be
  validated together with **--shake**.

## 0.1.1
- Experimental (still to be validated and hence not enabled by
  default) shake detection in the axial direction, enable with the
  **--shake** command line argument.

## 0.1.0
- Slightly improved log file messages.

## 0.0.6
- Fixed problem with non quadratic image sizes.
- Not saying when it is using the BigTIFF format any more.

## 0.0.5
- added approximate conversion from emission wavelength to color.
