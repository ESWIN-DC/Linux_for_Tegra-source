/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION. All rights reserved.
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

#include <bl_common.h>
#include <debug.h>
#include <smmu.h>
#include <tegra_def.h>
#include <tegra_mc_def.h>

#define BOARD_SYSTEM_FPGA_BASE		U(1)
#define BASE_CONFIG_SMMU_DEVICES	U(2)
#define MAX_NUM_SMMU_DEVICES		U(3)

static uint32_t tegra_misc_read_32(uint32_t off)
{
	return mmio_read_32((uint64_t)TEGRA_MISC_BASE + off);
}

/*******************************************************************************
 * Array to hold SMMU context for Tegra194
 ******************************************************************************/
static __attribute__((aligned(16))) smmu_regs_t tegra194_smmu_context[] = {
	_START_OF_TABLE_,
	mc_make_sid_security_cfg(HDAR),
	mc_make_sid_security_cfg(HOST1XDMAR),
	mc_make_sid_security_cfg(NVENCSRD),
	mc_make_sid_security_cfg(SATAR),
	mc_make_sid_security_cfg(NVENCSWR),
	mc_make_sid_security_cfg(HDAW),
	mc_make_sid_security_cfg(SATAW),
	mc_make_sid_security_cfg(ISPRA),
	mc_make_sid_security_cfg(ISPFALR),
	mc_make_sid_security_cfg(ISPWA),
	mc_make_sid_security_cfg(ISPWB),
	mc_make_sid_security_cfg(XUSB_HOSTR),
	mc_make_sid_security_cfg(XUSB_HOSTW),
	mc_make_sid_security_cfg(XUSB_DEVR),
	mc_make_sid_security_cfg(XUSB_DEVW),
	mc_make_sid_security_cfg(TSECSRD),
	mc_make_sid_security_cfg(TSECSWR),
	mc_make_sid_security_cfg(SDMMCRA),
	mc_make_sid_security_cfg(SDMMCR),
	mc_make_sid_security_cfg(SDMMCRAB),
	mc_make_sid_security_cfg(SDMMCWA),
	mc_make_sid_security_cfg(SDMMCW),
	mc_make_sid_security_cfg(SDMMCWAB),
	mc_make_sid_security_cfg(VICSRD),
	mc_make_sid_security_cfg(VICSWR),
	mc_make_sid_security_cfg(VIW),
	mc_make_sid_security_cfg(NVDECSRD),
	mc_make_sid_security_cfg(NVDECSWR),
	mc_make_sid_security_cfg(APER),
	mc_make_sid_security_cfg(APEW),
	mc_make_sid_security_cfg(NVJPGSRD),
	mc_make_sid_security_cfg(NVJPGSWR),
	mc_make_sid_security_cfg(SESRD),
	mc_make_sid_security_cfg(SESWR),
	mc_make_sid_security_cfg(AXIAPR),
	mc_make_sid_security_cfg(AXIAPW),
	mc_make_sid_security_cfg(ETRR),
	mc_make_sid_security_cfg(ETRW),
	mc_make_sid_security_cfg(TSECSRDB),
	mc_make_sid_security_cfg(TSECSWRB),
	mc_make_sid_security_cfg(AXISR),
	mc_make_sid_security_cfg(AXISW),
	mc_make_sid_security_cfg(EQOSR),
	mc_make_sid_security_cfg(EQOSW),
	mc_make_sid_security_cfg(UFSHCR),
	mc_make_sid_security_cfg(UFSHCW),
	mc_make_sid_security_cfg(NVDISPLAYR),
	mc_make_sid_security_cfg(BPMPR),
	mc_make_sid_security_cfg(BPMPW),
	mc_make_sid_security_cfg(BPMPDMAR),
	mc_make_sid_security_cfg(BPMPDMAW),
	mc_make_sid_security_cfg(AONR),
	mc_make_sid_security_cfg(AONW),
	mc_make_sid_security_cfg(AONDMAR),
	mc_make_sid_security_cfg(AONDMAW),
	mc_make_sid_security_cfg(SCER),
	mc_make_sid_security_cfg(SCEW),
	mc_make_sid_security_cfg(SCEDMAR),
	mc_make_sid_security_cfg(SCEDMAW),
	mc_make_sid_security_cfg(APEDMAR),
	mc_make_sid_security_cfg(APEDMAW),
	mc_make_sid_security_cfg(NVDISPLAYR1),
	mc_make_sid_security_cfg(VICSRD1),
	mc_make_sid_security_cfg(NVDECSRD1),
	mc_make_sid_security_cfg(VIFALR),
	mc_make_sid_security_cfg(VIFALW),
	mc_make_sid_security_cfg(DLA0RDA),
	mc_make_sid_security_cfg(DLA0FALRDB),
	mc_make_sid_security_cfg(DLA0WRA),
	mc_make_sid_security_cfg(DLA0FALWRB),
	mc_make_sid_security_cfg(DLA1RDA),
	mc_make_sid_security_cfg(DLA1FALRDB),
	mc_make_sid_security_cfg(DLA1WRA),
	mc_make_sid_security_cfg(DLA1FALWRB),
	mc_make_sid_security_cfg(PVA0RDA),
	mc_make_sid_security_cfg(PVA0RDB),
	mc_make_sid_security_cfg(PVA0RDC),
	mc_make_sid_security_cfg(PVA0WRA),
	mc_make_sid_security_cfg(PVA0WRB),
	mc_make_sid_security_cfg(PVA0WRC),
	mc_make_sid_security_cfg(PVA1RDA),
	mc_make_sid_security_cfg(PVA1RDB),
	mc_make_sid_security_cfg(PVA1RDC),
	mc_make_sid_security_cfg(PVA1WRA),
	mc_make_sid_security_cfg(PVA1WRB),
	mc_make_sid_security_cfg(PVA1WRC),
	mc_make_sid_security_cfg(RCER),
	mc_make_sid_security_cfg(RCEW),
	mc_make_sid_security_cfg(RCEDMAR),
	mc_make_sid_security_cfg(RCEDMAW),
	mc_make_sid_security_cfg(NVENC1SRD),
	mc_make_sid_security_cfg(NVENC1SWR),
	mc_make_sid_security_cfg(PCIE0R),
	mc_make_sid_security_cfg(PCIE0W),
	mc_make_sid_security_cfg(PCIE1R),
	mc_make_sid_security_cfg(PCIE1W),
	mc_make_sid_security_cfg(PCIE2AR),
	mc_make_sid_security_cfg(PCIE2AW),
	mc_make_sid_security_cfg(PCIE3R),
	mc_make_sid_security_cfg(PCIE3W),
	mc_make_sid_security_cfg(PCIE4R),
	mc_make_sid_security_cfg(PCIE4W),
	mc_make_sid_security_cfg(PCIE5R),
	mc_make_sid_security_cfg(PCIE5W),
	mc_make_sid_security_cfg(ISPFALW),
	mc_make_sid_security_cfg(DLA0RDA1),
	mc_make_sid_security_cfg(DLA1RDA1),
	mc_make_sid_security_cfg(PVA0RDA1),
	mc_make_sid_security_cfg(PVA0RDB1),
	mc_make_sid_security_cfg(PVA1RDA1),
	mc_make_sid_security_cfg(PVA1RDB1),
	mc_make_sid_security_cfg(PCIE5R1),
	mc_make_sid_security_cfg(NVENCSRD1),
	mc_make_sid_security_cfg(NVENC1SRD1),
	mc_make_sid_security_cfg(ISPRA1),
	mc_make_sid_security_cfg(PCIE0R1),
	mc_make_sid_security_cfg(MIU0R),
	mc_make_sid_security_cfg(MIU0W),
	mc_make_sid_security_cfg(MIU1R),
	mc_make_sid_security_cfg(MIU1W),
	mc_make_sid_security_cfg(MIU2R),
	mc_make_sid_security_cfg(MIU2W),
	mc_make_sid_security_cfg(MIU3R),
	mc_make_sid_security_cfg(MIU3W),
	mc_make_sid_override_cfg(HDAR),
	mc_make_sid_override_cfg(HOST1XDMAR),
	mc_make_sid_override_cfg(NVENCSRD),
	mc_make_sid_override_cfg(SATAR),
	mc_make_sid_override_cfg(NVENCSWR),
	mc_make_sid_override_cfg(HDAW),
	mc_make_sid_override_cfg(SATAW),
	mc_make_sid_override_cfg(ISPRA),
	mc_make_sid_override_cfg(ISPFALR),
	mc_make_sid_override_cfg(ISPWA),
	mc_make_sid_override_cfg(ISPWB),
	mc_make_sid_override_cfg(XUSB_HOSTR),
	mc_make_sid_override_cfg(XUSB_HOSTW),
	mc_make_sid_override_cfg(XUSB_DEVR),
	mc_make_sid_override_cfg(XUSB_DEVW),
	mc_make_sid_override_cfg(TSECSRD),
	mc_make_sid_override_cfg(TSECSWR),
	mc_make_sid_override_cfg(SDMMCRA),
	mc_make_sid_override_cfg(SDMMCR),
	mc_make_sid_override_cfg(SDMMCRAB),
	mc_make_sid_override_cfg(SDMMCWA),
	mc_make_sid_override_cfg(SDMMCW),
	mc_make_sid_override_cfg(SDMMCWAB),
	mc_make_sid_override_cfg(VICSRD),
	mc_make_sid_override_cfg(VICSWR),
	mc_make_sid_override_cfg(VIW),
	mc_make_sid_override_cfg(NVDECSRD),
	mc_make_sid_override_cfg(NVDECSWR),
	mc_make_sid_override_cfg(APER),
	mc_make_sid_override_cfg(APEW),
	mc_make_sid_override_cfg(NVJPGSRD),
	mc_make_sid_override_cfg(NVJPGSWR),
	mc_make_sid_override_cfg(SESRD),
	mc_make_sid_override_cfg(SESWR),
	mc_make_sid_override_cfg(AXIAPR),
	mc_make_sid_override_cfg(AXIAPW),
	mc_make_sid_override_cfg(ETRR),
	mc_make_sid_override_cfg(ETRW),
	mc_make_sid_override_cfg(TSECSRDB),
	mc_make_sid_override_cfg(TSECSWRB),
	mc_make_sid_override_cfg(AXISR),
	mc_make_sid_override_cfg(AXISW),
	mc_make_sid_override_cfg(EQOSR),
	mc_make_sid_override_cfg(EQOSW),
	mc_make_sid_override_cfg(UFSHCR),
	mc_make_sid_override_cfg(UFSHCW),
	mc_make_sid_override_cfg(NVDISPLAYR),
	mc_make_sid_override_cfg(BPMPR),
	mc_make_sid_override_cfg(BPMPW),
	mc_make_sid_override_cfg(BPMPDMAR),
	mc_make_sid_override_cfg(BPMPDMAW),
	mc_make_sid_override_cfg(AONR),
	mc_make_sid_override_cfg(AONW),
	mc_make_sid_override_cfg(AONDMAR),
	mc_make_sid_override_cfg(AONDMAW),
	mc_make_sid_override_cfg(SCER),
	mc_make_sid_override_cfg(SCEW),
	mc_make_sid_override_cfg(SCEDMAR),
	mc_make_sid_override_cfg(SCEDMAW),
	mc_make_sid_override_cfg(APEDMAR),
	mc_make_sid_override_cfg(APEDMAW),
	mc_make_sid_override_cfg(NVDISPLAYR1),
	mc_make_sid_override_cfg(VICSRD1),
	mc_make_sid_override_cfg(NVDECSRD1),
	mc_make_sid_override_cfg(VIFALR),
	mc_make_sid_override_cfg(VIFALW),
	mc_make_sid_override_cfg(DLA0RDA),
	mc_make_sid_override_cfg(DLA0FALRDB),
	mc_make_sid_override_cfg(DLA0WRA),
	mc_make_sid_override_cfg(DLA0FALWRB),
	mc_make_sid_override_cfg(DLA1RDA),
	mc_make_sid_override_cfg(DLA1FALRDB),
	mc_make_sid_override_cfg(DLA1WRA),
	mc_make_sid_override_cfg(DLA1FALWRB),
	mc_make_sid_override_cfg(PVA0RDA),
	mc_make_sid_override_cfg(PVA0RDB),
	mc_make_sid_override_cfg(PVA0RDC),
	mc_make_sid_override_cfg(PVA0WRA),
	mc_make_sid_override_cfg(PVA0WRB),
	mc_make_sid_override_cfg(PVA0WRC),
	mc_make_sid_override_cfg(PVA1RDA),
	mc_make_sid_override_cfg(PVA1RDB),
	mc_make_sid_override_cfg(PVA1RDC),
	mc_make_sid_override_cfg(PVA1WRA),
	mc_make_sid_override_cfg(PVA1WRB),
	mc_make_sid_override_cfg(PVA1WRC),
	mc_make_sid_override_cfg(RCER),
	mc_make_sid_override_cfg(RCEW),
	mc_make_sid_override_cfg(RCEDMAR),
	mc_make_sid_override_cfg(RCEDMAW),
	mc_make_sid_override_cfg(NVENC1SRD),
	mc_make_sid_override_cfg(NVENC1SWR),
	mc_make_sid_override_cfg(PCIE0R),
	mc_make_sid_override_cfg(PCIE0W),
	mc_make_sid_override_cfg(PCIE1R),
	mc_make_sid_override_cfg(PCIE1W),
	mc_make_sid_override_cfg(PCIE2AR),
	mc_make_sid_override_cfg(PCIE2AW),
	mc_make_sid_override_cfg(PCIE3R),
	mc_make_sid_override_cfg(PCIE3W),
	mc_make_sid_override_cfg(PCIE4R),
	mc_make_sid_override_cfg(PCIE4W),
	mc_make_sid_override_cfg(PCIE5R),
	mc_make_sid_override_cfg(PCIE5W),
	mc_make_sid_override_cfg(ISPFALW),
	mc_make_sid_override_cfg(DLA0RDA1),
	mc_make_sid_override_cfg(DLA1RDA1),
	mc_make_sid_override_cfg(PVA0RDA1),
	mc_make_sid_override_cfg(PVA0RDB1),
	mc_make_sid_override_cfg(PVA1RDA1),
	mc_make_sid_override_cfg(PVA1RDB1),
	mc_make_sid_override_cfg(PCIE5R1),
	mc_make_sid_override_cfg(NVENCSRD1),
	mc_make_sid_override_cfg(NVENC1SRD1),
	mc_make_sid_override_cfg(ISPRA1),
	mc_make_sid_override_cfg(PCIE0R1),
	mc_make_sid_override_cfg(MIU0R),
	mc_make_sid_override_cfg(MIU0W),
	mc_make_sid_override_cfg(MIU1R),
	mc_make_sid_override_cfg(MIU1W),
	mc_make_sid_override_cfg(MIU2R),
	mc_make_sid_override_cfg(MIU2W),
	mc_make_sid_override_cfg(MIU3R),
	mc_make_sid_override_cfg(MIU3W),
	smmu_make_cfg(TEGRA_SMMU0_BASE),
	smmu_make_cfg(TEGRA_SMMU2_BASE),
	smmu_bypass_cfg,	/* TBU settings */
	_END_OF_TABLE_,
};

/*******************************************************************************
 * Handler to return the pointer to the SMMU's context struct
 ******************************************************************************/
smmu_regs_t *plat_get_smmu_ctx(void)
{
	/* index of _END_OF_TABLE_ */
	tegra194_smmu_context[0].val = (uint32_t)ARRAY_SIZE(tegra194_smmu_context) - 1U;

	return tegra194_smmu_context;
}

/*******************************************************************************
 * Handler to return the support SMMU devices number
 ******************************************************************************/
uint32_t plat_get_num_smmu_devices(void)
{
	uint32_t ret_num = MAX_NUM_SMMU_DEVICES;
	uint32_t board_revid = ((tegra_misc_read_32(MISCREG_EMU_REVID) >> \
							BOARD_SHIFT_BITS) & BOARD_MASK_BITS);

	if (board_revid == BOARD_SYSTEM_FPGA_BASE) {
		ret_num = BASE_CONFIG_SMMU_DEVICES;
	}

	return ret_num;
}
