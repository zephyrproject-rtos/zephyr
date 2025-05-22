/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/firmware/nrf_ironside/call.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT nordic_ironside_call

#define SHM_NODE     DT_INST_PHANDLE(0, memory_region)
#define NUM_BUFS     (DT_REG_SIZE(SHM_NODE) / sizeof(struct ironside_call_buf))
#define ALL_BUF_BITS BIT_MASK(NUM_BUFS)

/* Note: this area is already zero-initialized at reset time. */
static struct ironside_call_buf *const bufs = (void *)DT_REG_ADDR(SHM_NODE);

#if defined(CONFIG_DCACHE_LINE_SIZE)
BUILD_ASSERT((DT_REG_ADDR(SHM_NODE) % CONFIG_DCACHE_LINE_SIZE) == 0);
BUILD_ASSERT((sizeof(struct ironside_call_buf) % CONFIG_DCACHE_LINE_SIZE) == 0);
#endif

static const struct mbox_dt_spec mbox_rx = MBOX_DT_SPEC_INST_GET(0, rx);
static const struct mbox_dt_spec mbox_tx = MBOX_DT_SPEC_INST_GET(0, tx);

static K_EVENT_DEFINE(alloc_evts);
static K_EVENT_DEFINE(rsp_evts);

static void ironside_call_rsp(const struct device *dev, mbox_channel_id_t channel_id,
			      void *user_data, struct mbox_msg *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(user_data);
	ARG_UNUSED(data);

	struct ironside_call_buf *buf;
	uint32_t rsp_buf_bits = 0;

	/* Check which buffers are not being dispatched currently. Those must
	 * not be cache-invalidated, in case they're used in thread context.
	 *
	 * This value will remain valid as long as ironside_call_rsp is never
	 * preempted by ironside_call_dispatch; the former runs in MBOX ISR,
	 * while the latter shall not run in ISR (because of k_event_wait).
	 */
	const uint32_t skip_buf_bits = k_event_test(&rsp_evts, ALL_BUF_BITS);

	for (int i = 0; i < NUM_BUFS; i++) {
		if (skip_buf_bits & BIT(i)) {
			continue;
		}

		buf = &bufs[i];

		sys_cache_data_invd_range(buf, sizeof(*buf));
		barrier_dmem_fence_full();

		if (buf->status != IRONSIDE_CALL_STATUS_IDLE &&
		    buf->status != IRONSIDE_CALL_STATUS_REQ) {
			rsp_buf_bits |= BIT(i);
		}
	}
	k_event_post(&rsp_evts, rsp_buf_bits);
}

static int ironside_call_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int err;

	k_event_set(&alloc_evts, ALL_BUF_BITS);
	k_event_set(&rsp_evts, ALL_BUF_BITS);

	err = mbox_register_callback_dt(&mbox_rx, ironside_call_rsp, NULL);
	__ASSERT_NO_MSG(err == 0);

	err = mbox_set_enabled_dt(&mbox_rx, 1);
	__ASSERT_NO_MSG(err == 0);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, ironside_call_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_NRF_IRONSIDE_CALL_INIT_PRIORITY, NULL);

struct ironside_call_buf *ironside_call_alloc(void)
{
	uint32_t avail_buf_bits;
	uint32_t alloc_buf_bit;

	do {
		avail_buf_bits = k_event_wait(&alloc_evts, ALL_BUF_BITS, false, K_FOREVER);

		/* Try allocating the first available block.
		 * If it's claimed by another thread, go back and wait for another block.
		 */
		alloc_buf_bit = LSB_GET(avail_buf_bits);
	} while (k_event_clear(&alloc_evts, alloc_buf_bit) == 0);

	return &bufs[u32_count_trailing_zeros(alloc_buf_bit)];
}

void ironside_call_dispatch(struct ironside_call_buf *buf)
{
	const uint32_t buf_bit = BIT(buf - bufs);
	int err;

	buf->status = IRONSIDE_CALL_STATUS_REQ;
	barrier_dmem_fence_full();

	sys_cache_data_flush_range(buf, sizeof(*buf));

	k_event_clear(&rsp_evts, buf_bit);

	err = mbox_send_dt(&mbox_tx, NULL);
	__ASSERT_NO_MSG(err == 0);

	k_event_wait(&rsp_evts, buf_bit, false, K_FOREVER);
}

void ironside_call_release(struct ironside_call_buf *buf)
{
	const uint32_t buf_bit = BIT(buf - bufs);

	buf->status = IRONSIDE_CALL_STATUS_IDLE;
	barrier_dmem_fence_full();

	sys_cache_data_flush_range(buf, sizeof(*buf));

	k_event_post(&alloc_evts, buf_bit);
}
