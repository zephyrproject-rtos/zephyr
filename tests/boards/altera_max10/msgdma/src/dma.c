/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <soc.h>
#include <kernel_structs.h>
#include <device.h>
#include <drivers/dma.h>

#define DMA_BUFF_SIZE		1024

enum dma_op_status {
	DMA_OP_STAT_NONE = 0,
	DMA_OP_STAT_ERR,
	DMA_OP_STAT_SUCCESS,
};

static enum dma_op_status dma_stat;

static char tx_data[DMA_BUFF_SIZE];
static char rx_data[DMA_BUFF_SIZE];

static struct dma_config dma_cfg = {0};
static struct dma_block_config dma_block_cfg = {0};

static void dma_user_callback(void *arg, u32_t id, int error_code)
{
	if (error_code == 0) {
		TC_PRINT("DMA completed successfully\n");
		dma_stat = DMA_OP_STAT_SUCCESS;
	} else {
		TC_PRINT("DMA error occurred!! (%d)\n", error_code);
		dma_stat = DMA_OP_STAT_ERR;
	}
}

void test_msgdma(void)
{
	struct device *dma;
	static u32_t chan_id;
	int i;

	dma = device_get_binding(CONFIG_DMA_0_NAME);
	zassert_true(dma != NULL, "DMA_0 device not found!!");

	/* Init tx buffer */
	for (i = 0; i < DMA_BUFF_SIZE; i++) {
		tx_data[i] = i;
	}

	/* Init DMA config info */
	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1U;
	dma_cfg.dest_data_size = 1U;
	dma_cfg.source_burst_length = 1U;
	dma_cfg.dest_burst_length = 1U;
	dma_cfg.dma_callback = dma_user_callback;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

	/*
	 * Set channel id to 0 as Nios-II
	 * MSGDMA only supports one channel
	 */
	chan_id = 0U;

	/* Init DMA descriptor info */
	dma_block_cfg.block_size = DMA_BUFF_SIZE;
	dma_block_cfg.source_address = (u32_t)tx_data;
	dma_block_cfg.dest_address = (u32_t)rx_data;

	/* Configure DMA */
	zassert_true(dma_config(dma, chan_id, &dma_cfg) == 0,
						"DMA config error");

	/* Make sure all the data is written out to memory */
	z_nios2_dcache_flush_all();

	/* Start DMA operation */
	zassert_true(dma_start(dma, chan_id) == 0, "DMA start error");

	while (dma_stat == DMA_OP_STAT_NONE) {
		k_busy_wait(10);
	}

	/* Invalidate the data cache */
	z_nios2_dcache_flush_no_writeback(rx_data, DMA_BUFF_SIZE);

	zassert_true(dma_stat == DMA_OP_STAT_SUCCESS,
			"Nios-II DMA operation failed!!");

	zassert_true(!memcmp(&tx_data, &rx_data, DMA_BUFF_SIZE),
					"Nios-II DMA Test failed!!");

}

void test_main(void)
{
	ztest_test_suite(nios2_msgdma_test,
			 ztest_unit_test(test_msgdma));
	ztest_run_test_suite(nios2_msgdma_test);
}
