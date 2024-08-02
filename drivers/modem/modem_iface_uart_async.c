/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

#include "modem_context.h"
#include "modem_iface_uart.h"

LOG_MODULE_REGISTER(modem_iface_uart_async, CONFIG_MODEM_LOG_LEVEL);

#define RX_BUFFER_SIZE CONFIG_MODEM_IFACE_UART_ASYNC_RX_BUFFER_SIZE
#define RX_BUFFER_NUM CONFIG_MODEM_IFACE_UART_ASYNC_RX_NUM_BUFFERS

K_MEM_SLAB_DEFINE(uart_modem_async_rx_slab, RX_BUFFER_SIZE, RX_BUFFER_NUM, 1);

static void iface_uart_async_callback(const struct device *dev,
				      struct uart_event *evt,
				      void *user_data)
{
	struct modem_iface *iface = user_data;
	struct modem_iface_uart_data *data = iface->iface_data;
	uint32_t written;
	void *buf;
	int rc;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&data->tx_sem);
		break;
	case UART_RX_BUF_REQUEST:
		/* Allocate next RX buffer for UART driver */
		rc = k_mem_slab_alloc(&uart_modem_async_rx_slab, (void **)&buf, K_NO_WAIT);
		if (rc < 0) {
			/* Major problems, UART_RX_BUF_RELEASED event is not being generated, or
			 * CONFIG_MODEM_IFACE_UART_ASYNC_RX_NUM_BUFFERS is not large enough.
			 */
			LOG_ERR("RX buffer starvation");
			break;
		}
		/* Provide the buffer to the UART driver */
		uart_rx_buf_rsp(dev, buf, RX_BUFFER_SIZE);
		break;
	case UART_RX_BUF_RELEASED:
		/* UART driver is done with memory, free it */
		k_mem_slab_free(&uart_modem_async_rx_slab, (void *)evt->data.rx_buf.buf);
		break;
	case UART_RX_RDY:
		/* Place received data on the ring buffer */
		written = ring_buf_put(&data->rx_rb,
				       evt->data.rx.buf + evt->data.rx.offset,
				       evt->data.rx.len);
		if (written != evt->data.rx.len) {
			LOG_WRN("Received bytes dropped from ring buf");
		}
		/* Notify upper layer that new data has arrived */
		k_sem_give(&data->rx_sem);
		break;
	case UART_RX_STOPPED:
		break;
	case UART_RX_DISABLED:
		/* RX stopped (likely due to line error), re-enable it */
		rc = k_mem_slab_alloc(&uart_modem_async_rx_slab, (void **)&buf, K_FOREVER);
		if (rc < 0) {
			LOG_ERR("RX disabled and buffer starvation");
			break;
		}
		rc = uart_rx_enable(dev, buf, RX_BUFFER_SIZE,
				    CONFIG_MODEM_IFACE_UART_ASYNC_RX_TIMEOUT_US);
		if (rc < 0) {
			LOG_ERR("Failed to re-enable UART");
		}
		break;
	default:
		break;
	}
}

static int modem_iface_uart_async_read(struct modem_iface *iface,
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

	/* Pull data off the ring buffer */
	data = iface->iface_data;
	*bytes_read = ring_buf_get(&data->rx_rb, buf, size);
	return 0;
}

static int modem_iface_uart_async_write(struct modem_iface *iface,
					const uint8_t *buf, size_t size)
{
	struct modem_iface_uart_data *data;
	int rc;

	if (!iface || !iface->iface_data) {
		return -EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	/* Start the transmission */
	rc = uart_tx(iface->dev, buf, size, SYS_FOREVER_MS);
	if (rc >= 0) {
		/* Wait until the transmission completes */
		data = iface->iface_data;
		k_sem_take(&data->tx_sem, K_FOREVER);
	}
	return rc;
}

int modem_iface_uart_init_dev(struct modem_iface *iface,
			      const struct device *dev)
{
	struct modem_iface_uart_data *data;
	void *buf;
	int rc;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	/* Check if there's already a device inited to this iface. If so,
	 * interrupts needs to be disabled on that too before switching to avoid
	 * race conditions with modem_iface_uart_isr.
	 */
	if (iface->dev) {
		LOG_WRN("Device %s already inited", iface->dev->name);
		uart_rx_disable(iface->dev);
	}

	iface->dev = dev;
	data = iface->iface_data;

	/* Configure async UART callback */
	rc = uart_callback_set(dev, iface_uart_async_callback, iface);
	if (rc < 0) {
		LOG_ERR("Failed to set UART callback");
		return rc;
	}
	/* Enable reception permanently on the interface */
	k_mem_slab_alloc(&uart_modem_async_rx_slab, (void **)&buf, K_FOREVER);
	rc = uart_rx_enable(dev, buf, RX_BUFFER_SIZE, CONFIG_MODEM_IFACE_UART_ASYNC_RX_TIMEOUT_US);
	if (rc < 0) {
		LOG_ERR("Failed to enable UART RX");
	}
	return rc;
}

int modem_iface_uart_init(struct modem_iface *iface, struct modem_iface_uart_data *data,
			  const struct modem_iface_uart_config *config)
{
	int ret;

	if (iface == NULL || data == NULL || config == NULL) {
		return -EINVAL;
	}

	iface->iface_data = data;
	iface->read = modem_iface_uart_async_read;
	iface->write = modem_iface_uart_async_write;

	ring_buf_init(&data->rx_rb, config->rx_rb_buf_len, config->rx_rb_buf);
	k_sem_init(&data->rx_sem, 0, 1);
	k_sem_init(&data->tx_sem, 0, 1);

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
