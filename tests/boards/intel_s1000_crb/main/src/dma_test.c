/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Sample app to illustrate dma transfer on Intel S1000 CRB.
 *
 * Intel S1000 CRB - Xtensa
 * --------------------
 *
 * The dma_cavs driver is being used.
 *
 * In this sample app, multi-block dma is tested in the following manner
 *   - Define 2 strings which will serve as 2 blocks of source data.
 *   - Define 2 empty buffers to receive the data from the DMA operation.
 *   - Set dma channel configuration including source/dest addr, burstlen etc.
 *   - Set direction memory-to-memory
 *   - Start transfer
 *
 * Expected Results
 *   - Data is transferred correctly from src to dest. The DMAed string should
 *     be printed on to the console. No error should be seen.
 */

#include <zephyr.h>
#include <sys/printk.h>

#include <device.h>
#include <drivers/dma.h>

#include <string.h>
#include <xtensa/hal.h>

/* size of stack area used by each thread */
#define STACKSIZE               1024

/* scheduling priority used by each thread */
#define PRIORITY                7

/* delay between greetings (in ms) */
#define SLEEPTIME               K_MSEC(500)

/* max time to be waited for dma to complete (in ms) */
#define WAITTIME                K_MSEC(1000)

#define MAX_TRANSFERS		4

/* This semaphore is used as a signal from the dma isr to the app
 * to let it know the DMA is complete. The app should wait till
 * this event comes indicating the completion of DMA.
 */
K_SEM_DEFINE(dma_sem, 0, 1);

extern struct k_sem thread_sem;

#define DMA_DEVICE_NAME		CONFIG_DMA_0_NAME
#define RX_BUFF_SIZE		(48)

struct transfers {
	const char *source;
	char *destination;
	size_t size;
};

static const char tx_data[] = "It is harder to be kind than to be wise";
static const char tx_data2[] = "India have a good cricket team";
static const char tx_data3[] = "Virat: the best ever?";
static const char tx_data4[] = "Phenomenon";
static char rx_data[RX_BUFF_SIZE] = { 0 };
static char rx_data2[RX_BUFF_SIZE] = { 0 };
static char rx_data3[RX_BUFF_SIZE] = { 0 };
static char rx_data4[RX_BUFF_SIZE] = { 0 };

static struct transfers transfer_blocks[MAX_TRANSFERS] = {
	{
		.source = tx_data,
		.destination = rx_data,
		.size = sizeof(tx_data),
	},
	{
		.source = tx_data2,
		.destination = rx_data2,
		.size = sizeof(tx_data2),
	},
	{
		.source = tx_data3,
		.destination = rx_data3,
		.size = sizeof(tx_data3),
	},
	{
		.source = tx_data4,
		.destination = rx_data4,
		.size = sizeof(tx_data4),
	},
};
static const struct device *dma_device;
static uint32_t current_block_count, total_block_count;

static void test_done(const struct device *dma_dev, void *arg,
		      uint32_t channel, int error_code)
{
	uint32_t src, dst;
	size_t size;

	current_block_count++;

	if (error_code != 0) {
		printk("DMA transfer met an error = 0x%x\n", error_code);
		k_sem_give(&dma_sem);
	} else if (current_block_count < total_block_count) {
		src = (uint32_t)transfer_blocks[current_block_count].source;
		dst = (uint32_t)transfer_blocks[current_block_count].destination;
		size = transfer_blocks[current_block_count].size;
		dma_reload(dma_dev, channel, src, dst, size);
		dma_start(dma_dev, channel);
	} else {
		printk("DMA transfer done\n");
		k_sem_give(&dma_sem);
	}
}

static int test_task(uint32_t chan_id, uint32_t blen, uint32_t block_count)
{
	struct dma_config dma_cfg = {0};

	struct dma_block_config dma_block_cfg = {
		.block_size = sizeof(tx_data),
		.source_address = (uint32_t)tx_data,
		.dest_address = (uint32_t)rx_data,
	};

	if (block_count > ARRAY_SIZE(transfer_blocks)) {
		printk("block_count %u is greater than %ld\n", block_count,
			ARRAY_SIZE(transfer_blocks));
		return -1;
	}

	dma_device = device_get_binding(DMA_DEVICE_NAME);

	if (!dma_device) {
		printk("Cannot get dma controller\n");
		return -1;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = blen;
	dma_cfg.dest_burst_length = blen;
	dma_cfg.dma_callback = test_done;
	dma_cfg.complete_callback_en = 0U;
	dma_cfg.error_callback_en = 1U;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

	printk("Preparing DMA Controller: Chan_ID=%u, BURST_LEN=%u\n",
			chan_id, blen);

	(void)memset(rx_data, 0, sizeof(rx_data));
	(void)memset(rx_data2, 0, sizeof(rx_data2));
	(void)memset(rx_data3, 0, sizeof(rx_data3));
	(void)memset(rx_data4, 0, sizeof(rx_data4));

	/*
	 * dma_block_cfg4 is assigned to 0 by default. Hence if user_data is
	 * not assigned, it will be NULL implying there are no more blocks to
	 * transfer
	 */

	if (dma_config(dma_device, chan_id, &dma_cfg)) {
		printk("ERROR: configuring\n");
		return -1;
	}

	printk("Starting the transfer\n");

	current_block_count = 0U;
	total_block_count = block_count;

	if (dma_start(dma_device, chan_id)) {
		printk("ERROR: transfer\n");
		return -1;
	}

	/* Wait a while for the dma to complete */
	if (k_sem_take(&dma_sem, WAITTIME)) {
		printk("*** timed out waiting for dma to complete ***\n");
	}

	xthal_dcache_region_invalidate(rx_data, RX_BUFF_SIZE);
	xthal_dcache_region_invalidate(rx_data2, RX_BUFF_SIZE);
	xthal_dcache_region_invalidate(rx_data3, RX_BUFF_SIZE);
	xthal_dcache_region_invalidate(rx_data4, RX_BUFF_SIZE);

	/* Intentionally break has been omitted (fall-through) */
	switch (block_count) {
	case 4:
		if (strcmp(tx_data4, rx_data4) != 0) {
			return -1;
		}
		printk("%s\n", rx_data4);

	case 3:
		if (strcmp(tx_data3, rx_data3) != 0) {
			return -1;
		}
		printk("%s\n", rx_data3);

	case 2:
		if (strcmp(tx_data2, rx_data2) != 0) {
			return -1;
		}
		printk("%s\n", rx_data2);

	case 1:
		if (strcmp(tx_data, rx_data) != 0) {
			return -1;
		}
		printk("%s\n", rx_data);
		break;

	default:
		printk("Invalid block count %d\n", block_count);
		return -1;
	}

	return 0;
}

/* export test cases */
void dma_thread(void)
{
	while (1) {
		k_sem_take(&thread_sem, K_FOREVER);
		if (test_task(0, 8, 2) == 0) {
			printk("DMA Passed\n");
		} else {
			printk("DMA Failed\n");
		}
		k_sem_give(&thread_sem);
		k_sleep(SLEEPTIME);

		k_sem_take(&thread_sem, K_FOREVER);
		if (test_task(1, 8, 3) == 0) {
			printk("DMA Passed\n");
		} else {
			printk("DMA Failed\n");
		}
		k_sem_give(&thread_sem);
		k_sleep(SLEEPTIME);

		k_sem_take(&thread_sem, K_FOREVER);
		if (test_task(0, 16, 4) == 0) {
			printk("DMA Passed\n");
		} else {
			printk("DMA Failed\n");
		}
		k_sem_give(&thread_sem);
		k_sleep(SLEEPTIME);

		k_sem_take(&thread_sem, K_FOREVER);
		if (test_task(1, 16, 1) == 0) {
			printk("DMA Passed\n");
		} else {
			printk("DMA Failed\n");
		}
		k_sem_give(&thread_sem);
		k_sleep(SLEEPTIME);
	}
}

K_THREAD_DEFINE(dma_thread_id, STACKSIZE, dma_thread, NULL, NULL, NULL,
		PRIORITY, 0, 0);
