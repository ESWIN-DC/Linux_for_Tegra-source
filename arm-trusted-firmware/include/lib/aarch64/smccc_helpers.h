/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __SMCCC_HELPERS_H__
#define __SMCCC_HELPERS_H__

#include <smccc.h>

#ifndef __ASSEMBLY__
#include <context.h>

/* Convenience macros to return from SMC handler */
#define SMC_RET0(_h)	{					\
	return 0;						\
}
#define SMC_RET1(_h, _x0)	{				\
	write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X0), (_x0));	\
	SMC_RET0(_h);						\
}
#define SMC_RET2(_h, _x0, _x1)	{				\
	write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X1), (_x1));	\
	SMC_RET1(_h, (_x0));					\
}
#define SMC_RET3(_h, _x0, _x1, _x2)	{			\
	write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X2), (_x2));	\
	SMC_RET2(_h, (_x0), (_x1));				\
}
#define SMC_RET4(_h, _x0, _x1, _x2, _x3)	{		\
	write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X3), (_x3));	\
	SMC_RET3(_h, (_x0), (_x1), (_x2));			\
}
#define SMC_RET5(_h, _x0, _x1, _x2, _x3, _x4)	{		\
	write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X4), (_x4));	\
	SMC_RET4(_h, (_x0), (_x1), (_x2), (_x3));		\
}
#define SMC_RET6(_h, _x0, _x1, _x2, _x3, _x4, _x5)	{	\
	write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X5), (_x5));	\
	SMC_RET5(_h, (_x0), (_x1), (_x2), (_x3), (_x4));	\
}
#define SMC_RET7(_h, _x0, _x1, _x2, _x3, _x4, _x5, _x6)	{	\
	write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X6), (_x6));	\
	SMC_RET6(_h, (_x0), (_x1), (_x2), (_x3), (_x4), (_x5));	\
}
#define SMC_RET8(_h, _x0, _x1, _x2, _x3, _x4, _x5, _x6, _x7) {	\
	write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X7), (_x7));	\
	SMC_RET7(_h, (_x0), (_x1), (_x2), (_x3), (_x4), (_x5), (_x6));	\
}

/*
 * Convenience macros to access general purpose registers using handle provided
 * to SMC handler. These take the offset values defined in context.h
 */
#define SMC_GET_GP(_h, _g)					\
	read_ctx_reg((get_gpregs_ctx(_h)), (_g))
#define SMC_SET_GP(_h, _g, _v)					\
	write_ctx_reg((get_gpregs_ctx(_h)), (_g), (_v))

/*
 * Convenience macros to access EL3 context registers using handle provided to
 * SMC handler. These take the offset values defined in context.h
 */
#define SMC_GET_EL3(_h, _e)					\
	read_ctx_reg((get_el3state_ctx(_h)), (_e))
#define SMC_SET_EL3(_h, _e, _v)					\
	write_ctx_reg((get_el3state_ctx(_h)), (_e), (_v))

/* Return a UUID in the SMC return registers */
#define SMC_UUID_RET(_h, _uuid)					\
	SMC_RET4(handle, ((const uint32_t *) &(_uuid))[0],	\
			 ((const uint32_t *) &(_uuid))[1],	\
			 ((const uint32_t *) &(_uuid))[2],	\
			 ((const uint32_t *) &(_uuid))[3])

/*
 * Helper macro to write UUID value to cpu_context_t.
 */
#define write_uuid_to_ctx(_h, _uuid)						\
	do {									\
		uint32_t val;							\
		val = _uuid.time_low;						\
		write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X3), (val));	\
		val = (uint32_t)_uuid.time_mid | _uuid.time_hi_and_version;	\
		write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X2), (val));	\
		val = (uint32_t)_uuid.clock_seq_hi_and_reserved |		\
			_uuid.clock_seq_low | _uuid.node[0] |			\
			_uuid.node[1];						\
		write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X1), (val));	\
		val = (uint32_t)_uuid.node[2] | _uuid.node[3] |			\
		      _uuid.node[4] | _uuid.node[5];				\
		write_ctx_reg((get_gpregs_ctx(_h)), (CTX_GPREG_X0), (val));	\
	} while (false)								\

/*
 * Helper macro to retrieve the SMC parameters from cpu_context_t.
 */
#define get_smc_params_from_ctx(_hdl, _x1, _x2, _x3, _x4)	\
	do {							\
		const gp_regs_t *regs = get_gpregs_ctx(_hdl);	\
		_x1 = read_ctx_reg(regs, CTX_GPREG_X1);		\
		_x2 = read_ctx_reg(regs, CTX_GPREG_X2);		\
		_x3 = read_ctx_reg(regs, CTX_GPREG_X3);		\
		_x4 = read_ctx_reg(regs, CTX_GPREG_X4);		\
	} while (0)

#endif /*__ASSEMBLY__*/

#endif /* __SMCCC_HELPERS_H__ */