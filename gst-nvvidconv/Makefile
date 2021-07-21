# Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

SO_NAME := libgstnvvidconv.so

CC := gcc

GST_INSTALL_DIR?=/usr/lib/aarch64-linux-gnu/gstreamer-1.0/
LIB_INSTALL_DIR?=/usr/lib/aarch64-linux-gnu/tegra/
CFLAGS:=
LIBS:= -lnvbuf_utils

SRCS := $(wildcard *.c)

INCLUDES += -I./

PKGS := gstreamer-1.0 \
	gstreamer-base-1.0 \
	gstreamer-video-1.0 \
	gstreamer-allocators-1.0 \
	glib-2.0

OBJS := $(SRCS:.c=.o)

CFLAGS += -fPIC

CFLAGS += `pkg-config --cflags $(PKGS)`

LDFLAGS = -Wl,--no-undefined -L$(LIB_INSTALL_DIR) -Wl,-rpath,$(LIB_INSTALL_DIR)

LIBS += `pkg-config --libs $(PKGS)`

all: $(SO_NAME)

%.o: %.c
	$(CC) -c $< $(CFLAGS) $(INCLUDES) -o $@

$(SO_NAME): $(OBJS)
	$(CC) -shared -o $(SO_NAME) $(OBJS) $(LIBS) $(LDFLAGS)

.PHONY: install
DEST_DIR?= $(GST_INSTALL_DIR)
install: $(SO_NAME)
	cp -vp $(SO_NAME) $(DEST_DIR)

.PHONY: clean
clean:
	rm -rf $(OBJS) $(SO_NAME)
