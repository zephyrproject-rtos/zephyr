/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Public header file for the NS16550 UART
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SERIAL_UART_NS16550_H_
#define ZEPHYR_INCLUDE_DRIVERS_SERIAL_UART_NS16550_H_

/**
 * @brief Set DLF
 *
 * @note This only applies to Synopsys Designware UART IP block.
 */
#define CMD_SET_DLF	0x01

/**
 * @brief Get the device address from uart_ns16550.
 *
 * @param dev UART device instance
 */
uint32_t uart_ns16550_get_port(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_SERIAL_UART_NS16550_H_ */
