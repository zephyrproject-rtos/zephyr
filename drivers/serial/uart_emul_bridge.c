/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_uart_emul_bridge

#include <zephyr/device.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_emul_bridge, CONFIG_UART_LOG_LEVEL);

/* Index of the direction property enum. */
enum uart_emul_bridge_direction {
	UART_EMUL_BRIDGE_BOTH,
	UART_EMUL_BRIDGE_FORWARD,
	UART_EMUL_BRIDGE_BACKWARD,
};

struct uart_emul_bridge_config {
	const struct device *uart;
	const struct device *emul;
	enum uart_emul_bridge_direction direction;
};

struct uart_emul_bridge_data {
	/* Data from the emulated UART waiting for the UART to transmit it. */
	struct ring_buf rb;
	uint8_t buf[CONFIG_UART_EMUL_BRIDGE_BUF_SIZE];
	struct k_spinlock lock;
};

/* Data received on the UART goes to the receive side of the emulated UART. */
static void uart_emul_bridge_rx(const struct device *dev, const struct device *bridge_dev)
{
	const struct uart_emul_bridge_config *cfg = bridge_dev->config;
	uint8_t buf[16];
	int n;

	while (uart_irq_rx_ready(dev) > 0) {
		n = uart_fifo_read(dev, buf, sizeof(buf));
		if (n <= 0) {
			break;
		}

		if (uart_emul_put_rx_data(cfg->emul, buf, n) < (uint32_t)n) {
			LOG_WRN("%s: rx fifo full, data dropped", cfg->emul->name);
		}
	}
}

/* The UART transmits what the emulated UART wrote. */
static void uart_emul_bridge_tx(const struct device *dev, const struct device *bridge_dev)
{
	struct uart_emul_bridge_data *data = bridge_dev->data;
	uint8_t *buf;
	uint32_t len;
	int n;

	len = ring_buf_get_ptr(&data->rb, &buf, 0);
	if (len == 0U) {
		uart_irq_tx_disable(dev);
		return;
	}

	n = uart_fifo_fill(dev, buf, len);
	if (n < 0) {
		LOG_ERR("%s: tx error: %d", dev->name, n);
		return;
	}

	ring_buf_consume(&data->rb, (uint32_t)n);
}

static void uart_emul_bridge_isr(const struct device *dev, void *user_data)
{
	const struct device *bridge_dev = user_data;

	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev) > 0) {
		uart_emul_bridge_rx(dev, bridge_dev);
	}

	if (uart_irq_tx_ready(dev) > 0) {
		uart_emul_bridge_tx(dev, bridge_dev);
	}
}

/* Called by the emulated UART, from any context, when it has data to transmit. */
static void uart_emul_bridge_tx_data_ready(const struct device *emul, size_t size,
					   void *user_data)
{
	const struct device *bridge_dev = user_data;
	const struct uart_emul_bridge_config *cfg = bridge_dev->config;
	struct uart_emul_bridge_data *data = bridge_dev->data;
	k_spinlock_key_t key;
	uint8_t *buf;
	uint32_t len;
	uint32_t n;

	ARG_UNUSED(size);

	key = k_spin_lock(&data->lock);
	while ((len = ring_buf_put_ptr(&data->rb, &buf, 0)) > 0U) {
		n = uart_emul_get_tx_data(emul, buf, len);
		ring_buf_commit(&data->rb, n);
		if (n < len) {
			break;
		}
	}
	k_spin_unlock(&data->lock, key);

	if (len == 0U) {
		n = uart_emul_flush_tx_data(emul);
		if (n > 0U) {
			LOG_WRN("%s: buffer full, %u bytes dropped", bridge_dev->name, n);
		}
	}

	uart_irq_tx_enable(cfg->uart);
}

static int uart_emul_bridge_init(const struct device *dev)
{
	const struct uart_emul_bridge_config *cfg = dev->config;
	struct uart_emul_bridge_data *data = dev->data;

	if (!device_is_ready(cfg->uart)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->uart);
		return -ENODEV;
	}

	if (!device_is_ready(cfg->emul)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->emul);
		return -ENODEV;
	}

	ring_buf_init(&data->rb, sizeof(data->buf), data->buf);
	uart_irq_callback_user_data_set(cfg->uart, uart_emul_bridge_isr, (void *)dev);

	if (cfg->direction != UART_EMUL_BRIDGE_BACKWARD) {
		uart_irq_rx_enable(cfg->uart);
	}

	if (cfg->direction != UART_EMUL_BRIDGE_FORWARD) {
		uart_emul_callback_tx_data_ready_set(cfg->emul, uart_emul_bridge_tx_data_ready,
						     (void *)dev);
	}

	return 0;
}

#define UART_EMUL_BRIDGE_DEFINE(inst)                                                              \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_INST_PHANDLE(inst, emul), zephyr_uart_emul),            \
		     "The emul peer must be a zephyr,uart-emul node");                             \
												   \
	static const struct uart_emul_bridge_config uart_emul_bridge_config_##inst = {             \
		.uart = DEVICE_DT_GET(DT_INST_PHANDLE(inst, uart)),                                \
		.emul = DEVICE_DT_GET(DT_INST_PHANDLE(inst, emul)),                                \
		.direction = DT_INST_ENUM_IDX(inst, direction),                                    \
	};                                                                                         \
												   \
	static struct uart_emul_bridge_data uart_emul_bridge_data_##inst;                          \
												   \
	DEVICE_DT_INST_DEFINE(inst, uart_emul_bridge_init, NULL, &uart_emul_bridge_data_##inst,    \
			      &uart_emul_bridge_config_##inst, POST_KERNEL,                        \
			      CONFIG_SERIAL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(UART_EMUL_BRIDGE_DEFINE)
