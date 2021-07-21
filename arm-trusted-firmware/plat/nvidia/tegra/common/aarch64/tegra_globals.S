/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch.h>
#include <asm_macros.S>

	/* Global functions */
	.globl	tegra_get_sec_entry_point
	.globl	tegra_set_sec_entry_point
	.globl	tegra_get_console_base
	.globl	tegra_set_console_base
	.globl	tegra_get_bl31_phys_base
	.globl	tegra_get_bl31_text_start
	.globl	tegra_get_bl31_text_end
	.globl	tegra_get_bl31_rw_start
	.globl	tegra_get_bl31_rw_end
	.globl	tegra_get_bl31_rodata_start
	.globl	tegra_get_bl31_rodata_end

func tegra_get_sec_entry_point
	adr	x0, tegra_sec_entry_point
	ldr	x0, [x0]
	ret
endfunc tegra_get_sec_entry_point

func tegra_set_sec_entry_point
	adr	x1, tegra_sec_entry_point
	str	x0, [x1]
	dsb	sy
	ret
endfunc tegra_set_sec_entry_point

func tegra_get_console_base
	adr	x0, tegra_console_base
	ldr	x0, [x0]
	ret
endfunc tegra_get_console_base

func tegra_set_console_base
	adr	x1, tegra_console_base
	str	x0, [x1]
	dsb	sy
	ret
endfunc tegra_set_console_base

func tegra_get_bl31_phys_base
	adr	x0, tegra_bl31_phys_base
	ldr	x0, [x0]
	ret
endfunc tegra_get_bl31_phys_base

func tegra_get_bl31_text_start
	adr	x0, __TEXT_START__
	ret
endfunc tegra_get_bl31_text_start

func tegra_get_bl31_text_end
	adr	x0, __TEXT_END__
	ret
endfunc tegra_get_bl31_text_end

func tegra_get_bl31_rw_start
	adr	x0, __RW_START__
	ret
endfunc tegra_get_bl31_rw_start

func tegra_get_bl31_rw_end
	adr	x0, __RW_END__
	ret
endfunc tegra_get_bl31_rw_end

func tegra_get_bl31_rodata_start
	adr	x0, __RODATA_START__
	ret
endfunc tegra_get_bl31_rodata_start

func tegra_get_bl31_rodata_end
	adr	x0, __RODATA_END__
	ret
endfunc tegra_get_bl31_rodata_end
