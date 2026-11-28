/*
 * Copyright (c) 2025 Ayush Singh, BeagleBoard.org
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/ipc/rpmsg_service.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT rpmsg_uart

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_rpmsg, CONFIG_UART_LOG_LEVEL);

#define RX_BUF_SIZE CONFIG_UART_RPMSG_RX_BUF_SIZE

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define TX_BUF_SIZE     CONFIG_UART_RPMSG_TX_BUF_SIZE
#define TX_CHUNK_SIZE   CONFIG_UART_RPMSG_TX_CHUNK_SIZE
/*
 * Limit how many times one soft-IRQ work item calls the UART callback so a
 * busy client cannot starve the system workqueue. If RX or TX is still
 * pending afterward, another soft IRQ is queued.
 */
#define SOFT_IRQ_ROUNDS 65
#endif

struct uart_rpmsg_data {
	const struct device *dev;
	int rpmsg_endpoint;
	uint8_t rx_rb_buf[RX_BUF_SIZE];
	struct ring_buf rx_rb;
	struct k_spinlock rx_lock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uint8_t tx_rb_buf[TX_BUF_SIZE];
	struct ring_buf tx_rb;
	struct k_spinlock tx_lock;
	struct k_work soft_irq_work;
	struct k_work tx_drain_work;
	bool rx_irq_enabled;
	bool tx_irq_enabled;
	uart_irq_callback_user_data_t user_callback;
	void *user_data;
#endif
};

struct uart_rpmsg_config {
	rpmsg_ept_cb cb;
};

static int uart_rpmsg_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct uart_rpmsg_data *data = dev->data;
	uint32_t read;

	K_SPINLOCK(&data->rx_lock) {
		read = ring_buf_get(&data->rx_rb, p_char, 1);
	}

	return (read > 0U) ? 0 : -1;
}

static void uart_rpmsg_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_rpmsg_data *data = dev->data;

	/*
	 * Send right away so printk and early console keep working even when
	 * the interrupt-driven TX path is idle. Those bytes can appear mixed
	 * with fifo_fill traffic on the host side.
	 */
	(void)rpmsg_service_send(data->rpmsg_endpoint, &c, sizeof(c));
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static bool uart_rpmsg_rx_irq_pending(struct uart_rpmsg_data *data)
{
	bool pending = false;

	K_SPINLOCK(&data->rx_lock) {
		pending = data->rx_irq_enabled && !ring_buf_is_empty(&data->rx_rb);
	}

	return pending;
}

static bool uart_rpmsg_tx_irq_pending(struct uart_rpmsg_data *data)
{
	bool pending = false;

	/*
	 * "TX ready" means the software FIFO still has room for uart_fifo_fill(),
	 * not that the queue has drained empty. Shell and similar clients need
	 * that so they can keep writing while drain work catches up.
	 */
	K_SPINLOCK(&data->tx_lock) {
		pending = data->tx_irq_enabled && (ring_buf_space_get(&data->tx_rb) > 0U);
	}

	return pending;
}

static void uart_rpmsg_submit_soft_irq(struct uart_rpmsg_data *data)
{
	(void)k_work_submit(&data->soft_irq_work);
}

static void uart_rpmsg_soft_irq_handler(struct k_work *work)
{
	struct uart_rpmsg_data *data = CONTAINER_OF(work, struct uart_rpmsg_data, soft_irq_work);
	uart_irq_callback_user_data_t callback = data->user_callback;
	void *user_data = data->user_data;
	int rounds = SOFT_IRQ_ROUNDS;

	if (callback == NULL) {
		return;
	}

	while (rounds-- > 0) {
		if (!uart_rpmsg_rx_irq_pending(data) && !uart_rpmsg_tx_irq_pending(data)) {
			return;
		}

		callback(data->dev, user_data);
	}

	if (uart_rpmsg_rx_irq_pending(data) || uart_rpmsg_tx_irq_pending(data)) {
		uart_rpmsg_submit_soft_irq(data);
	}
}

static void uart_rpmsg_tx_drain_handler(struct k_work *work)
{
	struct uart_rpmsg_data *data = CONTAINER_OF(work, struct uart_rpmsg_data, tx_drain_work);
	uint8_t chunk[TX_CHUNK_SIZE];
	uint32_t chunk_len;
	int sent;
	bool kick_soft_irq = false;

	do {
		chunk_len = 0U;

		K_SPINLOCK(&data->tx_lock) {
			chunk_len = ring_buf_get(&data->tx_rb, chunk, sizeof(chunk));
		}

		if (chunk_len == 0U) {
			break;
		}

		sent = rpmsg_service_send(data->rpmsg_endpoint, chunk, chunk_len);
		if (sent < 0) {
			LOG_WRN_ONCE("RPMsg TX failed (%d)", sent);
			break;
		}
	} while (true);

	K_SPINLOCK(&data->tx_lock) {
		kick_soft_irq = data->tx_irq_enabled && (ring_buf_space_get(&data->tx_rb) > 0U);
	}

	if (kick_soft_irq) {
		uart_rpmsg_submit_soft_irq(data);
	}
}

static int uart_rpmsg_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	struct uart_rpmsg_data *data = dev->data;
	uint32_t written;

	if (size <= 0) {
		return 0;
	}

	K_SPINLOCK(&data->tx_lock) {
		written = ring_buf_put(&data->tx_rb, tx_data, (uint32_t)size);
	}

	if (written > 0U) {
		(void)k_work_submit(&data->tx_drain_work);
	}

	return (int)written;
}

static int uart_rpmsg_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct uart_rpmsg_data *data = dev->data;
	uint32_t read;

	if (size <= 0) {
		return 0;
	}

	K_SPINLOCK(&data->rx_lock) {
		read = ring_buf_get(&data->rx_rb, rx_data, (uint32_t)size);
	}

	return (int)read;
}

static int uart_rpmsg_irq_tx_ready(const struct device *dev)
{
	return uart_rpmsg_tx_irq_pending(dev->data) ? 1 : 0;
}

static int uart_rpmsg_irq_tx_complete(const struct device *dev)
{
	struct uart_rpmsg_data *data = dev->data;
	bool empty = false;

	K_SPINLOCK(&data->tx_lock) {
		empty = ring_buf_is_empty(&data->tx_rb);
	}

	return empty ? 1 : 0;
}

static void uart_rpmsg_irq_tx_enable(const struct device *dev)
{
	struct uart_rpmsg_data *data = dev->data;
	bool kick = false;

	K_SPINLOCK(&data->tx_lock) {
		data->tx_irq_enabled = true;
		kick = ring_buf_space_get(&data->tx_rb) > 0U;
	}

	if (kick) {
		uart_rpmsg_submit_soft_irq(data);
	}
}

static void uart_rpmsg_irq_tx_disable(const struct device *dev)
{
	struct uart_rpmsg_data *data = dev->data;

	K_SPINLOCK(&data->tx_lock) {
		data->tx_irq_enabled = false;
	}
}

static int uart_rpmsg_irq_rx_ready(const struct device *dev)
{
	return uart_rpmsg_rx_irq_pending(dev->data) ? 1 : 0;
}

static void uart_rpmsg_irq_rx_enable(const struct device *dev)
{
	struct uart_rpmsg_data *data = dev->data;
	bool kick = false;

	K_SPINLOCK(&data->rx_lock) {
		data->rx_irq_enabled = true;
		kick = !ring_buf_is_empty(&data->rx_rb);
	}

	if (kick) {
		uart_rpmsg_submit_soft_irq(data);
	}
}

static void uart_rpmsg_irq_rx_disable(const struct device *dev)
{
	struct uart_rpmsg_data *data = dev->data;

	K_SPINLOCK(&data->rx_lock) {
		data->rx_irq_enabled = false;
	}
}

static int uart_rpmsg_irq_is_pending(const struct device *dev)
{
	struct uart_rpmsg_data *data = dev->data;

	return (uart_rpmsg_rx_irq_pending(data) || uart_rpmsg_tx_irq_pending(data)) ? 1 : 0;
}

static void uart_rpmsg_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *user_data)
{
	struct uart_rpmsg_data *data = dev->data;

	data->user_callback = cb;
	data->user_data = user_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static void uart_rpmsg_on_rx_bytes(struct uart_rpmsg_data *data, const void *payload, size_t len)
{
	uint32_t written;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	bool kick_soft_irq = false;
#endif

	K_SPINLOCK(&data->rx_lock) {
		written = ring_buf_put(&data->rx_rb, payload, len);
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
		kick_soft_irq = data->rx_irq_enabled && (written > 0U);
#endif
	}

	if (written < len) {
		LOG_WRN("Ring buffer full, drop %zu bytes", len - written);
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (kick_soft_irq) {
		uart_rpmsg_submit_soft_irq(data);
	}
#endif
}

static DEVICE_API(uart, uart_rpmsg_driver_api) = {
	.poll_in = uart_rpmsg_poll_in,
	.poll_out = uart_rpmsg_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_rpmsg_fifo_fill,
	.fifo_read = uart_rpmsg_fifo_read,
	.irq_tx_enable = uart_rpmsg_irq_tx_enable,
	.irq_tx_disable = uart_rpmsg_irq_tx_disable,
	.irq_tx_ready = uart_rpmsg_irq_tx_ready,
	.irq_tx_complete = uart_rpmsg_irq_tx_complete,
	.irq_rx_enable = uart_rpmsg_irq_rx_enable,
	.irq_rx_disable = uart_rpmsg_irq_rx_disable,
	.irq_rx_ready = uart_rpmsg_irq_rx_ready,
	.irq_is_pending = uart_rpmsg_irq_is_pending,
	.irq_callback_set = uart_rpmsg_irq_callback_set,
#endif
};

static int uart_rpmsg_init(const struct device *dev)
{
	const struct uart_rpmsg_config *conf = dev->config;
	struct uart_rpmsg_data *data = dev->data;
	int ret;

	data->dev = dev;
	ring_buf_init(&data->rx_rb, sizeof(data->rx_rb_buf), data->rx_rb_buf);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	ring_buf_init(&data->tx_rb, sizeof(data->tx_rb_buf), data->tx_rb_buf);
	k_work_init(&data->soft_irq_work, uart_rpmsg_soft_irq_handler);
	k_work_init(&data->tx_drain_work, uart_rpmsg_tx_drain_handler);
#endif

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
		ARG_UNUSED(priv);                                                                  \
		ARG_UNUSED(ept);                                                                   \
		ARG_UNUSED(src);                                                                   \
                                                                                                   \
		uart_rpmsg_on_rx_bytes(&uart_rpmsg_data_##n, data, len);                           \
                                                                                                   \
		return RPMSG_SUCCESS;                                                              \
	}                                                                                          \
                                                                                                   \
	static const struct uart_rpmsg_config uart_rpmsg_config_##n = {                            \
		.cb = rpmsg_recv_tty_callback_##n,                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_rpmsg_init, NULL, &uart_rpmsg_data_##n,                      \
			      &uart_rpmsg_config_##n, POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY,    \
			      &uart_rpmsg_driver_api)

DT_INST_FOREACH_STATUS_OKAY(UART_RPMSG_INIT)
