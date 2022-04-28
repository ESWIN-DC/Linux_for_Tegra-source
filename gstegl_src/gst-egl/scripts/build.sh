#!/bin/bash

CURDIR="$(dirname $0)"
TOPDIR="$(dirname $0)/.."

pushd $TOPDIR

./configure
cp $CURDIR/Makefile.public Makefile
CUDA_VER=10.2 make

popd

# And then replace libgstnveglglessink.so:
# gstegl_src/gst-egl$ sudo cp libgstnveglglessink.so /usr/lib/aarch64-linux-gnu/gstreamer-1.0/
