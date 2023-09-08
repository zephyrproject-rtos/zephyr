/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "modem_backend_uart_async.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(modem_backend_uart);

#include <zephyr/kernel.h>
#include <string.h>

#define MODEM_BACKEND_UART_ASYNC_STATE_TRANSMITTING_BIT       (0)
#define MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF0_USED_BIT       (1)
#define MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF1_USED_BIT       (2)
#define MODEM_BACKEND_UART_ASYNC_STATE_RX_RBUF_USED_INDEX_BIT (3)

static void modem_backend_uart_async_flush(struct modem_backend_uart *backend)
{
	uint8_t c;

	while (uart_fifo_read(backend->uart, &c, 1) > 0) {
		continue;
	}
}

static uint8_t modem_backend_uart_async_rx_rbuf_used_index(struct modem_backend_uart *backend)
{
	return atomic_test_bit(&backend->async.state,
			       MODEM_BACKEND_UART_ASYNC_STATE_RX_RBUF_USED_INDEX_BIT);
}

static void modem_backend_uart_async_rx_rbuf_used_swap(struct modem_backend_uart *backend)
{
	uint8_t rx_rbuf_index = modem_backend_uart_async_rx_rbuf_used_index(backend);

	if (rx_rbuf_index) {
		atomic_clear_bit(&backend->async.state,
				 MODEM_BACKEND_UART_ASYNC_STATE_RX_RBUF_USED_INDEX_BIT);
	} else {
		atomic_set_bit(&backend->async.state,
			       MODEM_BACKEND_UART_ASYNC_STATE_RX_RBUF_USED_INDEX_BIT);
	}
}

static void modem_backend_uart_async_event_handler(const struct device *dev,
						   struct uart_event *evt, void *user_data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *) user_data;

	uint8_t receive_rb_used_index;
	uint32_t received;

	switch (evt->type) {
	case UART_TX_DONE:
		atomic_clear_bit(&backend->async.state,
				 MODEM_BACKEND_UART_ASYNC_STATE_TRANSMITTING_BIT);

		break;

	case UART_TX_ABORTED:
		LOG_WRN("Transmit aborted");
		atomic_clear_bit(&backend->async.state,
				 MODEM_BACKEND_UART_ASYNC_STATE_TRANSMITTING_BIT);

		break;

	case UART_RX_BUF_REQUEST:
		if (!atomic_test_and_set_bit(&backend->async.state,
					     MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF0_USED_BIT)) {
			uart_rx_buf_rsp(backend->uart, backend->async.receive_bufs[0],
					backend->async.receive_buf_size);

			break;
		}

		if (!atomic_test_and_set_bit(&backend->async.state,
					     MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF1_USED_BIT)) {
			uart_rx_buf_rsp(backend->uart, backend->async.receive_bufs[1],
					backend->async.receive_buf_size);

			break;
		}

		LOG_WRN("No receive buffer available");
		break;

	case UART_RX_BUF_RELEASED:
		if (evt->data.rx_buf.buf == backend->async.receive_bufs[0]) {
			atomic_clear_bit(&backend->async.state,
					 MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF0_USED_BIT);

			break;
		}

		if (evt->data.rx_buf.buf == backend->async.receive_bufs[1]) {
			atomic_clear_bit(&backend->async.state,
					 MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF1_USED_BIT);

			break;
		}

		LOG_WRN("Unknown receive buffer released");
		break;

	case UART_RX_RDY:
		receive_rb_used_index = modem_backend_uart_async_rx_rbuf_used_index(backend);

		received = ring_buf_put(&backend->async.receive_rdb[receive_rb_used_index],
				       &evt->data.rx.buf[evt->data.rx.offset],
				       evt->data.rx.len);

		if (received < evt->data.rx.len) {
			ring_buf_reset(&backend->async.receive_rdb[receive_rb_used_index]);
			LOG_WRN("Receive buffer overrun");
			break;
		}

		k_work_submit(&backend->receive_ready_work);
		break;

	case UART_RX_DISABLED:
		k_work_submit(&backend->async.rx_disabled_work);
		break;

	case UART_RX_STOPPED:
		LOG_WRN("Receive stopped for reasons: %u", (uint8_t)evt->data.rx_stop.reason);
		break;

	default:
		break;
	}
}

static int modem_backend_uart_async_open(void *data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;
	int ret;

	atomic_set(&backend->async.state, 0);
	modem_backend_uart_async_flush(backend);
	ring_buf_reset(&backend->async.receive_rdb[0]);
	ring_buf_reset(&backend->async.receive_rdb[1]);

	/* Reserve receive buffer 0 */
	atomic_set_bit(&backend->async.state,
		       MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF0_USED_BIT);

	/*
	 * Receive buffer 0 is used internally by UART, receive ring buffer 0 is
	 * used to store received data.
	 */
	ret = uart_rx_enable(backend->uart, backend->async.receive_bufs[0],
			     backend->async.receive_buf_size, 3000);

	if (ret < 0) {
		return ret;
	}

	modem_pipe_notify_opened(&backend->pipe);
	return 0;
}

static int modem_backend_uart_async_transmit(void *data, const uint8_t *buf, size_t size)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;
	bool transmitting;
	uint32_t bytes_to_transmit;
	int ret;

	transmitting = atomic_test_and_set_bit(&backend->async.state,
					       MODEM_BACKEND_UART_ASYNC_STATE_TRANSMITTING_BIT);

	if (transmitting) {
		return 0;
	}

	/* Determine amount of bytes to transmit */
	bytes_to_transmit = (size < backend->async.transmit_buf_size)
			  ? size
			  : backend->async.transmit_buf_size;

	/* Copy buf to transmit buffer which is passed to UART */
	memcpy(backend->async.transmit_buf, buf, bytes_to_transmit);

	ret = uart_tx(backend->uart, backend->async.transmit_buf, bytes_to_transmit,
		      CONFIG_MODEM_BACKEND_UART_ASYNC_TRANSMIT_TIMEOUT_MS * 1000L);

	if (ret < 0) {
		LOG_WRN("Failed to start async transmit");
		return ret;
	}

	return (int)bytes_to_transmit;
}

static int modem_backend_uart_async_receive(void *data, uint8_t *buf, size_t size)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;

	uint32_t received;
	uint8_t receive_rdb_unused;

	received = 0;
	receive_rdb_unused = modem_backend_uart_async_rx_rbuf_used_index(backend) ? 0 : 1;

	/* Read data from unused ring double buffer first */
	received += ring_buf_get(&backend->async.receive_rdb[receive_rdb_unused], buf, size);

	if (ring_buf_is_empty(&backend->async.receive_rdb[receive_rdb_unused]) == false) {
		return (int)received;
	}

	/* Swap receive ring double buffer */
	modem_backend_uart_async_rx_rbuf_used_swap(backend);

	/* Read data from previously used buffer */
	receive_rdb_unused = modem_backend_uart_async_rx_rbuf_used_index(backend) ? 0 : 1;

	received += ring_buf_get(&backend->async.receive_rdb[receive_rdb_unused],
				   &buf[received], (size - received));

	return (int)received;
}

static int modem_backend_uart_async_close(void *data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;

	uart_rx_disable(backend->uart);
	return 0;
}

struct modem_pipe_api modem_backend_uart_async_api = {
	.open = modem_backend_uart_async_open,
	.transmit = modem_backend_uart_async_transmit,
	.receive = modem_backend_uart_async_receive,
	.close = modem_backend_uart_async_close,
};

bool modem_backend_uart_async_is_supported(struct modem_backend_uart *backend)
{
	return uart_callback_set(backend->uart, modem_backend_uart_async_event_handler,
				 backend) == 0;
}

static void modem_backend_uart_async_notify_closed(struct k_work *item)
{
	struct modem_backend_uart_async *async =
		CONTAINER_OF(item, struct modem_backend_uart_async, rx_disabled_work);

	struct modem_backend_uart *backend =
		CONTAINER_OF(async, struct modem_backend_uart, async);

	modem_pipe_notify_closed(&backend->pipe);
}

void modem_backend_uart_async_init(struct modem_backend_uart *backend,
				   const struct modem_backend_uart_config *config)
{
	uint32_t receive_buf_size_quarter = config->receive_buf_size / 4;

	/* Split receive buffer into 4 buffers, use 2 parts for UART receive double buffer */
	backend->async.receive_buf_size = receive_buf_size_quarter;
	backend->async.receive_bufs[0] = &config->receive_buf[0];
	backend->async.receive_bufs[1] = &config->receive_buf[receive_buf_size_quarter];

	/* Use remaining 2 parts for receive double ring buffer */
	ring_buf_init(&backend->async.receive_rdb[0], receive_buf_size_quarter,
		      &config->receive_buf[receive_buf_size_quarter * 2]);

	ring_buf_init(&backend->async.receive_rdb[1], receive_buf_size_quarter,
		      &config->receive_buf[receive_buf_size_quarter * 3]);

	backend->async.transmit_buf = config->transmit_buf;
	backend->async.transmit_buf_size = config->transmit_buf_size;
	k_work_init(&backend->async.rx_disabled_work, modem_backend_uart_async_notify_closed);
	modem_pipe_init(&backend->pipe, backend, &modem_backend_uart_async_api);
}
