/* dma.c - DMA test source file */

/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2021 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify zephyr dma memory to memory transfer loops
 * @details
 * - Test Steps
 *   -# Set dma channel configuration including source/dest addr, burstlen
 *   -# Set direction memory-to-memory
 *   -# Start transfer
 *   -# Move to next dest addr
 *   -# Back to first step
 * - Expected Results
 *   -# Data is transferred correctly from src to dest, for each loop
 */

#include <zephyr.h>

#include <device.h>
#include <drivers/dma.h>
#include <ztest.h>

/* in millisecond */
#define SLEEPTIME 1000

#define TRANSFER_LOOPS (5)
#define RX_BUFF_SIZE (64)

#if CONFIG_NOCACHE_MEMORY
static const char TX_DATA[] = "The quick brown fox jumps over the lazy dog";
static __aligned(16) char tx_data[64] __used
	__attribute__((__section__(".nocache")));
static __aligned(16) char rx_data[TRANSFER_LOOPS][RX_BUFF_SIZE] __used
	__attribute__((__section__(".nocache.dma")));
#else
/* this src memory shall be in RAM to support usingas a DMA source pointer.*/
static char tx_data[] =
	"The quick brown fox jumps over the lazy dog";
static __aligned(16) char rx_data[TRANSFER_LOOPS][RX_BUFF_SIZE] = { { 0 } };
#endif

#define DMA_DEVICE_NAME CONFIG_DMA_LOOP_TRANSFER_DRV_NAME

volatile uint8_t transfer_count;
static struct dma_config dma_cfg = {0};
static struct dma_block_config dma_block_cfg = {0};

static void test_transfer(const struct device *dev, uint32_t id)
{
	transfer_count++;
	if (transfer_count < TRANSFER_LOOPS) {
		dma_block_cfg.block_size = strlen(tx_data);
#ifdef CONFIG_DMA_64BIT
		dma_block_cfg.source_address = (uint64_t)tx_data;
		dma_block_cfg.dest_address = (uint64_t)rx_data[transfer_count];
#else
		dma_block_cfg.source_address = (uint32_t)tx_data;
		dma_block_cfg.dest_address = (uint32_t)rx_data[transfer_count];
#endif

		zassert_false(dma_config(dev, id, &dma_cfg),
					"Not able to config transfer %d",
					transfer_count + 1);
		zassert_false(dma_start(dev, id),
					"Not able to start next transfer %d",
					transfer_count + 1);
	}
}

static void dma_user_callback(const struct device *dma_dev, void *arg,
			      uint32_t id, int error_code)
{
	zassert_false(error_code, "DMA could not proceed, an error occurred\n");

#ifdef CONFIG_DMAMUX_STM32
	/* the channel is the DMAMUX's one
	 * the device is the DMAMUX, given through
	 * the stream->user_data by the dma_stm32_irq_handler
	 */
	test_transfer((struct device *)arg, id);
#else
	test_transfer(dma_dev, id);
#endif /* CONFIG_DMAMUX_STM32 */
}

static int test_loop(void)
{
	const struct device *dma;
	static int chan_id;

	TC_PRINT("DMA memory to memory transfer started on %s\n",
	       DMA_DEVICE_NAME);
	TC_PRINT("Preparing DMA Controller\n");

#if CONFIG_NOCACHE_MEMORY
	memset(tx_data, 0, sizeof(tx_data));
	memcpy(tx_data, TX_DATA, sizeof(TX_DATA));
#endif

	memset(rx_data, 0, sizeof(rx_data));

	dma = device_get_binding(DMA_DEVICE_NAME);
	if (!dma) {
		TC_PRINT("Cannot get dma controller\n");
		return TC_FAIL;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
#ifdef CONFIG_DMAMUX_STM32
	dma_cfg.user_data = (struct device *)dma;
#else
	dma_cfg.user_data = NULL;
#endif /* CONFIG_DMAMUX_STM32 */
	dma_cfg.dma_callback = dma_user_callback;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

#ifdef CONFIG_DMA_MCUX_TEST_SLOT_START
	dma_cfg.dma_slot = CONFIG_DMA_MCUX_TEST_SLOT_START;
#endif

	chan_id = dma_request_channel(dma, NULL);
	if (chan_id < 0) {
		TC_PRINT("this platform do not support the dma channel\n");
		chan_id = CONFIG_DMA_LOOP_TRANSFER_CHANNEL_NR;
	}
	transfer_count = 0;
	TC_PRINT("Starting the transfer on channel %d and waiting for 1 second\n", chan_id);
	dma_block_cfg.block_size = strlen(tx_data);
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)rx_data[transfer_count];

	if (dma_config(dma, chan_id, &dma_cfg)) {
		TC_PRINT("ERROR: transfer config (%d)\n", chan_id);
		return TC_FAIL;
	}

	if (dma_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer start (%d)\n", chan_id);
		return TC_FAIL;
	}

	k_sleep(K_MSEC(SLEEPTIME));

	if (transfer_count < TRANSFER_LOOPS) {
		transfer_count = TRANSFER_LOOPS;
		TC_PRINT("ERROR: unfinished transfer\n");
		if (dma_stop(dma, chan_id)) {
			TC_PRINT("ERROR: transfer stop\n");
		}
		return TC_FAIL;
	}

	TC_PRINT("Each RX buffer should contain the full TX buffer string.\n");

	for (int i = 0; i < TRANSFER_LOOPS; i++) {
		TC_PRINT("RX data Loop %d: %s\n", i, rx_data[i]);
		if (strncmp(tx_data, rx_data[i], sizeof(rx_data[i])) != 0) {
			return TC_FAIL;
		}
	}

	TC_PRINT("Finished: DMA\n");
	return TC_PASS;
}

/* export test cases */
void test_dma_m2m_loop(void)
{
	zassert_true((test_loop() == TC_PASS), NULL);
}
