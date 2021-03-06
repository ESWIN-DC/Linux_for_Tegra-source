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

#ifndef __TEGRA_DEF_H__
#define __TEGRA_DEF_H__

#include <utils_def.h>

/*******************************************************************************
 * Chip specific page table and MMU setup constants
 ******************************************************************************/
#define PLAT_PHY_ADDR_SPACE_SIZE	(ULL(1) << 40)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(ULL(1) << 40)

/*******************************************************************************
 * These values are used by the PSCI implementation during the `CPU_SUSPEND`
 * and `SYSTEM_SUSPEND` calls as the `state-id` field in the 'power state'
 * parameter.
 ******************************************************************************/
#define PSTATE_ID_CORE_IDLE		U(6)
#define PSTATE_ID_CORE_POWERDN		U(7)
#define PSTATE_ID_SOC_POWERDN		U(2)

/*******************************************************************************
 * Platform power states (used by PSCI framework)
 *
 * - PLAT_MAX_RET_STATE should be less than lowest PSTATE_ID
 * - PLAT_MAX_OFF_STATE should be greater than the highest PSTATE_ID
 ******************************************************************************/
#define PLAT_MAX_RET_STATE		U(1)
#define PLAT_MAX_OFF_STATE		U(8)

/*******************************************************************************
 * Secure IRQ definitions
 ******************************************************************************/
#define TEGRA194_MAX_SEC_IRQS		U(2)
#define TEGRA194_TOP_WDT_IRQ		U(49)
#define TEGRA194_AON_WDT_IRQ		U(50)

#define TEGRA194_SEC_IRQ_TARGET_MASK	U(0xFF) /* 8 Carmel */

/*******************************************************************************
 * Clock identifier for the SE device
 ******************************************************************************/
#define TEGRA194_CLK_SE			U(124)
#define TEGRA_CLK_SE			TEGRA194_CLK_SE

/*******************************************************************************
 * Tegra Miscellanous register constants
 ******************************************************************************/
#define TEGRA_MISC_BASE			U(0x00100000)

#define HARDWARE_REVISION_OFFSET	U(0x4)
#define MISCREG_EMU_REVID		U(0x3160)
#define  BOARD_MASK_BITS		U(0xFF)
#define  BOARD_SHIFT_BITS		U(24)
#define MISCREG_PFCFG			U(0x200C)

/*******************************************************************************
 * Tegra General Purpose Centralised DMA constants
 ******************************************************************************/
#define TEGRA_GPCDMA_BASE		U(0x02610000)

/*******************************************************************************
 * Tegra Memory Controller constants
 ******************************************************************************/
#define TEGRA_MC_STREAMID_BASE		U(0x02C00000)
#define TEGRA_MC_BASE			U(0x02C10000)

/* General Security Carveout register macros */
#define MC_GSC_CONFIG_REGS_SIZE		U(0x40)
#define MC_GSC_LOCK_CFG_SETTINGS_BIT	(U(1) << 1)
#define MC_GSC_ENABLE_TZ_LOCK_BIT	(U(1) << 0)
#define MC_GSC_SIZE_RANGE_4KB_SHIFT	U(27)
#define MC_GSC_BASE_LO_SHIFT		U(12)
#define MC_GSC_BASE_LO_MASK		U(0xFFFFF)
#define MC_GSC_BASE_HI_SHIFT		U(0)
#define MC_GSC_BASE_HI_MASK		U(3)
#define MC_GSC_ENABLE_CPU_SECURE_BIT    (U(1) << 31)

/* TZDRAM carveout configuration registers */
#define MC_SECURITY_CFG0_0		U(0x70)
#define MC_SECURITY_CFG1_0		U(0x74)
#define MC_SECURITY_CFG3_0		U(0x9BC)

#define MC_SECURITY_BOM_MASK		(U(0xFFF) << 20)
#define MC_SECURITY_SIZE_MB_MASK	(U(0x1FFF) << 0)
#define MC_SECURITY_BOM_HI_MASK		(U(0x3) << 0)

#define MC_SECURITY_CFG_REG_CTRL_0 U(0x154)
#define  SECURITY_CFG_WRITE_ACCESS_BIT     (U(0x1) << 0)
#define  SECURITY_CFG_WRITE_ACCESS_ENABLE  U(0x0)
#define  SECURITY_CFG_WRITE_ACCESS_DISABLE U(0x1)

/* Video Memory carveout configuration registers */
#define MC_VIDEO_PROTECT_BASE_HI	U(0x978)
#define MC_VIDEO_PROTECT_BASE_LO	U(0x648)
#define MC_VIDEO_PROTECT_SIZE_MB	U(0x64c)

/*
 * Carveout (MC_SECURITY_CARVEOUT24) registers used to clear the
 * non-overlapping Video memory region
 */
#define MC_VIDEO_PROTECT_CLEAR_CFG	U(0x25A0)
#define MC_VIDEO_PROTECT_CLEAR_BASE_LO	U(0x25A4)
#define MC_VIDEO_PROTECT_CLEAR_BASE_HI	U(0x25A8)
#define MC_VIDEO_PROTECT_CLEAR_SIZE	U(0x25AC)
#define MC_VIDEO_PROTECT_CLEAR_ACCESS_CFG0	U(0x25B0)

/* TZRAM carveout (MC_SECURITY_CARVEOUT11) configuration registers */
#define MC_TZRAM_CARVEOUT_CFG		U(0x2190)
#define MC_TZRAM_BASE_LO		U(0x2194)
#define MC_TZRAM_BASE_HI		U(0x2198)
#define MC_TZRAM_SIZE			U(0x219C)
#define MC_TZRAM_CLIENT_ACCESS0_CFG0	U(0x21A0)
#define MC_TZRAM_CLIENT_ACCESS1_CFG0	U(0x21A4)
#define  TZRAM_ALLOW_MPCORER		(U(1) << 7)
#define  TZRAM_ALLOW_MPCOREW		(U(1) << 25)

/* Memory Controller Reset Control registers */
#define  MC_CLIENT_HOTRESET_CTRL1_DLAA_FLUSH_ENB	(U(1) << 28)
#define  MC_CLIENT_HOTRESET_CTRL1_DLA1A_FLUSH_ENB	(U(1) << 29)
#define  MC_CLIENT_HOTRESET_CTRL1_PVA0A_FLUSH_ENB	(U(1) << 30)
#define  MC_CLIENT_HOTRESET_CTRL1_PVA1A_FLUSH_ENB	(U(1) << 31)

/*******************************************************************************
 * Tegra UART Controller constants
 ******************************************************************************/
#define TEGRA_UARTA_BASE		U(0x03100000)
#define TEGRA_UARTB_BASE		U(0x03110000)
#define TEGRA_UARTC_BASE		U(0x0C280000)
#define TEGRA_UARTD_BASE		U(0x03130000)
#define TEGRA_UARTE_BASE		U(0x03140000)
#define TEGRA_UARTF_BASE		U(0x03150000)
#define TEGRA_UARTG_BASE		U(0x0C290000)

/*******************************************************************************
 * XUSB PADCTL
 ******************************************************************************/
#define TEGRA_XUSB_PADCTL_BASE			U(0x03520000)
#define TEGRA_XUSB_PADCTL_SIZE			U(0x10000)
#define XUSB_PADCTL_HOST_AXI_STREAMID_PF_0	U(0x136c)
#define XUSB_PADCTL_HOST_AXI_STREAMID_VF_0	U(0x1370)
#define XUSB_PADCTL_HOST_AXI_STREAMID_VF_1	U(0x1374)
#define XUSB_PADCTL_HOST_AXI_STREAMID_VF_2	U(0x1378)
#define XUSB_PADCTL_HOST_AXI_STREAMID_VF_3	U(0x137c)
#define XUSB_PADCTL_DEV_AXI_STREAMID_PF_0	U(0x139c)

/*******************************************************************************
 * Tegra Fuse Controller related constants
 ******************************************************************************/
#define TEGRA_FUSE_BASE			U(0x03820000)
#define  SECURITY_MODE			U(0x1A0)
#define  OPT_SUBREVISION		U(0x248)
#define  SUBREVISION_MASK		U(0xF)

/* ODM Production Fuse not set */
#define ODM_PROD_FUSE_DISABLED		0U

/* Clock identifier for the FUSE device */
#define TEGRA194_CLK_FUSE		U(40)
#define TEGRA_CLK_FUSE			TEGRA194_CLK_FUSE

/*******************************************************************************
 * GICv2 & interrupt handling related constants
 ******************************************************************************/
#define TEGRA_GICD_BASE			U(0x03881000)
#define TEGRA_GICC_BASE			U(0x03882000)

/*******************************************************************************
 * Security Engine related constants
 ******************************************************************************/
#define TEGRA_SE0_BASE			U(0x03AC0000)
#define  SE0_MUTEX_WATCHDOG_NS_LIMIT	U(0x6C)
#define  SE0_AES0_ENTROPY_SRC_AGE_CTRL	U(0x2FC)
#define TEGRA_PKA1_BASE			U(0x03AD0000)
#define  SE_PKA1_CTRL_SE_MUTEX_TMOUT_DFTVAL U(0x144)
#define  PKA1_MUTEX_WATCHDOG_NS_LIMIT	SE_PKA1_CTRL_SE_MUTEX_TMOUT_DFTVAL
#define TEGRA_RNG1_BASE			U(0x03AE0000)
#define  RNG1_MUTEX_WATCHDOG_NS_LIMIT	U(0xFE0)

/*******************************************************************************
 * Tegra HSP doorbell #0 constants
 ******************************************************************************/
#define TEGRA_HSP_DBELL_BASE		U(0x03C90000)
#define  HSP_DBELL_1_ENABLE		U(0x104)
#define  HSP_DBELL_3_TRIGGER		U(0x300)
#define  HSP_DBELL_3_ENABLE		U(0x304)

/*******************************************************************************
 * Tegra hardware synchronization primitives for the SPE engine
 ******************************************************************************/
#define TEGRA_AON_HSP_SM_6_7_BASE	U(0x0c190000)
#define TEGRA_CONSOLE_SPE_BASE		(TEGRA_AON_HSP_SM_6_7_BASE + U(0x8000))

/*******************************************************************************
 * Tegra micro-seconds timer constants
 ******************************************************************************/
#define TEGRA_TMRUS_BASE		U(0x0C2E0000)
#define TEGRA_TMRUS_SIZE		U(0x10000)

/*******************************************************************************
 * Tegra Power Mgmt Controller constants
 ******************************************************************************/
#define TEGRA_PMC_BASE			U(0x0C360000)

/*******************************************************************************
 * Tegra scratch registers constants
 ******************************************************************************/
#define TEGRA_SCRATCH_BASE		U(0x0C390000)
#define  SECURE_SCRATCH_RSV75   	U(0x2BC)
#define  SECURE_SCRATCH_RSV81_LO	U(0x2EC)
#define  SECURE_SCRATCH_RSV81_HI	U(0x2F0)
#define  SECURE_SCRATCH_RSV97		U(0x36C)
#define  SECURE_SCRATCH_RSV99_LO	U(0x37C)
#define  SECURE_SCRATCH_RSV99_HI	U(0x380)
#define  SECURE_SCRATCH_RSV109_LO	U(0x3CC)
#define  SECURE_SCRATCH_RSV109_HI	U(0x3D0)

#define SCRATCH_BL31_PARAMS_HI_ADDR	SECURE_SCRATCH_RSV75
#define  SCRATCH_BL31_PARAMS_HI_ADDR_MASK  U(0xFFFF)
#define  SCRATCH_BL31_PARAMS_HI_ADDR_SHIFT U(0)
#define SCRATCH_BL31_PARAMS_LO_ADDR	SECURE_SCRATCH_RSV81_LO
#define SCRATCH_BL31_PLAT_PARAMS_HI_ADDR SECURE_SCRATCH_RSV75
#define  SCRATCH_BL31_PLAT_PARAMS_HI_ADDR_MASK  U(0xFFFF0000)
#define  SCRATCH_BL31_PLAT_PARAMS_HI_ADDR_SHIFT U(16)
#define SCRATCH_BL31_PLAT_PARAMS_LO_ADDR SECURE_SCRATCH_RSV81_HI
#define SCRATCH_SECURE_BOOTP_FCFG	SECURE_SCRATCH_RSV97
#define SCRATCH_SMMU_TABLE_ADDR_LO	SECURE_SCRATCH_RSV99_LO
#define SCRATCH_SMMU_TABLE_ADDR_HI	SECURE_SCRATCH_RSV99_HI
#define SCRATCH_RESET_VECTOR_LO		SECURE_SCRATCH_RSV109_LO
#define SCRATCH_RESET_VECTOR_HI		SECURE_SCRATCH_RSV109_HI

/*******************************************************************************
 * Tegra Memory Mapped Control Register Access Bus constants
 ******************************************************************************/
#define TEGRA_MMCRAB_BASE		U(0x0E000000)

/*******************************************************************************
 * Tegra SMMU Controller constants
 ******************************************************************************/
#define TEGRA_SMMU0_BASE		U(0x12000000)
#define TEGRA_SMMU1_BASE		U(0x11000000)
#define TEGRA_SMMU2_BASE		U(0x10000000)

/*******************************************************************************
 * Tegra TZRAM constants
 ******************************************************************************/
#define TEGRA_TZRAM_BASE		U(0x40000000)
#define TEGRA_TZRAM_SIZE		U(0x40000)

/*******************************************************************************
 * Tegra CCPLEX-BPMP IPC constants
 ******************************************************************************/
#define TEGRA_BPMP_IPC_TX_PHYS_BASE	U(0x4004C000)
#define TEGRA_BPMP_IPC_RX_PHYS_BASE	U(0x4004D000)
#define TEGRA_BPMP_IPC_CH_MAP_SIZE	U(0x1000) /* 4KB */

/*******************************************************************************
 * Tegra Clock and Reset Controller constants
 ******************************************************************************/
#define TEGRA_CAR_RESET_BASE		U(0x20000000)
#define TEGRA_GPU_RESET_REG_OFFSET	U(0x18)
#define TEGRA_GPU_RESET_GPU_SET_OFFSET  U(0x1C)
#define  GPU_RESET_BIT			(U(1) << 0)
#define  GPU_SET_BIT                    (U(1) << 0)
#define TEGRA_GPCDMA_RST_SET_REG_OFFSET	U(0x6A0004)
#define TEGRA_GPCDMA_RST_CLR_REG_OFFSET	U(0x6A0008)

/*******************************************************************************
 * Tegra DRAM memory base address
 ******************************************************************************/
#define TEGRA_DRAM_BASE			ULL(0x80000000)
#define TEGRA_DRAM_END			ULL(0xFFFFFFFFF)

/*******************************************************************************
 * XUSB STREAMIDs
 ******************************************************************************/
#define TEGRA_SID_XUSB_HOST			U(0x1b)
#define TEGRA_SID_XUSB_DEV			U(0x1c)
#define TEGRA_SID_XUSB_VF0			U(0x5d)
#define TEGRA_SID_XUSB_VF1			U(0x5e)
#define TEGRA_SID_XUSB_VF2			U(0x5f)
#define TEGRA_SID_XUSB_VF3			U(0x60)

#endif /* __TEGRA_DEF_H__ */
