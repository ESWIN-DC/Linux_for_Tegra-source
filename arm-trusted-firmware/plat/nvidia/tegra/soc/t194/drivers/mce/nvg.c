/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
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
#include <debug.h>
#include <denver.h>
#include <mmio.h>
#include <mce_private.h>
#include <platform_def.h>
#include <sys/errno.h>
#include <t194_nvg.h>
#include <tegra_private.h>

#define	ID_AFR0_EL1_CACHE_OPS_SHIFT	U(12)
#define	ID_AFR0_EL1_CACHE_OPS_MASK	U(0xF)
/*
 * Reports the major and minor version of this interface.
 *
 * NVGDATA[0:31]: SW(R) Minor Version
 * NVGDATA[32:63]: SW(R) Major Version
 */
uint64_t nvg_get_version(void)
{
	nvg_set_request((uint64_t)TEGRA_NVG_CHANNEL_VERSION);

	return (uint64_t)nvg_get_result();
}

/*
 * Set the expected wake time in TSC ticks for the next low-power state the
 * core enters.
 *
 * NVGDATA[0:31]: SW(RW), WAKE_TIME
 */
void nvg_set_wake_time(uint32_t wake_time)
{
	/* time (TSC ticks) until the core is expected to get a wake event */
	nvg_set_request_data((uint64_t)TEGRA_NVG_CHANNEL_WAKE_TIME, (uint64_t)wake_time);
}

/*
 * This request allows updating of CLUSTER_CSTATE, CCPLEX_CSTATE and
 * SYSTEM_CSTATE values.
 *
 * NVGDATA[0:2]: SW(RW), CLUSTER_CSTATE
 * NVGDATA[7]: SW(W), update cluster flag
 * NVGDATA[8:10]: SW(RW), CG_CSTATE
 * NVGDATA[15]: SW(W), update ccplex flag
 * NVGDATA[16:19]: SW(RW), SYSTEM_CSTATE
 * NVGDATA[23]: SW(W), update system flag
 * NVGDATA[31]: SW(W), update wake mask flag
 * NVGDATA[32:63]: SW(RW), WAKE_MASK
 */
void nvg_update_cstate_info(uint32_t cluster, uint32_t ccplex,
		uint32_t system, uint32_t wake_mask, uint8_t update_wake_mask)
{
	uint64_t val = 0;

	/* update CLUSTER_CSTATE? */
	if (cluster != 0U) {
		val |= ((uint64_t)cluster & CLUSTER_CSTATE_MASK) |
				CLUSTER_CSTATE_UPDATE_BIT;
	}

	/* update CCPLEX_CSTATE? */
	if (ccplex != 0U) {
		val |= (((uint64_t)ccplex & CCPLEX_CSTATE_MASK) << CCPLEX_CSTATE_SHIFT) |
				CCPLEX_CSTATE_UPDATE_BIT;
	}

	/* update SYSTEM_CSTATE? */
	if (system != 0U) {
		val |= (((uint64_t)system & SYSTEM_CSTATE_MASK) << SYSTEM_CSTATE_SHIFT) |
				SYSTEM_CSTATE_UPDATE_BIT;
	}

	/* update wake mask value? */
	if (update_wake_mask != 0U) {
		val |= CSTATE_WAKE_MASK_UPDATE_BIT;
	}

	/* set the wake mask */
	val |= ((uint64_t)wake_mask & CSTATE_WAKE_MASK_CLEAR) << CSTATE_WAKE_MASK_SHIFT;

	/* set the updated cstate info */
	nvg_set_request_data((uint64_t)TEGRA_NVG_CHANNEL_CSTATE_INFO, val);
}

/*
 * Return a non-zero value if the CCPLEX is able to enter SC7
 *
 * NVGDATA[0]: SW(R), Is allowed result
 */
int32_t nvg_is_sc7_allowed(void)
{
	/* issue command to check if SC7 is allowed */
	nvg_set_request((uint64_t)TEGRA_NVG_CHANNEL_IS_SC7_ALLOWED);

	/* 1 = SC7 allowed, 0 = SC7 not allowed */
	return (int32_t)nvg_get_result();
}

/*
 * Wake an offlined logical core. Note that a core is offlined by entering
 * a C-state where the WAKE_MASK is all 0.
 *
 * NVGDATA[0:3]: SW(W) logical core to online
 */
int32_t nvg_online_core(uint32_t core)
{
	int32_t ret = 0;

	/* sanity check the core ID value */
	if (core > (uint32_t)PLATFORM_CORE_COUNT) {
		ERROR("%s: unknown core id (%d)\n", __func__, core);
		ret = -EINVAL;
	} else {
		/* get a core online */
		nvg_set_request_data((uint64_t)TEGRA_NVG_CHANNEL_ONLINE_CORE,
					(uint64_t)core & MCE_CORE_ID_MASK);
	}

	return ret;
}

/*
 * MC GSC (General Security Carveout) register values are expected to be
 * changed by TrustZone ARM code after boot.
 *
 * NVGDATA[0:15] SW(R) GSC enun
 */
int32_t nvg_update_ccplex_gsc(uint32_t gsc_idx)
{
	int32_t ret = 0;

	/* sanity check GSC ID */
	if (gsc_idx > (uint32_t)TEGRA_NVG_CHANNEL_UPDATE_GSC_VPR) {
		ERROR("%s: unknown gsc_idx (%d)\n", __func__, gsc_idx);
		ret = -EINVAL;
	} else {
		nvg_set_request_data((uint64_t)TEGRA_NVG_CHANNEL_UPDATE_CCPLEX_GSC,
								(uint64_t)gsc_idx);
	}

	return ret;
}

/*
 * Cache clean and invalidate, clear TR-bit operation for all CCPLEX caches.
 */
int32_t nvg_roc_clean_cache_trbits(void)
{
	int32_t ret = 0;

	/* check if cache flush through mts is supported */
	if (((read_id_afr0_el1() >> ID_AFR0_EL1_CACHE_OPS_SHIFT) &
			ID_AFR0_EL1_CACHE_OPS_MASK) == 1U) {
		if (nvg_cache_inval_all() == 0U) {
			ERROR("%s: failed\n", __func__);
			ret = -EINVAL;
		}
	} else {
		ret = -EINVAL;
	}
	return ret;
}

/*
 * Set the power state for a core
 */
int32_t nvg_enter_cstate(uint32_t state, uint32_t wake_time)
{
	int32_t ret = 0;
	uint64_t val = 0ULL;

	/* check for allowed power state */
	if ((state != (uint32_t)TEGRA_NVG_CORE_C0) &&
		(state != (uint32_t)TEGRA_NVG_CORE_C1) &&
	    (state != (uint32_t)TEGRA_NVG_CORE_C6) &&
		(state != (uint32_t)TEGRA_NVG_CORE_C7))
	{
		ERROR("%s: unknown cstate (%d)\n", __func__, state);
		ret = -EINVAL;
	} else {
		/* time (TSC ticks) until the core is expected to get a wake event */
		nvg_set_wake_time(wake_time);

		/* set the core cstate */
		val = read_actlr_el1() & ~ACTLR_EL1_PMSTATE_MASK;
		write_actlr_el1(val | (uint64_t)state);
	}

	return ret;
}

#if ENABLE_STRICT_CHECKING_MODE
/*
 * Enable strict checking mode
 *
 * NVGDATA[3] strict_check ON + lock
 */
void nvg_enable_strict_checking_mode(void)
{
	uint64_t params = (uint64_t)(STRICT_CHECKING_ENABLED_SET |
				     STRICT_CHECKING_LOCKED_SET);

	nvg_set_request_data((uint64_t)TEGRA_NVG_CHANNEL_SECURITY_CONFIG, params);
}
#endif

/*
 * Request a reboot
 *
 * NVGDATA[0]: reboot command
 */
void nvg_system_reboot(void)
{
	/* issue command for reboot */
	nvg_set_request_data((uint64_t)TEGRA_NVG_CHANNEL_SHUTDOWN,
			     (uint64_t)TEGRA_NVG_REBOOT);
}

/*
 * Request a shutdown
 *
 * NVGDATA[0]: shutdown command
 */
void nvg_system_shutdown(void)
{
	/* issue command for shutdown */
	nvg_set_request_data((uint64_t)TEGRA_NVG_CHANNEL_SHUTDOWN,
			     (uint64_t)TEGRA_NVG_SHUTDOWN);
}
