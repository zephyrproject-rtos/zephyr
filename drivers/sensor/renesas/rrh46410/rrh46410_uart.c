/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rrh46410

#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>

#include "rrh46410.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(uart)

LOG_MODULE_DECLARE(RRH46410, CONFIG_SENSOR_LOG_LEVEL);

static void rrh46410_uart_flush(const struct device *uart_dev)
{
	uint8_t tmp;

	while (uart_fifo_read(uart_dev, &tmp, 1) > 0) {
		LOG_ERR("flush: %d", tmp);
		continue;
	}
}

static void rrh46410_buffer_reset(struct rrh46410_data *data)
{
	memset(data->read_buffer, 0, data->read_index);
	data->read_index = 0;
}

static void rrh46410_uart_isr(const struct device *uart_dev, void *user_data)
{
	const struct device *dev = user_data;
	struct rrh46410_data *data = dev->data;
	int rc;
	uint8_t received_checksum = 0x00;

	if (!device_is_ready(uart_dev)) {
		LOG_DBG("UART device is not ready");
		return;
	}

	if (!uart_irq_update(uart_dev)) {
		LOG_DBG("Unable to process interrupts");
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		LOG_DBG("No RX data");
		return;
	}

	rc = uart_fifo_read(uart_dev, &data->read_buffer[data->read_index], RRH46410_BUFFER_LENGTH);

	if (rc < 0 ) {
		LOG_ERR("UART read failed: %d", rc < 0 ? rc : -ERANGE);
		rrh46410_uart_flush(uart_dev);
		LOG_HEXDUMP_WRN(data->read_buffer, data->read_index, "Discarding");
		rrh46410_buffer_reset(data);
	} else {
		data->read_index += rc;

		for(int i = 1; i < data->read_index - 1; i++){
			received_checksum += data->read_buffer[i];
		}

		received_checksum = ~received_checksum;

		if(data->read_buffer[data->read_index - 1] == received_checksum){
			k_sem_give(&data->uart_rx_sem);
		}
	}
}

static int rrh46410_await_receive(struct rrh46410_data *data)
{
	int rc = k_sem_take(&data->uart_rx_sem, K_MSEC(RRH46410_MAX_RESPONSE_DELAY));
	
	/* Reset semaphore if sensor did not respond within maximum specified response time */
	if (rc == -EAGAIN) {
		k_sem_reset(&data->uart_rx_sem);
	}

	return rc;
}

static int rrh46410_uart_transceive(const struct device *dev, uint8_t *command_data, size_t data_size)
{
	const struct rrh46410_config *cfg = dev->config;
	struct rrh46410_data *data = dev->data;
	int rc;

	k_mutex_lock(&data->uart_mutex, K_FOREVER);

	rrh46410_buffer_reset(data);

	for (int i = 0; i != data_size; i++) {
		uart_poll_out(cfg->bus_cfg.uart_dev, command_data[i]);
	}

	rc = rrh46410_await_receive(data);
	if (rc != 0) {
		LOG_WRN("UART did not receive a response: %d", rc);
	}

	k_mutex_unlock(&data->uart_mutex);

	return rc;
}

static int rrh46410_uart_read_data(const struct device *dev, uint8_t *rx_buff, size_t data_size)
{

	return 0;
}

static const struct rrh46410_transfer_function rrh46410_uart_transfer_fn = {
	.read_data = rrh46410_uart_read_data,
	.write_data = rrh46410_uart_transceive,
};

int rrh46410_uart_init(const struct device *dev)
{
	struct rrh46410_data *data = dev->data;
	const struct rrh46410_config *cfg = dev->config;
	int rc;

	data->hw_tf = &rrh46410_uart_transfer_fn;

	k_mutex_init(&data->uart_mutex);
	k_sem_init(&data->uart_rx_sem, 0, 1);

	uart_irq_rx_disable(cfg->bus_cfg.uart_dev);
	uart_irq_tx_disable(cfg->bus_cfg.uart_dev);

	rc = uart_irq_callback_user_data_set(cfg->bus_cfg.uart_dev, rrh46410_uart_isr, (void *)dev);
	if (rc != 0) {
		LOG_ERR("UART IRQ setup failed: %d", rc);
		return rc;
	}

	uart_irq_rx_enable(cfg->bus_cfg.uart_dev);

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(uart) */
