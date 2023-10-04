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

#if DT_NODE_EXISTS(DT_NODELABEL(dut))
#define UART_NODE DT_NODELABEL(dut)
#elif defined(CONFIG_SOC_ESP32C3)
#define UART_NODE DT_NODELABEL(uart1)
#else
#define UART_NODE DT_CHOSEN(zephyr_console)
#endif

#endif /* __TEST_UART_H__ */
