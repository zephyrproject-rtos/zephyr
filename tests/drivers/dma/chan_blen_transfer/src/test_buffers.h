/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#define GUARD_BUF_SIZE (16)
#define TEST_BUF_SIZE (48)

extern char tx_data[TEST_BUF_SIZE];
extern char rx_data[TEST_BUF_SIZE + GUARD_BUF_SIZE];

#if CONFIG_DMA_TRANSFER_LONG_BLOCK_SIZE > 0
#define LONG_BUF_SIZE CONFIG_DMA_TRANSFER_LONG_BLOCK_SIZE

extern uint8_t long_tx_data[LONG_BUF_SIZE];
extern uint8_t long_rx_data[LONG_BUF_SIZE + GUARD_BUF_SIZE];
#endif
