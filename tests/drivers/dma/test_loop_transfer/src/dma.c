/* dma.c - DMA test source file */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>

#include <device.h>
#include <dma.h>
#include <misc/printk.h>
#include <string.h>

/* in millisecond */
#define SLEEPTIME  1000

#define TRANSFER_LOOPS (5)
#define RX_BUFF_SIZE (50)

static const char tx_data[] = "The quick brown fox jumps over the lazy dog";
static char rx_data[TRANSFER_LOOPS][RX_BUFF_SIZE] = {{ 0 } };

#define DMA_DEVICE_NAME "DMA_0"

volatile uint8_t transfer_count;

static void test_transfer(struct device *dev, void *data)
{
	int ret;
	struct dma_transfer_config dma_trans = {0};
	uint32_t *chan_id = data;

	transfer_count++;
	if (transfer_count < TRANSFER_LOOPS) {
		dma_trans.block_size = strlen(tx_data);
		dma_trans.source_address = (uint32_t *)tx_data;
		dma_trans.destination_address =
			    (uint32_t *)rx_data[transfer_count];

		ret = dma_transfer_config(dev, *chan_id, &dma_trans);
		if (ret == 0) {
			dma_transfer_start(dev, *chan_id);
		}
	}
}

static void test_error(struct device *dev, void *data)
{
	printk("DMA could not proceed, an error occurred\n");
}

void main(void)
{
	struct device *dma;
	static uint32_t chan_id;
	struct dma_channel_config dma_chan_cfg = {0};
	struct dma_transfer_config dma_trans = {0};

	printk("DMA memory to memory transfer started on %s\n", DMA_DEVICE_NAME);
	printk("Preparing DMA Controller\n");

	dma = device_get_binding(DMA_DEVICE_NAME);
	if (!dma) {
		printk("Cannot get dma controller\n");
		return;
	}

	dma_chan_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_chan_cfg.source_transfer_width = TRANS_WIDTH_8;
	dma_chan_cfg.destination_transfer_width = TRANS_WIDTH_8;
	dma_chan_cfg.source_burst_length = BURST_TRANS_LENGTH_1;
	dma_chan_cfg.destination_burst_length = BURST_TRANS_LENGTH_1;
	dma_chan_cfg.dma_transfer = test_transfer;
	dma_chan_cfg.dma_error = test_error;

	chan_id = 0;
	dma_chan_cfg.callback_data = (void *)&chan_id;

	if (dma_channel_config(dma, chan_id, &dma_chan_cfg)) {
		printk("Error: configuration\n");
		return;
	}

	printk("Starting the transfer and waiting for 1 second\n");
	dma_trans.block_size = strlen(tx_data);
	dma_trans.source_address = (uint32_t *)tx_data;
	dma_trans.destination_address = (uint32_t *)rx_data[transfer_count];

	if (dma_transfer_config(dma, chan_id, &dma_trans)) {
		printk("ERROR: transfer config\n");
		return;
	}

	if (dma_transfer_start(dma, chan_id)) {
		printk("ERROR: transfer start\n");
		return;
	}

	k_sleep(SLEEPTIME);

	if (transfer_count < TRANSFER_LOOPS) {
		transfer_count = TRANSFER_LOOPS;
		printk("ERROR: unfinished transfer\n");
		if (dma_transfer_stop(dma, chan_id)) {
			printk("ERROR: transfer stop\n");
		}
	}

	printk("Each RX buffer should contain the full TX buffer string.\n");
	printk("TX data: %s\n", tx_data);

	for (int i = 0; i < TRANSFER_LOOPS; i++) {
		printk("RX data Loop %d: %s\n", i, rx_data[i]);
	}

	printk("Finished: DMA\n");
}
