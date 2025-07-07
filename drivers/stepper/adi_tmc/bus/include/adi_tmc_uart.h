/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_UART_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_UART_H_

#include <zephyr/drivers/uart.h>

/**
 * @brief Calculate CRC for TMC UART datagram
 *
 * @param datagram Pointer to datagram buffer
 * @param len Length of datagram
 * @return uint8_t CRC value
 */
uint8_t tmc_uart_calc_crc(const uint8_t *datagram, uint8_t len);

/**
 * @brief Read register via UART single-wire interface
 *
 * @param uart Pointer to UART device specification
 * @param device_addr Secondary address of TMC device
 * @param register_address Register address to read
 * @param data Pointer to store read data
 * @return int 0 on success, negative errno on failure
 */
int tmc_uart_read_register(const struct device *uart, uint8_t device_addr, uint8_t register_address,
			   uint32_t *data);

/**
 * @brief Write register via UART single-wire interface
 *
 * @param uart Pointer to UART device specification
 * @param device_addr Secondary address of the TMC device
 * @param register_address Register address to write
 * @param data Data to write to register
 * @return int 0 on success, negative errno on failure
 */
int tmc_uart_write_register(const struct device *uart, uint8_t device_addr,
			    uint8_t register_address, uint32_t data);

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_UART_H_ */
