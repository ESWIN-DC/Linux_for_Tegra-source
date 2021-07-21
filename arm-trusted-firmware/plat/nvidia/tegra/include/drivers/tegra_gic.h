/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __TEGRA_GIC_H__
#define __TEGRA_GIC_H__

/*******************************************************************************
 * Per-CPU struct describing FIQ state to be stored
 ******************************************************************************/
typedef struct pcpu_fiq_state {
	uint64_t elr_el3;
	uint64_t spsr_el3;
} pcpu_fiq_state_t;

/*******************************************************************************
 * Struct describing per-FIQ configuration settings
 ******************************************************************************/
typedef struct irq_sec_cfg {
	/* IRQ number */
	uint32_t irq;
	/* Target CPUs servicing this interrupt */
	uint32_t target_cpus;
	/* type = INTR_TYPE_S_EL1 or INTR_TYPE_EL3 */
	uint32_t type;
} irq_sec_cfg_t;

typedef struct tegra_gic_cfg {
	const irq_sec_cfg_t *irq_cfg;
	const uint32_t *g0_int_array;
	uint32_t g0_int_num;
	const uint32_t *g1s_int_array;
	uint32_t g1s_int_num;
} tegra_gic_cfg_t;

/*******************************************************************************
 * Fucntion declarations
 ******************************************************************************/
void tegra_gic_cpuif_deactivate(void);
void tegra_gic_init(void);
void tegra_gic_pcpu_init(void);
void tegra_gic_setup(tegra_gic_cfg_t *cfg);

#endif /* __TEGRA_GIC_H__ */
