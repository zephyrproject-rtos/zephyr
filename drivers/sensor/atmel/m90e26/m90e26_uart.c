/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(m90e26_uart, CONFIG_SENSOR_LOG_LEVEL);

#include "m90e26.h"

static void m90e26_uart_flush(const struct device *uart_dev)
{
	uint8_t c;

	while (uart_fifo_read(uart_dev, &c, 1) > 0) {
		continue;
	}
}

static void m90e26_uart_rx_callback(const struct device *dev, void *user_data)
{
	const struct m90e26_config *config = (const struct m90e26_config *)(user_data);
	struct m90e26_data *data = (struct m90e26_data *)dev->data;
	const struct device *uart_dev = config->bus.uart.bus;

	if (!uart_dev) {
		return;
	}

	if (!uart_irq_update(uart_dev)) {
		LOG_ERR("Unable to start processing interrupts");
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	if (data->bus_lock.lock_count == 0) {
		/* Not currently performing a read or write operation */
		return;
	}

	if (M90E26_IS_READ_CMD(data->frame.addr)) {
		/* Reading response for a read command */

		if (uart_fifo_read(uart_dev, &data->frame.data_high, 3) == 3) {
			k_mutex_unlock(&data->rx_lock);
		}
	} else {
		/* Reading response for a write command */
		if (uart_fifo_read(uart_dev, &data->frame.rcv_checksum, 1) == 1) {
			k_mutex_unlock(&data->rx_lock);
		}
	}
}

static int m90e26_bus_check_uart(const struct device *dev)
{
	int ret = 0;
	const struct m90e26_config *config = (const struct m90e26_config *)(dev->config);
	struct m90e26_data *data = (struct m90e26_data *)dev->data;

	if (config->bus.uart.config.baudrate != 9600 && config->bus.uart.config.baudrate != 2400) {
		LOG_ERR("Invalid UART baudrate for %s device. Supported baudrates are 9600 and "
			"2400.",
			dev->name);
		return -EINVAL;
	}

	if (!device_is_ready(config->bus.uart.bus)) {
		LOG_ERR("UART bus not ready for device %s.", dev->name);
		return -ENODEV;
	}

	uart_irq_rx_disable(config->bus.uart.bus);
	uart_irq_tx_disable(config->bus.uart.bus);

	m90e26_uart_flush(config->bus.uart.bus);

	ret = uart_configure(config->bus.uart.bus, &config->bus.uart.config);
	if (ret < 0) {
		LOG_ERR("Unable to configure UART port for device %s. Error: %d", dev->name, ret);
		return ret;
	}

	ret = uart_irq_callback_user_data_set(config->bus.uart.bus, m90e26_uart_rx_callback,
					      (void *)dev);
	if (ret < 0) {
		LOG_ERR("Failed to set UART IRQ callback for device %s. Error: %d", dev->name, ret);
		return ret;
	}

	ret = k_mutex_lock(&data->rx_lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	uart_irq_rx_enable(config->bus.uart.bus);

	return 0;
}

static int m90e26_read_reg_uart(const struct device *dev, const m90e26_register_t addr,
				m90e26_data_value_t *value)
{
	const struct m90e26_config *config = (const struct m90e26_config *)(dev->config);
	struct m90e26_data *data = (struct m90e26_data *)dev->data;
	const uint8_t host_checksum = (uint8_t)(M90E26_CMD_READ_MASK | addr);
	int ret = 0;

	data->frame.start_byte = M90E26_UART_START_BYTE;
	data->frame.addr = M90E26_CMD_READ_MASK | addr;
	data->frame.data_high = 0;
	data->frame.data_low = 0;
	data->frame.rcv_checksum = 0;

	ret = k_mutex_lock(&data->bus_lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	uart_poll_out(config->bus.uart.bus, data->frame.start_byte);
	uart_poll_out(config->bus.uart.bus, data->frame.addr);
	uart_poll_out(config->bus.uart.bus, host_checksum);

	ret = k_mutex_lock(&data->rx_lock, K_MSEC(5));
	if (ret < 0) {
		LOG_ERR("No response from device %s for UART read of reg 0x%02X", dev->name, addr);
		goto end;
	}

	if (host_checksum != data->frame.rcv_checksum) {
		LOG_ERR("UART read checksum mismatch for reg 0x%02X: sent 0x%02X, received 0x%02X",
			addr, data->frame.rcv_checksum, host_checksum);
		ret = -EIO;
		goto end;
	}

	value->uint16 = ((uint16_t)data->frame.data_high << 8) | (uint16_t)data->frame.data_low;

end:
	k_mutex_unlock(&data->bus_lock);
	return ret;
}

static int m90e26_write_reg_uart(const struct device *dev, const m90e26_register_t addr,
				 const m90e26_data_value_t *value)
{
	const struct m90e26_config *config = (const struct m90e26_config *)(dev->config);
	struct m90e26_data *data = (struct m90e26_data *)dev->data;
	const uint8_t host_checksum =
		(uint8_t)(((addr & M90E26_CMD_WRITE_MASK) + ((value->uint16 >> 8) & 0xFF) +
			   (value->uint16 & 0xFF)) &
			  0xFF);
	int ret = 0;

	data->frame.start_byte = M90E26_UART_START_BYTE;
	data->frame.addr = addr & M90E26_CMD_WRITE_MASK;
	data->frame.data_high = (value->uint16 >> 8) & 0xFF;
	data->frame.data_low = value->uint16 & 0xFF;
	data->frame.rcv_checksum = 0;

	ret = k_mutex_lock(&data->bus_lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	uart_poll_out(config->bus.uart.bus, data->frame.start_byte);
	uart_poll_out(config->bus.uart.bus, data->frame.addr);
	uart_poll_out(config->bus.uart.bus, data->frame.data_high);
	uart_poll_out(config->bus.uart.bus, data->frame.data_low);
	uart_poll_out(config->bus.uart.bus, host_checksum);

	ret = k_mutex_lock(&data->rx_lock, K_MSEC(5));
	if (ret < 0) {
		LOG_ERR("No response from device %s for UART write to reg 0x%02X", dev->name, addr);
		goto end;
	}

	if (host_checksum != data->frame.rcv_checksum) {
		LOG_ERR("UART write checksum mismatch for reg 0x%02X: sent 0x%02X, received 0x%02X",
			addr, data->frame.rcv_checksum, host_checksum);
		ret = -EIO;
		goto end;
	}
end:
	k_mutex_unlock(&data->bus_lock);
	return ret;
}

const struct m90e26_bus_io m90e26_bus_io_uart = {
	.bus_check = m90e26_bus_check_uart,
	.read = m90e26_read_reg_uart,
	.write = m90e26_write_reg_uart,
};
