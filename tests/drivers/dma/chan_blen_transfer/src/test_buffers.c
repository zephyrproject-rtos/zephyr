/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "test_buffers.h"

__aligned(CONFIG_DMA_BLEN_ALIGNMENT) char tx_data[TEST_BUF_SIZE] =
	"It is harder to be kind than to be wise........";
__aligned(CONFIG_DMA_BLEN_ALIGNMENT) char rx_data[TEST_BUF_SIZE] = {0};
