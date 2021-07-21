/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <bl_common.h>
#include <platform.h>
#include <tegra_def.h>
#include <tegra_private.h>
#include <xlat_tables_v2.h>

/* sets of MMIO ranges setup */
#define MMIO_RANGE_0_ADDR	0x50000000
#define MMIO_RANGE_1_ADDR	0x60000000
#define MMIO_RANGE_2_ADDR	0x70000000
#define MMIO_RANGE_SIZE		0x200000

/*
 * Table of regions to map using the MMU.
 */
static const mmap_region_t tegra_mmap[] = {
	MAP_REGION_FLAT(MMIO_RANGE_0_ADDR, MMIO_RANGE_SIZE,
			MT_DEVICE | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(MMIO_RANGE_1_ADDR, MMIO_RANGE_SIZE,
			MT_DEVICE | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(MMIO_RANGE_2_ADDR, MMIO_RANGE_SIZE,
			MT_DEVICE | MT_RW | MT_SECURE),
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
 * The Tegra power domain tree has a single system level power domain i.e. a
 * single root node. The first entry in the power domain descriptor specifies
 * the number of power domains at the highest power level.
 *******************************************************************************
 */
const unsigned char tegra_power_domain_tree_desc[] = {
	/* No of root nodes */
	1,
	/* No of clusters */
	PLATFORM_CLUSTER_COUNT,
	/* No of CPU cores */
	PLATFORM_CORE_COUNT,
};

/*******************************************************************************
 * This function returns the Tegra default topology tree information.
 ******************************************************************************/
const unsigned char *plat_get_power_domain_tree_desc(void)
{
	return tegra_power_domain_tree_desc;
}

unsigned int plat_get_syscnt_freq2(void)
{
	return 12000000;
}

/*******************************************************************************
 * Maximum supported UART controllers
 ******************************************************************************/
#define TEGRA132_MAX_UART_PORTS		5

/*******************************************************************************
 * This variable holds the UART port base addresses
 ******************************************************************************/
static uint32_t tegra132_uart_addresses[TEGRA132_MAX_UART_PORTS + 1] = {
	0,	/* undefined - treated as an error case */
	TEGRA_UARTA_BASE,
	TEGRA_UARTB_BASE,
	TEGRA_UARTC_BASE,
	TEGRA_UARTD_BASE,
	TEGRA_UARTE_BASE,
};

/*******************************************************************************
 * Retrieve the UART controller base to be used as the console
 ******************************************************************************/
uint32_t plat_get_console_from_id(int id)
{
	if (id > TEGRA132_MAX_UART_PORTS)
		return 0;

	return tegra132_uart_addresses[id];
}

/*******************************************************************************
 * Initialize the GIC and SGIs
 ******************************************************************************/
void plat_gic_setup(void)
{
	tegra_gic_setup(NULL);
}

/*******************************************************************************
 * Return pointer to the BL31 params from previous bootloader
 ******************************************************************************/
bl31_params_t *plat_get_bl31_params(void)
{
	return NULL;
}

/*******************************************************************************
 * Return pointer to the BL31 platform params from previous bootloader
 ******************************************************************************/
plat_params_from_bl2_t *plat_get_bl31_plat_params(void)
{
	return NULL;
}

/*******************************************************************************
 * Handler for early platform setup
 ******************************************************************************/
void plat_early_platform_setup(void)
{
	; /* do nothing */
}

/*******************************************************************************
 * Handler for late platform setup
 ******************************************************************************/
void plat_late_platform_setup(void)
{
	; /* do nothing */
}

/*******************************************************************************
 * Handler to indicate support for System Suspend
 ******************************************************************************/
bool plat_supports_system_suspend(void)
{
	return true;
}
