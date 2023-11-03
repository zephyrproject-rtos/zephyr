/*
 * Copyright (c) 2023 Fabian Blatz
 * Copyright (c) 2023 Trackunit Corporation
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
#include <zephyr/sys/atomic.h>

LOG_MODULE_REGISTER(uart_emul, CONFIG_UART_LOG_LEVEL);

#define UART_EMUL_ASYNC_ENABLED_BIT     0
#define UART_EMUL_CALLBACK_SET_BUSY_BIT 1
#define UART_EMUL_TX_BUSY_BIT           2
#define UART_EMUL_TX_ABORT_BUSY_BIT     3
#define UART_EMUL_RX_ENABLE_BUSY_BIT    4
#define UART_EMUL_RX_BUF_RSP_BUSY_BIT   5
#define UART_EMUL_RX_DISABLE_BUSY_BIT   6

struct uart_emul_config {
	bool loopback;
	size_t latch_buffer_size;
};

struct uart_emul_work {
	struct k_work work;
	const struct device *dev;
};

/* Device run time data */
struct uart_emul_data {
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
	struct uart_emul_work irq_work;
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_udata;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	const struct device *dev;
	atomic_t async_state;

	/* Set callback to NULL to disable async UART */
	struct k_work async_callback_set_work;
	uart_callback_t async_callback_set_callback;
	void *async_callback_set_user_data;

	/*
	 * Async transmit data passed with work item. The work item is
	 * delayed to emulate it taking time to perform the transmit.
	 */
	struct k_work_delayable async_tx_dwork;
	const uint8_t *async_tx_buf;
	size_t async_tx_len;

	/* Async transmit abort work item carries no data */
	struct k_work async_tx_abort_work;

	/* Async receive enable work */
	struct k_work async_rx_enable_work;
	uint8_t *async_rx_enable_buf;
	size_t async_rx_enable_len;

	/* Async receive buf response work */
	struct k_work async_rx_buf_rsp_work;
	uint8_t *async_rx_buf_rsp_buf;
	size_t async_rx_buf_rsp_len;

	/*
	 * The async work item is delayed to emulate time passed while
	 * waiting for more data.
	 */
	struct k_work_delayable async_rx_dwork;

	/* */
	struct k_work async_rx_disable_work;

	uart_callback_t async_callback;
	void *async_user_data;

	/*
	 * UART drivers reserve two linear RX buffers. One is actively used, the other
	 * is swapped in once the active RX buffer is full. If swapping is not possible,
	 * the driver will stop receiving data.
	 */
	uint8_t *async_active_rx_buffer;
	size_t async_active_rx_buffer_size;
	uint8_t *async_reserve_rx_buffer;
	size_t async_reserve_rx_buffer_size;
	size_t async_active_rx_buffer_offset;
	size_t async_active_rx_buffer_length;
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

#ifdef CONFIG_UART_ASYNC_API
static void uart_emul_async_callback_set_internal(struct uart_emul_data *data,
						  uart_callback_t callback,
						  void *user_data)
{
	struct k_work_sync sync;

	__ASSERT(!atomic_test_and_set_bit(&data->async_state, UART_EMUL_CALLBACK_SET_BUSY_BIT),
		 "Async callback set already in progress");

	data->async_callback_set_callback = callback;
	data->async_callback_set_user_data = user_data;
	k_work_submit_to_queue(&uart_emul_work_q, &data->async_callback_set_work);
	k_work_flush(&data->async_callback_set_work, &sync);
}
#endif /* CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_emul_irq_callback_set_internal(struct uart_emul_data *data,
						uart_irq_callback_user_data_t cb,
						void *user_data)
{
	data->irq_cb = cb;
	data->irq_cb_udata = user_data;

	if (data->irq_cb == NULL) {
		data->rx_irq_en = false;
		data->tx_irq_en = false;
	}
}
#endif /* #ifdef CONFIG_UART_INTERRUPT_DRIVEN */

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
	struct uart_emul_work *uwork = CONTAINER_OF(work, struct uart_emul_work, work);
	const struct device *dev = uwork->dev;
	struct uart_emul_data *data = dev->data;
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
		(void)k_work_submit_to_queue(&uart_emul_work_q, &data->irq_work.work);
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
		(void)k_work_submit_to_queue(&uart_emul_work_q, &data->irq_work.work);
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
	struct uart_emul_data *data = dev->data;

	uart_emul_irq_callback_set_internal(data, cb, user_data);

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	uart_emul_async_callback_set_internal(data, NULL, NULL);
#endif /* CONFIG_UART_EXCLUSIVE_API_CALLBACKS */
}

static int uart_emul_irq_update(const struct device *dev)
{
	return 1;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
static void uart_emul_async_callback_set_handler(struct k_work *work)
{
	struct uart_emul_data *data =
		CONTAINER_OF(work, struct uart_emul_data, async_callback_set_work);

	if (data->async_callback_set_callback == NULL) {
		atomic_clear_bit(&data->async_state, UART_EMUL_ASYNC_ENABLED_BIT);
	}

	data->async_callback = data->async_callback_set_callback;
	data->async_user_data = data->async_callback_set_user_data;
	atomic_clear_bit(&data->async_state, UART_EMUL_CALLBACK_SET_BUSY_BIT);

	if (data->async_callback) {
		atomic_set_bit(&data->async_state, UART_EMUL_ASYNC_ENABLED_BIT);
	}
}

static void uart_emul_async_tx_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_emul_data *data = CONTAINER_OF(dwork, struct uart_emul_data, async_tx_dwork);
	uint32_t written;
	struct uart_event evt = {
		.type = UART_TX_DONE,
		.data.tx.buf = data->async_tx_buf,
		.data.tx.len = data->async_tx_len,
	};

	K_SPINLOCK(&data->tx_lock) {
		written = ring_buf_put(data->tx_rb, data->async_tx_buf, data->async_tx_len);
	}

	__ASSERT(written == data->async_tx_len, "Async TX overrun");
	LOG_DBG("Async TX from 0x%zx of %zu bytes done", (size_t)data->async_tx_buf,
		data->async_tx_len);
	atomic_clear_bit(&data->async_state, UART_EMUL_TX_BUSY_BIT);
	__ASSERT(data->async_callback != NULL, "Async callback must be set");
	data->async_callback(data->dev, &evt, data->async_user_data);
}

static void uart_emul_async_tx_abort_handler(struct k_work *work)
{
	struct uart_emul_data *data =
		CONTAINER_OF(work, struct uart_emul_data, async_tx_abort_work);

	struct uart_event evt = {
		.type = UART_TX_ABORTED,
	};

	k_work_cancel_delayable(&data->async_tx_dwork);
	atomic_clear_bit(&data->async_state, UART_EMUL_TX_ABORT_BUSY_BIT);
	__ASSERT(data->async_callback != NULL, "Async callback must be set");
	LOG_DBG("Async TX aborted");
	data->async_callback(data->dev, &evt, data->async_user_data);
}

static void uart_emul_async_rx_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_emul_data *data = CONTAINER_OF(dwork, struct uart_emul_data, async_rx_dwork);
	struct uart_event evt;
	uint32_t space;
	uint32_t written;

	__ASSERT(data->async_callback != NULL, "Async callback must be set");
	__ASSERT(data->async_active_rx_buffer != NULL, "Async RX is disabled");

	/* Put data into active RX buffer */
	space = data->async_active_rx_buffer_size - data->async_active_rx_buffer_offset;
	K_SPINLOCK(&data->rx_lock) {
		written = ring_buf_get(data->rx_rb, data->async_active_rx_buffer, space);
	}

	/* Nothing do do if no data was received */
	if (written == 0) {
		return;
	}

	LOG_DBG("Async RX putting %u bytes into 0x%zx at offset %zu", written,
		(size_t)data->async_active_rx_buffer,
		data->async_active_rx_buffer_offset);
	evt.type = UART_RX_RDY;
	evt.data.rx.buf = data->async_active_rx_buffer;
	evt.data.rx.offset = data->async_active_rx_buffer_offset;
	evt.data.rx.len = written;
	data->async_callback(data->dev, &evt, data->async_user_data);

	data->async_active_rx_buffer_offset += written;

	if (written < space) {
		return;
	}

	LOG_DBG("Releasing async RX buffer 0x%zx", (size_t)data->async_active_rx_buffer);
	evt.type = UART_RX_BUF_RELEASED;
	evt.data.rx_buf.buf = data->async_active_rx_buffer;
	data->async_callback(data->dev, &evt, data->async_user_data);
	data->async_active_rx_buffer = NULL;

	if (!data->async_reserve_rx_buffer) {
		LOG_DBG("Disabling async RX due to missing reserve RX buffer");
		evt.type = UART_RX_DISABLED;
		data->async_callback(data->dev, &evt, data->async_user_data);
		return;
	}

	/* Switch to reserved RX buffer */
	LOG_DBG("Async RX switched to reserved RX buffer 0x%zx",
		(size_t)data->async_reserve_rx_buffer);
	data->async_active_rx_buffer = data->async_reserve_rx_buffer;
	data->async_active_rx_buffer_size = data->async_reserve_rx_buffer_size;
	data->async_reserve_rx_buffer_size = 0;
	data->async_active_rx_buffer_offset = 0;
	data->async_active_rx_buffer_length = 0;
	data->async_reserve_rx_buffer = NULL;

	/* Request new reserve RX buffer */
	evt.type = UART_RX_BUF_REQUEST;
	data->async_callback(data->dev, &evt, data->async_user_data);

	/* Reschedule async RX work if there is still data to receive */
	K_SPINLOCK(&data->rx_lock) {
		if (!ring_buf_is_empty(data->rx_rb)) {
			k_work_schedule_for_queue(&uart_emul_work_q,
						  &data->async_rx_dwork,
						  K_MSEC(10));
		}
	}
}

static void uart_emul_async_rx_enable_handler(struct k_work *work)
{
	struct uart_emul_data *data =
		CONTAINER_OF(work, struct uart_emul_data, async_rx_enable_work);

	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	__ASSERT(data->async_callback != NULL, "Async callback must be set");
	__ASSERT(data->async_reserve_rx_buffer == NULL, "Async RX state invalid");

	LOG_DBG("Requesting reserve async RX buffer");
	data->async_callback(data->dev, &evt, data->async_user_data);

	data->async_active_rx_buffer = data->async_rx_enable_buf;
	data->async_active_rx_buffer_size = data->async_rx_enable_len;
	data->async_active_rx_buffer_offset = 0;
	data->async_active_rx_buffer_length = 0;
	atomic_clear_bit(&data->async_state, UART_EMUL_RX_ENABLE_BUSY_BIT);
	LOG_DBG("Async RX started with initial buffer 0x%zx of size %zu",
		(size_t)data->async_active_rx_buffer, data->async_active_rx_buffer_size);
}

static void uart_emul_async_rx_buf_rsp_handler(struct k_work *work)
{
	struct uart_emul_data *data =
		CONTAINER_OF(work, struct uart_emul_data, async_rx_buf_rsp_work);

	__ASSERT(data->async_reserve_rx_buffer == NULL,
		 "Provided unsolicited RX reserve buffer");

	data->async_reserve_rx_buffer = data->async_rx_buf_rsp_buf;
	data->async_reserve_rx_buffer_size = data->async_rx_buf_rsp_len;
	LOG_DBG("Received async RX reserve buffer 0x%zx of size %zu",
		(size_t)data->async_reserve_rx_buffer, data->async_active_rx_buffer_size);
	atomic_clear_bit(&data->async_state, UART_EMUL_RX_BUF_RSP_BUSY_BIT);
}

static void uart_emul_async_rx_disable_handler(struct k_work *work)
{
	struct uart_emul_data *data =
		CONTAINER_OF(work, struct uart_emul_data, async_rx_disable_work);
	struct uart_event evt;

	if (data->async_active_rx_buffer) {
		LOG_DBG("Releasing active async RX buffer 0x%zx",
			(size_t)data->async_active_rx_buffer);
		evt.type = UART_RX_BUF_RELEASED;
		evt.data.rx_buf.buf = data->async_active_rx_buffer;
		data->async_callback(data->dev, &evt, data->async_user_data);
		data->async_active_rx_buffer = NULL;
	}

	if (data->async_reserve_rx_buffer) {
		LOG_DBG("Releasing reserve async RX buffer 0x%zx",
			(size_t)data->async_reserve_rx_buffer);
		evt.type = UART_RX_BUF_RELEASED;
		evt.data.rx_buf.buf = data->async_reserve_rx_buffer;
		data->async_callback(data->dev, &evt, data->async_user_data);
		data->async_reserve_rx_buffer = NULL;
	}

	LOG_DBG("Async RX disabled");
	k_work_cancel_delayable(&data->async_rx_dwork);
	evt.type = UART_RX_DISABLED;
	data->async_callback(data->dev, &evt, data->async_user_data);
}

static int uart_emul_async_callback_set(const struct device *dev, uart_callback_t callback,
					void *user_data)
{
	struct uart_emul_data *data = dev->data;

	uart_emul_async_callback_set_internal(data, callback, user_data);

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	uart_emul_irq_callback_set_internal(data, NULL, NULL);
#endif /* CONFIG_UART_EXCLUSIVE_API_CALLBACKS */
	return 0;
}

static int uart_emul_tx(const struct device *dev, const uint8_t *buf, size_t len,
			int32_t timeout)
{
	struct uart_emul_data *data = dev->data;

	__ASSERT(buf != NULL, "Buffer must be provided");
	__ASSERT(len > 0, "Buffer length must be more than 0");
	__ASSERT(timeout > -2, "Timeout must be postive or -1");
	__ASSERT(!atomic_test_and_set_bit(&data->async_state, UART_EMUL_TX_BUSY_BIT),
		 "Async TX already in progress");

	data->async_tx_buf = buf;
	data->async_tx_len = len;
	LOG_DBG("Starting async transfer from 0x%zx of %zu bytes", (size_t)buf, len);
	k_work_schedule_for_queue(&uart_emul_work_q, &data->async_tx_dwork, K_MSEC(10));
	return 0;
}

static int uart_emul_tx_abort(const struct device *dev)
{
	struct uart_emul_data *data = dev->data;

	__ASSERT(!atomic_test_and_set_bit(&data->async_state, UART_EMUL_TX_ABORT_BUSY_BIT),
		 "Async TX abort already in progress");

	LOG_DBG("Requesting Async TX abort");
	k_work_submit_to_queue(&uart_emul_work_q, &data->async_tx_abort_work);
	return 0;
}

static int uart_emul_rx_enable(const struct device *dev, uint8_t *buf, size_t len,
			       int32_t timeout)
{
	struct uart_emul_data *data = dev->data;

	__ASSERT(buf != NULL, "Buffer must be provided");
	__ASSERT(len > 0, "Buffer must not be of size 0");
	__ASSERT(timeout > -2, "Timeout can only be positive and -1");
	__ASSERT(!atomic_test_and_set_bit(&data->async_state, UART_EMUL_RX_ENABLE_BUSY_BIT),
		 "Async RX enable already in progress");

	data->async_rx_enable_buf = buf;
	data->async_rx_enable_len = len;
	LOG_DBG("Submitting async RX enable request with initial buffer 0x%zx of size %zu",
		(size_t)buf, len);
	k_work_submit_to_queue(&uart_emul_work_q, &data->async_rx_enable_work);
	return 0;
}

static int uart_emul_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_emul_data *data = dev->data;

	__ASSERT(buf != NULL, "Buffer must be provided");
	__ASSERT(len > 0, "Buffer must not be of size 0");
	__ASSERT(!atomic_test_and_set_bit(&data->async_state, UART_EMUL_RX_BUF_RSP_BUSY_BIT),
		 "Async RX buf response already in progress");

	data->async_rx_buf_rsp_buf = buf;
	data->async_rx_buf_rsp_len = len;
	LOG_DBG("Submitting async RX buf response with buffer 0x%zx of size %zu",
		(size_t)buf, len);
	k_work_submit_to_queue(&uart_emul_work_q, &data->async_rx_buf_rsp_work);
	return 0;
}

static int uart_emul_rx_disable(const struct device *dev)
{
	struct uart_emul_data *data = dev->data;

	__ASSERT(!atomic_test_and_set_bit(&data->async_state, UART_EMUL_RX_DISABLE_BUSY_BIT),
		 "Async RX disalbe already in progress");

	LOG_DBG("Submitting async RX disable request");
	k_work_submit_to_queue(&uart_emul_work_q, &data->async_rx_disable_work);
	return 0;
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
	.callback_set = uart_emul_async_callback_set,
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

uint32_t uart_emul_put_rx_data(const struct device *dev, uint8_t *data, size_t size)
{
	struct uart_emul_data *drv_data = dev->data;

	__ASSERT(data != NULL, "Data must be provided");
	__ASSERT(size > 0, "Size must be more than 0");

	K_SPINLOCK(&drv_data->rx_lock) {
		__ASSERT(ring_buf_put(drv_data->rx_rb, data, size) == size,
				      "emula RX buffer overrun");
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (drv_data->rx_irq_en) {
		k_work_submit_to_queue(&uart_emul_work_q, &drv_data->irq_work.work);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	if (atomic_test_bit(&drv_data->async_state, UART_EMUL_ASYNC_ENABLED_BIT)) {
		k_work_schedule_for_queue(&uart_emul_work_q, &drv_data->async_rx_dwork,
					  K_MSEC(10));
	}
#endif /* CONFIG_UART_ASYNC_API */
	return size;
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

#define UART_EMUL_RX_FIFO_SIZE(inst) (DT_INST_PROP(inst, rx_fifo_size))
#define UART_EMUL_TX_FIFO_SIZE(inst) (DT_INST_PROP(inst, tx_fifo_size))

#define UART_EMUL_IRQ_WORK_INIT(inst)                                                              \
	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                                                   \
		   (.irq_work = {.dev = DEVICE_DT_INST_GET(inst),                                  \
				 .work = Z_WORK_INITIALIZER(uart_emul_irq_handler)},))

#define UART_EMUL_ASYNC_INITIALIZER(inst)                                                          \
	IF_ENABLED(                                                                                \
		CONFIG_UART_ASYNC_API,                                                             \
		(                                                                                  \
			.dev = DEVICE_DT_INST_GET(inst),                                           \
			.async_state = ATOMIC_INIT(0),                                             \
			.async_callback_set_work =                                                 \
				Z_WORK_INITIALIZER(uart_emul_async_callback_set_handler),          \
			.async_tx_dwork = Z_WORK_DELAYABLE_INITIALIZER(uart_emul_async_tx_handler),\
			.async_tx_abort_work =                                                     \
				Z_WORK_INITIALIZER(uart_emul_async_tx_abort_handler),              \
			.async_rx_dwork = Z_WORK_DELAYABLE_INITIALIZER(uart_emul_async_rx_handler),\
			.async_rx_enable_work =                                                    \
				Z_WORK_INITIALIZER(uart_emul_async_rx_enable_handler),             \
			.async_rx_buf_rsp_work =                                                   \
				Z_WORK_INITIALIZER(uart_emul_async_rx_buf_rsp_handler),            \
			.async_rx_disable_work =                                                   \
				Z_WORK_INITIALIZER(uart_emul_async_rx_disable_handler),            \
		)                                                                                  \
	)

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
		.rx_rb = &uart_emul_##inst##_rx_rb,                                                \
		.tx_rb = &uart_emul_##inst##_tx_rb,                                                \
		UART_EMUL_IRQ_WORK_INIT(inst)                                                      \
		UART_EMUL_ASYNC_INITIALIZER(inst)                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &uart_emul_data_##inst, &uart_emul_cfg_##inst,     \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_emul_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_UART_EMUL)
