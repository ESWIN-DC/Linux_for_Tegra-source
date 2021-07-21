/*
 * Copyright (c) 2020, NVIDIA Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <err_ptr.h>
#include <mm.h>
#include <trusty_std.h>
#include <fuse.h>

#define FUSE_OPT_VENDOR_CODE_0		0x200
#define FUSE_OPT_FAB_CODE_0		0x204
#define FUSE_OPT_LOT_CODE_0_0		0x208
#define FUSE_OPT_LOT_CODE_1_0		0x20c
#define FUSE_OPT_WAFER_ID_0		0x210
#define FUSE_OPT_X_COORDINATE_0		0x214
#define FUSE_OPT_Y_COORDINATE_0		0x218
#define FUSE_OPT_OPS_RESERVED_0		0x220
#define OPT_VENDOR_CODE_MASK		0xF
#define OPT_FAB_CODE_MASK		0x3F
#define OPT_LOT_CODE_1_MASK		0xfffffff
#define OPT_WAFER_ID_MASK		0x3F
#define OPT_X_COORDINATE_MASK		0x1FF
#define OPT_Y_COORDINATE_MASK		0x1FF
#define OPT_OPS_RESERVED_MASK		0x3F
#define ECID_ECID0_0_RSVD1_MASK		0x3F
#define ECID_ECID0_0_Y_MASK		0x1FF
#define ECID_ECID0_0_Y_RANGE		6
#define ECID_ECID0_0_X_MASK		0x1FF
#define ECID_ECID0_0_X_RANGE		15
#define ECID_ECID0_0_WAFER_MASK		0x3F
#define ECID_ECID0_0_WAFER_RANGE	24
#define ECID_ECID0_0_LOT1_MASK		0x3
#define ECID_ECID0_0_LOT1_RANGE		30
#define ECID_ECID1_0_LOT1_MASK		0x3FFFFFF
#define ECID_ECID1_0_LOT0_MASK		0x3F
#define ECID_ECID1_0_LOT0_RANGE		26
#define ECID_ECID2_0_LOT0_MASK		0x3FFFFFF
#define ECID_ECID2_0_FAB_MASK		0x3F
#define ECID_ECID2_0_FAB_RANGE		26
#define ECID_ECID3_0_VENDOR_MASK	0xF

#define NV_FUSE_READ(reg) *((uint32_t *)(fuse_base + reg))

static uint32_t ecid[4];

uint32_t *fuse_get_queried_ecid(void)
{
	return ecid;
}

void fuse_query_ecid(void)
{
	void *fuse_base = NULL;
	uint32_t vendor;
	uint32_t fab;
	uint32_t wafer;
	uint32_t lot0;
	uint32_t lot1;
	uint32_t x;
	uint32_t y;
	uint32_t rsvd1;
	uint32_t reg;

	uint32_t *ret = (uint32_t *)mmap(NULL, TEGRA_FUSE_SIZE, MMAP_FLAG_IO_HANDLE, 2);
	if (IS_ERR(ret)) {
		TLOGE("%s: mmap failure: err = %d, size = %x\n",
			__func__, PTR_ERR(ret), TEGRA_FUSE_SIZE);
		return;
	}

	fuse_base = (void *)ret;

	reg = NV_FUSE_READ(FUSE_OPT_VENDOR_CODE_0);
	vendor = reg & OPT_VENDOR_CODE_MASK;

	reg = NV_FUSE_READ(FUSE_OPT_FAB_CODE_0);
	fab = reg & OPT_FAB_CODE_MASK;

	lot0 = NV_FUSE_READ(FUSE_OPT_LOT_CODE_0_0);

	lot1 = 0;
	reg = NV_FUSE_READ(FUSE_OPT_LOT_CODE_1_0);
	lot1 = reg & OPT_LOT_CODE_1_MASK;

	reg = NV_FUSE_READ(FUSE_OPT_WAFER_ID_0);
	wafer = reg & OPT_WAFER_ID_MASK;

	reg = NV_FUSE_READ(FUSE_OPT_X_COORDINATE_0);
	x = reg & OPT_X_COORDINATE_MASK;

	reg = NV_FUSE_READ(FUSE_OPT_Y_COORDINATE_0);
	y = reg & OPT_Y_COORDINATE_MASK;

	reg = NV_FUSE_READ(FUSE_OPT_OPS_RESERVED_0);
	rsvd1 = reg & OPT_OPS_RESERVED_MASK;

	reg = 0;
	reg |= rsvd1 && ECID_ECID0_0_RSVD1_MASK;
	reg |= (y & ECID_ECID0_0_Y_MASK) << ECID_ECID0_0_Y_RANGE;
	reg |= (x & ECID_ECID0_0_X_MASK) << ECID_ECID0_0_X_RANGE;
	reg |= (wafer & ECID_ECID0_0_WAFER_MASK) << ECID_ECID0_0_WAFER_RANGE;
	reg |= (lot1 & ECID_ECID0_0_LOT1_MASK) << ECID_ECID0_0_LOT1_RANGE;
	ecid[0] = reg;

	lot1 >>= 2;

	reg = 0;
	reg |= lot1 & ECID_ECID1_0_LOT1_MASK;
	reg |= (lot0 & ECID_ECID1_0_LOT0_MASK) << ECID_ECID1_0_LOT0_RANGE;
	ecid[1] = reg;

	lot0 >>= 6;

	reg = 0;
	reg |= lot0 & ECID_ECID2_0_LOT0_MASK;
	reg |= (fab & ECID_ECID2_0_FAB_MASK) << ECID_ECID2_0_FAB_RANGE;
	ecid[2] = reg;

	reg = 0;
	reg |= vendor & ECID_ECID3_0_VENDOR_MASK;
	ecid[3] = reg;

	if (munmap(fuse_base, TEGRA_FUSE_SIZE) != 0) {
		TLOGE("%s: failed to unmap fuse region\n", __func__);
	}
}
