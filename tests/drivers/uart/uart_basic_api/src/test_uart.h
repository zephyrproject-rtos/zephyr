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
#include <zephyr/ztest.h>

#if CONFIG_SHELL
void test_uart_configure(void);
void test_uart_config_get(void);
void test_uart_poll_out(void);
void test_uart_poll_in(void);
#if CONFIG_UART_WIDE_DATA
void test_uart_configure_wide(void);
void test_uart_config_get_wide(void);
#endif
#if CONFIG_UART_INTERRUPT_DRIVEN
void test_uart_fifo_fill(void);
void test_uart_fifo_read(void);
void test_uart_pending(void);
#endif

#endif

#endif /* __TEST_UART_H__ */
