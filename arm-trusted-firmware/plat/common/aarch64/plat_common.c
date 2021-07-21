/*
 * Copyright (c) 2014-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <console.h>
#include <platform.h>
#include <xlat_mmu_helpers.h>

/*
 * The following platform setup functions are weakly defined. They
 * provide typical implementations that may be re-used by multiple
 * platforms but may also be overridden by a platform if required.
 */
#pragma weak bl31_plat_enable_mmu
#pragma weak bl32_plat_enable_mmu
#pragma weak bl31_plat_runtime_setup
#if !ERROR_DEPRECATED
#pragma weak plat_get_syscnt_freq2
#endif /* ERROR_DEPRECATED */

void bl31_plat_enable_mmu(uint32_t flags)
{
	enable_mmu_el3(flags);
}

void bl32_plat_enable_mmu(uint32_t flags)
{
	enable_mmu_el1(flags);
}

void bl31_plat_runtime_setup(void)
{
	/*
	 * Finish the use of console driver in BL31 so that any runtime logs
	 * from BL31 will be suppressed.
	 */
	console_uninit();
}

#if !ENABLE_PLAT_COMPAT
/*
 * Helper function for platform_get_pos() when platform compatibility is
 * disabled. This is to enable SPDs using the older platform API to continue
 * to work.
 */
uint32_t platform_core_pos_helper(uint64_t mpidr)
{
	int32_t idx = plat_core_pos_by_mpidr(mpidr);
	assert(idx >= 0);
	return (uint32_t)idx;
}
#endif


#if !ERROR_DEPRECATED
uint32_t plat_get_syscnt_freq2(void)
{
	uint64_t freq = plat_get_syscnt_freq();

	assert((freq >> 32U) == 0U);

	return (uint32_t)freq;
}
#endif /* ERROR_DEPRECATED */
