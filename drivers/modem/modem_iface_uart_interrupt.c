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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_iface_uart, CONFIG_MODEM_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

#include "modem_context.h"
#include "modem_iface_uart.h"

/**
 * @brief  Drains UART.
 *
 * @note   Discards remaining data.
 *
 * @param  iface: modem interface.
 *
 * @retval None.
 */
static void modem_iface_uart_flush(struct modem_iface *iface)
{
	uint8_t c;

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
 * @param  uart_dev: uart device.
 *
 * @retval None.
 */
static void modem_iface_uart_isr(const struct device *uart_dev,
				 void *user_data)
{
	struct modem_context *ctx;
	struct modem_iface_uart_data *data;
	int rx = 0, ret;
	uint8_t *dst;
	uint32_t partial_size = 0;
	uint32_t total_size = 0;

	ARG_UNUSED(user_data);

	/* lookup the modem context */
	ctx = modem_context_from_iface_dev(uart_dev);
	if (!ctx || !ctx->iface.iface_data) {
		return;
	}

	data = (struct modem_iface_uart_data *)(ctx->iface.iface_data);
	/* get all of the data off UART as fast as we can */
	while (uart_irq_update(ctx->iface.dev) &&
	       uart_irq_rx_ready(ctx->iface.dev)) {
		if (!partial_size) {
			partial_size = ring_buf_put_claim(&data->rx_rb, &dst,
							  UINT32_MAX);
		}
		if (!partial_size) {
			if (data->hw_flow_control) {
				uart_irq_rx_disable(ctx->iface.dev);
			} else {
				LOG_ERR("Rx buffer doesn't have enough space");
				modem_iface_uart_flush(&ctx->iface);
			}
			break;
		}

		rx = uart_fifo_read(ctx->iface.dev, dst, partial_size);
		if (rx <= 0) {
			continue;
		}

		dst += rx;
		total_size += rx;
		partial_size -= rx;
	}

	ret = ring_buf_put_finish(&data->rx_rb, total_size);
	__ASSERT_NO_MSG(ret == 0);

	if (total_size > 0) {
		k_sem_give(&data->rx_sem);
	}
}

static int modem_iface_uart_read(struct modem_iface *iface,
				 uint8_t *buf, size_t size, size_t *bytes_read)
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

	if (data->hw_flow_control && *bytes_read == 0) {
		uart_irq_rx_enable(iface->dev);
	}

	return 0;
}

static int modem_iface_uart_write(struct modem_iface *iface,
				  const uint8_t *buf, size_t size)
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

int modem_iface_uart_init_dev(struct modem_iface *iface,
			      const struct device *dev)
{
	/* get UART device */
	const struct device *prev = iface->dev;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	/* Check if there's already a device inited to this iface. If so,
	 * interrupts needs to be disabled on that too before switching to avoid
	 * race conditions with modem_iface_uart_isr.
	 */
	if (prev) {
		uart_irq_tx_disable(prev);
		uart_irq_rx_disable(prev);
	}

	uart_irq_rx_disable(dev);
	uart_irq_tx_disable(dev);
	iface->dev = dev;

	modem_iface_uart_flush(iface);
	uart_irq_callback_set(iface->dev, modem_iface_uart_isr);
	uart_irq_rx_enable(iface->dev);

	if (prev) {
		uart_irq_rx_enable(prev);
	}

	return 0;
}

int modem_iface_uart_init(struct modem_iface *iface, struct modem_iface_uart_data *data,
			  const struct modem_iface_uart_config *config)
{
	int ret;

	if (iface == NULL || data == NULL || config == NULL) {
		return -EINVAL;
	}

	iface->iface_data = data;
	iface->read = modem_iface_uart_read;
	iface->write = modem_iface_uart_write;

	ring_buf_init(&data->rx_rb, config->rx_rb_buf_len, config->rx_rb_buf);
	k_sem_init(&data->rx_sem, 0, 1);

	/* Configure hardware flow control */
	data->hw_flow_control = config->hw_flow_control;

	/* Get UART device */
	ret = modem_iface_uart_init_dev(iface, config->dev);
	if (ret < 0) {
		iface->iface_data = NULL;
		iface->read = NULL;
		iface->write = NULL;

		return ret;
	}

	return 0;
}
