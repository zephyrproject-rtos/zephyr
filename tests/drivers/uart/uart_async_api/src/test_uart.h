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

#include <zephyr/drivers/uart.h>
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
#elif defined(CONFIG_BOARD_NUCLEO_F103RB) || \
	defined(CONFIG_BOARD_NUCLEO_G071RB) || \
	defined(CONFIG_BOARD_NUCLEO_G474RE) || \
	defined(CONFIG_BOARD_NUCLEO_WL55JC)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(usart1))
#elif defined(CONFIG_BOARD_NUCLEO_H723ZG) || \
	defined(CONFIG_BOARD_NUCLEO_H743ZI) || \
	defined(CONFIG_BOARD_STM32F3_DISCO)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(usart2))
#elif defined(CONFIG_BOARD_NUCLEO_L4R5ZI) || \
	defined(CONFIG_BOARD_NUCLEO_L152RE) || \
	defined(CONFIG_BOARD_STM32L562E_DK) || \
	defined(CONFIG_BOARD_NUCLEO_L552ZE_Q)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(usart3))
#elif defined(CONFIG_BOARD_DISCO_L475_IOT1)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(uart4))
#elif defined(CONFIG_BOARD_NUCLEO_F429ZI) || \
	defined(CONFIG_BOARD_NUCLEO_F207ZG) || \
	defined(CONFIG_BOARD_NUCLEO_F767ZI) || \
	defined(CONFIG_BOARD_NUCLEO_F746ZG)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(usart6))
#elif defined(CONFIG_BOARD_FRDM_K82F)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(lpuart0))
#elif defined(CONFIG_BOARD_MIMXRT1020_EVK) || \
	defined(CONFIG_BOARD_MIMXRT1024_EVK) || \
	defined(CONFIG_BOARD_MIMXRT1160_EVK_CM4) || \
	defined(CONFIG_BOARD_MIMXRT1170_EVK_CM4) || \
	defined(CONFIG_BOARD_MIMXRT1160_EVK_CM7) || \
	defined(CONFIG_BOARD_MIMXRT1170_EVK_CM7) || \
	defined(CONFIG_BOARD_TWR_KE18F)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(lpuart2))
#elif defined(CONFIG_BOARD_MIMXRT1050_EVK) || \
	defined(CONFIG_BOARD_MIMXRT1060_EVK) || \
	defined(CONFIG_BOARD_MIMXRT1060_EVKB) || \
	defined(CONFIG_BOARD_MIMXRT1064_EVK)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(lpuart3))
#elif defined(CONFIG_BOARD_MIMXRT1010_EVK) || \
	defined(CONFIG_BOARD_MIMXRT1015_EVK)
#define UART_DEVICE_NAME DT_LABEL(DT_NODELABEL(lpuart4))
#else
#define UART_DEVICE_NAME DT_LABEL(DT_CHOSEN(zephyr_console))
#endif

void init_test(void);

void test_single_read(void);
void test_multiple_rx_enable(void);
void test_chained_read(void);
void test_double_buffer(void);
void test_read_abort(void);
void test_write_abort(void);
void test_forever_timeout(void);
void test_long_buffers(void);
void test_chained_write(void);

void test_single_read_setup(void);
void test_multiple_rx_enable_setup(void);
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
