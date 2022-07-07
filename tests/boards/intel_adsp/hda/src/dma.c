/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/kernel.h>
#include <ztest.h>
#include <cavs_ipc.h>
#include <zephyr/drivers/dma.h>
#include "tests.h"

#define IPC_TIMEOUT K_MSEC(1500)
#define DMA_BUF_SIZE 256
#define TRANSFER_SIZE 256
#define TRANSFER_COUNT 8

#define ALIGNMENT DMA_BUF_ALIGNMENT(DT_NODELABEL(hda_host_in))
static __aligned(ALIGNMENT) uint8_t dma_buf[DMA_BUF_SIZE];

#define HDA_HOST_IN_BASE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_in), reg, 0)
#define HDA_HOST_OUT_BASE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_out), reg, 0)
#define HDA_STREAM_COUNT DT_PROP(DT_NODELABEL(hda_host_out), dma_channels)
#define HDA_REGBLOCK_SIZE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_out), reg, 1)
#include <cavs_hda.h>

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
 * Tests host input streams with the DMA API
 *
 * Note that the order of operations in this test are important and things potentially will not
 * work in horrible and unexpected ways if not done as they are here.
 */
void test_hda_host_in_dma(void)
{
	const struct device *dma;
	int res, channel;
	uint32_t last_msg_cnt;

	printk("smoke testing hda with fifo buffer at address %p, size %d\n",
	       dma_buf, DMA_BUF_SIZE);

	cavs_ipc_set_message_handler(CAVS_HOST_DEV, ipc_message, NULL);

	printk("Using buffer of size %d at addr %p\n", DMA_BUF_SIZE, dma_buf);

	/* setup a ramp in the buffer */
	for (uint32_t i = 0; i < DMA_BUF_SIZE; i++) {
		dma_buf[i] = i & 0xff;
	}

#if (IS_ENABLED(CONFIG_KERNEL_COHERENCE))
	zassert_true(arch_mem_coherent(dma_buf), "Buffer is unexpectedly incoherent!");
#else
	/* The buffer is in the cached address range and must be flushed */
	zassert_false(arch_mem_coherent(dma_buf), "Buffer is unexpectedly coherent!");
	z_xtensa_cache_flush(dma_buf, DMA_BUF_SIZE);
#endif

	dma = device_get_binding("HDA_HOST_IN");
	zassert_not_null(dma, "Expected a valid DMA device pointer");

	channel = dma_request_channel(dma, NULL);
	zassert_true(channel >= 0, "Expected a valid DMA channel");

	printk("dma channel: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, channel);

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_RESET, channel, IPC_TIMEOUT);

	printk("host reset: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, channel);

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_CONFIG,
		    channel | (DMA_BUF_SIZE << 8), IPC_TIMEOUT);
	printk("host config: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, channel);


	struct dma_block_config block_cfg = {
		.block_size = DMA_BUF_SIZE,
		.source_address = (uint32_t)(&dma_buf[0]),
	};

	struct dma_config dma_cfg = {
		.block_count = 1,
		.channel_direction = MEMORY_TO_HOST,
		.head_block = &block_cfg,
	};

	res = dma_config(dma, channel, &dma_cfg);
	printk("dsp dma config: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, channel);
	zassert_ok(res, "Expected dma config to succeed");

	res = dma_start(dma, channel);
	printk("dsp dma start: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, channel);
	zassert_ok(res, "Expected dma start to succeed");

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_START, channel, IPC_TIMEOUT);

	printk("host start: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, channel);

	for (uint32_t i = 0; i < TRANSFER_COUNT; i++) {
		res = dma_reload(dma, channel, 0, 0, DMA_BUF_SIZE);
		zassert_ok(res, "Expected dma reload to succeed");
		printk("dsp dma reload: "); cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, channel);

		struct dma_status status;
		int j;
		/* up to 10mS wait time */
		for (j = 0; j < 100; j++) {
			res = dma_get_status(dma, channel, &status);
			zassert_ok(res, "Expected dma status to succeed");
			if (status.read_position == status.write_position) {
				break;
			}
			k_busy_wait(100);
		}
		printk("dsp read write equal after %d uS: ", j*100);
		cavs_hda_dbg("host_in", HDA_HOST_IN_BASE, channel);

		last_msg_cnt = msg_cnt;
		hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_VALIDATE, channel,
			    IPC_TIMEOUT);

		WAIT_FOR(msg_cnt > last_msg_cnt, 10000, k_msleep(1));
		zassert_true(msg_res == 1,
			     "Expected data validation to be true from Host");
	}

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_RESET,
		    channel, IPC_TIMEOUT);

	res = dma_stop(dma, channel);
	zassert_ok(res, "Expected dma stop to succeed");
}

/*
 * Tests host output streams with the DMA API
 */
void test_hda_host_out_dma(void)
{
	const struct device *dma;
	int res, channel;
	bool is_ramp;


	printk("smoke testing hda with fifo buffer at address %p, size %d\n",
	       dma_buf, DMA_BUF_SIZE);

	cavs_ipc_set_message_handler(CAVS_HOST_DEV, ipc_message, NULL);

	printk("Using buffer of size %d at addr %p\n", DMA_BUF_SIZE, dma_buf);

	dma = device_get_binding("HDA_HOST_OUT");
	zassert_not_null(dma, "Expected a valid DMA device pointer");

	channel = dma_request_channel(dma, NULL);
	zassert_true(channel >= 0, "Expected a valid DMA channel");

	printk("dma channel: "); cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, channel);

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_RESET,
		    (channel + 7), IPC_TIMEOUT);

	printk("host reset: "); cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, channel);

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_CONFIG,
		    (channel + 7) | (DMA_BUF_SIZE << 8), IPC_TIMEOUT);

	printk("host config: "); cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, channel);

	struct dma_block_config block_cfg = {
		.block_size = DMA_BUF_SIZE,
		.source_address = (uint32_t)(&dma_buf[0]),
	};

	struct dma_config dma_cfg = {
		.block_count = 1,
		.channel_direction = HOST_TO_MEMORY,
		.head_block = &block_cfg,
	};

	res = dma_config(dma, channel, &dma_cfg);
	printk("dsp dma config: "); cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, channel);
	zassert_ok(res, "Expected dma config to succeed");

	res = dma_start(dma, channel);
	printk("dsp dma start: "); cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, channel);
	zassert_ok(res, "Expected dma start to succeed");

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_START, (channel + 7), IPC_TIMEOUT);

	printk("host start: ");
	cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, channel);

	for (uint32_t i = 0; i < TRANSFER_COUNT; i++) {
		hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_SEND,
			    (channel + 7) | (DMA_BUF_SIZE << 8), IPC_TIMEOUT);

		printk("host send: ");
		cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, channel);

		/* TODO add a dma_poll() style call for xfer ready/complete maybe? */
		WAIT_FOR(cavs_hda_buf_full(HDA_HOST_OUT_BASE, channel), 10000, k_msleep(1));
		printk("dsp wait for full: ");
		cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, channel);

#if (IS_ENABLED(CONFIG_KERNEL_COHERENCE))
	zassert_true(arch_mem_coherent(dma_buf), "Buffer is unexpectedly incoherent!");
#else
		/* The buffer is in the cached address range and must be invalidated
		 * prior to reading.
		 */
		zassert_false(arch_mem_coherent(dma_buf), "Buffer is unexpectedly coherent!");
		z_xtensa_cache_inv(dma_buf, DMA_BUF_SIZE);
#endif

		is_ramp = true;
		for (int j = 0; j < DMA_BUF_SIZE; j++) {
			printk("dma_buf[%d] = %d\n", j, dma_buf[j]);
			if (dma_buf[j] != j) {
				is_ramp = false;
			}
		}
		zassert_true(is_ramp, "Expected data to be a ramp");

		res = dma_reload(dma, channel, 0, 0, DMA_BUF_SIZE);
		zassert_ok(res, "Expected dma reload to succeed");
		printk("dsp dma reload: "); cavs_hda_dbg("host_out", HDA_HOST_IN_BASE, channel);
	}

	hda_ipc_msg(CAVS_HOST_DEV, IPCCMD_HDA_RESET, (channel + 7), IPC_TIMEOUT);

	printk("host reset: "); cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, channel);

	res = dma_stop(dma, channel);
	zassert_ok(res, "Expected dma stop to succeed");
	printk("dsp dma stop: "); cavs_hda_dbg("host_out", HDA_HOST_OUT_BASE, channel);
}
