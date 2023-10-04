/*
 * Copyright (c) 2023 Fabian Blatz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_uart_emul

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_emul, CONFIG_UART_LOG_LEVEL);

struct uart_emul_config {
	bool loopback;
};

/* Device run time data */
struct uart_emul_data {
	struct uart_config cfg;

	struct ring_buf *rx_rb;
	struct k_spinlock rx_lock;

	uart_emul_callback_tx_data_ready_t tx_data_ready_cb;
	void *user_data;

	struct ring_buf *tx_rb;
	struct k_spinlock tx_lock;
};

int uart_emul_poll_in(const struct device *dev, unsigned char *p_char)
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

void uart_emul_poll_out(const struct device *dev, unsigned char out_char)
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

int uart_emul_err_check(const struct device *dev)
{
	return 0;
}

int uart_emul_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct uart_emul_data *drv_data = dev->data;

	memcpy(&drv_data->cfg, cfg, sizeof(struct uart_config));
	return 0;
}

int uart_emul_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct uart_emul_data *drv_data = dev->data;

	memcpy(cfg, &drv_data->cfg, sizeof(struct uart_config));
	return 0;
}

static const struct uart_driver_api uart_emul_api = {
	.poll_in = uart_emul_poll_in,
	.poll_out = uart_emul_poll_out,
	.config_get = uart_emul_config_get,
	.configure = uart_emul_configure,
	.err_check = uart_emul_err_check
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
	k_spinlock_key_t key;
	uint32_t count;

	key = k_spin_lock(&drv_data->rx_lock);
	count = ring_buf_put(drv_data->rx_rb, data, size);
	k_spin_unlock(&drv_data->rx_lock, key);
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

#define UART_EMUL_RX_FIFO_SIZE(inst) (DT_INST_PROP(inst, rx_fifo_size))
#define UART_EMUL_TX_FIFO_SIZE(inst) (DT_INST_PROP(inst, tx_fifo_size))

#define DEFINE_UART_EMUL(inst)                                                                     \
                                                                                                   \
	RING_BUF_DECLARE(uart_emul_##inst##_rx_rb, UART_EMUL_RX_FIFO_SIZE(inst));                  \
	RING_BUF_DECLARE(uart_emul_##inst##_tx_rb, UART_EMUL_TX_FIFO_SIZE(inst));                  \
                                                                                                   \
	static struct uart_emul_config uart_emul_cfg_##inst = {                                    \
		.loopback = DT_INST_PROP(inst, loopback),                                          \
	};                                                                                         \
	static struct uart_emul_data uart_emul_data_##inst = {                                     \
		.rx_rb = &uart_emul_##inst##_rx_rb,                                                \
		.tx_rb = &uart_emul_##inst##_tx_rb,                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &uart_emul_data_##inst, &uart_emul_cfg_##inst,     \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_emul_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_UART_EMUL)
