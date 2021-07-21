/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
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

#include <arch_helpers.h>
#include <assert.h>
#include <bl31.h>
#include <bl_common.h>
#include <console.h>
#include <context.h>
#include <context_mgmt.h>
#include <cortex_a57.h>
#include <debug.h>
#include <denver.h>
#include <interrupt_mgmt.h>
#include <mce.h>
#include <mce_private.h>
#include <platform.h>
#include <tegra_def.h>
#include <tegra_mc_def.h>
#include <tegra_platform.h>
#include <tegra_private.h>
#include <xlat_tables_v2.h>

/* ID for spe-console */
#define TEGRA_CONSOLE_SPE_ID		0xFE

/*******************************************************************************
 * The Tegra power domain tree has a single system level power domain i.e. a
 * single root node. The first entry in the power domain descriptor specifies
 * the number of power domains at the highest power level.
 *******************************************************************************
 */
static const uint8_t tegra_power_domain_tree_desc[] = {
	/* No of root nodes */
	1,
	/* No of clusters */
	PLATFORM_CLUSTER_COUNT,
	/* No of CPU cores - cluster0 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster1 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster2 */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
	/* No of CPU cores - cluster3 */
	PLATFORM_MAX_CPUS_PER_CLUSTER
};

/*******************************************************************************
 * This function returns the Tegra default topology tree information.
 ******************************************************************************/
const uint8_t *plat_get_power_domain_tree_desc(void)
{
	return tegra_power_domain_tree_desc;
}

/*
 * Table of regions to map using the MMU.
 */
static const mmap_region_t tegra_mmap[] = {
	MAP_REGION_FLAT(TEGRA_MISC_BASE, 0x4000U, /* 16KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_TSA_BASE, 0x20000U, /* 128KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_GPCDMA_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_MC_STREAMID_BASE, 0x8000U, /* 32KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_MC_BASE, 0x8000U, /* 32KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
#if !ENABLE_CONSOLE_SPE
	MAP_REGION_FLAT(TEGRA_UARTA_BASE, 0x20000U, /* 128KB - UART A, B*/
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_UARTC_BASE, 0x20000U, /* 128KB - UART C, G */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_UARTD_BASE, 0x30000U, /* 192KB - UART D, E, F */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
#endif
	MAP_REGION_FLAT(TEGRA_FUSE_BASE, 0x1000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_XUSB_PADCTL_BASE, 0x2000U, /* 8KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_GICD_BASE, 0x1000, /* 4KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_GICC_BASE, 0x1000, /* 4KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_SE0_BASE, 0x1000U, /* 4KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_PKA1_BASE, 0x1000U, /* 4KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_RNG1_BASE, 0x1000U, /* 4KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_HSP_DBELL_BASE, 0x1000U, /* 4KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
#if ENABLE_CONSOLE_SPE
	MAP_REGION_FLAT(TEGRA_CONSOLE_SPE_BASE, 0x1000U, /* 4KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
#endif
	MAP_REGION_FLAT(TEGRA_TMRUS_BASE, TEGRA_TMRUS_SIZE, /* 4KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_SCRATCH_BASE, 0x1000U, /* 4KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_SMMU2_BASE, 0x800000U, /* 8MB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_SMMU1_BASE, 0x800000U, /* 8MB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_SMMU0_BASE, 0x800000U, /* 8MB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_BPMP_IPC_TX_PHYS_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_CAR_RESET_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	{0}
};

/*******************************************************************************
 * Set up the pagetables as per the platform memory map & initialize the MMU
 ******************************************************************************/
const mmap_region_t *plat_get_mmio_map(void)
{
	/* MMIO space */
	return tegra_mmap;
}

/*******************************************************************************
 * Handler to get the System Counter Frequency
 ******************************************************************************/
uint32_t plat_get_syscnt_freq2(void)
{
	return 31250000;
}

#if !ENABLE_CONSOLE_SPE
/*******************************************************************************
 * Maximum supported UART controllers
 ******************************************************************************/
#define TEGRA194_MAX_UART_PORTS		7

/*******************************************************************************
 * This variable holds the UART port base addresses
 ******************************************************************************/
static uint32_t tegra194_uart_addresses[TEGRA194_MAX_UART_PORTS + 1] = {
	0,	/* undefined - treated as an error case */
	TEGRA_UARTA_BASE,
	TEGRA_UARTB_BASE,
	TEGRA_UARTC_BASE,
	TEGRA_UARTD_BASE,
	TEGRA_UARTE_BASE,
	TEGRA_UARTF_BASE,
	TEGRA_UARTG_BASE,
};
#endif

/*******************************************************************************
 * Retrieve the UART controller base to be used as the console
 ******************************************************************************/
uint32_t plat_get_console_from_id(int32_t id)
{
	uint32_t ret;

#if ENABLE_CONSOLE_SPE
	if (id == TEGRA_CONSOLE_SPE_ID) {
		ret = TEGRA_CONSOLE_SPE_BASE;
	} else {
		ret = 0;
	}
#else
	if (id > TEGRA194_MAX_UART_PORTS) {
		ret = 0;
	} else {
		ret = tegra194_uart_addresses[id];
	}
#endif

	return ret;
}

/*******************************************************************************
 * Handler for early platform setup
 ******************************************************************************/
void plat_early_platform_setup(void)
{
	const plat_params_from_bl2_t *params_from_bl2 = bl31_get_plat_params();
	uint8_t enable_ccplex_lock_step = params_from_bl2->enable_ccplex_lock_step;
	uint64_t actlr_el3, actlr_el2;

	/* sanity check MCE firmware compatibility */
	mce_verify_firmware_version();

	/*
	 * Program XUSB STREAMIDs
	 * ======================
	 * T19x XUSB has support for XUSB virtualization. It will have one
	 * physical function (PF) and four Virtual function (VF)
	 *
	 * There were below two SIDs for XUSB until T186.
	 * 1) #define TEGRA_SID_XUSB_HOST    0x1bU
	 * 2) #define TEGRA_SID_XUSB_DEV    0x1cU
	 *
	 * We have below four new SIDs added for VF(s)
	 * 3) #define TEGRA_SID_XUSB_VF0    0x5dU
	 * 4) #define TEGRA_SID_XUSB_VF1    0x5eU
	 * 5) #define TEGRA_SID_XUSB_VF2    0x5fU
	 * 6) #define TEGRA_SID_XUSB_VF3    0x60U
	 *
	 * When virtualization is enabled then we have to disable SID override
	 * and program above SIDs in below newly added SID registers in XUSB
	 * PADCTL MMIO space. These registers are TZ protected and so need to
	 * be done in ATF.
	 * a) #define XUSB_PADCTL_HOST_AXI_STREAMID_PF_0 (0x136cU)
	 * b) #define XUSB_PADCTL_DEV_AXI_STREAMID_PF_0  (0x139cU)
	 * c) #define XUSB_PADCTL_HOST_AXI_STREAMID_VF_0 (0x1370U)
	 * d) #define XUSB_PADCTL_HOST_AXI_STREAMID_VF_1 (0x1374U)
	 * e) #define XUSB_PADCTL_HOST_AXI_STREAMID_VF_2 (0x1378U)
	 * f) #define XUSB_PADCTL_HOST_AXI_STREAMID_VF_3 (0x137cU)
	 *
	 * This change disables SID override and programs XUSB SIDs in
	 * above registers to support both virtualization and
	 * non-virtualization platforms
	 */
	if (tegra_platform_is_silicon() || tegra_platform_is_fpga()) {

		mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
			XUSB_PADCTL_HOST_AXI_STREAMID_PF_0, TEGRA_SID_XUSB_HOST);
		mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
			XUSB_PADCTL_HOST_AXI_STREAMID_VF_0, TEGRA_SID_XUSB_VF0);
		mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
			XUSB_PADCTL_HOST_AXI_STREAMID_VF_1, TEGRA_SID_XUSB_VF1);
		mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
			XUSB_PADCTL_HOST_AXI_STREAMID_VF_2, TEGRA_SID_XUSB_VF2);
		mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
			XUSB_PADCTL_HOST_AXI_STREAMID_VF_3, TEGRA_SID_XUSB_VF3);
		mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
			XUSB_PADCTL_DEV_AXI_STREAMID_PF_0, TEGRA_SID_XUSB_DEV);
	}

	/*
	 * Enable dual execution optimized translations for EL2 and EL3.
	 */
	if (enable_ccplex_lock_step != 0U) {
		actlr_el3 = read_actlr_el3();
		actlr_el3 |= DENVER_CPU_ENABLE_DUAL_EXEC_EL3;
		write_actlr_el3(actlr_el3);

		actlr_el2 = read_actlr_el2();
		actlr_el2 |= DENVER_CPU_ENABLE_DUAL_EXEC_EL2;
		write_actlr_el2(actlr_el2);
	}

	/*
	 * Enable Uncore Perfmon counters only when FUSE_SECURITY_MODE_0/ODM
	 * Production fuse is not set. This fuse is customer-controlled and
	 * will be set by OEM in their product's production.
	 */
	if (mmio_read_32(TEGRA_FUSE_BASE + SECURITY_MODE)
			== ODM_PROD_FUSE_DISABLED) {
		actlr_el3 = read_actlr_el3();
		actlr_el3 |= DENVER_CPU_ENABLE_MDCR_EL3_SPME;
		write_actlr_el3(actlr_el3);
	}
}

/* Secure IRQs for Tegra194 */
static const irq_sec_cfg_t tegra194_sec_irqs[] = {
	{
		TEGRA194_TOP_WDT_IRQ,
		TEGRA194_SEC_IRQ_TARGET_MASK,
		INTR_TYPE_EL3,
	},
	{
		TEGRA194_AON_WDT_IRQ,
		TEGRA194_SEC_IRQ_TARGET_MASK,
		INTR_TYPE_EL3,
	},
};

/*******************************************************************************
 * Initialize the GIC and SGIs
 ******************************************************************************/
void plat_gic_setup(void)
{
	tegra_gic_cfg_t tegra194_gic_cfg = {0};

	tegra194_gic_cfg.irq_cfg = tegra194_sec_irqs;
	tegra194_gic_cfg.g0_int_num = ARRAY_SIZE(tegra194_sec_irqs);
	tegra_gic_setup(&tegra194_gic_cfg);

	/*
	 * Initialize the FIQ handler only if the platform supports any
	 * FIQ interrupt sources.
	 */
	if (sizeof(tegra194_sec_irqs) > 0U) {
		tegra_fiq_handler_setup();
	}
}

/*******************************************************************************
 * Return pointer to the BL31 params from previous bootloader
 ******************************************************************************/
bl31_params_t *plat_get_bl31_params(void)
{
	uint32_t reg;
	uint64_t val;

	reg = mmio_read_32(TEGRA_SCRATCH_BASE + SCRATCH_BL31_PARAMS_HI_ADDR);
	val = ((uint64_t)reg & SCRATCH_BL31_PARAMS_HI_ADDR_MASK) >>
			 SCRATCH_BL31_PARAMS_HI_ADDR_SHIFT;
	val <<= 32;
	val |= (uint64_t)mmio_read_32(TEGRA_SCRATCH_BASE + SCRATCH_BL31_PARAMS_LO_ADDR);

	return (bl31_params_t *)(uintptr_t)val;
}

/*******************************************************************************
 * Return pointer to the BL31 platform params from previous bootloader
 ******************************************************************************/
plat_params_from_bl2_t *plat_get_bl31_plat_params(void)
{
	uint32_t reg;
	uint64_t val;

	reg = mmio_read_32(TEGRA_SCRATCH_BASE + SCRATCH_BL31_PLAT_PARAMS_HI_ADDR);
	val = ((uint64_t)reg & SCRATCH_BL31_PLAT_PARAMS_HI_ADDR_MASK) >>
			 SCRATCH_BL31_PLAT_PARAMS_HI_ADDR_SHIFT;
	val <<= 32;
	val |= (uint64_t)mmio_read_32(TEGRA_SCRATCH_BASE + SCRATCH_BL31_PLAT_PARAMS_LO_ADDR);

	return (plat_params_from_bl2_t *)(uintptr_t)val;
}

/*******************************************************************************
 * Handler for late platform setup
 ******************************************************************************/
void plat_late_platform_setup(void)
{
#if ENABLE_STRICT_CHECKING_MODE
	/*
	 * Enable strict checking after programming the GSC for
	 * enabling TZSRAM and TZDRAM
	 */
	mce_enable_strict_checking();
#endif
}

/*******************************************************************************
 * Handler to indicate support for System Suspend
 ******************************************************************************/
bool plat_supports_system_suspend(void)
{
	return true;
}
