/*
 * Copyright (c) 2025 Ayush Singh, BeagleBoard.org
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/ipc/rpmsg_service.h>

#define DT_DRV_COMPAT rpmsg_uart

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_rpmsg, CONFIG_UART_LOG_LEVEL);

#define RX_BUF_SIZE CONFIG_UART_RPMSG_RX_BUF_SIZE

struct uart_rpmsg_data {
	int rpmsg_endpoint;
	uint8_t rx_rb_buf[RX_BUF_SIZE];
	struct ring_buf rx_rb;
	struct k_spinlock rx_lock;
};

struct uart_rpmsg_config {
	rpmsg_ept_cb cb;
};

static int uart_rpmsg_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct uart_rpmsg_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t read;

	key = k_spin_lock(&data->rx_lock);
	read = ring_buf_get(&data->rx_rb, p_char, 1);
	k_spin_unlock(&data->rx_lock, key);

	return (read > 0) ? 0 : 1;
}

static void uart_rpmsg_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_rpmsg_data *data = dev->data;

	rpmsg_service_send(data->rpmsg_endpoint, &c, sizeof(c));
}

static DEVICE_API(uart, uart_rpmsg_driver_api) = {
	.poll_in = uart_rpmsg_poll_in,
	.poll_out = uart_rpmsg_poll_out,
};

static int uart_rpmsg_init(const struct device *dev)
{
	int ret;
	struct uart_rpmsg_data *data = dev->data;
	const struct uart_rpmsg_config *conf = dev->config;

	ring_buf_init(&data->rx_rb, sizeof(data->rx_rb_buf), data->rx_rb_buf);

	ret = rpmsg_service_register_endpoint("rpmsg-tty", conf->cb);
	if (ret < 0) {
		return ret;
	}

	data->rpmsg_endpoint = ret;

	return 0;
}

#define UART_RPMSG_INIT(n)                                                                         \
	static struct uart_rpmsg_data uart_rpmsg_data_##n = {};                                    \
                                                                                                   \
	static int rpmsg_recv_tty_callback_##n(struct rpmsg_endpoint *ept, void *data, size_t len, \
					       uint32_t src, void *priv)                           \
	{                                                                                          \
		int written;                                                                       \
		struct uart_rpmsg_data *drv_data = &uart_rpmsg_data_##n;                           \
		k_spinlock_key_t key;                                                              \
                                                                                                   \
		ARG_UNUSED(priv);                                                                  \
		ARG_UNUSED(ept);                                                                   \
		ARG_UNUSED(src);                                                                   \
                                                                                                   \
		key = k_spin_lock(&drv_data->rx_lock);                                             \
		written = ring_buf_put(&drv_data->rx_rb, data, len);                               \
		if (written < len) {                                                               \
			LOG_WRN("Ring buffer full, drop %zd bytes", len - written);                \
		}                                                                                  \
		k_spin_unlock(&drv_data->rx_lock, key);                                            \
                                                                                                   \
		return RPMSG_SUCCESS;                                                              \
	}                                                                                          \
                                                                                                   \
	static struct uart_rpmsg_config uart_rpmsg_config_##n = {                                  \
		.cb = rpmsg_recv_tty_callback_##n,                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_rpmsg_init, NULL, &uart_rpmsg_data_##n,                      \
			      &uart_rpmsg_config_##n, POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY,    \
			      &uart_rpmsg_driver_api)

DT_INST_FOREACH_STATUS_OKAY(UART_RPMSG_INIT)
