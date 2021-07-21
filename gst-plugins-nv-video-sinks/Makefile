###############################################################################
#
# Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#
###############################################################################

CC:= gcc
TARGET_NAME := libgstnvvideosinks.so

SRCS := common/context.c \
	common/display.c \
	common/egl/context_egl.c \
	common/renderer.c \
	common/renderer/renderer_gl.c \
	common/window.c \
	common/x11/display_x11.c \
	common/x11/window_x11.c \
	gstnvvideosinks.c \
	nv3dsink/gstnv3dsink.c

IS_GST_PLUGIN:=1

CFLAGS := -fPIC \
    -DNV_VIDEO_SINKS_HAS_EGL \
    -DNV_VIDEO_SINKS_HAS_GL \
    -DNV_VIDEO_SINKS_HAS_NV3DSINK \
    -DNV_VIDEO_SINKS_HAS_X11 \
    -DIS_DESKTOP

NEEDS_CUDA:=1

IGNORE_DS_PACKAGE_NAMING:=1

PACKAGE_BINARY_IN_DS:=1

LDFLAGS:= -shared

CAN_PUBLIC_LINK:=1

INC_PATHS =: \
	common \
	common/egl \
	common/renderer \
	common/x11 \
	/usr/local/include/gstreamer-1.0 \
	../../../3rdparty/gst/gst-nveglglessink/gst-libs/ \
	../../../nvutils/nvbufsurface/ \
	../

PKGS := glib-2.0 \
	gstreamer-1.0 \
	gstreamer-base-1.0 \
	gstreamer-video-1.0

CFLAGS += `pkg-config --cflags $(PKGS)`

LDFLAGS = -Wl,--no-undefined -L/usr/lib/aarch64-linux-gnu/tegra

LIBS = -lnvbufsurface -lGLESv2 -lEGL -lX11 -lm -lcuda -lcudart

LIBS += `pkg-config --libs $(PKGS)`

include ../../../deepstream/sdk/Rules.mk
