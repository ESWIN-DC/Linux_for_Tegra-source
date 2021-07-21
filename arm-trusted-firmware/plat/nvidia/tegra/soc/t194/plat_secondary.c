/*
 * Copyright (c) 2015-2017, ARM Limited and Contributors. All rights reserved.
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

#include <arch_helpers.h>
#include <debug.h>
#include <mmio.h>
#include <string.h>
#include <tegra194_private.h>
#include <tegra_def.h>
#include <tegra_private.h>

#define MISCREG_AA64_RST_LOW		0x2004U
#define MISCREG_AA64_RST_HIGH		0x2008U

#define CPU_RESET_MODE_AA64		1U

/*******************************************************************************
 * Setup secondary CPU vectors
 ******************************************************************************/
void plat_secondary_setup(void)
{
	uint32_t addr_low, addr_high;
	plat_params_from_bl2_t *params_from_bl2 = bl31_get_plat_params();
	uint64_t cpu_reset_handler_base, cpu_reset_handler_size;

	INFO("Setting up secondary CPU boot\n");

	/*
	 * The BL31 code resides in the TZSRAM which loses state
	 * when we enter System Suspend. Copy the wakeup trampoline
	 * code to TZDRAM to help us exit from System Suspend.
	 */
	cpu_reset_handler_base = tegra194_get_cpu_reset_handler_base();
	cpu_reset_handler_size = tegra194_get_cpu_reset_handler_size();
	tegra_memcpy16(params_from_bl2->tzdram_base,
			cpu_reset_handler_base,
			cpu_reset_handler_size);

	/* TZDRAM base will be used as the "resume" address */
	addr_low = (uint32_t)params_from_bl2->tzdram_base | CPU_RESET_MODE_AA64;
	addr_high = (uint32_t)((params_from_bl2->tzdram_base >> 32U) & 0x7ffU);

	/* write lower 32 bits first, then the upper 11 bits */
	mmio_write_32(TEGRA_MISC_BASE + MISCREG_AA64_RST_LOW, addr_low);
	mmio_write_32(TEGRA_MISC_BASE + MISCREG_AA64_RST_HIGH, addr_high);

	/* save reset vector to be used during SYSTEM_SUSPEND exit */
	mmio_write_32(TEGRA_SCRATCH_BASE + SCRATCH_RESET_VECTOR_LO,
			addr_low);
	mmio_write_32(TEGRA_SCRATCH_BASE + SCRATCH_RESET_VECTOR_HI,
			addr_high);
}
