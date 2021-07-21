/*
 * Copyright (c) 2013-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSCI_PRIVATE_H
#define PSCI_PRIVATE_H

#include <arch.h>
#include <bakery_lock.h>
#include <bl_common.h>
#include <cpu_data.h>
#include <psci.h>
#include <spinlock.h>
#include <stdbool.h>

#if HW_ASSISTED_COHERENCY

/*
 * On systems with hardware-assisted coherency, make PSCI cache operations NOP,
 * as PSCI participants are cache-coherent, and there's no need for explicit
 * cache maintenance operations or barriers to coordinate their state.
 */
#define psci_flush_dcache_range(addr, size)
#define psci_flush_cpu_data(member)
#define psci_inv_cpu_data(member)

#define psci_dsbish()

/*
 * On systems where participant CPUs are cache-coherent, we can use spinlocks
 * instead of bakery locks.
 */
#define DEFINE_PSCI_LOCK(_name)		spinlock_t _name
#define DECLARE_PSCI_LOCK(_name)	extern DEFINE_PSCI_LOCK(_name)

#define psci_lock_get(non_cpu_pd_node)				\
	spin_lock(&psci_locks[(non_cpu_pd_node)->lock_index])
#define psci_lock_release(non_cpu_pd_node)			\
	spin_unlock(&psci_locks[(non_cpu_pd_node)->lock_index])

#else

/*
 * If not all PSCI participants are cache-coherent, perform cache maintenance
 * and issue barriers wherever required to coordinate state.
 */
#define psci_flush_dcache_range(addr, size)	flush_dcache_range(addr, size)
#define psci_flush_cpu_data(member)		flush_cpu_data(member)
#define psci_inv_cpu_data(member)		inv_cpu_data(member)

#define psci_dsbish()				dsbish()

/*
 * Use bakery locks for state coordination as not all PSCI participants are
 * cache coherent.
 */
#define DEFINE_PSCI_LOCK(_name)		DEFINE_BAKERY_LOCK(_name)
#define DECLARE_PSCI_LOCK(_name)	DECLARE_BAKERY_LOCK(_name)

#define psci_lock_get(non_cpu_pd_node)				\
	bakery_lock_get(&psci_locks[(non_cpu_pd_node)->lock_index])
#define psci_lock_release(non_cpu_pd_node)			\
	bakery_lock_release(&psci_locks[(non_cpu_pd_node)->lock_index])

#endif

#define psci_lock_init(non_cpu_pd_node, idx)			\
	((non_cpu_pd_node)[(idx)].lock_index = (idx))

/*
 * The PSCI capability which are provided by the generic code but does not
 * depend on the platform or spd capabilities.
 */
#define PSCI_GENERIC_CAP	\
			(define_psci_cap(PSCI_VERSION) |		\
			define_psci_cap(PSCI_AFFINITY_INFO_AARCH64) |	\
			define_psci_cap(PSCI_FEATURES))

/*
 * The PSCI capabilities mask for 64 bit functions.
 */
#define PSCI_CAP_64BIT_MASK	\
			(define_psci_cap(PSCI_CPU_SUSPEND_AARCH64) |	\
			define_psci_cap(PSCI_CPU_ON_AARCH64) |		\
			define_psci_cap(PSCI_AFFINITY_INFO_AARCH64) |	\
			define_psci_cap(PSCI_MIG_AARCH64) |		\
			define_psci_cap(PSCI_MIG_INFO_UP_CPU_AARCH64) |	\
			define_psci_cap(PSCI_NODE_HW_STATE_AARCH64) |	\
			define_psci_cap(PSCI_SYSTEM_SUSPEND_AARCH64) |	\
			define_psci_cap(PSCI_STAT_RESIDENCY_AARCH64) |	\
			define_psci_cap(PSCI_STAT_COUNT_AARCH64))

/*
 * Helper macros to get/set the fields of PSCI per-cpu data.
 */
#define psci_set_aff_info_state(aff_state) \
		set_cpu_data(psci_svc_cpu_data.aff_info_state, (aff_state))
#define psci_get_aff_info_state() \
		get_cpu_data(psci_svc_cpu_data.aff_info_state)
#define psci_get_aff_info_state_by_idx(idx) \
		get_cpu_data_by_index(idx, psci_svc_cpu_data.aff_info_state)
#define psci_set_aff_info_state_by_idx(idx, aff_state) \
		set_cpu_data_by_index(idx, psci_svc_cpu_data.aff_info_state,\
					(aff_state))
#define psci_get_suspend_pwrlvl() \
		get_cpu_data(psci_svc_cpu_data.target_pwrlvl)
#define psci_set_suspend_pwrlvl(target_level) \
		set_cpu_data(psci_svc_cpu_data.target_pwrlvl, (target_level))
#define psci_set_cpu_local_state(state) \
		set_cpu_data(psci_svc_cpu_data.local_state, (state))
#define psci_get_cpu_local_state() \
		get_cpu_data(psci_svc_cpu_data.local_state)
#define psci_get_cpu_local_state_by_idx(idx) \
		get_cpu_data_by_index(idx, psci_svc_cpu_data.local_state)

/*
 * Helper macros for the CPU level spinlocks
 */
#define psci_spin_lock_cpu(idx)	spin_lock(&psci_cpu_pd_nodes[idx].cpu_lock)
#define psci_spin_unlock_cpu(idx) spin_unlock(&psci_cpu_pd_nodes[idx].cpu_lock)

/* Helper macro to identify a CPU standby request in PSCI Suspend call */
#define is_cpu_standby_req(is_power_down_state_m, retn_lvl) \
		(((((is_power_down_state_m) == 0U)) && ((retn_lvl) == 0U)) ? true : false)

/*******************************************************************************
 * The following two data structures implement the power domain tree. The tree
 * is used to track the state of all the nodes i.e. power domain instances
 * described by the platform. The tree consists of nodes that describe CPU power
 * domains i.e. leaf nodes and all other power domains which are parents of a
 * CPU power domain i.e. non-leaf nodes.
 ******************************************************************************/
typedef struct non_cpu_pwr_domain_node {
	/*
	 * Index of the first CPU power domain node level 0 which has this node
	 * as its parent.
	 */
	uint32_t cpu_start_idx;

	/*
	 * Number of CPU power domains which are siblings of the domain indexed
	 * by 'cpu_start_idx' i.e. all the domains in the range 'cpu_start_idx
	 * -> cpu_start_idx + ncpus' have this node as their parent.
	 */
	uint32_t ncpus;

	/*
	 * Index of the parent power domain node.
	 * TODO: Figure out whether to whether using pointer is more efficient.
	 */
	uint32_t parent_node;

	plat_local_state_t local_state;

	unsigned char level;

	/* For indexing the psci_lock array*/
	unsigned char lock_index;
} non_cpu_pd_node_t;

typedef struct cpu_pwr_domain_node {
	u_register_t mpidr;

	/*
	 * Index of the parent power domain node.
	 * TODO: Figure out whether to whether using pointer is more efficient.
	 */
	uint32_t parent_node;

	/*
	 * A CPU power domain does not require state coordination like its
	 * parent power domains. Hence this node does not include a bakery
	 * lock. A spinlock is required by the CPU_ON handler to prevent a race
	 * when multiple CPUs try to turn ON the same target CPU.
	 */
	spinlock_t cpu_lock;
} cpu_pd_node_t;

/*******************************************************************************
 * Data prototypes
 ******************************************************************************/
extern const plat_psci_ops_t *psci_plat_pm_ops;
extern non_cpu_pd_node_t psci_non_cpu_pd_nodes[PSCI_NUM_NON_CPU_PWR_DOMAINS];
extern cpu_pd_node_t psci_cpu_pd_nodes[PLATFORM_CORE_COUNT];
extern uint32_t psci_caps;

/* One lock is required per non-CPU power domain node */
DECLARE_PSCI_LOCK(psci_locks[PSCI_NUM_NON_CPU_PWR_DOMAINS]);

/*******************************************************************************
 * SPD's power management hooks registered with PSCI
 ******************************************************************************/
extern const spd_pm_ops_t *psci_spd_pm;

/*******************************************************************************
 * Function prototypes
 ******************************************************************************/
/* Private exported functions from psci_common.c */
int32_t psci_validate_power_state(uint32_t power_state,
			      psci_power_state_t *state_info);
void psci_query_sys_suspend_pwrstate(psci_power_state_t *state_info);
int32_t psci_validate_mpidr(u_register_t mpidr);
void psci_init_req_local_pwr_states(void);
void psci_get_target_local_pwr_states(uint32_t end_pwrlvl,
				      psci_power_state_t *target_state);
int32_t psci_validate_entry_point(entry_point_info_t *ep,
			uintptr_t entrypoint, u_register_t context_id);
void psci_get_parent_pwr_domain_nodes(uint32_t cpu_idx,
				      uint32_t end_lvl,
				      uint32_t node_index[]);
void psci_do_state_coordination(uint32_t end_pwrlvl,
				psci_power_state_t *state_info);
void psci_acquire_pwr_domain_locks(uint32_t end_pwrlvl,
				   uint32_t cpu_idx);
void psci_release_pwr_domain_locks(uint32_t end_pwrlvl,
				   uint32_t cpu_idx);
int32_t psci_validate_suspend_req(const psci_power_state_t *state_info,
			      uint32_t is_power_down_state);
uint32_t psci_find_max_off_lvl(const psci_power_state_t *state_info);
uint32_t psci_find_target_suspend_lvl(const psci_power_state_t *state_info);
void psci_set_pwr_domains_to_run(uint32_t end_pwrlvl);
void psci_print_power_domain_map(void);
uint32_t psci_is_last_on_cpu(void);
int32_t psci_spd_migrate_info(u_register_t *mpidr);
void psci_do_pwrdown_sequence(uint32_t power_level);

#if HW_ASSISTED_COHERENCY
/*
 * CPU power down is directly called only when HW_ASSISTED_COHERENCY is
 * available. Otherwise, this needs post-call stack maintenance, which is
 * handled in assembly.
 */
void prepare_cpu_pwr_dwn(uint32_t power_level);
#endif

/* Private exported functions from psci_on.c */
int32_t psci_cpu_on_start(u_register_t target_cpu,
		      entry_point_info_t *ep);

void psci_cpu_on_finish(uint32_t cpu_idx,
			psci_power_state_t *state_info);

/* Private exported functions from psci_off.c */
int32_t psci_do_cpu_off(uint32_t end_pwrlvl);

/* Private exported functions from psci_suspend.c */
void psci_cpu_suspend_start(entry_point_info_t *ep,
			uint32_t end_pwrlvl,
			psci_power_state_t *state_info,
			uint32_t is_power_down_state);

void psci_cpu_suspend_finish(uint32_t cpu_idx,
			psci_power_state_t *state_info);

/* Private exported functions from psci_helpers.S */
void psci_do_pwrdown_cache_maintenance(uint32_t pwr_level);
void psci_do_pwrup_cache_maintenance(void);

/* Private exported functions from psci_system_off.c */
void __dead2 psci_system_off(void);
void __dead2 psci_system_reset(void);

#if defined(ENABLE_PSCI_STAT) && (ENABLE_PSCI_STAT == 1)
/* Private exported functions from psci_stat.c */
void psci_stats_update_pwr_down(uint32_t end_pwrlvl,
			const psci_power_state_t *state_info);
void psci_stats_update_pwr_up(uint32_t end_pwrlvl,
			const psci_power_state_t *state_info);
u_register_t psci_stat_residency(u_register_t target_cpu,
			uint32_t power_state);
u_register_t psci_stat_count(u_register_t target_cpu,
			uint32_t power_state);
#endif
#endif /* PSCI_PRIVATE_H */
