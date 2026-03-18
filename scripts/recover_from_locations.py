progdesc = """recover image data from nd2 files with missing chunk map"""

import argparse
import tifffile
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os


def load_single_ImageDataSeq(fid, config, offset):
    w = config['width']
    h = config['height']
    nc = config['nc']
    images = []
    fid.seek(0)
    data = np.fromfile(fid, dtype=np.uint16, offset=offset-16, count=w*h*nc)
    for cc in range(0, nc):
        data1 = data.copy()
        I = data1[cc::nc]
        I = I.reshape((w, h))
        images.append(I)
    return images

def load_fov(fid, config, offsets, fov):
    nz = config.planes
    w = config.width
    h = config.height
    nc = config.ncolor
    image_ids = range(fov*nz, (fov+1)*nz)
    # print(image_ids)
    images = []
    for i in range(0, nc):
        images.append(np.zeros((nz, h, w), dtype=np.uint16))

    off = offsets[image_ids]
    for zz in range(0, nz):
        fid.seek(0)
        data = np.fromfile(fid, dtype=np.uint16, offset=off[zz], count=w*h*nc)
        for cc in range(0, nc):
            data1 = data.copy()
            I = data1[cc::nc]
            I = I.reshape((w, h))
            images[cc][zz, :, :] = I
        # plt.imshow(images[0][0, :, :], cmap='gray'); plt.show()
    return images

def parse_command_line_arguments():

    parser = argparse.ArgumentParser(
        description=progdesc,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('--nd2', required=True,
                        help='nd2 file to use')
    parser.add_argument('--width', required=True,
                        help='Image width in pixels',
                        type=int)
    parser.add_argument('--height', required=True,
                        help='Image height in pixels',
                        type=int)
    parser.add_argument('--ncolor', required=True,
                        help='Number of image colors/channels',
                        type=int)
    parser.add_argument('--planes', required=True,
                        help='Number of image planes/z-levels',
                        type=int)
    parser.add_argument('--bps', required=False, default=2,
                        help='Bytes per sample/sensor bits',
                        type=int)
    parser.add_argument('--overwrite', required=False, default=False, action='store_true')
    parser.add_argument('--plot', default=False, help="Plot some stuff");

    config = parser.parse_args()
    config.locfile = config.nd2 + '.recover_locations.csv'
    config.chunksize = config.width*config.height*config.bps*config.ncolor
    return config

def check_sizes_and_offsets(config, offsets):
    print(f"Expected chunk size: {config.chunksize} (Width x Height x nColor x BPS)")
    if len(offsets) < 2:
        return
    max_chunk = offsets[1]-offsets[0] - 4096 + 16
    for ii in range(1, len(offsets)):
        csize = offsets[ii]-offsets[ii-1] - 4096 + 16
        max_chunk = min(csize, max_chunk)
    if max_chunk >= config.chunksize:
        if max_chunk > config.chunksize + 5e5:
            print(f"Warning: Large slack, chunks can be {max_chunk} bytes which is {max_chunk/config.chunksize*100:.1f}% of the chunk size")
            return
        else:
            print(f"Parameters are feasible, chunks are at least {max_chunk} bytes")
            return
    else:
        print(f"Error -- the chunks in the file are smaller {max_chunk} than the expected chunk size")
        sys.exit(-1)

def plot_some_stuff(config, offsets):
    ImageDataSeq_nr = 929
    with open(config.nd2file, "rb") as fid:
        images = load_single_ImageDataSeq(fid, config, offsets[185])

    fix, axes = plt.subplots(nrows=1, ncols=len(images))
    for c in range(0, len(images)):
        ax = axes[c]
        ax.imshow(images[c]) #, cmap='gray')
    plt.suptitle(f"ImageDataSeq|{ImageDataSeq_nr}!", fontsize=14)
    plt.show()

    breakpoint()

    fov = 10 # 0-indexed
    with open(nd2file, "rb") as fid:
        images = load_fov(fid, config, offsets, 0)

        #breakpoint()

    fig, axes = plt.subplots(nrows = 1, ncols=config['nc'])
    for c in range(0, config['nc']):
        ax = axes[c]
        ax.imshow(images[c][39, :, :], cmap='gray') #, clim=[0,5000])


    plt.show()
    plt.imshow(images[0][40, :, :], cmap='gray'); plt.show()
    breakpoint()


if __name__ == '__main__':

    config = parse_command_line_arguments()
    loc = pd.read_csv(config.locfile, delimiter=',')
    offsets = np.array(loc['image_position'])

    check_sizes_and_offsets(config, offsets)

    # Load and show a single ImageDataSeq, please note that all indexes
    # are 0-based
    # check image id 185, 370, 555, 740, 925

    if config.plot:
        plot_some_stuff(config, offsets)


    outdir = config.nd2[0:-4] + '_recover/'

    print(f"Creating {outdir}")
    if not os.path.exists(outdir):
        os.mkdir(outdir)

    nfov = len(offsets)//config.planes
    print(f"Will try to recover {nfov} FOV")

    with open(config.nd2, "rb") as fid:
        for fov in range(0, nfov):
            images = load_fov(fid=fid, config=config, offsets=offsets, fov=fov)
            for ii, Im in enumerate(images):
                outname = f"{outdir}c{ii+1:02}_{fov+1:03}.tif"
                if os.path.exists(outname) and not config.overwrite:
                    print(f"{outname} already exists")
                else:
                    print(f"Writing {outname}")
                    tifffile.imwrite(outname, Im)
