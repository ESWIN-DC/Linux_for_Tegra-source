#
# Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# override with stdlib files used by the Tegra platform
STDLIB_SRCS	:=	$(addprefix lib/stdlib/,	\
			assert.c			\
			mem.c				\
			putchar.c			\
			strlen.c			\
			strnlen.c)

SOC_DIR			:=	plat/nvidia/tegra/soc/${TARGET_SOC}

# dump the state on crash console
CRASH_REPORTING		:=	1
$(eval $(call add_define,CRASH_REPORTING))

# enable assert() for release/debug builds
ENABLE_ASSERTIONS	:=	1
PLAT_LOG_LEVEL_ASSERT	:=	50
$(eval $(call add_define,PLAT_LOG_LEVEL_ASSERT))

# Disable the PSCI platform compatibility layer
ENABLE_PLAT_COMPAT	:=	0

# enable dynamic memory mapping
PLAT_XLAT_TABLES_DYNAMIC :=	1
$(eval $(call add_define,PLAT_XLAT_TABLES_DYNAMIC))

# Enable PSCI v1.0 extended state ID format
PSCI_EXTENDED_STATE_ID	:=	1

# code and read-only data should be put on separate memory pages
SEPARATE_CODE_AND_RODATA :=	1

# do not use coherent memory
USE_COHERENT_MEM	:=	0

# enable D-cache early during CPU warmboot
WARMBOOT_ENABLE_DCACHE_EARLY := 1

# Flag to enable WDT FIQ interrupt handling for Tegra SoCs
# prior to Tegra186
ENABLE_WDT_LEGACY_FIQ_HANDLING	?= 0

# Flag to allow relocation of BL32 image to TZDRAM during boot
RELOCATE_BL32_IMAGE		?= 0

include plat/nvidia/tegra/common/tegra_common.mk
include ${SOC_DIR}/platform_${TARGET_SOC}.mk

$(eval $(call add_define,TZDRAM_BASE))
$(eval $(call add_define,ENABLE_WDT_LEGACY_FIQ_HANDLING))
$(eval $(call add_define,RELOCATE_BL32_IMAGE))

# remove unused files from the common folders
BL_EXCLUDE_SRCS		:=	common/tf_snprintf.c	\
				plat/common/${ARCH}/plat_common.c
BL_COMMON_SOURCES	:=	$(filter-out $(BL_EXCLUDE_SRCS), $(BL_COMMON_SOURCES))
BL31_SOURCES		:=	$(filter-out $(BL_EXCLUDE_SRCS), $(BL31_SOURCES))

# modify BUILD_PLAT to point to SoC specific build directory
BUILD_PLAT	:=	${BUILD_BASE}/${PLAT}/${TARGET_SOC}/${BUILD_TYPE}

# platform cflags (enable signed comparisons, disable stdlib)
CFLAGS		+= -Wsign-compare -nostdlib
