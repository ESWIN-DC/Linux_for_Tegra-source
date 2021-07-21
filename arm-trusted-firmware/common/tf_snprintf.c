/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <platform.h>
#include <stdarg.h>

static void unsigned_dec_print(char **s, size_t n, size_t *chars_printed,
			       uint32_t unum)
{
	/* Enough for a 32-bit unsigned decimal integer (4294967295). */
	u_char num_buf[10];
	int32_t i=0;
	uint8_t rem;

	do {
		rem = (uint8_t)(unum % 10U);
		num_buf[i++] = (uint8_t)'0' + rem;
	} while ((unum /= 10U) != 0U);

	while (--i >= 0) {
		if (*chars_printed < n) {
			*(*s)++ = (char)num_buf[i];
		}
		(*chars_printed)++;
	}
}

/*******************************************************************
 * Reduced snprintf to be used for Trusted firmware.
 * The following type specifiers are supported:
 *
 * %d or %i - signed decimal format
 * %u - unsigned decimal format
 *
 * The function panics on all other formats specifiers.
 *
 * It returns the number of characters that would be written if the
 * buffer was big enough. If it returns a value lower than n, the
 * whole string has been written.
 *******************************************************************/
size_t tf_snprintf(char *s, size_t n, const char *fmt, ...)
{
	va_list args;
	int32_t num;
	uint32_t unum;
	size_t chars_printed = 0;

	if (n == 1U) {
		/* Buffer is too small to actually write anything else. */
		*s = '\0';
		n = 0;
	} else if (n >= 2U) {
		/* Reserve space for the terminator character. */
		n--;
	} else {
		; /* do nothing */
	}

	va_start(args, fmt);
	while (*fmt != '\0') {

		if (*fmt == '%') {
			fmt++;
			/* Check the format specifier. */
			switch (*fmt) {
			case 'i':
			case 'd':
				num = va_arg(args, int32_t);

				if (num < 0) {
					if (chars_printed < n) {
						*s++ = '-';
					}
					chars_printed++;

					unum = (uint32_t)-num;
				} else {
					unum = (uint32_t)num;
				}

				unsigned_dec_print(&s, n, &chars_printed, unum);
				break;
			case 'u':
				unum = va_arg(args, uint32_t);
				unsigned_dec_print(&s, n, &chars_printed, unum);
				break;
			default:
				/* Panic on any other format specifier. */
				ERROR("tf_snprintf: specifier with ASCII code '%d' not supported.",
				      *fmt);
				plat_panic_handler();
				break;
			}
			fmt++;
			continue;
		}

		if (chars_printed < n) {
			*s++ = *fmt;
		}
		fmt++;
		chars_printed++;
	}

	va_end(args);

	if (n > 0U) {
		*s = '\0';
	}

	return chars_printed;
}
