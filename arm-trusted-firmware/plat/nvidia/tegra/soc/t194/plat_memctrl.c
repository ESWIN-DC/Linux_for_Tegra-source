/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <bl_common.h>
#include <mce.h>
#include <memctrl_v2.h>
#include <tegra_mc_def.h>
#include <tegra_platform.h>

/*******************************************************************************
 * Struct to hold the memory controller settings
 ******************************************************************************/
static tegra_mc_settings_t tegra194_mc_settings = {
};

/*******************************************************************************
 * Handler to return the pointer to the memory controller's settings struct
 ******************************************************************************/
tegra_mc_settings_t *tegra_get_mc_settings(void)
{
	return &tegra194_mc_settings;
}

/*******************************************************************************
 * Handler to program the scratch registers with TZDRAM settings for the
 * resume firmware
 ******************************************************************************/
void plat_memctrl_tzdram_setup(uint64_t phys_base, uint64_t size_in_bytes)
{
	uint32_t sec_reg_ctrl = tegra_mc_read_32(MC_SECURITY_CFG_REG_CTRL_0);

	/*
	 * Check TZDRAM carveout register access status. Setup TZDRAM fence
	 * only if access is enabled.
	 */
	if ((sec_reg_ctrl & SECURITY_CFG_WRITE_ACCESS_BIT) ==
	     SECURITY_CFG_WRITE_ACCESS_ENABLE) {

		/*
		 * Setup the Memory controller to allow only secure accesses to
		 * the TZDRAM carveout
		 */
		INFO("Configuring TrustZone DRAM Memory Carveout\n");

		tegra_mc_write_32(MC_SECURITY_CFG0_0, (uint32_t)phys_base);
		tegra_mc_write_32(MC_SECURITY_CFG3_0, (uint32_t)(phys_base >> 32));
		tegra_mc_write_32(MC_SECURITY_CFG1_0, (uint32_t)(size_in_bytes >> 20));

		/*
		 * MCE propagates the security configuration values across the
		 * CCPLEX.
		 */
		(void)mce_update_gsc_tzdram();
	}
}
