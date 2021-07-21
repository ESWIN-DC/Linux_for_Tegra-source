/*
 * Copyright (c) 2013-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <stdlib.h>

/*
 * This is a basic implementation. This could be improved.
 */
void abort (void)
{
	ERROR("ABORT\n");
	panic();
}
