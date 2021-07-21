/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <debug.h>
#include <errata_report.h>
#include <runtime_svc.h>
#include <smccc.h>
#include <smccc_helpers.h>
#include <wa_cve_2017_5715.h>
#include <wa_cve_2018_3639.h>

static uint32_t smccc_version(void)
{
	return MAKE_SMCCC_VERSION(SMCCC_MAJOR_VERSION, SMCCC_MINOR_VERSION);
}

static int32_t smccc_arch_features(u_register_t arg)
{
	int32_t ret;

	switch (arg) {
	case SMCCC_VERSION:
	case SMCCC_ARCH_FEATURES:
		ret = (int32_t)SMC_OK;
		break;
#if WORKAROUND_CVE_2017_5715
	case SMCCC_ARCH_WORKAROUND_1:
		if (check_wa_cve_2017_5715() == ERRATA_NOT_APPLIES) {
			ret = 1;
		} else {
			ret = 0; /* ERRATA_APPLIES || ERRATA_MISSING */
		}
		break;
#endif
#if WORKAROUND_CVE_2018_3639
	case SMCCC_ARCH_WORKAROUND_2:
#if DYNAMIC_WORKAROUND_CVE_2018_3639
		/*
		 * On a platform where at least one CPU requires
		 * dynamic mitigation but others are either unaffected
		 * or permanently mitigated, report the latter as not
		 * needing dynamic mitigation.
		 */
		if (wa_cve_2018_3639_get_disable_ptr() == NULL) {
			ret = 1;
		} else {
			/*
			 * If we get here, this CPU requires dynamic mitigation
			 * so report it as such.
			 */
			ret = 0;
		}
#else
		/* Either the CPUs are unaffected or permanently mitigated */
		ret = SMCCC_ARCH_NOT_REQUIRED;
#endif
		break;
#endif
	default:
		ret = SMC_UNK;
		break;
	}

	return ret;
}

/*
 * Top-level Arm Architectural Service SMC handler.
 */
static uintptr_t arm_arch_svc_smc_handler(uint32_t smc_fid,
	u_register_t x1,
	u_register_t x2,
	u_register_t x3,
	u_register_t x4,
	void *cookie,
	void *handle,
	u_register_t flags)
{
	switch (smc_fid) {
	case SMCCC_VERSION:
		write_ctx_reg((get_gpregs_ctx(handle)), (CTX_GPREG_X0), ((uint64_t)smccc_version()));
		break;
	case SMCCC_ARCH_FEATURES:
		write_ctx_reg((get_gpregs_ctx(handle)), (CTX_GPREG_X0), ((uint64_t)smccc_arch_features(x1)));
		break;
#if WORKAROUND_CVE_2017_5715
	case SMCCC_ARCH_WORKAROUND_1:
		/*
		 * The workaround has already been applied on affected PEs
		 * during entry to EL3.  On unaffected PEs, this function
		 * has no effect.
		 */
		write_ctx_reg((get_gpregs_ctx(handle)), (CTX_GPREG_X0), (0));
		break;
#endif
#if WORKAROUND_CVE_2018_3639
	case SMCCC_ARCH_WORKAROUND_2:
		/*
		 * The workaround has already been applied on affected PEs
		 * requiring dynamic mitigation during entry to EL3.
		 * On unaffected or statically mitigated PEs, this function
		 * has no effect.
		 */
		write_ctx_reg((get_gpregs_ctx(handle)), (CTX_GPREG_X0), (0));
		break;
#endif
	default:
		WARN("Unimplemented Arm Architecture Service Call: 0x%x \n",
			smc_fid);
		write_ctx_reg((get_gpregs_ctx(handle)), (CTX_GPREG_X0), ((uint64_t)SMC_UNK));
		break;
	}

	return SMC_OK;
}

/* Register Standard Service Calls as runtime service */
DECLARE_RT_SVC(
		arm_arch_svc,
		(OEN_ARM_START),
		(OEN_ARM_END),
		(uint8_t)(SMC_TYPE_FAST),
		(NULL),
		(arm_arch_svc_smc_handler)
);
