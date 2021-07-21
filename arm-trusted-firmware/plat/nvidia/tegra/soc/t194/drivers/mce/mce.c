/*
 * Copyright (c) 2015-2017, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <mce.h>
#include <mce_private.h>
#include <platform_def.h>
#include <stdbool.h>
#include <sys/errno.h>
#include <t194_nvg.h>
#include <tegra_def.h>
#include <tegra_platform.h>
#include <tegra_private.h>

/* Handler to check if MCE firmware is supported */
static bool mce_firmware_not_supported(void)
{
	bool status;

	/* these platforms do not load MCE firmware */
	status = tegra_platform_is_linsim() || tegra_platform_is_qt() ||
		 tegra_platform_is_virt_dev_kit();

	return status;
}

/*******************************************************************************
 * Common handler for all MCE commands
 ******************************************************************************/
int32_t mce_command_handler(uint64_t cmd, uint64_t arg0, uint64_t arg1,
			uint64_t arg2)
{
	int32_t ret = 0;

	switch (cmd) {
	case (uint64_t)MCE_CMD_ENTER_CSTATE:
		ret = nvg_enter_cstate((uint32_t)arg0, (uint32_t)arg1);
		if (ret < 0) {
			ERROR("%s: enter_cstate failed(%d)\n", __func__, ret);
		}

		break;

	case (uint64_t)MCE_CMD_IS_SC7_ALLOWED:
		ret = nvg_is_sc7_allowed();
		if (ret < 0) {
			ERROR("%s: is_sc7_allowed failed(%d)\n", __func__, ret);
		}

		break;

	case (uint64_t)MCE_CMD_ONLINE_CORE:
		ret = nvg_online_core((uint32_t)arg0);
		if (ret < 0) {
			ERROR("%s: online_core failed(%d)\n", __func__, ret);
		}

		break;

	default:
		ERROR("unknown MCE command (%lu)\n", cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/*******************************************************************************
 * Handler to update carveout values for Video Memory Carveout region
 ******************************************************************************/
int32_t mce_update_gsc_videomem(void)
{
	int32_t ret = 0;

	/*
	 * MCE firmware is not running on simulation platforms.
	 */
	if (mce_firmware_not_supported()) {
		ret = -EINVAL;
	} else {
		ret = nvg_update_ccplex_gsc((uint32_t)TEGRA_NVG_CHANNEL_UPDATE_GSC_VPR);
	}

	return ret;
}

/*******************************************************************************
 * Handler to update carveout values for TZDRAM aperture
 ******************************************************************************/
int32_t mce_update_gsc_tzdram(void)
{
	int32_t ret = 0;

	/*
	 * MCE firmware is not running on simulation platforms.
	 */
	if (mce_firmware_not_supported()) {
		ret = -EINVAL;
	} else {
		ret = nvg_update_ccplex_gsc((uint32_t)TEGRA_NVG_CHANNEL_UPDATE_GSC_TZ_DRAM);
	}

	return ret;
}

/*******************************************************************************
 * Handler to update carveout values for TZ SysRAM aperture
 ******************************************************************************/
int32_t mce_update_gsc_tzram(void)
{
	int32_t ret = 0;

	/*
	 * MCE firmware is not running on simulation platforms.
	 */
	if (mce_firmware_not_supported()) {
		ret = -EINVAL;
	} else {
		ret = nvg_update_ccplex_gsc((uint32_t)TEGRA_NVG_CHANNEL_UPDATE_GSC_TZRAM);
	}

	return ret;
}

/*******************************************************************************
 * Handler to issue the UPDATE_CSTATE_INFO request
 ******************************************************************************/
void mce_update_cstate_info(const mce_cstate_info_t *cstate)
{
	/* issue the UPDATE_CSTATE_INFO request */
	nvg_update_cstate_info(cstate->cluster, cstate->ccplex, cstate->system,
		cstate->wake_mask, cstate->update_wake_mask);
}

/*******************************************************************************
 * Handler to read the MCE firmware version and check if it is compatible
 * with interface header the BL3-1 was compiled against
 ******************************************************************************/
void mce_verify_firmware_version(void)
{
	uint64_t version;
	uint32_t major, minor;

	/*
	 * MCE firmware is not running on simulation platforms.
	 */
	if (mce_firmware_not_supported()) {
		return;
	}

	/*
	 * Read the MCE firmware version and extract the major and minor
	 * version fields
	 */
	version = nvg_get_version();
	minor = (uint32_t)version;
	major = (uint32_t)(version >> 32);

	INFO("MCE Version - HW=%d:%d, SW=%d:%d\n", major, minor,
		TEGRA_NVG_VERSION_MAJOR, TEGRA_NVG_VERSION_MINOR);

	/*
	 * Verify that the MCE firmware version and the interface header
	 * match
	 */
	if (major != (uint32_t)TEGRA_NVG_VERSION_MAJOR) {
		ERROR("MCE major version mismatch\n");
		panic();
	}

	if (minor < (uint32_t)TEGRA_NVG_VERSION_MINOR) {
		ERROR("MCE minor version mismatch\n");
		panic();
	}
}

#if ENABLE_STRICT_CHECKING_MODE
/*******************************************************************************
 * Handler to enable the strict checking mode
 ******************************************************************************/
void mce_enable_strict_checking(void)
{
	uint64_t sctlr = read_sctlr_el3();
	int32_t ret = 0;

	if (tegra_platform_is_silicon() || tegra_platform_is_fpga()) {
		/*
		 * Step1: TZ-DRAM and TZRAM should be setup before the MMU is
		 * enabled.
		 *
		 * The common code makes sure that TZDRAM/TZRAM are already
		 * enabled before calling into this handler. If this is not the
		 * case, the following sequence must be executed before moving
		 * on to step 2.
		 *
		 * tlbialle1is();
		 * tlbialle3is();
		 * dsbsy();
		 *
		 */
		if ((sctlr & (uint64_t)SCTLR_M_BIT) == (uint64_t)SCTLR_M_BIT) {
			tlbialle1is();
			tlbialle3is();
			dsbsy();
		}

		/*
		 * Step2: SCF flush - Clean and invalidate caches and clear the
		 * TR-bits
		 */
		ret = nvg_roc_clean_cache_trbits();
		if (ret < 0) {
			ERROR("%s: flush cache_trbits failed(%d)\n", __func__,
				ret);
			return;
		}

		/*
		 * Step3: Issue the SECURITY_CONFIG request to MCE to enable
		 * strict checking mode.
		 */
		nvg_enable_strict_checking_mode();
	}
}
#endif

/*******************************************************************************
 * Handler to power down the entire system
 ******************************************************************************/
void mce_system_shutdown(void)
{
	nvg_system_shutdown();
}

/*******************************************************************************
 * Handler to reboot the entire system
 ******************************************************************************/
void mce_system_reboot(void)
{
	nvg_system_reboot();
}
