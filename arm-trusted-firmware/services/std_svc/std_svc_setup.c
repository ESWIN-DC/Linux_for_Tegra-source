/*
 * Copyright (c) 2014-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <context_mgmt.h>
#include <cpu_data.h>
#include <debug.h>
#include <pmf.h>
#include <psci.h>
#include <runtime_instr.h>
#include <runtime_svc.h>
#include <smccc_helpers.h>
#include <std_svc.h>
#include <stdint.h>
#include <uuid.h>

/* Standard Service UUID */
DEFINE_SVC_UUID((arm_svc_uid),
		(0x108d905b), (0xf863), (0x47e8), (0xae), (0x2d),
		(0xc0), (0xfb), (0x56), (0x41), (0xf6), (0xe2));

/* Setup Standard Services */
static int32_t std_svc_setup(void)
{
	uintptr_t svc_arg;

	svc_arg = get_arm_std_svc_args(PSCI_FID_MASK);
	assert(svc_arg != 0U);

	/*
	 * PSCI is the only specification implemented as a Standard Service.
	 * The `psci_setup()` also does EL3 architectural setup.
	 */
	return psci_setup((const psci_lib_args_t *)svc_arg);
}

/*
 * Top-level Standard Service SMC handler. This handler will in turn dispatch
 * calls to PSCI SMC handler
 */
uintptr_t std_svc_smc_handler(uint32_t smc_fid,
			     u_register_t x1,
			     u_register_t x2,
			     u_register_t x3,
			     u_register_t x4,
			     void *cookie,
			     void *handle,
			     u_register_t flags)
{
	/*
	 * Dispatch PSCI calls to PSCI SMC handler and return its return
	 * value
	 */
	if (is_psci_fid(smc_fid)) {
		uint64_t ret;

#if ENABLE_RUNTIME_INSTRUMENTATION

		/*
		 * Flush cache line so that even if CPU power down happens
		 * the timestamp update is reflected in memory.
		 */
		PMF_WRITE_TIMESTAMP(rt_instr_svc,
		    RT_INSTR_ENTER_PSCI,
		    PMF_CACHE_MAINT,
		    get_cpu_data(cpu_data_pmf_ts[CPU_DATA_PMF_TS0_IDX]));
#endif

		ret = psci_smc_handler(smc_fid, x1, x2, x3, x4,
		    cookie, handle, flags);

#if ENABLE_RUNTIME_INSTRUMENTATION
		PMF_CAPTURE_TIMESTAMP(rt_instr_svc,
		    RT_INSTR_EXIT_PSCI,
		    PMF_NO_CACHE_MAINT);
#endif

		SMC_RET1(handle, ret);
	}

	switch (smc_fid) {
	case ARM_STD_SVC_CALL_COUNT:
		/*
		 * Return the number of Standard Service Calls. PSCI is the only
		 * standard service implemented; so return number of PSCI calls
		 */
		write_ctx_reg((get_gpregs_ctx(handle)), (CTX_GPREG_X0), (PSCI_NUM_CALLS));
		break;

	case ARM_STD_SVC_UID:
		/* Return UID to the caller */
		write_uuid_to_ctx((handle), (arm_svc_uid));
		break;

	case ARM_STD_SVC_VERSION:
		/* Return the version of current implementation */
		write_ctx_reg((get_gpregs_ctx(handle)), (CTX_GPREG_X0), (STD_SVC_VERSION_MAJOR));
		write_ctx_reg((get_gpregs_ctx(handle)), (CTX_GPREG_X1), (STD_SVC_VERSION_MINOR));
		break;

	default:
		WARN("Unimplemented Standard Service Call: 0x%x \n", smc_fid);
		write_ctx_reg((get_gpregs_ctx(handle)), (CTX_GPREG_X0), ((uint64_t)SMC_UNK));
		break;
	}

	return 0;
}

/* Register Standard Service Calls as runtime service */
DECLARE_RT_SVC(
		std_svc,

		(OEN_STD_START),
		(OEN_STD_END),
		((uint8_t)SMC_TYPE_FAST),
		(std_svc_setup),
		(std_svc_smc_handler)
);
