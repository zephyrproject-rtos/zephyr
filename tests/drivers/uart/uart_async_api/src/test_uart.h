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

#if defined(CONFIG_BOARD_NRF52840DK_NRF52840)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(uart0))
#elif defined(CONFIG_BOARD_NRF9160DK_NRF9160)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(uart1))
#elif defined(CONFIG_BOARD_ATSAMD21_XPRO)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(sercom1))
#elif defined(CONFIG_BOARD_ATSAMR21_XPRO)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(sercom3))
#elif defined(CONFIG_BOARD_ATSAME54_XPRO)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(sercom1))
#else
#define UART_DEVICE_NAME CONFIG_UART_CONSOLE_ON_DEV_NAME
#endif

void init_test(void);

void test_single_read(void);
void test_chained_read(void);
void test_double_buffer(void);
void test_read_abort(void);
void test_write_abort(void);
void test_forever_timeout(void);
void test_long_buffers(void);
void test_chained_write(void);

void test_single_read_setup(void);
void test_chained_read_setup(void);
void test_double_buffer_setup(void);
void test_read_abort_setup(void);
void test_write_abort_setup(void);
void test_forever_timeout_setup(void);
void test_long_buffers_setup(void);
void test_chained_write_setup(void);

#ifdef CONFIG_USERSPACE
void set_permissions(void);
#endif

#endif /* __TEST_UART_H__ */
