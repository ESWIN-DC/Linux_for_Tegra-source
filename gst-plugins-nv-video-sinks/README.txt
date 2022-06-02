###############################################################################
#
# Copyright (c) 2019-2022, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
###############################################################################

Steps to compile the "gst-plugins-nv-video-sinks" sources:

1) Install gstreamer related packages using the command:

	sudo apt-get install gstreamer1.0-tools gstreamer1.0-alsa \
		gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
		gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
		gstreamer1.0-libav libgstreamer1.0-dev \
		libgstreamer-plugins-base1.0-dev libegl1-mesa-dev

2) Install CUDA Runtime 10.0+

3) Extract the package "libgstnvvideosinks_src.tbz2" as follow:

	tar xvjf libgstnvvideosinks_src.tbz2`

4) cd "gst-plugins-nv-video-sinks"

5) Export the appropriate CUDA_VER using - "export CUDA_VER=<cuda-version>"

5) run "make" to create "libgstnvvideosinks.so"

6) run "sudo make install" to install "libgstnvvideosinks.so" in
   "/usr/lib/aarch64-linux-gnu/gstreamer-1.0".

7) run "make install DEST_DIR=<location>" to install at different <location>.
