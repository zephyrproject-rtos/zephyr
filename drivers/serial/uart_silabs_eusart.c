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
#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_silabs_ldma.h>
#endif

LOG_MODULE_REGISTER(uart_silabs_eusart, CONFIG_UART_LOG_LEVEL);

#ifdef CONFIG_UART_ASYNC_API
struct uart_dma_channel {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_block_config blk_cfg;
	struct dma_config dma_cfg;
	uint8_t priority;
	uint8_t *buffer;
	size_t buffer_length;
	volatile size_t counter;
	size_t offset;
	struct k_work_delayable timeout_work;
	int32_t timeout;
	bool enabled;
};
#endif
struct uart_silabs_eusart_config {
	EUSART_TypeDef *eusart;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	void (*irq_config_func)(const struct device *dev);
#endif
};

enum uart_silabs_eusart_pm_lock {
	UART_SILABS_EUSART_PM_LOCK_TX,
	UART_SILABS_EUSART_PM_LOCK_TX_POLL,
	UART_SILABS_EUSART_PM_LOCK_RX,
	UART_SILABS_EUSART_PM_LOCK_COUNT,
};

struct uart_silabs_eusart_data {
	struct uart_config *uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
#ifdef CONFIG_UART_ASYNC_API
	const struct device *uart_dev;
	uart_callback_t async_cb;
	void *async_user_data;
	struct uart_dma_channel dma_rx;
	struct uart_dma_channel dma_tx;
	uint8_t *rx_next_buffer;
	size_t rx_next_buffer_len;
#endif
#ifdef CONFIG_PM
	ATOMIC_DEFINE(pm_lock, UART_SILABS_EUSART_PM_LOCK_COUNT);
#endif
};

static int uart_silabs_eusart_pm_action(const struct device *dev, enum pm_device_action action);

/**
 * @brief Get PM lock on low power states
 *
 * @param dev  UART device struct
 * @param lock UART PM lock type
 *
 * @return true if lock was taken, false otherwise
 */
static bool uart_silabs_eusart_pm_lock_get(const struct device *dev,
					   enum uart_silabs_eusart_pm_lock lock)
{
#ifdef CONFIG_PM
	struct uart_silabs_eusart_data *data = dev->data;
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
static bool uart_silabs_eusart_pm_lock_put(const struct device *dev,
					   enum uart_silabs_eusart_pm_lock lock)
{
#ifdef CONFIG_PM
	struct uart_silabs_eusart_data *data = dev->data;
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

static int uart_silabs_eusart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_silabs_eusart_config *config = dev->config;

	if (EUSART_StatusGet(config->eusart) & EUSART_STATUS_RXFL) {
		*c = EUSART_Rx(config->eusart);
		return 0;
	}

	return -1;
}

static void uart_silabs_eusart_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_silabs_eusart_config *config = dev->config;

	/* EUSART_Tx function already waits for the transmit buffer being empty
	 * and waits for the bus to be free to transmit.
	 */
	EUSART_Tx(config->eusart, c);
}

static int uart_silabs_eusart_err_check(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;
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
static int uart_silabs_eusart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_silabs_eusart_config *config = dev->config;
	int i = 0;

	while ((i < len) && (EUSART_StatusGet(config->eusart) & EUSART_STATUS_TXFL)) {

		config->eusart->TXDATA = (uint32_t)tx_data[i++];
	}

	if (!(EUSART_StatusGet(config->eusart) & EUSART_STATUS_TXFL)) {
		EUSART_IntClear(config->eusart, EUSART_IF_TXFL);
	}

	return i;
}

static int uart_silabs_eusart_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	const struct uart_silabs_eusart_config *config = dev->config;
	int i = 0;

	while ((i < len) && (EUSART_StatusGet(config->eusart) & EUSART_STATUS_RXFL)) {
		rx_data[i++] = (uint8_t)config->eusart->RXDATA;
	}

	if (!(EUSART_StatusGet(config->eusart) & EUSART_STATUS_RXFL)) {
		EUSART_IntClear(config->eusart, EUSART_IF_RXFL);
	}

	return i;
}

static void uart_silabs_eusart_irq_tx_enable(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;

	(void)uart_silabs_eusart_pm_lock_get(dev, UART_SILABS_EUSART_PM_LOCK_TX);
	EUSART_IntClear(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
	EUSART_IntEnable(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
}

static void uart_silabs_eusart_irq_tx_disable(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;

	EUSART_IntDisable(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
	EUSART_IntClear(config->eusart, EUSART_IEN_TXFL | EUSART_IEN_TXC);
	(void)uart_silabs_eusart_pm_lock_put(dev, UART_SILABS_EUSART_PM_LOCK_TX);
}

static int uart_silabs_eusart_irq_tx_complete(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;
	uint32_t flags = EUSART_IntGet(config->eusart);

	EUSART_IntClear(config->eusart, EUSART_IF_TXC);

	return !!(flags & EUSART_IF_TXC);
}

static int uart_silabs_eusart_irq_tx_ready(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;

	return (config->eusart->IEN & EUSART_IEN_TXFL) &&
	       (EUSART_IntGet(config->eusart) & EUSART_IF_TXFL);
}

static void uart_silabs_eusart_irq_rx_enable(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;

	(void)uart_silabs_eusart_pm_lock_get(dev, UART_SILABS_EUSART_PM_LOCK_RX);
	EUSART_IntClear(config->eusart, EUSART_IEN_RXFL);
	EUSART_IntEnable(config->eusart, EUSART_IEN_RXFL);
}

static void uart_silabs_eusart_irq_rx_disable(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;

	EUSART_IntDisable(config->eusart, EUSART_IEN_RXFL);
	EUSART_IntClear(config->eusart, EUSART_IEN_RXFL);
	(void)uart_silabs_eusart_pm_lock_put(dev, UART_SILABS_EUSART_PM_LOCK_RX);
}

static int uart_silabs_eusart_irq_rx_ready(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;

	return (config->eusart->IEN & EUSART_IEN_RXFL) &&
	       (EUSART_IntGet(config->eusart) & EUSART_IF_RXFL);
}

static void uart_silabs_eusart_irq_err_enable(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;

	EUSART_IntClear(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
	EUSART_IntEnable(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
}

static void uart_silabs_eusart_irq_err_disable(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;

	EUSART_IntDisable(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
	EUSART_IntClear(config->eusart, EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
}

static int uart_silabs_eusart_irq_is_pending(const struct device *dev)
{
	return uart_silabs_eusart_irq_tx_ready(dev) || uart_silabs_eusart_irq_rx_ready(dev);
}

static int uart_silabs_eusart_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_silabs_eusart_irq_callback_set(const struct device *dev,
						uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct uart_silabs_eusart_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
static inline void async_user_callback(struct uart_silabs_eusart_data *data,
				       struct uart_event *event)
{
	if (data->async_cb) {
		data->async_cb(data->uart_dev, event, data->async_user_data);
	}
}

static inline void async_timer_start(struct k_work_delayable *work,
	int32_t timeout)
{
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static inline void async_evt_rx_rdy(struct uart_silabs_eusart_data *data)
{
	struct uart_event event = {
		.type = UART_RX_RDY,
		.data.rx.buf = data->dma_rx.buffer,
		.data.rx.len = data->dma_rx.counter - data->dma_rx.offset,
		.data.rx.offset = data->dma_rx.offset
	};

	data->dma_rx.offset = data->dma_rx.counter;

	if (event.data.rx.len > 0) {
		async_user_callback(data, &event);
	}
}

static inline void async_evt_tx_done(struct uart_silabs_eusart_data *data)
{
	struct uart_event event = {
		.type = UART_TX_DONE,
		.data.tx.buf = data->dma_tx.buffer,
		.data.tx.len = data->dma_tx.counter
	};

	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	async_user_callback(data, &event);
}

static inline void async_evt_tx_abort(struct uart_silabs_eusart_data *data)
{
	struct uart_event event = {
		.type = UART_TX_ABORTED,
		.data.tx.buf = data->dma_tx.buffer,
		.data.tx.len = data->dma_tx.counter
	};

	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	async_user_callback(data, &event);
}

static inline void async_evt_rx_err(struct uart_silabs_eusart_data *data, int err_code)
{
	struct uart_event event = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = err_code,
		.data.rx_stop.data.len = data->dma_rx.counter,
		.data.rx_stop.data.offset = 0,
		.data.rx_stop.data.buf = data->dma_rx.buffer
	};

	async_user_callback(data, &event);
}

static inline void async_evt_rx_buf_release(struct uart_silabs_eusart_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->dma_rx.buffer,
	};

	async_user_callback(data, &evt);
}

static inline void async_evt_rx_buf_request(struct uart_silabs_eusart_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(data, &evt);
}

static int uart_silabs_eusart_async_callback_set(const struct device *dev, uart_callback_t callback,
						 void *user_data)
{
	struct uart_silabs_eusart_data *data = dev->data;

	data->async_cb = callback;
	data->async_user_data = user_data;

	return 0;
}

static void uart_silabs_eusart_dma_replace_buffer(const struct device *dev)
{
	struct uart_silabs_eusart_data *data = dev->data;

	data->dma_rx.offset = 0;
	data->dma_rx.counter = 0;
	data->dma_rx.buffer = data->rx_next_buffer;
	data->dma_rx.buffer_length = data->rx_next_buffer_len;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	async_evt_rx_buf_request(data);
}

static void uart_silabs_eusart_dma_rx_flush(struct uart_silabs_eusart_data *data)
{
	struct dma_status stat;
	size_t rx_rcv_len;

	if (!dma_get_status(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &stat)) {
		rx_rcv_len = data->dma_rx.buffer_length - stat.pending_length;
		if (rx_rcv_len > data->dma_rx.offset) {
			data->dma_rx.counter = rx_rcv_len;
			async_evt_rx_rdy(data);
		}
	}
}

void uart_silabs_eusart_dma_rx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
				  int status)
{
	const struct device *uart_dev = user_data;
	struct uart_silabs_eusart_data *data = uart_dev->data;
	struct uart_event disabled_event = {.type = UART_RX_DISABLED};

	if (status < 0) {
		async_evt_rx_err(data, status);
		return;
	}

	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);

	data->dma_rx.counter = data->dma_rx.buffer_length;

	async_evt_rx_rdy(data);

	if (data->rx_next_buffer) {
		async_evt_rx_buf_release(data);
		uart_silabs_eusart_dma_replace_buffer(uart_dev);
	} else {
		dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
		data->dma_rx.enabled = false;
		async_evt_rx_buf_release(data);
		async_user_callback(data, &disabled_event);
	}
}

void uart_silabs_eusart_dma_tx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
				  int status)
{
	const struct device *uart_dev = user_data;
	struct uart_silabs_eusart_data *data = uart_dev->data;

	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	data->dma_tx.enabled = false;
}

static int uart_silabs_eusart_async_tx(const struct device *dev, const uint8_t *tx_data,
				       size_t buf_size, int32_t timeout)
{
	const struct uart_silabs_eusart_config *config = dev->config;
	struct uart_silabs_eusart_data *data = dev->data;
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

	(void)uart_silabs_eusart_pm_lock_get(dev, UART_SILABS_EUSART_PM_LOCK_TX);

	EUSART_IntClear(config->eusart, EUSART_IF_TXC);
	EUSART_IntEnable(config->eusart, EUSART_IF_TXC);

	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &data->dma_tx.dma_cfg);
	if (ret) {
		LOG_ERR("dma tx config error!");
		return ret;
	}

	ret = dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	if (ret) {
		LOG_ERR("UART err: TX DMA start failed!");
		return ret;
	}

	async_timer_start(&data->dma_tx.timeout_work, data->dma_tx.timeout);

	data->dma_tx.enabled = true;

	return 0;
}

static int uart_silabs_eusart_async_tx_abort(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;
	struct uart_silabs_eusart_data *data = dev->data;
	size_t tx_buffer_length = data->dma_tx.buffer_length;
	struct dma_status stat;

	if (!tx_buffer_length) {
		return -EFAULT;
	}

	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);

	EUSART_IntDisable(config->eusart, EUSART_IF_TXC);
	EUSART_IntClear(config->eusart, EUSART_IF_TXC);
	(void)uart_silabs_eusart_pm_lock_put(dev, UART_SILABS_EUSART_PM_LOCK_TX);

	(void)k_work_cancel_delayable(&data->dma_tx.timeout_work);

	if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = tx_buffer_length - stat.pending_length;
	}

	data->dma_tx.enabled = false;

	async_evt_tx_abort(data);

	return 0;
}

static int uart_silabs_eusart_async_rx_enable(const struct device *dev, uint8_t *rx_buf,
					      size_t buf_size, int32_t timeout)
{
	const struct uart_silabs_eusart_config *config = dev->config;
	struct uart_silabs_eusart_data *data = dev->data;
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

	(void)uart_silabs_eusart_pm_lock_get(dev, UART_SILABS_EUSART_PM_LOCK_RX);
	EUSART_IntClear(config->eusart, EUSART_IF_RXOF | EUSART_IF_RXTO);
	EUSART_IntEnable(config->eusart, EUSART_IF_RXOF);
	EUSART_IntEnable(config->eusart, EUSART_IF_RXTO);

	data->dma_rx.enabled = true;

	async_evt_rx_buf_request(data);

	return ret;
}

static int uart_silabs_eusart_async_rx_disable(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;
	EUSART_TypeDef *eusart = config->eusart;
	struct uart_silabs_eusart_data *data = dev->data;
	struct uart_event disabled_event = {.type = UART_RX_DISABLED};

	if (!data->dma_rx.enabled) {
		return -EFAULT;
	}

	dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	EUSART_IntDisable(eusart, EUSART_IF_RXOF);
	EUSART_IntDisable(eusart, EUSART_IF_RXTO);
	EUSART_IntClear(eusart, EUSART_IF_RXOF | EUSART_IF_RXTO);
	(void)uart_silabs_eusart_pm_lock_put(dev, UART_SILABS_EUSART_PM_LOCK_RX);

	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);

	uart_silabs_eusart_dma_rx_flush(data);

	async_evt_rx_buf_release(data);

	if (data->rx_next_buffer) {
		struct uart_event rx_next_buf_release_evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = data->rx_next_buffer,
		};
		async_user_callback(data, &rx_next_buf_release_evt);
	}

	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	data->dma_rx.enabled = false;

	async_user_callback(data, &disabled_event);

	return 0;
}

static int uart_silabs_eusart_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_silabs_eusart_data *data = dev->data;
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

static void uart_silabs_eusart_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_channel *rx_channel = CONTAINER_OF(dwork,
			struct uart_dma_channel, timeout_work);
	struct uart_silabs_eusart_data *data = CONTAINER_OF(rx_channel,
			struct uart_silabs_eusart_data, dma_rx);

	uart_silabs_eusart_dma_rx_flush(data);
}

static void uart_silabs_eusart_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_channel *tx_channel = CONTAINER_OF(dwork,
			struct uart_dma_channel, timeout_work);
	struct uart_silabs_eusart_data *data = CONTAINER_OF(tx_channel,
			struct uart_silabs_eusart_data, dma_tx);
	const struct device *dev = data->uart_dev;

	uart_silabs_eusart_async_tx_abort(dev);
}

static int uart_silabs_eusart_async_init(const struct device *dev)
{
	const struct uart_silabs_eusart_config *config = dev->config;
	EUSART_TypeDef *eusart = config->eusart;
	struct uart_silabs_eusart_data *data = dev->data;

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

	k_work_init_delayable(&data->dma_rx.timeout_work,
		uart_silabs_eusart_async_rx_timeout);
	k_work_init_delayable(&data->dma_tx.timeout_work,
		uart_silabs_eusart_async_tx_timeout);

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
#endif /* CONFIG_UART_ASYNC_API */

static void uart_silabs_eusart_isr(const struct device *dev)
{
	struct uart_silabs_eusart_data *data = dev->data;
	const struct uart_silabs_eusart_config *config = dev->config;
	EUSART_TypeDef *eusart = config->eusart;
	uint32_t flags = EUSART_IntGet(eusart);
#ifdef CONFIG_UART_ASYNC_API
	struct dma_status stat;
#endif
	if (flags & EUSART_IF_TXC) {
		if (uart_silabs_eusart_pm_lock_put(dev, UART_SILABS_EUSART_PM_LOCK_TX_POLL)) {
			EUSART_IntDisable(eusart, EUSART_IEN_TXC);
			EUSART_IntClear(eusart, EUSART_IF_TXC);
		}
	}
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
#endif
#ifdef CONFIG_UART_ASYNC_API
	if (flags & EUSART_IF_RXTO) {

		if (data->dma_rx.timeout == 0) {
			uart_silabs_eusart_dma_rx_flush(data);
		} else {
			async_timer_start(&data->dma_rx.timeout_work, data->dma_rx.timeout);
		}

		EUSART_IntClear(eusart, EUSART_IF_RXTO);
	}
	if (flags & EUSART_IF_RXOF) {
		async_evt_rx_err(data, UART_ERROR_OVERRUN);

		uart_silabs_eusart_async_rx_disable(dev);

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
			(void)uart_silabs_eusart_pm_lock_put(dev, UART_SILABS_EUSART_PM_LOCK_TX);
		}

		async_evt_tx_done(data);
	}
#endif /* CONFIG_UART_ASYNC_API */
}

static inline EUSART_Parity_TypeDef uart_silabs_eusart_cfg2ll_parity(enum uart_config_parity parity)
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

static inline enum uart_config_parity uart_silabs_eusart_ll2cfg_parity(EUSART_Parity_TypeDef parity)
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

static inline EUSART_Stopbits_TypeDef
uart_silabs_eusart_cfg2ll_stopbits(enum uart_config_stop_bits sb)
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

static inline enum uart_config_stop_bits
uart_silabs_eusart_ll2cfg_stopbits(EUSART_Stopbits_TypeDef sb)
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

static inline EUSART_Databits_TypeDef
uart_silabs_eusart_cfg2ll_databits(enum uart_config_data_bits db, enum uart_config_parity p)
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

static inline enum uart_config_data_bits
uart_silabs_eusart_ll2cfg_databits(EUSART_Databits_TypeDef db, EUSART_Parity_TypeDef p)
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

static inline EUSART_HwFlowControl_TypeDef
uart_silabs_eusart_cfg2ll_hwctrl(enum uart_config_flow_control fc)
{
	if (fc == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return eusartHwFlowControlCtsAndRts;
	}

	return eusartHwFlowControlNone;
}

static inline enum uart_config_flow_control
uart_silabs_eusart_ll2cfg_hwctrl(EUSART_HwFlowControl_TypeDef fc)
{
	if (fc == eusartHwFlowControlCtsAndRts) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_FLOW_CTRL_NONE;
}

static void uart_silabs_eusart_configure_peripheral(const struct device *dev, bool enable)
{
	const struct uart_silabs_eusart_config *config = dev->config;
	const struct uart_silabs_eusart_data *data = dev->data;
	EUSART_UartInit_TypeDef eusartInit = EUSART_UART_INIT_DEFAULT_HF;
	EUSART_AdvancedInit_TypeDef advancedSettings = EUSART_ADVANCED_INIT_DEFAULT;

	eusartInit.baudrate = data->uart_cfg->baudrate;
	eusartInit.parity = uart_silabs_eusart_cfg2ll_parity(data->uart_cfg->parity);
	eusartInit.stopbits = uart_silabs_eusart_cfg2ll_stopbits(data->uart_cfg->stop_bits);
	eusartInit.databits = uart_silabs_eusart_cfg2ll_databits(data->uart_cfg->data_bits,
							 data->uart_cfg->parity);
	advancedSettings.hwFlowControl =
		uart_silabs_eusart_cfg2ll_hwctrl(data->uart_cfg->flow_ctrl);
	eusartInit.advancedSettings = &advancedSettings;
	eusartInit.enable = enable ? eusartEnable : eusartDisable;

	EUSART_UartInitHf(config->eusart, &eusartInit);
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_silabs_eusart_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_silabs_eusart_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	struct uart_silabs_eusart_data *data = dev->data;

#ifdef CONFIG_UART_ASYNC_API
	if (data->dma_rx.enabled || data->dma_tx.enabled) {
		return -EBUSY;
	}
#endif

	if ((cfg->parity == UART_CFG_PARITY_MARK) || (cfg->parity == UART_CFG_PARITY_SPACE)) {
		return -ENOSYS;
	}

	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_DTR_DSR ||
	    cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485) {
		return -ENOSYS;
	}

	*data->uart_cfg = *cfg;

	EUSART_Enable(base, eusartDisable);
	uart_silabs_eusart_configure_peripheral(dev, false);

#ifdef CONFIG_UART_ASYNC_API
	config->eusart->CFG1 |= EUSART_CFG1_RXTIMEOUT_ONEFRAME;
#endif
	EUSART_Enable(base, eusartEnable);

	return 0;
};

static int uart_silabs_eusart_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_silabs_eusart_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;

	cfg->baudrate = uart_cfg->baudrate;
	cfg->parity = uart_cfg->parity;
	cfg->stop_bits = uart_cfg->stop_bits;
	cfg->data_bits = uart_cfg->data_bits;
	cfg->flow_ctrl = uart_cfg->flow_ctrl;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_silabs_eusart_init(const struct device *dev)
{
	int err;
	const struct uart_silabs_eusart_config *config = dev->config;

	/* The peripheral and gpio clock are already enabled from soc and gpio driver */
	/* Enable EUSART clock */
	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0) {
		return err;
	}

	uart_silabs_eusart_configure_peripheral(dev, false);

#ifdef CONFIG_UART_ASYNC_API
	config->eusart->CFG1 |= EUSART_CFG1_RXTIMEOUT_SIXFRAMES;
#endif

	config->irq_config_func(dev);

#ifdef CONFIG_UART_ASYNC_API
	err = uart_silabs_eusart_async_init(dev);
	if (err < 0) {
		return err;
	}
#endif

	return pm_device_driver_init(dev, uart_silabs_eusart_pm_action);
}

static int uart_silabs_eusart_pm_action(const struct device *dev, enum pm_device_action action)
{
	__maybe_unused struct uart_silabs_eusart_data *data = dev->data;
	const struct uart_silabs_eusart_config *config = dev->config;
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
#ifdef CONFIG_UART_ASYNC_API
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

static DEVICE_API(uart, uart_silabs_eusart_driver_api) = {
	.poll_in = uart_silabs_eusart_poll_in,
	.poll_out = uart_silabs_eusart_poll_out,
	.err_check = uart_silabs_eusart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_silabs_eusart_configure,
	.config_get = uart_silabs_eusart_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_silabs_eusart_fifo_fill,
	.fifo_read = uart_silabs_eusart_fifo_read,
	.irq_tx_enable = uart_silabs_eusart_irq_tx_enable,
	.irq_tx_disable = uart_silabs_eusart_irq_tx_disable,
	.irq_tx_complete = uart_silabs_eusart_irq_tx_complete,
	.irq_tx_ready = uart_silabs_eusart_irq_tx_ready,
	.irq_rx_enable = uart_silabs_eusart_irq_rx_enable,
	.irq_rx_disable = uart_silabs_eusart_irq_rx_disable,
	.irq_rx_ready = uart_silabs_eusart_irq_rx_ready,
	.irq_err_enable = uart_silabs_eusart_irq_err_enable,
	.irq_err_disable = uart_silabs_eusart_irq_err_disable,
	.irq_is_pending = uart_silabs_eusart_irq_is_pending,
	.irq_update = uart_silabs_eusart_irq_update,
	.irq_callback_set = uart_silabs_eusart_irq_callback_set,
#endif
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_silabs_eusart_async_callback_set,
	.tx = uart_silabs_eusart_async_tx,
	.tx_abort = uart_silabs_eusart_async_tx_abort,
	.rx_enable = uart_silabs_eusart_async_rx_enable,
	.rx_disable = uart_silabs_eusart_async_rx_disable,
	.rx_buf_rsp = uart_silabs_eusart_async_rx_buf_rsp,
#endif
};

#ifdef CONFIG_UART_ASYNC_API

#define UART_DMA_CHANNEL_INIT(index, dir)                                                          \
	.dma_##dir = {                                                                             \
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),                   \
		.dma_cfg = {                                                                       \
			.dma_slot = SILABS_LDMA_REQSEL_TO_SLOT(                                    \
				    DT_INST_DMAS_CELL_BY_NAME(index, dir, slot)),                  \
			.source_data_size = 1,                                                     \
			.dest_data_size = 1,                                                       \
			.source_burst_length = 1,                                                  \
			.dest_burst_length = 1,                                                    \
			.dma_callback = uart_silabs_eusart_dma_##dir##_cb,                         \
		}                                                                                  \
	},
#define UART_DMA_CHANNEL(index, dir)                                                               \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, dmas),                                            \
		 (UART_DMA_CHANNEL_INIT(index, dir)), ())
#else

#define UART_DMA_CHANNEL(index, dir)

#endif


#define SILABS_EUSART_IRQ_HANDLER_FUNC(idx) .irq_config_func = uart_silabs_eusart_config_func_##idx,
#define SILABS_EUSART_IRQ_HANDLER(idx)                                                             \
	static void uart_silabs_eusart_config_func_##idx(const struct device *dev)                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, rx, irq),                                     \
			    DT_INST_IRQ_BY_NAME(idx, rx, priority), uart_silabs_eusart_isr,        \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, tx, irq),                                     \
			    DT_INST_IRQ_BY_NAME(idx, tx, priority), uart_silabs_eusart_isr,        \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, rx, irq));                                     \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, tx, irq));                                     \
	}

#define SILABS_EUSART_INIT(idx)                                                                    \
	SILABS_EUSART_IRQ_HANDLER(idx);                                                            \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	PM_DEVICE_DT_INST_DEFINE(idx, uart_silabs_eusart_pm_action);                               \
                                                                                                   \
	static struct uart_config uart_cfg_##idx = {                                               \
		.baudrate = DT_INST_PROP(idx, current_speed),                                      \
		.parity = DT_INST_ENUM_IDX(idx, parity),                                           \
		.stop_bits = DT_INST_ENUM_IDX(idx, stop_bits),                                     \
		.data_bits = DT_INST_ENUM_IDX(idx, data_bits),                                     \
		.flow_ctrl = DT_INST_PROP(idx, hw_flow_control) ? UART_CFG_FLOW_CTRL_RTS_CTS       \
								: UART_CFG_FLOW_CTRL_NONE,         \
	};                                                                                         \
                                                                                                   \
	static const struct uart_silabs_eusart_config uart_silabs_eusart_cfg_##idx = {             \
		.eusart = (EUSART_TypeDef *)DT_INST_REG_ADDR(idx),                                 \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),                                        \
		SILABS_EUSART_IRQ_HANDLER_FUNC(idx)                                                \
	};                                                                                         \
                                                                                                   \
	static struct uart_silabs_eusart_data uart_silabs_eusart_data_##idx = {                    \
		.uart_cfg = &uart_cfg_##idx,                                                       \
		UART_DMA_CHANNEL(idx, rx)                                                          \
		UART_DMA_CHANNEL(idx, tx)                                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, uart_silabs_eusart_init, PM_DEVICE_DT_INST_GET(idx),            \
			      &uart_silabs_eusart_data_##idx, &uart_silabs_eusart_cfg_##idx,       \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,                           \
			      &uart_silabs_eusart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SILABS_EUSART_INIT)
