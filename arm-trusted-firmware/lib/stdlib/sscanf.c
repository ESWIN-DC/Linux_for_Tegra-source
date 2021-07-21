/*
 * Copyright (c) 2015-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/cdefs.h>

/*
 * TODO: This is not a real implementation of the sscanf() function. It just
 * returns the number of expected arguments based on the number of '%' found
 * in the format string.
 */
int
sscanf(const char *str, char const *fmt, ...)
{
	int ret = 0;

	while (*fmt != '\0') {
		if (*fmt == '%') {
			ret++;
		}
		fmt++;
	}

	return ret;
}
