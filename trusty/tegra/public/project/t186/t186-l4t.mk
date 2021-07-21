#
# Copyright (c) 2018-2020, NVIDIA CORPORATION. All rights reserved
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

# This makefile should only be included by l4t build variants
ifeq ($(filter l4t%, $(TRUSTY_VARIANT)),)
$(error TRUSTY_VARIANT=$(TRUSTY_VARIANT) is not an l4t variant)
endif

TOP := $(TEGRA_TOP)

# L4T only builds some test applications
TRUSTY_ALL_USER_TASKS := \
	sample/ipc-unittest/main \
	sample/ipc-unittest/srv

# Disable TAs from nvidia-sample for OTE to avoid conflict
ifeq ($(filter l4t-partner-ote, $(TRUSTY_VARIANT)),)
TRUSTY_ALL_USER_TASKS += \
	nvidia-sample/hwkey-agent \
	nvidia-sample/luks-srv
endif
