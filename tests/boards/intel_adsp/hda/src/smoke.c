/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/kernel.h>
#include <ztest.h>
#include <cavs_ipc.h>
#include <zephyr/devicetree.h>
#include "tests.h"

#define HDA_HOST_IN_BASE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_in), reg, 0)
#define HDA_HOST_OUT_BASE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_out), reg, 0)
#define HDA_STREAM_COUNT DT_PROP(DT_NODELABEL(hda_host_out), dma_channels)
#define HDA_REGBLOCK_SIZE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_out), reg, 1)
#include <cavs_hda.h>

#define IPC_TIMEOUT K_MSEC(1500)
#define STREAM_ID 3U
#define HDA_BUF_SIZE 256
#define TRANSFER_COUNT 8

#define ALIGNMENT DT_PROP(DT_NODELABEL(hda_host_in), dma_buf_alignment)
static __aligned(ALIGNMENT) uint8_t hda_buf[HDA_BUF_SIZE];

static volatile int msg_cnt;
static volatile int msg_res;

static bool ipc_message(const struct device *dev, void *arg,
			uint32_t data, uint32_t ext_data)
{
	printk("HDA message received, data %u, ext_data %u\n", data, ext_data);
	msg_res = data;
	msg_cnt++;
	return true;
}

/*
 * Tests host input streams
 *
 * Note that the order of operations in this test are important and things potentially will not
 * work in horrible and unexpected ways if not done as they are here.
 */
void test_hda_host_in_smoke(void)
{
	int res;
	uint32_t last_msg_cnt;

	printk("smoke testing hda with fifo buffer at address %p, size %d\n",
	       hda_buf, HDA_BUF_SIZE);

	cavs_ipc_set_message_handler(CAVS_HOST_DEV, ipc_message, NULL);

	printk("Using buffer of size %d at addr %p\n", HDA_BUF_SIZE, hda_buf);

	/* setup a ramp in the buffer */
	for (uint32_t i = 0; i < HDA_BUF_SIZE; i++) {
		hda_buf[i] = i & 0xff;
	}

#if (IS_ENABLED(CONFIG_KERNEL_COHERENCE))
	zassert_true(arch_mem_coherent(hda_buf), "Buffer is unexpectedly incoherent!");
#else
	/* The buffer is in the cached address range and must be flushed */
	zassert_false(arch_mem_coherent(hda_buf), "Buffer is unexpectedly coherent!");
	z_xtensa_cache_flush(hda_buf, HDA_BUF_SIZE);
#endif

	cavs_hda_init(HDA_HOST_IN_BASE, STREAM_ID);
	printk("dsp init: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, STREAM_ID);

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_RESET, STREAM_ID, IPC_TIMEOUT);
	printk("host reset: ");
	cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, STREAM_ID);

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_CONFIG,
		    STREAM_ID | (HDA_BUF_SIZE << 8), IPC_TIMEOUT);
	printk("host config: ");
	cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, STREAM_ID);

	res = cavs_hda_set_buffer(HDA_HOST_IN_BASE, STREAM_ID, hda_buf, HDA_BUF_SIZE);
	printk("dsp set_buffer: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, STREAM_ID);
	zassert_ok(res, "Expected set buffer to succeed");

	cavs_hda_enable(HDA_HOST_IN_BASE, STREAM_ID);
	printk("dsp enable: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, STREAM_ID);

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_START, STREAM_ID, IPC_TIMEOUT);

	printk("host start: ");
	cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, STREAM_ID);

	for (uint32_t i = 0; i < TRANSFER_COUNT; i++) {
		cavs_hda_host_commit(HDA_HOST_IN_BASE, STREAM_ID, HDA_BUF_SIZE);
		printk("dsp inc_pos: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, STREAM_ID);

		WAIT_FOR(cavs_hda_wp_rp_eq(HDA_HOST_IN_BASE, STREAM_ID), 10000, k_msleep(1));
		printk("dsp wp_rp_eq: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, STREAM_ID);

		last_msg_cnt = msg_cnt;
		hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_VALIDATE, STREAM_ID,
			    IPC_TIMEOUT);

		WAIT_FOR(msg_cnt > last_msg_cnt, 10000, k_msleep(1));
		zassert_true(msg_res == 1,
			     "Expected data validation to be true from Host");
	}

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_RESET, STREAM_ID, IPC_TIMEOUT);
	cavs_hda_disable(HDA_HOST_IN_BASE, STREAM_ID);
}

/*
 * Tests host output streams
 *
 * Note that the order of operations in this test are important and things potentially will not
 * work in horrible and unexpected ways if not done as they are here.
 */
void test_hda_host_out_smoke(void)
{
	int res;
	bool is_ramp;

	printk("smoke testing hda with fifo buffer at address %p, size %d\n",
	       hda_buf, HDA_BUF_SIZE);

	cavs_ipc_set_message_handler(CAVS_HOST_DEV, ipc_message, NULL);

	printk("Using buffer of size %d at addr %p\n", HDA_BUF_SIZE, hda_buf);

	cavs_hda_init(HDA_HOST_OUT_BASE, STREAM_ID);
	printk("dsp init: "); cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, STREAM_ID);

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_RESET, (STREAM_ID + 7), IPC_TIMEOUT);
	printk("host reset: ");
	cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, STREAM_ID);

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_CONFIG,
		    (STREAM_ID + 7) | (HDA_BUF_SIZE << 8), IPC_TIMEOUT);

	printk("host config: ");
	cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, STREAM_ID);

	res = cavs_hda_set_buffer(HDA_HOST_OUT_BASE, STREAM_ID, hda_buf, HDA_BUF_SIZE);
	printk("dsp set buffer: "); cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, STREAM_ID);
	zassert_ok(res, "Expected set buffer to succeed");

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_START, (STREAM_ID + 7), IPC_TIMEOUT);
	printk("host start: ");
	cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, STREAM_ID);

	cavs_hda_enable(HDA_HOST_OUT_BASE, STREAM_ID);
	printk("dsp enable: ");
	cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, STREAM_ID);

	for (uint32_t i = 0; i < TRANSFER_COUNT; i++) {
		for (int j = 0; j < HDA_BUF_SIZE; j++) {
			hda_buf[j] = 0;
		}

		hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_SEND,
			    (STREAM_ID + 7) | (HDA_BUF_SIZE << 8), IPC_TIMEOUT);
		printk("host send: ");
		cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, STREAM_ID);


		WAIT_FOR(cavs_hda_buf_full(HDA_HOST_OUT_BASE, STREAM_ID), 10000, k_msleep(1));
		printk("dsp wait for full: ");
		cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, STREAM_ID);

#if (IS_ENABLED(CONFIG_KERNEL_COHERENCE))
	zassert_true(arch_mem_coherent(hda_buf), "Buffer is unexpectedly incoherent!");
#else
		/* The buffer is in the cached address range and must be invalidated
		 * prior to reading.
		 */
		zassert_false(arch_mem_coherent(hda_buf), "Buffer is unexpectedly coherent!");
		z_xtensa_cache_inv(hda_buf, HDA_BUF_SIZE);
#endif

		is_ramp = true;
		for (int j = 0; j < HDA_BUF_SIZE; j++) {
			/* printk("hda_buf[%d] = %d\n", j, hda_buf[j]); */ /* DEBUG HELPER */
			if (hda_buf[j] != j) {
				is_ramp = false;
			}
		}
		zassert_true(is_ramp, "Expected data to be a ramp");

		cavs_hda_host_commit(HDA_HOST_OUT_BASE, STREAM_ID, HDA_BUF_SIZE);
		printk("dsp inc pos: "); cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, STREAM_ID);

	}

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_RESET, (STREAM_ID + 7),
		    IPC_TIMEOUT);

	printk("host reset: ");
	cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, STREAM_ID);

	cavs_hda_disable(HDA_HOST_OUT_BASE, STREAM_ID);
	printk("dsp disable: "); cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, STREAM_ID);
}
