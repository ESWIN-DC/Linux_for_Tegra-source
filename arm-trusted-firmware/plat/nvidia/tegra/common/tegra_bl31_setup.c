/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <bl31.h>
#include <bl_common.h>
#include <console.h>
#include <cortex_a57.h>
#include <cortex_a53.h>
#include <debug.h>
#include <denver.h>
#include <errno.h>
#include <memctrl.h>
#include <mmio.h>
#include <platform.h>
#include <platform_def.h>
#include <profiler.h>
#include <stddef.h>
#include <string.h>
#include <tegra_def.h>
#include <tegra_platform.h>
#include <tegra_private.h>

static entry_point_info_t bl33_image_ep_info, bl32_image_ep_info;
static plat_params_from_bl2_t plat_bl31_params_from_bl2 = {
	.tzdram_size = TZDRAM_SIZE
};

/*******************************************************************************
 * Common handler to enable MMU
 ******************************************************************************/
void bl31_plat_enable_mmu(uint32_t flags)
{
	enable_mmu_el3(flags);
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for
 * security state specified. BL33 corresponds to the non-secure image type
 * while BL32 corresponds to the secure image type.
 ******************************************************************************/
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *ep = NULL;

	if (type == NON_SECURE) {
		ep = &bl33_image_ep_info;
	} else {
		/* return BL32 entry point info if it is valid */
		if ((type == SECURE) && (bl32_image_ep_info.pc != 0U)) {
			ep = &bl32_image_ep_info;
		}
	}

	return ep;
}

/*******************************************************************************
 * Return a pointer to the 'plat_params_from_bl2_t' structure. The BL2 image
 * passes this platform specific information.
 ******************************************************************************/
plat_params_from_bl2_t *bl31_get_plat_params(void)
{
	return &plat_bl31_params_from_bl2;
}

/*******************************************************************************
 * Perform any BL31 specific platform actions. Populate the BL33 and BL32 image
 * info.
 ******************************************************************************/
void bl31_early_platform_setup(bl31_params_t *from_bl2,
				void *plat_params_from_bl2)
{
	bl31_params_t **from_bl2_ptr = &from_bl2;
	plat_params_from_bl2_t *plat_params =
		(plat_params_from_bl2_t *)plat_params_from_bl2;
#if LOG_LEVEL >= LOG_LEVEL_INFO
	uint64_t impl = (read_midr() >> MIDR_IMPL_SHIFT) & MIDR_IMPL_MASK;
#endif
	uint64_t console_base;
	uint32_t console_clock;
	int32_t ret;

	/*
	 * For RESET_TO_BL31 systems, BL31 is the first bootloader to run so
	 * there's no argument to relay from a previous bootloader. Platforms
	 * might use custom ways to get arguments.
	 */

	if (from_bl2 == NULL) {
		*from_bl2_ptr = plat_get_bl31_params();
	}
	if (plat_params == NULL) {
		plat_params = plat_get_bl31_plat_params();
	}

	/*
	 * Copy BL3-3, BL3-2 entry point information.
	 * They are stored in Secure RAM, in BL2's address space.
	 */
	assert(from_bl2 != NULL);
	assert(from_bl2->bl33_ep_info != NULL);
	bl33_image_ep_info = *from_bl2->bl33_ep_info;

	if (from_bl2->bl32_ep_info != NULL) {
		bl32_image_ep_info = *from_bl2->bl32_ep_info;
	}

	/*
	 * Parse platform specific parameters
	 */
	assert(plat_params != NULL);
	plat_bl31_params_from_bl2.tzdram_base = plat_params->tzdram_base;
	plat_bl31_params_from_bl2.tzdram_size = plat_params->tzdram_size;
	plat_bl31_params_from_bl2.uart_id = plat_params->uart_id;
	plat_bl31_params_from_bl2.l2_ecc_parity_prot_dis = plat_params->l2_ecc_parity_prot_dis;
	plat_bl31_params_from_bl2.sc7entry_fw_size = plat_params->sc7entry_fw_size;
	plat_bl31_params_from_bl2.sc7entry_fw_base = plat_params->sc7entry_fw_base;

	/*
	 * It is very important that we run either from TZDRAM or TZSRAM base.
	 * Add an explicit check here.
	 */
	if ((plat_bl31_params_from_bl2.tzdram_base != (uint64_t)BL31_BASE) &&
	    (TEGRA_TZRAM_BASE != (uint64_t)BL31_BASE)) {
		panic();
	}

	/*
	 * The previous bootloader passes the base address of the shared memory
	 * location to store the boot profiler logs. Sanity check the
	 * address and initilise the profiler library, if it looks ok.
	 */
	ret = bl31_check_ns_address(plat_params->boot_profiler_shmem_base,
			PROFILER_SIZE_BYTES);
	if (ret == (int32_t)0) {

		/* store the membase for the profiler lib */
		plat_bl31_params_from_bl2.boot_profiler_shmem_base =
			plat_params->boot_profiler_shmem_base;

		/* initialise the profiler library */
		boot_profiler_init(plat_params->boot_profiler_shmem_base,
				   TEGRA_TMRUS_BASE);
	}

	/*
	 * Add timestamp for platform early setup entry.
	 */
	boot_profiler_add_record("[TF] early setup entry");

	/*
	 * Initialize delay timer
	 */
	tegra_delay_timer_init();

	/*
	 * Reference clock used by the FPGAs is a lot slower.
	 */
	if (tegra_platform_is_fpga()) {
		console_clock = TEGRA_BOOT_UART_CLK_13_MHZ;
	} else {
		console_clock = TEGRA_BOOT_UART_CLK_408_MHZ;
	}

	/*
	 * Get the base address of the UART controller to be used for the
	 * console and configure the UART port to be used as the console
	 */
	console_base = plat_get_console_from_id(plat_params->uart_id);
	tegra_set_console_base(console_base);
	(void)console_init(console_base, console_clock, TEGRA_CONSOLE_BAUDRATE);

	/* Early platform setup for Tegra SoCs */
	plat_early_platform_setup();

	/*
	 * Do initial security configuration to allow DRAM/device access.
	 */
	tegra_memctrl_tzdram_setup(plat_bl31_params_from_bl2.tzdram_base,
			(uint32_t)plat_bl31_params_from_bl2.tzdram_size);

#if RELOCATE_BL32_IMAGE
	/*
	 * The previous bootloader might not have placed the BL32 image
	 * inside the TZDRAM. Platform handler to allow relocation of BL32
	 * image to TZDRAM memory. This behavior might change per platform.
	 */
	plat_relocate_bl32_image(from_bl2->bl32_image_info);
#endif

	/*
	 * Add timestamp for platform early setup exit.
	 */
	boot_profiler_add_record("[TF] early setup exit");

	INFO("BL3-1: Boot CPU: %s Processor [%lx]\n", (impl == DENVER_IMPL) ?
		"Denver" : "ARM", read_mpidr());
}

/*******************************************************************************
 * Initialize the gic, configure the SCR.
 ******************************************************************************/
void bl31_platform_setup(void)
{
	/*
	 * Add timestamp for platform setup entry.
	 */
	boot_profiler_add_record("[TF] plat setup entry");

	/* Initialize the gic cpu and distributor interfaces */
	plat_gic_setup();

	/*
	 * Setup secondary CPU POR infrastructure.
	 */
	plat_secondary_setup();

	/*
	 * Initial Memory Controller configuration.
	 */
	tegra_memctrl_setup();

	/*
	 * Set up the TZRAM memory aperture to allow only secure world
	 * access
	 */
	tegra_memctrl_tzram_setup(TEGRA_TZRAM_BASE, TEGRA_TZRAM_SIZE);

	/*
	 * Late setup handler to allow platforms to performs additional
	 * functionality.
	 * This handler gets called with MMU enabled.
	 */
	plat_late_platform_setup();

	/*
	 * Add timestamp for platform setup exit.
	 */
	boot_profiler_add_record("[TF] plat setup exit");

	INFO("BL3-1: Tegra platform setup complete\n");
}

/*******************************************************************************
 * Perform any BL3-1 platform runtime setup prior to BL3-1 cold boot exit
 ******************************************************************************/
void bl31_plat_runtime_setup(void)
{
	/*
	 * During cold boot, it is observed that the arbitration
	 * bit is set in the Memory controller leading to false
	 * error interrupts in the non-secure world. To avoid
	 * this, clean the interrupt status register before
	 * booting into the non-secure world
	 */
	tegra_memctrl_clear_pending_interrupts();

	/*
	 * During boot, USB3 and flash media (SDMMC/SATA) devices need
	 * access to IRAM. Because these clients connect to the MC and
	 * do not have a direct path to the IRAM, the MC implements AHB
	 * redirection during boot to allow path to IRAM. In this mode
	 * accesses to a programmed memory address aperture are directed
	 * to the AHB bus, allowing access to the IRAM. This mode must be
	 * disabled before we jump to the non-secure world.
	 */
	tegra_memctrl_disable_ahb_redirection();

	/*
	 * Add final timestamp before exiting BL31.
	 */
	boot_profiler_add_record("[TF] bl31 exit");
	boot_profiler_deinit();
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this only intializes the mmu in a quick and dirty way.
 ******************************************************************************/
void bl31_plat_arch_setup(void)
{
	uint64_t local_rw_start = tegra_get_bl31_rw_start();
	uint64_t local_rw_end = tegra_get_bl31_rw_end();
	uint64_t local_rodata_start = tegra_get_bl31_rodata_start();
	uint64_t local_rodata_end = tegra_get_bl31_rodata_end();
	uint64_t rw_size, rodata_size;
	uint64_t code_base = tegra_get_bl31_text_start();
	uint64_t code_size = tegra_get_bl31_text_end() - code_base;
	const mmap_region_t *plat_mmio_map = NULL;
#if USE_COHERENT_MEM
	uint32_t coh_start, coh_size;
#endif

	const plat_params_from_bl2_t *params_from_bl2 = bl31_get_plat_params();

	/*
	 * Add timestamp for arch setup entry.
	 */
	boot_profiler_add_record("[TF] arch setup entry");

	/* add MMIO space */
	plat_mmio_map = plat_get_mmio_map();
	if (plat_mmio_map != NULL) {
		mmap_add(plat_mmio_map);
	}
	else {
		WARN("MMIO map not available\n");
	}

	rw_size = local_rw_end - local_rw_start;
	rodata_size = local_rodata_end - local_rodata_start;

	/* add memory regions */
	mmap_add_region(local_rw_start, local_rw_start,
			rw_size,
			(uint8_t)MT_MEMORY | (uint8_t)MT_RW | (uint8_t)MT_SECURE);
	mmap_add_region(local_rodata_start, local_rodata_start,
			rodata_size,
			MT_RO_DATA | (uint8_t)MT_SECURE);
	mmap_add_region(code_base, code_base,
			code_size,
			MT_CODE | (uint8_t)MT_SECURE);

#if USE_COHERENT_MEM
	coh_start = total_base + (BL_COHERENT_RAM_BASE - BL31_RO_BASE);
	coh_size = BL_COHERENT_RAM_END - BL_COHERENT_RAM_BASE;

	mmap_add_region(coh_start, coh_start,
			coh_size,
			(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE);
#endif

	/* map TZDRAM used by BL31 as coherent memory */
	if (TEGRA_TZRAM_BASE == tegra_get_bl31_phys_base()) {
		mmap_add_region(params_from_bl2->tzdram_base,
				params_from_bl2->tzdram_base,
				BL31_SIZE,
				(uint8_t)MT_DEVICE | (uint8_t)MT_RW | (uint8_t)MT_SECURE);
	}

	/* set up translation tables */
	init_xlat_tables();

	/* enable the MMU */
	enable_mmu_el3(0);

	/*
	 * Add timestamp for arch setup exit.
	 */
	boot_profiler_add_record("[TF] arch setup exit");

	INFO("BL3-1: Tegra: MMU enabled\n");
}

/*******************************************************************************
 * Check if the given NS DRAM range is valid
 ******************************************************************************/
int32_t bl31_check_ns_address(uint64_t base, uint64_t size_in_bytes)
{

	uint64_t end = base + size_in_bytes - ULL(1);
	uint64_t tzdram_end = (uint64_t)TZDRAM_BASE + (uint64_t)TZDRAM_SIZE;
	uint64_t bl31_phys_base = tegra_get_bl31_phys_base();
	int32_t ret = 0;

	/*
	 * Check if the NS DRAM address is valid
	 */
	if ((base < TEGRA_DRAM_BASE) || (base >= TEGRA_DRAM_END) ||
	    (end > TEGRA_DRAM_END)) {

		ERROR("NS address is out-of-bounds!\n");
		ret = -EFAULT;
	}

	/*
	 * TZDRAM aperture contains the BL31 and/or BL32 images, so we need
	 * to check if the NS DRAM range overlaps the TZDRAM aperture.
	 */
	if ((base < end) && ((base > tzdram_end) || (end < bl31_phys_base))) {
		; /* do nothing */
	} else {
		/* overlap */
		ERROR("NS address overlaps TZDRAM!\n");
		ret = -ENOTSUP;
	}

	/* valid NS address */
	return ret;
}
