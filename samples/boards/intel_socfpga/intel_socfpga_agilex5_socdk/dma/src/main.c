/*
 * Copyright (c) 2023 Intel Corporation

 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>
#include <string.h>

#if DT_HAS_COMPAT_STATUS_OKAY(snps_designware_dma_axi)
#define DMA_DEV_COMPAT snps_designware_dma_axi
#else
#error "Enable one DMA controller"
#endif

#define DEVICE_GENERATE_DEV_INFO(node_id) { \
			.dma_dev = DEVICE_DT_GET(node_id), \
			.max_channel = DT_PROP(node_id, dma_channels), \
		},

#define DEVS_FOR_DT_COMPAT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, DEVICE_GENERATE_DEV_INFO)

struct dma_ctrl_info {
	const struct device *dma_dev;
	uint32_t max_channel;
};

static struct dma_ctrl_info ctrl_info[] = {
	DEVS_FOR_DT_COMPAT(DMA_DEV_COMPAT)
};

#define DMA_COUNT ARRAY_SIZE(ctrl_info)

#define XFERS 50U
#define XFER_SIZE 200U

#define DATA_SIZE 4U
#define BURST_LEN 4U

static struct k_sem dma_complete;
static struct dma_block_config dma_block_cfgs[XFERS];

static __aligned(32) char tx_data[XFERS][XFER_SIZE];
static __aligned(32) char rx_data[XFERS][XFER_SIZE];

static void dma_callback(const struct device *dev, void *user_data, uint32_t channel, int status)
{

	if (status) {
		printk("DMA transfer encountered error status:%d\n", status);
		return;
	}

	k_sem_give(&dma_complete);
}

int main(void)
{
	struct dma_config dma_cfg = { 0 };
	uint32_t inst = 0, ch = 0;
	uint32_t i = 0, j = 0;
	int ret = 0;

	k_sem_init(&dma_complete, 0, 1);

	for (i = 0; i < XFERS; i++) {
		for (j = 0; j < XFER_SIZE; j++) {
			tx_data[i][j] = j;
		}
	}

	printk("DMA driver testing for Memory to Memory transfer\n");

	/* Test all DMA controller instances for memory to memory transfer */
	for (inst = 0; inst < DMA_COUNT; inst++) {
		/* Check if the DMA instance is initialized and ready */
		if (!device_is_ready(ctrl_info[inst].dma_dev)) {
			printk("DMA:%s is not ready\n", ctrl_info[inst].dma_dev->name);
			return -ENODEV;
		}

		for (ch = 0; ch < ctrl_info[inst].max_channel; ch++) {

			memset((void *)rx_data, 0, sizeof(rx_data));

			dma_cfg.channel_direction = MEMORY_TO_MEMORY;
			/* memory transfer is optimal for 4-byte transfer */
			dma_cfg.source_data_size = DATA_SIZE;
			dma_cfg.dest_data_size = DATA_SIZE;
			dma_cfg.source_burst_length = BURST_LEN;
			dma_cfg.dest_burst_length = BURST_LEN;
			dma_cfg.block_count = XFERS;
			dma_cfg.head_block = dma_block_cfgs;
			dma_cfg.complete_callback_en = 0;
			dma_cfg.dma_callback = dma_callback;

			memset((void *)dma_block_cfgs, 0, sizeof(dma_block_cfgs));

			for (int i = 0; i < XFERS; i++) {
				dma_block_cfgs[i].block_size = sizeof(tx_data);
				dma_block_cfgs[i].source_address = (uint64_t)(tx_data);
				dma_block_cfgs[i].dest_address = (uint64_t)(rx_data);

				if (i < XFERS - 1) {
					dma_block_cfgs[i].next_block = &dma_block_cfgs[i+1];
				}
			}

			ret = dma_config(ctrl_info[inst].dma_dev, (ch + 1), &dma_cfg);
			if (ret != 0) {
				printk("Failed to configure dma, ret:%d\n",  ret);
				return ret;
			}

			ret = dma_start(ctrl_info[inst].dma_dev, (ch + 1));
			if (ret != 0) {
				printk("dma transfer failed, ret:%d\n", ret);
				return ret;
			}

			k_sem_take(&dma_complete, K_FOREVER);

			if (!memcmp((void *)tx_data, (void *)rx_data, (XFERS * XFER_SIZE))) {
				printk("%s: Successfully transferred %d "
					"blocks of size:%d channel:%d\n",
					ctrl_info[inst].dma_dev->name,
					XFERS, XFER_SIZE, (ch + 1));
			} else {
				printk("Mismatch while doing DMA transfer\n");
				return -1;
			}
		}
	}

	printk("DMA testing complete\n");

	return 0;
}
