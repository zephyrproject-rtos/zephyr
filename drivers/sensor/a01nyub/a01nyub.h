/*
 * Copyright (c) 2023 SteadConnect
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_A01NYUB
#define ZEPHYR_DRIVERS_SENSOR_A01NYUB

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#define A01NYUB_BUF_LEN 4
#define A01NYUB_CHECKSUM_IDX 3

/* Arbitrary max duration to wait for the response */
#define A01NYUB_WAIT K_MILLISECONDS(50)

const struct uart_config uart_cfg = {
	.baudrate = 9600,
	.parity = UART_CFG_PARITY_NONE,
	.stop_bits = UART_CFG_STOP_BITS_1,
	.data_bits = UART_CFG_DATA_BITS_8,
	.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};

struct a01nyub_data {
	/* Max data length is 16 bits */
	uint16_t data;
	uint8_t xfer_bytes;
	uint8_t rd_data[A01NYUB_BUF_LEN];
};

struct a01nyub_cfg {
	const struct device *uart_dev;
	uart_irq_callback_user_data_t cb;
};

#define A01NYUB_HEADER 0xff

#endif /* ZEPHYR_DRIVERS_SENSOR_A01NYUB */
