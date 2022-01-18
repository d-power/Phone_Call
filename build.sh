#!/bin/sh

compile=arm-linux-gnueabi

target=$(pwd)/release

if [ ! -d $target ]; then
    mkdir -p $target
fi

aclocal
autoheader
libtoolize --automake --copy --debug --force
automake --add-missing
autoconf

# set -x

BUILD=`uname -m`

FLAGS="-D$platform"

./configure --prefix=$target \
        --build=$BUILD  \
        --host=$compile \
        CFLAGS="-fPIC -D_GNU_SOURCE -Os -Wall -Werror"   \
        CXXFLAGS="-fPIC -D_GNU_SOURCE -Os -Wall -Werror" \
