/*
 * Copyright (c) 2023 Fabian Blatz
 * Copyright (c) 2024 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_uart_emul

#include <errno.h>

#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(uart_emul, CONFIG_UART_LOG_LEVEL);

struct uart_emul_config {
	bool loopback;
	size_t latch_buffer_size;
};

/* Device run time data */
struct uart_emul_data {
	const struct device *dev;

	struct uart_config cfg;
	int errors;

	struct ring_buf *rx_rb;
	struct k_spinlock rx_lock;

	uart_emul_callback_tx_data_ready_t tx_data_ready_cb;
	void *user_data;

	struct ring_buf *tx_rb;
	struct k_spinlock tx_lock;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	bool rx_irq_en;
	bool tx_irq_en;
	struct k_work irq_work;

	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_udata;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	bool rx_async_en;
	bool rx_stopping;
	bool rx_release_on_timeout;

	struct k_work tx_work;
	struct k_work rx_work;
	struct k_work rx_disable_work;
	struct k_work_delayable rx_timeout_work;

	uart_callback_t uart_callback;
	void *callback_user_data;

	const uint8_t *tx_buf;
	size_t tx_buf_len;
	size_t tx_buf_offset;

	uint8_t *rx_buf;
	size_t rx_buf_len;
	size_t rx_buf_offset;
	size_t rx_buf_data_len;
	int32_t rx_buf_timeout;

	uint8_t *rx_buf_next;
	size_t rx_buf_next_len;
#endif /* CONFIG_UART_ASYNC_API */
};

/*
 * Define local thread to emulate different thread priorities.
 *
 * A UART driver may call back from within a thread with higher or lower priority
 * than the thread calling the UART API. This can hide potential concurrency issues,
 * especially if the thread priorities are the same, or even using the same thread
 * in case the system work queue.
 */
K_THREAD_STACK_DEFINE(uart_emul_stack_area, CONFIG_UART_EMUL_WORK_Q_STACK_SIZE);
struct k_work_q uart_emul_work_q;

int uart_emul_init_work_q(void)
{
	k_work_queue_init(&uart_emul_work_q);
	k_work_queue_start(&uart_emul_work_q, uart_emul_stack_area,
			   K_THREAD_STACK_SIZEOF(uart_emul_stack_area),
			   CONFIG_UART_EMUL_WORK_Q_PRIORITY, NULL);
	return 0;
}

SYS_INIT(uart_emul_init_work_q, POST_KERNEL, 0);

static int uart_emul_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct uart_emul_data *drv_data = dev->data;
	k_spinlock_key_t key;
	uint32_t read;

	key = k_spin_lock(&drv_data->rx_lock);
	read = ring_buf_get(drv_data->rx_rb, p_char, 1);
	k_spin_unlock(&drv_data->rx_lock, key);

	if (!read) {
		LOG_DBG("Rx buffer is empty");
		return -1;
	}

	return 0;
}

static void uart_emul_poll_out(const struct device *dev, unsigned char out_char)
{
	struct uart_emul_data *drv_data = dev->data;
	const struct uart_emul_config *drv_cfg = dev->config;
	k_spinlock_key_t key;
	uint32_t written;

	key = k_spin_lock(&drv_data->tx_lock);
	written = ring_buf_put(drv_data->tx_rb, &out_char, 1);
	k_spin_unlock(&drv_data->tx_lock, key);

	if (!written) {
		LOG_DBG("Tx buffer is full");
		return;
	}

	if (drv_cfg->loopback) {
		uart_emul_put_rx_data(dev, &out_char, 1);
	}
	if (drv_data->tx_data_ready_cb) {
		(drv_data->tx_data_ready_cb)(dev, ring_buf_size_get(drv_data->tx_rb),
					     drv_data->user_data);
	}
}

static int uart_emul_err_check(const struct device *dev)
{
	struct uart_emul_data *drv_data = dev->data;
	int errors = drv_data->errors;

	drv_data->errors = 0;
	return errors;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_emul_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct uart_emul_data *drv_data = dev->data;

	memcpy(&drv_data->cfg, cfg, sizeof(struct uart_config));
	return 0;
}

static int uart_emul_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct uart_emul_data *drv_data = dev->data;

	memcpy(cfg, &drv_data->cfg, sizeof(struct uart_config));
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_emul_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	int ret;
	struct uart_emul_data *data = dev->data;
	const struct uart_emul_config *config = dev->config;
	uint32_t put_size = MIN(config->latch_buffer_size, size);

	K_SPINLOCK(&data->tx_lock) {
		ret = ring_buf_put(data->tx_rb, tx_data, put_size);
	}

	if (config->loopback) {
		uart_emul_put_rx_data(dev, (uint8_t *)tx_data, put_size);
	}
	if (data->tx_data_ready_cb) {
		data->tx_data_ready_cb(dev, ring_buf_size_get(data->tx_rb), data->user_data);
	}

	return ret;
}

static int uart_emul_fifo_read(const struct device *dev, uint8_t *rx_data, int size)
{
	struct uart_emul_data *data = dev->data;
	const struct uart_emul_config *config = dev->config;
	uint32_t bytes_to_read;

	K_SPINLOCK(&data->rx_lock) {
		bytes_to_read = MIN(config->latch_buffer_size, ring_buf_size_get(data->rx_rb));
		bytes_to_read = MIN(bytes_to_read, size);
		ring_buf_get(data->rx_rb, rx_data, bytes_to_read);
	}

	return bytes_to_read;
}

static int uart_emul_irq_tx_ready(const struct device *dev)
{
	bool ready = false;
	struct uart_emul_data *data = dev->data;

	K_SPINLOCK(&data->tx_lock) {
		if (!data->tx_irq_en) {
			K_SPINLOCK_BREAK;
		}

		ready = ring_buf_space_get(data->tx_rb) > 0;
	}

	return ready;
}

static int uart_emul_irq_rx_ready(const struct device *dev)
{
	bool ready = false;
	struct uart_emul_data *data = dev->data;

	K_SPINLOCK(&data->rx_lock) {
		if (!data->rx_irq_en) {
			K_SPINLOCK_BREAK;
		}

		ready = !ring_buf_is_empty(data->rx_rb);
	}

	return ready;
}

static void uart_emul_irq_handler(struct k_work *work)
{
	struct uart_emul_data *data = CONTAINER_OF(work, struct uart_emul_data, irq_work);
	const struct device *dev = data->dev;
	uart_irq_callback_user_data_t cb = data->irq_cb;
	void *udata = data->irq_cb_udata;

	if (cb == NULL) {
		LOG_DBG("No IRQ callback configured for uart_emul device %p", dev);
		return;
	}

	while (true) {
		bool have_work = false;

		K_SPINLOCK(&data->tx_lock) {
			if (!data->tx_irq_en) {
				K_SPINLOCK_BREAK;
			}

			have_work = have_work || ring_buf_space_get(data->tx_rb) > 0;
		}

		K_SPINLOCK(&data->rx_lock) {
			if (!data->rx_irq_en) {
				K_SPINLOCK_BREAK;
			}

			have_work = have_work || !ring_buf_is_empty(data->rx_rb);
		}

		if (!have_work) {
			break;
		}

		cb(dev, udata);
	}
}

static int uart_emul_irq_is_pending(const struct device *dev)
{
	return uart_emul_irq_tx_ready(dev) || uart_emul_irq_rx_ready(dev);
}

static void uart_emul_irq_tx_enable(const struct device *dev)
{
	bool submit_irq_work;
	struct uart_emul_data *const data = dev->data;

	K_SPINLOCK(&data->tx_lock) {
		data->tx_irq_en = true;
		submit_irq_work = ring_buf_space_get(data->tx_rb) > 0;
	}

	if (submit_irq_work) {
		(void)k_work_submit_to_queue(&uart_emul_work_q, &data->irq_work);
	}
}

static void uart_emul_irq_rx_enable(const struct device *dev)
{
	bool submit_irq_work;
	struct uart_emul_data *const data = dev->data;

	K_SPINLOCK(&data->rx_lock) {
		data->rx_irq_en = true;
		submit_irq_work = !ring_buf_is_empty(data->rx_rb);
	}

	if (submit_irq_work) {
		(void)k_work_submit_to_queue(&uart_emul_work_q, &data->irq_work);
	}
}

static void uart_emul_irq_tx_disable(const struct device *dev)
{
	struct uart_emul_data *const data = dev->data;

	K_SPINLOCK(&data->tx_lock) {
		data->tx_irq_en = false;
	}
}

static void uart_emul_irq_rx_disable(const struct device *dev)
{
	struct uart_emul_data *const data = dev->data;

	K_SPINLOCK(&data->rx_lock) {
		data->rx_irq_en = false;
	}
}

static int uart_emul_irq_tx_complete(const struct device *dev)
{
	bool tx_complete = false;
	struct uart_emul_data *const data = dev->data;

	K_SPINLOCK(&data->tx_lock) {
		tx_complete = ring_buf_is_empty(data->tx_rb);
	}

	return tx_complete;
}

static void uart_emul_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				       void *user_data)
{
	struct uart_emul_data *const data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_udata = user_data;
}

static int uart_emul_irq_update(const struct device *dev)
{
	return 1;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
static void uart_emul_post_event(const struct device *dev, struct uart_event *evt)
{
	struct uart_emul_data *data = dev->data;

	if (!data->uart_callback) {
		LOG_DBG("No async callback configured for uart_emul device %p", dev);
	}

	data->uart_callback(dev, evt, data->callback_user_data);
}

static void uart_emul_simple_event(const struct device *dev, enum uart_event_type type)
{
	uart_emul_post_event(dev, &(struct uart_event){.type = type});
}

static void uart_emul_async_switch_buf_nolock(struct uart_emul_data *data)
{
	data->rx_buf = data->rx_buf_next;
	data->rx_buf_len = data->rx_buf_next_len;
	data->rx_buf_offset = 0;
	data->rx_buf_data_len = 0;
	data->rx_buf_next = NULL;
	data->rx_buf_next_len = 0;
}

static void uart_emul_async_rx_timeout_handler(struct k_work *_work)
{
	struct k_work_delayable *work = k_work_delayable_from_work(_work);
	struct uart_emul_data *data = CONTAINER_OF(work, struct uart_emul_data, rx_timeout_work);
	const struct device *dev = data->dev;

	uint8_t *rx_buf;
	size_t rx_buf_len;
	size_t rx_buf_offset;
	size_t rx_buf_data_len;
	bool rx_en;
	bool rx_buf_released = false;
	bool rx_stopped = false;

	K_SPINLOCK(&data->rx_lock) {
		rx_en = data->rx_async_en;
		rx_buf = data->rx_buf;
		rx_buf_len = data->rx_buf_len;
		rx_buf_offset = data->rx_buf_offset;
		rx_buf_data_len = data->rx_buf_data_len;

		data->rx_buf_offset += rx_buf_data_len;
		data->rx_buf_data_len = 0;

		if (data->rx_buf_offset >= rx_buf_len ||
		    (rx_buf_data_len > 0 && data->rx_release_on_timeout)) {
			rx_buf_released = true;
			uart_emul_async_switch_buf_nolock(data);
			if (data->rx_buf == NULL) {
				/* There was no second buffer scheduled, so stop receiving */
				rx_stopped = true;
				data->rx_async_en = false;
			}
		}
	}

	if (!rx_en || rx_buf == NULL || rx_buf_data_len == 0) {
		return;
	}

	struct uart_event rx_rdy_event = {
		.type = UART_RX_RDY,
		.data.rx = {
			.buf = rx_buf,
			.offset = rx_buf_offset,
			.len = rx_buf_data_len,
		},
	};

	uart_emul_post_event(dev, &rx_rdy_event);

	if (rx_buf_released) {
		struct uart_event rx_buf_released_event = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = rx_buf,
		};

		uart_emul_post_event(dev, &rx_buf_released_event);
	}
	if (rx_stopped) {
		uart_emul_simple_event(dev, UART_RX_DISABLED);
	}
}

static void uart_emul_async_rx_handler(struct k_work *work)
{
	struct uart_emul_data *data = CONTAINER_OF(work, struct uart_emul_data, rx_work);
	const struct device *dev = data->dev;

	bool rx_en = false;
	bool empty = true;

	do {
		bool rx_rdy = false;
		bool buf_request = false;

		uint8_t *rx_buf = NULL;
		size_t buf_len;
		size_t offset;
		size_t data_len;

		K_SPINLOCK(&data->rx_lock) {
			rx_en = data->rx_async_en;
			rx_buf = data->rx_buf;
			buf_len = data->rx_buf_len;
			offset = data->rx_buf_offset;
			data_len = data->rx_buf_data_len;
			empty = ring_buf_is_empty(data->rx_rb);

			if (!rx_en) {
				K_SPINLOCK_BREAK;
			}

			if (rx_buf == NULL) {
				uart_emul_async_switch_buf_nolock(data);
				rx_buf = data->rx_buf;
				buf_len = data->rx_buf_len;
				offset = data->rx_buf_offset;
				data_len = data->rx_buf_data_len;
			}

			if (rx_buf == NULL) {
				/* During the last iteration the buffer was released but the
				 * application did not provide a new buffer. Stop RX and quit now.
				 */
				data->rx_async_en = false;
				K_SPINLOCK_BREAK;
			}

			if (empty) {
				K_SPINLOCK_BREAK;
			}

			buf_request = data_len == 0 && data->rx_buf_next == NULL;

			uint32_t read = ring_buf_get(data->rx_rb, &rx_buf[offset + data_len],
						     buf_len - (offset + data_len));
			data_len += read;
			data->rx_buf_data_len = data_len;

			if (offset + data_len >= data->rx_buf_len) {
				rx_rdy = true;
				data->rx_buf = NULL;
				data->rx_buf_len = 0;
				data->rx_buf_offset = 0;
				data->rx_buf_data_len = 0;
			}
		}

		if (!rx_en) {
			break;
		}

		if (rx_buf == NULL) {
			uart_emul_simple_event(dev, UART_RX_DISABLED);
			break;
		}

		if (empty && data->rx_buf_timeout != SYS_FOREVER_US) {
			(void)k_work_reschedule_for_queue(&uart_emul_work_q, &data->rx_timeout_work,
							  K_USEC(data->rx_buf_timeout));
		}

		if (buf_request) {
			uart_emul_simple_event(dev, UART_RX_BUF_REQUEST);
		}

		if (rx_rdy) {
			struct uart_event rx_rdy_event = {
				.type = UART_RX_RDY,
				.data.rx = {
					.buf = rx_buf,
					.offset = offset,
					.len = data_len,
				},
			};

			uart_emul_post_event(dev, &rx_rdy_event);

			struct uart_event rx_buf_released_event = {
				.type = UART_RX_BUF_RELEASED,
				.data.rx_buf.buf = rx_buf,
			};

			uart_emul_post_event(dev, &rx_buf_released_event);
		}
	} while (rx_en && !empty);
}

static void uart_emul_async_tx_handler(struct k_work *work)
{
	struct uart_emul_data *data = CONTAINER_OF(work, struct uart_emul_data, tx_work);
	const struct device *dev = data->dev;
	const struct uart_emul_config *config = dev->config;

	uint32_t written;

	const uint8_t *tx_buf = NULL;
	size_t tx_buf_len = 0;
	size_t tx_buf_offset = 0;
	bool tx_done = true;

	K_SPINLOCK(&data->tx_lock) {
		tx_buf = data->tx_buf;
		tx_buf_len = data->tx_buf_len;
		tx_buf_offset = data->tx_buf_offset;

		if (!tx_buf) {
			K_SPINLOCK_BREAK;
		}

		written = ring_buf_put(data->tx_rb, &data->tx_buf[tx_buf_offset],
				       tx_buf_len - tx_buf_offset);
		tx_done = written == (tx_buf_len - tx_buf_offset);
		if (!tx_done) {
			data->tx_buf_offset += written;
			K_SPINLOCK_BREAK;
		}
		data->tx_buf = NULL;
		data->tx_buf_len = 0;
		data->tx_buf_offset = 0;
	}

	if (!tx_buf) {
		return;
	}

	if (config->loopback && written) {
		uint32_t loop_written = uart_emul_put_rx_data(dev, &tx_buf[tx_buf_offset], written);

		if (loop_written < written) {
			LOG_WRN("Lost %" PRIu32 " bytes on loopback", written - loop_written);
		}
	}
	if (data->tx_data_ready_cb) {
		(data->tx_data_ready_cb)(dev, ring_buf_size_get(data->tx_rb), data->user_data);
	}

	if ((config->loopback && written) || !written) {
		/* When using the loopback fixture, just allow to drop all bytes in the ring buffer
		 * not consumed by tx_data_ready_cb().
		 */

		uint32_t flushed = uart_emul_flush_tx_data(dev);

		if (flushed) {
			if (written) {
				LOG_DBG("Flushed %" PRIu32 " unused bytes from tx buffer", flushed);
			} else {
				LOG_WRN("Flushed %" PRIu32
					" unused bytes from tx buffer to break out of infinite "
					"loop! Consume or flush the bytes from the tx ring buffer "
					"in your test case to prevent this!",
					flushed);
			}
		}
	}

	if (!tx_done) {
		/* We are not done yet, yield back into workqueue.
		 *
		 * This would basically be an infinite loop when tx_data_ready_cb() does not consume
		 * the bytes in the tx ring buffer.
		 */
		k_work_submit_to_queue(&uart_emul_work_q, &data->tx_work);
		return;
	}

	struct uart_event tx_done_event = {
		.type = UART_TX_DONE,
		.data.tx = {
			.buf = tx_buf,
			.len = tx_buf_len,
		},
	};

	uart_emul_post_event(dev, &tx_done_event);
}

static void uart_emul_rx_stop(const struct device *dev, struct uart_emul_data *data)
{
	uint8_t *rx_buf = NULL;
	size_t rx_buf_offset = 0;
	size_t rx_buf_data_len = 0;

	k_work_cancel_delayable(&data->rx_timeout_work);

	K_SPINLOCK(&data->rx_lock) {
		if (!data->rx_async_en) {
			K_SPINLOCK_BREAK;
		}
		rx_buf = data->rx_buf;
		rx_buf_offset = data->rx_buf_offset;
		rx_buf_data_len = data->rx_buf_data_len;

		data->rx_buf = NULL;
		data->rx_buf_len = 0;
		data->rx_buf_offset = 0;
		data->rx_buf_data_len = 0;
		data->rx_buf_next = NULL;
		data->rx_buf_next_len = 0;
		data->rx_async_en = false;
		data->rx_stopping = false;
	}

	if (rx_buf == NULL) {
		return;
	}

	if (rx_buf_data_len > 0) {
		struct uart_event rx_rdy_event = {
			.type = UART_RX_RDY,
			.data.rx = {
				.buf = rx_buf,
				.offset = rx_buf_offset,
				.len = rx_buf_data_len,
			},
		};

		uart_emul_post_event(dev, &rx_rdy_event);
	}

	struct uart_event rx_buf_released_event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = rx_buf,
	};

	uart_emul_post_event(dev, &rx_buf_released_event);
	uart_emul_simple_event(dev, UART_RX_DISABLED);
}

static void uart_emul_async_rx_disable_handler(struct k_work *work)
{
	struct uart_emul_data *data = CONTAINER_OF(work, struct uart_emul_data, rx_disable_work);
	const struct device *dev = data->dev;

	uart_emul_rx_stop(dev, data);
}

static int uart_emul_callback_set(const struct device *dev, uart_callback_t callback,
				  void *user_data)
{
	struct uart_emul_data *data = dev->data;

	data->uart_callback = callback;
	data->callback_user_data = user_data;

	return 0;
}

static int uart_emul_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	struct uart_emul_data *data = dev->data;
	int ret = 0;

	K_SPINLOCK(&data->tx_lock) {
		if (data->tx_buf) {
			ret = -EBUSY;
			K_SPINLOCK_BREAK;
		}

		data->tx_buf = buf;
		data->tx_buf_len = len;
		data->tx_buf_offset = 0;

		k_work_submit_to_queue(&uart_emul_work_q, &data->tx_work);
	}

	return ret;
}

static int uart_emul_tx_abort(const struct device *dev)
{
	struct uart_emul_data *data = dev->data;
	const uint8_t *tx_buf = NULL;
	size_t tx_buf_sent;

	K_SPINLOCK(&data->tx_lock) {
		tx_buf = data->tx_buf;
		tx_buf_sent = data->tx_buf_offset;

		data->tx_buf = NULL;
		data->tx_buf_len = 0;
		data->tx_buf_offset = 0;

		k_work_cancel(&data->tx_work);
	}

	if (!tx_buf) {
		return -EFAULT;
	}

	struct uart_event tx_aborted_event = {
		.type = UART_TX_ABORTED,
		.data.tx = {
			.buf = tx_buf,
			.len = tx_buf_sent,
		},
	};

	uart_emul_post_event(dev, &tx_aborted_event);

	return 0;
}

static int uart_emul_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_emul_data *data = dev->data;
	int ret = 0;

	K_SPINLOCK(&data->rx_lock) {
		if (!data->rx_async_en) {
			ret = -EACCES;
			K_SPINLOCK_BREAK;
		}

		if (data->rx_buf_next != NULL) {
			ret = -EBUSY;
			K_SPINLOCK_BREAK;
		}

		data->rx_buf_next = buf;
		data->rx_buf_next_len = len;
	}

	return ret;
}

static int uart_emul_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	struct uart_emul_data *data = dev->data;
	int ret = 0;
	bool rx_stopping;

	K_SPINLOCK(&data->rx_lock) {
		rx_stopping = data->rx_stopping;
		k_work_cancel(&data->rx_disable_work);
	}

	if (rx_stopping) {
		uart_emul_rx_stop(dev, data);
	}

	K_SPINLOCK(&data->rx_lock) {
		if (data->rx_async_en) {
			ret = -EBUSY;
			K_SPINLOCK_BREAK;
		}

		data->rx_async_en = true;
		data->rx_buf = buf;
		data->rx_buf_len = len;
		data->rx_buf_timeout = timeout;
		data->rx_buf_offset = 0;
		data->rx_buf_data_len = 0;
		data->rx_buf_next = NULL;
		data->rx_buf_next_len = 0;

		if (!ring_buf_is_empty(data->rx_rb)) {
			(void)k_work_submit_to_queue(&uart_emul_work_q, &data->rx_work);
		}
	}

	return ret;
}

static int uart_emul_rx_disable(const struct device *dev)
{
	struct uart_emul_data *data = dev->data;
	int ret = 0;

	K_SPINLOCK(&data->rx_lock) {
		if (!data->rx_async_en) {
			ret = -EFAULT;
			K_SPINLOCK_BREAK;
		}
		data->rx_stopping = true;
		k_work_submit_to_queue(&uart_emul_work_q, &data->rx_disable_work);
	}

	return ret;
}
#endif /* CONFIG_UART_ASYNC_API */

static const struct uart_driver_api uart_emul_api = {
	.poll_in = uart_emul_poll_in,
	.poll_out = uart_emul_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.config_get = uart_emul_config_get,
	.configure = uart_emul_configure,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
	.err_check = uart_emul_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_emul_fifo_fill,
	.fifo_read = uart_emul_fifo_read,
	.irq_tx_enable = uart_emul_irq_tx_enable,
	.irq_rx_enable = uart_emul_irq_rx_enable,
	.irq_tx_disable = uart_emul_irq_tx_disable,
	.irq_rx_disable = uart_emul_irq_rx_disable,
	.irq_tx_ready = uart_emul_irq_tx_ready,
	.irq_rx_ready = uart_emul_irq_rx_ready,
	.irq_tx_complete = uart_emul_irq_tx_complete,
	.irq_callback_set = uart_emul_irq_callback_set,
	.irq_update = uart_emul_irq_update,
	.irq_is_pending = uart_emul_irq_is_pending,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_emul_callback_set,
	.tx = uart_emul_tx,
	.tx_abort = uart_emul_tx_abort,
	.rx_enable = uart_emul_rx_enable,
	.rx_buf_rsp = uart_emul_rx_buf_rsp,
	.rx_disable = uart_emul_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
};

void uart_emul_callback_tx_data_ready_set(const struct device *dev,
					  uart_emul_callback_tx_data_ready_t cb, void *user_data)
{
	struct uart_emul_data *drv_data = dev->data;

	drv_data->tx_data_ready_cb = cb;
	drv_data->user_data = user_data;
}

uint32_t uart_emul_put_rx_data(const struct device *dev, const uint8_t *data, size_t size)
{
	struct uart_emul_data *drv_data = dev->data;
	uint32_t count;
	__unused bool empty;
	__unused bool irq_en;
	__unused bool rx_en;

	K_SPINLOCK(&drv_data->rx_lock) {
		count = ring_buf_put(drv_data->rx_rb, data, size);
		empty = ring_buf_is_empty(drv_data->rx_rb);
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (irq_en = drv_data->rx_irq_en;));
		IF_ENABLED(CONFIG_UART_ASYNC_API, (rx_en = drv_data->rx_async_en;));
	}

	if (count < size) {
		uart_emul_set_errors(dev, UART_ERROR_OVERRUN);
	}

	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (
		if (count > 0 && irq_en && !empty) {
			(void)k_work_submit_to_queue(&uart_emul_work_q, &drv_data->irq_work);
		}
	))
	IF_ENABLED(CONFIG_UART_ASYNC_API, (
		if (count > 0 && rx_en && !empty) {
			(void)k_work_submit_to_queue(&uart_emul_work_q, &drv_data->rx_work);
		}
	))

	return count;
}

uint32_t uart_emul_get_tx_data(const struct device *dev, uint8_t *data, size_t size)
{
	struct uart_emul_data *drv_data = dev->data;
	k_spinlock_key_t key;
	uint32_t count;

	key = k_spin_lock(&drv_data->tx_lock);
	count = ring_buf_get(drv_data->tx_rb, data, size);
	k_spin_unlock(&drv_data->tx_lock, key);
	return count;
}

uint32_t uart_emul_flush_rx_data(const struct device *dev)
{
	struct uart_emul_data *drv_data = dev->data;
	k_spinlock_key_t key;
	uint32_t count;

	key = k_spin_lock(&drv_data->rx_lock);
	count = ring_buf_size_get(drv_data->rx_rb);
	ring_buf_reset(drv_data->rx_rb);
	k_spin_unlock(&drv_data->rx_lock, key);
	return count;
}

uint32_t uart_emul_flush_tx_data(const struct device *dev)
{
	struct uart_emul_data *drv_data = dev->data;
	k_spinlock_key_t key;
	uint32_t count;

	key = k_spin_lock(&drv_data->tx_lock);
	count = ring_buf_size_get(drv_data->tx_rb);
	ring_buf_reset(drv_data->tx_rb);
	k_spin_unlock(&drv_data->tx_lock, key);
	return count;
}

void uart_emul_set_errors(const struct device *dev, int errors)
{
	struct uart_emul_data *drv_data = dev->data;

	drv_data->errors |= errors;
}

void uart_emul_set_release_buffer_on_timeout(const struct device *dev, bool release_on_timeout)
{
	__unused struct uart_emul_data *drv_data = dev->data;

	IF_ENABLED(CONFIG_UART_ASYNC_API, (drv_data->rx_release_on_timeout = release_on_timeout;));
}

#define UART_EMUL_RX_FIFO_SIZE(inst) (DT_INST_PROP(inst, rx_fifo_size))
#define UART_EMUL_TX_FIFO_SIZE(inst) (DT_INST_PROP(inst, tx_fifo_size))

#define DEFINE_UART_EMUL(inst)                                                                     \
                                                                                                   \
	RING_BUF_DECLARE(uart_emul_##inst##_rx_rb, UART_EMUL_RX_FIFO_SIZE(inst));                  \
	RING_BUF_DECLARE(uart_emul_##inst##_tx_rb, UART_EMUL_TX_FIFO_SIZE(inst));                  \
                                                                                                   \
	static struct uart_emul_config uart_emul_cfg_##inst = {                                    \
		.loopback = DT_INST_PROP(inst, loopback),                                          \
		.latch_buffer_size = DT_INST_PROP(inst, latch_buffer_size),                        \
	};                                                                                         \
	static struct uart_emul_data uart_emul_data_##inst = {                                     \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
		.rx_rb = &uart_emul_##inst##_rx_rb,                                                \
		.tx_rb = &uart_emul_##inst##_tx_rb,                                                \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                                           \
			   (.irq_work = Z_WORK_INITIALIZER(uart_emul_irq_handler),))               \
		IF_ENABLED(CONFIG_UART_ASYNC_API,                                                  \
				(.tx_work = Z_WORK_INITIALIZER(uart_emul_async_tx_handler),        \
				.rx_timeout_work = Z_WORK_DELAYABLE_INITIALIZER(                   \
					uart_emul_async_rx_timeout_handler),                       \
				.rx_work = Z_WORK_INITIALIZER(uart_emul_async_rx_handler),         \
				.rx_disable_work = Z_WORK_INITIALIZER(                             \
					uart_emul_async_rx_disable_handler),))                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &uart_emul_data_##inst, &uart_emul_cfg_##inst,     \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_emul_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_UART_EMUL)
