/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/modem/backend/uart_slm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_backend_uart_slm, CONFIG_MODEM_MODULES_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <string.h>

struct rx_buf_t {
	atomic_t ref_counter;
	uint8_t buf[];
};

static inline struct rx_buf_t *block_start_get(struct modem_backend_uart_slm *backend, uint8_t *buf)
{
	size_t block_num;

	/* Find the correct block. */
	block_num = (((size_t)buf - sizeof(struct rx_buf_t) - (size_t)backend->rx_slab.buffer) /
		     backend->rx_buf_size);

	return (struct rx_buf_t *)&backend->rx_slab.buffer[block_num * backend->rx_buf_size];
}

static struct rx_buf_t *rx_buf_alloc(struct modem_backend_uart_slm *backend)
{
	struct rx_buf_t *buf;

	if (k_mem_slab_alloc(&backend->rx_slab, (void **)&buf, K_NO_WAIT)) {
		return NULL;
	}
	atomic_set(&buf->ref_counter, 1);

	return buf;
}

static void rx_buf_ref(struct modem_backend_uart_slm *backend, void *buf)
{
	atomic_inc(&(block_start_get(backend, buf)->ref_counter));
}

static void rx_buf_unref(struct modem_backend_uart_slm *backend, void *buf)
{
	struct rx_buf_t *uart_buf = block_start_get(backend, buf);
	atomic_t ref_counter = atomic_dec(&uart_buf->ref_counter);

	if (ref_counter == 1) {
		k_mem_slab_free(&backend->rx_slab, (void *)uart_buf);
	}
}

enum {
	MODEM_BACKEND_UART_SLM_STATE_OPEN_BIT,
	MODEM_BACKEND_UART_SLM_STATE_TRANSMIT_BIT,
	MODEM_BACKEND_UART_SLM_STATE_RECOVERY_BIT,
};

static int modem_backend_uart_slm_rx_enable(struct modem_backend_uart_slm *backend)
{
	int ret;
	struct rx_buf_t *buf = rx_buf_alloc(backend);

	if (!buf) {
		return -ENOMEM;
	}

	ret = uart_rx_enable(backend->uart, buf->buf,
			     backend->rx_buf_size - sizeof(struct rx_buf_t),
			     CONFIG_MODEM_BACKEND_UART_SLM_RECEIVE_IDLE_TIMEOUT_MS * 1000);
	if (ret) {
		rx_buf_unref(backend, buf->buf);
		return ret;
	}

	return 0;
}

static void modem_backend_uart_slm_rx_recovery(struct modem_backend_uart_slm *backend)
{
	int err;

	if (!atomic_test_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_RECOVERY_BIT)) {
		return;
	}

	err = modem_backend_uart_slm_rx_enable(backend);
	if (err) {
		LOG_DBG("RX recovery failed: %d", err);
		return;
	}

	if (!atomic_test_and_clear_bit(&backend->state,
				       MODEM_BACKEND_UART_SLM_STATE_RECOVERY_BIT)) {
		/* Closed during recovery. */
		uart_rx_disable(backend->uart);
	} else {
		LOG_DBG("RX recovery success");
	}
}

static bool modem_backend_uart_slm_is_uart_stopped(const struct modem_backend_uart_slm *backend)
{
	if (!atomic_test_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_OPEN_BIT) &&
	    !atomic_test_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_RECOVERY_BIT) &&
	    !atomic_test_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_TRANSMIT_BIT)) {
		return true;
	}

	return false;
}

static bool modem_backend_uart_slm_is_open(const struct modem_backend_uart_slm *backend)
{
	return atomic_test_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_OPEN_BIT);
}

static void modem_backend_uart_slm_event_handler(const struct device *dev, struct uart_event *evt,
						 void *user_data)
{
	struct modem_backend_uart_slm *backend = (struct modem_backend_uart_slm *)user_data;
	struct slm_rx_queue_event rx_event;
	int err;

	switch (evt->type) {
	case UART_TX_DONE:
		ring_buf_get_finish(&backend->transmit_rb, evt->data.tx.len);
		atomic_clear_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_TRANSMIT_BIT);
		k_work_submit(&backend->transmit_idle_work);
		break;

	case UART_TX_ABORTED:
		ring_buf_get_finish(&backend->transmit_rb, evt->data.tx.len);
		if (!modem_backend_uart_slm_is_open(backend)) {
			/* When we are closing, send the remaining data after re-open. */
			atomic_clear_bit(&backend->state,
					 MODEM_BACKEND_UART_SLM_STATE_TRANSMIT_BIT);
			break;
		}
		if (evt->data.tx.len != 0) {
			/* If we were able to send some data, attempt to send the remaining
			 * data before releasing the transmit bit.
			 */
			uint8_t *buf;
			size_t bytes_to_transmit =
				ring_buf_get_claim(&backend->transmit_rb, &buf,
						   ring_buf_capacity_get(&backend->transmit_rb));

			err = uart_tx(backend->uart, buf, bytes_to_transmit,
				      CONFIG_MODEM_BACKEND_UART_SLM_TRANSMIT_TIMEOUT_MS * 1000L);
			if (err) {
				LOG_ERR("Failed to %s %u bytes. (%d)", "start async transmit for",
					bytes_to_transmit, err);
				atomic_clear_bit(&backend->state,
						 MODEM_BACKEND_UART_SLM_STATE_TRANSMIT_BIT);
			}
			break;
		}

		/* We were not able to send anything. Start dropping data. */
		LOG_ERR("Transmit aborted (%u bytes dropped)",
			ring_buf_size_get(&backend->transmit_rb));
		atomic_clear_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_TRANSMIT_BIT);
		k_work_submit(&backend->transmit_idle_work);
		break;

	case UART_RX_BUF_REQUEST:
		struct rx_buf_t *buf = rx_buf_alloc(backend);

		if (!buf) {
			LOG_DBG("No receive buffer, disabling RX");
			break;
		}
		err = uart_rx_buf_rsp(backend->uart, buf->buf,
				      backend->rx_buf_size - sizeof(struct rx_buf_t));
		if (err) {
			LOG_ERR("uart_rx_buf_rsp: %d", err);
			rx_buf_unref(backend, buf->buf);
		}
		break;

	case UART_RX_BUF_RELEASED:
		if (evt->data.rx_buf.buf) {
			rx_buf_unref(backend, evt->data.rx_buf.buf);
		}
		break;

	case UART_RX_RDY:
		if (evt->data.rx.buf) {
			rx_buf_ref(backend, evt->data.rx.buf);
			rx_event.buf = &evt->data.rx.buf[evt->data.rx.offset];
			rx_event.len = evt->data.rx.len;
			err = k_msgq_put(&backend->rx_queue, &rx_event, K_NO_WAIT);
			if (err) {
				LOG_WRN("RX queue overflow: %d (dropped %u)", err,
					evt->data.rx.len);
				rx_buf_unref(backend, evt->data.rx.buf);
				break;
			}
			k_work_schedule(&backend->receive_ready_work, K_NO_WAIT);
		}
		break;

	case UART_RX_DISABLED:
		if (atomic_test_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_OPEN_BIT)) {
			if (!atomic_test_and_set_bit(&backend->state,
						     MODEM_BACKEND_UART_SLM_STATE_RECOVERY_BIT)) {
				k_work_schedule(&backend->receive_ready_work, K_NO_WAIT);
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

	if (modem_backend_uart_slm_is_uart_stopped(backend)) {
		k_work_submit(&backend->rx_disabled_work);
	}
}

static int modem_backend_uart_slm_open(void *data)
{
	struct modem_backend_uart_slm *backend = (struct modem_backend_uart_slm *)data;
	struct rx_buf_t *rx_buf = rx_buf_alloc(backend);
	int ret;

	if (!rx_buf) {
		return -ENOMEM;
	}

	atomic_clear(&backend->state);
	atomic_set_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_TRANSMIT_BIT);
	atomic_set_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_OPEN_BIT);

	if (!ring_buf_is_empty(&backend->transmit_rb)) {
		/* Transmit was aborted due to modem_backend_uart_slm_close.
		 * Send the remaining data before allowing further transmits.
		 */
		uint8_t *tx_buf;
		const uint32_t tx_buf_size = ring_buf_get_claim(
			&backend->transmit_rb, &tx_buf, ring_buf_size_get(&backend->transmit_rb));

		ret = uart_tx(backend->uart, tx_buf, tx_buf_size,
			      CONFIG_MODEM_BACKEND_UART_SLM_TRANSMIT_TIMEOUT_MS * 1000L);
		if (ret) {
			LOG_ERR("Failed to %s %u bytes. (%d)", "start async transmit for",
				tx_buf_size, ret);
			atomic_clear_bit(&backend->state,
					 MODEM_BACKEND_UART_SLM_STATE_TRANSMIT_BIT);
		}
	} else {
		/* Previous transmit was not aborted. */
		atomic_clear_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_TRANSMIT_BIT);
	}

	ret = uart_rx_enable(backend->uart, rx_buf->buf,
			     backend->rx_buf_size - sizeof(struct rx_buf_t),
			     CONFIG_MODEM_BACKEND_UART_SLM_RECEIVE_IDLE_TIMEOUT_MS * 1000L);
	if (ret < 0) {
		rx_buf_unref(backend, rx_buf->buf);
		atomic_clear_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_OPEN_BIT);
		return ret;
	}

	modem_pipe_notify_opened(&backend->pipe);
	return 0;
}

#ifdef CONFIG_MODEM_STATS
static uint32_t get_transmit_buf_size(const struct modem_backend_uart_slm *backend)
{
	return ring_buf_capacity_get(&backend->transmit_rb);
}

static uint32_t get_receive_buf_size(struct modem_backend_uart_slm *backend)
{
	return (backend->rx_buf_size - sizeof(struct rx_buf_t)) * backend->rx_buf_count;
}

static void advertise_transmit_buf_stats(struct modem_backend_uart_slm *backend, uint32_t length)
{
	modem_stats_buffer_advertise_length(&backend->transmit_buf_stats, length);
}

static void advertise_receive_buf_stats(struct modem_backend_uart_slm *backend, uint32_t reserved)
{
	modem_stats_buffer_advertise_length(&backend->receive_buf_stats, reserved);
}
#endif

static int modem_backend_uart_slm_transmit(void *data, const uint8_t *buf, size_t size)
{
	struct modem_backend_uart_slm *backend = (struct modem_backend_uart_slm *)data;
	bool transmitting;
	uint32_t bytes_to_transmit;
	int ret;
	uint8_t *tx_buf;

	if (!modem_backend_uart_slm_is_open(backend)) {
		return -EPERM;
	}

	transmitting =
		atomic_test_and_set_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_TRANSMIT_BIT);
	if (transmitting) {
		return 0;
	}

	/* Copy buf to transmit ring buffer which is passed to UART. */
	ring_buf_reset(&backend->transmit_rb);
	ring_buf_put(&backend->transmit_rb, buf, size);
	bytes_to_transmit = ring_buf_get_claim(&backend->transmit_rb, &tx_buf, size);

	ret = uart_tx(backend->uart, tx_buf, bytes_to_transmit,
		      CONFIG_MODEM_BACKEND_UART_SLM_TRANSMIT_TIMEOUT_MS * 1000L);

#ifdef CONFIG_MODEM_STATS
	advertise_transmit_buf_stats(backend, bytes_to_transmit);
#endif

	if (ret != 0) {
		LOG_ERR("Failed to %s %u bytes. (%d)", "start async transmit for",
			bytes_to_transmit, ret);
		atomic_clear_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_TRANSMIT_BIT);

		return ret;
	}

	return (int)bytes_to_transmit;
}

static int modem_backend_uart_slm_receive(void *data, uint8_t *buf, size_t size)
{
	struct modem_backend_uart_slm *backend = (struct modem_backend_uart_slm *)data;
	size_t received = 0;
	size_t copy_size = 0;

#ifdef CONFIG_MODEM_STATS
	struct slm_rx_queue_event rx_event;
	size_t reserved = backend->rx_event.len;

	for (int i = 0; i < k_msgq_num_used_get(&backend->rx_queue); i++) {
		if (k_msgq_peek_at(&backend->rx_queue, &rx_event, i)) {
			break;
		}
		reserved += rx_event.len;
	}
	advertise_receive_buf_stats(backend, reserved);
#endif
	while (size > received) {
		/* Keeping track of the rx_event allows us to receive less than what the event
		 * indicates.
		 */
		if (backend->rx_event.len == 0) {
			if (k_msgq_get(&backend->rx_queue, &backend->rx_event, K_NO_WAIT)) {
				break;
			}
		}
		copy_size = MIN(size - received, backend->rx_event.len);
		memcpy(buf, backend->rx_event.buf, copy_size);
		buf += copy_size;
		received += copy_size;
		backend->rx_event.buf += copy_size;
		backend->rx_event.len -= copy_size;

		if (backend->rx_event.len == 0) {
			rx_buf_unref(backend, backend->rx_event.buf);
		}
	}

	if (backend->rx_event.len != 0 || k_msgq_num_used_get(&backend->rx_queue) != 0) {
		k_work_schedule(&backend->receive_ready_work, K_NO_WAIT);
	}

	modem_backend_uart_slm_rx_recovery(backend);

	return (int)received;
}

static int modem_backend_uart_slm_close(void *data)
{
	struct modem_backend_uart_slm *backend = (struct modem_backend_uart_slm *)data;

	atomic_clear_bit(&backend->state, MODEM_BACKEND_UART_SLM_STATE_OPEN_BIT);
	uart_tx_abort(backend->uart);

	if (!atomic_test_and_clear_bit(&backend->state,
				       MODEM_BACKEND_UART_SLM_STATE_RECOVERY_BIT)) {
		/* Disable the RX, if recovery is not ongoing. */
		uart_rx_disable(backend->uart);
	}

	return 0;
}

static void modem_backend_uart_slm_receive_ready_handler(struct k_work *item)
{
	struct modem_backend_uart_slm *backend =
		CONTAINER_OF(k_work_delayable_from_work(item), struct modem_backend_uart_slm,
			     receive_ready_work);

	modem_pipe_notify_receive_ready(&backend->pipe);
}

static void modem_backend_uart_slm_transmit_idle_handler(struct k_work *item)
{
	struct modem_backend_uart_slm *backend =
		CONTAINER_OF(item, struct modem_backend_uart_slm, transmit_idle_work);

	modem_pipe_notify_transmit_idle(&backend->pipe);
}

static void modem_backend_uart_slm_notify_closed(struct k_work *item)
{

	struct modem_backend_uart_slm *backend =
		CONTAINER_OF(item, struct modem_backend_uart_slm, rx_disabled_work);

	modem_pipe_notify_closed(&backend->pipe);
}

#ifdef CONFIG_MODEM_STATS
static void init_stats(struct modem_backend_uart_slm *backend)
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

static const struct modem_pipe_api modem_backend_uart_slm_api = {
	.open = modem_backend_uart_slm_open,
	.transmit = modem_backend_uart_slm_transmit,
	.receive = modem_backend_uart_slm_receive,
	.close = modem_backend_uart_slm_close,
};

struct modem_pipe *modem_backend_uart_slm_init(struct modem_backend_uart_slm *backend,
					       const struct modem_backend_uart_slm_config *config)
{
	int err;

	__ASSERT_NO_MSG(config->uart != NULL);
	__ASSERT_NO_MSG(config->receive_buf != NULL);
	__ASSERT_NO_MSG(config->receive_buf_size > 1);
	__ASSERT_NO_MSG((config->receive_buf_size % 2) == 0);
	__ASSERT_NO_MSG(config->transmit_buf != NULL);
	__ASSERT_NO_MSG(config->transmit_buf_size > 0);

	memset(backend, 0x00, sizeof(*backend));
	backend->uart = config->uart;
	k_work_init_delayable(&backend->receive_ready_work,
			      modem_backend_uart_slm_receive_ready_handler);
	k_work_init(&backend->transmit_idle_work, modem_backend_uart_slm_transmit_idle_handler);
	k_work_init(&backend->rx_disabled_work, modem_backend_uart_slm_notify_closed);

	err = uart_callback_set(backend->uart, modem_backend_uart_slm_event_handler, backend);
	if (err) {
		LOG_ERR("uart_callback_set failed. (%d)", err);
		return NULL;
	}

	int32_t buf_size = (int32_t)config->receive_buf_size;

	backend->rx_buf_count = CONFIG_MODEM_BACKEND_UART_SLM_BUFFER_COUNT;

	/* k_mem_slab_init requires a word-aligned buffer. */
	__ASSERT((uintptr_t)config->receive_buf % sizeof(void *) == 0,
		 "Receive buffer is not word-aligned");

	/* Make sure all the buffers will be aligned. */
	buf_size -= (config->receive_buf_size % (sizeof(uint32_t) * backend->rx_buf_count));
	backend->rx_buf_size = buf_size / backend->rx_buf_count;
	__ASSERT_NO_MSG(backend->rx_buf_size > sizeof(struct rx_buf_t));

	/* Initialize the RX buffers and event queue. */
	err = k_mem_slab_init(&backend->rx_slab, config->receive_buf, backend->rx_buf_size,
			      backend->rx_buf_count);
	if (err) {
		LOG_ERR("k_mem_slab_init failed. (%d)", err);
		return NULL;
	}
	k_msgq_init(&backend->rx_queue, (char *)backend->rx_queue_buf,
		    sizeof(struct slm_rx_queue_event), CONFIG_MODEM_BACKEND_UART_SLM_BUFFER_COUNT);

	ring_buf_init(&backend->transmit_rb, config->transmit_buf_size, config->transmit_buf);

	modem_pipe_init(&backend->pipe, backend, &modem_backend_uart_slm_api);

#ifdef CONFIG_MODEM_STATS
	init_stats(backend);
#endif
	return &backend->pipe;
}
