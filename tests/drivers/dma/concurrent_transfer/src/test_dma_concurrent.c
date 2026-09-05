/*
 * Copyright (c) 2026 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify concurrent multichannel DMA transfers
 * @details
 * - Test Steps
 *   -# Acquire CONFIG_DMA_NUM_TEST_CHAN channels on the controller
 *   -# Configure a memory-to-memory transfer on every channel
 *   -# Start all channels, so every transfer is in flight at the same time
 *   -# Wait for one completion callback per channel
 * - Expected Results
 *   -# Every channel signals completion and copies its buffer intact
 */

#include <inttypes.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define NUM_CHAN     CONFIG_DMA_NUM_TEST_CHAN
#define XFER_SIZE    CONFIG_DMA_CONCURRENT_XFER_SIZE
#define BURST_LEN    CONFIG_DMA_CONCURRENT_BURST_LEN
#define XFER_TIMEOUT K_MSEC(CONFIG_DMA_CONCURRENT_TIMEOUT_MS)

#define DMA_TEST_NODE      DT_PATH(zephyr_user)
#define DMA_TEST_DEVS_PROP dma_test_devs

#if DT_NODE_HAS_PROP(DMA_TEST_NODE, DMA_TEST_DEVS_PROP)
/* Boards list the DMA controllers to test in a zephyr,user dma-test-devs
 * phandle list.
 */
#define DMA_TEST_DEV_COUNT DT_PROP_LEN(DMA_TEST_NODE, DMA_TEST_DEVS_PROP)
#define DMA_TEST_DEV_GET(idx, _)                                                                   \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(DMA_TEST_NODE, DMA_TEST_DEVS_PROP, idx))
#define DMA_TEST_DEV0_NODE DT_PHANDLE_BY_IDX(DMA_TEST_NODE, DMA_TEST_DEVS_PROP, 0)
#else
/* Legacy single-controller boards use a tst_dma0 devicetree label. */
#define DMA_TEST_DEV_COUNT 1
#define DMA_TEST_DEV_GET(idx, _) DEVICE_DT_GET(DT_NODELABEL(tst_dma0))
#define DMA_TEST_DEV0_NODE DT_NODELABEL(tst_dma0)
#endif

#define DMA_DATA_ALIGNMENT DT_PROP_OR(DMA_TEST_DEV0_NODE, dma_buf_addr_alignment, 32)

static const struct device *const dma_test_devs[] = {
	LISTIFY(DMA_TEST_DEV_COUNT, DMA_TEST_DEV_GET, (,))
};

#if CONFIG_NOCACHE_MEMORY
static __aligned(DMA_DATA_ALIGNMENT) uint8_t tx_buf[NUM_CHAN][XFER_SIZE] __used
	__attribute__((__section__(".nocache")));
static __aligned(DMA_DATA_ALIGNMENT) uint8_t rx_buf[NUM_CHAN][XFER_SIZE] __used
	__attribute__((__section__(".nocache.dma")));
#else
static __aligned(DMA_DATA_ALIGNMENT) uint8_t tx_buf[NUM_CHAN][XFER_SIZE];
static __aligned(DMA_DATA_ALIGNMENT) uint8_t rx_buf[NUM_CHAN][XFER_SIZE] = { { 0 } };
#endif

static struct dma_block_config blk_cfg[NUM_CHAN];

static K_SEM_DEFINE(xfer_sem, 0, NUM_CHAN);
static volatile int dma_complete_status;

static void dma_test_callback(const struct device *dma_dev, void *arg, uint32_t id, int status)
{
	ARG_UNUSED(dma_dev);
	ARG_UNUSED(arg);

	if (status < 0) {
		TC_PRINT("callback status %d on channel %u\n", status, id);
		dma_complete_status = status;
	}

	k_sem_give(&xfer_sem);
}

static int test_concurrent(const struct device *dma)
{
	struct dma_config dma_cfg = { 0 };
	int chan_ids[NUM_CHAN];
	int res = TC_PASS;

	if (!device_is_ready(dma)) {
		TC_PRINT("dma controller device is not ready\n");
		return TC_FAIL;
	}

	TC_PRINT("DMA concurrent transfer started on %s\n", dma->name);
	TC_PRINT("Testing %d channels, %d bytes/block, burst %d, alignment %d\n", NUM_CHAN,
		 XFER_SIZE, BURST_LEN, DMA_DATA_ALIGNMENT);

	/* Generate TX data and clear RX buffers. */
	for (int c = 0; c < NUM_CHAN; c++) {
		for (int i = 0; i < XFER_SIZE; i++) {
			tx_buf[c][i] = (uint8_t)(c ^ i);
		}
		memset(rx_buf[c], 0, XFER_SIZE);
	}

	dma_complete_status = 0;
	k_sem_reset(&xfer_sem);

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = BURST_LEN;
	dma_cfg.dest_burst_length = BURST_LEN;
	dma_cfg.dma_callback = dma_test_callback;
	dma_cfg.complete_callback_en = 1U;
	dma_cfg.error_callback_dis = 0U;
	dma_cfg.block_count = 1U;
#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	/* Acquire every channel up front. */
	for (int c = 0; c < NUM_CHAN; c++) {
		chan_ids[c] = dma_request_channel(dma, NULL);
		if (chan_ids[c] >= 0) {
			TC_PRINT("Acquired channel %d (index %d)\n", chan_ids[c], c);
			continue;
		}

		if (c > 0) {
			/* Exit the test if we cannot acquire all channels,
			 * release any that were acquired.
			 */
			TC_PRINT("only %d of %d channels available on %s\n", c, NUM_CHAN,
				 dma->name);
			for (int i = 0; i < c; i++) {
				dma_release_channel(dma, chan_ids[i]);
			}
			ztest_test_skip();
			return TC_SKIP;
		}

		/* Fall back to a fixed block of consecutive indices */
		TC_PRINT("platform does not support dma_request_channel(), "
			 "using CONFIG_DMA_CONCURRENT_CHANNEL_NR + index\n");
		for (int i = 0; i < NUM_CHAN; i++) {
			chan_ids[i] = CONFIG_DMA_CONCURRENT_CHANNEL_NR + i;
			TC_PRINT("Using channel %d (index %d)\n", chan_ids[i], i);
		}
		break;
	}

	for (int c = 0; c < NUM_CHAN; c++) {
		TC_PRINT("Configuring channel %d\n", chan_ids[c]);

		memset(&blk_cfg[c], 0, sizeof(blk_cfg[c]));
		blk_cfg[c].block_size = XFER_SIZE;
#ifdef CONFIG_DMA_64BIT
		blk_cfg[c].source_address = (uint64_t)tx_buf[c];
		blk_cfg[c].dest_address = (uint64_t)rx_buf[c];
		TC_PRINT("  source addr 0x%" PRIx64 ", dest addr 0x%" PRIx64 ", block_size %d\n",
			 blk_cfg[c].source_address, blk_cfg[c].dest_address, XFER_SIZE);
#else
		blk_cfg[c].source_address = (uint32_t)tx_buf[c];
		blk_cfg[c].dest_address = (uint32_t)rx_buf[c];
		TC_PRINT("  source addr 0x%x, dest addr 0x%x, block_size %d\n",
			 blk_cfg[c].source_address, blk_cfg[c].dest_address, XFER_SIZE);
#endif
		dma_cfg.head_block = &blk_cfg[c];

		if (dma_config(dma, chan_ids[c], &dma_cfg)) {
			TC_PRINT("ERROR: transfer config (%d)\n", chan_ids[c]);
			res = TC_FAIL;
			goto out;
		}
	}

	TC_PRINT("Starting all %d channels concurrently\n", NUM_CHAN);

	for (int c = 0; c < NUM_CHAN; c++) {
		if (dma_start(dma, chan_ids[c])) {
			TC_PRINT("ERROR: transfer start (%d)\n", chan_ids[c]);
			res = TC_FAIL;
			goto out;
		}
	}

	TC_PRINT("Waiting for %d completions\n", NUM_CHAN);

	for (int c = 0; c < NUM_CHAN; c++) {
		if (k_sem_take(&xfer_sem, XFER_TIMEOUT) != 0) {
			TC_PRINT("ERROR: timed out waiting for completions (%d of %d)\n", c,
				 NUM_CHAN);
			res = TC_FAIL;
			goto out;
		}
	}

	if (dma_complete_status < 0) {
		TC_PRINT("ERROR: DMA transfer error %d\n", dma_complete_status);
		res = TC_FAIL;
		goto out;
	}

	for (int c = 0; c < NUM_CHAN; c++) {
		TC_PRINT("Verifying rx_buf[%d]\n", c);
		if (memcmp(tx_buf[c], rx_buf[c], XFER_SIZE) != 0) {
			TC_PRINT("ERROR: data mismatch on channel %d\n", chan_ids[c]);
			res = TC_FAIL;
			goto out;
		}
	}

	TC_PRINT("Finished DMA: %s\n", dma->name);

out:
	for (int c = 0; c < NUM_CHAN; c++) {
		if (res != TC_PASS && dma_stop(dma, chan_ids[c])) {
			TC_PRINT("ERROR: transfer stop (%d)\n", chan_ids[c]);
		}
		dma_release_channel(dma, chan_ids[c]);
	}

	return res;
}

/* Generate one set of test cases per DMA controller under test so a failure
 * on one controller does not prevent the remaining controllers from running.
 */
#define DEFINE_DMA_CONCURRENT_TESTS(idx, _)                                                        \
	ZTEST(dma_m2m_concurrent, test_dma##idx##_m2m_concurrent_channels)                         \
	{                                                                                          \
		zassert_true(test_concurrent(dma_test_devs[idx]) == TC_PASS,                       \
			     "%s failed concurrent channel transfer",                              \
			     dma_test_devs[idx]->name);                                            \
	}

LISTIFY(DMA_TEST_DEV_COUNT, DEFINE_DMA_CONCURRENT_TESTS, ())
