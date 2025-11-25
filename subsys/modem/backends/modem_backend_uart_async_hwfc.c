/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "modem_backend_uart_async.h"
#include "../modem_workqueue.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_backend_uart_async_hwfc, CONFIG_MODEM_MODULES_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

struct rx_buf_t {
	atomic_t ref_counter;
	uint8_t buf[];
};

static inline struct rx_buf_t *block_start_get(struct modem_backend_uart_async *async, uint8_t *buf)
{
	size_t block_num;

	/* Find the correct block. */
	block_num = (((size_t)buf - sizeof(struct rx_buf_t) - (size_t)async->rx_slab.buffer) /
		     async->rx_buf_size);

	return (struct rx_buf_t *) &async->rx_slab.buffer[block_num * async->rx_buf_size];
}

static struct rx_buf_t *rx_buf_alloc(struct modem_backend_uart_async *async)
{
	struct rx_buf_t *buf;

	if (k_mem_slab_alloc(&async->rx_slab, (void **) &buf, K_NO_WAIT)) {
		return NULL;
	}
	atomic_set(&buf->ref_counter, 1);

	return buf;
}

static void rx_buf_ref(struct modem_backend_uart_async *async, void *buf)
{
	atomic_inc(&(block_start_get(async, buf)->ref_counter));
}

static void rx_buf_unref(struct modem_backend_uart_async *async, void *buf)
{
	struct rx_buf_t *uart_buf = block_start_get(async, buf);
	atomic_t ref_counter = atomic_dec(&uart_buf->ref_counter);

	if (ref_counter == 1) {
		k_mem_slab_free(&async->rx_slab, (void *)uart_buf);
	}
}

enum {
	MODEM_BACKEND_UART_ASYNC_STATE_OPEN_BIT,
	MODEM_BACKEND_UART_ASYNC_STATE_TRANSMIT_BIT,
	MODEM_BACKEND_UART_ASYNC_STATE_RECOVERY_BIT,
};

static int modem_backend_uart_async_hwfc_rx_enable(struct modem_backend_uart *backend)
{
	int ret;
	struct rx_buf_t *buf = rx_buf_alloc(&backend->async);

	if (!buf) {
		return -ENOMEM;
	}

	ret = uart_rx_enable(backend->uart, buf->buf,
			     backend->async.rx_buf_size - sizeof(struct rx_buf_t),
			     CONFIG_MODEM_BACKEND_UART_ASYNC_RECEIVE_IDLE_TIMEOUT_MS * 1000);
	if (ret) {
		rx_buf_unref(&backend->async, buf->buf);
		return ret;
	}

	return 0;
}

static void modem_backend_uart_async_hwfc_rx_recovery(struct modem_backend_uart *backend)
{
	int err;

	if (!atomic_test_bit(&backend->async.common.state,
			     MODEM_BACKEND_UART_ASYNC_STATE_RECOVERY_BIT)) {
		return;
	}

	err = modem_backend_uart_async_hwfc_rx_enable(backend);
	if (err) {
		LOG_DBG("RX recovery failed: %d", err);
		return;
	}

	if (!atomic_test_and_clear_bit(&backend->async.common.state,
				       MODEM_BACKEND_UART_ASYNC_STATE_RECOVERY_BIT)) {
		/* Closed during recovery. */
		uart_rx_disable(backend->uart);
	} else {
		LOG_DBG("RX recovery success");
	}
}

static bool modem_backend_uart_async_hwfc_is_uart_stopped(const struct modem_backend_uart *backend)
{
	if (!atomic_test_bit(&backend->async.common.state,
			     MODEM_BACKEND_UART_ASYNC_STATE_OPEN_BIT) &&
	    !atomic_test_bit(&backend->async.common.state,
			     MODEM_BACKEND_UART_ASYNC_STATE_RECOVERY_BIT) &&
	    !atomic_test_bit(&backend->async.common.state,
			     MODEM_BACKEND_UART_ASYNC_STATE_TRANSMIT_BIT)) {
		return true;
	}

	return false;
}

static bool modem_backend_uart_async_hwfc_is_open(const struct modem_backend_uart *backend)
{
	return atomic_test_bit(&backend->async.common.state,
			       MODEM_BACKEND_UART_ASYNC_STATE_OPEN_BIT);
}

static void modem_backend_uart_async_hwfc_event_handler(const struct device *dev,
						   struct uart_event *evt, void *user_data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *) user_data;
	struct rx_queue_event rx_event;
	int err;

	switch (evt->type) {
	case UART_TX_DONE:
		atomic_clear_bit(&backend->async.common.state,
				 MODEM_BACKEND_UART_ASYNC_STATE_TRANSMIT_BIT);
		modem_work_submit(&backend->transmit_idle_work);
		break;

	case UART_TX_ABORTED:
		if (modem_backend_uart_async_hwfc_is_open(backend)) {
			LOG_WRN("Transmit aborted (%zu sent)", evt->data.tx.len);
		}
		atomic_clear_bit(&backend->async.common.state,
				 MODEM_BACKEND_UART_ASYNC_STATE_TRANSMIT_BIT);
		modem_work_submit(&backend->transmit_idle_work);

		break;

	case UART_RX_BUF_REQUEST:
		struct rx_buf_t *buf = rx_buf_alloc(&backend->async);

		if (!buf) {
			LOG_DBG("No receive buffer, disabling RX");
			break;
		}
		err = uart_rx_buf_rsp(backend->uart, buf->buf,
				      backend->async.rx_buf_size - sizeof(struct rx_buf_t));
		if (err) {
			LOG_ERR("uart_rx_buf_rsp: %d", err);
			rx_buf_unref(&backend->async, buf->buf);
		}
		break;

	case UART_RX_BUF_RELEASED:
		if (evt->data.rx_buf.buf) {
			rx_buf_unref(&backend->async, evt->data.rx_buf.buf);
		}
		break;

	case UART_RX_RDY:
		if (evt->data.rx.buf) {
			rx_buf_ref(&backend->async, evt->data.rx.buf);
			rx_event.buf = &evt->data.rx.buf[evt->data.rx.offset];
			rx_event.len = evt->data.rx.len;
			err = k_msgq_put(&backend->async.rx_queue, &rx_event, K_NO_WAIT);
			if (err) {
				LOG_WRN("RX queue overflow: %d (dropped %u)", err,
					evt->data.rx.len);
				rx_buf_unref(&backend->async, evt->data.rx.buf);
				break;
			}
			modem_work_schedule(&backend->receive_ready_work, K_NO_WAIT);
		}
		break;

	case UART_RX_DISABLED:
		if (atomic_test_bit(&backend->async.common.state,
				    MODEM_BACKEND_UART_ASYNC_STATE_OPEN_BIT)) {
			if (!atomic_test_and_set_bit(&backend->async.common.state,
						     MODEM_BACKEND_UART_ASYNC_STATE_RECOVERY_BIT)) {
				modem_work_schedule(&backend->receive_ready_work, K_NO_WAIT);
				LOG_DBG("RX recovery started");
			}
		}
		break;

	case UART_RX_STOPPED:
		LOG_WRN("Receive stopped for reasons: %u", (uint8_t)evt->data.rx_stop.reason);
		break;

	default:
		break;
	}

	if (modem_backend_uart_async_hwfc_is_uart_stopped(backend)) {
		modem_work_submit(&backend->async.common.rx_disabled_work);
	}
}

static int modem_backend_uart_async_hwfc_open(void *data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;
	struct rx_buf_t *buf = rx_buf_alloc(&backend->async);
	int ret;

	if (!buf) {
		return -ENOMEM;
	}

	if (backend->dtr_gpio) {
		gpio_pin_set_dt(backend->dtr_gpio, 1);
	}

	atomic_clear(&backend->async.common.state);
	atomic_set_bit(&backend->async.common.state, MODEM_BACKEND_UART_ASYNC_STATE_OPEN_BIT);

	ret = uart_rx_enable(backend->uart, buf->buf,
			     backend->async.rx_buf_size - sizeof(struct rx_buf_t),
			     CONFIG_MODEM_BACKEND_UART_ASYNC_RECEIVE_IDLE_TIMEOUT_MS * 1000L);
	if (ret < 0) {
		rx_buf_unref(&backend->async, buf->buf);
		atomic_clear(&backend->async.common.state);
		return ret;
	}

	modem_pipe_notify_opened(&backend->pipe);
	return 0;
}

#if CONFIG_MODEM_STATS
static uint32_t get_receive_buf_size(struct modem_backend_uart *backend)
{
	return (backend->async.rx_buf_size - sizeof(struct rx_buf_t)) * backend->async.rx_buf_count;
}

static void advertise_transmit_buf_stats(struct modem_backend_uart *backend, uint32_t length)
{
	modem_stats_buffer_advertise_length(&backend->transmit_buf_stats, length);
}

static void advertise_receive_buf_stats(struct modem_backend_uart *backend, uint32_t reserved)
{
	modem_stats_buffer_advertise_length(&backend->receive_buf_stats, reserved);
}
#endif

static uint32_t get_transmit_buf_size(const struct modem_backend_uart *backend)
{
	return backend->async.common.transmit_buf_size;
}

static int modem_backend_uart_async_hwfc_transmit(void *data, const uint8_t *buf, size_t size)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;
	bool transmitting;
	uint32_t bytes_to_transmit;
	int ret;

	transmitting = atomic_test_and_set_bit(&backend->async.common.state,
					       MODEM_BACKEND_UART_ASYNC_STATE_TRANSMIT_BIT);
	if (transmitting) {
		return 0;
	}

	/* Determine amount of bytes to transmit */
	bytes_to_transmit = MIN(size, get_transmit_buf_size(backend));

	/* Copy buf to transmit buffer which is passed to UART */
	memcpy(backend->async.common.transmit_buf, buf, bytes_to_transmit);

	ret = uart_tx(backend->uart, backend->async.common.transmit_buf, bytes_to_transmit,
		      CONFIG_MODEM_BACKEND_UART_ASYNC_TRANSMIT_TIMEOUT_MS * 1000L);

#if CONFIG_MODEM_STATS
	advertise_transmit_buf_stats(backend, bytes_to_transmit);
#endif

	if (ret != 0) {
		LOG_ERR("Failed to %s %u bytes. (%d)",
			"start async transmit for", bytes_to_transmit, ret);
		return ret;
	}

	return (int)bytes_to_transmit;
}

static int modem_backend_uart_async_hwfc_receive(void *data, uint8_t *buf, size_t size)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;
	size_t received = 0;
	size_t copy_size = 0;

#if CONFIG_MODEM_STATS
	struct rx_queue_event rx_event;
	size_t reserved = backend->async.rx_event.len;

	for (int i = 0; i < k_msgq_num_used_get(&backend->async.rx_queue); i++) {
		if (k_msgq_peek_at(&backend->async.rx_queue, &rx_event, i)) {
			break;
		}
		reserved += rx_event.len;
	}
	advertise_receive_buf_stats(backend, reserved);
#endif
	while (size > received) {
		/* Keeping track of the async.rx_event allows us to receive less than what the event
		 * indicates.
		 */
		if (backend->async.rx_event.len == 0) {
			if (k_msgq_get(&backend->async.rx_queue, &backend->async.rx_event,
				       K_NO_WAIT)) {
				break;
			}
		}
		copy_size = MIN(size - received, backend->async.rx_event.len);
		memcpy(buf, backend->async.rx_event.buf, copy_size);
		buf += copy_size;
		received += copy_size;
		backend->async.rx_event.buf += copy_size;
		backend->async.rx_event.len -= copy_size;

		if (backend->async.rx_event.len	== 0) {
			rx_buf_unref(&backend->async, backend->async.rx_event.buf);
		}
	}

	if (backend->async.rx_event.len != 0 ||
	    k_msgq_num_used_get(&backend->async.rx_queue) != 0) {
		modem_work_schedule(&backend->receive_ready_work, K_NO_WAIT);
	}

	modem_backend_uart_async_hwfc_rx_recovery(backend);

	return (int)received;
}

static int modem_backend_uart_async_hwfc_close(void *data)
{
	struct modem_backend_uart *backend = (struct modem_backend_uart *)data;

	atomic_clear_bit(&backend->async.common.state, MODEM_BACKEND_UART_ASYNC_STATE_OPEN_BIT);
	uart_tx_abort(backend->uart);

	if (!atomic_test_and_clear_bit(&backend->async.common.state,
				      MODEM_BACKEND_UART_ASYNC_STATE_RECOVERY_BIT)) {
		/* Disable the RX, if recovery is not ongoing. */
		uart_rx_disable(backend->uart);
	}

	if (backend->dtr_gpio) {
		gpio_pin_set_dt(backend->dtr_gpio, 0);
	}
	return 0;
}

static const struct modem_pipe_api modem_backend_uart_async_api = {
	.open = modem_backend_uart_async_hwfc_open,
	.transmit = modem_backend_uart_async_hwfc_transmit,
	.receive = modem_backend_uart_async_hwfc_receive,
	.close = modem_backend_uart_async_hwfc_close,
};

bool modem_backend_uart_async_is_supported(struct modem_backend_uart *backend)
{
	return uart_callback_set(backend->uart, modem_backend_uart_async_hwfc_event_handler,
				 backend) == 0;
}

static void modem_backend_uart_async_hwfc_notify_closed(struct k_work *item)
{
	struct modem_backend_uart_async_common *common =
		CONTAINER_OF(item, struct modem_backend_uart_async_common, rx_disabled_work);

	struct modem_backend_uart_async *async =
		CONTAINER_OF(common, struct modem_backend_uart_async, common);

	struct modem_backend_uart *backend =
		CONTAINER_OF(async, struct modem_backend_uart, async);

	modem_pipe_notify_closed(&backend->pipe);
}

#if CONFIG_MODEM_STATS
static void init_stats(struct modem_backend_uart *backend)
{
	char name[CONFIG_MODEM_STATS_BUFFER_NAME_SIZE];
	uint32_t receive_buf_size;
	uint32_t transmit_buf_size;

	receive_buf_size = get_receive_buf_size(backend);
	transmit_buf_size = get_transmit_buf_size(backend);

	snprintk(name, sizeof(name), "%s_%s", backend->uart->name, "rx");
	modem_stats_buffer_init(&backend->receive_buf_stats, name, receive_buf_size);
	snprintk(name, sizeof(name), "%s_%s", backend->uart->name, "tx");
	modem_stats_buffer_init(&backend->transmit_buf_stats, name, transmit_buf_size);
}
#endif

int modem_backend_uart_async_init(struct modem_backend_uart *backend,
				   const struct modem_backend_uart_config *config)
{
	int32_t buf_size = (int32_t)config->receive_buf_size;
	int err;

	backend->async.rx_buf_count = CONFIG_MODEM_BACKEND_UART_ASYNC_HWFC_BUFFER_COUNT;

	/* k_mem_slab_init requires a word-aligned buffer. */
	__ASSERT((uintptr_t)config->receive_buf % sizeof(void *) == 0,
		 "Receive buffer is not word-aligned");

	/* Make sure all the buffers will be aligned. */
	buf_size -= (config->receive_buf_size % (sizeof(uint32_t) * backend->async.rx_buf_count));
	backend->async.rx_buf_size = buf_size / backend->async.rx_buf_count;
	__ASSERT_NO_MSG(backend->async.rx_buf_size > sizeof(struct rx_buf_t));

	/* Initialize the RX buffers and event queue. */
	err = k_mem_slab_init(&backend->async.rx_slab, config->receive_buf,
			      backend->async.rx_buf_size, backend->async.rx_buf_count);
	if (err) {
		return err;
	}
	k_msgq_init(&backend->async.rx_queue, (char *)&backend->async.rx_queue_buf,
		sizeof(struct rx_queue_event), CONFIG_MODEM_BACKEND_UART_ASYNC_HWFC_BUFFER_COUNT);

	backend->async.common.transmit_buf = config->transmit_buf;
	backend->async.common.transmit_buf_size = config->transmit_buf_size;
	k_work_init(&backend->async.common.rx_disabled_work,
		    modem_backend_uart_async_hwfc_notify_closed);

	modem_pipe_init(&backend->pipe, backend, &modem_backend_uart_async_api);

#if CONFIG_MODEM_STATS
	init_stats(backend);
#endif
	return 0;
}
