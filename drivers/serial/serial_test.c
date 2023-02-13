/*
 * Copyright (c) 2021, Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real serial driver. It is used to instantiate struct
 * devices for the "vnd,serial" devicetree compatible used in test code.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/uart/serial_test.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#define DT_DRV_COMPAT vnd_serial
struct serial_vnd_data {
#ifdef CONFIG_RING_BUFFER
	struct ring_buf *written;
	struct ring_buf *read_queue;
#endif
	serial_vnd_write_cb_t callback;
	void *callback_data;
};

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

#ifdef CONFIG_RING_BUFFER
int serial_vnd_queue_in_data(const struct device *dev, unsigned char *c, uint32_t size)
{
	struct serial_vnd_data *data = dev->data;

	if (data == NULL || data->read_queue == NULL) {
		return -ENOTSUP;
	}
	return ring_buf_put(data->read_queue, c, size);
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

static const struct uart_driver_api serial_vnd_api = {
	.poll_in = serial_vnd_poll_in,
	.poll_out = serial_vnd_poll_out,
	.err_check = serial_vnd_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = serial_vnd_configure,
	.config_get = serial_vnd_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
};

static int serial_vnd_init(const struct device *dev)
{
	return 0;
}

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
	DEVICE_DT_INST_DEFINE(n, &serial_vnd_init, NULL, &serial_vnd_data_##n, NULL, POST_KERNEL,  \
			      CONFIG_SERIAL_INIT_PRIORITY, &serial_vnd_api);

DT_INST_FOREACH_STATUS_OKAY(VND_SERIAL_INIT)
