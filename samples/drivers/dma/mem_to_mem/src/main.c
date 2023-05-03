/*
 * Copyright (c) 2023 Intel Corporation

 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/sys/printk.h>
#include <string.h>

#if defined(CONFIG_DMA_DW_AXI)
#define DMA DT_COMPAT_GET_ANY_STATUS_OKAY(snps_designware_dma_axi)
#define DMA_CHANNEL 1U
#else
#error "Unsupported DMA Driver"
#endif

/* Number of block to transfer */
#ifndef XFERS
#define XFERS 10U
#endif

/* Number of bytes per block to transfer */
#ifndef XFER_SIZE
#define XFER_SIZE 200U
#endif

/* transfer width */
#ifndef DATA_SIZE
#define DATA_SIZE 4U
#endif

/* burst transaction length */
#ifndef BURST_LEN
#define BURST_LEN 4U
#endif

/* dma channel */
#ifndef DMA_CHANNEL
#define DMA_CHANNEL 0U
#endif

/* dma transfer status */
volatile int xfer_status;

static struct k_sem dma_complete;

/* transfer and receive buffers */
static uint8_t tx_data[XFERS][XFER_SIZE];
static uint8_t rx_data[XFERS][XFER_SIZE];

static void dma_callback(const struct device *dev, void *user_data, uint32_t channel, int status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	ARG_UNUSED(channel);

	xfer_status = status;
	(void)k_sem_give(&dma_complete);
}

int main(void)
{
	int ret;
	uint8_t i, j;
	struct dma_config dma_cfg = {0};
	static struct dma_block_config dma_block_cfgs[XFERS];
	const struct device *const dma_dev = DEVICE_DT_GET(DMA);

	printk("Sample application for Memory to Memory transfer using dma controller\n");

	/* check if the DMA instance is initialized and ready */
	if (!device_is_ready(dma_dev)) {
		printk("DMA driver is not ready\n");
		return -ENODEV;
	}

	/* write transfer buffer content to transfer using dma */
	for (i = 0; i < XFERS; i++) {
		for (j = 0; j < XFER_SIZE; j++) {
			tx_data[i][j] = j;
		}
	}

	ret = k_sem_init(&dma_complete, 0, 1);
	if (ret != 0) {
		printk("Failed to initialize semaphore");
		return ret;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = DATA_SIZE;
	dma_cfg.dest_data_size = DATA_SIZE;
	dma_cfg.source_burst_length = BURST_LEN;
	dma_cfg.dest_burst_length = BURST_LEN;
	dma_cfg.block_count = XFERS;
	dma_cfg.head_block = dma_block_cfgs;
	dma_cfg.complete_callback_en = 0;
	dma_cfg.dma_callback = dma_callback;
	dma_cfg.user_data = NULL;

	/*
	 * dma callback will always be invoked when dma transfer completes without any error
	 *
	 * if there are any dma transfer errors, callback will be invoked if
	 * error_callback_en = 0
	 */
	dma_cfg.error_callback_en = 0;

	memset((void *)dma_block_cfgs, 0, sizeof(dma_block_cfgs));

	/* configure XFERS number of blocks */
	for (int i = 0; i < XFERS; i++) {
		dma_block_cfgs[i].block_size = XFER_SIZE;
		dma_block_cfgs[i].source_address = (uint64_t)(tx_data[i]);
		dma_block_cfgs[i].dest_address = (uint64_t)(rx_data[i]);

		/* configure the next block address to current block */
		if (i < XFERS - 1) {
			dma_block_cfgs[i].next_block = &dma_block_cfgs[i + 1];
		}
	}

	/* configure dma descriptors */
	ret = dma_config(dma_dev, DMA_CHANNEL, &dma_cfg);
	if (ret != 0) {
		printk("Failed to configure dma, ret:%d\n", ret);
		return ret;
	}

	/* start dma transfer */
	ret = dma_start(dma_dev, DMA_CHANNEL);
	if (ret != 0) {
		printk("dma start transfer failed, ret:%d\n", ret);
		return ret;
	}

	(void)k_sem_take(&dma_complete, K_FOREVER);

	/* check if dma transfer encountered any error */
	if (xfer_status) {
		printk("dma transfer encountered error status:%d\n", xfer_status);
		return xfer_status;
	}

	/* compare tx and rx buffer to check if the memory contents are same */
	if (!memcmp((void *)tx_data, (void *)rx_data, (XFERS * XFER_SIZE))) {
		printk("%s: Successfully transferred %d "
		       "blocks of size:%d channel:%d\n",
		       dma_dev->name, XFERS, XFER_SIZE, DMA_CHANNEL);
	} else {
		printk("Mismatch while performing DMA transfer\n");
		return -EIO;
	}

	printk("Sample application for dma transfer complete\n");

	return 0;
}
