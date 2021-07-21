/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2020, NVIDIA Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <bpmp_ipc.h>
#include <bl_common.h>
#include <context.h>
#include <context_mgmt.h>
#include <debug.h>
#include <denver.h>
#include <mce.h>
#include <mce_private.h>
#include <platform.h>
#include <psci.h>
#include <se.h>
#include <smmu.h>
#include <stdbool.h>
#include <string.h>
#include <t194_nvg.h>
#include <tegra194_private.h>
#include <tegra_platform.h>
#include <tegra_private.h>
#include <tegra_def.h>

/* state id mask */
#define TEGRA194_STATE_ID_MASK		0xFU
/* constants to get power state's wake time */
#define TEGRA194_WAKE_TIME_MASK		0x0FFFFFF0U
#define TEGRA194_WAKE_TIME_SHIFT	4U
/* default core wake mask for CPU_SUSPEND */
#define TEGRA194_CORE_WAKE_MASK		0x180cU

static struct t19x_psci_percpu_data {
	uint32_t wake_time;
} __aligned(CACHE_WRITEBACK_GRANULE) t19x_percpu_data[PLATFORM_CORE_COUNT];

int32_t tegra_soc_validate_power_state(uint32_t power_state,
					psci_power_state_t *req_state)
{
	uint8_t state_id = (uint8_t)psci_get_pstate_id(power_state) & TEGRA194_STATE_ID_MASK;
	uint32_t cpu = plat_my_core_pos();
	int32_t ret = PSCI_E_SUCCESS;

	/* save the core wake time (in TSC ticks)*/
	t19x_percpu_data[cpu].wake_time = (power_state & TEGRA194_WAKE_TIME_MASK)
			<< TEGRA194_WAKE_TIME_SHIFT;

	/*
	 * Clean percpu_data[cpu] to DRAM. This needs to be done to ensure that
	 * the correct value is read in tegra_soc_pwr_domain_suspend(), which
	 * is called with caches disabled. It is possible to read a stale value
	 * from DRAM in that function, because the L2 cache is not flushed
	 * unless the cluster is entering CC6/CC7.
	 */
	clean_dcache_range((uint64_t)&t19x_percpu_data[cpu],
			sizeof(t19x_percpu_data[cpu]));

	/* Sanity check the requested state id */
	switch (state_id) {
	case PSTATE_ID_CORE_IDLE:

		/* Core idle request */
		req_state->pwr_domain_state[MPIDR_AFFLVL0] = PLAT_MAX_RET_STATE;
		req_state->pwr_domain_state[MPIDR_AFFLVL1] = PSCI_LOCAL_STATE_RUN;
		break;

	default:
		WARN("%s: unsupported state id (%d)\n", __func__, state_id);
		ret = PSCI_E_INVALID_PARAMS;
		break;
	}

	return ret;
}

int32_t tegra_soc_cpu_standby(plat_local_state_t cpu_state)
{
	uint32_t cpu = plat_my_core_pos();
	mce_cstate_info_t cstate_info = { 0 };

	/* Program default wake mask */
	cstate_info.wake_mask = TEGRA194_CORE_WAKE_MASK;
	cstate_info.update_wake_mask = 1;
	mce_update_cstate_info(&cstate_info);

	/* Enter CPU idle */
	(void)mce_command_handler((uint64_t)MCE_CMD_ENTER_CSTATE,
				  (uint64_t)TEGRA_NVG_CORE_C6,
				  t19x_percpu_data[cpu].wake_time,
				  0U);

	return PSCI_E_SUCCESS;
}

int32_t tegra_soc_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	const plat_local_state_t *pwr_domain_state;
	uint8_t stateid_afflvl2;
	plat_params_from_bl2_t *params_from_bl2 = bl31_get_plat_params();
	uint64_t smmu_ctx_base;
	uint32_t val;
	mce_cstate_info_t cstate_info = { 0 };
	int32_t ret;

	/* get the state ID */
	pwr_domain_state = target_state->pwr_domain_state;
	stateid_afflvl2 = pwr_domain_state[PLAT_MAX_PWR_LVL] &
		TEGRA194_STATE_ID_MASK;

	if (stateid_afflvl2 == PSTATE_ID_SOC_POWERDN) {

		/* save 'Secure Boot' Processor Feature Config Register */
		val = mmio_read_32(TEGRA_MISC_BASE + MISCREG_PFCFG);
		mmio_write_32(TEGRA_SCRATCH_BASE + SCRATCH_SECURE_BOOTP_FCFG, val);

		/* save SMMU context */
		smmu_ctx_base = params_from_bl2->tzdram_base +
				tegra194_get_smmu_ctx_offset();
		tegra_smmu_save_context((uintptr_t)smmu_ctx_base);

		/*
		 * Suspend SE, RNG1 and PKA1 only on silcon and fpga,
		 * since VDK does not support atomic se ctx save
		 */
		if (tegra_platform_is_silicon() || tegra_platform_is_fpga()) {
			ret = tegra_se_suspend();
			assert(ret == 0);
		}

		/* Prepare for system suspend */
		cstate_info.cluster = (uint32_t)TEGRA_NVG_CLUSTER_CC6;
		cstate_info.ccplex = (uint32_t)TEGRA_NVG_CG_CG7;
		cstate_info.system = (uint32_t)TEGRA_NVG_SYSTEM_SC7;
		cstate_info.system_state_force = 1;
		cstate_info.update_wake_mask = 1;
		mce_update_cstate_info(&cstate_info);

		do {
			val = (uint32_t)mce_command_handler(
					(uint32_t)MCE_CMD_IS_SC7_ALLOWED,
					(uint32_t)TEGRA_NVG_CORE_C7,
					MCE_CORE_SLEEP_TIME_INFINITE,
					0U);
		} while (val == 0U);

		/* Instruct the MCE to enter system suspend state */
		(void)mce_command_handler((uint64_t)MCE_CMD_ENTER_CSTATE,
				(uint64_t)TEGRA_NVG_CORE_C7,
				MCE_CORE_SLEEP_TIME_INFINITE, 0U);

		/* set system suspend state for house-keeping */
		tegra194_set_system_suspend_entry();

	}

	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * Helper function to check if this is the last ON CPU in the cluster
 ******************************************************************************/
static bool tegra_last_on_cpu_in_cluster(const plat_local_state_t *states,
			uint32_t ncpu)
{
	plat_local_state_t target;
	bool last_on_cpu = true;
	uint32_t num_cpus = ncpu, pos = 0;

	do {
		target = states[pos];
		if (target != PLAT_MAX_OFF_STATE) {
			last_on_cpu = false;
		}
		--num_cpus;
		pos++;
	} while (num_cpus != 0U);

	return last_on_cpu;
}

/*******************************************************************************
 * Helper function to get target power state for the cluster
 ******************************************************************************/
static plat_local_state_t tegra_get_afflvl1_pwr_state(const plat_local_state_t *states,
			uint32_t ncpu)
{
	uint32_t core_pos = (uint32_t)read_mpidr() & (uint32_t)MPIDR_CPU_MASK;
	plat_local_state_t target = states[core_pos];
	mce_cstate_info_t cstate_info = { 0 };

	/* CPU suspend */
	if (target == PSTATE_ID_CORE_POWERDN) {

		/* Program default wake mask */
		cstate_info.wake_mask = TEGRA194_CORE_WAKE_MASK;
		cstate_info.update_wake_mask = 1;
		mce_update_cstate_info(&cstate_info);
	}

	/* CPU off */
	if (target == PLAT_MAX_OFF_STATE) {

		/* Enable cluster powerdn from last CPU in the cluster */
		if (tegra_last_on_cpu_in_cluster(states, ncpu)) {

			/* Enable CC6 state and turn off wake mask */
			cstate_info.cluster = (uint32_t)TEGRA_NVG_CLUSTER_CC6;
                        cstate_info.ccplex = (uint32_t)TEGRA_NVG_CG_CG7;
                        cstate_info.system_state_force = 1;
			cstate_info.update_wake_mask = 1;
			mce_update_cstate_info(&cstate_info);

		} else {

			/* Turn off wake_mask */
			cstate_info.update_wake_mask = 1;
			mce_update_cstate_info(&cstate_info);
			target = PSCI_LOCAL_STATE_RUN;
		}
	}

	return target;
}

/*******************************************************************************
 * Platform handler to calculate the proper target power level at the
 * specified affinity level
 ******************************************************************************/
plat_local_state_t tegra_soc_get_target_pwr_state(uint32_t lvl,
					     const plat_local_state_t *states,
					     uint32_t ncpu)
{
	plat_local_state_t target = PSCI_LOCAL_STATE_RUN;
	uint32_t cpu = plat_my_core_pos();

	/* System Suspend */
	if ((lvl == (uint32_t)MPIDR_AFFLVL2) && (states[cpu] == PSTATE_ID_SOC_POWERDN)) {
		target = PSTATE_ID_SOC_POWERDN;
	}

	/* CPU off, CPU suspend */
	if (lvl == (uint32_t)MPIDR_AFFLVL1) {
		target = tegra_get_afflvl1_pwr_state(states, ncpu);
	}

	/* target cluster/system state */
	return target;
}

int32_t tegra_soc_pwr_domain_power_down_wfi(const psci_power_state_t *target_state)
{
	const plat_local_state_t *pwr_domain_state =
		target_state->pwr_domain_state;
	plat_params_from_bl2_t *params_from_bl2 = bl31_get_plat_params();
	uint8_t stateid_afflvl2 = pwr_domain_state[PLAT_MAX_PWR_LVL] &
		TEGRA194_STATE_ID_MASK;
	uint64_t val;

	if (stateid_afflvl2 == PSTATE_ID_SOC_POWERDN) {
		/*
		 * The TZRAM loses power when we enter system suspend. To
		 * allow graceful exit from system suspend, we need to copy
		 * BL3-1 over to TZDRAM.
		 */
		val = params_from_bl2->tzdram_base +
		      tegra194_get_cpu_reset_handler_size();
		tegra_memcpy16(val, BL31_BASE, (uintptr_t)&__BL31_END__ -
				(uintptr_t)BL31_BASE);
	}

	return PSCI_E_SUCCESS;
}

int32_t tegra_soc_pwr_domain_on(u_register_t mpidr)
{
	uint64_t target_cpu = mpidr & MPIDR_CPU_MASK;
	uint64_t target_cluster = (mpidr & MPIDR_CLUSTER_MASK) >>
			MPIDR_AFFINITY_BITS;

	if (target_cluster > ((uint32_t)PLATFORM_CLUSTER_COUNT - 1U)) {
		ERROR("%s: unsupported CPU (0x%lx)\n", __func__ , mpidr);
		return PSCI_E_NOT_PRESENT;
	}

	/* construct the target CPU # */
	target_cpu += (target_cluster * 2U);

	(void)mce_command_handler((uint64_t)MCE_CMD_ONLINE_CORE, target_cpu, 0U, 0U);

	return PSCI_E_SUCCESS;
}

int32_t tegra_soc_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	const plat_params_from_bl2_t *params_from_bl2 = bl31_get_plat_params();
	uint8_t enable_ccplex_lock_step = params_from_bl2->enable_ccplex_lock_step;
	uint8_t stateid_afflvl2 = target_state->pwr_domain_state[PLAT_MAX_PWR_LVL];
	cpu_context_t *ctx = cm_get_context(NON_SECURE);
	uint64_t actlr_el3, actlr_el2, actlr_el1;
	int32_t ret = PSCI_E_SUCCESS;

	/*
	 * Reset power state info for CPUs when onlining, we set
	 * deepest power when offlining a core but that may not be
	 * requested by non-secure sw which controls idle states. It
	 * will re-init this info from non-secure software when the
	 * core come online.
	 */
	actlr_el1 = read_ctx_reg((get_sysregs_ctx(ctx)), (CTX_ACTLR_EL1));
	actlr_el1 &= ~DENVER_CPU_PMSTATE_MASK;
	actlr_el1 |= DENVER_CPU_PMSTATE_C1;
	write_ctx_reg((get_sysregs_ctx(ctx)), (CTX_ACTLR_EL1), (actlr_el1));

	/*
	 * Check if we are exiting from deep sleep and restore SE
	 * context if we are.
	 */
	if (stateid_afflvl2 == PSTATE_ID_SOC_POWERDN) {

#if ENABLE_STRICT_CHECKING_MODE
		/*
		 * Enable strict checking after programming the GSC for
		 * enabling TZSRAM and TZDRAM
		 */
		mce_enable_strict_checking();
#endif

		/* Init SMMU */
		tegra_smmu_init();

		/* Resume SE, RNG1 and PKA1 */
		tegra_se_resume();

		/*
		 * Program XUSB STREAMIDs
		 * ======================
		 * T19x XUSB has support for XUSB virtualization. It will
		 * have one physical function (PF) and four Virtual functions
		 * (VF)
		 *
		 * There were below two SIDs for XUSB until T186.
		 * 1) #define TEGRA_SID_XUSB_HOST    0x1bU
		 * 2) #define TEGRA_SID_XUSB_DEV    0x1cU
		 *
		 * We have below four new SIDs added for VF(s)
		 * 3) #define TEGRA_SID_XUSB_VF0    0x5dU
		 * 4) #define TEGRA_SID_XUSB_VF1    0x5eU
		 * 5) #define TEGRA_SID_XUSB_VF2    0x5fU
		 * 6) #define TEGRA_SID_XUSB_VF3    0x60U
		 *
		 * When virtualization is enabled then we have to disable SID
		 * override and program above SIDs in below newly added SID
		 * registers in XUSB PADCTL MMIO space. These registers are
		 * TZ protected and so need to be done in ATF.
		 *
		 * a) #define XUSB_PADCTL_HOST_AXI_STREAMID_PF_0 (0x136cU)
		 * b) #define XUSB_PADCTL_DEV_AXI_STREAMID_PF_0  (0x139cU)
		 * c) #define XUSB_PADCTL_HOST_AXI_STREAMID_VF_0 (0x1370U)
		 * d) #define XUSB_PADCTL_HOST_AXI_STREAMID_VF_1 (0x1374U)
		 * e) #define XUSB_PADCTL_HOST_AXI_STREAMID_VF_2 (0x1378U)
		 * f) #define XUSB_PADCTL_HOST_AXI_STREAMID_VF_3 (0x137cU)
		 *
		 * This change disables SID override and programs XUSB SIDs
		 * in above registers to support both virtualization and
		 * non-virtualization platforms
		 */
		if (tegra_platform_is_silicon() || tegra_platform_is_fpga()) {

			mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
				XUSB_PADCTL_HOST_AXI_STREAMID_PF_0, TEGRA_SID_XUSB_HOST);
			mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
				XUSB_PADCTL_HOST_AXI_STREAMID_VF_0, TEGRA_SID_XUSB_VF0);
			mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
				XUSB_PADCTL_HOST_AXI_STREAMID_VF_1, TEGRA_SID_XUSB_VF1);
			mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
				XUSB_PADCTL_HOST_AXI_STREAMID_VF_2, TEGRA_SID_XUSB_VF2);
			mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
				XUSB_PADCTL_HOST_AXI_STREAMID_VF_3, TEGRA_SID_XUSB_VF3);
			mmio_write_32(TEGRA_XUSB_PADCTL_BASE +
				XUSB_PADCTL_DEV_AXI_STREAMID_PF_0, TEGRA_SID_XUSB_DEV);
		}

		/* Enable FUSE clock before reading FUSE_SECURITY_MODE register */
		ret = tegra_bpmp_ipc_enable_clock(TEGRA194_CLK_FUSE);
		assert(ret == 0);

		/*
		 * Enable Uncore Perfmon counters only when FUSE_SECURITY_MODE_0/ODM
		 * Production fuse is not set. This fuse is customer-controlled and
		 * will be set by OEM in their product's production.
		 */
		if (mmio_read_32(TEGRA_FUSE_BASE + SECURITY_MODE)
				== ODM_PROD_FUSE_DISABLED) {
			actlr_el3 = read_actlr_el3();
			actlr_el3 |= DENVER_CPU_ENABLE_MDCR_EL3_SPME;
			write_actlr_el3(actlr_el3);
		}

		/* Disable FUSE clock before reading FUSE_SECURITY_MODE register */
		ret = tegra_bpmp_ipc_disable_clock(TEGRA194_CLK_FUSE);
		assert(ret == 0);

		/*
	 	 * Enable dual execution optimized translations for EL2 and EL3.
	 	 */
		if (enable_ccplex_lock_step != 0U) {
			actlr_el3 = read_actlr_el3();
			actlr_el3 |= DENVER_CPU_ENABLE_DUAL_EXEC_EL3;
			write_actlr_el3(actlr_el3);

			actlr_el2 = read_actlr_el2();
			actlr_el2 |= DENVER_CPU_ENABLE_DUAL_EXEC_EL2;
			write_actlr_el2(actlr_el2);
		}
	}

	return PSCI_E_SUCCESS;
}

int32_t tegra_soc_pwr_domain_off(const psci_power_state_t *target_state)
{
	uint64_t impl = (read_midr() >> MIDR_IMPL_SHIFT) & MIDR_IMPL_MASK;

	(void)target_state;

	/* Disable Denver's DCO operations */
	if (impl == DENVER_IMPL) {
		denver_disable_dco();
	}

	/* Turn off CPU */
	(void)mce_command_handler((uint64_t)MCE_CMD_ENTER_CSTATE,
			(uint64_t)TEGRA_NVG_CORE_C7, MCE_CORE_SLEEP_TIME_INFINITE, 0U);

	return PSCI_E_SUCCESS;
}

__dead2 void tegra_soc_prepare_system_off(void)
{
	/* System power off */
	mce_system_shutdown();

	wfi();

	/* wait for the system to power down */
	for (;;) {
		;
	}
}

int32_t tegra_soc_prepare_system_reset(void)
{
	/* System reboot */
	mce_system_reboot();

	return PSCI_E_SUCCESS;
}
