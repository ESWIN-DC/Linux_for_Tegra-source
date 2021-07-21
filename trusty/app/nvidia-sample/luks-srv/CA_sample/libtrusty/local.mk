#
# Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

TRUSTY_SRC_DIR = libtrusty
TRUSTY_SRC_OUTDIR = $(OUT_DIR)/$(TRUSTY_SRC_DIR)
TRUSTY_SRC = $(TRUSTY_SRC_DIR)/trusty.c
TRUSTY_OBJ = $(patsubst %.c,%.o,$(TRUSTY_SRC))
TRUSTY_OUT_OBJ = $(patsubst $(TRUSTY_SRC_DIR)%,$(TRUSTY_SRC_OUTDIR)%,$(TRUSTY_OBJ))

TRUSTY_FLAGS = $(C_FLAGS) \
	-fPIC

libtrusty: $(TRUSTY_OUT_OBJ)
	$(AR) rcs $(TRUSTY_SRC_OUTDIR)/$@.a $^

$(TRUSTY_OUT_OBJ):
	mkdir -p $(dir $@)
	$(CC) $(TRUSTY_FLAGS) -o $@ -c $(patsubst $(TRUSTY_SRC_OUTDIR)%,$(TRUSTY_SRC_DIR)%,$(@:%o=%c))
