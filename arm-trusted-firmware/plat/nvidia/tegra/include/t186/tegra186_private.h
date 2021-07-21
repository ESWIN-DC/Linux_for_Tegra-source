/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __TEGRA186_PRIVATE_H__
#define __TEGRA186_PRIVATE_H__

void tegra186_cpu_reset_handler(void);
uint64_t tegra186_get_cpu_reset_handler_base(void);
uint64_t tegra186_get_cpu_reset_handler_size(void);
uint64_t tegra186_get_smmu_ctx_offset(void);
void tegra186_set_system_suspend_entry(void);

#endif /* __TEGRA186_PRIVATE_H__ */
