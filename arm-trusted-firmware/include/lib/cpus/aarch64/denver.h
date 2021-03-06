/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __DENVER_H__
#define __DENVER_H__

/* MIDR values for Denver */
#define DENVER_MIDR_PN0			U(0x4E0F0000)
#define DENVER_MIDR_PN1			U(0x4E0F0010)
#define DENVER_MIDR_PN2			U(0x4E0F0020)
#define DENVER_MIDR_PN3			U(0x4E0F0030)
#define DENVER_MIDR_PN4			U(0x4E0F0040)

/* Implementer code in the MIDR register */
#define DENVER_IMPL			U(0x4E)

/* CPU state ids - implementation defined */
#define DENVER_CPU_STATE_POWER_DOWN	U(0x3)

/* Speculative store buffering */
#define DENVER_CPU_DIS_SSB_EL3		(U(1) << 11)
#define DENVER_PN4_CPU_DIS_SSB_EL3	(U(1) << 18)

/* Speculative memory disambiguation */
#define DENVER_CPU_DIS_MD_EL3		(U(1) << 9)
#define DENVER_PN4_CPU_DIS_MD_EL3	(U(1) << 17)

/* Core power management states */
#define DENVER_CPU_PMSTATE_C1		U(0x1)
#define DENVER_CPU_PMSTATE_C6		U(0x6)
#define DENVER_CPU_PMSTATE_C7		U(0x7)
#define DENVER_CPU_PMSTATE_MASK		U(0xF)

/* ACTRL_ELx bits to enable dual execution*/
#define DENVER_CPU_ENABLE_DUAL_EXEC_EL2 (ULL(1) << 9)
#define DENVER_CPU_ENABLE_DUAL_EXEC_EL3 (ULL(1) << 9)

/* ACTRL_EL3 bit for Secure Performance Monitors enable */
#define DENVER_CPU_ENABLE_MDCR_EL3_SPME (ULL(1) << 6)

#ifndef __ASSEMBLY__

/* Disable Dynamic Code Optimisation */
void denver_disable_dco(void);

#endif

#endif /* __DENVER_H__ */
