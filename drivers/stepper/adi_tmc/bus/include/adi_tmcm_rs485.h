/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Alexis Czezar C Torreno
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_TMCM_RS485_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_TMCM_RS485_H_

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>

/**
 * @brief TMCM RS485 command parameters
 */
struct tmcm_rs485_cmd {
	uint8_t module_addr;
	uint8_t command_number;
	uint8_t type_number;
	uint8_t bank_number;
	uint32_t data;
};

/**
 * @brief Calculate checksum for TMCM RS485 datagram
 *
 * @param datagram Pointer to datagram buffer
 * @return Checksum value
 */
uint8_t tmcm_rs485_checksum(const uint8_t *datagram);

/**
 * @brief Send command via RS485 interface
 *
 * @param rs485 Pointer to RS485 UART device
 * @param cmd Pointer to command parameters
 * @param reply Pointer to store reply data (MSB first)
 * @param de_gpio Pointer to DE GPIO spec (NULL if not used)
 * @return 0 on success, negative errno on failure
 */
int tmcm_rs485_send_command(const struct device *rs485, const struct tmcm_rs485_cmd *cmd,
			    uint32_t *reply, const struct gpio_dt_spec *de_gpio);

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_TMCM_RS485_H_ */
