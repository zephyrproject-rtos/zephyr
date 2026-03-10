/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Alexis Czezar C Torreno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <adi_tmcm_rs485.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rs485, CONFIG_STEPPER_LOG_LEVEL);

/* TMCM RS485 read/write datagram size */
#define ADI_TMCM_RS485_DATAGRAM_SIZE     9
/* Delay in microseconds for RS485 driver enable settling time */
#define ADI_TMCM_RS485_DE_SETTLE_TIME_US 10
/* Timeout in milliseconds for echo byte response */
#define ADI_TMCM_RS485_ECHO_TIMEOUT_MS   100
/* Timeout in milliseconds for reply byte response */
#define ADI_TMCM_RS485_REPLY_TIMEOUT_MS  1000

uint8_t tmcm_rs485_checksum(const uint8_t *datagram)
{
	uint8_t checksum = 0;

	for (uint8_t i = 0; i < ADI_TMCM_RS485_DATAGRAM_SIZE - 1; i++) {
		checksum = checksum + datagram[i];
	}

	return checksum;
}

static inline void rs485_de_set(const struct gpio_dt_spec *de_gpio, int value)
{
	if (de_gpio != NULL && de_gpio->port != NULL) {
		gpio_pin_set_dt(de_gpio, value);
	}
}

int tmcm_rs485_send_command(const struct device *rs485, const struct tmcm_rs485_cmd *cmd,
			    uint32_t *reply, const struct gpio_dt_spec *de_gpio)
{
	uint8_t write_buffer[ADI_TMCM_RS485_DATAGRAM_SIZE];
	uint8_t read_buffer[ADI_TMCM_RS485_DATAGRAM_SIZE];
	struct uart_config rs485_cfg;
	uint8_t echo;
	int err = 0;

	/* Get current RS485 configuration */
	err = uart_config_get(rs485, &rs485_cfg);
	if (err) {
		LOG_ERR("Failed to get UART configuration: %d", err);
		return -EIO;
	}

	/* Format the TMCM RS485 datagram */
	write_buffer[0] = cmd->module_addr;
	write_buffer[1] = cmd->command_number;
	write_buffer[2] = cmd->type_number;
	write_buffer[3] = cmd->bank_number;
	sys_put_be32(cmd->data, &write_buffer[4]);
	write_buffer[8] = tmcm_rs485_checksum(write_buffer);

	LOG_HEXDUMP_DBG(write_buffer, ADI_TMCM_RS485_DATAGRAM_SIZE, "TX:");

	/* Flush any stale RX data before starting */
	while (uart_poll_in(rs485, &echo) == 0) {
		/* discard */
	}

	/* ---- TRANSMIT PHASE ---- */

	/* DE HIGH: enable RS485 driver (transmit mode) */
	rs485_de_set(de_gpio, 1);
	k_busy_wait(ADI_TMCM_RS485_DE_SETTLE_TIME_US);

	/* Send all 9 bytes, reading back echo for each */
	for (size_t i = 0; i < ADI_TMCM_RS485_DATAGRAM_SIZE; i++) {
		uart_poll_out(rs485, write_buffer[i]);

		/* Wait for and consume the echo byte */
		k_timepoint_t end = sys_timepoint_calc(K_MSEC(ADI_TMCM_RS485_ECHO_TIMEOUT_MS));

		do {
			err = uart_poll_in(rs485, &echo);
		} while (err == -1 && !sys_timepoint_expired(end));

		if (err == -1) {
			LOG_ERR("Timeout waiting for echo byte %d", i);
			rs485_de_set(de_gpio, 0);
			return -EAGAIN;
		}

		if (echo != write_buffer[i]) {
			LOG_ERR("Echo mismatch byte %d: got 0x%02X expected 0x%02X", i, echo,
				write_buffer[i]);
			rs485_de_set(de_gpio, 0);
			return -EIO;
		}
	}

	/* DE LOW: disable RS485 driver (switch to receive mode) */
	rs485_de_set(de_gpio, 0);

	/* ---- RECEIVE PHASE ---- */

	/* Receive 9-byte reply with timeout */
	for (size_t i = 0; i < ADI_TMCM_RS485_DATAGRAM_SIZE; i++) {
		k_timepoint_t end = sys_timepoint_calc(K_MSEC(ADI_TMCM_RS485_REPLY_TIMEOUT_MS));

		do {
			err = uart_poll_in(rs485, &read_buffer[i]);
		} while (err == -1 && !sys_timepoint_expired(end));

		if (err == -1) {
			LOG_ERR("Timeout waiting for reply byte %d", i);
			return -EAGAIN;
		}
		if (err < 0) {
			LOG_ERR("Error %d receiving byte %d", err, i);
			return -EIO;
		}
	}

	LOG_HEXDUMP_DBG(read_buffer, ADI_TMCM_RS485_DATAGRAM_SIZE, "RX:");

	/* Validate checksum */
	uint8_t checksum = tmcm_rs485_checksum(read_buffer);

	if (checksum != read_buffer[8]) {
		LOG_ERR("Checksum mismatch: got 0x%02x, expected 0x%02x", read_buffer[8], checksum);
		return -EIO;
	}

	/* Check status byte (byte 2): 100 = STATUS_OK */
	if (read_buffer[2] != 100) {
		LOG_ERR("Command failed with status: %d", read_buffer[2]);
		return -EIO;
	}

	/* Extract 32-bit reply value */
	*reply = sys_get_be32(&read_buffer[4]);

	return 0;
}
