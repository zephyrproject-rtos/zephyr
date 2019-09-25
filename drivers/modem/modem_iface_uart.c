/** @file
 * @brief interface for modem context
 *
 * UART-based modem interface implementation for modem context driver.
 */

/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(modem_iface_uart, CONFIG_MODEM_LOG_LEVEL);

#include <kernel.h>
#include <drivers/uart.h>

#include "modem_context.h"
#include "modem_iface_uart.h"

/**
 * @brief  Drains UART.
 *
 * @note   Discards remaining data.
 *
 * @param  *iface: modem interface.
 *
 * @retval None.
 */
static void modem_iface_uart_flush(struct modem_iface *iface)
{
	u8_t c;

	while (uart_fifo_read(iface->dev, &c, 1) > 0) {
		continue;
	}
}

/**
 * @brief  Modem interface interrupt handler.
 *
 * @note   Fills interfaces ring buffer with received data.
 *         When ring buffer is full the data is discarded.
 *
 * @param  *uart_dev: uart device.
 *
 * @retval None.
 */
static void modem_iface_uart_isr(struct device *uart_dev)
{
	struct modem_context *ctx;
	struct modem_iface_uart_data *data;
	int rx = 0, ret;

	/* lookup the modem context */
	ctx = modem_context_from_iface_dev(uart_dev);
	if (!ctx || !ctx->iface.iface_data) {
		return;
	}

	data = (struct modem_iface_uart_data *)(ctx->iface.iface_data);
	/* get all of the data off UART as fast as we can */
	while (uart_irq_update(ctx->iface.dev) &&
	       uart_irq_rx_ready(ctx->iface.dev)) {
		rx = uart_fifo_read(ctx->iface.dev,
				    data->isr_buf, data->isr_buf_len);
		if (rx <= 0) {
			continue;
		}

		ret = ring_buf_put(&data->rx_rb, data->isr_buf, rx);
		if (ret != rx) {
			LOG_ERR("Rx buffer doesn't have enough space. "
				"Bytes pending: %d, written: %d",
				rx, ret);
			modem_iface_uart_flush(&ctx->iface);
			k_sem_give(&data->rx_sem);
			break;
		}

		k_sem_give(&data->rx_sem);
	}
}

static int modem_iface_uart_read(struct modem_iface *iface,
				 u8_t *buf, size_t size, size_t *bytes_read)
{
	struct modem_iface_uart_data *data;

	if (!iface || !iface->iface_data) {
		return -EINVAL;
	}

	if (size == 0) {
		*bytes_read = 0;
		return 0;
	}

	data = (struct modem_iface_uart_data *)(iface->iface_data);
	*bytes_read = ring_buf_get(&data->rx_rb, buf, size);

	return 0;
}

static int modem_iface_uart_write(struct modem_iface *iface,
				  const u8_t *buf, size_t size)
{
	if (!iface || !iface->iface_data) {
		return -EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	do {
		uart_poll_out(iface->dev, *buf++);
	} while (--size);

	return 0;
}

int modem_iface_uart_init(struct modem_iface *iface,
			  struct modem_iface_uart_data *data,
			  const char *dev_name)
{
	if (!iface || !data) {
		return -EINVAL;
	}

	/* get UART device */
	iface->dev = device_get_binding(dev_name);
	if (!iface->dev) {
		return -ENODEV;
	}

	iface->iface_data = data;
	iface->read = modem_iface_uart_read;
	iface->write = modem_iface_uart_write;

	ring_buf_init(&data->rx_rb, data->rx_rb_buf_len, data->rx_rb_buf);
	k_sem_init(&data->rx_sem, 0, 1);

	uart_irq_rx_disable(iface->dev);
	uart_irq_tx_disable(iface->dev);
	modem_iface_uart_flush(iface);
	uart_irq_callback_set(iface->dev, modem_iface_uart_isr);
	uart_irq_rx_enable(iface->dev);

	return 0;
}
