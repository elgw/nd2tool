## Ubuntu 24.04

The Nikon libraries requires `libtiff.so.5` which is not present among
the Ubuntu 24 packages. You can install it from a previous release with:

``` shell
wget http://security.ubuntu.com/ubuntu/pool/main/t/tiff/libtiff5_4.3.0-6ubuntu0.10_amd64.deb
ar -x libtiff5_4.3.0-6ubuntu0.10_amd64.deb
tar --zstd -xvf data.tar.zst
sudo cp usr/lib/x86_64-linux-gnu/libtiff.so.* /usr/lib/x86_64-linux-gnu/
```

### Complete install steps
On 2024-09-10 I tested to install nd2tool using
`ubuntu-24.04.1-live-server-amd64.iso` and virtualbox, these steps
were required:

``` shell
sudo apt-get install git make gcc
libcjson-dev cmake g++ libtiff5-dev

# Get the older version of libtiff
wget http://security.ubuntu.com/ubuntu/pool/main/t/tiff/libtiff5_4.3.0-6ubuntu0.10_amd64.deb
ar -x libtiff5_4.3.0-6ubuntu0.10_amd64.deb
tar --zstd -xvf data.tar.zst
sudo cp usr/lib/x86_64-linux-gnu/libtiff.so.* /usr/lib/x86_64-linux-gnu/

git clone https://github.com/elgw/nd2tool.git

cd nd2tool
mkdir build
cd build
make
sudo make install
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```

To make the last line permanent, see [library paths](#library-paths) below

## libtiff5 only
As a side note, it is possible to use only libtiff5. In that case
 update `CMakeLists.txt` to be:

``` cmake
# target_link_libraries(nd2tool tiff)
target_link_libraries(nd2tool -l:libtiff.so.5)
```


## Library paths

Either add
``` shell
EXPORT LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```
to the `.bashrc` file (if you use bash)

or

add

```
include /usr/local/lib
```
to
`sudo emacs /etc/ld.so.conf`
