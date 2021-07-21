/*
 * Copyright (c) 2014-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __STD_SVC_H__
#define __STD_SVC_H__

/* SMC function IDs for Standard Service queries */

#define ARM_STD_SVC_CALL_COUNT		0x8400ff00
#define ARM_STD_SVC_UID			0x8400ff01
/*					0x8400ff02 is reserved */
#define ARM_STD_SVC_VERSION		0x8400ff03

/* ARM Standard Service Calls version numbers */
#define STD_SVC_VERSION_MAJOR		0x0
#define STD_SVC_VERSION_MINOR		0x1

/*
 * Get the ARM Standard Service argument from EL3 Runtime.
 * This function must be implemented by EL3 Runtime and the
 * `svc_mask` identifies the service. `svc_mask` is a bit
 * mask identifying the range of SMC function IDs available
 * to the service.
 */
uintptr_t get_arm_std_svc_args(uint32_t svc_mask);

uintptr_t std_svc_smc_handler(uint32_t smc_fid,
			     u_register_t x1,
			     u_register_t x2,
			     u_register_t x3,
			     u_register_t x4,
			     void *cookie,
			     void *handle,
			     u_register_t flags);

#endif /* __STD_SVC_H__ */
