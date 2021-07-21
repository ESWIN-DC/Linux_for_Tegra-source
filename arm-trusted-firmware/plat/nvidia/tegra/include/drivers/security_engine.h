/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef __SECURITY_ENGINE_H__
#define __SECURITY_ENGINE_H__

/*******************************************************************************
 * Structure definition
 ******************************************************************************/

/* Security Engine Linked List */
struct tegra_se_ll {
	/* DMA buffer address */
	uint32_t addr;
	/* Data length in DMA buffer */
	uint32_t data_len;
};

#define SE_LL_MAX_BUFFER_NUM			4
typedef struct tegra_se_io_lst {
	volatile uint32_t last_buff_num;
	volatile struct tegra_se_ll buffer[SE_LL_MAX_BUFFER_NUM];
} tegra_se_io_lst_t __attribute__((aligned(4)));

/* SE device structure */
typedef struct tegra_se_dev {
	/* Security Engine ID */
	const int se_num;
	/* SE base address */
	const uint64_t se_base;
	/* SE context size in AES blocks */
	const uint32_t ctx_size_blks;
	/* pointer to source linked list buffer */
	tegra_se_io_lst_t *src_ll_buf;
	/* pointer to destination linked list buffer */
	tegra_se_io_lst_t *dst_ll_buf;
	/* LP context buffer pointer */
	uint32_t *ctx_save_buf;
} tegra_se_dev_t;

/* PKA1 device structure */
typedef struct tegra_pka_dev {
	/* PKA1 base address */
	uint64_t pka_base;
} tegra_pka_dev_t;

/*******************************************************************************
 * Public interface
 ******************************************************************************/
void tegra_se_init(void);
int tegra_se_suspend(void);
void tegra_se_resume(void);
int tegra_se_save_tzram(void);
int32_t tegra_se_save_sha256_hash(uint64_t bl31_base, uint32_t src_len_inbyte);

#endif /* __SECURITY_ENGINE_H__ */
