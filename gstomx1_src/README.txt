This package contains the code for libgstomx.so library

DEPENDENCIES:
------------
(1) EGL
(2) GstEGL (use the one distributed in the same release)
(3) Gstreamer >= 1.2.3

The above dependencies(except (2)) can be obtained from any standard distribution
(like https://launchpad.net/ubuntu) or can be self-built.
This package was built against the (above) libs present in Ubuntu 16.04.
Please note that below instructions will build libgstomx.so for aarch64.

1. Pre-Reqs

Install the required packages as below:

sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev autotools-dev \
autoconf autopoint automake pkg-config libtool gtk-doc-tools libgtk2.0-dev libegl1-mesa-dev

INSTALLATION:
-------------
1) Untar the package and enter the dir
2) export NOCONFIGURE=true
3) export GST_EGL_LIBS="-lgstnvegl-1.0 -lEGL -lX11 -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0"
4) sudo ln -s /usr/lib/aarch64-linux-gnu/libgstnvegl-1.0.so.0 /usr/lib/aarch64-linux-gnu/libgstnvegl-1.0.so
5) ./autogen.sh
6) ./configure
Note:
Pass the appropriate flags and toolchain path to "configure" for tha ARM build.
To configure on tegra platform, pass "--with-omx-target=tegra" param to configure.
7) make
8) sudo make install

