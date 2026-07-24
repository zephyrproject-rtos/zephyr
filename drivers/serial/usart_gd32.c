/*
 * Copyright (c) 2021, ATL Electronics
 * Copyright (c) 2025 Aleksandr Senin <al@meshium.net>
 * Copyright (c) 2026 Liu Changjie <liucj1228@outlook.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_usart

#include <errno.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_gd32.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

#include <gd32_usart.h>

/* Unify GD32 HAL USART status register name to USART_STAT */
#ifndef USART_STAT
#define USART_STAT USART_STAT0
#endif
#ifndef USART_RDATA
#define USART_RDATA USART_DATA
#endif
#ifndef USART_TDATA
#define USART_TDATA USART_DATA
#endif
#ifndef USART_RECEIVE_DMA_ENABLE
#define USART_RECEIVE_DMA_ENABLE USART_DENR_ENABLE
#endif
#ifndef USART_RECEIVE_DMA_DISABLE
#define USART_RECEIVE_DMA_DISABLE USART_DENR_DISABLE
#endif
#ifndef USART_TRANSMIT_DMA_ENABLE
#define USART_TRANSMIT_DMA_ENABLE USART_DENT_ENABLE
#endif
#ifndef USART_TRANSMIT_DMA_DISABLE
#define USART_TRANSMIT_DMA_DISABLE USART_DENT_DISABLE
#endif

#ifdef CONFIG_UART_ASYNC_API
enum gd32_usart_dma_dir {
	GD32_USART_DMA_TX,
	GD32_USART_DMA_RX,
};

struct gd32_usart_dma_config {
	const struct device *dev;
	uint32_t channel;
	uint32_t slot;
	uint32_t config;
};

struct gd32_usart_dma_data {
	const struct device *uart_dev;
	struct dma_config config;
	struct dma_block_config block;
	struct k_work_delayable timeout_work;
	uint8_t *buf;
	size_t len;
	size_t offset;
	uint8_t *next_buf;
	size_t next_len;
	int32_t timeout_us;
	bool busy;
	bool requested;
};
#endif /* CONFIG_UART_ASYNC_API */

struct gd32_usart_config {
	uint32_t reg;
	uint16_t clkid;
	struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pcfg;
	uint32_t parity;
#ifdef CONFIG_UART_ASYNC_API
	const struct gd32_usart_dma_config dma_tx;
	const struct gd32_usart_dma_config dma_rx;
#endif /* CONFIG_UART_ASYNC_API */
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t irq_config_func;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */
};

struct gd32_usart_data {
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	uart_callback_t async_cb;
	void *async_user_data;
	struct gd32_usart_dma_data dma_tx;
	struct gd32_usart_dma_data dma_rx;
#endif /* CONFIG_UART_ASYNC_API */
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	enum uart_config_parity parity;
	enum uart_config_stop_bits stop_bits;
	enum uart_config_data_bits data_bits;
	enum uart_config_flow_control flow_ctrl;
	bool initialized;
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
};

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#ifdef CONFIG_UART_ASYNC_API
static void usart_gd32_rx_idle_clear(uint32_t reg);
static void usart_gd32_rx_idle(const struct device *dev);
#endif /* CONFIG_UART_ASYNC_API */

static void usart_gd32_isr(const struct device *dev)
{
#ifdef CONFIG_UART_ASYNC_API
	const struct gd32_usart_config *const cfg = dev->config;

	if (usart_interrupt_flag_get(cfg->reg, USART_INT_FLAG_IDLE) == SET) {
		usart_gd32_rx_idle_clear(cfg->reg);
		usart_gd32_rx_idle(dev);
	}
#endif /* CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct gd32_usart_data *const data = dev->data;

	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_ASYNC_API
static void usart_gd32_async_callback(const struct device *dev, struct uart_event *evt)
{
	struct gd32_usart_data *const data = dev->data;

	if (data->async_cb) {
		data->async_cb(dev, evt, data->async_user_data);
	}
}

static void usart_gd32_rx_idle_clear(uint32_t reg)
{
	volatile uint32_t stat;
	volatile uint32_t data;

	stat = USART_STAT(reg);
	data = USART_RDATA(reg);
	ARG_UNUSED(stat);
	ARG_UNUSED(data);

	usart_flag_clear(reg, USART_FLAG_IDLE);
}

static bool usart_gd32_dma_ready(const struct gd32_usart_dma_config *dma)
{
	return dma->dev && device_is_ready(dma->dev);
}

static bool usart_gd32_async_ready(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	return usart_gd32_dma_ready(&cfg->dma_tx) && usart_gd32_dma_ready(&cfg->dma_rx);
}

static int usart_gd32_dma_request_channel(const struct gd32_usart_dma_config *dma,
					  struct gd32_usart_dma_data *dma_data)
{
	uint32_t ch_filter;
	int ret;

	if (dma_data->requested) {
		return 0;
	}

	ch_filter = BIT(dma->channel);
	ret = dma_request_channel(dma->dev, &ch_filter);
	if (ret < 0) {
		return ret;
	}

	dma_data->requested = true;

	return 0;
}

static void usart_gd32_dma_release_channel(const struct gd32_usart_dma_config *dma,
					   struct gd32_usart_dma_data *dma_data)
{
	if (!dma_data->requested) {
		return;
	}

	dma_release_channel(dma->dev, dma->channel);
	dma_data->requested = false;
}

static uint32_t usart_gd32_rx_data_addr(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	return (uint32_t)(uintptr_t)&USART_RDATA(cfg->reg);
}

static uint32_t usart_gd32_tx_data_addr(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	return (uint32_t)(uintptr_t)&USART_TDATA(cfg->reg);
}

static void usart_gd32_dma_tx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
				 int status)
{
	const struct device *dev = user_data;
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;
	size_t sent = data->dma_tx.len;

	if (status != 0) {
		struct dma_status dma_status = {0};

		if (dma_get_status(cfg->dma_tx.dev, cfg->dma_tx.channel, &dma_status) == 0) {
			sent = data->dma_tx.len - MIN(data->dma_tx.len, dma_status.pending_length);
		}
	}

	struct uart_event evt = {
		.type = status ? UART_TX_ABORTED : UART_TX_DONE,
		.data.tx = {
			.buf = data->dma_tx.buf,
			.len = sent,
		},
	};

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	usart_dma_transmit_config(cfg->reg, USART_TRANSMIT_DMA_DISABLE);
	data->dma_tx.busy = false;
	usart_gd32_dma_release_channel(&cfg->dma_tx, &data->dma_tx);
	usart_gd32_async_callback(dev, &evt);
}

static void usart_gd32_dma_rx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
				 int status);

static void usart_gd32_rx_rdy(const struct device *dev, size_t len)
{
	struct gd32_usart_data *const data = dev->data;

	if (len > data->dma_rx.offset) {
		struct uart_event evt = {
			.type = UART_RX_RDY,
			.data.rx = {
				.buf = data->dma_rx.buf,
				.len = len - data->dma_rx.offset,
				.offset = data->dma_rx.offset,
			},
		};

		usart_gd32_async_callback(dev, &evt);
		data->dma_rx.offset = len;
	}
}

static void usart_gd32_rx_release(const struct device *dev, uint8_t *buf)
{
	struct uart_event release_evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf = {
			.buf = buf,
		},
	};

	usart_gd32_async_callback(dev, &release_evt);
}

static void usart_gd32_rx_request(const struct device *dev)
{
	struct uart_event request_evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	usart_gd32_async_callback(dev, &request_evt);
}

static void usart_gd32_rx_disabled(const struct device *dev)
{
	struct uart_event disabled_evt = {
		.type = UART_RX_DISABLED,
	};

	usart_gd32_async_callback(dev, &disabled_evt);
}

static bool usart_gd32_rx_timeout_enabled(const struct device *dev)
{
	struct gd32_usart_data *const data = dev->data;

	return data->dma_rx.timeout_us != SYS_FOREVER_US;
}

static void usart_gd32_rx_idle_enable(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	if (!usart_gd32_rx_timeout_enabled(dev)) {
		return;
	}

	usart_gd32_rx_idle_clear(cfg->reg);
	usart_interrupt_enable(cfg->reg, USART_INT_IDLE);
}

static void usart_gd32_rx_idle_disable(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;

	usart_interrupt_disable(cfg->reg, USART_INT_IDLE);
	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);
}

static size_t usart_gd32_rx_received_len(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;
	struct dma_status status = {0};

	if (dma_get_status(cfg->dma_rx.dev, cfg->dma_rx.channel, &status) != 0) {
		return 0U;
	}

	return data->dma_rx.len - MIN(data->dma_rx.len, status.pending_length);
}

static void usart_gd32_rx_stop(const struct device *dev, size_t len)
{
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;

	usart_gd32_rx_idle_disable(dev);
	usart_dma_receive_config(cfg->reg, USART_RECEIVE_DMA_DISABLE);

	usart_gd32_rx_rdy(dev, len);
	if (data->dma_rx.buf) {
		usart_gd32_rx_release(dev, data->dma_rx.buf);
	}

	if (data->dma_rx.next_buf) {
		usart_gd32_rx_release(dev, data->dma_rx.next_buf);
		data->dma_rx.next_buf = NULL;
		data->dma_rx.next_len = 0U;
	}

	data->dma_rx.busy = false;
	usart_gd32_dma_release_channel(&cfg->dma_rx, &data->dma_rx);
	usart_gd32_rx_disabled(dev);
}

static int usart_gd32_dma_configure(const struct device *dev, enum gd32_usart_dma_dir dir,
				    uint8_t *buf, size_t len)
{
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;
	const struct gd32_usart_dma_config *dma;
	struct gd32_usart_dma_data *dma_data;
	int ret;

	if (dir == GD32_USART_DMA_TX) {
		dma = &cfg->dma_tx;
		dma_data = &data->dma_tx;
	} else {
		dma = &cfg->dma_rx;
		dma_data = &data->dma_rx;
	}

	if (!usart_gd32_dma_ready(dma)) {
		return -ENODEV;
	}

	ret = usart_gd32_dma_request_channel(dma, dma_data);
	if (ret < 0) {
		return ret;
	}

	memset(&dma_data->config, 0, sizeof(dma_data->config));
	memset(&dma_data->block, 0, sizeof(dma_data->block));

	dma_data->config.dma_slot = dma->slot;
	dma_data->config.channel_priority = GD32_DMA_CONFIG_PRIORITY(dma->config);
	dma_data->config.block_count = 1U;
	dma_data->config.head_block = &dma_data->block;
	dma_data->config.user_data = (void *)dev;
	dma_data->config.source_burst_length = 1U;
	dma_data->config.dest_burst_length = 1U;
	dma_data->config.source_data_size = 1U;
	dma_data->config.dest_data_size = 1U;

	dma_data->block.block_size = len;

	if (dir == GD32_USART_DMA_TX) {
		dma_data->config.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_data->config.dma_callback = usart_gd32_dma_tx_cb;
		dma_data->block.source_address = (uint32_t)(uintptr_t)buf;
		dma_data->block.dest_address = usart_gd32_tx_data_addr(dev);
		dma_data->block.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		dma_data->block.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	} else {
		dma_data->config.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_data->config.dma_callback = usart_gd32_dma_rx_cb;
		dma_data->block.source_address = usart_gd32_rx_data_addr(dev);
		dma_data->block.dest_address = (uint32_t)(uintptr_t)buf;
		dma_data->block.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		dma_data->block.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	}

	dma_data->buf = buf;
	dma_data->len = len;
	dma_data->offset = 0U;

	return dma_config(dma->dev, dma->channel, &dma_data->config);
}

static int usart_gd32_rx_start(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;
	int ret;

	ret = usart_gd32_dma_configure(dev, GD32_USART_DMA_RX, buf, len);
	if (ret < 0) {
		return ret;
	}

	data->dma_rx.busy = true;
	usart_dma_receive_config(cfg->reg, USART_RECEIVE_DMA_ENABLE);

	ret = dma_start(cfg->dma_rx.dev, cfg->dma_rx.channel);
	if (ret < 0) {
		usart_dma_receive_config(cfg->reg, USART_RECEIVE_DMA_DISABLE);
		data->dma_rx.busy = false;
		return ret;
	}

	usart_gd32_rx_idle_enable(dev);

	return 0;
}

static void usart_gd32_dma_rx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
				 int status)
{
	const struct device *dev = user_data;
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;
	uint8_t *next_buf;
	size_t next_len;
	size_t len;
	int ret;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	if (!data->dma_rx.busy) {
		return;
	}

	len = (status == 0) ? data->dma_rx.len : usart_gd32_rx_received_len(dev);

	if (status != 0) {
		struct uart_event stopped_evt = {
			.type = UART_RX_STOPPED,
			.data.rx_stop = {
				.reason = UART_ERROR_OVERRUN,
			},
		};

		usart_gd32_async_callback(dev, &stopped_evt);
		usart_gd32_rx_stop(dev, len);
		return;
	}

	usart_gd32_rx_idle_disable(dev);
	usart_dma_receive_config(cfg->reg, USART_RECEIVE_DMA_DISABLE);

	usart_gd32_rx_rdy(dev, len);
	usart_gd32_rx_release(dev, data->dma_rx.buf);

	next_buf = data->dma_rx.next_buf;
	next_len = data->dma_rx.next_len;
	data->dma_rx.next_buf = NULL;
	data->dma_rx.next_len = 0U;

	if (!next_buf) {
		data->dma_rx.busy = false;
		usart_gd32_dma_release_channel(&cfg->dma_rx, &data->dma_rx);
		usart_gd32_rx_disabled(dev);
		return;
	}

	ret = usart_gd32_rx_start(dev, next_buf, next_len);
	if (ret < 0) {
		usart_gd32_rx_release(dev, next_buf);
		data->dma_rx.busy = false;
		usart_gd32_dma_release_channel(&cfg->dma_rx, &data->dma_rx);
		usart_gd32_rx_disabled(dev);
		return;
	}

	usart_gd32_rx_request(dev);
}

static void usart_gd32_rx_idle(const struct device *dev)
{
	struct gd32_usart_data *const data = dev->data;
	size_t len;

	if (!data->dma_rx.busy || !usart_gd32_rx_timeout_enabled(dev)) {
		return;
	}

	len = usart_gd32_rx_received_len(dev);
	if (len <= data->dma_rx.offset) {
		return;
	}

	(void)k_work_reschedule(&data->dma_rx.timeout_work, K_USEC(data->dma_rx.timeout_us));
}

static void usart_gd32_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gd32_usart_dma_data *dma_rx =
		CONTAINER_OF(dwork, struct gd32_usart_dma_data, timeout_work);
	const struct device *dev = dma_rx->uart_dev;
	size_t len;
	unsigned int key;

	key = irq_lock();

	if (!dma_rx->busy) {
		irq_unlock(key);
		return;
	}

	len = usart_gd32_rx_received_len(dev);
	if (len <= dma_rx->offset) {
		irq_unlock(key);
		return;
	}

	usart_gd32_rx_rdy(dev, len);

	irq_unlock(key);
}

static int usart_gd32_callback_set(const struct device *dev, uart_callback_t callback,
				   void *user_data)
{
	struct gd32_usart_data *const data = dev->data;

	if (!usart_gd32_async_ready(dev)) {
		return -ENOSYS;
	}

	data->async_cb = callback;
	data->async_user_data = user_data;

	return 0;
}

static int usart_gd32_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;
	int ret;

	ARG_UNUSED(timeout);

	if (buf == NULL || len == 0U) {
		return -EINVAL;
	}

	if (data->dma_tx.busy) {
		return -EBUSY;
	}

	if (!usart_gd32_dma_ready(&cfg->dma_tx)) {
		return -ENODEV;
	}

	ret = usart_gd32_dma_configure(dev, GD32_USART_DMA_TX, (uint8_t *)buf, len);
	if (ret < 0) {
		return ret;
	}

	data->dma_tx.busy = true;
	usart_dma_transmit_config(cfg->reg, USART_TRANSMIT_DMA_ENABLE);

	ret = dma_start(cfg->dma_tx.dev, cfg->dma_tx.channel);
	if (ret < 0) {
		usart_dma_transmit_config(cfg->reg, USART_TRANSMIT_DMA_DISABLE);
		data->dma_tx.busy = false;
		return ret;
	}

	return 0;
}

static int usart_gd32_tx_abort(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;
	struct dma_status status = {0};
	size_t sent = 0U;

	if (!data->dma_tx.busy) {
		return -EFAULT;
	}

	if (dma_get_status(cfg->dma_tx.dev, cfg->dma_tx.channel, &status) == 0) {
		sent = data->dma_tx.len - MIN(data->dma_tx.len, status.pending_length);
	}
	(void)dma_stop(cfg->dma_tx.dev, cfg->dma_tx.channel);
	usart_dma_transmit_config(cfg->reg, USART_TRANSMIT_DMA_DISABLE);

	data->dma_tx.busy = false;
	usart_gd32_dma_release_channel(&cfg->dma_tx, &data->dma_tx);

	struct uart_event evt = {
		.type = UART_TX_ABORTED,
		.data.tx = {
			.buf = data->dma_tx.buf,
			.len = sent,
		},
	};

	usart_gd32_async_callback(dev, &evt);

	return 0;
}

static int usart_gd32_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	struct gd32_usart_data *const data = dev->data;
	int ret;

	if (buf == NULL || len == 0U) {
		return -EINVAL;
	}

	if (timeout < 0 && timeout != SYS_FOREVER_US) {
		return -EINVAL;
	}

	if (data->dma_rx.busy) {
		return -EBUSY;
	}

	data->dma_rx.next_buf = NULL;
	data->dma_rx.next_len = 0U;
	data->dma_rx.timeout_us = timeout;
	ret = usart_gd32_rx_start(dev, buf, len);
	if (ret < 0) {
		return ret;
	}

	usart_gd32_rx_request(dev);

	return 0;
}

static int usart_gd32_rx_disable(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;
	struct dma_status status = {0};
	uint8_t *buf;
	uint8_t *next_buf;
	size_t received = 0U;
	unsigned int key;

	key = irq_lock();
	if (!data->dma_rx.busy) {
		irq_unlock(key);
		return -EFAULT;
	}

	if (dma_get_status(cfg->dma_rx.dev, cfg->dma_rx.channel, &status) == 0) {
		received = data->dma_rx.len - MIN(data->dma_rx.len, status.pending_length);
	}
	(void)dma_stop(cfg->dma_rx.dev, cfg->dma_rx.channel);

	usart_interrupt_disable(cfg->reg, USART_INT_IDLE);
	usart_dma_receive_config(cfg->reg, USART_RECEIVE_DMA_DISABLE);

	buf = data->dma_rx.buf;
	next_buf = data->dma_rx.next_buf;
	data->dma_rx.next_buf = NULL;
	data->dma_rx.next_len = 0U;
	data->dma_rx.busy = false;
	usart_gd32_dma_release_channel(&cfg->dma_rx, &data->dma_rx);
	irq_unlock(key);

	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);

	usart_gd32_rx_rdy(dev, received);
	if (buf) {
		usart_gd32_rx_release(dev, buf);
	}
	if (next_buf) {
		usart_gd32_rx_release(dev, next_buf);
	}
	usart_gd32_rx_disabled(dev);

	return 0;
}

static int usart_gd32_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct gd32_usart_data *const data = dev->data;
	unsigned int key;
	int ret = 0;

	if (buf == NULL || len == 0U) {
		return -EINVAL;
	}

	key = irq_lock();
	if (!data->dma_rx.busy) {
		ret = -EACCES;
		goto unlock;
	}

	if (data->dma_rx.next_buf) {
		ret = -EBUSY;
		goto unlock;
	}

	data->dma_rx.next_buf = buf;
	data->dma_rx.next_len = len;

unlock:
	irq_unlock(key);
	return ret;
}
#endif /* CONFIG_UART_ASYNC_API */

static int usart_gd32_init(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;
	uint32_t word_length;
	uint32_t parity;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/**
	 * In order to keep the transfer data size to 8 bits(1 byte),
	 * append word length to 9BIT if parity bit enabled.
	 */
	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		parity = USART_PM_NONE;
		word_length = USART_WL_8BIT;
		break;
	case UART_CFG_PARITY_ODD:
		parity = USART_PM_ODD;
		word_length = USART_WL_9BIT;
		break;
	case UART_CFG_PARITY_EVEN:
		parity = USART_PM_EVEN;
		word_length = USART_WL_9BIT;
		break;
	default:
		return -ENOTSUP;
	}

	(void)clock_control_on(GD32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clkid);

	(void)reset_line_toggle_dt(&cfg->reset);

	usart_baudrate_set(cfg->reg, data->baud_rate);
	usart_parity_config(cfg->reg, parity);
	usart_word_length_set(cfg->reg, word_length);
	/* Default to 1 stop bit */
	usart_stop_bit_set(cfg->reg, USART_STB_1BIT);
	usart_receive_config(cfg->reg, USART_RECEIVE_ENABLE);
	usart_transmit_config(cfg->reg, USART_TRANSMIT_ENABLE);
	usart_enable(cfg->reg);

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */
#ifdef CONFIG_UART_ASYNC_API
	data->dma_tx.uart_dev = dev;
	data->dma_rx.uart_dev = dev;
	data->dma_rx.timeout_us = SYS_FOREVER_US;
	k_work_init_delayable(&data->dma_rx.timeout_work, usart_gd32_rx_timeout);
#endif /* CONFIG_UART_ASYNC_API */
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	/* Initialize runtime configuration from Devicetree defaults */
	data->parity = cfg->parity;
	data->data_bits = UART_CFG_DATA_BITS_8;
	data->stop_bits = UART_CFG_STOP_BITS_1;
	data->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	data->initialized = true;
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
	return 0;
}

static int usart_gd32_poll_in(const struct device *dev, unsigned char *c)
{
	const struct gd32_usart_config *const cfg = dev->config;
	uint32_t status;

	status = usart_flag_get(cfg->reg, USART_FLAG_RBNE);

	if (!status) {
		return -EPERM;
	}

	*c = usart_data_receive(cfg->reg);

	return 0;
}

static void usart_gd32_poll_out(const struct device *dev, unsigned char c)
{
	const struct gd32_usart_config *const cfg = dev->config;

	usart_data_transmit(cfg->reg, c);

	while (usart_flag_get(cfg->reg, USART_FLAG_TBE) == RESET) {
		;
	}
}

static int usart_gd32_err_check(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;
	uint32_t status = USART_STAT(cfg->reg);
	int errors = 0;

	if (status & USART_FLAG_ORERR) {
		usart_flag_clear(cfg->reg, USART_FLAG_ORERR);

		errors |= UART_ERROR_OVERRUN;
	}

	if (status & USART_FLAG_PERR) {
		usart_flag_clear(cfg->reg, USART_FLAG_PERR);

		errors |= UART_ERROR_PARITY;
	}

	if (status & USART_FLAG_FERR) {
		usart_flag_clear(cfg->reg, USART_FLAG_FERR);

		errors |= UART_ERROR_FRAMING;
	}

	usart_flag_clear(cfg->reg, USART_FLAG_NERR);

	return errors;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int usart_gd32_configure(const struct device *dev, const struct uart_config *cfg_new)
{
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;
	uint32_t parity_bits;
	uint32_t word_length;
	uint32_t stop_bits_hw;

	if (cfg_new == NULL) {
		return -EINVAL;
	}

	if (cfg_new->baudrate == 0U) {
		return -EINVAL;
	}

	if (cfg_new->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	switch (cfg_new->parity) {
	case UART_CFG_PARITY_NONE:
		parity_bits = USART_PM_NONE;
		break;
	case UART_CFG_PARITY_ODD:
		parity_bits = USART_PM_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		parity_bits = USART_PM_EVEN;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg_new->data_bits) {
	case UART_CFG_DATA_BITS_8:
	case UART_CFG_DATA_BITS_7:
		break;
	default:
		return -EINVAL;
	}

	if (cfg_new->data_bits == UART_CFG_DATA_BITS_7 && cfg_new->parity == UART_CFG_PARITY_NONE) {
		return -EINVAL;
	}

	/* Map word length depending on requested data bits and parity */
	if (cfg_new->parity == UART_CFG_PARITY_NONE) {
		/* 8N* uses 8-bit word length */
		word_length = USART_WL_8BIT;
	} else {
		/* With parity: 8 data bits -> 9-bit word length, 7 data bits -> 8-bit */
		word_length = (cfg_new->data_bits == UART_CFG_DATA_BITS_8) ? USART_WL_9BIT
									   : USART_WL_8BIT;
	}

	switch (cfg_new->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		stop_bits_hw = USART_STB_1BIT;
		break;
	case UART_CFG_STOP_BITS_2:
		stop_bits_hw = USART_STB_2BIT;
		break;
	default:
		return -EINVAL;
	}

	if (data->baud_rate == cfg_new->baudrate && data->parity == cfg_new->parity &&
	    data->data_bits == cfg_new->data_bits && data->stop_bits == cfg_new->stop_bits &&
	    data->flow_ctrl == cfg_new->flow_ctrl) {
		return 0;
	}

	unsigned int key = irq_lock();

	usart_disable(cfg->reg);

	usart_parity_config(cfg->reg, parity_bits);
	usart_word_length_set(cfg->reg, word_length);
	usart_stop_bit_set(cfg->reg, stop_bits_hw);
	usart_baudrate_set(cfg->reg, cfg_new->baudrate);

	usart_receive_config(cfg->reg, USART_RECEIVE_ENABLE);
	usart_transmit_config(cfg->reg, USART_TRANSMIT_ENABLE);
	usart_enable(cfg->reg);

	irq_unlock(key);

	data->baud_rate = cfg_new->baudrate;
	data->parity = cfg_new->parity;
	data->data_bits = cfg_new->data_bits;
	data->stop_bits = cfg_new->stop_bits;
	data->flow_ctrl = cfg_new->flow_ctrl;

	return 0;
}

static int usart_gd32_config_get(const struct device *dev, struct uart_config *cfg_out)
{
	struct gd32_usart_data *const data = dev->data;

	if (cfg_out == NULL) {
		return -EINVAL;
	}

	if (!data->initialized) {
		return -ENODEV;
	}

	cfg_out->baudrate = data->baud_rate;
	cfg_out->parity = data->parity;
	cfg_out->stop_bits = data->stop_bits;
	cfg_out->data_bits = data->data_bits;
	cfg_out->flow_ctrl = data->flow_ctrl;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
int usart_gd32_fifo_fill(const struct device *dev, const uint8_t *tx_data,
			 int len)
{
	const struct gd32_usart_config *const cfg = dev->config;
	int num_tx = 0U;

	while ((len - num_tx > 0) &&
	       usart_flag_get(cfg->reg, USART_FLAG_TBE)) {
		usart_data_transmit(cfg->reg, tx_data[num_tx++]);
	}

	return num_tx;
}

int usart_gd32_fifo_read(const struct device *dev, uint8_t *rx_data,
			 const int size)
{
	const struct gd32_usart_config *const cfg = dev->config;
	int num_rx = 0U;

	while ((size - num_rx > 0) &&
	       usart_flag_get(cfg->reg, USART_FLAG_RBNE)) {
		rx_data[num_rx++] = usart_data_receive(cfg->reg);
	}

	return num_rx;
}

void usart_gd32_irq_tx_enable(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	usart_interrupt_enable(cfg->reg, USART_INT_TC);
}

void usart_gd32_irq_tx_disable(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	usart_interrupt_disable(cfg->reg, USART_INT_TC);
}

int usart_gd32_irq_tx_ready(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	return usart_flag_get(cfg->reg, USART_FLAG_TBE) &&
	       usart_interrupt_flag_get(cfg->reg, USART_INT_FLAG_TC);
}

int usart_gd32_irq_tx_complete(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	return usart_flag_get(cfg->reg, USART_FLAG_TC);
}

void usart_gd32_irq_rx_enable(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	usart_interrupt_enable(cfg->reg, USART_INT_RBNE);
}

void usart_gd32_irq_rx_disable(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	usart_interrupt_disable(cfg->reg, USART_INT_RBNE);
}

int usart_gd32_irq_rx_ready(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	return usart_flag_get(cfg->reg, USART_FLAG_RBNE);
}

void usart_gd32_irq_err_enable(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	usart_interrupt_enable(cfg->reg, USART_INT_ERR);
	usart_interrupt_enable(cfg->reg, USART_INT_PERR);
}

void usart_gd32_irq_err_disable(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	usart_interrupt_disable(cfg->reg, USART_INT_ERR);
	usart_interrupt_disable(cfg->reg, USART_INT_PERR);
}

int usart_gd32_irq_is_pending(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;

	return ((usart_flag_get(cfg->reg, USART_FLAG_RBNE) &&
		 usart_interrupt_flag_get(cfg->reg, USART_INT_FLAG_RBNE)) ||
		(usart_flag_get(cfg->reg, USART_FLAG_TC) &&
		 usart_interrupt_flag_get(cfg->reg, USART_INT_FLAG_TC)));
}

void usart_gd32_irq_callback_set(const struct device *dev,
				 uart_irq_callback_user_data_t cb,
				 void *user_data)
{
	struct gd32_usart_data *const data = dev->data;

	data->user_cb = cb;
	data->user_data = user_data;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, usart_gd32_driver_api) = {
	.poll_in = usart_gd32_poll_in,
	.poll_out = usart_gd32_poll_out,
	.err_check = usart_gd32_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = usart_gd32_configure,
	.config_get = usart_gd32_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = usart_gd32_callback_set,
	.tx = usart_gd32_tx,
	.tx_abort = usart_gd32_tx_abort,
	.rx_enable = usart_gd32_rx_enable,
	.rx_disable = usart_gd32_rx_disable,
	.rx_buf_rsp = usart_gd32_rx_buf_rsp,
#endif /* CONFIG_UART_ASYNC_API */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = usart_gd32_fifo_fill,
	.fifo_read = usart_gd32_fifo_read,
	.irq_tx_enable = usart_gd32_irq_tx_enable,
	.irq_tx_disable = usart_gd32_irq_tx_disable,
	.irq_tx_ready = usart_gd32_irq_tx_ready,
	.irq_tx_complete = usart_gd32_irq_tx_complete,
	.irq_rx_enable = usart_gd32_irq_rx_enable,
	.irq_rx_disable = usart_gd32_irq_rx_disable,
	.irq_rx_ready = usart_gd32_irq_rx_ready,
	.irq_err_enable = usart_gd32_irq_err_enable,
	.irq_err_disable = usart_gd32_irq_err_disable,
	.irq_is_pending = usart_gd32_irq_is_pending,
	.irq_callback_set = usart_gd32_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define GD32_USART_IRQ_HANDLER(n)						\
	static void usart_gd32_config_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    usart_gd32_isr,					\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
		irq_enable(DT_INST_IRQN(n));					\
	}
#define GD32_USART_IRQ_HANDLER_FUNC_INIT(n)					\
	.irq_config_func = usart_gd32_config_func_##n
#else /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */
#define GD32_USART_IRQ_HANDLER(n)
#define GD32_USART_IRQ_HANDLER_FUNC_INIT(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_ASYNC_API
#define GD32_USART_DMA_INITIALIZER(idx, dir)					\
	{									\
		.dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(idx, dir)),	\
		.channel = DT_INST_DMAS_CELL_BY_NAME(idx, dir, channel),		\
		.slot = COND_CODE_1(						\
			DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1),		\
			(DT_INST_DMAS_CELL_BY_NAME(idx, dir, slot)), (0)),	\
		.config = DT_INST_DMAS_CELL_BY_NAME(idx, dir, config),		\
		}

#define GD32_USART_DMA_INIT(idx, dir)						\
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, dir),				\
		    (GD32_USART_DMA_INITIALIZER(idx, dir)), ({0}))

#define GD32_USART_DMA_CONFIG_INIT(n)						\
	.dma_tx = GD32_USART_DMA_INIT(n, tx),					\
	.dma_rx = GD32_USART_DMA_INIT(n, rx),
#else
#define GD32_USART_DMA_CONFIG_INIT(n)
#endif /* CONFIG_UART_ASYNC_API */

#define GD32_USART_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);						\
	GD32_USART_IRQ_HANDLER(n)						\
	static struct gd32_usart_data usart_gd32_data_##n = {			\
		.baud_rate = DT_INST_PROP(n, current_speed),			\
	};									\
	static const struct gd32_usart_config usart_gd32_config_##n = {		\
		.reg = DT_INST_REG_ADDR(n),					\
		.clkid = DT_INST_CLOCKS_CELL(n, id),				\
		.reset = RESET_DT_SPEC_INST_GET(n),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.parity = DT_INST_ENUM_IDX(n, parity),				\
		GD32_USART_DMA_CONFIG_INIT(n)					\
		 GD32_USART_IRQ_HANDLER_FUNC_INIT(n)				\
	};									\
	DEVICE_DT_INST_DEFINE(n, usart_gd32_init,				\
			      NULL,						\
			      &usart_gd32_data_##n,				\
			      &usart_gd32_config_##n, PRE_KERNEL_1,		\
			      CONFIG_SERIAL_INIT_PRIORITY,			\
			      &usart_gd32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GD32_USART_INIT)
