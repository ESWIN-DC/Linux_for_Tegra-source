/*
 * Copyright (c) 2014-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>

/***********************************************************
 * The tf_printf implementation for all BL stages
 ***********************************************************/

#define get_num_va_args(args_m, lcount) \
	(((lcount) > 1) ? va_arg(args_m, long long int) :	\
	(((lcount) != 0) ? va_arg(args_m, long int) : va_arg(args_m, int)))

#define get_unum_va_args(args_m, lcount) \
	(((lcount) > 1) ? va_arg(args_m, unsigned long long int) :	\
	(((lcount) != 0) ? va_arg(args_m, unsigned long int) : va_arg(args_m, unsigned int)))

static void string_print(const char *str)
{
	while (*str != '\0') {
		(void)putchar((int32_t)*str++);
	}
}

static void unsigned_num_print(uint64_t unum, uint32_t radix)
{
	/* Just need enough space to store 64 bit decimal integer */
	char num_buf[20];
	int32_t i = 0;
	uint32_t rem;

	do {
		rem = (uint32_t)unum % radix;
		if (rem < 0xaU) {
			num_buf[i++] = '0' + rem;
		} else {
			num_buf[i++] = 'a' + (rem - 0xaU);
		}
	} while ((unum /= radix) != 0U);

	while (--i >= 0) {
		(void)putchar((int32_t)num_buf[i]);
	}
}

/*******************************************************************
 * Reduced format print for Trusted firmware.
 * The following type specifiers are supported by this print
 * %x - hexadecimal format
 * %s - string format
 * %d or %i - signed decimal format
 * %u - unsigned decimal format
 * %p - pointer format
 *
 * The following length specifiers are supported by this print
 * %l - long int (64-bit on AArch64)
 * %ll - long long int (64-bit on AArch64)
 * %z - size_t sized integer formats (64 bit on AArch64)
 *
 * The print exits on all other formats specifiers other than valid
 * combinations of the above specifiers.
 *******************************************************************/
void tf_printf(const char *fmt, ...)
{
	va_list args;
	int32_t l_count;
	int64_t num;
	uint64_t unum;
	char *str;
	uintptr_t ptr;

	va_start(args, fmt);
	while (*fmt != '\0') {
		l_count = 0;

		if (*fmt == '%') {
			fmt++;

			/* adjust l_count for 'l' and 'z' cases */
			while ((*fmt == 'l') || (*fmt == 'z')) {
				if (*fmt == 'l') {
					l_count++;
				}
				if ((*fmt == 'z')) {
					l_count = 2;
				}
				fmt++;
			};

			/* Check the format specifier */
			switch (*fmt) {
			case 'i': /* Fall through to next one */
			case 'd':
				num = get_num_va_args(args, l_count);
				if (num < 0) {
					(void)putchar((int32_t)'-');
					unum = (uint64_t)-num;
				} else {
					unum = (uint64_t)num;
				}

				unsigned_num_print(unum, 10);
				break;
			case 's':
				str = va_arg(args, char *);
				string_print(str);
				break;
			case 'p':
				ptr = va_arg(args, uintptr_t);
				if (ptr != 0U) {
					string_print("0x");
				}

				unsigned_num_print(ptr, 16);
				break;
			case 'x':
				unum = get_unum_va_args(args, l_count);
				unsigned_num_print(unum, 16);
				break;
			case 'u':
				unum = get_unum_va_args(args, l_count);
				unsigned_num_print(unum, 10);
				break;
			default:
				/* assert on any other format specifier */
				assert(true);
				break;
			}
			fmt++;
		} else {
			(void)putchar((int32_t)*fmt++);
		}
	}

	va_end(args);
}
