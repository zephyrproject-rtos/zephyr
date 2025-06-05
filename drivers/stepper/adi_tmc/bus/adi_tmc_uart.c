/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <adi_tmc_uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc_uart, CONFIG_STEPPER_LOG_LEVEL);

/* TMC UART standard datagram size */
#define ADI_TMC_UART_DATAGRAM_SIZE          8
/* TMC UART read request datagram size */
#define ADI_TMC_UART_READ_REQ_DATAGRAM_SIZE 4
/* TMC UART protocol constants */
#define ADI_TMC_UART_SYNC_BYTE              0x05
#define ADI_TMC_UART_WRITE_BIT              0x80

uint8_t tmc_uart_calc_crc(const uint8_t *datagram, uint8_t len)
{
	uint8_t crc = 0;

	for (uint8_t i = 0; i < len; i++) {
		uint8_t current_byte = datagram[i];

		for (uint8_t j = 0; j < 8; j++) {
			if ((crc >> 7) ^ (current_byte & 0x01)) {
				crc = (crc << 1) ^ 0x07;
			} else {
				crc = (crc << 1) & 0xFF;
			}
			current_byte = current_byte >> 1;
		}
	}

	return crc;
}

static int tmc_uart_send_byte_with_echo(const struct device *uart, uint8_t byte)
{
	uint8_t echo_byte;
	int err;

	uart_poll_out(uart, byte);

	/* Wait for echo with timeout */
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(5)); /* 5ms timeout for echo */

	do {
		err = uart_poll_in(uart, &echo_byte);
		if (err >= 0 && echo_byte == byte) {
			/* Received matching echo */
			return 0;
		}
	} while (err == -1 && !sys_timepoint_expired(end));

	LOG_ERR("Echo mismatch or timeout: sent 0x%02X", byte);
	return -EIO;
}

int tmc_uart_write_register(const struct device *uart, uint8_t device_addr,
			    uint8_t register_address, uint32_t data)
{
	uint8_t buffer[ADI_TMC_UART_DATAGRAM_SIZE];
	int err = 0;

	/* Format the UART TMC datagram */
	buffer[0] = ADI_TMC_UART_SYNC_BYTE;                    /* Sync byte */
	buffer[1] = device_addr;                               /* Device address */
	buffer[2] = register_address | ADI_TMC_UART_WRITE_BIT; /* Register address with write bit */
	sys_put_be32(data, &buffer[3]);                        /* Write data */
	buffer[7] = tmc_uart_calc_crc(buffer, ADI_TMC_UART_DATAGRAM_SIZE - 1U); /* CRC */

	/* Send datagram byte by byte using polling */
	for (size_t i = 0; i < ADI_TMC_UART_DATAGRAM_SIZE; i++) {
		err = tmc_uart_send_byte_with_echo(uart, buffer[i]);
		if (err) {
			LOG_ERR("Failed to send byte %d: 0x%02X", i, buffer[i]);
			return err;
		}
	}

	return 0;
}

int tmc_uart_read_register(const struct device *uart, uint8_t device_addr, uint8_t register_address,
			   uint32_t *data)
{
	uint8_t write_buffer[ADI_TMC_UART_READ_REQ_DATAGRAM_SIZE];
	uint8_t read_buffer[ADI_TMC_UART_DATAGRAM_SIZE];
	struct uart_config uart_cfg;

	int err = 0;

	/* Get current UART configuration */
	err = uart_config_get(uart, &uart_cfg);
	if (err) {
		LOG_ERR("Failed to get UART configuration: %d", err);
		return -EIO;
	}

	/* Calculate delay based on UART baudrate (SENDDELAY: default=8 bit times))*/
	uint32_t delay_us = (8 * 1000000) / uart_cfg.baudrate;

	/* Format the UART TMC datagram for read */
	write_buffer[0] = ADI_TMC_UART_SYNC_BYTE; /* Sync byte */
	write_buffer[1] = device_addr;            /* Device address */
	write_buffer[2] = register_address;       /* Register address */
	write_buffer[3] = tmc_uart_calc_crc(write_buffer, 3);

	/* Send read request byte by byte and wait for each echo */
	for (size_t i = 0; i < ADI_TMC_UART_READ_REQ_DATAGRAM_SIZE; i++) {
		err = tmc_uart_send_byte_with_echo(uart, write_buffer[i]);
		if (err) {
			LOG_ERR("Failed to send byte %d: 0x%02X", i, write_buffer[i]);
			return -EIO;
		}
	}

	/* Small delay to allow device to prepare response */
	k_busy_wait(delay_us);

	/* Receive response with timeout */
	for (size_t i = 0; i < sizeof(read_buffer) && (err == 0); i++) {
		k_timepoint_t end = sys_timepoint_calc(K_MSEC(1000));

		do {
			err = uart_poll_in(uart, &read_buffer[i]);
		} while (err == -1 && !sys_timepoint_expired(end));

		if (err == -1) {
			LOG_ERR("Timeout waiting for byte %d for register 0x%x", i,
				register_address);
			return -EAGAIN;
		}
		if (err < 0) {
			LOG_ERR("Error %d receiving byte %d for register 0x%x", err, i,
				register_address);
			return -EIO;
		}
	}

	/* Print the received bytes */
	LOG_HEXDUMP_DBG(read_buffer, ADI_TMC_UART_DATAGRAM_SIZE, "Received bytes:");

	/* Validate CRC */
	uint8_t crc = tmc_uart_calc_crc(read_buffer, ADI_TMC_UART_DATAGRAM_SIZE - 1U);

	if (crc != read_buffer[7]) {
		LOG_ERR("CRC mismatch for register 0x%x: got 0x%x, expected 0x%x", register_address,
			read_buffer[7], crc);
		return -EIO;
	}

	/* Construct a 32-bit register value from received bytes */
	*data = sys_get_be32(&read_buffer[3]);

	return 0;
}
