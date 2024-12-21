/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/serial/uart_async_to_irq.h>
#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(UART_ASYNC_TO_IRQ_LOG_NAME, CONFIG_UART_LOG_LEVEL);

/* Internal state flags. */

/* RX interrupt enabled. */
#define A2I_RX_IRQ_ENABLED	BIT(0)

/* TX interrupt enabled. */
#define A2I_TX_IRQ_ENABLED	BIT(1)

/* Error interrupt enabled. */
#define A2I_ERR_IRQ_ENABLED	BIT(2)

/* Receiver to be kept enabled. */
#define A2I_RX_ENABLE		BIT(3)

/* TX busy. */
#define A2I_TX_BUSY		BIT(4)

/* Error pending. */
#define A2I_ERR_PENDING		BIT(5)

static struct uart_async_to_irq_data *get_data(const struct device *dev)
{
	struct uart_async_to_irq_data **data = dev->data;

	return *data;
}

static const struct uart_async_to_irq_config *get_config(const struct device *dev)
{
	const struct uart_async_to_irq_config * const *config = dev->config;

	return *config;
}

/* Function calculates RX timeout based on baudrate. */
static uint32_t get_rx_timeout(const struct device *dev)
{
	struct uart_config cfg = { 0 };
	int err;
	uint32_t baudrate;

	err = uart_config_get(dev, &cfg);
	if (err == 0) {
		baudrate = cfg.baudrate;
	} else {
		baudrate = get_config(dev)->baudrate;
	}

	uint32_t us = (CONFIG_UART_ASYNC_TO_INT_DRIVEN_RX_TIMEOUT * 1000000) / baudrate;

	return us;
}

static int rx_enable(const struct device *dev,
		     struct uart_async_to_irq_data *data,
		     uint8_t *buf,
		     size_t len)
{
	int err;
	const struct uart_async_to_irq_config *config = get_config(dev);

	err = config->api->rx_enable(dev, buf, len, get_rx_timeout(dev));

	return err;
}

static int try_rx_enable(const struct device *dev, struct uart_async_to_irq_data *data)
{
	uint8_t *buf = uart_async_rx_buf_req(&data->rx.async_rx);
	size_t len = uart_async_rx_get_buf_len(&data->rx.async_rx);

	if (buf == NULL) {
		return -EBUSY;
	}

	return rx_enable(dev, data, buf, len);
}

static void on_rx_buf_req(const struct device *dev,
			  const struct uart_async_to_irq_config *config,
			  struct uart_async_to_irq_data *data)
{
	struct uart_async_rx *async_rx = &data->rx.async_rx;
	uint8_t *buf = uart_async_rx_buf_req(async_rx);
	size_t len = uart_async_rx_get_buf_len(async_rx);

	if (buf) {
		int err = config->api->rx_buf_rsp(dev, buf, len);

		if (err < 0) {
			uart_async_rx_on_buf_rel(async_rx, buf);
		}
	} else {
		atomic_inc(&data->rx.pending_buf_req);
	}
}

static void on_rx_dis(const struct device *dev, struct uart_async_to_irq_data *data)
{
	if (data->flags & A2I_RX_ENABLE) {
		int err;

		if (data->rx.async_rx.pending_bytes == 0) {
			uart_async_rx_reset(&data->rx.async_rx);
		}

		err = try_rx_enable(dev, data);
		if (err == 0) {
			data->rx.pending_buf_req = 0;
		}

		LOG_INST_DBG(get_config(dev)->log, "Reenabling RX from RX_DISABLED (err:%d)", err);
		__ASSERT((err >= 0) || (err == -EBUSY), "err: %d", err);
		return;
	}

	k_sem_give(&data->rx.sem);
}

static void uart_async_to_irq_callback(const struct device *dev,
					struct uart_event *evt,
					void *user_data)
{
	struct uart_async_to_irq_data *data = (struct uart_async_to_irq_data *)user_data;
	const struct uart_async_to_irq_config *config = get_config(dev);
	bool call_handler = false;

	switch (evt->type) {
	case UART_TX_DONE:
		atomic_and(&data->flags, ~A2I_TX_BUSY);
		call_handler = data->flags & A2I_TX_IRQ_ENABLED;
		break;
	case UART_RX_RDY:
		uart_async_rx_on_rdy(&data->rx.async_rx, evt->data.rx.buf, evt->data.rx.len);
		call_handler = data->flags & A2I_RX_IRQ_ENABLED;
		break;
	case UART_RX_BUF_REQUEST:
		on_rx_buf_req(dev, config, data);
		break;
	case UART_RX_BUF_RELEASED:
		uart_async_rx_on_buf_rel(&data->rx.async_rx, evt->data.rx_buf.buf);
		break;
	case UART_RX_STOPPED:
		atomic_or(&data->flags, A2I_ERR_PENDING);
		call_handler = data->flags & A2I_ERR_IRQ_ENABLED;
		break;
	case UART_RX_DISABLED:
		on_rx_dis(dev, data);
		break;
	default:
		break;
	}

	if (data->callback && call_handler) {
		atomic_inc(&data->irq_req);
		config->trampoline(dev);
	}
}

int z_uart_async_to_irq_fifo_fill(const struct device *dev, const uint8_t *buf, int len)
{
	struct uart_async_to_irq_data *data = get_data(dev);
	const struct uart_async_to_irq_config *config = get_config(dev);
	int err;

	len = MIN(len, data->tx.len);
	if (atomic_or(&data->flags, A2I_TX_BUSY) & A2I_TX_BUSY) {
		return 0;
	}

	memcpy(data->tx.buf, buf, len);

	err = config->api->tx(dev, data->tx.buf, len, SYS_FOREVER_US);
	if (err < 0) {
		atomic_and(&data->flags, ~A2I_TX_BUSY);
		return 0;
	}

	return len;
}

/** Interrupt driven FIFO read function */
int z_uart_async_to_irq_fifo_read(const struct device *dev,
				uint8_t *buf,
				const int len)
{
	struct uart_async_to_irq_data *data = get_data(dev);
	const struct uart_async_to_irq_config *config = get_config(dev);
	struct uart_async_rx *async_rx = &data->rx.async_rx;
	size_t claim_len;
	uint8_t *claim_buf;

	claim_len = uart_async_rx_data_claim(async_rx, &claim_buf, len);
	if (claim_len == 0) {
		return 0;
	}

	memcpy(buf, claim_buf, claim_len);
	bool buf_available = uart_async_rx_data_consume(async_rx, claim_len);

	if (data->rx.pending_buf_req && buf_available) {
		buf = uart_async_rx_buf_req(async_rx);
		__ASSERT_NO_MSG(buf != NULL);
		int err;
		size_t rx_len = uart_async_rx_get_buf_len(async_rx);

		atomic_dec(&data->rx.pending_buf_req);
		err = config->api->rx_buf_rsp(dev, buf, rx_len);
		if (err < 0) {
			if (err == -EACCES) {
				data->rx.pending_buf_req = 0;
				err = rx_enable(dev, data, buf, rx_len);
			}
			if (err < 0) {
				return err;
			}
		}
	}

	return (int)claim_len;
}

static void dir_disable(const struct device *dev, uint32_t flag)
{
	struct uart_async_to_irq_data *data = get_data(dev);

	atomic_and(&data->flags, ~flag);
}

static void dir_enable(const struct device *dev, uint32_t flag)
{
	struct uart_async_to_irq_data *data = get_data(dev);

	atomic_or(&data->flags, flag);

	atomic_inc(&data->irq_req);
	get_config(dev)->trampoline(dev);
}

/** Interrupt driven transfer enabling function */
void z_uart_async_to_irq_irq_tx_enable(const struct device *dev)
{
	dir_enable(dev, A2I_TX_IRQ_ENABLED);
}

/** Interrupt driven transfer disabling function */
void z_uart_async_to_irq_irq_tx_disable(const struct device *dev)
{
	dir_disable(dev, A2I_TX_IRQ_ENABLED);
}

/** Interrupt driven transfer ready function */
int z_uart_async_to_irq_irq_tx_ready(const struct device *dev)
{
	struct uart_async_to_irq_data *data = get_data(dev);
	bool ready = (data->flags & A2I_TX_IRQ_ENABLED) && !(data->flags & A2I_TX_BUSY);

	/* async API handles arbitrary sizes */
	return ready ? data->tx.len : 0;
}

/** Interrupt driven receiver enabling function */
void z_uart_async_to_irq_irq_rx_enable(const struct device *dev)
{
	dir_enable(dev, A2I_RX_IRQ_ENABLED);
}

/** Interrupt driven receiver disabling function */
void z_uart_async_to_irq_irq_rx_disable(const struct device *dev)
{
	dir_disable(dev, A2I_RX_IRQ_ENABLED);
}

/** Interrupt driven transfer complete function */
int z_uart_async_to_irq_irq_tx_complete(const struct device *dev)
{
	return z_uart_async_to_irq_irq_tx_ready(dev) > 0 ? 1 : 0;
}

/** Interrupt driven receiver ready function */
int z_uart_async_to_irq_irq_rx_ready(const struct device *dev)
{
	struct uart_async_to_irq_data *data = get_data(dev);

	return (data->flags & A2I_RX_IRQ_ENABLED) && (data->rx.async_rx.pending_bytes > 0);
}

/** Interrupt driven error enabling function */
void z_uart_async_to_irq_irq_err_enable(const struct device *dev)
{
	dir_enable(dev, A2I_ERR_IRQ_ENABLED);
}

/** Interrupt driven error disabling function */
void z_uart_async_to_irq_irq_err_disable(const struct device *dev)
{
	dir_disable(dev, A2I_ERR_IRQ_ENABLED);
}

/** Interrupt driven pending status function */
int z_uart_async_to_irq_irq_is_pending(const struct device *dev)
{
	bool tx_rdy = z_uart_async_to_irq_irq_tx_ready(dev);
	bool rx_rdy = z_uart_async_to_irq_irq_rx_ready(dev);
	struct uart_async_to_irq_data *data = get_data(dev);
	bool err_pending = atomic_and(&data->flags, ~A2I_ERR_PENDING) & A2I_ERR_PENDING;

	return tx_rdy || rx_rdy || err_pending;
}

/** Interrupt driven interrupt update function */
int z_uart_async_to_irq_irq_update(const struct device *dev)
{
	return 1;
}

/** Set the irq callback function */
void z_uart_async_to_irq_irq_callback_set(const struct device *dev,
			 uart_irq_callback_user_data_t cb,
			 void *user_data)
{
	struct uart_async_to_irq_data *data = get_data(dev);

	data->callback = cb;
	data->user_data = user_data;
}

int uart_async_to_irq_rx_enable(const struct device *dev)
{
	struct uart_async_to_irq_data *data = get_data(dev);
	int err;

	err = try_rx_enable(dev, data);
	if (err == 0) {
		atomic_or(&data->flags, A2I_RX_ENABLE);
	}

	return err;
}

int uart_async_to_irq_rx_disable(const struct device *dev)
{
	struct uart_async_to_irq_data *data = get_data(dev);
	const struct uart_async_to_irq_config *config = get_config(dev);
	int err;

	if (atomic_and(&data->flags, ~A2I_RX_ENABLE) & A2I_RX_ENABLE) {
		err = config->api->rx_disable(dev);
		if (err < 0) {
			return err;
		}
		k_sem_take(&data->rx.sem, K_FOREVER);
	}

	uart_async_rx_reset(&data->rx.async_rx);

	return 0;
}

void uart_async_to_irq_trampoline_cb(const struct device *dev)
{
	struct uart_async_to_irq_data *data = get_data(dev);

	do {
		data->callback(dev, data->user_data);
	} while (atomic_dec(&data->irq_req) > 1);
}

int uart_async_to_irq_init(const struct device *dev)
{
	struct uart_async_to_irq_data *data = get_data(dev);
	const struct uart_async_to_irq_config *config = get_config(dev);
	int err;

	data->tx.buf = config->tx_buf;
	data->tx.len = config->tx_len;

	k_sem_init(&data->rx.sem, 0, 1);

	err = config->api->callback_set(dev, uart_async_to_irq_callback, data);
	if (err < 0) {
		return err;
	}

	return uart_async_rx_init(&data->rx.async_rx, &config->async_rx);
}
