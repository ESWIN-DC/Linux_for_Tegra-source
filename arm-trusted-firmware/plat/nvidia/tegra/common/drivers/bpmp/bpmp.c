/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
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
#include <assert.h>
#include <bpmp.h>
#include <debug.h>
#include <delay_timer.h>
#include <errno.h>
#include <mmio.h>
#include <platform.h>
#include <stdbool.h>
#include <string.h>
#include <tegra_def.h>

#define BPMP_TIMEOUT	500 /* 500ms */

static uint32_t channel_base[NR_CHANNELS];
static uint32_t bpmp_init_state = BPMP_INIT_PENDING;

static uint32_t channel_field(uint ch)
{
	return mmio_read_32(TEGRA_RES_SEMA_BASE + STA_OFFSET) & CH_MASK(ch);
}

static bool master_free(uint ch)
{
	return channel_field(ch) == MA_FREE(ch);
}

static bool master_acked(uint ch)
{
	return channel_field(ch) == MA_ACKD(ch);
}

static void signal_slave(uint ch)
{
	mmio_write_32(TEGRA_RES_SEMA_BASE + CLR_OFFSET, CH_MASK(ch));
}

static void free_master(uint ch)
{
	mmio_write_32(TEGRA_RES_SEMA_BASE + CLR_OFFSET,
		      MA_ACKD(ch) ^ MA_FREE(ch));
}

/* should be called with local irqs disabled */
int32_t tegra_bpmp_send_receive_atomic(int mrq, const void *ob_data, int ob_sz,
		void *ib_data, int ib_sz)
{
	uint ch = (uint)plat_my_core_pos();
	mb_data_t *p = (mb_data_t *)(uintptr_t)channel_base[ch];
	int32_t ret = -ETIMEDOUT, timeout = 0;

	if (bpmp_init_state == BPMP_INIT_COMPLETE) {

		/* loop until BPMP is free */
		for (timeout = 0; timeout < BPMP_TIMEOUT; timeout++) {
			if (master_free(ch) == true) {
				break;
			}

			mdelay(1);
		}

		if (timeout != BPMP_TIMEOUT) {

			/* generate the command struct */
			p->code = mrq;
			p->flags = DO_ACK;
			(void)memcpy((void *)p->data, ob_data, (size_t)ob_sz);

			/* signal command ready to the BPMP */
			signal_slave(ch);
			mmio_write_32(TEGRA_PRI_ICTLR_BASE + CPU_IEP_FIR_SET,
				      (1U << INT_SHR_SEM_OUTBOX_FULL));

			/* loop until the command is executed */
			for (timeout = 0; timeout < BPMP_TIMEOUT; timeout++) {
				if (master_acked(ch) == true) {
					break;
				}

				mdelay(1);
			}

			if (timeout != BPMP_TIMEOUT) {

				/* get the command response */
				(void)memcpy(ib_data, (const void *)p->data,
					     (size_t)ib_sz);

				/* return error code */
				ret = p->code;

				/* free this channel */
				free_master(ch);
			}
		}

	} else {
		/* return error code */
		ret = -EINVAL;
	}

	if (timeout == BPMP_TIMEOUT) {
		ERROR("Timed out waiting for bpmp's response\n");
	}

	return ret;
}

int tegra_bpmp_init(void)
{
	uint32_t val, base, timeout = BPMP_TIMEOUT;
	uint ch;
	int ret = 0;

	if (bpmp_init_state == BPMP_INIT_PENDING) {

		/* check if the bpmp processor is alive. */
		do {
			val = mmio_read_32(TEGRA_RES_SEMA_BASE + STA_OFFSET);
			if (val != SIGN_OF_LIFE) {
				mdelay(1);
				timeout--;
			}

		} while ((val != SIGN_OF_LIFE) && (timeout > 0U));

		if (val == SIGN_OF_LIFE) {

			/* check if clock for the atomics block is enabled */
			val = mmio_read_32(TEGRA_CAR_RESET_BASE + TEGRA_CLK_ENB_V);
			if ((val & CAR_ENABLE_ATOMICS) == 0) {
				ERROR("Clock to the atomics block is disabled\n");
			}

			/* check if the atomics block is out of reset */
			val = mmio_read_32(TEGRA_CAR_RESET_BASE + TEGRA_RST_DEV_CLR_V);
			if ((val & CAR_ENABLE_ATOMICS) == CAR_ENABLE_ATOMICS) {
				ERROR("Reset to the atomics block is asserted\n");
			}

			/* base address to get the result from Atomics */
			base = TEGRA_ATOMICS_BASE + RESULT0_REG_OFFSET;

			/* channel area is setup by BPMP before signaling handshake */
			for (ch = 0; ch < NR_CHANNELS; ch++) {

				/* issue command to get the channel base address */
				mmio_write_32(base, (ch << TRIGGER_ID_SHIFT) |
					      ATOMIC_CMD_GET);

				/* get the base address for the channel */
				channel_base[ch] = mmio_read_32(base);

				/* increment result register offset */
				base += 4U;
			}

			/* mark state as "initialized" */
			bpmp_init_state = BPMP_INIT_COMPLETE;

			/* the channel values have to be visible across all cpus */
			flush_dcache_range((uint64_t)channel_base,
					   sizeof(channel_base));
			flush_dcache_range((uint64_t)&bpmp_init_state,
					   sizeof(bpmp_init_state));

			INFO("%s: done\n", __func__);

		} else {
			ERROR("BPMP not powered on\n");

			/* bpmp is not present in the system */
			bpmp_init_state = BPMP_NOT_PRESENT;

			/* communication timed out */
			ret = -ETIMEDOUT;
		}
	}

	return ret;
}

void tegra_bpmp_suspend(void)
{
	/* freeze the interface */
	if (bpmp_init_state == BPMP_INIT_COMPLETE) {
		bpmp_init_state = BPMP_SUSPEND_ENTRY;
		flush_dcache_range((uint64_t)&bpmp_init_state,
				   sizeof(bpmp_init_state));
	}
}

void tegra_bpmp_resume(void)
{
	uint32_t val, timeout = 0;

	if (bpmp_init_state == BPMP_SUSPEND_ENTRY) {

		/* check if the bpmp processor is alive. */
		do {

			val = mmio_read_32(TEGRA_RES_SEMA_BASE + STA_OFFSET);
			if (val != SIGN_OF_LIFE) {
				mdelay(1);
				timeout++;
			}

		} while ((val != SIGN_OF_LIFE) && (timeout < BPMP_TIMEOUT));

		if (val == SIGN_OF_LIFE) {

			INFO("%s: BPMP took %d ms to resume\n", __func__, timeout);

			/* mark state as "initialized" */
			bpmp_init_state = BPMP_INIT_COMPLETE;

			/* state has to be visible across all cpus */
			flush_dcache_range((uint64_t)&bpmp_init_state,
					   sizeof(bpmp_init_state));
		} else {
			ERROR("BPMP not powered on\n");
		}
	}
}
