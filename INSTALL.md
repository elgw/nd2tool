## Ubuntu 24.04

The Nikon libraries requires `libtiff.so.5`, get it from some other
package (as it can't be found among the standard libraries) and place
it in the system library path.

In the `CMakeLists.txt` update so that there is:

``` cmake
# target_link_libraries(nd2tool tiff)
target_link_libraries(nd2tool -l:libtiff.so.5)
```


The libraries will be installed under `/usr/local/lib` so make sure
that this path is searched for, either by adding

```
include /usr/local/lib
```
to
`sudo emacs /etc/ld.so.conf`

or by adding

``` shell
EXPORT LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```
to the `.bashrc` file.
