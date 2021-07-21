/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <ep_info.h>
#include <interrupt_mgmt.h>
#include <mce.h>
#include <platform.h>
#include <tegra_def.h>
#include <tegra_platform.h>
#include <tegra_private.h>
#include <xlat_tables_v2.h>

/*******************************************************************************
 * Tegra186 CPU numbers in cluster #0
 *******************************************************************************
 */
#define TEGRA186_CLUSTER0_CORE2		2U
#define TEGRA186_CLUSTER0_CORE3		3U

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
	MAP_REGION_FLAT(TEGRA_MISC_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_TSA_BASE, 0x20000U, /* 128KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_MC_STREAMID_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_MC_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_UARTA_BASE, 0x20000U, /* 128KB - UART A, B*/
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_UARTC_BASE, 0x20000U, /* 128KB - UART C, G */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_UARTD_BASE, 0x30000U, /* 192KB - UART D, E, F */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_FUSE_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_GICD_BASE, 0x20000U, /* 128KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_SE0_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_PKA1_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_RNG1_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_CAR_RESET_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_PMC_BASE, 0x40000U, /* 256KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_TMRUS_BASE, 0x1000U, /* 4KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RO | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_SCRATCH_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_MMCRAB_BASE, 0x60000U, /* 384KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_ARM_ACTMON_CTR_BASE, 0x20000U, /* 128KB - ARM/Denver */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_SMMU0_BASE, 0x1000000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_HSP_DBELL_BASE, 0x10000U, /* 64KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_BPMP_IPC_TX_PHYS_BASE, TEGRA_BPMP_IPC_CH_MAP_SIZE, /* 4KB */
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE),
	MAP_REGION_FLAT(TEGRA_BPMP_IPC_RX_PHYS_BASE, TEGRA_BPMP_IPC_CH_MAP_SIZE, /* 4KB */
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

/*******************************************************************************
 * Maximum supported UART controllers
 ******************************************************************************/
#define TEGRA186_MAX_UART_PORTS		7

/*******************************************************************************
 * This variable holds the UART port base addresses
 ******************************************************************************/
static uint32_t tegra186_uart_addresses[TEGRA186_MAX_UART_PORTS + 1] = {
	0,	/* undefined - treated as an error case */
	TEGRA_UARTA_BASE,
	TEGRA_UARTB_BASE,
	TEGRA_UARTC_BASE,
	TEGRA_UARTD_BASE,
	TEGRA_UARTE_BASE,
	TEGRA_UARTF_BASE,
	TEGRA_UARTG_BASE,
};

/*******************************************************************************
 * Retrieve the UART controller base to be used as the console
 ******************************************************************************/
uint32_t plat_get_console_from_id(int32_t id)
{
	uint32_t ret;

	if (id > TEGRA186_MAX_UART_PORTS) {
		ret = 0;
	} else {
		ret = tegra186_uart_addresses[id];
	}

	return ret;
}

/*******************************************************************************
 * Handler for early platform setup
 ******************************************************************************/
void plat_early_platform_setup(void)
{
	uint64_t impl, val;
	const plat_params_from_bl2_t *plat_params = bl31_get_plat_params();

	/* sanity check MCE firmware compatibility */
	mce_verify_firmware_version();

	impl = (read_midr() >> MIDR_IMPL_SHIFT) & (uint64_t)MIDR_IMPL_MASK;

	/*
	 * Enable ECC and Parity Protection for Cortex-A57 CPUs (Tegra186
	 * A02p and beyond).
	 */
	if ((plat_params->l2_ecc_parity_prot_dis != 1) &&
	    (impl != (uint64_t)DENVER_IMPL)) {

		val = read_l2ctlr_el1();
		val |= CORTEX_A57_L2_ECC_PARITY_PROTECTION_BIT;
		write_l2ctlr_el1(val);
	}
}

/*******************************************************************************
 * Handler for late platform setup
 ******************************************************************************/
void plat_late_platform_setup(void)
{
	; /* do nothing */
}

/* Secure IRQs for Tegra186 */
static const irq_sec_cfg_t tegra186_sec_irqs[] = {
	{
		TEGRA186_TOP_WDT_IRQ,
		TEGRA186_SEC_IRQ_TARGET_MASK,
		INTR_TYPE_EL3,
	},
	{
		TEGRA186_AON_WDT_IRQ,
		TEGRA186_SEC_IRQ_TARGET_MASK,
		INTR_TYPE_EL3,
	},
};

/*******************************************************************************
 * Initialize the GIC and SGIs
 ******************************************************************************/
void plat_gic_setup(void)
{
	tegra_gic_cfg_t tegra186_gic_cfg = {0};

	tegra186_gic_cfg.irq_cfg = tegra186_sec_irqs;
	tegra186_gic_cfg.g0_int_num = ARRAY_SIZE(tegra186_sec_irqs);
	tegra_gic_setup(&tegra186_gic_cfg);

	/*
	 * Initialize the FIQ handler only if the platform supports any
	 * FIQ interrupt sources.
	 */
	if (sizeof(tegra186_sec_irqs) > 0U) {
		tegra_fiq_handler_setup();
	}
}

/*******************************************************************************
 * Return pointer to the BL31 params from previous bootloader
 ******************************************************************************/
bl31_params_t *plat_get_bl31_params(void)
{
	uint32_t val;

	val = mmio_read_32(TEGRA_SCRATCH_BASE + SCRATCH_BL31_PARAMS_ADDR);

	return (bl31_params_t *)(uintptr_t)val;
}

/*******************************************************************************
 * Return pointer to the BL31 platform params from previous bootloader
 ******************************************************************************/
plat_params_from_bl2_t *plat_get_bl31_plat_params(void)
{
	uint32_t val;

	val = mmio_read_32(TEGRA_SCRATCH_BASE + SCRATCH_BL31_PLAT_PARAMS_ADDR);

	return (plat_params_from_bl2_t *)(uintptr_t)val;
}

/*******************************************************************************
 * This function implements a part of the critical interface between the psci
 * generic layer and the platform that allows the former to query the platform
 * to convert an MPIDR to a unique linear index. An error code (-1) is returned
 * in case the MPIDR is invalid.
 ******************************************************************************/
int32_t plat_core_pos_by_mpidr(u_register_t mpidr)
{
	u_register_t cluster_id, cpu_id, pos;
	int32_t ret;

	cluster_id = (mpidr >> (u_register_t)MPIDR_AFF1_SHIFT) & (u_register_t)MPIDR_AFFLVL_MASK;
	cpu_id = (mpidr >> (u_register_t)MPIDR_AFF0_SHIFT) & (u_register_t)MPIDR_AFFLVL_MASK;

	/*
	 * Validate cluster_id by checking whether it represents
	 * one of the two clusters present on the platform.
	 * Validate cpu_id by checking whether it represents a CPU in
	 * one of the two clusters present on the platform.
	 */
	if ((cluster_id >= (u_register_t)PLATFORM_CLUSTER_COUNT) ||
	    (cpu_id >= (u_register_t)PLATFORM_MAX_CPUS_PER_CLUSTER)) {
		ret = PSCI_E_NOT_PRESENT;
	} else {
		/* calculate the core position */
		pos = cpu_id + (cluster_id << 2U);

		/* check for non-existent CPUs */
		if ((pos == TEGRA186_CLUSTER0_CORE2) || (pos == TEGRA186_CLUSTER0_CORE3)) {
			ret = PSCI_E_NOT_PRESENT;
		} else {
			ret = (int32_t)pos;
		}
	}

	return ret;
}

/*******************************************************************************
 * Handler to relocate BL32 image to TZDRAM
 ******************************************************************************/
void plat_relocate_bl32_image(const image_info_t *bl32_img_info)
{
	const plat_params_from_bl2_t *plat_bl31_params = plat_get_bl31_plat_params();
	const entry_point_info_t *bl32_ep_info = bl31_plat_get_next_image_ep_info(SECURE);
	uint64_t tzdram_start, tzdram_end, bl32_start, bl32_end;

	if ((bl32_img_info != NULL) && (bl32_ep_info != NULL)) {

		/* Relocate BL32 if it resides outside of the TZDRAM */
		tzdram_start = plat_bl31_params->tzdram_base;
		tzdram_end = plat_bl31_params->tzdram_base +
				plat_bl31_params->tzdram_size;
		bl32_start = bl32_img_info->image_base;
		bl32_end = bl32_img_info->image_base + bl32_img_info->image_size;

		assert(tzdram_end > tzdram_start);
		assert(bl32_end > bl32_start);
		assert(bl32_ep_info->pc > tzdram_start);
		assert(bl32_ep_info->pc < tzdram_end);

		/* relocate BL32 */
		if ((bl32_start >= tzdram_end) || (bl32_end <= tzdram_start)) {

			INFO("Relocate BL32 to TZDRAM\n");

			tegra_memcpy(bl32_ep_info->pc, bl32_start,
				    (uint64_t)bl32_img_info->image_size);

			/* clean up non-secure intermediate buffer */
			tegra_zeromem(bl32_start, (uint64_t)bl32_img_info->image_size);
		}
	}
}

/*******************************************************************************
 * Handler to indicate support for System Suspend
 ******************************************************************************/
bool plat_supports_system_suspend(void)
{
	return true;
}
