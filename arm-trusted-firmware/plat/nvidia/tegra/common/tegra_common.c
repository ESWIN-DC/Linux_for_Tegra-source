/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <gpcdma.h>
#include <tegra_platform.h>
#include <tegra_private.h>

/*******************************************************************************
 * Helper memcpy function.
 ******************************************************************************/
void tegra_memcpy(uint64_t dst, uint64_t src, uint64_t num_bytes)
{
#if defined(USE_GPC_DMA) && USE_GPC_DMA
	tegra_gpcdma_memcpy(dst, src, (uint32_t)num_bytes);
#else
	tegra_memcpy16(dst, src, num_bytes);
#endif
}

/*******************************************************************************
 * Helper zeromem function.
 ******************************************************************************/
void tegra_zeromem(uint64_t base, uint64_t num_bytes)
{
#if defined(USE_GPC_DMA) && USE_GPC_DMA
	tegra_gpcdma_zeromem(base, (uint32_t)num_bytes);
#else
	tegra_zeromem16(base, num_bytes);
#endif
}
