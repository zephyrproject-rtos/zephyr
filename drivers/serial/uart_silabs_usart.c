/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_usart_uart

#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <em_usart.h>
#ifdef CONFIG_UART_SILABS_USART_ASYNC
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_silabs_ldma.h>
#endif

LOG_MODULE_REGISTER(uart_silabs_usart, CONFIG_UART_LOG_LEVEL);

#define SILABS_USART_TIMER_COMPARE_VALUE 0xff
#define SILABS_USART_TIMEOUT_TO_TIMERCOUNTER(timeout, baudrate)                                    \
	((timeout * NSEC_PER_USEC) / ((NSEC_PER_SEC / baudrate) * SILABS_USART_TIMER_COMPARE_VALUE))

#ifdef CONFIG_UART_SILABS_USART_ASYNC
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
	int32_t timeout_cnt;
	int32_t timeout;
	bool enabled;
};
#endif
struct uart_silabs_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	USART_TypeDef *base;
	void (*irq_config_func)(const struct device *dev);
};

enum uart_silabs_pm_lock {
	UART_SILABS_PM_LOCK_TX,
	UART_SILABS_PM_LOCK_TX_POLL,
	UART_SILABS_PM_LOCK_RX,
	UART_SILABS_PM_LOCK_COUNT,
};

struct uart_silabs_data {
	struct uart_config *uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
#ifdef CONFIG_UART_SILABS_USART_ASYNC
	const struct device *uart_dev;
	uart_callback_t async_cb;
	void *async_user_data;
	struct uart_dma_channel dma_rx;
	struct uart_dma_channel dma_tx;
	uint8_t *rx_next_buffer;
	size_t rx_next_buffer_len;
#endif
#ifdef CONFIG_PM
	ATOMIC_DEFINE(pm_lock, UART_SILABS_PM_LOCK_COUNT);
#endif
};

static int uart_silabs_pm_action(const struct device *dev, enum pm_device_action action);

/**
 * @brief Get PM lock on low power states
 *
 * @param dev  UART device struct
 * @param lock UART PM lock type
 *
 * @return true if lock was taken, false otherwise
 */
static bool uart_silabs_pm_lock_get(const struct device *dev, enum uart_silabs_pm_lock lock)
{
#ifdef CONFIG_PM
	struct uart_silabs_data *data = dev->data;
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
static bool uart_silabs_pm_lock_put(const struct device *dev, enum uart_silabs_pm_lock lock)
{
#ifdef CONFIG_PM
	struct uart_silabs_data *data = dev->data;
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

static int uart_silabs_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_silabs_config *config = dev->config;
	uint32_t flags = USART_StatusGet(config->base);

	if (flags & USART_STATUS_RXDATAV) {
		*c = USART_Rx(config->base);
		return 0;
	}

	return -1;
}

static void uart_silabs_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_silabs_config *config = dev->config;

	if (uart_silabs_pm_lock_get(dev, UART_SILABS_PM_LOCK_TX_POLL)) {
		USART_IntEnable(config->base, USART_IF_TXC);
	}

	USART_Tx(config->base, c);
}

static int uart_silabs_err_check(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;
	uint32_t flags = USART_IntGet(config->base);
	int err = 0;

	if (flags & USART_IF_RXOF) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & USART_IF_PERR) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & USART_IF_FERR) {
		err |= UART_ERROR_FRAMING;
	}

	USART_IntClear(config->base, USART_IF_RXOF | USART_IF_PERR | USART_IF_FERR);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_silabs_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_silabs_config *config = dev->config;
	int i = 0;

	while ((i < len) && (config->base->STATUS & USART_STATUS_TXBL)) {
		config->base->TXDATA = tx_data[i++];
	}

	return i;
}

static int uart_silabs_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	const struct uart_silabs_config *config = dev->config;
	int i = 0;

	while ((i < len) && (config->base->STATUS & USART_STATUS_RXDATAV)) {
		rx_data[i++] = (uint8_t)config->base->RXDATA;
	}

	return i;
}

static void uart_silabs_irq_tx_enable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	(void)uart_silabs_pm_lock_get(dev, UART_SILABS_PM_LOCK_TX);
	USART_IntEnable(config->base, USART_IEN_TXBL | USART_IEN_TXC);
}

static void uart_silabs_irq_tx_disable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	USART_IntDisable(config->base, USART_IEN_TXBL | USART_IEN_TXC);
	(void)uart_silabs_pm_lock_put(dev, UART_SILABS_PM_LOCK_TX);
}

static int uart_silabs_irq_tx_complete(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;
	uint32_t flags = USART_IntGet(config->base);

	USART_IntClear(config->base, USART_IF_TXC);

	return !!(flags & USART_IF_TXC);
}

static int uart_silabs_irq_tx_ready(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;
	uint32_t flags = USART_IntGetEnabled(config->base);

	return !!(flags & USART_IF_TXBL);
}

static void uart_silabs_irq_rx_enable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	(void)uart_silabs_pm_lock_get(dev, UART_SILABS_PM_LOCK_RX);
	USART_IntEnable(config->base, USART_IEN_RXDATAV);
}

static void uart_silabs_irq_rx_disable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	USART_IntDisable(config->base, USART_IEN_RXDATAV);
	(void)uart_silabs_pm_lock_put(dev, UART_SILABS_PM_LOCK_RX);
}

static int uart_silabs_irq_rx_full(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;
	uint32_t flags = USART_IntGet(config->base);

	return !!(flags & USART_IF_RXDATAV);
}

static int uart_silabs_irq_rx_ready(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	return (config->base->IEN & USART_IEN_RXDATAV) && uart_silabs_irq_rx_full(dev);
}

static void uart_silabs_irq_err_enable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	USART_IntEnable(config->base, USART_IF_RXOF | USART_IF_PERR | USART_IF_FERR);
}

static void uart_silabs_irq_err_disable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;

	USART_IntDisable(config->base, USART_IF_RXOF | USART_IF_PERR | USART_IF_FERR);
}

static int uart_silabs_irq_is_pending(const struct device *dev)
{
	return uart_silabs_irq_tx_ready(dev) || uart_silabs_irq_rx_ready(dev);
}

static int uart_silabs_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_silabs_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct uart_silabs_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_SILABS_USART_ASYNC
static inline void async_user_callback(struct uart_silabs_data *data, struct uart_event *event)
{
	if (data->async_cb) {
		data->async_cb(data->uart_dev, event, data->async_user_data);
	}
}

static inline void async_evt_rx_rdy(struct uart_silabs_data *data)
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

static inline void async_evt_tx_done(struct uart_silabs_data *data)
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

static inline void async_evt_tx_abort(struct uart_silabs_data *data)
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

static inline void async_evt_rx_err(struct uart_silabs_data *data, int err_code)
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

static inline void async_evt_rx_buf_release(struct uart_silabs_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->dma_rx.buffer,
	};

	async_user_callback(data, &evt);
}

static inline void async_evt_rx_buf_request(struct uart_silabs_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(data, &evt);
}

static int uart_silabs_async_callback_set(const struct device *dev, uart_callback_t callback,
					  void *user_data)
{
	struct uart_silabs_data *data = dev->data;

	data->async_cb = callback;
	data->async_user_data = user_data;

	return 0;
}

static void uart_silabs_dma_replace_buffer(const struct device *dev)
{
	struct uart_silabs_data *data = dev->data;

	data->dma_rx.offset = 0;
	data->dma_rx.counter = 0;
	data->dma_rx.buffer = data->rx_next_buffer;
	data->dma_rx.buffer_length = data->rx_next_buffer_len;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	async_evt_rx_buf_request(data);
}

static void uart_silabs_dma_rx_flush(struct uart_silabs_data *data)
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

void uart_silabs_dma_rx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
			   int status)
{
	const struct device *uart_dev = user_data;
	struct uart_silabs_data *data = uart_dev->data;
	struct uart_event disabled_event = {.type = UART_RX_DISABLED};

	if (status < 0) {
		async_evt_rx_err(data, status);
		return;
	}

	data->dma_rx.counter = data->dma_rx.buffer_length;

	async_evt_rx_rdy(data);

	if (data->rx_next_buffer) {
		async_evt_rx_buf_release(data);
		uart_silabs_dma_replace_buffer(uart_dev);
	} else {
		dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);
		data->dma_rx.enabled = false;
		async_evt_rx_buf_release(data);
		async_user_callback(data, &disabled_event);
	}
}

void uart_silabs_dma_tx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
			   int status)
{
	const struct device *uart_dev = user_data;
	struct uart_silabs_data *data = uart_dev->data;

	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	data->dma_tx.enabled = false;
}

static int uart_silabs_async_tx(const struct device *dev, const uint8_t *tx_data, size_t buf_size,
				int32_t timeout)
{
	const struct uart_silabs_config *config = dev->config;
	struct uart_silabs_data *data = dev->data;
	int ret;

	if (!data->dma_tx.dma_dev) {
		return -ENODEV;
	}

	if (data->dma_tx.buffer_length) {
		return -EBUSY;
	}

	data->dma_tx.buffer = (uint8_t *)tx_data;
	data->dma_tx.buffer_length = buf_size;

	/* User timeout is expressed as number of TCMP2 interrupt which occurs every
	 * SILABS_USART_TIMER_COMPARE_VALUE baud-times
	 */
	if (data->uart_cfg->baudrate > 0 && timeout >= 0) {
		data->dma_tx.timeout =
			SILABS_USART_TIMEOUT_TO_TIMERCOUNTER(timeout, data->uart_cfg->baudrate);
	} else {
		data->dma_tx.timeout = 0;
	}

	data->dma_tx.blk_cfg.source_address = (uint32_t)data->dma_tx.buffer;
	data->dma_tx.blk_cfg.block_size = data->dma_tx.buffer_length;

	(void)uart_silabs_pm_lock_get(dev, UART_SILABS_PM_LOCK_TX);
	USART_IntClear(config->base, USART_IF_TXC | USART_IF_TCMP2);
	USART_IntEnable(config->base, USART_IF_TXC);
	if (timeout >= 0) {
		USART_IntEnable(config->base, USART_IF_TCMP2);
	}

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

	data->dma_tx.enabled = true;

	return 0;
}

static int uart_silabs_async_tx_abort(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;
	struct uart_silabs_data *data = dev->data;
	size_t tx_buffer_length = data->dma_tx.buffer_length;
	struct dma_status stat;

	if (!tx_buffer_length) {
		return -EFAULT;
	}

	USART_IntDisable(config->base, USART_IF_TXC);
	USART_IntDisable(config->base, USART_IF_TCMP2);
	USART_IntClear(config->base, USART_IF_TXC | USART_IF_TCMP2);
	(void)uart_silabs_pm_lock_put(dev, UART_SILABS_PM_LOCK_TX);

	if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = tx_buffer_length - stat.pending_length;
	}

	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	data->dma_tx.enabled = false;

	async_evt_tx_abort(data);

	return 0;
}

static int uart_silabs_async_rx_enable(const struct device *dev, uint8_t *rx_buf, size_t buf_size,
				       int32_t timeout)
{
	const struct uart_silabs_config *config = dev->config;
	struct uart_silabs_data *data = dev->data;
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

	/* User timeout is expressed as number of TCMP1 interrupt which occurs every
	 * SILABS_USART_TIMER_COMPARE_VALUE baud-times
	 */
	if (data->uart_cfg->baudrate > 0 && timeout >= 0) {
		data->dma_rx.timeout =
			SILABS_USART_TIMEOUT_TO_TIMERCOUNTER(timeout, data->uart_cfg->baudrate);
	} else {
		data->dma_rx.timeout = 0;
	}

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

	(void)uart_silabs_pm_lock_get(dev, UART_SILABS_PM_LOCK_RX);
	USART_IntClear(config->base, USART_IF_RXOF | USART_IF_TCMP1);
	USART_IntEnable(config->base, USART_IF_RXOF);

	if (timeout >= 0) {
		USART_IntEnable(config->base, USART_IF_TCMP1);
	}

	data->dma_rx.enabled = true;

	async_evt_rx_buf_request(data);

	return ret;
}

static int uart_silabs_async_rx_disable(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;
	USART_TypeDef *usart = config->base;
	struct uart_silabs_data *data = dev->data;
	struct uart_event disabled_event = {.type = UART_RX_DISABLED};

	if (!data->dma_rx.enabled) {
		return -EFAULT;
	}

	dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	USART_IntDisable(usart, USART_IF_RXOF);
	USART_IntDisable(usart, USART_IF_TCMP1);
	USART_IntClear(usart, USART_IF_RXOF | USART_IF_TCMP1);
	(void)uart_silabs_pm_lock_put(dev, UART_SILABS_PM_LOCK_RX);

	if (!data->dma_rx.enabled) {
		usart->CMD = USART_CMD_CLEARRX;
	}

	uart_silabs_dma_rx_flush(data);

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

static int uart_silabs_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_silabs_data *data = dev->data;
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

static int uart_silabs_async_init(const struct device *dev)
{
	const struct uart_silabs_config *config = dev->config;
	USART_TypeDef *usart = config->base;
	struct uart_silabs_data *data = dev->data;

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

	memset(&data->dma_rx.blk_cfg, 0, sizeof(data->dma_rx.blk_cfg));
	data->dma_rx.blk_cfg.source_address = (uintptr_t)&(usart->RXDATA);
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
	data->dma_tx.blk_cfg.dest_address = (uintptr_t)&(usart->TXDATA);
	data->dma_tx.blk_cfg.source_address = 0;
	data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->dma_tx.dma_cfg.complete_callback_en = 1;
	data->dma_tx.dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
	data->dma_tx.dma_cfg.user_data = (void *)dev;

	config->base->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;
	config->base->TIMECMP1 =
		USART_TIMECMP1_TSTOP_RXACT | USART_TIMECMP1_TSTART_RXEOF |
		USART_TIMECMP1_RESTARTEN |
		(SILABS_USART_TIMER_COMPARE_VALUE << _USART_TIMECMP1_TCMPVAL_SHIFT);
	config->base->TIMECMP2 =
		USART_TIMECMP2_TSTOP_TXST | USART_TIMECMP2_TSTART_TXEOF | USART_TIMECMP2_RESTARTEN |
		(SILABS_USART_TIMER_COMPARE_VALUE << _USART_TIMECMP2_TCMPVAL_SHIFT);

	return 0;
}
#endif /* CONFIG_UART_SILABS_USART_ASYNC */

static void uart_silabs_isr(const struct device *dev)
{
	__maybe_unused struct uart_silabs_data *data = dev->data;
	const struct uart_silabs_config *config = dev->config;
	USART_TypeDef *usart = config->base;
	uint32_t flags = USART_IntGet(usart);
#ifdef CONFIG_UART_SILABS_USART_ASYNC
	struct dma_status stat;
#endif

	if (flags & USART_IF_TXC) {
		if (uart_silabs_pm_lock_put(dev, UART_SILABS_PM_LOCK_TX_POLL)) {
			USART_IntDisable(usart, USART_IEN_TXC);
			USART_IntClear(usart, USART_IF_TXC);
		}
	}
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
#endif
#ifdef CONFIG_UART_SILABS_USART_ASYNC
	if (flags & USART_IF_TCMP1) {

		data->dma_rx.timeout_cnt++;
		if (data->dma_rx.timeout_cnt >= data->dma_rx.timeout) {
			uart_silabs_dma_rx_flush(data);

			usart->TIMECMP1 &= ~_USART_TIMECMP1_TSTART_MASK;
			usart->TIMECMP1 |= USART_TIMECMP1_TSTART_RXEOF;
			data->dma_rx.timeout_cnt = 0;
		}

		USART_IntClear(usart, USART_IF_TCMP1);
	}
	if (flags & USART_IF_RXOF) {
		async_evt_rx_err(data, UART_ERROR_OVERRUN);

		uart_silabs_async_rx_disable(dev);

		USART_IntClear(usart, USART_IF_RXOF);
	}
	if (flags & USART_IF_TXC) {
		if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
			data->dma_tx.counter = data->dma_tx.buffer_length - stat.pending_length;
		}

		if (data->dma_tx.counter == data->dma_tx.buffer_length) {
			USART_IntDisable(config->base, USART_IF_TXC);
			USART_IntDisable(config->base, USART_IF_TCMP2);
			USART_IntClear(usart, USART_IF_TXC | USART_IF_TCMP2);
			(void)uart_silabs_pm_lock_put(dev, UART_SILABS_PM_LOCK_TX);

			usart->TIMECMP2 &= ~_USART_TIMECMP2_TSTART_MASK;
			usart->TIMECMP2 |= USART_TIMECMP2_TSTART_DISABLE;
		}

		async_evt_tx_done(data);
	}
	if (flags & USART_IF_TCMP2) {
		data->dma_tx.timeout_cnt++;
		if (data->dma_tx.timeout_cnt >= data->dma_tx.timeout) {
			usart->TIMECMP2 &= ~_USART_TIMECMP2_TSTART_MASK;
			usart->TIMECMP2 |= USART_TIMECMP2_TSTART_DISABLE;
			data->dma_tx.timeout_cnt = 0;

			uart_silabs_async_tx_abort(dev);
		}

		USART_IntClear(usart, USART_IF_TCMP2);
	}
#endif /* CONFIG_UART_SILABS_USART_ASYNC */
}

static inline USART_Parity_TypeDef uart_silabs_cfg2ll_parity(
	enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return usartOddParity;
	case UART_CFG_PARITY_EVEN:
		return usartEvenParity;
	case UART_CFG_PARITY_NONE:
	default:
		return usartNoParity;
	}
}

static inline USART_Stopbits_TypeDef uart_silabs_cfg2ll_stopbits(
	enum uart_config_stop_bits sb)
{
	switch (sb) {
	case UART_CFG_STOP_BITS_0_5:
		return usartStopbits0p5;
	case UART_CFG_STOP_BITS_1:
		return usartStopbits1;
	case UART_CFG_STOP_BITS_2:
		return usartStopbits2;
	case UART_CFG_STOP_BITS_1_5:
		return usartStopbits1p5;
	default:
		return usartStopbits1;
	}
}

static inline USART_Databits_TypeDef uart_silabs_cfg2ll_databits(
	enum uart_config_data_bits db, enum uart_config_parity p)
{
	switch (db) {
	case UART_CFG_DATA_BITS_7:
		if (p == UART_CFG_PARITY_NONE) {
			return usartDatabits7;
		} else {
			return usartDatabits8;
		}
	case UART_CFG_DATA_BITS_9:
		return usartDatabits9;
	case UART_CFG_DATA_BITS_8:
	default:
		if (p == UART_CFG_PARITY_NONE) {
			return usartDatabits8;
		} else {
			return usartDatabits9;
		}
		return usartDatabits8;
	}
}

static inline USART_HwFlowControl_TypeDef uart_silabs_cfg2ll_hwctrl(
	enum uart_config_flow_control fc)
{
	if (fc == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return usartHwFlowControlCtsAndRts;
	}

	return usartHwFlowControlNone;
}

static inline enum uart_config_parity uart_silabs_ll2cfg_parity(USART_Parity_TypeDef parity)
{
	switch (parity) {
	case usartOddParity:
		return UART_CFG_PARITY_ODD;
	case usartEvenParity:
		return UART_CFG_PARITY_EVEN;
	case usartNoParity:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static inline enum uart_config_stop_bits uart_silabs_ll2cfg_stopbits(USART_Stopbits_TypeDef sb)
{
	switch (sb) {
	case usartStopbits0p5:
		return UART_CFG_STOP_BITS_0_5;
	case usartStopbits1:
		return UART_CFG_STOP_BITS_1;
	case usartStopbits1p5:
		return UART_CFG_STOP_BITS_1_5;
	case usartStopbits2:
		return UART_CFG_STOP_BITS_2;
	default:
		return UART_CFG_STOP_BITS_1;
	}
}

static inline enum uart_config_data_bits uart_silabs_ll2cfg_databits(USART_Databits_TypeDef db,
								    USART_Parity_TypeDef p)
{
	switch (db) {
	case usartDatabits7:
		if (p == usartNoParity) {
			return UART_CFG_DATA_BITS_7;
		} else {
			return UART_CFG_DATA_BITS_6;
		}
	case usartDatabits9:
		if (p == usartNoParity) {
			return UART_CFG_DATA_BITS_9;
		} else {
			return UART_CFG_DATA_BITS_8;
		}
	case usartDatabits8:
	default:
		if (p == usartNoParity) {
			return UART_CFG_DATA_BITS_8;
		} else {
			return UART_CFG_DATA_BITS_7;
		}
	}
}

static inline enum uart_config_flow_control uart_silabs_ll2cfg_hwctrl(
	USART_HwFlowControl_TypeDef fc)
{
	if (fc == usartHwFlowControlCtsAndRts) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_FLOW_CTRL_NONE;
}

static void uart_silabs_configure_peripheral(const struct device *dev, bool enable)
{
	const struct uart_silabs_config *config = dev->config;
	const struct uart_silabs_data *data = dev->data;
	USART_InitAsync_TypeDef usartInit = USART_INITASYNC_DEFAULT;

	usartInit.baudrate = data->uart_cfg->baudrate;
	usartInit.parity = uart_silabs_cfg2ll_parity(data->uart_cfg->parity);
	usartInit.stopbits = uart_silabs_cfg2ll_stopbits(data->uart_cfg->stop_bits);
	usartInit.databits = uart_silabs_cfg2ll_databits(data->uart_cfg->data_bits,
							 data->uart_cfg->parity);
	usartInit.hwFlowControl = uart_silabs_cfg2ll_hwctrl(data->uart_cfg->flow_ctrl);
	usartInit.enable = enable ? usartEnable : usartDisable;

	USART_InitAsync(config->base, &usartInit);
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_silabs_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	const struct uart_silabs_config *config = dev->config;
	USART_TypeDef *base = config->base;
	struct uart_silabs_data *data = dev->data;

#ifdef CONFIG_UART_SILABS_USART_ASYNC
	if (data->dma_rx.enabled || data->dma_tx.enabled) {
		return -EBUSY;
	}
#endif

	if ((cfg->parity == UART_CFG_PARITY_MARK) ||
	    (cfg->parity == UART_CFG_PARITY_SPACE)) {
		return -ENOSYS;
	}

	if (cfg->parity > UART_CFG_PARITY_SPACE) {
		return -EINVAL;
	}

	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_DTR_DSR ||
	    cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485) {
		return -ENOSYS;
	}

	if (cfg->flow_ctrl > UART_CFG_FLOW_CTRL_RS485) {
		return -EINVAL;
	}

	*data->uart_cfg = *cfg;
	USART_Enable(base, usartDisable);

	uart_silabs_configure_peripheral(dev, true);

	return 0;
};

static int uart_silabs_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct uart_silabs_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;

	cfg->baudrate = uart_cfg->baudrate;
	cfg->parity = uart_cfg->parity;
	cfg->stop_bits = uart_cfg->stop_bits;
	cfg->data_bits = uart_cfg->data_bits;
	cfg->flow_ctrl = uart_cfg->flow_ctrl;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_silabs_init(const struct device *dev)
{
	int err;
	const struct uart_silabs_config *config = dev->config;

	/* The peripheral and gpio clock are already enabled from soc and gpio driver */
	/* Enable USART clock */
	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	uart_silabs_configure_peripheral(dev, false);

	config->irq_config_func(dev);

#ifdef CONFIG_UART_SILABS_USART_ASYNC
	err = uart_silabs_async_init(dev);
	if (err < 0) {
		return err;
	}
#endif
	return pm_device_driver_init(dev, uart_silabs_pm_action);
}

static int uart_silabs_pm_action(const struct device *dev, enum pm_device_action action)
{
	int err;
	const struct uart_silabs_config *config = dev->config;
	__maybe_unused struct uart_silabs_data *data = dev->data;

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

		USART_Enable(config->base, usartEnable);
	} else if (IS_ENABLED(CONFIG_PM_DEVICE) && (action == PM_DEVICE_ACTION_SUSPEND)) {
#ifdef CONFIG_UART_SILABS_USART_ASYNC
		/* Entering suspend requires there to be no active asynchronous calls. */
		__ASSERT_NO_MSG(!data->dma_rx.enabled);
		__ASSERT_NO_MSG(!data->dma_tx.enabled);
#endif
		USART_Enable(config->base, usartDisable);

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

static DEVICE_API(uart, uart_silabs_driver_api) = {
	.poll_in = uart_silabs_poll_in,
	.poll_out = uart_silabs_poll_out,
	.err_check = uart_silabs_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_silabs_configure,
	.config_get = uart_silabs_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_silabs_fifo_fill,
	.fifo_read = uart_silabs_fifo_read,
	.irq_tx_enable = uart_silabs_irq_tx_enable,
	.irq_tx_disable = uart_silabs_irq_tx_disable,
	.irq_tx_complete = uart_silabs_irq_tx_complete,
	.irq_tx_ready = uart_silabs_irq_tx_ready,
	.irq_rx_enable = uart_silabs_irq_rx_enable,
	.irq_rx_disable = uart_silabs_irq_rx_disable,
	.irq_rx_ready = uart_silabs_irq_rx_ready,
	.irq_err_enable = uart_silabs_irq_err_enable,
	.irq_err_disable = uart_silabs_irq_err_disable,
	.irq_is_pending = uart_silabs_irq_is_pending,
	.irq_update = uart_silabs_irq_update,
	.irq_callback_set = uart_silabs_irq_callback_set,
#endif
#ifdef CONFIG_UART_SILABS_USART_ASYNC
	.callback_set = uart_silabs_async_callback_set,
	.tx = uart_silabs_async_tx,
	.tx_abort = uart_silabs_async_tx_abort,
	.rx_enable = uart_silabs_async_rx_enable,
	.rx_disable = uart_silabs_async_rx_disable,
	.rx_buf_rsp = uart_silabs_async_rx_buf_rsp,
#endif
};

#ifdef CONFIG_UART_SILABS_USART_ASYNC

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
			.dma_callback = uart_silabs_dma_##dir##_cb,                                \
		}                                                                                  \
	},
#define UART_DMA_CHANNEL(index, dir)                                                               \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, dmas),                                            \
		 (UART_DMA_CHANNEL_INIT(index, dir)), ())
#else

#define UART_DMA_CHANNEL(index, dir)

#endif

#define SILABS_USART_IRQ_HANDLER_FUNC(idx) .irq_config_func = usart_silabs_config_func_##idx,
#define SILABS_USART_IRQ_HANDLER(idx)                                                              \
	static void usart_silabs_config_func_##idx(const struct device *dev)                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, rx, irq),                                     \
			    DT_INST_IRQ_BY_NAME(idx, rx, priority), uart_silabs_isr,               \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, tx, irq),                                     \
			    DT_INST_IRQ_BY_NAME(idx, tx, priority), uart_silabs_isr,               \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, rx, irq));                                     \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, tx, irq));                                     \
	}

#define SILABS_USART_INIT(idx)                                                                     \
	SILABS_USART_IRQ_HANDLER(idx);                                                             \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	PM_DEVICE_DT_INST_DEFINE(idx, uart_silabs_pm_action);                                      \
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
	static const struct uart_silabs_config uart_silabs_cfg_##idx = {                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.base = (USART_TypeDef *)DT_INST_REG_ADDR(idx),                                    \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),                                        \
		SILABS_USART_IRQ_HANDLER_FUNC(idx)                                                 \
	};                                                                                         \
                                                                                                   \
	static struct uart_silabs_data uart_silabs_data_##idx = {                                  \
		.uart_cfg = &uart_cfg_##idx,                                                       \
		UART_DMA_CHANNEL(idx, rx)                                                          \
		UART_DMA_CHANNEL(idx, tx)                                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, uart_silabs_init, PM_DEVICE_DT_INST_GET(idx),                   \
			      &uart_silabs_data_##idx, &uart_silabs_cfg_##idx, PRE_KERNEL_1,       \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_silabs_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SILABS_USART_INIT)
