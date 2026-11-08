/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

#include "atm90e26.h"

LOG_MODULE_DECLARE(atm90e26, CONFIG_ENERGY_METER_LOG_LEVEL);

/* The chip answers within 5 ms; bytes take 10 bit times each (8N1) */
#define ATM90E26_UART_ANSWER_US 5000U
#define ATM90E26_UART_MARGIN_US 5000U

static void atm90e26_uart_isr(const struct device *uart, void *user_data)
{
	const struct device *dev = user_data;
	struct atm90e26_data *data = dev->data;
	uint8_t byte;

	uart_irq_update(uart);

	while ((uart_irq_rx_ready(uart) > 0) && (uart_fifo_read(uart, &byte, 1) == 1)) {
		if (data->rx_len >= data->rx_expected) {
			continue;
		}

		data->rx_buf[data->rx_len++] = byte;
		if (data->rx_len == data->rx_expected) {
			k_sem_give(&data->rx_done);
		}
	}
}

static int atm90e26_uart_check(const struct device *dev)
{
	const struct atm90e26_config *cfg = dev->config;
	struct atm90e26_data *data = dev->data;
	struct uart_config uart_cfg;
	int ret;

	if (!device_is_ready(cfg->bus.uart)) {
		LOG_ERR("UART not ready");
		return -ENODEV;
	}

	ret = uart_config_get(cfg->bus.uart, &uart_cfg);
	if (ret < 0) {
		LOG_ERR("Cannot read the UART configuration (%d)", ret);
		return ret;
	}

	if (((uart_cfg.baudrate != 2400U) && (uart_cfg.baudrate != 9600U)) ||
	    (uart_cfg.data_bits != UART_CFG_DATA_BITS_8) ||
	    (uart_cfg.parity != UART_CFG_PARITY_NONE) ||
	    (uart_cfg.stop_bits != UART_CFG_STOP_BITS_1)) {
		LOG_ERR("UART must be 2400 or 9600 baud, 8N1");
		return -EINVAL;
	}

	data->baudrate = uart_cfg.baudrate;
	k_sem_init(&data->rx_done, 0, 1);

	uart_irq_rx_disable(cfg->bus.uart);
	uart_irq_tx_disable(cfg->bus.uart);

	return uart_irq_callback_user_data_set(cfg->bus.uart, atm90e26_uart_isr, (void *)dev);
}

static int atm90e26_uart_transfer(const struct device *dev, const uint8_t *tx, size_t tx_len,
				  size_t rx_len)
{
	const struct atm90e26_config *cfg = dev->config;
	struct atm90e26_data *data = dev->data;
	uint32_t bytes_us = ((tx_len + rx_len) * 10U * USEC_PER_SEC) / data->baudrate;
	uint8_t byte;
	int ret;

	/* Drop anything left over from an aborted transaction */
	while (uart_poll_in(cfg->bus.uart, &byte) == 0) {
	}

	data->rx_len = 0;
	data->rx_expected = rx_len;
	k_sem_reset(&data->rx_done);
	uart_irq_rx_enable(cfg->bus.uart);

	for (size_t i = 0; i < tx_len; i++) {
		uart_poll_out(cfg->bus.uart, tx[i]);
	}

	ret = k_sem_take(&data->rx_done,
			 K_USEC(ATM90E26_UART_ANSWER_US + ATM90E26_UART_MARGIN_US + bytes_us));

	uart_irq_rx_disable(cfg->bus.uart);

	if (ret < 0) {
		LOG_DBG("No answer for command 0x%02x", tx[1]);
		return -ETIMEDOUT;
	}

	return 0;
}

static int atm90e26_uart_read(const struct device *dev, uint8_t addr, uint16_t *val)
{
	struct atm90e26_data *data = dev->data;
	uint8_t cmd = ATM90E26_READ | (addr & ATM90E26_ADDR_MASK);
	const uint8_t tx[] = {ATM90E26_UART_START, cmd, cmd};
	int ret;

	ret = atm90e26_uart_transfer(dev, tx, sizeof(tx), 3);
	if (ret < 0) {
		return ret;
	}

	*val = ((uint16_t)data->rx_buf[0] << 8) | data->rx_buf[1];

	/* The chip checksums a read answer over the two data bytes */
	if ((uint8_t)(data->rx_buf[0] + data->rx_buf[1]) != data->rx_buf[2]) {
		return -EBADMSG;
	}

	return 0;
}

static int atm90e26_uart_write(const struct device *dev, uint8_t addr, uint16_t val)
{
	struct atm90e26_data *data = dev->data;
	uint8_t cmd = addr & ATM90E26_ADDR_MASK;
	uint8_t hi = (uint8_t)(val >> 8);
	uint8_t lo = (uint8_t)val;
	const uint8_t tx[] = {ATM90E26_UART_START, cmd, hi, lo, (uint8_t)(cmd + hi + lo)};
	int ret;

	ret = atm90e26_uart_transfer(dev, tx, sizeof(tx), 1);
	if (ret < 0) {
		return ret;
	}

	/* The chip echoes the checksum of a write it accepted */
	return (data->rx_buf[0] == tx[4]) ? 0 : -EBADMSG;
}

const struct atm90e26_bus_io atm90e26_bus_io_uart = {
	.check = atm90e26_uart_check,
	.read = atm90e26_uart_read,
	.write = atm90e26_uart_write,
};
