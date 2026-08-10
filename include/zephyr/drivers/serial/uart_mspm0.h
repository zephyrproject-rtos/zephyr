/*
 * Copyright (c) 2026 Bang & Olufsen A/S, Denmark
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Public header file for the TI MSPM0 UART driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SERIAL_UART_MSPM0_H_
#define ZEPHYR_INCLUDE_DRIVERS_SERIAL_UART_MSPM0_H_

/**
 * @brief Drive or release a break condition on the TX line.
 *
 * Intended to be used through uart_drv_cmd() (requires
 * @kconfig{CONFIG_UART_MSPM0_DRV_CMD}) to generate the break field.
 *
 */
#define UART_MSPM0_CMD_LIN_SEND_BREAK 0x01

#endif /* ZEPHYR_INCLUDE_DRIVERS_SERIAL_UART_MSPM0_H_ */
