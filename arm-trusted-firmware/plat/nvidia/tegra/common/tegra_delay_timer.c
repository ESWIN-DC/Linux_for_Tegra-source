/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <delay_timer.h>
#include <mmio.h>
#include <platform.h>
#include <tegra_def.h>
#include <tegra_private.h>
#include <utils_def.h>

/* Ticks elapsed in one second by a signal of 1 MHz */
#define MHZ_TICKS_PER_SEC U(1000000)

/* Enable physical secure timer */
#define CNTPS_CTL_EL1_ENABLE_TIMER	(U(1) << 0)
#define CNTPS_CTL_EL1_DISABLE_INTERRUPT	(U(1) << 1)
#define ENABLE_CNTPS_EL1_TIMER		(CNTPS_CTL_EL1_DISABLE_INTERRUPT | \
					 CNTPS_CTL_EL1_ENABLE_TIMER)

static uint32_t tegra_timer_get_value(void)
{
	/* make sure cnts_tval_el1 timer is enabled */
	write_cntps_ctl_el1(ENABLE_CNTPS_EL1_TIMER);

	/*
	 * Generic delay timer implementation expects the timer to be a down
	 * counter. We apply bitwise NOT operator to the tick values returned
	 * by read_cntps_tval_el1() to simulate the down counter. The value is
	 * clipped from 64 to 32 bits.
	 */
	return (uint32_t)(~read_cntps_tval_el1());
}

/*
 * Initialise the on-chip free rolling us counter as the delay
 * timer.
 */
void tegra_delay_timer_init(void)
{
	static timer_ops_t tegra_timer_ops;

	/* Value in ticks */
	uint32_t multiplier = MHZ_TICKS_PER_SEC;

	/* Value in ticks per second (Hz) */
	uint32_t divider  = plat_get_syscnt_freq2();

	/* Reduce multiplier and divider by dividing them repeatedly by 10 */
	while (((multiplier % 10U) == 0U) && ((divider % 10U) == 0U)) {
		multiplier /= 10U;
		divider /= 10U;
	}

	/* enable cnts_tval_el1 timer */
	write_cntps_ctl_el1(ENABLE_CNTPS_EL1_TIMER);

	/* register the timer */
	tegra_timer_ops.get_timer_value = tegra_timer_get_value;
	tegra_timer_ops.clk_mult = multiplier;
	tegra_timer_ops.clk_div = divider;
	timer_init(&tegra_timer_ops);
}
