#!/bin/bash
set -e # exit on error

NAME="Erik Wernersson"
EMAIL="https://github.com/elgw/nd2tool/issues"



if [ $EUID = 0 ]; then
    echo "WARNING: You are running as root, please abort!"
    echo "Sleeping for 10 s"
    sleep 10
fi

savedir="$(pwd)"
builddir=$(mktemp -d)
cd $builddir
pkgdir=$(mktemp -d)/nd2tool
echo "pkgdir=$pkgdir"

ver_major=`sed -rn 's/^#define.*ND2TOOL_VERSION_MAJOR.*([0-9]+).*$/\1/p' < $savedir/src/nd2tool.h`
ver_minor=`sed -rn 's/^#define.*ND2TOOL_VERSION_MINOR.*([0-9]+).*$/\1/p' < $savedir/src/nd2tool.h`
ver_patch=`sed -rn 's/^#define.*ND2TOOL_VERSION_PATCH.*([0-9]+).*$/\1/p' < $savedir/src/nd2tool.h`
pkgver="${ver_major}.${ver_minor}.${ver_patch}"
echo "pkgver=$pkgver"
arch=$(dpkg --print-architecture)
echo "arch=$arch"

# Copy files to the correct places
# binary
mkdir -p $pkgdir/usr/local/bin
cp $savedir/bin/nd2tool-linux-amd64 $pkgdir/usr/local/bin/nd2tool
# libraries
mkdir -p $pkgdir/usr/local/lib
cp $savedir/lib/liblimfile.so $pkgdir/usr/local/lib/
cp $savedir/lib/libnd2readsdk-shared.so $pkgdir/usr/local/lib/
# man pages
mkdir -p $pkgdir/usr/share/man/man1/
cp $savedir/doc/nd2tool.1 $pkgdir/usr/share/man/man1/

cd $pkgdir
size=$(du -k usr | tail -n1 | sed 's/usr//')

mkdir $pkgdir/DEBIAN/
cat > $pkgdir/DEBIAN/control << END
Package: nd2tool
Version: $pkgver
Architecture: $arch
Section: science
Depends: libcjson1, libtiff5
Conflicts:
Maintainer: $NAME <$EMAIL>
Installed-Size: $size
Priority: optional
Homepage: https://github.com/elgw/nd2tool/
Description: nd2tool
 A command line tool for conversion from nd2 to tif
END

cd $pkgdir/../
dpkg-deb --build nd2tool nd2tool_${pkgver}_${arch}.deb
mv nd2tool_${pkgver}_${arch}.deb "$savedir"
