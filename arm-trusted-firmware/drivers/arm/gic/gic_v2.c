/*
 * Copyright (c) 2013-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <assert.h>
#include <gic_v2.h>
#include <interrupt_mgmt.h>
#include <mmio.h>
#include <stdbool.h>

/*******************************************************************************
 * GIC Distributor interface accessors for reading entire registers
 ******************************************************************************/

uint32_t gicd_read_igroupr(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> IGROUPR_SHIFT;
	return mmio_read_32(base + GICD_IGROUPR + (n << 2));
}

uint32_t gicd_read_isenabler(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> ISENABLER_SHIFT;
	return mmio_read_32(base + GICD_ISENABLER + (n << 2));
}

uint32_t gicd_read_icenabler(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> ICENABLER_SHIFT;
	return mmio_read_32(base + GICD_ICENABLER + (n << 2));
}

uint32_t gicd_read_ispendr(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> ISPENDR_SHIFT;
	return mmio_read_32(base + GICD_ISPENDR + (n << 2));
}

uint32_t gicd_read_icpendr(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> ICPENDR_SHIFT;
	return mmio_read_32(base + GICD_ICPENDR + (n << 2));
}

uint32_t gicd_read_isactiver(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> ISACTIVER_SHIFT;
	return mmio_read_32(base + GICD_ISACTIVER + (n << 2));
}

uint32_t gicd_read_icactiver(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> ICACTIVER_SHIFT;
	return mmio_read_32(base + GICD_ICACTIVER + (n << 2));
}

uint32_t gicd_read_ipriorityr(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> IPRIORITYR_SHIFT;
	return mmio_read_32(base + GICD_IPRIORITYR + (n << 2));
}

uint32_t gicd_read_itargetsr(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> ITARGETSR_SHIFT;
	return mmio_read_32(base + GICD_ITARGETSR + (n << 2));
}

uint32_t gicd_read_icfgr(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> ICFGR_SHIFT;
	return mmio_read_32(base + GICD_ICFGR + (n << 2));
}

uint32_t gicd_read_cpendsgir(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> CPENDSGIR_SHIFT;
	return mmio_read_32(base + GICD_CPENDSGIR + (n << 2));
}

uint32_t gicd_read_spendsgir(uintptr_t base, uint32_t id)
{
	uint64_t n = (uint64_t)id >> SPENDSGIR_SHIFT;
	return mmio_read_32(base + GICD_SPENDSGIR + (n << 2));
}

/*******************************************************************************
 * GIC Distributor interface accessors for writing entire registers
 ******************************************************************************/

void gicd_write_igroupr(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> IGROUPR_SHIFT;
	mmio_write_32(base + GICD_IGROUPR + (n << 2), val);
}

void gicd_write_isenabler(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> ISENABLER_SHIFT;
	mmio_write_32(base + GICD_ISENABLER + (n << 2), val);
}

void gicd_write_icenabler(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> ICENABLER_SHIFT;
	mmio_write_32(base + GICD_ICENABLER + (n << 2), val);
}

void gicd_write_ispendr(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> ISPENDR_SHIFT;
	mmio_write_32(base + GICD_ISPENDR + (n << 2), val);
}

void gicd_write_icpendr(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> ICPENDR_SHIFT;
	mmio_write_32(base + GICD_ICPENDR + (n << 2), val);
}

void gicd_write_isactiver(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> ISACTIVER_SHIFT;
	mmio_write_32(base + GICD_ISACTIVER + (n << 2), val);
}

void gicd_write_icactiver(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> ICACTIVER_SHIFT;
	mmio_write_32(base + GICD_ICACTIVER + (n << 2), val);
}

void gicd_write_ipriorityr(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> IPRIORITYR_SHIFT;
	mmio_write_32(base + GICD_IPRIORITYR + (n << 2), val);
}

void gicd_write_itargetsr(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> ITARGETSR_SHIFT;
	mmio_write_32(base + GICD_ITARGETSR + (n << 2), val);
}

void gicd_write_icfgr(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> ICFGR_SHIFT;
	mmio_write_32(base + GICD_ICFGR + (n << 2), val);
}

void gicd_write_cpendsgir(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> CPENDSGIR_SHIFT;
	mmio_write_32(base + GICD_CPENDSGIR + (n << 2), val);
}

void gicd_write_spendsgir(uintptr_t base, uint32_t id, uint32_t val)
{
	uint64_t n = (uint64_t)id >> SPENDSGIR_SHIFT;
	mmio_write_32(base + GICD_SPENDSGIR + (n << 2), val);
}

/*******************************************************************************
 * GIC Distributor interface accessors for individual interrupt manipulation
 ******************************************************************************/
uint32_t gicd_get_igroupr(uintptr_t base, uint32_t id)
{
	uint32_t bit_num = id & ((1U << IGROUPR_SHIFT) - 1U);
	uint32_t reg_val = gicd_read_igroupr(base, id);

	return (reg_val >> bit_num) & 0x1U;
}

void gicd_set_igroupr(uintptr_t base, uint32_t id)
{
	uint32_t bit_num = id & ((1U << IGROUPR_SHIFT) - 1U);
	uint32_t reg_val = gicd_read_igroupr(base, id);

	gicd_write_igroupr(base, id, reg_val | ((uint32_t)1 << bit_num));
}

void gicd_clr_igroupr(uintptr_t base, uint32_t id)
{
	uint32_t bit_num = id & ((1U << IGROUPR_SHIFT) - 1U);
	uint32_t reg_val = gicd_read_igroupr(base, id);

	gicd_write_igroupr(base, id, reg_val & ~(1U << bit_num));
}

void gicd_set_isenabler(uintptr_t base, uint32_t id)
{
	uint32_t bit_num = id & ((1U << ISENABLER_SHIFT) - 1U);

	gicd_write_isenabler(base, id, ((uint32_t)1 << bit_num));
}

void gicd_set_icenabler(uintptr_t base, uint32_t id)
{
	uint32_t bit_num = id & ((1U << ICENABLER_SHIFT) - 1U);

	gicd_write_icenabler(base, id, ((uint32_t)1 << bit_num));
}

void gicd_set_ispendr(uintptr_t base, uint32_t id)
{
	uint32_t bit_num = id & ((1U << ISPENDR_SHIFT) - 1U);

	gicd_write_ispendr(base, id, ((uint32_t)1 << bit_num));
}

void gicd_set_icpendr(uintptr_t base, uint32_t id)
{
	uint32_t bit_num = id & ((1U << ICPENDR_SHIFT) - 1U);

	gicd_write_icpendr(base, id, ((uint32_t)1 << bit_num));
}

void gicd_set_isactiver(uintptr_t base, uint32_t id)
{
	uint32_t bit_num = id & ((1U << ISACTIVER_SHIFT) - 1U);

	gicd_write_isactiver(base, id, ((uint32_t)1 << bit_num));
}

void gicd_set_icactiver(uintptr_t base, uint32_t id)
{
	uint32_t bit_num = id & ((1U << ICACTIVER_SHIFT) - 1U);

	gicd_write_icactiver(base, id, ((uint32_t)1 << bit_num));
}

/*
 * Make sure that the interrupt's group is set before expecting
 * this function to do its job correctly.
 */
void gicd_set_ipriorityr(uintptr_t base, uint32_t id, uint32_t pri)
{
	/*
	 * Enforce ARM recommendation to manage priority values such
	 * that group1 interrupts always have a lower priority than
	 * group0 interrupts.
	 * Note, lower numerical values are higher priorities so the comparison
	 * checks below are reversed from what might be expected.
	 */
	assert(((gicd_get_igroupr(base, id) == GRP1) ?
		((pri >= GIC_HIGHEST_NS_PRIORITY) &&
			(pri <= GIC_LOWEST_NS_PRIORITY)) :
		(pri <= GIC_LOWEST_SEC_PRIORITY)) != false);

	mmio_write_8(base + GICD_IPRIORITYR + id, (uint8_t)(pri & GIC_PRI_MASK));
}

void gicd_set_itargetsr(uintptr_t base, uint32_t id, uint32_t target)
{
	mmio_write_8(base + GICD_ITARGETSR + id, (uint8_t)(target & GIC_TARGET_CPU_MASK));
}

/*******************************************************************************
 * This function allows the interrupt management framework to determine (through
 * the platform) which interrupt line (IRQ/FIQ) to use for an interrupt type to
 * route it to EL3. The interrupt line is represented as the bit position of the
 * IRQ or FIQ bit in the SCR_EL3.
 ******************************************************************************/
uint32_t gicv2_interrupt_type_to_line(uint32_t cpuif_base, uint32_t type)
{
	uint32_t gicc_ctlr;

	/* Non-secure interrupts are signalled on the IRQ line always */
	if (type == INTR_TYPE_NS) {
		return __builtin_ctz(SCR_IRQ_BIT);
	}

	/*
	 * Secure interrupts are signalled using the IRQ line if the FIQ_EN
	 * bit is not set else they are signalled using the FIQ line.
	 */
	gicc_ctlr = gicc_read_ctlr(cpuif_base);
	if ((gicc_ctlr & FIQ_EN) != 0U) {
		return __builtin_ctz(SCR_FIQ_BIT);
	} else {
		return __builtin_ctz(SCR_IRQ_BIT);
	}
}
