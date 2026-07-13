/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "test_buffers.h"

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), dma_test_devs)
#define DMA_DATA_ALIGNMENT                                                                         \
	DT_PROP_OR(DT_PHANDLE_BY_IDX(DT_PATH(zephyr_user), dma_test_devs, 0),                     \
		   dma_buf_addr_alignment, 32)
#else
#define DMA_DATA_ALIGNMENT DT_PROP_OR(DT_NODELABEL(tst_dma0), dma_buf_addr_alignment, 32)
#endif

#if CONFIG_NOCACHE_MEMORY
__aligned(DMA_DATA_ALIGNMENT) char tx_data[TEST_BUF_SIZE] __used
	__attribute__((__section__(".nocache"))) =
		"It is harder to be kind than to be wise........";
__aligned(DMA_DATA_ALIGNMENT) char rx_data[TEST_BUF_SIZE + GUARD_BUF_SIZE] __used
	__attribute__((__section__(".nocache.dma"))) = {0};
#else
__aligned(DMA_DATA_ALIGNMENT) char tx_data[TEST_BUF_SIZE] =
	"It is harder to be kind than to be wise........";
__aligned(DMA_DATA_ALIGNMENT) char rx_data[TEST_BUF_SIZE + GUARD_BUF_SIZE] = {0};
#endif
