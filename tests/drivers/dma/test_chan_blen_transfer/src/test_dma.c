/*
 * Copyright (c) 2016 Intel Corporation
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

/**
 * @addtogroup t_dma_mem_to_mem
 * @{
 * @defgroup t_dma_m2m_chan_burst test_dma_m2m_chan_burst
 * @brief TestPurpose: verify zephyr dma memory to memory transfer
 * @details
 * - Test Steps
 *   -# Set dma channel configuration including source/dest addr, burstlen
 *   -# Set direction memory-to-memory
 *   -# Start transter
 * - Expected Results
 *   -# Data is transferred correctly from src to dest
 * @}
 */

#include <zephyr.h>
#include <dma.h>
#include <ztest.h>

#define DMA_DEVICE_NAME CONFIG_DMA_0_NAME
#define RX_BUFF_SIZE (48)

static const char tx_data[] = "It is harder to be kind than to be wise";
static char rx_data[RX_BUFF_SIZE] = { 0 };

static void test_done(struct device *dev, void *data)
{
	TC_PRINT("DMA transfer done\n");
}

static void test_error(struct device *dev, void *data)
{
	TC_PRINT("DMA transfer met an error\n");
}

static int test_task(uint32_t chan_id, uint32_t blen)
{
	enum dma_burst_length burst_len = BURST_TRANS_LENGTH_1;
	struct dma_channel_config dma_chan_cfg = {0};
	struct dma_transfer_config dma_trans = {0};
	struct device *dma = device_get_binding(DMA_DEVICE_NAME);

	if (!dma) {
		TC_PRINT("Cannot get dma controller\n");
		return TC_FAIL;
	}

	switch (blen) {
	case 8:
		burst_len = BURST_TRANS_LENGTH_8;
		break;
	case 16:
		burst_len = BURST_TRANS_LENGTH_16;
		break;
	default:
		burst_len = BURST_TRANS_LENGTH_1;
	}

	dma_chan_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_chan_cfg.source_transfer_width = TRANS_WIDTH_8;
	dma_chan_cfg.destination_transfer_width = TRANS_WIDTH_8;
	dma_chan_cfg.source_burst_length = burst_len;
	dma_chan_cfg.destination_burst_length = burst_len;
	dma_chan_cfg.dma_transfer = test_done;
	dma_chan_cfg.dma_error = test_error;
	dma_chan_cfg.callback_data = (void *)&chan_id;

	TC_PRINT("Preparing DMA Controller: Chan_ID=%u, BURST_LEN=%u\n",
			chan_id, burst_len);
	if (dma_channel_config(dma, chan_id, &dma_chan_cfg)) {
		TC_PRINT("Error: configuration\n");
		return TC_FAIL;
	}

	TC_PRINT("Starting the transfer\n");
	dma_trans.block_size = strlen(tx_data);
	dma_trans.source_address = (uint32_t *)tx_data;
	dma_trans.destination_address = (uint32_t *)rx_data;

	if (dma_transfer_config(dma, chan_id, &dma_trans)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}

	if (dma_transfer_start(dma, chan_id)) {
		TC_PRINT("ERROR: transfer\n");
		return TC_FAIL;
	}
	k_sleep(2000);
	TC_PRINT("%s\n", rx_data);
	if (strcmp(tx_data, rx_data) != 0)
		return TC_FAIL;
	return TC_PASS;
}

/* export test cases */
void test_dma_m2m_chan0_burst8(void)
{
	assert_true((test_task(0, 8) == TC_PASS), NULL);
}

void test_dma_m2m_chan1_burst8(void)
{
	assert_true((test_task(1, 8) == TC_PASS), NULL);
}

void test_dma_m2m_chan0_burst16(void)
{
	assert_true((test_task(0, 16) == TC_PASS), NULL);
}

void test_dma_m2m_chan1_burst16(void)
{
	assert_true((test_task(1, 16) == TC_PASS), NULL);
}
