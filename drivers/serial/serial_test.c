/*
 * Copyright (c) 2021, Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real serial driver. It is used to instantiate struct
 * devices for the "vnd,serial" devicetree compatible used in test code.
 */

#include <stdbool.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/uart/serial_test.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>

LOG_MODULE_REGISTER(mock_serial, CONFIG_LOG_DEFAULT_LEVEL);

#define DT_DRV_COMPAT vnd_serial
struct serial_vnd_data {
#ifdef CONFIG_RING_BUFFER
	struct ring_buf *written;
	struct ring_buf *read_queue;
#endif
	serial_vnd_write_cb_t callback;
	void *callback_data;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_isr;
	bool irq_rx_enabled;
	bool irq_tx_enabled;
#endif
#ifdef CONFIG_UART_ASYNC_API
	uart_callback_t async_cb;
	void *async_cb_user_data;
	uint8_t *read_buf;
	size_t read_size;
	size_t read_position;
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static bool is_irq_rx_pending(const struct device *dev)
{
	struct serial_vnd_data *data = dev->data;

	return !ring_buf_is_empty(data->read_queue);
}

static bool is_irq_tx_pending(const struct device *dev)
{
	struct serial_vnd_data *data = dev->data;

	return ring_buf_space_get(data->written) != 0;
}

static void irq_process(const struct device *dev)
{
	struct serial_vnd_data *data = dev->data;

	for (;;) {
		bool rx_rdy = is_irq_rx_pending(dev);
		bool tx_rdy = is_irq_tx_pending(dev);
		bool rx_int = rx_rdy && data->irq_rx_enabled;
		bool tx_int = tx_rdy && data->irq_tx_enabled;

		LOG_DBG("rx_rdy %d tx_rdy %d", rx_rdy, tx_rdy);
		LOG_DBG("rx_int %d tx_int %d", rx_int, tx_int);

		if (!(rx_int || tx_int)) {
			break;
		}

		LOG_DBG("isr");
		if (!data->irq_isr) {
			LOG_ERR("no isr registered");
			break;
		}
		data->irq_isr(dev, NULL);
	};
}

static void irq_rx_enable(const struct device *dev)
{
	struct serial_vnd_data *data = dev->data;

	data->irq_rx_enabled = true;
	LOG_DBG("rx enabled");
	irq_process(dev);
}

static void irq_rx_disable(const struct device *dev)
{
	struct serial_vnd_data *data = dev->data;

	data->irq_rx_enabled = false;
	LOG_DBG("rx disabled");
}

static int irq_rx_ready(const struct device *dev)
{
	struct serial_vnd_data *data = dev->data;
	bool ready = !ring_buf_is_empty(data->read_queue);

	LOG_DBG("rx ready: %d", ready);
	return ready;
}

static void irq_tx_enable(const struct device *dev)
{
	struct serial_vnd_data *data = dev->data;

	LOG_DBG("tx enabled");
	data->irq_tx_enabled = true;
	irq_process(dev);
}

static void irq_tx_disable(const struct device *dev)
{
	struct serial_vnd_data *data = dev->data;

	data->irq_tx_enabled = false;
	LOG_DBG("tx disabled");
}

static int irq_tx_ready(const struct device *dev)
{
	struct serial_vnd_data *data = dev->data;
	bool ready = (ring_buf_space_get(data->written) != 0);

	LOG_DBG("tx ready: %d", ready);
	return ready;
}

static void irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
			     void *user_data)
{
	struct serial_vnd_data *data = dev->data;

	/* Not implemented. Ok because `user_data` is always NULL in the current
	 * implementation of core UART API.
	 */
	__ASSERT_NO_MSG(user_data == NULL);

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS) && defined(CONFIG_UART_ASYNC_API)
	if (data->read_buf) {
		LOG_ERR("Setting callback to NULL while asynchronous API is in use.");
	}
	data->async_cb = NULL;
	data->async_cb_user_data = NULL;
#endif

	data->irq_isr = cb;
	LOG_DBG("callback set");
}

static int fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	struct serial_vnd_data *data = dev->data;
	uint32_t write_len = ring_buf_put(data->written, tx_data, size);

	if (data->callback) {
		data->callback(dev, data->callback_data);
	}
	return write_len;
}

static int fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct serial_vnd_data *data = dev->data;
	int read_len = ring_buf_get(data->read_queue, rx_data, size);

	LOG_HEXDUMP_DBG(rx_data, read_len, "");
	return read_len;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int serial_vnd_poll_in(const struct device *dev, unsigned char *c)
{
#ifdef CONFIG_RING_BUFFER
	struct serial_vnd_data *data = dev->data;
	uint32_t bytes_read;

	if (data == NULL || data->read_queue == NULL) {
		return -ENOTSUP;
	}
	bytes_read = ring_buf_get(data->read_queue, c, 1);
	if (bytes_read == 1) {
		return 0;
	}
	return -1;
#else
	return -ENOTSUP;
#endif
}

static void serial_vnd_poll_out(const struct device *dev, unsigned char c)
{
	struct serial_vnd_data *data = dev->data;

#ifdef CONFIG_RING_BUFFER
	if (data == NULL || data->written == NULL) {
		return;
	}
	ring_buf_put(data->written, &c, 1);
#endif
	if (data->callback) {
		data->callback(dev, data->callback_data);
	}
}

#ifdef CONFIG_UART_ASYNC_API
static void async_rx_run(const struct device *dev);
#endif

#ifdef CONFIG_RING_BUFFER
int serial_vnd_queue_in_data(const struct device *dev, const unsigned char *c, uint32_t size)
{
	struct serial_vnd_data *data = dev->data;
	int write_size;

	if (data == NULL || data->read_queue == NULL) {
		return -ENOTSUP;
	}
	write_size = ring_buf_put(data->read_queue, c, size);

	LOG_DBG("size %u write_size %u", size, write_size);
	LOG_HEXDUMP_DBG(c, write_size, "");

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (write_size > 0) {
		irq_process(dev);
	}
#endif

#ifdef CONFIG_UART_ASYNC_API
	async_rx_run(dev);
#endif

	return write_size;
}

uint32_t serial_vnd_out_data_size_get(const struct device *dev)
{
	struct serial_vnd_data *data = dev->data;

	if (data == NULL || data->written == NULL) {
		return -ENOTSUP;
	}
	return ring_buf_size_get(data->written);
}

uint32_t serial_vnd_read_out_data(const struct device *dev, unsigned char *out_data, uint32_t size)
{
	struct serial_vnd_data *data = dev->data;

	if (data == NULL || data->written == NULL) {
		return -ENOTSUP;
	}
	return ring_buf_get(data->written, out_data, size);
}

uint32_t serial_vnd_peek_out_data(const struct device *dev, unsigned char *out_data, uint32_t size)
{
	struct serial_vnd_data *data = dev->data;

	if (data == NULL || data->written == NULL) {
		return -ENOTSUP;
	}
	return ring_buf_peek(data->written, out_data, size);
}
#endif

void serial_vnd_set_callback(const struct device *dev, serial_vnd_write_cb_t callback,
			     void *user_data)
{
	struct serial_vnd_data *data = dev->data;

	if (data == NULL) {
		return;
	}
	data->callback = callback;
	data->callback_data = user_data;
}

static int serial_vnd_err_check(const struct device *dev)
{
	return -ENOTSUP;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int serial_vnd_configure(const struct device *dev, const struct uart_config *cfg)
{
	return -ENOTSUP;
}

static int serial_vnd_config_get(const struct device *dev, struct uart_config *cfg)
{
	return -ENOTSUP;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_ASYNC_API
static int serial_vnd_callback_set(const struct device *dev, uart_callback_t callback,
				   void *user_data)
{
	struct serial_vnd_data *data = dev->data;

	if (data == NULL) {
		return -ENOTSUP;
	}

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS) && defined(CONFIG_UART_INTERRUPT_DRIVEN)
	data->irq_isr = NULL;
#endif

	if (callback == NULL && data->read_buf) {
		LOG_ERR("Setting callback to NULL while asynchronous API is in use.");
	}

	data->async_cb = callback;
	data->async_cb_user_data = user_data;

	return 0;
}

static int serial_vnd_api_tx(const struct device *dev, const uint8_t *tx_data, size_t len,
			     int32_t timeout)
{
	struct serial_vnd_data *data = dev->data;
	struct uart_event evt;
	uint32_t write_len;

	if (data == NULL) {
		return -ENOTSUP;
	}

	if (data->async_cb == NULL) {
		return -EINVAL;
	}

	write_len = ring_buf_put(data->written, tx_data, len);
	if (data->callback) {
		data->callback(dev, data->callback_data);
	}

	__ASSERT(write_len == len, "Ring buffer full. Async wait not implemented.");

	evt = (struct uart_event){
		.type = UART_TX_DONE,
		.data.tx.buf = tx_data,
		.data.tx.len = len,
	};
	data->async_cb(dev, &evt, data->async_cb_user_data);

	return 0;
}

static void async_rx_run(const struct device *dev)
{
	struct serial_vnd_data *data = dev->data;
	struct uart_event evt;
	uint32_t read_len;
	uint32_t read_remaining;

	if (!data->read_buf) {
		return;
	}

	__ASSERT_NO_MSG(data->async_cb);

	read_remaining = data->read_size - data->read_position;

	read_len = ring_buf_get(data->read_queue, &data->read_buf[data->read_position],
				read_remaining);

	if (read_len != 0) {
		evt = (struct uart_event){
			.type = UART_RX_RDY,
			.data.rx.buf = data->read_buf,
			.data.rx.len = read_len,
			.data.rx.offset = data->read_position,
		};
		data->async_cb(dev, &evt, data->async_cb_user_data);
	}

	data->read_position += read_len;

	if (data->read_position == data->read_size) {
		data->read_buf = NULL;
		evt = (struct uart_event){
			.type = UART_RX_DISABLED,
		};
		data->async_cb(dev, &evt, data->async_cb_user_data);
	}
}

static int serial_vnd_rx_enable(const struct device *dev, uint8_t *read_buf, size_t read_size,
				int32_t timeout)
{
	struct serial_vnd_data *data = dev->data;

	LOG_WRN("read_size %zd", read_size);

	if (data == NULL) {
		return -ENOTSUP;
	}

	if (data->async_cb == NULL) {
		return -EINVAL;
	}

	__ASSERT(timeout == SYS_FOREVER_MS, "Async timeout not implemented.");

	data->read_buf = read_buf;
	data->read_size = read_size;
	data->read_position = 0;

	async_rx_run(dev);

	return 0;
}
#endif /* CONFIG_UART_ASYNC_API */

static const struct uart_driver_api serial_vnd_api = {
	.poll_in = serial_vnd_poll_in,
	.poll_out = serial_vnd_poll_out,
	.err_check = serial_vnd_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = serial_vnd_configure,
	.config_get = serial_vnd_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_callback_set = irq_callback_set,
	.irq_rx_enable = irq_rx_enable,
	.irq_rx_disable = irq_rx_disable,
	.irq_rx_ready = irq_rx_ready,
	.irq_tx_enable = irq_tx_enable,
	.irq_tx_disable = irq_tx_disable,
	.irq_tx_ready = irq_tx_ready,
	.fifo_read = fifo_read,
	.fifo_fill = fifo_fill,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = serial_vnd_callback_set,
	.tx = serial_vnd_api_tx,
	.rx_enable = serial_vnd_rx_enable,
#endif /* CONFIG_UART_ASYNC_API */
};

#define VND_SERIAL_DATA_BUFFER(n)                                                                  \
	RING_BUF_DECLARE(written_data_##n, DT_INST_PROP(n, buffer_size));                          \
	RING_BUF_DECLARE(read_queue_##n, DT_INST_PROP(n, buffer_size));                            \
	static struct serial_vnd_data serial_vnd_data_##n = {                                      \
		.written = &written_data_##n,                                                      \
		.read_queue = &read_queue_##n,                                                     \
	};
#define VND_SERIAL_DATA(n) static struct serial_vnd_data serial_vnd_data_##n = {};
#define VND_SERIAL_INIT(n)                                                                         \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, buffer_size), (VND_SERIAL_DATA_BUFFER(n)),            \
		    (VND_SERIAL_DATA(n)))                                                          \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &serial_vnd_data_##n, NULL, POST_KERNEL,              \
			      CONFIG_SERIAL_INIT_PRIORITY, &serial_vnd_api);

DT_INST_FOREACH_STATUS_OKAY(VND_SERIAL_INIT)
