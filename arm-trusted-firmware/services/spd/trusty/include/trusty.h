/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TRUSTY_H
#define TRUSTY_H

struct smc_args trusty_init_context_stack(void **sp, void *new_stack);
struct smc_args trusty_context_switch_helper(void **sp, void *smc_params);

#endif /* TRUSTY_H */
