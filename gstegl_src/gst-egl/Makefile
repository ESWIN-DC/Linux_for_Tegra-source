################################################################################
# Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
################################################################################

CC:= gcc
TARGET_NAME:= libnvdsgst_eglglessink.so

IS_GST_PLUGIN:=1

_private_source_dir := .nvtegra/pre-gen-source_x86

SRCS:= ext/eglgles/gsteglglessink.c \
       ext/eglgles/gstegladaptation.c \
       ext/eglgles/gstegljitter.c \
       ext/eglgles/gstegladaptation_egl.c \
       ext/eglgles/video_platform_wrapper.c \
       gst-libs/gst/egl/egl.c

CFLAGS:= -DHAVE_X11 -DHAVE_CONFIG_H -DIS_DESKTOP

NEEDS_CUDA:=1

INC_PATHS:= $(_private_source_dir) \
            gst-libs \
            . \
            NV_ROOT_DIR/deepstream/sdk/external/3rdparty/include \
            NV_ROOT_DIR/nvutils/nvbufsurface/

LIBS:= -LTARGET_DIR/../../../libs/ -lnvbuf_fdmap \
       -lGLESv2 -lEGL -lX11 -lm -lcuda -lcudart

PACKAGE_BINARY_IN_DS:=1

include ../../../deepstream/sdk/Rules.mk
