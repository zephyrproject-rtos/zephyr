/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <intel_adsp_ipc.h>
#include <zephyr/devicetree.h>
#include "tests.h"

#define HDA_HOST_IN_BASE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_in), reg, 0)
#define HDA_HOST_OUT_BASE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_out), reg, 0)
#define HDA_STREAM_COUNT DT_PROP(DT_NODELABEL(hda_host_out), dma_channels)
#define HDA_REGBLOCK_SIZE DT_PROP_BY_IDX(DT_NODELABEL(hda_host_out), reg, 1)
#include <intel_adsp_hda.h>

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
ZTEST(intel_adsp_hda, test_hda_host_in_smoke)
{
	int res;
	uint32_t last_msg_cnt;

	printk("smoke testing hda with fifo buffer at address %p, size %d\n",
		hda_buf, HDA_BUF_SIZE);

	intel_adsp_ipc_set_message_handler(INTEL_ADSP_IPC_HOST_DEV, ipc_message, NULL);

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

	intel_adsp_hda_init(HDA_HOST_IN_BASE, HDA_REGBLOCK_SIZE, STREAM_ID);
	hda_dump_regs(HOST_IN, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp init");

	hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_RESET, STREAM_ID, IPC_TIMEOUT);
	hda_dump_regs(HOST_IN, HDA_REGBLOCK_SIZE, STREAM_ID, "host reset");

	hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_CONFIG,
		    STREAM_ID | (HDA_BUF_SIZE << 8), IPC_TIMEOUT);
	hda_dump_regs(HOST_IN, HDA_REGBLOCK_SIZE, STREAM_ID, "host config");

	res = intel_adsp_hda_set_buffer(HDA_HOST_IN_BASE, HDA_REGBLOCK_SIZE, STREAM_ID,
				hda_buf, HDA_BUF_SIZE);
	hda_dump_regs(HOST_IN, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp set_buffer");
	zassert_ok(res, "Expected set buffer to succeed");

	intel_adsp_hda_enable(HDA_HOST_IN_BASE, HDA_REGBLOCK_SIZE, STREAM_ID);
	hda_dump_regs(HOST_IN, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp enable");

	hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_START, STREAM_ID, IPC_TIMEOUT);
	hda_dump_regs(HOST_IN, HDA_REGBLOCK_SIZE, STREAM_ID, "host start");

	for (uint32_t i = 0; i < TRANSFER_COUNT; i++) {
		intel_adsp_hda_host_commit(HDA_HOST_IN_BASE, HDA_REGBLOCK_SIZE, STREAM_ID,
			HDA_BUF_SIZE);
		hda_dump_regs(HOST_IN, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp inc_pos");

		WAIT_FOR(intel_adsp_hda_wp_rp_eq(HDA_HOST_IN_BASE, HDA_REGBLOCK_SIZE, STREAM_ID),
			10000, k_msleep(1));
		hda_dump_regs(HOST_IN, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp wp == rp");

		last_msg_cnt = msg_cnt;
		hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_VALIDATE, STREAM_ID,
			    IPC_TIMEOUT);

		WAIT_FOR(msg_cnt > last_msg_cnt, 10000, k_msleep(1));
		zassert_true(msg_res == 1,
			     "Expected data validation to be true from Host");
	}

	hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_RESET, STREAM_ID, IPC_TIMEOUT);
	hda_dump_regs(HOST_IN, HDA_REGBLOCK_SIZE, STREAM_ID, "host reset");

	intel_adsp_hda_disable(HDA_HOST_IN_BASE, HDA_REGBLOCK_SIZE, STREAM_ID);
	hda_dump_regs(HOST_IN, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp disable");
}

/*
 * Tests host output streams
 *
 * Note that the order of operations in this test are important and things potentially will not
 * work in horrible and unexpected ways if not done as they are here.
 */
ZTEST(intel_adsp_hda, test_hda_host_out_smoke)
{
	int res;
	bool is_ramp;

	printk("smoke testing hda with fifo buffer at address %p, size %d\n",
		hda_buf, HDA_BUF_SIZE);

	intel_adsp_ipc_set_message_handler(INTEL_ADSP_IPC_HOST_DEV, ipc_message, NULL);

	printk("Using buffer of size %d at addr %p\n", HDA_BUF_SIZE, hda_buf);

	intel_adsp_hda_init(HDA_HOST_OUT_BASE, HDA_REGBLOCK_SIZE, STREAM_ID);
	hda_dump_regs(HOST_OUT, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp init");

	hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_RESET, (STREAM_ID + 7), IPC_TIMEOUT);
	hda_dump_regs(HOST_OUT, HDA_REGBLOCK_SIZE, STREAM_ID, "host reset");

	hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_CONFIG,
		    (STREAM_ID + 7) | (HDA_BUF_SIZE << 8), IPC_TIMEOUT);
	hda_dump_regs(HOST_OUT, HDA_REGBLOCK_SIZE, STREAM_ID, "host config");

	res = intel_adsp_hda_set_buffer(HDA_HOST_OUT_BASE, HDA_REGBLOCK_SIZE, STREAM_ID,
		hda_buf, HDA_BUF_SIZE);
	hda_dump_regs(HOST_OUT, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp set buffer");
	zassert_ok(res, "Expected set buffer to succeed");

	hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_START, (STREAM_ID + 7), IPC_TIMEOUT);
	hda_dump_regs(HOST_OUT, HDA_REGBLOCK_SIZE, STREAM_ID, "host start");

	intel_adsp_hda_enable(HDA_HOST_OUT_BASE, HDA_REGBLOCK_SIZE, STREAM_ID);
	hda_dump_regs(HOST_OUT, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp enable");

	for (uint32_t i = 0; i < TRANSFER_COUNT; i++) {
		for (int j = 0; j < HDA_BUF_SIZE; j++) {
			hda_buf[j] = 0;
		}

		hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_SEND,
			    (STREAM_ID + 7) | (HDA_BUF_SIZE << 8), IPC_TIMEOUT);
		hda_dump_regs(HOST_OUT, HDA_REGBLOCK_SIZE, STREAM_ID, "host send");


		WAIT_FOR(intel_adsp_hda_buf_full(HDA_HOST_OUT_BASE, HDA_REGBLOCK_SIZE, STREAM_ID),
			10000, k_msleep(1));
		hda_dump_regs(HOST_OUT, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp wait for full");

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

		intel_adsp_hda_host_commit(HDA_HOST_OUT_BASE, HDA_REGBLOCK_SIZE, STREAM_ID,
			HDA_BUF_SIZE);
		hda_dump_regs(HOST_OUT, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp inc pos");

	}

	hda_ipc_msg(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_HDA_RESET, (STREAM_ID + 7),
		    IPC_TIMEOUT);
	hda_dump_regs(HOST_OUT, HDA_REGBLOCK_SIZE, STREAM_ID, "host reset");

	intel_adsp_hda_disable(HDA_HOST_OUT_BASE, HDA_REGBLOCK_SIZE, STREAM_ID);
	hda_dump_regs(HOST_OUT, HDA_REGBLOCK_SIZE, STREAM_ID, "dsp disable");
}

ZTEST_SUITE(intel_adsp_hda, NULL, NULL, NULL, NULL, NULL);
