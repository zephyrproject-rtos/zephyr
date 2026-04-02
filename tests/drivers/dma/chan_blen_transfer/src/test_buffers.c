/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "test_buffers.h"

#define DMA_DATA_ALIGNMENT DT_INST_PROP_OR(tst_dma0, dma_buf_addr_alignment, 32)

__aligned(DMA_DATA_ALIGNMENT) char tx_data[TEST_BUF_SIZE] =
	"It is harder to be kind than to be wise........";
__aligned(DMA_DATA_ALIGNMENT) char rx_data[TEST_BUF_SIZE + GUARD_BUF_SIZE] = {0};
