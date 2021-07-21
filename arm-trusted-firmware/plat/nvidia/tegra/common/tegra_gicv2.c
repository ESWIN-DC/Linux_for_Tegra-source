/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <bl_common.h>
#include <gicv2.h>
#include <platform_def.h>
#include <tegra_private.h>
#include <tegra_def.h>
#include <utils.h>

/******************************************************************************
 * Tegra common helper to setup the GICv2 driver data.
 *****************************************************************************/
void tegra_gic_setup(tegra_gic_cfg_t *cfg)
{
	/*
	 * Tegra GIC configuration settings
	 */
	static gicv2_driver_data_t tegra_gic_data;

	/*
	 * Register Tegra GICv2 driver
	 */
	tegra_gic_data.gicd_base = TEGRA_GICD_BASE;
	tegra_gic_data.gicc_base = TEGRA_GICC_BASE;
	tegra_gic_data.g0_interrupt_num = cfg->g0_int_num;
	tegra_gic_data.g0_interrupt_array = cfg->g0_int_array;
	gicv2_driver_init(&tegra_gic_data);
}

/******************************************************************************
 * Tegra common helper to initialize the GICv2 only driver.
 *****************************************************************************/
void tegra_gic_init(void)
{
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();
}

/******************************************************************************
 * Tegra common helper to disable the GICv2 CPU interface
 *****************************************************************************/
void tegra_gic_cpuif_deactivate(void)
{
	gicv2_cpuif_disable();
}

/******************************************************************************
 * Tegra common helper to initialize the per cpu distributor interface
 * in GICv2
 *****************************************************************************/
void tegra_gic_pcpu_init(void)
{
	gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();
}
