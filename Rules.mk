TOP_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
INSTALL_DIR := $(TOP_DIR)/dist

# Clear the flags from env
CPPFLAGS := -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
LDFLAGS :=

# Verbose flag
ifeq ($(VERBOSE), 1)
AT =
else
AT = @
endif

# ARM ABI of the target platform
ifeq ($(TEGRA_ARMABI),)
TEGRA_ARMABI ?= aarch64-linux-gnu
endif

# Location of the target rootfs
ifeq ($(shell uname -m), aarch64)
TARGET_ROOTFS :=
else
ifeq ($(TARGET_ROOTFS),)
$(error Please specify the target rootfs path if you are cross-compiling)
endif
endif

# Location of the CUDA Toolkit
CUDA_PATH 	:= /usr/local/cuda

# Use absolute path for better access from everywhere
CLASS_DIR 	:= $(TOP_DIR)/third_party/tegra_multimedia_api-R32.2.3/samples/common/classes
ALGO_CUDA_DIR 	:= $(TOP_DIR)/third_party/tegra_multimedia_api-R32.2.3/samples/common/algorithm/cuda
ALGO_TRT_DIR 	:= $(TOP_DIR)/third_party/tegra_multimedia_api-R32.2.3/samples/common/algorithm/trt

ifeq ($(shell uname -m), aarch64)
CROSS_COMPILE :=
else
CROSS_COMPILE ?= aarch64-linux-gnu-
endif
AS             = $(AT) $(CROSS_COMPILE)as
LD             = $(AT) $(CROSS_COMPILE)ld
CC             = $(AT) $(CROSS_COMPILE)gcc
CPP            = $(AT) $(CROSS_COMPILE)g++
AR             = $(AT) $(CROSS_COMPILE)ar
NM             = $(AT) $(CROSS_COMPILE)nm
STRIP          = $(AT) $(CROSS_COMPILE)strip
OBJCOPY        = $(AT) $(CROSS_COMPILE)objcopy
OBJDUMP        = $(AT) $(CROSS_COMPILE)objdump
NVCC           = $(AT) $(CUDA_PATH)/bin/nvcc -ccbin $(filter-out $(AT), $(CPP))

# Specify the logical root directory for headers and libraries.
ifneq ($(TARGET_ROOTFS),)
CPPFLAGS += --sysroot=$(TARGET_ROOTFS)
LDFLAGS += \
	-Wl,-rpath-link=$(TARGET_ROOTFS)/lib/$(TEGRA_ARMABI) \
	-Wl,-rpath-link=$(TARGET_ROOTFS)/usr/lib/$(TEGRA_ARMABI) \
	-Wl,-rpath-link=$(TARGET_ROOTFS)/usr/lib/$(TEGRA_ARMABI)/tegra \
	-Wl,-rpath-link=$(TARGET_ROOTFS)/$(CUDA_PATH)/lib64
endif

# All common header files
CPPFLAGS += \
	-fPIC \
	-I"$(TOP_DIR)/third_party/spdlog/include" \
	-I"$(TOP_DIR)/third_party/json/include" \
	-I"$(TOP_DIR)/third_party/tegra_multimedia_api-R32.2.3/include" \
	-I"$(TOP_DIR)/third_party/tegra_multimedia_api-R32.2.3/include/libjpeg-8b" \
	-I"$(ALGO_CUDA_DIR)" \
	-I"$(ALGO_TRT_DIR)" \
	-I"$(TARGET_ROOTFS)/$(CUDA_PATH)/include" \
	-I"$(TARGET_ROOTFS)/usr/include/$(TEGRA_ARMABI)" \
	-I"$(TARGET_ROOTFS)/usr/include/libdrm" \
	-I"$(TARGET_ROOTFS)/usr/include/opencv4"

# All common dependent libraries
LDFLAGS += \
	-lpthread -lv4l2 -lEGL -lGLESv2 -lX11 \
	-lnvbuf_utils -lnvjpeg -lnvosd -ldrm \
	-lcuda -lcudart \
	-lnvinfer -lnvparsers \
	-L"$(TARGET_ROOTFS)/$(CUDA_PATH)/lib64" \
	-L"$(TARGET_ROOTFS)/usr/lib/$(TEGRA_ARMABI)" \
	-L"$(TARGET_ROOTFS)/usr/lib/$(TEGRA_ARMABI)/tegra" \
	-L"$(TARGET_ROOTFS)/usr/lib/$(TEGRA_ARMABI)/tegra-egl" \
	-L/usr/local/cuda-10.2/targets/aarch64-linux/lib/ \
