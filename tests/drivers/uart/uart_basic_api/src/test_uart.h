/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Intel Corporation
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

#include <zephyr/drivers/uart.h>
#include <ztest.h>

void test_uart_configure(void);
void test_uart_config_get(void);
void test_uart_poll_out(void);
void test_uart_fifo_fill(void);
void test_uart_fifo_read(void);
void test_uart_poll_in(void);
void test_uart_pending(void);

#endif /* __TEST_UART_H__ */
