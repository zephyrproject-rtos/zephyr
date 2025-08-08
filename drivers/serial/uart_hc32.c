/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xhsc_hc32_uart

/**
 * @brief Driver for UART port on HC32 family processor.
 * @note
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/drivers/interrupt_controller/intc_hc32.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/hc32_clock_control.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>

#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma/dma_hc32.h>
#include <zephyr/drivers/dma.h>
#endif

#include <zephyr/linker/sections.h>
#include "uart_hc32.h"

#include <hc32_ll_usart.h>
#include <hc32_ll_interrupts.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(uart_hc32, CONFIG_UART_LOG_LEVEL);

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if HC32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define HC32_UART_DOMAIN_CLOCK_SUPPORT 1
#else
#define HC32_UART_DOMAIN_CLOCK_SUPPORT 0
#endif

static inline void uart_hc32_set_parity(const struct device *dev,
					 uint32_t parity)
{
	const struct uart_hc32_config *config = dev->config;

	USART_SetParity(config->usart, parity);
}

static inline uint32_t uart_hc32_get_parity(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetParity(config->usart);
}

static inline void uart_hc32_set_stopbits(const struct device *dev,
					   uint32_t stopbits)
{
	const struct uart_hc32_config *config = dev->config;

	USART_SetStopBit(config->usart, stopbits);
}

static inline uint32_t uart_hc32_get_stopbits(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetStopBit(config->usart);
}

static inline void uart_hc32_set_databits(const struct device *dev,
					   uint32_t databits)
{
	const struct uart_hc32_config *config = dev->config;

	USART_SetDataWidth(config->usart, databits);
}

static inline uint32_t uart_hc32_get_databits(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetDataWidth(config->usart);
}

static inline void uart_hc32_set_hwctrl(const struct device *dev,
					 uint32_t hwctrl)
{
	const struct uart_hc32_config *config = dev->config;

	USART_SetHWFlowControl(config->usart, hwctrl);
}

static inline uint32_t uart_hc32_get_hwctrl(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetHWFlowControl(config->usart);
}

static inline void uart_hc32_set_baudrate(const struct device *dev,
					 uint32_t baud_rate)
{
	const struct uart_hc32_config *config = dev->config;

	(void)USART_SetBaudrate(config->usart, baud_rate, NULL);
}

static inline uint32_t uart_hc32_cfg2ll_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return USART_PARITY_ODD;
	case UART_CFG_PARITY_EVEN:
		return USART_PARITY_EVEN;
	case UART_CFG_PARITY_NONE:
	default:
		return USART_PARITY_NONE;
	}
}

static inline enum uart_config_parity uart_hc32_ll2cfg_parity(uint32_t parity)
{
	switch (parity) {
	case USART_PARITY_ODD:
		return UART_CFG_PARITY_ODD;
	case USART_PARITY_EVEN:
		return UART_CFG_PARITY_EVEN;
	case USART_PARITY_NONE:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static inline uint32_t uart_hc32_cfg2ll_stopbits(
		enum uart_config_stop_bits sb)
{
	switch (sb) {
	/* Some MCU's don't support 0.5 stop bits */
#ifdef USART_STOPBIT_2BIT
	case UART_CFG_STOP_BITS_2:
		return USART_STOPBIT_2BIT;
#endif	/* USART_STOPBIT_2BIT */

#ifdef USART_STOPBIT_1BIT
	case UART_CFG_STOP_BITS_1:
#endif	/* USART_STOPBIT_1BIT */
	default:
		return USART_STOPBIT_1BIT;
	}
}

static inline enum uart_config_stop_bits uart_hc32_ll2cfg_stopbits(uint32_t sb)
{
	switch (sb) {
#ifdef USART_STOPBIT_2BIT
	case USART_STOPBIT_2BIT:
		return UART_CFG_STOP_BITS_2;
#endif	/* USART_STOPBIT_1BIT */

#ifdef USART_STOPBIT_1BIT
		case USART_STOPBIT_1BIT:
#endif	/* USART_STOPBIT_1BIT */
		default:
			return UART_CFG_STOP_BITS_1;
	}
}

static inline uint32_t uart_hc32_cfg2ll_databits(enum uart_config_data_bits db)
{
	switch (db) {
/* Some MCU's don't support 9B datawidth */
#ifdef USART_DATA_WIDTH_9BIT
	case UART_CFG_DATA_BITS_9:
		return USART_DATA_WIDTH_9BIT;
#endif	/* USART_DATA_WIDTH_9BIT */
	case UART_CFG_DATA_BITS_8:
	default:
		return USART_DATA_WIDTH_8BIT;
	}
}

static inline enum
	uart_config_data_bits uart_hc32_ll2cfg_databits(uint32_t db)
{
	switch (db) {
/* Some MCU's don't support 9B datawidth */
#ifdef USART_DATA_WIDTH_9BIT
	case USART_DATA_WIDTH_9BIT:
		return UART_CFG_DATA_BITS_9;
#endif	/* USART_DATA_WIDTH_9BIT */
	case USART_DATA_WIDTH_8BIT:
	default:
		return UART_CFG_DATA_BITS_8;
	}
}

/**
 * @brief  Get DDL hardware flow control define from
 *         Zephyr hardware flow control option.
 * @note   Supports only UART_CFG_FLOW_CTRL_RTS_CTS and UART_CFG_FLOW_CTRL_RS485.
 * @param  fc: Zephyr hardware flow control option.
 * @retval USART_HW_FLOWCTRL_RTS, for device under supporting RTS_CTS.
 */
static inline uint32_t uart_hc32_cfg2ll_hwctrl(enum uart_config_flow_control fc)
{
	/* default config */
	return USART_HW_FLOWCTRL_RTS;
}

/**
 * @brief  Get Zephyr hardware flow control option from
 *         DDL hardware flow control define.
 * @note
 * @param  fc: DDL hardware flow control definition.
 * @retval UART_CFG_FLOW_CTRL_RTS_CTS, or UART_CFG_FLOW_CTRL_NONE.
 */
static inline enum uart_config_flow_control uart_hc32_ll2cfg_hwctrl(uint32_t fc)
{
	/* DDL driver compatible with cfg, just return none */
	return UART_CFG_FLOW_CTRL_NONE;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_hc32_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	const struct uart_hc32_config *config = dev->config;
	struct uart_hc32_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;
	const uint32_t parity = uart_hc32_cfg2ll_parity(cfg->parity);
	const uint32_t stopbits = uart_hc32_cfg2ll_stopbits(cfg->stop_bits);
	const uint32_t databits = uart_hc32_cfg2ll_databits(cfg->data_bits);

	/* Hardware doesn't support mark or space parity */
	if ((cfg->parity == UART_CFG_PARITY_MARK) ||
		(cfg->parity == UART_CFG_PARITY_SPACE)) {
		return -ENOTSUP;
	}

	/* Driver does not supports parity + 9 databits */
	if ((cfg->parity != UART_CFG_PARITY_NONE) &&
		(cfg->data_bits == USART_DATA_WIDTH_9BIT)) {
		return -ENOTSUP;
	}

	/* When the transformed ddl parity don't match with what was requested,
	 * then it's not supported
	 */
	if (uart_hc32_ll2cfg_parity(parity) != cfg->parity) {
		return -ENOTSUP;
	}

	/* When the transformed ddl stop bits don't match with what was requested,
	 * then it's not supported
	 */
	if (uart_hc32_ll2cfg_stopbits(stopbits) != cfg->stop_bits) {
		return -ENOTSUP;
	}

	/* When the transformed ddl databits don't match with what was requested,
	 * then it's not supported
	 */
	if (uart_hc32_ll2cfg_databits(databits) != cfg->data_bits) {
		return -ENOTSUP;
	}

	USART_FuncCmd(config->usart, USART_TX | USART_RX, DISABLE);
	uart_hc32_set_parity(dev, parity);
	uart_hc32_set_stopbits(dev, stopbits);
	uart_hc32_set_databits(dev, databits);
	uart_hc32_set_baudrate(dev, uart_cfg->baudrate);
	USART_FuncCmd(config->usart, USART_TX | USART_RX, ENABLE);

	return 0;
}

static int uart_hc32_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct uart_hc32_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;

	cfg->baudrate = uart_cfg->baudrate;
	cfg->parity = uart_hc32_ll2cfg_parity(uart_hc32_get_parity(dev));
	cfg->stop_bits = uart_hc32_ll2cfg_stopbits(uart_hc32_get_stopbits(dev));
	cfg->data_bits = uart_hc32_ll2cfg_databits(uart_hc32_get_databits(dev));
	cfg->flow_ctrl = uart_hc32_ll2cfg_hwctrl(uart_hc32_get_hwctrl(dev));

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_hc32_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_hc32_config *config = dev->config;

	if (USART_GetStatus(config->usart,
							USART_FLAG_OVERRUN | USART_FLAG_RX_TIMEOUT)) {
		USART_ClearStatus(config->usart,
							USART_FLAG_OVERRUN | USART_FLAG_FRAME_ERR);
	}

	if (SET != USART_GetStatus(config->usart, USART_FLAG_RX_FULL))
	{
		return EIO;
	}

	*c = (unsigned char)USART_ReadData(config->usart);
	return 0;
}

static void uart_hc32_poll_out(const struct device *dev, unsigned char c)
{
	unsigned int key;
	const struct uart_hc32_config *config = dev->config;

	key = irq_lock();
	while (1)
	{
		if (USART_GetStatus(config->usart, USART_FLAG_TX_EMPTY)) {
			break;
		}
	}

	USART_WriteData(config->usart, (uint16_t)c);
	irq_unlock(key);
}

static int uart_hc32_err_check(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;
	uint32_t err = 0U;

	if (USART_GetStatus(config->usart, USART_FLAG_OVERRUN)) {
		err |= UART_ERROR_OVERRUN;
	}

	if (USART_GetStatus(config->usart, USART_FLAG_FRAME_ERR)) {
		err |= UART_ERROR_FRAMING;
	}

	if (USART_GetStatus(config->usart, USART_FLAG_PARITY_ERR)) {
		err |= UART_ERROR_PARITY;
	}

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

typedef void (*fifo_fill_fn)(const struct device *dev, const void *tx_data,
				 const uint8_t offset);

static int uart_hc32_fifo_fill_visitor(const struct device *dev,
			const void *tx_data, int size, fifo_fill_fn fill_fn)
{
	const struct uart_hc32_config *config = dev->config;
	uint8_t num_tx = 0U;
	unsigned int key;

	if (!USART_GetStatus(config->usart, USART_FLAG_TX_EMPTY)) {
		return num_tx;
	}

	/* Lock interrupts to prevent nested interrupts or thread switch */
	key = irq_lock();

	/*
	TXE flag will be set by hardware when moving data
	from DR register to Shift register
	*/
	while ((size - num_tx > 0) &&
		USART_GetStatus(config->usart, USART_FLAG_TX_EMPTY)) {
		/* Send a character */
		fill_fn(dev, tx_data, num_tx);
		num_tx++;
	}

	irq_unlock(key);

	return num_tx;
}

static void fifo_fill_with_u8(const struct device *dev,
					 const void *tx_data, const uint8_t offset)
{
	const uint8_t *data = (const uint8_t *)tx_data;
	/* Send a character (8bit) */
	uart_hc32_poll_out(dev, data[offset]);
}

static int uart_hc32_fifo_fill(const struct device *dev,
			const uint8_t *tx_data, int size)
{
	if (UART_CFG_DATA_BITS_9 ==
			uart_hc32_ll2cfg_databits(uart_hc32_get_databits(dev))) {
		return -ENOTSUP;
	}
	return uart_hc32_fifo_fill_visitor(dev, (const void *)tx_data, size,
						fifo_fill_with_u8);
}

typedef void (*fifo_read_fn)(const struct uart_hc32_config *config, void *rx_data,
				 const uint8_t offset);

static int uart_hc32_fifo_read_visitor(const struct device *dev,
			void *rx_data, const int size, fifo_read_fn read_fn)
{
	const struct uart_hc32_config *config = dev->config;
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) &&
			USART_GetStatus(config->usart, USART_FLAG_RX_FULL)) {
		/* RXNE flag will be cleared upon read from RDR register */

		read_fn(config, rx_data, num_rx);
		num_rx++;

		/* Clear overrun error flag */
		if (USART_GetStatus(config->usart, USART_FLAG_OVERRUN)) {
			USART_ClearStatus(config->usart, USART_FLAG_OVERRUN);
		}
	}

	return num_rx;
}

static void fifo_read_with_u8(const struct uart_hc32_config *config,
				void *rx_data, const uint8_t offset)
{
	uint8_t *data = (uint8_t *)rx_data;

	USART_UART_Receive(config->usart, &data[offset], 1U, 0xFFU);
}

static int uart_hc32_fifo_read(const struct device *dev,
			uint8_t *rx_data, const int size)
{
	if (UART_CFG_DATA_BITS_9 ==
			uart_hc32_ll2cfg_databits(uart_hc32_get_databits(dev))) {
		return -ENOTSUP;
	}
	return uart_hc32_fifo_read_visitor(dev, (void *)rx_data, size,
						fifo_read_with_u8);
}

static void uart_hc32_irq_tx_enable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	USART_FuncCmd(config->usart,
		USART_INT_TX_EMPTY | USART_INT_TX_CPLT, ENABLE);
}

static void uart_hc32_irq_tx_disable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	USART_FuncCmd(config->usart,
		USART_INT_TX_EMPTY | USART_INT_TX_CPLT, DISABLE);
}

static int uart_hc32_irq_tx_ready(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetStatus(config->usart, USART_FLAG_TX_EMPTY);
}

static int uart_hc32_irq_tx_complete(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetStatus(config->usart, USART_FLAG_TX_CPLT);
}

static void uart_hc32_irq_rx_enable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	USART_FuncCmd(config->usart, USART_INT_RX, ENABLE);
}

static void uart_hc32_irq_rx_disable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	USART_FuncCmd(config->usart, USART_INT_RX, DISABLE);
}

static int uart_hc32_irq_rx_ready(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetStatus(config->usart, USART_FLAG_RX_FULL);
}

static void uart_hc32_irq_err_enable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	/* Enable FE, ORE parity error interruption */
	USART_FuncCmd(config->usart, USART_RX_TIMEOUT |
					 USART_INT_RX_TIMEOUT, ENABLE);
}

static void uart_hc32_irq_err_disable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	USART_FuncCmd(config->usart, USART_RX_TIMEOUT |
					 USART_INT_RX_TIMEOUT, DISABLE);
}

static int uart_hc32_irq_is_pending(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetStatus(config->usart, USART_FLAG_ALL);
}

static int uart_hc32_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

/*
  user may use void *user_data = x to specify irq type, the x may be:
  0：usart_hc32f4_rx_error_isr
  1：usart_hc32f4_rx_full_isr
  2：usart_hc32f4_tx_empty_isr
  3：usart_hc32f4_tx_complete_isr
  4：usart_hc32f4_rx_timeout_isr
  others or user_data = NULL：set cb for all interrupt handlers
*/
static void uart_hc32_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *user_data)
{
	uint32_t i;
	struct uart_hc32_data *data = dev->data;

	if ((user_data == NULL) || (*(uint32_t *)user_data >= UART_INT_NUM)) {
		for (i = 0; i< UART_INT_NUM; i++) {
			data->cb[i].user_cb = cb;
			data->cb[i].user_data = user_data;
		}
	} else {
		i = *(uint32_t *)user_data;
		data->cb[i].user_cb = cb;
		data->cb[i].user_data = user_data;
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API

static inline void async_user_callback(struct uart_hc32_data *data,
				struct uart_event *event)
{
	if (data->async_cb) {
		data->async_cb(data->uart_dev, event, data->async_user_data);
	}
}

static inline void async_evt_rx_rdy(struct uart_hc32_data *data)
{
	LOG_DBG("rx_rdy: (%d %d)", data->dma_rx.offset, data->dma_rx.counter);

	struct uart_event event = {
		.type = UART_RX_RDY,
		.data.rx.buf = data->dma_rx.buffer,
		.data.rx.len = data->dma_rx.counter - data->dma_rx.offset,
		.data.rx.offset = data->dma_rx.offset
	};

	/* update the current pos for new data */
	data->dma_rx.offset = data->dma_rx.counter;

	/* send event only for new data */
	if (event.data.rx.len > 0) {
		async_user_callback(data, &event);
	}
}

static inline void async_evt_rx_err(struct uart_hc32_data *data, int err_code)
{
	LOG_DBG("rx error: %d", err_code);

	struct uart_event event = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = err_code,
		.data.rx_stop.data.len = data->dma_rx.counter,
		.data.rx_stop.data.offset = 0,
		.data.rx_stop.data.buf = data->dma_rx.buffer
	};

	async_user_callback(data, &event);
}

static inline void async_evt_tx_done(struct uart_hc32_data *data)
{
	LOG_DBG("tx done: %d", data->dma_tx.counter);

	struct uart_event event = {
		.type = UART_TX_DONE,
		.data.tx.buf = data->dma_tx.buffer,
		.data.tx.len = data->dma_tx.counter
	};

	/* Reset tx buffer */
	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	async_user_callback(data, &event);
}

static inline void async_evt_tx_abort(struct uart_hc32_data *data)
{
	LOG_DBG("tx abort: %d", data->dma_tx.counter);

	struct uart_event event = {
		.type = UART_TX_ABORTED,
		.data.tx.buf = data->dma_tx.buffer,
		.data.tx.len = data->dma_tx.counter
	};

	/* Reset tx buffer */
	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	async_user_callback(data, &event);
}

static inline void async_evt_rx_buf_request(struct uart_hc32_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(data, &evt);
}

static inline void async_evt_rx_buf_release(struct uart_hc32_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->dma_rx.buffer,
	};

	async_user_callback(data, &evt);
}

static inline void async_timer_start(struct k_work_delayable *work,
					 int32_t timeout)
{
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		/* start timer */
		LOG_DBG("async timer started for %d us", timeout);
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static inline void async_timer_restart(struct k_work_delayable *work,
					 int32_t timeout, void *fn)
{
	if (timeout != 0) {
		(void)k_work_cancel_delayable(work);
		k_work_init_delayable(work, fn);
		LOG_DBG("async timer re-started for %d us", timeout);
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static void uart_hc32_dma_rx_flush(const struct device *dev)
{
	struct dma_status stat;
	struct uart_hc32_data *data = dev->data;

	if (dma_get_status(data->dma_rx.dma_dev,
				data->dma_rx.dma_channel, &stat) == 0) {
		size_t rx_rcv_len = data->dma_rx.buffer_length -
					stat.pending_length;
		if (rx_rcv_len > data->dma_rx.offset) {
			data->dma_rx.counter = rx_rcv_len;

			async_evt_rx_rdy(data);
		}
	}
}

static int uart_hc32_async_callback_set(const struct device *dev,
					 uart_callback_t callback,
					 void *user_data)
{
	struct uart_hc32_data *data = dev->data;
	uint8_t i = 0;
#if CONFIG_UART_INTERRUPT_DRIVEN
	for(i = 0; i < UART_INT_NUM; i++)
	{
		data->cb[i].user_cb = NULL;
		data->cb[i].user_data = NULL;
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->async_cb = callback;
	data->async_user_data = user_data;
#endif/* CONFIG_UART_EXCLUSIVE_API_CALLBACKS */

	return 0;
}

static inline void uart_hc32_dma_tx_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static inline void uart_hc32_dma_rx_enable(const struct device *dev)
{
	struct uart_hc32_data *data = dev->data;

	data->dma_rx.enabled = true;
}

static inline void uart_hc32_dma_rx_disable(const struct device *dev)
{
	struct uart_hc32_data *data = dev->data;

	data->dma_rx.enabled = false;
}

static int uart_hc32_async_rx_disable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;
	struct uart_hc32_data *data = dev->data;
	struct uart_event disabled_event = {
		.type = UART_RX_DISABLED
	};

	if (!data->dma_rx.enabled) {
		async_user_callback(data, &disabled_event);
		return -EFAULT;
	}

	uart_hc32_dma_rx_flush(dev);

	async_evt_rx_buf_release(data);

	uart_hc32_dma_rx_disable(dev);

	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);

	dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	if (data->rx_next_buffer) {
		struct uart_event rx_next_buf_release_evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = data->rx_next_buffer,
		};
		async_user_callback(data, &rx_next_buf_release_evt);
	}

	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	/* When async rx is disabled, enable instance of uart to function normally */
	USART_FuncCmd(config->usart, USART_INT_RX, ENABLE);

	LOG_DBG("rx: disabled");

	async_user_callback(data, &disabled_event);

	return 0;
}

void uart_hc32_dma_tx_cb(const struct device *dma_dev, void *user_data,
				   uint32_t channel, int status)
{
	struct dma_hc32_config_user_data *cfg = user_data;
	struct device *uart_dev = cfg->user_data;
	struct uart_hc32_data *data = uart_dev->data;
	struct dma_status stat;

	unsigned int key = irq_lock();

	/* Disable TX */
	uart_hc32_dma_tx_disable(uart_dev);

	(void)k_work_cancel_delayable(&data->dma_tx.timeout_work);

	if (!dma_get_status(data->dma_tx.dma_dev,
				data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = data->dma_tx.buffer_length -
					stat.pending_length;
	}

	data->dma_tx.buffer_length = 0;

	async_evt_tx_done(data);

	irq_unlock(key);
}

static void uart_hc32_dma_replace_buffer(const struct device *dev)
{
	struct uart_hc32_data *data = dev->data;

	/* Replace the buffer and reload the DMA */
	LOG_DBG("Replacing RX buffer: %d", data->rx_next_buffer_len);

	/* reload DMA */
	data->dma_rx.offset = 0;
	data->dma_rx.counter = 0;
	data->dma_rx.buffer = data->rx_next_buffer;
	data->dma_rx.buffer_length = data->rx_next_buffer_len;
	data->dma_rx.blk_cfg.block_size = data->dma_rx.buffer_length;
	data->dma_rx.blk_cfg.dest_address = (uint32_t)data->dma_rx.buffer;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	dma_reload(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
			data->dma_rx.blk_cfg.source_address,
			data->dma_rx.blk_cfg.dest_address,
			data->dma_rx.blk_cfg.block_size);

	dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	/* Request next buffer */
	async_evt_rx_buf_request(data);
}

static int uart_hc32_async_tx_abort(const struct device *dev)
{
	struct uart_hc32_data *data = dev->data;
	size_t tx_buffer_length = data->dma_tx.buffer_length;
	struct dma_status stat;

	if (tx_buffer_length == 0) {
		return -EFAULT;
	}

	(void)k_work_cancel_delayable(&data->dma_tx.timeout_work);
	if (!dma_get_status(data->dma_tx.dma_dev,
				data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = tx_buffer_length - stat.pending_length;
	}

	dma_suspend(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	async_evt_tx_abort(data);

	return 0;
}

static void uart_hc32_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct hc32_dma_cfg *rx_stream = CONTAINER_OF(dwork,
			struct hc32_dma_cfg, timeout_work);
	struct uart_hc32_data *data = CONTAINER_OF(rx_stream,
			struct uart_hc32_data, dma_rx);
	const struct device *dev = data->uart_dev;

	LOG_DBG("rx timeout");

	if (data->dma_rx.counter == data->dma_rx.buffer_length) {
		uart_hc32_async_rx_disable(dev);
	} else {
		uart_hc32_dma_rx_flush(dev);
		async_timer_restart(&data->dma_rx.timeout_work, data->dma_rx.timeout,
			uart_hc32_async_rx_timeout);
	}
}

static void uart_hc32_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct hc32_dma_cfg *tx_stream = CONTAINER_OF(dwork,
			struct hc32_dma_cfg, timeout_work);
	struct uart_hc32_data *data = CONTAINER_OF(tx_stream,
			struct uart_hc32_data, dma_tx);
	const struct device *dev = data->uart_dev;

	uart_hc32_async_tx_abort(dev);

	LOG_DBG("tx: async timeout");

	async_timer_restart(&data->dma_tx.timeout_work, data->dma_tx.timeout,
			uart_hc32_async_tx_timeout);
}

void uart_hc32_dma_rx_cb(const struct device *dma_dev, void *user_data,
				   uint32_t channel, int status)
{
	struct dma_hc32_config_user_data *cfg = user_data;
	struct device *uart_dev = cfg->user_data;
	struct uart_hc32_data *data = uart_dev->data;

	if (status < 0) {
		async_evt_rx_err(data, status);
		return;
	}

	/* true since this functions occurs when buffer if full */
	data->dma_rx.counter = data->dma_rx.buffer_length;

	async_evt_rx_rdy(data);

	if (data->rx_next_buffer != NULL) {
		async_evt_rx_buf_release(data);
		uart_hc32_dma_replace_buffer(uart_dev);
		async_timer_restart(&data->dma_rx.timeout_work,
			data->dma_tx.timeout, uart_hc32_async_rx_timeout);
	} else {
		(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);
	}
}

static int uart_hc32_async_tx(const struct device *dev,
		const uint8_t *tx_data, size_t buf_size, int32_t timeout)
{
	const struct uart_hc32_config *config = dev->config;
	struct uart_hc32_data *data = dev->data;
	CM_USART_TypeDef *USARTx = ((CM_USART_TypeDef *)config->usart);
	int ret;

	if (data->dma_tx.dma_dev == NULL) {
		return -ENODEV;
	}

	if (data->dma_tx.buffer_length != 0) {
		return -EBUSY;
	}

	data->dma_tx.buffer = (uint8_t *)tx_data;
	data->dma_tx.buffer_length = buf_size;
	data->dma_tx.timeout = timeout;

	LOG_DBG("tx: l=%d", data->dma_tx.buffer_length);
	USART_FuncCmd(config->usart, USART_INT_TX_EMPTY, DISABLE);
	/*
	TC flag = 1 after init and not generate TC event request.
	To create TC event request disabling tx before dma configuration and
	enable tx after dma start
	*/
	USART_FuncCmd(config->usart, USART_TX, DISABLE);

	/* set source address */
	data->dma_tx.blk_cfg.source_address = (uint32_t)data->dma_tx.buffer;
	data->dma_tx.blk_cfg.dest_address = (uint32_t)&USARTx->TDR;
	data->dma_tx.blk_cfg.block_size = data->dma_tx.buffer_length;

	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.dma_channel,
				&data->dma_tx.dma_cfg);

	if (ret != 0) {
		LOG_ERR("dma tx config error!");
		return -EINVAL;
	}

	/* Start and enable TX DMA requests */
	if (dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel)) {
		LOG_ERR("UART err: TX DMA start failed!");
		return -EFAULT;
	}

	/*
	TC flag = 1 after init and not generate TC event request.
	To create TC event request disabling tx before dma configuration and
	enable tx after dma start
	*/
	USART_FuncCmd(config->usart, USART_TX, ENABLE);

	/* Start TX timer */
	async_timer_start(&data->dma_tx.timeout_work, data->dma_tx.timeout);

	return 0;
}

static int uart_hc32_async_rx_enable(const struct device *dev,
		uint8_t *rx_buf, size_t buf_size, int32_t timeout)
{
	const struct uart_hc32_config *config = dev->config;
	struct uart_hc32_data *data = dev->data;
	CM_USART_TypeDef *USARTx = ((CM_USART_TypeDef *)config->usart);
	int ret;

	if (data->dma_rx.dma_dev == NULL) {
		return -ENODEV;
	}

	if (data->dma_rx.enabled) {
		LOG_WRN("RX was already enabled");
		return -EBUSY;
	}

	USART_FuncCmd(config->usart, USART_INT_RX, DISABLE);

	data->dma_rx.offset = 0;
	data->dma_rx.buffer = rx_buf;
	data->dma_rx.buffer_length = buf_size;
	data->dma_rx.counter = 0;
	data->dma_rx.timeout = timeout;

	data->dma_rx.blk_cfg.block_size = buf_size;
	data->dma_rx.blk_cfg.dest_address = (uint32_t)data->dma_rx.buffer;
	data->dma_rx.blk_cfg.source_address = (uint32_t)&USARTx->RDR;

	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
				&(data->dma_rx.dma_cfg));

	if (ret != 0) {
		LOG_ERR("UART ERR: RX DMA config failed!");
		return -EINVAL;
	}

	if (dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel)) {
		LOG_ERR("UART ERR: RX DMA start failed!");
		return -EFAULT;
	}

	/* Enable RX DMA requests */
	uart_hc32_dma_rx_enable(dev);

	USART_FuncCmd(config->usart, USART_RX, ENABLE);

	/* Request next buffer */
	async_evt_rx_buf_request(data);

	async_timer_start(&data->dma_rx.timeout_work, data->dma_rx.timeout);

	LOG_DBG("async rx enabled");

	return ret;
}

static int uart_hc32_async_rx_buf_rsp(const struct device *dev, uint8_t *buf,
					   size_t len)
{
	struct uart_hc32_data *data = dev->data;

	LOG_DBG("replace buffer (%d)", len);
	data->rx_next_buffer = buf;
	data->rx_next_buffer_len = len;

	return 0;
}

static int uart_hc32_async_init(const struct device *dev)
{
	struct uart_hc32_data *data = dev->data;

	data->uart_dev = dev;

	if (data->dma_rx.dma_dev != NULL) {
		if (!device_is_ready(data->dma_rx.dma_dev)) {
			return -ENODEV;
		}
	}

	if (data->dma_tx.dma_dev != NULL) {
		if (!device_is_ready(data->dma_tx.dma_dev)) {
			return -ENODEV;
		}
	}

	/* Disable both TX and RX DMA requests */
	uart_hc32_dma_rx_disable(dev);
	uart_hc32_dma_tx_disable(dev);

	/* Configure dma rx config */
	memset(&data->dma_rx.blk_cfg, 0U, sizeof(data->dma_rx.blk_cfg));

	data->dma_rx.blk_cfg.dest_address = 0; /* dest not ready */
	data->dma_rx.blk_cfg.source_addr_adj = data->dma_rx.src_addr_increment;
	data->dma_rx.blk_cfg.dest_addr_adj = data->dma_rx.dst_addr_increment;

	/* RX disable circular buffer */
	data->dma_rx.blk_cfg.source_reload_en  = 0;
	data->dma_rx.blk_cfg.dest_reload_en = 0;

	data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;

	data->dma_rx.user_cfg.user_data = (void *)dev;
	data->dma_rx.dma_cfg.user_data = (void *)&data->dma_rx.user_cfg;

	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	k_work_init_delayable(&data->dma_rx.timeout_work,
							uart_hc32_async_rx_timeout);

	/* Configure dma tx config */
	memset(&data->dma_tx.blk_cfg, 0U, sizeof(data->dma_tx.blk_cfg));

	data->dma_tx.blk_cfg.source_address = 0U; /* not ready */
	data->dma_tx.blk_cfg.source_addr_adj = data->dma_tx.src_addr_increment;
	data->dma_tx.blk_cfg.dest_addr_adj = data->dma_tx.dst_addr_increment;
	data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;

	data->dma_tx.user_cfg.user_data = (void *)dev;
	data->dma_tx.dma_cfg.user_data = (void *)&data->dma_tx.user_cfg;
	k_work_init_delayable(&data->dma_tx.timeout_work,
							uart_hc32_async_tx_timeout);

	return 0;
}
#endif /* CONFIG_UART_ASYNC_API */

static const struct uart_driver_api uart_hc32_driver_api = {
	.poll_in = uart_hc32_poll_in,
	.poll_out = uart_hc32_poll_out,
	.err_check = uart_hc32_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_hc32_configure,
	.config_get = uart_hc32_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_hc32_fifo_fill,
	.fifo_read = uart_hc32_fifo_read,
	.irq_tx_enable = uart_hc32_irq_tx_enable,
	.irq_tx_disable = uart_hc32_irq_tx_disable,
	.irq_tx_ready = uart_hc32_irq_tx_ready,
	.irq_tx_complete = uart_hc32_irq_tx_complete,
	.irq_rx_enable = uart_hc32_irq_rx_enable,
	.irq_rx_disable = uart_hc32_irq_rx_disable,
	.irq_rx_ready = uart_hc32_irq_rx_ready,
	.irq_err_enable = uart_hc32_irq_err_enable,
	.irq_err_disable = uart_hc32_irq_err_disable,
	.irq_is_pending = uart_hc32_irq_is_pending,
	.irq_update = uart_hc32_irq_update,
	.irq_callback_set = uart_hc32_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_hc32_async_callback_set,
	.tx = uart_hc32_async_tx,
	.tx_abort = uart_hc32_async_tx_abort,
	.rx_enable = uart_hc32_async_rx_enable,
	.rx_disable = uart_hc32_async_rx_disable,
	.rx_buf_rsp = uart_hc32_async_rx_buf_rsp,
#endif /* CONFIG_UART_ASYNC_API */
};

static int uart_hc32_registers_configure(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;
	struct uart_hc32_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;
	stc_usart_uart_init_t stcUartInit;

	USART_FuncCmd(config->usart, HC32_UART_FUNC, DISABLE);

	(void)USART_UART_StructInit(&stcUartInit);
	stcUartInit.u32ClockDiv = USART_CLK_DIV16;
	stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
	stcUartInit.u32Baudrate = uart_cfg->baudrate;
	stcUartInit.u32StopBit = uart_hc32_cfg2ll_stopbits(uart_cfg->stop_bits);
	stcUartInit.u32Parity = uart_hc32_cfg2ll_parity(uart_cfg->parity);

	(void)USART_UART_Init(config->usart, &stcUartInit, NULL);
	USART_FuncCmd(config->usart, USART_TX | USART_RX, ENABLE);

	return 0;
}


static inline void __uart_hc32_get_clock(const struct device *dev)
{
	struct uart_hc32_data *data = dev->data;
	const struct device *const clk = DEVICE_DT_GET(HC32_CLOCK_CONTROL_NODE);

	data->clock = clk;
}

static int uart_hc32_clocks_enable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;
	struct uart_hc32_data *data = dev->data;
	int err;

	__uart_hc32_get_clock(dev);

	if (!device_is_ready(data->clock)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* enable clock */
	err = clock_control_on(data->clock, (clock_control_subsys_t)config->clk_cfg);
	if (err != 0) {
		LOG_ERR("Could not enable UART clock");
		return err;
	}
	return 0;
}

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_hc32_init(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;
	int err;

	err = uart_hc32_clocks_enable(dev);
	if (err < 0) {
		return err;
	}

	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	err = uart_hc32_registers_configure(dev);
	if (err < 0) {
		return err;
	}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	if ((uart_irq_config_func_t)NULL != config->irq_config_func) {
		config->irq_config_func(dev);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	/* config DMA  */
#ifdef CONFIG_UART_ASYNC_API
	return uart_hc32_async_init(dev);
#else
	return 0;
#endif
}

#if defined(CONFIG_UART_ASYNC_API)

/* src_dev and dest_dev should be 'MEMORY' or 'PERIPHERAL'. */
#define UART_DMA_CHANNEL_INIT(index, dir, dir_cap, src_dev, dest_dev)		\
	.dma_dev = DEVICE_DT_GET(HC32_DMA_CTLR(index, dir)),					\
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),			\
	.dma_cfg = {															\
		.channel_direction = HC32_DMA_CONFIG_DIRECTION(						\
				HC32_DMA_CHANNEL_CONFIG(index, dir)),						\
		.source_data_size = HC32_DMA_CONFIG_DATA_SIZE(						\
				HC32_DMA_CHANNEL_CONFIG(index, dir)),						\
		.dest_data_size = HC32_DMA_CONFIG_DATA_SIZE(						\
				HC32_DMA_CHANNEL_CONFIG(index, dir)),						\
		.source_burst_length = 1, /* SINGLE transfer */						\
		.dest_burst_length = 1,												\
		.block_count = 1,													\
		.dma_callback = uart_hc32_dma_##dir##_cb,							\
	},																		\
	.user_cfg = {															\
		.slot = HC32_DMA_SLOT(index, dir),									\
	},																		\
	.src_addr_increment = HC32_DMA_CONFIG_##src_dev##_ADDR_INC(				\
			HC32_DMA_CHANNEL_CONFIG(index, dir)),							\
	.dst_addr_increment = HC32_DMA_CONFIG_##dest_dev##_ADDR_INC(			\
			HC32_DMA_CHANNEL_CONFIG(index, dir)),
#else
#define HC32_UART_DMA_CFG_DECL(index)
#endif /* CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_ASYNC_API
/* If defined async, let dma deal with transmission */
#define UART_DMA_CHANNEL(index, dir, DIR, src, dest)						\
	.dma_##dir = {															\
		COND_CODE_1(														\
			DT_INST_DMAS_HAS_NAME(index, dir),								\
			(UART_DMA_CHANNEL_INIT(index, dir, DIR, src, dest)),			\
			())																\
	},
#else
#define UART_DMA_CHANNEL(index, dir, DIR, src, dest)
#endif /* CONFIG_UART_ASYNC_API */

#define DT_USART_HAS_INTR(node_id)											\
		IS_ENABLED(DT_CAT4(node_id, _P_, interrupts, _EXISTS))

#define DT_IS_INTR_EXIST(index)												\
		DT_USART_HAS_INTR(DT_INST(index, xhsc_hc32_uart))

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define USART_IRQ_ISR_CONFIG(isr_name_prefix, isr_idx, index)				\
	IRQ_CONNECT(															\
		DT_INST_IRQ_BY_IDX(index, isr_idx, irq),							\
		DT_INST_IRQ_BY_IDX(index, isr_idx, priority),						\
		DT_CAT3(isr_name_prefix, _, index),									\
		DEVICE_DT_INST_GET(index),											\
		0);																	\
	hc32_intc_irq_signin(													\
		DT_INST_IRQ_BY_IDX(index, isr_idx, irq),							\
		DT_INST_IRQ_BY_IDX(index, isr_idx, int_src));						\
	irq_enable(DT_INST_IRQ_BY_IDX(index, isr_idx, irq));

#define HC32_UART_IRQ_HANDLER_DECL(index)									\
	static void usart_hc32_config_func_##index(const struct device *dev);	\
	static void	usart_hc32_rx_error_isr_##index(const struct device *dev);	\
	static void usart_hc32_rx_full_isr_##index(const struct device *dev);	\
	static void usart_hc32_tx_empty_isr_##index(const struct device *dev);	\
	static void usart_hc32_tx_complete_isr_##index(const struct device *dev);

#define HC32_UART_IRQ_HANDLER_DEF(index)									\
	static void usart_hc32_rx_error_isr_##index(const struct device *dev)	\
	{																		\
		struct uart_hc32_data *const data = dev->data;						\
																			\
		if (data->cb[UART_INT_IDX_EI].user_cb) {							\
			data->cb[UART_INT_IDX_EI].user_cb(dev,							\
				data->cb[UART_INT_IDX_EI].user_data);						\
		}																	\
	}																		\
																			\
	static void usart_hc32_rx_full_isr_##index(const struct device *dev)	\
	{																		\
		struct uart_hc32_data *const data = dev->data;						\
																			\
		if (data->cb[UART_INT_IDX_RI].user_cb) {							\
			data->cb[UART_INT_IDX_RI].user_cb(dev,							\
			data->cb[UART_INT_IDX_RI].user_data);							\
		}																	\
	}																		\
																			\
	static void 															\
		usart_hc32_tx_empty_isr_##index(const struct device *dev)			\
	{																		\
		struct uart_hc32_data *const data = dev->data;						\
																			\
		if (data->cb[UART_INT_IDX_TI].user_cb) {							\
			data->cb[UART_INT_IDX_TI].user_cb(dev,							\
				data->cb[UART_INT_IDX_TI].user_data);						\
		}																	\
	}																		\
																			\
	static void usart_hc32_tx_complete_isr_##index(const struct device *dev)\
	{																		\
		struct uart_hc32_data *const data = dev->data;						\
																			\
		if (data->cb[UART_INT_IDX_TCI].user_cb) {							\
			data->cb[UART_INT_IDX_TCI].user_cb(dev,							\
				data->cb[UART_INT_IDX_TCI].user_data);						\
		}																	\
	}																		\
																			\
	static void usart_hc32_config_func_##index(const struct device *dev)	\
	{																		\
		USART_IRQ_ISR_CONFIG(usart_hc32_rx_error_isr, 0, index)				\
		USART_IRQ_ISR_CONFIG(usart_hc32_rx_full_isr, 1, index)				\
		USART_IRQ_ISR_CONFIG(usart_hc32_tx_empty_isr, 2, index)				\
		USART_IRQ_ISR_CONFIG(usart_hc32_tx_complete_isr, 3, index)			\
	}

#define HC32_UART_IRQ_HANDLER_FUNC(index)									\
	.irq_config_func = usart_hc32_config_func_##index,

#define HC32_UART_IRQ_HANDLER_NULL(index)									\
	.irq_config_func = (uart_irq_config_func_t)NULL,

#define HC32_UART_IRQ_HANDLER_PRE_FUNC(index)								\
	COND_CODE_1(															\
			DT_IS_INTR_EXIST(index),										\
			(HC32_UART_IRQ_HANDLER_FUNC(index)),							\
			(HC32_UART_IRQ_HANDLER_NULL(index)))

#define HC32_UART_IRQ_HANDLER_PRE_DECL(index)								\
	COND_CODE_1(															\
			DT_IS_INTR_EXIST(index),										\
			(HC32_UART_IRQ_HANDLER_DECL(index)),							\
			())
#define HC32_UART_IRQ_HANDLER_PRE_DEF(index)								\
	COND_CODE_1(															\
			DT_IS_INTR_EXIST(index),										\
			(HC32_UART_IRQ_HANDLER_DEF(index)),								\
			())
#else /* CONFIG_UART_INTERRUPT_DRIVEN */
#define HC32_UART_IRQ_HANDLER_PRE_DECL(index)
#define HC32_UART_IRQ_HANDLER_PRE_DEF(index)
#define HC32_UART_IRQ_HANDLER_PRE_FUNC(index)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define HC32_UART_CLOCK_DECL(index)										\
static const struct hc32_modules_clock_sys uart_fcg_config_##index[]	\
		= HC32_MODULES_CLOCKS(DT_DRV_INST(index));

#define HC32_UART_INIT(index)											\
	HC32_UART_CLOCK_DECL(index)											\
	HC32_UART_IRQ_HANDLER_PRE_DECL(index)								\
	PINCTRL_DT_INST_DEFINE(index);										\
	static struct uart_config uart_cfg_##index = {						\
		.baudrate  = DT_INST_PROP_OR(index, current_speed,				\
						HC32_UART_DEFAULT_BAUDRATE),					\
		.parity	= DT_INST_ENUM_IDX_OR(index, parity,					\
						HC32_UART_DEFAULT_PARITY),						\
		.stop_bits = DT_INST_ENUM_IDX_OR(index, stop_bits,				\
						HC32_UART_DEFAULT_STOP_BITS),					\
		.data_bits = DT_INST_ENUM_IDX_OR(index, data_bits,				\
						HC32_UART_DEFAULT_DATA_BITS),					\
		.flow_ctrl = DT_INST_PROP(index, hw_flow_control)				\
						? UART_CFG_FLOW_CTRL_RTS_CTS					\
						: UART_CFG_FLOW_CTRL_NONE,						\
	};																	\
	static const struct uart_hc32_config uart_hc32_cfg_##index = {		\
		.usart = (CM_USART_TypeDef *)DT_INST_REG_ADDR(index),			\
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),				\
		.clk_cfg = uart_fcg_config_##index,								\
		HC32_UART_IRQ_HANDLER_PRE_FUNC(index)							\
	};																	\
	static struct uart_hc32_data uart_hc32_data_##index = {				\
		.uart_cfg = &uart_cfg_##index,									\
		UART_DMA_CHANNEL(index, rx, RX, SOURCE, DEST)					\
		UART_DMA_CHANNEL(index, tx, TX, SOURCE, DEST)					\
	};																	\
	DEVICE_DT_INST_DEFINE(index,										\
				&uart_hc32_init,										\
				NULL,													\
				&uart_hc32_data_##index, &uart_hc32_cfg_##index,		\
				PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,				\
				&uart_hc32_driver_api);									\
	HC32_UART_IRQ_HANDLER_PRE_DEF(index)

DT_INST_FOREACH_STATUS_OKAY(HC32_UART_INIT)
