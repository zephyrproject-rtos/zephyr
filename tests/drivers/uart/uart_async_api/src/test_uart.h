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
#include <zephyr/ztest.h>

/* RX and TX pins have to be connected together*/

/* RTT is first so that when testing RTT we ensure it
 * get selected over any other SoC specific uart
 */
#if defined(CONFIG_UART_RTT)
#define UART_DEVICE_DEV DT_NODELABEL(rtt0)
#elif defined(CONFIG_BOARD_NRF52840DK_NRF52840)
#define UART_DEVICE_DEV DT_NODELABEL(uart0)
#elif defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)
#define UART_DEVICE_DEV DT_NODELABEL(uart1)
#elif defined(CONFIG_BOARD_NRF9160DK_NRF9160)
#define UART_DEVICE_DEV DT_NODELABEL(uart1)
#elif defined(CONFIG_BOARD_ATSAMD21_XPRO)
#define UART_DEVICE_DEV DT_NODELABEL(sercom1)
#elif defined(CONFIG_BOARD_ATSAMR21_XPRO)
#define UART_DEVICE_DEV DT_NODELABEL(sercom3)
#elif defined(CONFIG_BOARD_ATSAML21_XPRO)
#define UART_DEVICE_NAME DT_NODELABEL(sercom1)
#elif defined(CONFIG_BOARD_ATSAMR34_XPRO)
#define UART_DEVICE_NAME DT_NODELABEL(sercom2)
#elif defined(CONFIG_BOARD_ATSAME54_XPRO)
#define UART_DEVICE_DEV DT_NODELABEL(sercom1)
#elif defined(CONFIG_BOARD_NUCLEO_F103RB) || \
	defined(CONFIG_BOARD_NUCLEO_G071RB) || \
	defined(CONFIG_BOARD_NUCLEO_G474RE) || \
	defined(CONFIG_BOARD_NUCLEO_WL55JC)
#define UART_DEVICE_DEV DT_NODELABEL(usart1)
#elif defined(CONFIG_BOARD_NUCLEO_H723ZG) || \
	defined(CONFIG_BOARD_NUCLEO_H743ZI) || \
	defined(CONFIG_BOARD_STM32F3_DISCO) || \
	defined(CONFIG_BOARD_NUCLEO_U575ZI_Q)
#define UART_DEVICE_DEV DT_NODELABEL(usart2)
#elif defined(CONFIG_BOARD_NUCLEO_L4R5ZI) || \
	defined(CONFIG_BOARD_NUCLEO_L152RE) || \
	defined(CONFIG_BOARD_STM32L562E_DK) || \
	defined(CONFIG_BOARD_NUCLEO_L552ZE_Q) || \
	defined(CONFIG_BOARD_B_U585I_IOT02A)
#define UART_DEVICE_DEV DT_NODELABEL(usart3)
#elif defined(CONFIG_BOARD_DISCO_L475_IOT1)
#define UART_DEVICE_DEV DT_NODELABEL(uart4)
#elif defined(CONFIG_BOARD_NUCLEO_F429ZI) || \
	defined(CONFIG_BOARD_NUCLEO_F207ZG) || \
	defined(CONFIG_BOARD_NUCLEO_F767ZI) || \
	defined(CONFIG_BOARD_NUCLEO_F746ZG)
#define UART_DEVICE_DEV DT_NODELABEL(usart6)
#elif defined(CONFIG_BOARD_FRDM_K82F)
#define UART_DEVICE_DEV DT_NODELABEL(lpuart0)
#elif defined(CONFIG_BOARD_NUCLEO_WB55RG)
#define UART_DEVICE_DEV DT_NODELABEL(lpuart1)
#elif defined(CONFIG_BOARD_MIMXRT1020_EVK) || \
	defined(CONFIG_BOARD_MIMXRT1024_EVK) || \
	defined(CONFIG_BOARD_MIMXRT1160_EVK_CM4) || \
	defined(CONFIG_BOARD_MIMXRT1170_EVK_CM4) || \
	defined(CONFIG_BOARD_MIMXRT1160_EVK_CM7) || \
	defined(CONFIG_BOARD_MIMXRT1170_EVK_CM7) || \
	defined(CONFIG_BOARD_TWR_KE18F)
#define UART_DEVICE_DEV DT_NODELABEL(lpuart2)
#elif defined(CONFIG_BOARD_MIMXRT1050_EVK) || \
	defined(CONFIG_BOARD_MIMXRT1060_EVK) || \
	defined(CONFIG_BOARD_MIMXRT1060_EVKB) || \
	defined(CONFIG_BOARD_MIMXRT1064_EVK)
#define UART_DEVICE_DEV DT_NODELABEL(lpuart3)
#elif defined(CONFIG_BOARD_MIMXRT1010_EVK) || \
	defined(CONFIG_BOARD_MIMXRT1015_EVK)
#define UART_DEVICE_DEV DT_NODELABEL(lpuart4)
#elif defined(CONFIG_SOC_ESP32C3)
	#define UART_DEVICE_DEV DT_NODELABEL(uart1)
#else
#define UART_DEVICE_DEV DT_CHOSEN(zephyr_console)
#endif

#endif /* __TEST_UART_H__ */
