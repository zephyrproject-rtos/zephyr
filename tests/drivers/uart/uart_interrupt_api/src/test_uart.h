/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2025 Analog Devices, Inc
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

/* RX and TX must not be connected together, to keep TX interrupts isolated */

#if DT_NODE_EXISTS(DT_NODELABEL(dut))
#define UART_NODE DT_NODELABEL(dut)
#else
#define UART_NODE DT_CHOSEN(zephyr_console)
#endif

#endif /* __TEST_UART_H__ */
