/*
 * Copyright 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart/uart_bridge.h>
#include <zephyr/pm/device.h>

#define DT_DRV_COMPAT zephyr_uart_bridge

LOG_MODULE_REGISTER(uart_bridge, LOG_LEVEL_INF);

#define RING_BUF_SIZE CONFIG_UART_BRIDGE_BUF_SIZE
#define RING_BUF_FULL_THRESHOLD (RING_BUF_SIZE / 3)

struct uart_bridge_config {
	const struct device *peer_dev[2];
};

struct uart_bridge_peer_data {
	uint8_t buf[RING_BUF_SIZE];
	struct ring_buf rb;
	bool paused;
};

struct uart_bridge_data {
	struct uart_bridge_peer_data peer[2];
};

static const struct device *uart_bridge_get_peer(const struct device *dev,
						 const struct device *bridge_dev)
{
	const struct uart_bridge_config *cfg = bridge_dev->config;

	if (dev == cfg->peer_dev[0]) {
		return cfg->peer_dev[1];
	} else if (dev == cfg->peer_dev[1]) {
		return cfg->peer_dev[0];
	} else {
		return NULL;
	}
}

void uart_bridge_settings_update(const struct device *dev,
				 const struct device *bridge_dev)
{
	struct uart_config cfg;
	const struct device *peer_dev = uart_bridge_get_peer(dev, bridge_dev);
	int ret;

	if (peer_dev == NULL) {
		LOG_DBG("%s: not a bridge dev", dev->name);
		return;
	}

	LOG_DBG("update settings: dev=%s bridge=%s peer=%s", dev->name,
		bridge_dev->name, peer_dev->name);

	ret = uart_config_get(dev, &cfg);
	if (ret) {
		LOG_WRN("%s: failed to get the uart config: %d", dev->name,
			ret);
		return;
	}

	ret = uart_configure(peer_dev, &cfg);
	if (ret) {
		LOG_WRN("%s: failed to set the uart config: %d", peer_dev->name,
			ret);
		return;
	}

	LOG_INF("uart settings: baudrate=%d parity=%d", cfg.baudrate,
		cfg.parity);
}

static uint8_t uart_bridge_get_idx(const struct device *dev,
				   const struct device *bridge_dev, bool own)
{
	const struct uart_bridge_config *cfg = bridge_dev->config;

	if (dev == cfg->peer_dev[0]) {
		return own ? 0 : 1;
	} else {
		return own ? 1 : 0;
	}
}

static void uart_bridge_handle_rx(const struct device *dev,
				  const struct device *bridge_dev)
{
	const struct uart_bridge_config *cfg = bridge_dev->config;
	struct uart_bridge_data *data = bridge_dev->data;

	const struct device *peer_dev =
		cfg->peer_dev[uart_bridge_get_idx(dev, bridge_dev, false)];
	struct uart_bridge_peer_data *own_data =
		&data->peer[uart_bridge_get_idx(dev, bridge_dev, true)];

	uint8_t *recv_buf;
	int rb_len, recv_len;
	int ret;

	if (ring_buf_space_get(&own_data->rb) < RING_BUF_FULL_THRESHOLD) {
		LOG_DBG("%s: buffer full: pause", dev->name);
		uart_irq_rx_disable(dev);
		own_data->paused = true;
		return;
	}

	rb_len = ring_buf_put_claim(&own_data->rb, &recv_buf, RING_BUF_SIZE);
	if (rb_len == 0) {
		LOG_WRN("%s: ring_buf full", dev->name);
		return;
	}

	recv_len = uart_fifo_read(dev, recv_buf, rb_len);
	if (recv_len < 0) {
		ring_buf_put_finish(&own_data->rb, 0);
		LOG_ERR("%s: rx error: %d", dev->name, recv_len);
		return;
	}

	ret = ring_buf_put_finish(&own_data->rb, recv_len);
	if (ret < 0) {
		LOG_ERR("%s: ring_buf_put_finish error: %d", dev->name, rb_len);
		return;
	}

	uart_irq_tx_enable(peer_dev);
}

static void uart_bridge_handle_tx(const struct device *dev,
				  const struct device *bridge_dev)
{
	const struct uart_bridge_config *cfg = bridge_dev->config;
	struct uart_bridge_data *data = bridge_dev->data;

	const struct device *peer_dev =
		cfg->peer_dev[uart_bridge_get_idx(dev, bridge_dev, false)];
	struct uart_bridge_peer_data *peer_data =
		&data->peer[uart_bridge_get_idx(dev, bridge_dev, false)];

	uint8_t *send_buf;
	int rb_len, sent_len;
	int ret;

	rb_len = ring_buf_get_claim(&peer_data->rb, &send_buf, RING_BUF_SIZE);
	if (rb_len == 0) {
		LOG_DBG("%s: buffer empty, disable tx irq", dev->name);
		uart_irq_tx_disable(dev);
		return;
	}

	sent_len = uart_fifo_fill(dev, send_buf, rb_len);
	if (sent_len < 0) {
		ring_buf_get_finish(&peer_data->rb, 0);
		LOG_ERR("%s: tx error: %d", dev->name, sent_len);
		return;
	}

	ret = ring_buf_get_finish(&peer_data->rb, sent_len);
	if (ret < 0) {
		LOG_ERR("ring_buf_get_finish error: %d", ret);
		return;
	}

	if (peer_data->paused &&
	    ring_buf_space_get(&peer_data->rb) > RING_BUF_FULL_THRESHOLD) {
		LOG_DBG("%s: buffer free: resume", dev->name);
		uart_irq_rx_enable(peer_dev);
		peer_data->paused = false;
		return;
	}
}

static void interrupt_handler(const struct device *dev, void *user_data)
{
	const struct device *bridge_dev = user_data;

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			uart_bridge_handle_rx(dev, bridge_dev);
		}
		if (uart_irq_tx_ready(dev)) {
			uart_bridge_handle_tx(dev, bridge_dev);
		}
	}
}

static int uart_bridge_pm_action(const struct device *dev,
				 enum pm_device_action action)
{
	const struct uart_bridge_config *cfg = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		uart_irq_rx_disable(cfg->peer_dev[0]);
		uart_irq_rx_disable(cfg->peer_dev[1]);
		uart_irq_callback_user_data_set(cfg->peer_dev[0], NULL, NULL);
		uart_irq_callback_user_data_set(cfg->peer_dev[1], NULL, NULL);
		break;
	case PM_DEVICE_ACTION_RESUME:
		uart_irq_callback_user_data_set(cfg->peer_dev[0], interrupt_handler,
						(void *)dev);
		uart_irq_callback_user_data_set(cfg->peer_dev[1], interrupt_handler,
						(void *)dev);
		uart_irq_rx_enable(cfg->peer_dev[0]);
		uart_irq_rx_enable(cfg->peer_dev[1]);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int uart_bridge_init(const struct device *dev)
{
	struct uart_bridge_data *data = dev->data;

	ring_buf_init(&data->peer[0].rb, RING_BUF_SIZE, data->peer[0].buf);
	ring_buf_init(&data->peer[1].rb, RING_BUF_SIZE, data->peer[1].buf);

	return pm_device_driver_init(dev, uart_bridge_pm_action);
}

#define UART_BRIDGE_INIT(n)							\
	BUILD_ASSERT(DT_INST_PROP_LEN(n, peers) == 2,				\
		     "uart-bridge peers property must have exactly 2 members");	\
										\
	static const struct uart_bridge_config uart_bridge_cfg_##n = {		\
		.peer_dev = {DT_INST_FOREACH_PROP_ELEM_SEP(			\
			n, peers, DEVICE_DT_GET_BY_IDX, (,))},			\
	};									\
										\
	static struct uart_bridge_data uart_bridge_data_##n;			\
										\
	PM_DEVICE_DT_INST_DEFINE(n, uart_bridge_pm_action);			\
										\
	DEVICE_DT_INST_DEFINE(n, uart_bridge_init, PM_DEVICE_DT_INST_GET(n),	\
			      &uart_bridge_data_##n, &uart_bridge_cfg_##n,	\
			      POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(UART_BRIDGE_INIT)
