/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __CORTEX_ARES_H__
#define __CORTEX_ARES_H__

/* Cortex-ARES MIDR for revision 0 */
#define CORTEX_ARES_MIDR		0x410fd0c0

/*******************************************************************************
 * CPU Extended Control register specific definitions.
 ******************************************************************************/
#define CORTEX_ARES_CPUPWRCTLR_EL1	S3_0_C15_C2_7
#define CORTEX_ARES_CPUECTLR_EL1	S3_0_C15_C1_4

/* Definitions of register field mask in CORTEX_ARES_CPUPWRCTLR_EL1 */
#define CORTEX_ARES_CORE_PWRDN_EN_MASK	0x1

#endif /* __CORTEX_ARES_H__ */
