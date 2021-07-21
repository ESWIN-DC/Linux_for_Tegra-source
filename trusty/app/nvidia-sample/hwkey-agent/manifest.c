/*
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <trusty_app_manifest.h>
#include <stddef.h>
#include <stdio.h>

trusty_app_manifest_t TRUSTY_APP_MANIFEST_ATTRS trusty_app_manifest =
{
	/*
	 * Each trusted app UUID should have a unique UUID that is
	 * generated from a UUID generator such as
         * https://www.uuidgenerator.net/
	 *
	 * UUID : {92b92883-f96a-4177-97d1-69b2e278a46f}
	 */
	{ 0x92b92883, 0xf96a, 0x4177,
	  { 0x97, 0xd1, 0x6b, 0xb2, 0xe2, 0x78, 0xa4, 0x6f } },

	/* Optional configuration options here */
	{
		TRUSTY_APP_CONFIG_MIN_HEAP_SIZE(MIN_HEAP_SIZE),
		TRUSTY_APP_CONFIG_MIN_STACK_SIZE(MIN_STACK_SIZE),

		/* SE register mapping */
		TRUSTY_APP_CONFIG_MAP_MEM(1, TEGRA_SE_BASE, TEGRA_SE_SIZE),

		/* fuse mapping */
		TRUSTY_APP_CONFIG_MAP_MEM(2, TEGRA_FUSE_BASE, TEGRA_FUSE_SIZE),
	},
};
