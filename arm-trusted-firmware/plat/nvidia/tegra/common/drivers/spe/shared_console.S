/*
 * Copyright (c) 2017-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <asm_macros.S>

#define CONSOLE_NUM_BYTES_SHIFT		24
#define CONSOLE_FLUSH_DATA_TO_PORT	(1 << 26)
#define CONSOLE_RING_DOORBELL		(1 << 31)
#define CONSOLE_IS_BUSY			(1 << 31)
#define CONSOLE_TIMEOUT			0xC000		/* 50 ms */
#define CONSOLE_WRITE			(CONSOLE_RING_DOORBELL | CONSOLE_FLUSH_DATA_TO_PORT)

	/*
	 * This file contains a driver implementation to make use of the
	 * real console implementation provided by the SPE firmware running
	 * SoCs after Tegra186.
	 *
	 * This console is shared by multiple components and the SPE firmware
	 * finally displays everything on the UART port.
	 */

	.globl	console_core_init
	.globl	console_core_putc
	.globl	console_core_getc
	.globl	console_core_flush

.macro	check_if_console_is_ready base, tmp1, tmp2, label
	/* wait until spe is ready or timeout expires */
	mrs	\tmp2, cntps_tval_el1
1:	ldr	\tmp1, [\base]
	and	\tmp1, \tmp1, #CONSOLE_IS_BUSY
	cbz	\tmp1, 2f
	mrs	\tmp1, cntps_tval_el1
	sub	\tmp1, \tmp2, \tmp1
	cmp	\tmp1, #CONSOLE_TIMEOUT
	b.lt	1b
	b	\label
2:
.endm

	/* -----------------------------------------------
	 * int console_core_init(uintptr_t base_addr,
	 * unsigned int uart_clk, unsigned int baud_rate)
	 * Function to initialize the console without a
	 * C Runtime to print debug information. This
	 * function will be accessed by console_init and
	 * crash reporting.
	 * In: x0 - console base address
	 *     w1 - Uart clock in Hz
	 *     w2 - Baud rate
	 * Out: return 1 on success else 0 on error
	 * Clobber list : x1, x2
	 * -----------------------------------------------
	 */
func console_core_init
	/* Check the input base address */
	cbz	x0, core_init_fail

	/* Dont use clock or baud rate, so ok to overwrite them */
	check_if_console_is_ready x0, x1, x2, core_init_fail

	mov	w0, #1
	ret
core_init_fail:
	mov	w0, wzr
	ret
endfunc console_core_init

	/* --------------------------------------------------------
	 * int console_core_putc(int c, uintptr_t base_addr)
	 * Function to output a character over the console. It
	 * returns the character printed on success or -1 on error.
	 * In : w0 - character to be printed
	 *      x1 - console base address
	 * Out : return -1 on error else return character.
	 * Clobber list : x2, x3
	 * --------------------------------------------------------
	 */
func console_core_putc
	/* Check the input parameter */
	cbz	x1, putc_error

	/* Prepend '\r' to '\n' */
	cmp	w0, #0xA
	b.ne	not_eol

	check_if_console_is_ready x1, x2, x3, putc_error

	/* spe is ready */
	mov	w2, #0xD		/* '\r' */
	and	w2, w2, #0xFF
	mov	w3, #(CONSOLE_WRITE | (1 << CONSOLE_NUM_BYTES_SHIFT))
	orr	w2, w2, w3
	str	w2, [x1]

not_eol:
	check_if_console_is_ready x1, x2, x3, putc_error

	/* spe is ready */
	mov	w2, w0
	and	w2, w2, #0xFF
	mov	w3, #(CONSOLE_WRITE | (1 << CONSOLE_NUM_BYTES_SHIFT))
	orr	w2, w2, w3
	str	w2, [x1]

	ret
putc_error:
	mov	x2, x30
	bl	console_uninit
	mov	x30, x2
	mov	w0, #-1
	ret
endfunc console_core_putc

	/* ---------------------------------------------
	 * int console_core_getc(uintptr_t base_addr)
	 * Function to get a character from the console.
	 * It returns the character grabbed on success
	 * or -1 on error.
	 * In : x0 - console base address
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func console_core_getc
	mov	w0, #-1
	ret
endfunc console_core_getc

	/* ---------------------------------------------
	 * int console_core_flush(uintptr_t base_addr)
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * In : x0 - console base address
	 * Out : return -1 on error else return 0.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func console_core_flush
	cbz	x0, flush_error

	/* flush console */
	mov	w1, #CONSOLE_WRITE
	str	w1, [x0]
	mov	w0, #0
	ret
flush_error:
	mov	w0, #-1
	ret
endfunc console_core_flush
