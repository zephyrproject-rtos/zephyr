/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "modem_backend_uart_async.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_backend_uart_async, CONFIG_MODEM_MODULES_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <string.h>

enum {
	MODEM_BACKEND_UART_ASYNC_STATE_TRANSMITTING_BIT,
	MODEM_BACKEND_UART_ASYNC_STATE_RECEIVING_BIT,
	MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF0_USED_BIT,
	MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF1_USED_BIT,
	MODEM_BACKEND_UART_ASYNC_STATE_OPEN_BIT,
};

static bool modem_backend_uart_async_is_uart_stopped(struct modem_backend_uart *backend)
{
	if (!atomic_test_bit(&backend->async.state,
			    MODEM_BACKEND_UART_ASYNC_STATE_TRANSMITTING_BIT) &&
	    !atomic_test_bit(&backend->async.state,
			    MODEM_BACKEND_UART_ASYNC_STATE_RECEIVING_BIT) &&
	    !atomic_test_bit(&backend->async.state,
			    MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF0_USED_BIT) &&
	    !atomic_test_bit(&backend->async.state,
			    MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF1_USED_BIT)) {
		return true;
	}

	return false;
}

static bool modem_backend_uart_async_is_open(struct modem_backend_uart *backend)
{
	return atomic_test_bit(&backend->async.state,
			       MODEM_BACKEND_UART_ASYNC_STATE_OPEN_BIT);
}

static void modem_backend_uart_async_event_handler(const struct device *dev,
						   struct uart_event *evt, void *user_data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *) user_data;
	k_spinlock_key_t key;
	uint32_t received;

	switch (evt->type) {
	case UART_TX_DONE:
		atomic_clear_bit(&backend->async.state,
				 MODEM_BACKEND_UART_ASYNC_STATE_TRANSMITTING_BIT);
		k_work_submit(&backend->transmit_idle_work);

		break;

	case UART_TX_ABORTED:
		if (modem_backend_uart_async_is_open(backend)) {
			LOG_WRN("Transmit aborted (%zu sent)", evt->data.tx.len);
		}
		atomic_clear_bit(&backend->async.state,
				 MODEM_BACKEND_UART_ASYNC_STATE_TRANSMITTING_BIT);
		k_work_submit(&backend->transmit_idle_work);

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
		key = k_spin_lock(&backend->async.receive_rb_lock);
		received = ring_buf_put(&backend->async.receive_rb,
					&evt->data.rx.buf[evt->data.rx.offset],
					evt->data.rx.len);

		if (received < evt->data.rx.len) {
			ring_buf_reset(&backend->async.receive_rb);
			k_spin_unlock(&backend->async.receive_rb_lock, key);
			LOG_WRN("Receive buffer overrun (%zu/%zu dropped)",
				evt->data.rx.len - received, evt->data.rx.len);
			break;
		}

		k_spin_unlock(&backend->async.receive_rb_lock, key);
		k_work_submit(&backend->receive_ready_work);
		break;

	case UART_RX_DISABLED:
		atomic_clear_bit(&backend->async.state,
				 MODEM_BACKEND_UART_ASYNC_STATE_RECEIVING_BIT);
		break;

	case UART_RX_STOPPED:
		LOG_WRN("Receive stopped for reasons: %u", (uint8_t)evt->data.rx_stop.reason);
		break;

	default:
		break;
	}

	if (modem_backend_uart_async_is_uart_stopped(backend)) {
		k_work_submit(&backend->async.rx_disabled_work);
	}
}

static int modem_backend_uart_async_open(void *data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;
	int ret;

	atomic_set(&backend->async.state, 0);
	ring_buf_reset(&backend->async.receive_rb);

	/* Reserve receive buffer 0 */
	atomic_set_bit(&backend->async.state,
		       MODEM_BACKEND_UART_ASYNC_STATE_RX_BUF0_USED_BIT);

	/*
	 * Receive buffer 0 is used internally by UART, receive ring buffer 0 is
	 * used to store received data.
	 */
	ret = uart_rx_enable(backend->uart, backend->async.receive_bufs[0],
			     backend->async.receive_buf_size,
			     CONFIG_MODEM_BACKEND_UART_ASYNC_RECEIVE_IDLE_TIMEOUT_MS * 1000L);

	if (ret < 0) {
		return ret;
	}

	atomic_set_bit(&backend->async.state,
		       MODEM_BACKEND_UART_ASYNC_STATE_RECEIVING_BIT);
	atomic_set_bit(&backend->async.state,
		       MODEM_BACKEND_UART_ASYNC_STATE_OPEN_BIT);

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

	if (ret != 0) {
		LOG_ERR("Failed to %s %u bytes. (%d)",
			"start async transmit for", bytes_to_transmit, ret);
		return ret;
	}

	return (int)bytes_to_transmit;
}

static int modem_backend_uart_async_receive(void *data, uint8_t *buf, size_t size)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;
	k_spinlock_key_t key;
	uint32_t received;
	bool empty;

	key = k_spin_lock(&backend->async.receive_rb_lock);
	received = ring_buf_get(&backend->async.receive_rb, buf, size);
	empty = ring_buf_is_empty(&backend->async.receive_rb);
	k_spin_unlock(&backend->async.receive_rb_lock, key);

	if (!empty) {
		k_work_submit(&backend->receive_ready_work);
	}

	return (int)received;
}

static int modem_backend_uart_async_close(void *data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;

	atomic_clear_bit(&backend->async.state, MODEM_BACKEND_UART_ASYNC_STATE_OPEN_BIT);
	uart_tx_abort(backend->uart);
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

	/* Use half the receive buffer for UART receive buffers */
	backend->async.receive_buf_size = receive_buf_size_quarter;
	backend->async.receive_bufs[0] = &config->receive_buf[0];
	backend->async.receive_bufs[1] = &config->receive_buf[receive_buf_size_quarter];

	/* Use half the receive buffer for the received data ring buffer */
	ring_buf_init(&backend->async.receive_rb, (receive_buf_size_quarter * 2),
		      &config->receive_buf[receive_buf_size_quarter * 2]);

	backend->async.transmit_buf = config->transmit_buf;
	backend->async.transmit_buf_size = config->transmit_buf_size;
	k_work_init(&backend->async.rx_disabled_work, modem_backend_uart_async_notify_closed);
	modem_pipe_init(&backend->pipe, backend, &modem_backend_uart_async_api);
}
