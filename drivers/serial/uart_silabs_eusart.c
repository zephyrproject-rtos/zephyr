/*
 * Copyright (c) 2024, Yishai Jaffe
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_eusart_uart

#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <em_eusart.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_silabs_ldma.h>

LOG_MODULE_REGISTER(uart_silabs_eusart, CONFIG_UART_LOG_LEVEL);

struct eusart_dma_channel {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_block_config blk_cfg;
	struct dma_config dma_cfg;
	uint8_t priority;
	uint8_t *buffer;
	size_t buffer_length;
	size_t counter;
	size_t offset;
	struct k_work_delayable timeout_work;
	int32_t timeout;
	bool enabled;
};

struct eusart_config {
	EUSART_TypeDef *eusart;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	void (*irq_config_func)(const struct device *dev);
};

enum eusart_pm_lock {
	EUSART_PM_LOCK_TX,
	EUSART_PM_LOCK_RX,
	EUSART_PM_LOCK_COUNT,
};

struct eusart_data {
	struct uart_config uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
#ifdef CONFIG_UART_SILABS_EUSART_ASYNC
	const struct device *uart_dev;
	uart_callback_t async_cb;
	void *async_user_data;
	struct eusart_dma_channel dma_rx;
	struct eusart_dma_channel dma_tx;
	uint8_t *rx_next_buffer;
	size_t rx_next_buffer_len;
#endif
#ifdef CONFIG_PM
	ATOMIC_DEFINE(pm_lock, EUSART_PM_LOCK_COUNT);
#endif
};

static int eusart_pm_action(const struct device *dev, enum pm_device_action action);

/**
 * @brief Get PM lock on low power states
 *
 * @param dev  UART device struct
 * @param lock UART PM lock type
 *
 * @return true if lock was taken, false otherwise
 */
static __maybe_unused bool eusart_pm_lock_get(const struct device *dev, enum eusart_pm_lock lock)
{
#ifdef CONFIG_PM
	struct eusart_data *data = dev->data;
	bool was_locked = atomic_test_and_set_bit(data->pm_lock, lock);

	if (!was_locked) {
		/* Lock out low-power states that would interfere with UART traffic */
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}

	return !was_locked;
#else
	return false;
#endif
}

/**
 * @brief Release PM lock on low power states
 *
 * @param dev  UART device struct
 * @param lock UART PM lock type
 *
 * @return true if lock was released, false otherwise
 */
static __maybe_unused bool eusart_pm_lock_put(const struct device *dev, enum eusart_pm_lock lock)
{
#ifdef CONFIG_PM
	struct eusart_data *data = dev->data;
	bool was_locked = atomic_test_and_clear_bit(data->pm_lock, lock);

	if (was_locked) {
		/* Unlock low-power states that would interfere with UART traffic */
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}

	return was_locked;
#else
	return false;
#endif
}

static int eusart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct eusart_config *config = dev->config;

	if (EUSART_StatusGet(config->eusart) & EUSART_STATUS_RXFL) {
		*c = EUSART_Rx(config->eusart);
		return 0;
	}

	return -1;
}

static void eusart_poll_out(const struct device *dev, unsigned char c)
{
	const struct eusart_config *config = dev->config;

	/* EUSART_Tx function already waits for the transmit buffer being empty
	 * and waits for the bus to be free to transmit.
	 */
	EUSART_Tx(config->eusart, c);

	while (!(config->eusart->STATUS & EUSART_STATUS_TXC)) {
	}
}

static int eusart_err_check(const struct device *dev)
{
	const struct eusart_config *config = dev->config;
	uint32_t flags = EUSART_IntGet(config->eusart);
	int err = 0;

	if (flags & EUSART_IF_RXOF) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & EUSART_IF_PERR) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & EUSART_IF_FERR) {
		err |= UART_ERROR_FRAMING;
	}

	EUSART_IntClear(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int eusart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct eusart_config *config = dev->config;
	int i = 0;

	while ((i < len) && (EUSART_StatusGet(config->eusart) & EUSART_STATUS_TXFL)) {
		config->eusart->TXDATA = (uint32_t)tx_data[i++];
	}

	if (!(EUSART_StatusGet(config->eusart) & EUSART_STATUS_TXFL)) {
		EUSART_IntClear(config->eusart, EUSART_IF_TXFL);
	}

	return i;
}

static int eusart_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	const struct eusart_config *config = dev->config;
	int i = 0;

	while ((i < len) && (EUSART_StatusGet(config->eusart) & EUSART_STATUS_RXFL)) {
		rx_data[i++] = (uint8_t)config->eusart->RXDATA;
	}

	if (!(EUSART_StatusGet(config->eusart) & EUSART_STATUS_RXFL)) {
		EUSART_IntClear(config->eusart, EUSART_IF_RXFL);
	}

	return i;
}

static void eusart_irq_tx_enable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	eusart_pm_lock_get(dev, EUSART_PM_LOCK_TX);
	EUSART_IntClear(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
	EUSART_IntEnable(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
}

static void eusart_irq_tx_disable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	EUSART_IntDisable(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
	EUSART_IntClear(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
	eusart_pm_lock_put(dev, EUSART_PM_LOCK_TX);
}

static int eusart_irq_tx_complete(const struct device *dev)
{
	const struct eusart_config *config = dev->config;
	uint32_t flags = EUSART_IntGet(config->eusart);

	EUSART_IntClear(config->eusart, EUSART_IF_TXC);

	return !!(flags & EUSART_IF_TXC);
}

static int eusart_irq_tx_ready(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	return (config->eusart->IEN & EUSART_IEN_TXFL) &&
	       (EUSART_IntGet(config->eusart) & EUSART_IF_TXFL);
}

static void eusart_irq_rx_enable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	eusart_pm_lock_get(dev, EUSART_PM_LOCK_RX);
	EUSART_IntClear(config->eusart, EUSART_IEN_RXFL);
	EUSART_IntEnable(config->eusart, EUSART_IEN_RXFL);
}

static void eusart_irq_rx_disable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	EUSART_IntDisable(config->eusart, EUSART_IEN_RXFL);
	EUSART_IntClear(config->eusart, EUSART_IEN_RXFL);
	eusart_pm_lock_put(dev, EUSART_PM_LOCK_RX);
}

static int eusart_irq_rx_ready(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	return (config->eusart->IEN & EUSART_IEN_RXFL) &&
	       (EUSART_IntGet(config->eusart) & EUSART_IF_RXFL);
}

static void eusart_irq_err_enable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	EUSART_IntClear(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
	EUSART_IntEnable(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
}

static void eusart_irq_err_disable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;

	EUSART_IntDisable(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
	EUSART_IntClear(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
}

static int eusart_irq_is_pending(const struct device *dev)
{
	return eusart_irq_tx_ready(dev) || eusart_irq_rx_ready(dev);
}

static int eusart_irq_update(const struct device *dev)
{
	return 1;
}

static void eusart_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				    void *cb_data)
{
	struct eusart_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_SILABS_EUSART_ASYNC
static void eusart_async_user_callback(struct eusart_data *data, struct uart_event *event)
{
	if (data->async_cb) {
		data->async_cb(data->uart_dev, event, data->async_user_data);
	}
}

static void eusart_async_timer_start(struct k_work_delayable *work, int32_t timeout)
{
	if (timeout != SYS_FOREVER_US && timeout != 0) {
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static void eusart_async_evt_rx_rdy(struct eusart_data *data)
{
	struct uart_event event = {
		.type = UART_RX_RDY,
		.data.rx.buf = data->dma_rx.buffer,
		.data.rx.len = data->dma_rx.counter - data->dma_rx.offset,
		.data.rx.offset = data->dma_rx.offset
	};

	data->dma_rx.offset = data->dma_rx.counter;

	if (event.data.rx.len > 0) {
		eusart_async_user_callback(data, &event);
	}
}

static void eusart_async_evt_tx_done(struct eusart_data *data)
{
	struct uart_event event = {
		.type = UART_TX_DONE,
		.data.tx.buf = data->dma_tx.buffer,
		.data.tx.len = data->dma_tx.counter
	};

	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	eusart_async_user_callback(data, &event);
}

static void eusart_async_evt_tx_abort(struct eusart_data *data)
{
	struct uart_event event = {
		.type = UART_TX_ABORTED,
		.data.tx.buf = data->dma_tx.buffer,
		.data.tx.len = data->dma_tx.counter
	};

	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	eusart_async_user_callback(data, &event);
}

static void eusart_async_evt_rx_err(struct eusart_data *data, int err_code)
{
	struct uart_event event = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = err_code,
		.data.rx_stop.data.len = data->dma_rx.counter,
		.data.rx_stop.data.offset = 0,
		.data.rx_stop.data.buf = data->dma_rx.buffer
	};

	eusart_async_user_callback(data, &event);
}

static void eusart_async_evt_rx_buf_release(struct eusart_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->dma_rx.buffer,
	};

	eusart_async_user_callback(data, &evt);
}

static void eusart_async_evt_rx_buf_request(struct eusart_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	eusart_async_user_callback(data, &evt);
}

static int eusart_async_callback_set(const struct device *dev, uart_callback_t callback,
				     void *user_data)
{
	struct eusart_data *data = dev->data;

	data->async_cb = callback;
	data->async_user_data = user_data;

	return 0;
}

static void eusart_dma_replace_buffer(const struct device *dev)
{
	struct eusart_data *data = dev->data;

	data->dma_rx.offset = 0;
	data->dma_rx.counter = 0;
	data->dma_rx.buffer = data->rx_next_buffer;
	data->dma_rx.buffer_length = data->rx_next_buffer_len;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	eusart_async_evt_rx_buf_request(data);
}

static void eusart_dma_rx_flush(struct eusart_data *data)
{
	struct dma_status stat;
	size_t rx_rcv_len;

	if (!dma_get_status(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &stat)) {
		rx_rcv_len = data->dma_rx.buffer_length - stat.pending_length;
		if (rx_rcv_len > data->dma_rx.offset) {
			data->dma_rx.counter = rx_rcv_len;
			eusart_async_evt_rx_rdy(data);
		}
	}
}

__maybe_unused static void eusart_dma_rx_cb(const struct device *dma_dev, void *user_data,
					    uint32_t channel, int status)
{
	const struct device *uart_dev = user_data;
	struct eusart_data *data = uart_dev->data;
	struct uart_event disabled_event = {
		.type = UART_RX_DISABLED
	};

	if (status < 0) {
		eusart_async_evt_rx_err(data, status);
		return;
	}

	k_work_cancel_delayable(&data->dma_rx.timeout_work);

	data->dma_rx.counter = data->dma_rx.buffer_length;

	eusart_async_evt_rx_rdy(data);

	if (data->rx_next_buffer) {
		eusart_async_evt_rx_buf_release(data);
		eusart_dma_replace_buffer(uart_dev);
	} else {
		dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
		data->dma_rx.enabled = false;
		eusart_async_evt_rx_buf_release(data);
		eusart_async_user_callback(data, &disabled_event);
	}
}

__maybe_unused static void eusart_dma_tx_cb(const struct device *dma_dev, void *user_data,
					    uint32_t channel, int status)
{
	const struct device *uart_dev = user_data;
	struct eusart_data *data = uart_dev->data;

	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	data->dma_tx.enabled = false;
}

static int eusart_async_tx(const struct device *dev, const uint8_t *tx_data, size_t buf_size,
			   int32_t timeout)
{
	const struct eusart_config *config = dev->config;
	struct eusart_data *data = dev->data;
	int ret;

	if (!data->dma_tx.dma_dev) {
		return -ENODEV;
	}

	if (data->dma_tx.buffer_length) {
		return -EBUSY;
	}

	data->dma_tx.buffer = (uint8_t *)tx_data;
	data->dma_tx.buffer_length = buf_size;
	data->dma_tx.timeout = timeout;

	data->dma_tx.blk_cfg.source_address = (uint32_t)data->dma_tx.buffer;
	data->dma_tx.blk_cfg.block_size = data->dma_tx.buffer_length;

	eusart_pm_lock_get(dev, EUSART_PM_LOCK_TX);

	EUSART_IntClear(config->eusart, EUSART_IF_TXC);
	EUSART_IntEnable(config->eusart, EUSART_IF_TXC);

	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &data->dma_tx.dma_cfg);
	if (ret) {
		LOG_ERR("dma tx config error!");
		return ret;
	}

	/* These 2 lines need to be call before dma_start otherwise uart and dma callback are
	 * called just after before the execution of these lines.
	 */
	data->dma_tx.enabled = true;

	eusart_async_timer_start(&data->dma_tx.timeout_work, data->dma_tx.timeout);

	ret = dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	if (ret) {
		LOG_ERR("UART err: TX DMA start failed!");
		data->dma_tx.enabled = false;
		k_work_cancel_delayable(&data->dma_tx.timeout_work);
		return ret;
	}

	return 0;
}

static int eusart_async_tx_abort(const struct device *dev)
{
	const struct eusart_config *config = dev->config;
	struct eusart_data *data = dev->data;
	size_t tx_buffer_length = data->dma_tx.buffer_length;
	struct dma_status stat;

	if (!tx_buffer_length) {
		return -EFAULT;
	}

	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);

	EUSART_IntDisable(config->eusart, EUSART_IF_TXC);
	EUSART_IntClear(config->eusart, EUSART_IF_TXC);
	eusart_pm_lock_put(dev, EUSART_PM_LOCK_TX);

	k_work_cancel_delayable(&data->dma_tx.timeout_work);

	if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = tx_buffer_length - stat.pending_length;
	}

	data->dma_tx.enabled = false;

	eusart_async_evt_tx_abort(data);

	return 0;
}

static int eusart_async_rx_enable(const struct device *dev, uint8_t *rx_buf, size_t buf_size,
				  int32_t timeout)
{
	const struct eusart_config *config = dev->config;
	struct eusart_data *data = dev->data;
	int ret;

	if (!data->dma_rx.dma_dev) {
		return -ENODEV;
	}

	if (data->dma_rx.enabled) {
		LOG_WRN("RX was already enabled");
		return -EBUSY;
	}

	data->dma_rx.offset = 0;
	data->dma_rx.buffer = rx_buf;
	data->dma_rx.buffer_length = buf_size;
	data->dma_rx.counter = 0;
	data->dma_rx.timeout = timeout;
	data->dma_rx.blk_cfg.block_size = buf_size;
	data->dma_rx.blk_cfg.dest_address = (uint32_t)data->dma_rx.buffer;

	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &data->dma_rx.dma_cfg);

	if (ret) {
		LOG_ERR("UART ERR: RX DMA config failed!");
		return -EINVAL;
	}

	if (dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel)) {
		LOG_ERR("UART ERR: RX DMA start failed!");
		return -EFAULT;
	}

	eusart_pm_lock_get(dev, EUSART_PM_LOCK_RX);
	EUSART_IntClear(config->eusart, EUSART_IF_RXOF | EUSART_IF_RXTO);
	EUSART_IntEnable(config->eusart, EUSART_IF_RXOF);
	EUSART_IntEnable(config->eusart, EUSART_IF_RXTO);

	data->dma_rx.enabled = true;

	eusart_async_evt_rx_buf_request(data);

	return ret;
}

static int eusart_async_rx_disable(const struct device *dev)
{
	const struct eusart_config *config = dev->config;
	EUSART_TypeDef *eusart = config->eusart;
	struct eusart_data *data = dev->data;
	struct uart_event disabled_event = {
		.type = UART_RX_DISABLED
	};

	if (!data->dma_rx.enabled) {
		return -EFAULT;
	}

	dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	EUSART_IntDisable(eusart, EUSART_IF_RXOF);
	EUSART_IntDisable(eusart, EUSART_IF_RXTO);
	EUSART_IntClear(eusart, EUSART_IF_RXOF | EUSART_IF_RXTO);
	eusart_pm_lock_put(dev, EUSART_PM_LOCK_RX);

	k_work_cancel_delayable(&data->dma_rx.timeout_work);

	eusart_dma_rx_flush(data);

	eusart_async_evt_rx_buf_release(data);

	if (data->rx_next_buffer) {
		struct uart_event rx_next_buf_release_evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = data->rx_next_buffer,
		};
		eusart_async_user_callback(data, &rx_next_buf_release_evt);
	}

	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	data->dma_rx.enabled = false;

	eusart_async_user_callback(data, &disabled_event);

	return 0;
}

static int eusart_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct eusart_data *data = dev->data;
	unsigned int key;
	int ret;

	key = irq_lock();

	if (data->rx_next_buffer) {
		return -EBUSY;
	} else if (!data->dma_rx.enabled) {
		return -EACCES;
	}

	data->rx_next_buffer = buf;
	data->rx_next_buffer_len = len;
	data->dma_rx.blk_cfg.dest_address = (uint32_t)buf;
	data->dma_rx.blk_cfg.block_size = len;

	irq_unlock(key);

	ret = silabs_ldma_append_block(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
				       &data->dma_rx.dma_cfg);
	if (ret) {
		LOG_ERR("UART ERR: RX DMA append failed!");
		return -EINVAL;
	}

	return ret;
}

static void eusart_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct eusart_dma_channel *rx_channel =
		CONTAINER_OF(dwork, struct eusart_dma_channel, timeout_work);
	struct eusart_data *data = CONTAINER_OF(rx_channel, struct eusart_data, dma_rx);

	eusart_dma_rx_flush(data);
}

static void eusart_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct eusart_dma_channel *tx_channel =
		CONTAINER_OF(dwork, struct eusart_dma_channel, timeout_work);
	struct eusart_data *data = CONTAINER_OF(tx_channel, struct eusart_data, dma_tx);
	const struct device *dev = data->uart_dev;

	eusart_async_tx_abort(dev);
}

static int eusart_async_init(const struct device *dev)
{
	const struct eusart_config *config = dev->config;
	EUSART_TypeDef *eusart = config->eusart;
	struct eusart_data *data = dev->data;

	data->uart_dev = dev;

	if (data->dma_rx.dma_dev) {
		if (!device_is_ready(data->dma_rx.dma_dev)) {
			return -ENODEV;
		}

		data->dma_rx.dma_channel = dma_request_channel(data->dma_rx.dma_dev, NULL);
	}

	if (data->dma_tx.dma_dev) {
		if (!device_is_ready(data->dma_tx.dma_dev)) {
			return -ENODEV;
		}

		data->dma_tx.dma_channel = dma_request_channel(data->dma_tx.dma_dev, NULL);
	}

	data->dma_rx.enabled = false;
	data->dma_tx.enabled = false;

	k_work_init_delayable(&data->dma_rx.timeout_work, eusart_async_rx_timeout);
	k_work_init_delayable(&data->dma_tx.timeout_work, eusart_async_tx_timeout);

	memset(&data->dma_rx.blk_cfg, 0, sizeof(data->dma_rx.blk_cfg));
	data->dma_rx.blk_cfg.source_address = (uintptr_t)&(eusart->RXDATA);
	data->dma_rx.blk_cfg.dest_address = 0;
	data->dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->dma_rx.dma_cfg.complete_callback_en = 1;
	data->dma_rx.dma_cfg.channel_priority = 3;
	data->dma_rx.dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;
	data->dma_rx.dma_cfg.user_data = (void *)dev;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	memset(&data->dma_tx.blk_cfg, 0, sizeof(data->dma_tx.blk_cfg));
	data->dma_tx.blk_cfg.dest_address = (uintptr_t)&(eusart->TXDATA);
	data->dma_tx.blk_cfg.source_address = 0;
	data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->dma_tx.dma_cfg.complete_callback_en = 1;
	data->dma_tx.dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
	data->dma_tx.dma_cfg.user_data = (void *)dev;

	return 0;
}
#endif /* CONFIG_UART_SILABS_EUSART_ASYNC */

static void eusart_isr(const struct device *dev)
{
	__maybe_unused struct eusart_data *data = dev->data;
#ifdef CONFIG_UART_SILABS_EUSART_ASYNC
	const struct eusart_config *config = dev->config;
	EUSART_TypeDef *eusart = config->eusart;
	uint32_t flags = EUSART_IntGet(eusart);
	struct dma_status stat;
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
#endif
#ifdef CONFIG_UART_SILABS_EUSART_ASYNC
	if (!data->dma_tx.dma_dev) {
		return;
	}
	if (flags & EUSART_IF_RXTO) {
		if (data->dma_rx.timeout == 0) {
			eusart_dma_rx_flush(data);
		} else {
			eusart_async_timer_start(&data->dma_rx.timeout_work, data->dma_rx.timeout);
		}

		EUSART_IntClear(eusart, EUSART_IF_RXTO);
	}

	if (flags & EUSART_IF_RXOF) {
		eusart_async_evt_rx_err(data, UART_ERROR_OVERRUN);

		eusart_async_rx_disable(dev);

		EUSART_IntClear(eusart, EUSART_IF_RXOF);
	}

	if (flags & EUSART_IF_TXC) {
		k_work_cancel_delayable(&data->dma_tx.timeout_work);

		if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
			data->dma_tx.counter = data->dma_tx.buffer_length - stat.pending_length;
		}

		if (data->dma_tx.counter == data->dma_tx.buffer_length) {
			EUSART_IntDisable(eusart, EUSART_IF_TXC);
			EUSART_IntClear(eusart, EUSART_IF_TXC);
			eusart_pm_lock_put(dev, EUSART_PM_LOCK_TX);
		}

		eusart_async_evt_tx_done(data);
	}
#endif /* CONFIG_UART_SILABS_EUSART_ASYNC */
}

static EUSART_Parity_TypeDef eusart_cfg2ll_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return eusartOddParity;
	case UART_CFG_PARITY_EVEN:
		return eusartEvenParity;
	case UART_CFG_PARITY_NONE:
	default:
		return eusartNoParity;
	}
}

static inline enum uart_config_parity eusart_ll2cfg_parity(EUSART_Parity_TypeDef parity)
{
	switch (parity) {
	case eusartOddParity:
		return UART_CFG_PARITY_ODD;
	case eusartEvenParity:
		return UART_CFG_PARITY_EVEN;
	case eusartNoParity:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static EUSART_Stopbits_TypeDef eusart_cfg2ll_stopbits(enum uart_config_stop_bits sb)
{
	switch (sb) {
	case UART_CFG_STOP_BITS_0_5:
		return eusartStopbits0p5;
	case UART_CFG_STOP_BITS_1:
		return eusartStopbits1;
	case UART_CFG_STOP_BITS_2:
		return eusartStopbits2;
	case UART_CFG_STOP_BITS_1_5:
		return eusartStopbits1p5;
	default:
		return eusartStopbits1;
	}
}

static inline enum uart_config_stop_bits eusart_ll2cfg_stopbits(EUSART_Stopbits_TypeDef sb)
{
	switch (sb) {
	case eusartStopbits0p5:
		return UART_CFG_STOP_BITS_0_5;
	case eusartStopbits1:
		return UART_CFG_STOP_BITS_1;
	case eusartStopbits1p5:
		return UART_CFG_STOP_BITS_1_5;
	case eusartStopbits2:
		return UART_CFG_STOP_BITS_2;
	default:
		return UART_CFG_STOP_BITS_1;
	}
}

static EUSART_Databits_TypeDef eusart_cfg2ll_databits(enum uart_config_data_bits db,
						      enum uart_config_parity p)
{
	switch (db) {
	case UART_CFG_DATA_BITS_7:
		if (p == UART_CFG_PARITY_NONE) {
			return eusartDataBits7;
		} else {
			return eusartDataBits8;
		}
	case UART_CFG_DATA_BITS_9:
		return eusartDataBits9;
	case UART_CFG_DATA_BITS_8:
	default:
		if (p == UART_CFG_PARITY_NONE) {
			return eusartDataBits8;
		} else {
			return eusartDataBits9;
		}
		return eusartDataBits8;
	}
}

static inline enum uart_config_data_bits eusart_ll2cfg_databits(EUSART_Databits_TypeDef db,
								EUSART_Parity_TypeDef p)
{
	switch (db) {
	case eusartDataBits7:
		if (p == eusartNoParity) {
			return UART_CFG_DATA_BITS_7;
		} else {
			return UART_CFG_DATA_BITS_6;
		}
	case eusartDataBits9:
		if (p == eusartNoParity) {
			return UART_CFG_DATA_BITS_9;
		} else {
			return UART_CFG_DATA_BITS_8;
		}
	case eusartDataBits8:
	default:
		if (p == eusartNoParity) {
			return UART_CFG_DATA_BITS_8;
		} else {
			return UART_CFG_DATA_BITS_7;
		}
	}
}

static EUSART_HwFlowControl_TypeDef eusart_cfg2ll_hwctrl(enum uart_config_flow_control fc)
{
	if (fc == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return eusartHwFlowControlCtsAndRts;
	}

	return eusartHwFlowControlNone;
}

static inline enum uart_config_flow_control eusart_ll2cfg_hwctrl(EUSART_HwFlowControl_TypeDef fc)
{
	if (fc == eusartHwFlowControlCtsAndRts) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_FLOW_CTRL_NONE;
}

static void eusart_configure_peripheral(const struct device *dev, bool enable)
{
	const struct eusart_config *config = dev->config;
	const struct eusart_data *data = dev->data;
	const struct uart_config *uart_cfg = &data->uart_cfg;
	EUSART_UartInit_TypeDef eusartInit = EUSART_UART_INIT_DEFAULT_HF;
	EUSART_AdvancedInit_TypeDef advancedSettings = EUSART_ADVANCED_INIT_DEFAULT;

	eusartInit.baudrate = uart_cfg->baudrate;
	eusartInit.parity = eusart_cfg2ll_parity(uart_cfg->parity);
	eusartInit.stopbits = eusart_cfg2ll_stopbits(uart_cfg->stop_bits);
	eusartInit.databits = eusart_cfg2ll_databits(uart_cfg->data_bits, uart_cfg->parity);
	advancedSettings.hwFlowControl = eusart_cfg2ll_hwctrl(uart_cfg->flow_ctrl);
	eusartInit.advancedSettings = &advancedSettings;
	eusartInit.enable = eusartDisable;

	EUSART_UartInitHf(config->eusart, &eusartInit);

#ifdef CONFIG_UART_SILABS_EUSART_ASYNC
	config->eusart->CFG1 |= EUSART_CFG1_RXTIMEOUT_ONEFRAME;
#endif

	if (enable) {
		EUSART_Enable(config->eusart, eusartEnable);
	}
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int eusart_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct eusart_config *config = dev->config;
	EUSART_TypeDef *eusart = config->eusart;
	struct eusart_data *data = dev->data;
	struct uart_config *uart_cfg = &data->uart_cfg;

#ifdef CONFIG_UART_SILABS_EUSART_ASYNC
	if (data->dma_rx.enabled || data->dma_tx.enabled) {
		return -EBUSY;
	}
#endif

	if (cfg->parity == UART_CFG_PARITY_MARK || cfg->parity == UART_CFG_PARITY_SPACE) {
		return -ENOSYS;
	}

	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_DTR_DSR ||
	    cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485) {
		return -ENOSYS;
	}

	*uart_cfg = *cfg;

	EUSART_Enable(eusart, eusartDisable);
	eusart_configure_peripheral(dev, true);

	return 0;
};

static int eusart_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct eusart_data *data = dev->data;
	struct uart_config *uart_cfg = &data->uart_cfg;

	*cfg = *uart_cfg;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int eusart_init(const struct device *dev)
{
	int err;
	const struct eusart_config *config = dev->config;

	/* The peripheral and gpio clock are already enabled from soc and gpio driver */
	/* Enable EUSART clock */
	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	eusart_configure_peripheral(dev, false);

	config->irq_config_func(dev);

#ifdef CONFIG_UART_SILABS_EUSART_ASYNC
	err = eusart_async_init(dev);
	if (err < 0) {
		return err;
	}
#endif

	return pm_device_driver_init(dev, eusart_pm_action);
}

static int eusart_pm_action(const struct device *dev, enum pm_device_action action)
{
	__maybe_unused struct eusart_data *data = dev->data;
	const struct eusart_config *config = dev->config;
	int err;

	if (action == PM_DEVICE_ACTION_RESUME) {
		err = clock_control_on(config->clock_dev,
				       (clock_control_subsys_t)&config->clock_cfg);
		if (err < 0 && err != -EALREADY) {
			return err;
		}

		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			return err;
		}

		EUSART_Enable(config->eusart, eusartEnable);
	} else if (IS_ENABLED(CONFIG_PM_DEVICE) && (action == PM_DEVICE_ACTION_SUSPEND)) {
#ifdef CONFIG_UART_SILABS_EUSART_ASYNC
		/* Entering suspend requires there to be no active asynchronous calls. */
		__ASSERT_NO_MSG(!data->dma_rx.enabled);
		__ASSERT_NO_MSG(!data->dma_tx.enabled);
#endif
		EUSART_Enable(config->eusart, eusartDisable);

		err = clock_control_off(config->clock_dev,
					(clock_control_subsys_t)&config->clock_cfg);
		if (err < 0) {
			return err;
		}

		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			return err;
		}

	} else {
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(uart, eusart_driver_api) = {
	.poll_in = eusart_poll_in,
	.poll_out = eusart_poll_out,
	.err_check = eusart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = eusart_configure,
	.config_get = eusart_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = eusart_fifo_fill,
	.fifo_read = eusart_fifo_read,
	.irq_tx_enable = eusart_irq_tx_enable,
	.irq_tx_disable = eusart_irq_tx_disable,
	.irq_tx_complete = eusart_irq_tx_complete,
	.irq_tx_ready = eusart_irq_tx_ready,
	.irq_rx_enable = eusart_irq_rx_enable,
	.irq_rx_disable = eusart_irq_rx_disable,
	.irq_rx_ready = eusart_irq_rx_ready,
	.irq_err_enable = eusart_irq_err_enable,
	.irq_err_disable = eusart_irq_err_disable,
	.irq_is_pending = eusart_irq_is_pending,
	.irq_update = eusart_irq_update,
	.irq_callback_set = eusart_irq_callback_set,
#endif
#ifdef CONFIG_UART_SILABS_EUSART_ASYNC
	.callback_set = eusart_async_callback_set,
	.tx = eusart_async_tx,
	.tx_abort = eusart_async_tx_abort,
	.rx_enable = eusart_async_rx_enable,
	.rx_disable = eusart_async_rx_disable,
	.rx_buf_rsp = eusart_async_rx_buf_rsp,
#endif
};

#ifdef CONFIG_UART_SILABS_EUSART_ASYNC

#define EUSART_DMA_CHANNEL_INIT(index, dir)                                                        \
	.dma_##dir = {                                                                             \
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                   \
		.dma_cfg = {                                                                       \
			.dma_slot = SILABS_LDMA_REQSEL_TO_SLOT(                                    \
				    DT_INST_DMAS_CELL_BY_NAME(index, dir, slot)),                  \
			.source_data_size = 1,                                                     \
			.dest_data_size = 1,                                                       \
			.source_burst_length = 1,                                                  \
			.dest_burst_length = 1,                                                    \
			.dma_callback = eusart_dma_##dir##_cb,                                     \
		}                                                                                  \
	},
#define EUSART_DMA_CHANNEL(index, dir)                                                             \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, dmas),                                            \
		 (EUSART_DMA_CHANNEL_INIT(index, dir)), ())
#else
#define EUSART_DMA_CHANNEL(index, dir)
#endif


#define SILABS_EUSART_IRQ_HANDLER_FUNC(idx) .irq_config_func = eusart_config_func_##idx,
#define SILABS_EUSART_IRQ_HANDLER(idx)                                                             \
	static void eusart_config_func_##idx(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, rx, irq),                                     \
			    DT_INST_IRQ_BY_NAME(idx, rx, priority), eusart_isr,                    \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, tx, irq),                                     \
			    DT_INST_IRQ_BY_NAME(idx, tx, priority), eusart_isr,                    \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, rx, irq));                                     \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, tx, irq));                                     \
	}

#define SILABS_EUSART_INIT(idx)                                                                    \
	SILABS_EUSART_IRQ_HANDLER(idx);                                                            \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	PM_DEVICE_DT_INST_DEFINE(idx, eusart_pm_action);                                           \
                                                                                                   \
	static const struct eusart_config eusart_cfg_##idx = {                                     \
		.eusart = (EUSART_TypeDef *)DT_INST_REG_ADDR(idx),                                 \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),                                        \
		SILABS_EUSART_IRQ_HANDLER_FUNC(idx)                                                \
	};                                                                                         \
                                                                                                   \
	static struct eusart_data eusart_data_##idx = {                                            \
		.uart_cfg = {                                                                      \
			.baudrate  = DT_INST_PROP(idx, current_speed),                             \
			.parity    = DT_INST_ENUM_IDX(idx, parity),                                \
			.stop_bits = DT_INST_ENUM_IDX(idx, stop_bits),                             \
			.data_bits = DT_INST_ENUM_IDX(idx, data_bits),                             \
			.flow_ctrl = DT_INST_PROP(idx, hw_flow_control)                            \
						? UART_CFG_FLOW_CTRL_RTS_CTS                       \
						: UART_CFG_FLOW_CTRL_NONE,                         \
		},                                                                                 \
		EUSART_DMA_CHANNEL(idx, rx)                                                        \
		EUSART_DMA_CHANNEL(idx, tx)                                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, eusart_init, PM_DEVICE_DT_INST_GET(idx), &eusart_data_##idx,    \
			      &eusart_cfg_##idx, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,        \
			      &eusart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SILABS_EUSART_INIT)
