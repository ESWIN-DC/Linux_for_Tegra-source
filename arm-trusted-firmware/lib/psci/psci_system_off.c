/*
 * Copyright (c) 2014-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <arch_helpers.h>
#include <assert.h>
#include <console.h>
#include <debug.h>
#include <platform.h>
#include "psci_private.h"

void psci_system_off(void)
{
	psci_print_power_domain_map();

	assert(psci_plat_pm_ops->system_off != NULL);

	/* Notify the Secure Payload Dispatcher */
	if ((psci_spd_pm != NULL) && (psci_spd_pm->svc_system_off != NULL)) {
		psci_spd_pm->svc_system_off();
	}

	(void)console_flush();

	/* Call the platform specific hook */
	psci_plat_pm_ops->system_off();

	/* This function does not return. We should never get here */
}

void psci_system_reset(void)
{
	psci_print_power_domain_map();

	assert(psci_plat_pm_ops->system_reset != NULL);

	/* Notify the Secure Payload Dispatcher */
	if ((psci_spd_pm != NULL) && (psci_spd_pm->svc_system_reset != NULL)) {
		psci_spd_pm->svc_system_reset();
	}

	(void)console_flush();

	/* Call the platform specific hook */
	psci_plat_pm_ops->system_reset();

	/* This function does not return. We should never get here */
}
