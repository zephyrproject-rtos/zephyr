/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UART cases header file
 *
 * Header file for UART cases
 */

#ifndef __TEST_UART_H__
#define __TEST_UART_H__

#include <drivers/uart.h>
#include <ztest.h>

/* RX and TX pins have to be connected together*/

#if defined(CONFIG_BOARD_NRF52840_PCA10056)
#define UART_DEVICE_NAME DT_UART_0_NAME
#elif defined(CONFIG_BOARD_NRF9160_PCA10090)
#define UART_DEVICE_NAME DT_UART_1_NAME
#else
#define UART_DEVICE_NAME CONFIG_UART_CONSOLE_ON_DEV_NAME
#endif

void test_single_read(void);
void test_chained_read(void);
void test_double_buffer(void);
void test_read_abort(void);
void test_write_abort(void);
void test_long_buffers(void);
void test_chained_write(void);

#endif /* __TEST_UART_H__ */
