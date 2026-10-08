/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief A driver for sending and receiving mcumgr packets over UART.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>
#include <zephyr/drivers/console/uart_mcumgr.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_mcumgr, CONFIG_MCUMGR_TRANSPORT_LOG_LEVEL);

/**
 * Whether an instance uses raw framing. Constant when only one framing is built.
 */
static inline bool uart_mcumgr_is_raw(const struct uart_mcumgr *mcumgr)
{
	if (!IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_SMP_OVER_CONSOLE)) {
		return true;
	}

	if (!IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_RAW_BINARY_NON_SMP_OVER_CONSOLE)) {
		return false;
	}

	return mcumgr->framing == UART_MCUMGR_FRAMING_RAW;
}

static struct uart_mcumgr_rx_buf *uart_mcumgr_alloc_rx_buf(struct uart_mcumgr *mcumgr)
{
	struct uart_mcumgr_rx_buf *rx_buf;
	void *block;
	int rc;

	rc = k_mem_slab_alloc(mcumgr->rx_slab, &block, K_NO_WAIT);
	if (rc != 0) {
		return NULL;
	}

	rx_buf = block;
	rx_buf->length = 0;
	return rx_buf;
}

void uart_mcumgr_free_rx_buf(struct uart_mcumgr *mcumgr, struct uart_mcumgr_rx_buf *rx_buf)
{
	void *block;

	block = rx_buf;
	k_mem_slab_free(mcumgr->rx_slab, block);
}

#if !defined(CONFIG_MCUMGR_TRANSPORT_UART_ASYNC)
/**
 * Reads a chunk of received data from the UART.
 */
static int uart_mcumgr_read_chunk(struct uart_mcumgr *mcumgr, void *buf, int capacity)
{
	return uart_fifo_read(mcumgr->dev, buf, capacity);
}
#endif

/**
 * Processes a single incoming byte.
 */
static struct uart_mcumgr_rx_buf *uart_mcumgr_rx_byte(struct uart_mcumgr *mcumgr, uint8_t byte)
{
	struct uart_mcumgr_rx_buf *rx_buf;

	if (uart_mcumgr_is_raw(mcumgr)) {
		/* Raw packets have no line structure, so each byte is a fragment. */
		rx_buf = uart_mcumgr_alloc_rx_buf(mcumgr);
		if (rx_buf == NULL) {
			LOG_WRN("Insufficient buffers, fragment dropped");
			return NULL;
		}

		rx_buf->data[rx_buf->length++] = byte;
		return rx_buf;
	}

	/* Frame markers never occur inside a frame, so they always start a new line. */
	if (byte == MCUMGR_SERIAL_HDR_PKT_1 || byte == MCUMGR_SERIAL_HDR_FRAG_1) {
		mcumgr->ignoring = false;
		if (mcumgr->cur_buf != NULL) {
			mcumgr->cur_buf->length = 0;
		}
	}

	if (!mcumgr->ignoring) {
		if (mcumgr->cur_buf == NULL) {
			mcumgr->cur_buf = uart_mcumgr_alloc_rx_buf(mcumgr);
			if (mcumgr->cur_buf == NULL) {
				LOG_WRN("Insufficient buffers, fragment dropped");
				mcumgr->ignoring = true;
			}
		}
	}

	rx_buf = mcumgr->cur_buf;
	if (!mcumgr->ignoring) {
		if (rx_buf->length >= sizeof(rx_buf->data)) {
			LOG_WRN("Line too long, fragment dropped");
			uart_mcumgr_free_rx_buf(mcumgr, mcumgr->cur_buf);
			mcumgr->cur_buf = NULL;
			mcumgr->ignoring = true;
		} else {
			rx_buf->data[rx_buf->length++] = byte;
		}
	}

	if (byte == '\n') {
		/* Fragment complete. */
		if (mcumgr->ignoring) {
			mcumgr->ignoring = false;
		} else {
			mcumgr->cur_buf = NULL;
			return rx_buf;
		}
	}

	return NULL;
}

#if defined(CONFIG_MCUMGR_TRANSPORT_UART_ASYNC)
static void uart_mcumgr_async(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct uart_mcumgr *mcumgr = user_data;
	struct uart_mcumgr_rx_buf *rx_buf;
	uint8_t *p;
	int len;

	switch (evt->type) {
	case UART_TX_DONE:
	case UART_TX_ABORTED:
		break;
	case UART_RX_RDY:
		len = evt->data.rx.len;
		p = &evt->data.rx.buf[evt->data.rx.offset];

		for (int i = 0; i < len; i++) {
			rx_buf = uart_mcumgr_rx_byte(mcumgr, p[i]);
			if (rx_buf != NULL) {
				mcumgr->recv_cb(rx_buf, mcumgr->user_data);
			}
		}
		break;
	case UART_RX_DISABLED:
		mcumgr->async_current = 0;
		break;
	case UART_RX_BUF_REQUEST:
		/*
		 * Note that when buffer gets filled, the UART_RX_BUF_RELEASED will be reported,
		 * aside to UART_RX_RDY.  The UART_RX_BUF_RELEASED is not processed because
		 * it has been assumed that the mcumgr will be able to consume bytes faster
		 * than UART will receive them and, since there is nothing to release, only
		 * UART_RX_BUF_REQUEST is processed.
		 */
		++mcumgr->async_current;
		mcumgr->async_current %= CONFIG_MCUMGR_TRANSPORT_UART_ASYNC_BUFS;
		uart_rx_buf_rsp(dev, mcumgr->async_buf[mcumgr->async_current],
				sizeof(mcumgr->async_buf[mcumgr->async_current]));
		break;
	case UART_RX_BUF_RELEASED:
	case UART_RX_STOPPED:
		break;
	}
}
#else
/**
 * ISR that is called when UART bytes are received.
 */
static void uart_mcumgr_isr(const struct device *unused, void *user_data)
{
	struct uart_mcumgr *mcumgr = user_data;
	struct uart_mcumgr_rx_buf *rx_buf;
	uint8_t buf[32];
	int chunk_len;
	int i;

	ARG_UNUSED(unused);

	uart_irq_update(mcumgr->dev);

	if (uart_irq_rx_ready(mcumgr->dev) <= 0) {
		return;
	}

	while (true) {
		chunk_len = uart_mcumgr_read_chunk(mcumgr, buf, sizeof(buf));
		if (chunk_len <= 0) {
			break;
		}

		for (i = 0; i < chunk_len; i++) {
			rx_buf = uart_mcumgr_rx_byte(mcumgr, buf[i]);
			if (rx_buf != NULL) {
				mcumgr->recv_cb(rx_buf, mcumgr->user_data);
			}
		}
	}
}
#endif

/**
 * Sends raw data over the UART.
 */
static int uart_mcumgr_send_raw(const void *data, int len, void *ctx)
{
	struct uart_mcumgr *mcumgr = ctx;
	const uint8_t *u8p;

	u8p = data;
	while (len--) {
		uart_poll_out(mcumgr->dev, *u8p++);
	}

	return 0;
}

int uart_mcumgr_send(struct uart_mcumgr *mcumgr, const uint8_t *data, int len)
{
#if defined(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_SMP_OVER_CONSOLE)
	if (!uart_mcumgr_is_raw(mcumgr)) {
		return mcumgr_serial_tx_pkt(data, len, uart_mcumgr_send_raw, mcumgr);
	}
#endif

	return uart_mcumgr_send_raw(data, len, mcumgr);
}

#if defined(CONFIG_MCUMGR_TRANSPORT_UART_ASYNC)
static int uart_mcumgr_setup(struct uart_mcumgr *mcumgr)
{
	int rc;

	rc = uart_callback_set(mcumgr->dev, uart_mcumgr_async, mcumgr);
	if (rc != 0) {
		return rc;
	}

	return uart_rx_enable(mcumgr->dev, mcumgr->async_buf[0], sizeof(mcumgr->async_buf[0]),
			      CONFIG_MCUMGR_TRANSPORT_UART_ASYNC_RX_TIMEOUT_US);
}
#else
static int uart_mcumgr_setup(struct uart_mcumgr *mcumgr)
{
	int rc;

	uart_irq_rx_disable(mcumgr->dev);
	uart_irq_tx_disable(mcumgr->dev);

	rc = uart_irq_callback_user_data_set(mcumgr->dev, uart_mcumgr_isr, mcumgr);
	if (rc != 0) {
		return rc;
	}

	uart_irq_rx_enable(mcumgr->dev);

	return 0;
}
#endif

int uart_mcumgr_register(struct uart_mcumgr *mcumgr, uart_mcumgr_recv_fn *cb, void *user_data)
{
	int rc = -ENODEV;

	if (device_is_ready(mcumgr->dev)) {
		mcumgr->recv_cb = cb;
		mcumgr->user_data = user_data;
		rc = uart_mcumgr_setup(mcumgr);
	}

	if (rc != 0) {
		LOG_ERR("%s setup failed: %d", mcumgr->dev->name, rc);
	}

	return rc;
}
