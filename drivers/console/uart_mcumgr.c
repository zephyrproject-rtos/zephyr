/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief A driver for sending and receiving mcumgr packets over UART.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/mgmt/mcumgr/serial.h>
#include <zephyr/drivers/console/uart_mcumgr.h>

static const struct device *uart_mcumgr_dev;

/** Callback to execute when a valid fragment has been received. */
static uart_mcumgr_recv_fn *uart_mgumgr_recv_cb;

/** Contains the fragment currently being received. */
static struct uart_mcumgr_rx_buf *uart_mcumgr_cur_buf;

/**
 * Whether the line currently being read should be ignored.  This is true if
 * the line is too long or if there is no buffer available to hold it.
 */
static bool uart_mcumgr_ignoring;

/** Contains buffers to hold incoming request fragments. */
K_MEM_SLAB_DEFINE(uart_mcumgr_slab, sizeof(struct uart_mcumgr_rx_buf),
		  CONFIG_UART_MCUMGR_RX_BUF_COUNT, 1);

#if defined(CONFIG_MCUMGR_SMP_UART_ASYNC)
uint8_t async_buffer[CONFIG_MCUMGR_SMP_UART_ASYNC_BUFS][CONFIG_MCUMGR_SMP_UART_ASYNC_BUF_SIZE];
static int async_current;
#endif

static struct uart_mcumgr_rx_buf *uart_mcumgr_alloc_rx_buf(void)
{
	struct uart_mcumgr_rx_buf *rx_buf;
	void *block;
	int rc;

	rc = k_mem_slab_alloc(&uart_mcumgr_slab, &block, K_NO_WAIT);
	if (rc != 0) {
		return NULL;
	}

	rx_buf = block;
	rx_buf->length = 0;
	return rx_buf;
}

void uart_mcumgr_free_rx_buf(struct uart_mcumgr_rx_buf *rx_buf)
{
	void *block;

	block = rx_buf;
	k_mem_slab_free(&uart_mcumgr_slab, &block);
}

#if !defined(CONFIG_MCUMGR_SMP_UART_ASYNC)
/**
 * Reads a chunk of received data from the UART.
 */
static int uart_mcumgr_read_chunk(void *buf, int capacity)
{
	if (!uart_irq_rx_ready(uart_mcumgr_dev)) {
		return 0;
	}

	return uart_fifo_read(uart_mcumgr_dev, buf, capacity);
}
#endif

/**
 * Processes a single incoming byte.
 */
static struct uart_mcumgr_rx_buf *uart_mcumgr_rx_byte(uint8_t byte)
{
	struct uart_mcumgr_rx_buf *rx_buf;

	if (!uart_mcumgr_ignoring) {
		if (uart_mcumgr_cur_buf == NULL) {
			uart_mcumgr_cur_buf = uart_mcumgr_alloc_rx_buf();
			if (uart_mcumgr_cur_buf == NULL) {
				/* Insufficient buffers; drop this fragment. */
				uart_mcumgr_ignoring = true;
			}
		}
	}

	rx_buf = uart_mcumgr_cur_buf;
	if (!uart_mcumgr_ignoring) {
		if (rx_buf->length >= sizeof(rx_buf->data)) {
			/* Line too long; drop this fragment. */
			uart_mcumgr_free_rx_buf(uart_mcumgr_cur_buf);
			uart_mcumgr_cur_buf = NULL;
			uart_mcumgr_ignoring = true;
		} else {
			rx_buf->data[rx_buf->length++] = byte;
		}
	}

	if (byte == '\n') {
		/* Fragment complete. */
		if (uart_mcumgr_ignoring) {
			uart_mcumgr_ignoring = false;
		} else {
			uart_mcumgr_cur_buf = NULL;
			return rx_buf;
		}
	}

	return NULL;
}

#if defined(CONFIG_MCUMGR_SMP_UART_ASYNC)
static void uart_mcumgr_async(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct uart_mcumgr_rx_buf *rx_buf;
	uint8_t *p;
	int len;

	ARG_UNUSED(dev);

	switch (evt->type) {
	case UART_TX_DONE:
	case UART_TX_ABORTED:
		break;
	case UART_RX_RDY:
		len = evt->data.rx.len;
		p = &evt->data.rx.buf[evt->data.rx.offset];

		for (int i = 0; i < len; i++) {
			rx_buf = uart_mcumgr_rx_byte(p[i]);
			if (rx_buf != NULL) {
				uart_mgumgr_recv_cb(rx_buf);
			}
		}
		break;
	case UART_RX_DISABLED:
		async_current = 0;
		break;
	case UART_RX_BUF_REQUEST:
		/*
		 * Note that when buffer gets filled, the UART_RX_BUF_RELEASED will be reported,
		 * aside to UART_RX_RDY.  The UART_RX_BUF_RELEASED is not processed because
		 * it has been assumed that the mcumgr will be able to consume bytes faster
		 * than UART will receive them and, since there is nothing to release, only
		 * UART_RX_BUF_REQUEST is processed.
		 */
		++async_current;
		async_current %= CONFIG_MCUMGR_SMP_UART_ASYNC_BUFS;
		uart_rx_buf_rsp(dev, async_buffer[async_current],
				sizeof(async_buffer[async_current]));
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
	struct uart_mcumgr_rx_buf *rx_buf;
	uint8_t buf[32];
	int chunk_len;
	int i;

	ARG_UNUSED(unused);
	ARG_UNUSED(user_data);

	while (uart_irq_update(uart_mcumgr_dev) &&
	       uart_irq_is_pending(uart_mcumgr_dev)) {

		chunk_len = uart_mcumgr_read_chunk(buf, sizeof(buf));
		if (chunk_len == 0) {
			continue;
		}

		for (i = 0; i < chunk_len; i++) {
			rx_buf = uart_mcumgr_rx_byte(buf[i]);
			if (rx_buf != NULL) {
				uart_mgumgr_recv_cb(rx_buf);
			}
		}
	}
}
#endif

/**
 * Sends raw data over the UART.
 */
static int uart_mcumgr_send_raw(const void *data, int len, void *arg)
{
	const uint8_t *u8p;

	u8p = data;
	while (len--) {
		uart_poll_out(uart_mcumgr_dev, *u8p++);
	}

	return 0;
}

int uart_mcumgr_send(const uint8_t *data, int len)
{
	return mcumgr_serial_tx_pkt(data, len, uart_mcumgr_send_raw, NULL);
}


#if defined(CONFIG_MCUMGR_SMP_UART_ASYNC)
static void uart_mcumgr_setup(const struct device *uart)
{
	uart_callback_set(uart, uart_mcumgr_async, NULL);

	uart_rx_enable(uart, async_buffer[0], sizeof(async_buffer[0]), 0);
}
#else
static void uart_mcumgr_setup(const struct device *uart)
{
	uint8_t c;

	uart_irq_rx_disable(uart);
	uart_irq_tx_disable(uart);

	/* Drain the fifo */
	while (uart_fifo_read(uart, &c, 1)) {
		continue;
	}

	uart_irq_callback_set(uart, uart_mcumgr_isr);

	uart_irq_rx_enable(uart);
}
#endif

void uart_mcumgr_register(uart_mcumgr_recv_fn *cb)
{
	uart_mgumgr_recv_cb = cb;

	uart_mcumgr_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_uart_mcumgr));

	if (device_is_ready(uart_mcumgr_dev)) {
		uart_mcumgr_setup(uart_mcumgr_dev);
	}
}
