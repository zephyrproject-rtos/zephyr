/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_uart

/**
 * @brief Driver for UART port on STM32 family processor.
 * @note  LPUART and U(S)ART have the same base and
 *        majority of operations are performed the same way.
 *        Please validate for newly added series.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <sys/__assert.h>
#include <soc.h>
#include <init.h>
#include <drivers/uart.h>
#include <drivers/pinmux.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <drivers/clock_control.h>

#ifdef CONFIG_UART_ASYNC_API
#include <dt-bindings/dma/stm32_dma.h>
#include <drivers/dma.h>
#endif

#include <linker/sections.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include "uart_stm32.h"

#include <stm32_ll_usart.h>
#include <stm32_ll_lpuart.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(uart_stm32);

#define HAS_LPUART_1 (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpuart1), \
					 st_stm32_lpuart, okay))

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct uart_stm32_config *const)(dev)->config)
#define DEV_DATA(dev)							\
	((struct uart_stm32_data *const)(dev)->data)
#define UART_STRUCT(dev)					\
	((USART_TypeDef *)(DEV_CFG(dev))->uconf.base)

#define TIMEOUT 1000

static inline void uart_stm32_set_baudrate(const struct device *dev,
					   uint32_t baud_rate)
{
	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	uint32_t clock_rate;

	/* Get clock rate */
	if (clock_control_get_rate(data->clock,
			       (clock_control_subsys_t *)&config->pclken,
			       &clock_rate) < 0) {
		LOG_ERR("Failed call clock_control_get_rate");
		return;
	}


#if HAS_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance)) {
		LL_LPUART_SetBaudRate(UartInstance,
				      clock_rate,
#ifdef USART_PRESC_PRESCALER
				      LL_USART_PRESCALER_DIV1,
#endif
				      baud_rate);
	} else {
#endif /* HAS_LPUART_1 */
#ifdef USART_CR1_OVER8
		LL_USART_SetOverSampling(UartInstance,
					 LL_USART_OVERSAMPLING_16);
#endif
		LL_USART_SetBaudRate(UartInstance,
				     clock_rate,
#ifdef USART_PRESC_PRESCALER
				     LL_USART_PRESCALER_DIV1,
#endif
#ifdef USART_CR1_OVER8
				     LL_USART_OVERSAMPLING_16,
#endif
				     baud_rate);

#if HAS_LPUART_1
	}
#endif /* HAS_LPUART_1 */
}

static inline void uart_stm32_set_parity(const struct device *dev,
					 uint32_t parity)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_SetParity(UartInstance, parity);
}

static inline uint32_t uart_stm32_get_parity(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_GetParity(UartInstance);
}

static inline void uart_stm32_set_stopbits(const struct device *dev,
					   uint32_t stopbits)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_SetStopBitsLength(UartInstance, stopbits);
}

static inline uint32_t uart_stm32_get_stopbits(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_GetStopBitsLength(UartInstance);
}

static inline void uart_stm32_set_databits(const struct device *dev,
					   uint32_t databits)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_SetDataWidth(UartInstance, databits);
}

static inline uint32_t uart_stm32_get_databits(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_GetDataWidth(UartInstance);
}

static inline void uart_stm32_set_hwctrl(const struct device *dev,
					 uint32_t hwctrl)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_SetHWFlowCtrl(UartInstance, hwctrl);
}

static inline uint32_t uart_stm32_get_hwctrl(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_GetHWFlowCtrl(UartInstance);
}

static inline uint32_t uart_stm32_cfg2ll_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return LL_USART_PARITY_ODD;
	case UART_CFG_PARITY_EVEN:
		return LL_USART_PARITY_EVEN;
	case UART_CFG_PARITY_NONE:
	default:
		return LL_USART_PARITY_NONE;
	}
}

static inline enum uart_config_parity uart_stm32_ll2cfg_parity(uint32_t parity)
{
	switch (parity) {
	case LL_USART_PARITY_ODD:
		return UART_CFG_PARITY_ODD;
	case LL_USART_PARITY_EVEN:
		return UART_CFG_PARITY_EVEN;
	case LL_USART_PARITY_NONE:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static inline uint32_t uart_stm32_cfg2ll_stopbits(enum uart_config_stop_bits sb)
{
	switch (sb) {
/* Some MCU's don't support 0.5 stop bits */
#ifdef LL_USART_STOPBITS_0_5
	case UART_CFG_STOP_BITS_0_5:
		return LL_USART_STOPBITS_0_5;
#endif	/* LL_USART_STOPBITS_0_5 */
	case UART_CFG_STOP_BITS_1:
		return LL_USART_STOPBITS_1;
/* Some MCU's don't support 1.5 stop bits */
#ifdef LL_USART_STOPBITS_1_5
	case UART_CFG_STOP_BITS_1_5:
		return LL_USART_STOPBITS_1_5;
#endif	/* LL_USART_STOPBITS_1_5 */
	case UART_CFG_STOP_BITS_2:
	default:
		return LL_USART_STOPBITS_2;
	}
}

static inline enum uart_config_stop_bits uart_stm32_ll2cfg_stopbits(uint32_t sb)
{
	switch (sb) {
/* Some MCU's don't support 0.5 stop bits */
#ifdef LL_USART_STOPBITS_0_5
	case LL_USART_STOPBITS_0_5:
		return UART_CFG_STOP_BITS_0_5;
#endif	/* LL_USART_STOPBITS_0_5 */
	case LL_USART_STOPBITS_1:
		return UART_CFG_STOP_BITS_1;
/* Some MCU's don't support 1.5 stop bits */
#ifdef LL_USART_STOPBITS_1_5
	case LL_USART_STOPBITS_1_5:
		return UART_CFG_STOP_BITS_1_5;
#endif	/* LL_USART_STOPBITS_1_5 */
	case LL_USART_STOPBITS_2:
	default:
		return UART_CFG_STOP_BITS_2;
	}
}

static inline uint32_t uart_stm32_cfg2ll_databits(enum uart_config_data_bits db)
{
	switch (db) {
/* Some MCU's don't support 7B or 9B datawidth */
#ifdef LL_USART_DATAWIDTH_7B
	case UART_CFG_DATA_BITS_7:
		return LL_USART_DATAWIDTH_7B;
#endif	/* LL_USART_DATAWIDTH_7B */
#ifdef LL_USART_DATAWIDTH_9B
	case UART_CFG_DATA_BITS_9:
		return LL_USART_DATAWIDTH_9B;
#endif	/* LL_USART_DATAWIDTH_9B */
	case UART_CFG_DATA_BITS_8:
	default:
		return LL_USART_DATAWIDTH_8B;
	}
}

static inline enum uart_config_data_bits uart_stm32_ll2cfg_databits(uint32_t db)
{
	switch (db) {
/* Some MCU's don't support 7B or 9B datawidth */
#ifdef LL_USART_DATAWIDTH_7B
	case LL_USART_DATAWIDTH_7B:
		return UART_CFG_DATA_BITS_7;
#endif	/* LL_USART_DATAWIDTH_7B */
#ifdef LL_USART_DATAWIDTH_9B
	case LL_USART_DATAWIDTH_9B:
		return UART_CFG_DATA_BITS_9;
#endif	/* LL_USART_DATAWIDTH_9B */
	case LL_USART_DATAWIDTH_8B:
	default:
		return UART_CFG_DATA_BITS_8;
	}
}

/**
 * @brief  Get LL hardware flow control define from
 *         Zephyr hardware flow control option.
 * @note   Supports only UART_CFG_FLOW_CTRL_RTS_CTS.
 * @param  fc: Zephyr hardware flow control option.
 * @retval LL_USART_HWCONTROL_RTS_CTS, or LL_USART_HWCONTROL_NONE.
 */
static inline uint32_t uart_stm32_cfg2ll_hwctrl(enum uart_config_flow_control fc)
{
	if (fc == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return LL_USART_HWCONTROL_RTS_CTS;
	}

	return LL_USART_HWCONTROL_NONE;
}

/**
 * @brief  Get Zephyr hardware flow control option from
 *         LL hardware flow control define.
 * @note   Supports only LL_USART_HWCONTROL_RTS_CTS.
 * @param  fc: LL hardware flow control definition.
 * @retval UART_CFG_FLOW_CTRL_RTS_CTS, or UART_CFG_FLOW_CTRL_NONE.
 */
static inline enum uart_config_flow_control uart_stm32_ll2cfg_hwctrl(uint32_t fc)
{
	if (fc == LL_USART_HWCONTROL_RTS_CTS) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_FLOW_CTRL_NONE;
}

static int uart_stm32_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	const uint32_t parity = uart_stm32_cfg2ll_parity(cfg->parity);
	const uint32_t stopbits = uart_stm32_cfg2ll_stopbits(cfg->stop_bits);
	const uint32_t databits = uart_stm32_cfg2ll_databits(cfg->data_bits);
	const uint32_t flowctrl = uart_stm32_cfg2ll_hwctrl(cfg->flow_ctrl);

	/* Hardware doesn't support mark or space parity */
	if ((cfg->parity == UART_CFG_PARITY_MARK) ||
	    (cfg->parity == UART_CFG_PARITY_SPACE)) {
		return -ENOTSUP;
	}

#if defined(LL_USART_STOPBITS_0_5) && HAS_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance) &&
	    (cfg->stop_bits == UART_CFG_STOP_BITS_0_5)) {
		return -ENOTSUP;
	}
#else
	if (cfg->stop_bits == UART_CFG_STOP_BITS_0_5) {
		return -ENOTSUP;
	}
#endif

#if defined(LL_USART_STOPBITS_1_5) && HAS_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance) &&
	    (cfg->stop_bits == UART_CFG_STOP_BITS_1_5)) {
		return -ENOTSUP;
	}
#else
	if (cfg->stop_bits == UART_CFG_STOP_BITS_1_5) {
		return -ENOTSUP;
	}
#endif

	/* Driver doesn't support 5 or 6 databits and potentially 7 or 9 */
	if ((cfg->data_bits == UART_CFG_DATA_BITS_5) ||
	    (cfg->data_bits == UART_CFG_DATA_BITS_6)
#ifndef LL_USART_DATAWIDTH_7B
	    || (cfg->data_bits == UART_CFG_DATA_BITS_7)
#endif /* LL_USART_DATAWIDTH_7B */
	    || (cfg->data_bits == UART_CFG_DATA_BITS_9)) {
		return -ENOTSUP;
	}

	/* Driver supports only RTS CTS flow control */
	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		if (!IS_UART_HWFLOW_INSTANCE(UartInstance) ||
		    UART_CFG_FLOW_CTRL_RTS_CTS != cfg->flow_ctrl) {
			return -ENOTSUP;
		}
	}

	LL_USART_Disable(UartInstance);

	if (parity != uart_stm32_get_parity(dev)) {
		uart_stm32_set_parity(dev, parity);
	}

	if (stopbits != uart_stm32_get_stopbits(dev)) {
		uart_stm32_set_stopbits(dev, stopbits);
	}

	if (databits != uart_stm32_get_databits(dev)) {
		uart_stm32_set_databits(dev, databits);
	}

	if (flowctrl != uart_stm32_get_hwctrl(dev)) {
		uart_stm32_set_hwctrl(dev, flowctrl);
	}

	if (cfg->baudrate != data->baud_rate) {
		uart_stm32_set_baudrate(dev, cfg->baudrate);
		data->baud_rate = cfg->baudrate;
	}

	LL_USART_Enable(UartInstance);
	return 0;
};

static int uart_stm32_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	cfg->baudrate = data->baud_rate;
	cfg->parity = uart_stm32_ll2cfg_parity(uart_stm32_get_parity(dev));
	cfg->stop_bits = uart_stm32_ll2cfg_stopbits(
		uart_stm32_get_stopbits(dev));
	cfg->data_bits = uart_stm32_ll2cfg_databits(
		uart_stm32_get_databits(dev));
	cfg->flow_ctrl = uart_stm32_ll2cfg_hwctrl(
		uart_stm32_get_hwctrl(dev));
	return 0;
}

static int uart_stm32_poll_in(const struct device *dev, unsigned char *c)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Clear overrun error flag */
	if (LL_USART_IsActiveFlag_ORE(UartInstance)) {
		LL_USART_ClearFlag_ORE(UartInstance);
	}

	if (!LL_USART_IsActiveFlag_RXNE(UartInstance)) {
		return -1;
	}

	*c = (unsigned char)LL_USART_ReceiveData8(UartInstance);

	return 0;
}

static void uart_stm32_poll_out(const struct device *dev,
					unsigned char c)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Wait for TXE flag to be raised */
	while (!LL_USART_IsActiveFlag_TXE(UartInstance)) {
	}

	LL_USART_ClearFlag_TC(UartInstance);

	LL_USART_TransmitData8(UartInstance, (uint8_t)c);
}

static int uart_stm32_err_check(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	uint32_t err = 0U;

	/* Check for errors, but don't clear them here.
	 * Some SoC clear all error flags when at least
	 * one is cleared. (e.g. F4X, F1X, and F2X)
	 */
	if (LL_USART_IsActiveFlag_ORE(UartInstance)) {
		err |= UART_ERROR_OVERRUN;
	}

	if (LL_USART_IsActiveFlag_PE(UartInstance)) {
		err |= UART_ERROR_PARITY;
	}

	if (LL_USART_IsActiveFlag_FE(UartInstance)) {
		err |= UART_ERROR_FRAMING;
	}

	if (err & UART_ERROR_OVERRUN) {
		LL_USART_ClearFlag_ORE(UartInstance);
	}

	if (err & UART_ERROR_PARITY) {
		LL_USART_ClearFlag_PE(UartInstance);
	}

	if (err & UART_ERROR_FRAMING) {
		LL_USART_ClearFlag_FE(UartInstance);
	}

	/* Clear noise error as well,
	 * it is not represented by the errors enum
	 */
	LL_USART_ClearFlag_NE(UartInstance);

	return err;
}

static inline void __uart_stm32_get_clock(const struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	data->clock = clk;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_stm32_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int size)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	uint8_t num_tx = 0U;

	while ((size - num_tx > 0) &&
	       LL_USART_IsActiveFlag_TXE(UartInstance)) {
		/* TXE flag will be cleared with byte write to DR|RDR register */

		/* Send a character (8bit , parity none) */
		LL_USART_TransmitData8(UartInstance, tx_data[num_tx++]);
	}

	return num_tx;
}

static int uart_stm32_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) &&
	       LL_USART_IsActiveFlag_RXNE(UartInstance)) {
		/* RXNE flag will be cleared upon read from DR|RDR register */

		/* Receive a character (8bit , parity none) */
		rx_data[num_rx++] = LL_USART_ReceiveData8(UartInstance);

		/* Clear overrun error flag */
		if (LL_USART_IsActiveFlag_ORE(UartInstance)) {
			LL_USART_ClearFlag_ORE(UartInstance);
		}
	}

	return num_rx;
}

static void uart_stm32_irq_tx_enable(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_EnableIT_TC(UartInstance);
}

static void uart_stm32_irq_tx_disable(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_DisableIT_TC(UartInstance);
}

static int uart_stm32_irq_tx_ready(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_TXE(UartInstance) &&
		LL_USART_IsEnabledIT_TC(UartInstance);
}

static int uart_stm32_irq_tx_complete(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_TC(UartInstance);
}

static void uart_stm32_irq_rx_enable(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_EnableIT_RXNE(UartInstance);
}

static void uart_stm32_irq_rx_disable(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_DisableIT_RXNE(UartInstance);
}

static int uart_stm32_irq_rx_ready(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_RXNE(UartInstance);
}

static void uart_stm32_irq_err_enable(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Enable FE, ORE interruptions */
	LL_USART_EnableIT_ERROR(UartInstance);
#if !defined(CONFIG_SOC_SERIES_STM32F0X) || defined(USART_LIN_SUPPORT)
	/* Enable Line break detection */
	if (IS_UART_LIN_INSTANCE(UartInstance)) {
		LL_USART_EnableIT_LBD(UartInstance);
	}
#endif
	/* Enable parity error interruption */
	LL_USART_EnableIT_PE(UartInstance);
}

static void uart_stm32_irq_err_disable(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Disable FE, ORE interruptions */
	LL_USART_DisableIT_ERROR(UartInstance);
#if !defined(CONFIG_SOC_SERIES_STM32F0X) || defined(USART_LIN_SUPPORT)
	/* Disable Line break detection */
	if (IS_UART_LIN_INSTANCE(UartInstance)) {
		LL_USART_DisableIT_LBD(UartInstance);
	}
#endif
	/* Disable parity error interruption */
	LL_USART_DisableIT_PE(UartInstance);
}

static int uart_stm32_irq_is_pending(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return ((LL_USART_IsActiveFlag_RXNE(UartInstance) &&
		 LL_USART_IsEnabledIT_RXNE(UartInstance)) ||
		(LL_USART_IsActiveFlag_TC(UartInstance) &&
		 LL_USART_IsEnabledIT_TC(UartInstance)));
}

static int uart_stm32_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_stm32_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	data->user_cb = cb;
	data->user_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API

static inline void async_user_callback(struct uart_stm32_data *data,
				struct uart_event *event)
{
	if (data->async_cb) {
		data->async_cb(data->uart_dev, event, data->async_user_data);
	}
}

static inline void async_evt_rx_rdy(struct uart_stm32_data *data)
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

static inline void async_evt_rx_err(struct uart_stm32_data *data, int err_code)
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

static inline void async_evt_tx_done(struct uart_stm32_data *data)
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

static inline void async_evt_tx_abort(struct uart_stm32_data *data)
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

static inline void async_evt_rx_buf_request(struct uart_stm32_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(data, &evt);
}

static inline void async_evt_rx_buf_release(struct uart_stm32_data *data)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->dma_rx.buffer,
	};

	async_user_callback(data, &evt);
}

static inline void async_timer_start(struct k_delayed_work *work,
				     int32_t timeout)
{
	if ((timeout != SYS_FOREVER_MS) && (timeout != 0)) {
		/* start timer */
		LOG_DBG("async timer started for %d ms", timeout);
		k_delayed_work_submit(work, K_MSEC(timeout));
	}
}

static void uart_stm32_dma_rx_flush(const struct device *dev)
{
	struct dma_status stat;
	struct uart_stm32_data *data = DEV_DATA(dev);

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

#endif /* CONFIG_UART_ASYNC_API */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)

static void uart_stm32_isr(const struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

#ifdef CONFIG_UART_ASYNC_API
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	if (LL_USART_IsEnabledIT_IDLE(UartInstance) &&
			LL_USART_IsActiveFlag_IDLE(UartInstance)) {

		LL_USART_ClearFlag_IDLE(UartInstance);

		LOG_DBG("idle interrupt occurred");

		/* Start the RX timer */
		async_timer_start(&data->dma_rx.timeout_work,
							data->dma_rx.timeout);

		if (data->dma_rx.timeout == 0) {
			uart_stm32_dma_rx_flush(dev);
		}
	}

	/* Clear errors */
	uart_stm32_err_check(dev);
#endif /* CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
}

#endif /* (CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API) */

#ifdef CONFIG_UART_ASYNC_API

static int uart_stm32_async_callback_set(const struct device *dev,
					 uart_callback_t callback,
					 void *user_data)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	data->async_cb = callback;
	data->async_user_data = user_data;

	return 0;
}

static inline void uart_stm32_dma_tx_enable(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_EnableDMAReq_TX(UartInstance);
}

static inline void uart_stm32_dma_tx_disable(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_DisableDMAReq_TX(UartInstance);
}

static inline void uart_stm32_dma_rx_enable(const struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);

	LL_USART_EnableDMAReq_RX(UartInstance);

	data->dma_rx.enabled = true;
}

static inline void uart_stm32_dma_rx_disable(const struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	data->dma_rx.enabled = false;
}

static int uart_stm32_async_rx_disable(const struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	struct uart_event disabled_event = {
		.type = UART_RX_DISABLED
	};

	if (!data->dma_rx.enabled) {
		async_user_callback(data, &disabled_event);
		return -EFAULT;
	}

	LL_USART_DisableIT_IDLE(UartInstance);

	uart_stm32_dma_rx_flush(dev);

	async_evt_rx_buf_release(data);

	uart_stm32_dma_rx_disable(dev);

	k_delayed_work_cancel(&data->dma_rx.timeout_work);

	dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	LOG_DBG("rx: disabled");

	async_user_callback(data, &disabled_event);

	return 0;
}

void uart_stm32_dma_tx_cb(const struct device *dma_dev, void *user_data,
			       uint32_t channel, int status)
{
	const struct device *uart_dev = user_data;
	struct uart_stm32_data *data = DEV_DATA(uart_dev);
	struct dma_status stat;
	unsigned int key = irq_lock();

	/* Disable TX */
	uart_stm32_dma_tx_disable(uart_dev);

	k_delayed_work_cancel(&data->dma_tx.timeout_work);

	data->dma_tx.buffer_length = 0;

	if (!dma_get_status(data->dma_tx.dma_dev,
				data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = data->dma_tx.buffer_length -
					stat.pending_length;
	}

	irq_unlock(key);

	async_evt_tx_done(data);
}

static void uart_stm32_dma_replace_buffer(const struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	/* Replace the buffer and relod the DMA */
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

	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_ClearFlag_IDLE(UartInstance);

	/* Request next buffer */
	async_evt_rx_buf_request(data);
}

void uart_stm32_dma_rx_cb(const struct device *dma_dev, void *user_data,
			       uint32_t channel, int status)
{
	const struct device *uart_dev = user_data;
	struct uart_stm32_data *data = DEV_DATA(uart_dev);

	if (status != 0) {
		async_evt_rx_err(data, status);
		return;
	}

	k_delayed_work_cancel(&data->dma_rx.timeout_work);

	/* true since this functions occurs when buffer if full */
	data->dma_rx.counter = data->dma_rx.buffer_length;

	async_evt_rx_rdy(data);

	if (data->rx_next_buffer != NULL) {
		async_evt_rx_buf_release(data);

		/* replace the buffer when the current
		 * is full and not the same as the next
		 * one.
		 */
		uart_stm32_dma_replace_buffer(uart_dev);
	} else {
		/* Buffer full without valid next buffer,
		 * an UART_RX_DISABLED event must be generated,
		 * but uart_stm32_async_rx_disable() cannot be
		 * called in ISR context. So force the RX timeout
		 * to minimum value and let the RX timeout to do the job.
		 */
		k_delayed_work_submit(&data->dma_rx.timeout_work, K_TICKS(1));
	}
}

static int uart_stm32_async_tx(const struct device *dev,
		const uint8_t *tx_data, size_t buf_size, int32_t timeout)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
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

	/* disable TX interrupt since DMA will handle it */
	LL_USART_DisableIT_TC(UartInstance);

	/* set source address */
	data->dma_tx.blk_cfg.source_address = (uint32_t)data->dma_tx.buffer;
	data->dma_tx.blk_cfg.block_size = data->dma_tx.buffer_length;

	ret = dma_config(data->dma_tx.dma_dev, data->dma_tx.dma_channel,
				&data->dma_tx.dma_cfg);

	if (ret != 0) {
		LOG_ERR("dma tx config error!");
		return -EINVAL;
	}

	if (dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel)) {
		LOG_ERR("UART err: TX DMA start failed!");
		return -EFAULT;
	}

	/* Start TX timer */
	async_timer_start(&data->dma_tx.timeout_work, data->dma_tx.timeout);

	/* Enable TX DMA requests */
	uart_stm32_dma_tx_enable(dev);

	return 0;
}

static int uart_stm32_async_rx_enable(const struct device *dev,
		uint8_t *rx_buf, size_t buf_size, int32_t timeout)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	int ret;

	if (data->dma_rx.dma_dev == NULL) {
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

	/* Disable RX interrupts to let DMA to handle it */
	LL_USART_DisableIT_RXNE(UartInstance);

	data->dma_rx.blk_cfg.block_size = buf_size;
	data->dma_rx.blk_cfg.dest_address = (uint32_t)data->dma_rx.buffer;

	ret = dma_config(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
				&data->dma_rx.dma_cfg);

	if (ret != 0) {
		LOG_ERR("UART ERR: RX DMA config failed!");
		return -EINVAL;
	}

	if (dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel)) {
		LOG_ERR("UART ERR: RX DMA start failed!");
		return -EFAULT;
	}

	/* Enable RX DMA requests */
	uart_stm32_dma_rx_enable(dev);

	/* Enable IRQ IDLE to define the end of a
	 * RX DMA transaction.
	 */
	LL_USART_ClearFlag_IDLE(UartInstance);
	LL_USART_EnableIT_IDLE(UartInstance);

	LL_USART_EnableIT_ERROR(UartInstance);

	/* Request next buffer */
	async_evt_rx_buf_request(data);

	LOG_DBG("async rx enabled");

	return ret;
}

static int uart_stm32_async_tx_abort(const struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	size_t tx_buffer_length = data->dma_tx.buffer_length;
	struct dma_status stat;

	if (tx_buffer_length == 0) {
		return -EFAULT;
	}

	k_delayed_work_cancel(&data->dma_tx.timeout_work);
	if (!dma_get_status(data->dma_tx.dma_dev,
				data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = tx_buffer_length - stat.pending_length;
	}

	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	async_evt_tx_abort(data);

	return 0;
}

static void uart_stm32_async_rx_timeout(struct k_work *work)
{
	struct uart_dma_stream *rx_stream = CONTAINER_OF(work,
			struct uart_dma_stream, timeout_work);
	struct uart_stm32_data *data = CONTAINER_OF(rx_stream,
			struct uart_stm32_data, dma_rx);
	const struct device *dev = data->uart_dev;

	LOG_DBG("rx timeout");

	if (data->dma_rx.counter == data->dma_rx.buffer_length) {
		uart_stm32_async_rx_disable(dev);
	} else {
		uart_stm32_dma_rx_flush(dev);
	}
}

static void uart_stm32_async_tx_timeout(struct k_work *work)
{
	struct uart_dma_stream *tx_stream = CONTAINER_OF(work,
			struct uart_dma_stream, timeout_work);
	struct uart_stm32_data *data = CONTAINER_OF(tx_stream,
			struct uart_stm32_data, dma_tx);
	const struct device *dev = data->uart_dev;

	uart_stm32_async_tx_abort(dev);

	LOG_DBG("tx: async timeout");
}

static int uart_stm32_async_rx_buf_rsp(const struct device *dev, uint8_t *buf,
				       size_t len)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	LOG_DBG("replace buffer (%d)", len);
	data->rx_next_buffer = buf;
	data->rx_next_buffer_len = len;

	return 0;
}

static int uart_stm32_async_init(const struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	data->uart_dev = dev;

	if (data->dma_rx.dma_dev != NULL) {
		if (!device_is_ready(data->dma_rx.dma_dev)) {
			return -ENODEV;
		}
	}

	if (data->dma_tx.dma_dev != NULL) {
		if (!device_is_ready(data->dma_rx.dma_dev)) {
			return -ENODEV;
		}
	}

	/* Disable both TX and RX DMA requests */
	uart_stm32_dma_rx_disable(dev);
	uart_stm32_dma_tx_disable(dev);

	k_delayed_work_init(&data->dma_rx.timeout_work,
			    uart_stm32_async_rx_timeout);
	k_delayed_work_init(&data->dma_tx.timeout_work,
			    uart_stm32_async_tx_timeout);

	/* Configure dma rx config */
	memset(&data->dma_rx.blk_cfg, 0, sizeof(data->dma_rx.blk_cfg));

#if defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32L1X)
	data->dma_rx.blk_cfg.source_address =
				LL_USART_DMA_GetRegAddr(UartInstance);
#else
	data->dma_rx.blk_cfg.source_address =
				LL_USART_DMA_GetRegAddr(UartInstance,
						LL_USART_DMA_REG_DATA_RECEIVE);
#endif

	data->dma_rx.blk_cfg.dest_address = 0; /* dest not ready */

	if (data->dma_rx.src_addr_increment) {
		data->dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	if (data->dma_rx.dst_addr_increment) {
		data->dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	/* RX disable circular buffer */
	data->dma_rx.blk_cfg.source_reload_en  = 0;
	data->dma_rx.blk_cfg.dest_reload_en = 0;
	data->dma_rx.blk_cfg.fifo_mode_control = data->dma_rx.fifo_threshold;

	data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;
	data->dma_rx.dma_cfg.user_data = (void *)dev;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	/* Configure dma tx config */
	memset(&data->dma_tx.blk_cfg, 0, sizeof(data->dma_tx.blk_cfg));

#if defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32L1X)
	data->dma_tx.blk_cfg.dest_address =
			LL_USART_DMA_GetRegAddr(UartInstance);
#else
	data->dma_tx.blk_cfg.dest_address =
			LL_USART_DMA_GetRegAddr(UartInstance,
					LL_USART_DMA_REG_DATA_TRANSMIT);
#endif

	data->dma_tx.blk_cfg.source_address = 0; /* not ready */

	if (data->dma_tx.src_addr_increment) {
		data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	if (data->dma_tx.dst_addr_increment) {
		data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	data->dma_tx.blk_cfg.fifo_mode_control = data->dma_tx.fifo_threshold;

	data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
	data->dma_tx.dma_cfg.user_data = (void *)dev;

	return 0;
}

#endif /* CONFIG_UART_ASYNC_API */

static const struct uart_driver_api uart_stm32_driver_api = {
	.poll_in = uart_stm32_poll_in,
	.poll_out = uart_stm32_poll_out,
	.err_check = uart_stm32_err_check,
	.configure = uart_stm32_configure,
	.config_get = uart_stm32_config_get,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_stm32_fifo_fill,
	.fifo_read = uart_stm32_fifo_read,
	.irq_tx_enable = uart_stm32_irq_tx_enable,
	.irq_tx_disable = uart_stm32_irq_tx_disable,
	.irq_tx_ready = uart_stm32_irq_tx_ready,
	.irq_tx_complete = uart_stm32_irq_tx_complete,
	.irq_rx_enable = uart_stm32_irq_rx_enable,
	.irq_rx_disable = uart_stm32_irq_rx_disable,
	.irq_rx_ready = uart_stm32_irq_rx_ready,
	.irq_err_enable = uart_stm32_irq_err_enable,
	.irq_err_disable = uart_stm32_irq_err_disable,
	.irq_is_pending = uart_stm32_irq_is_pending,
	.irq_update = uart_stm32_irq_update,
	.irq_callback_set = uart_stm32_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_stm32_async_callback_set,
	.tx = uart_stm32_async_tx,
	.tx_abort = uart_stm32_async_tx_abort,
	.rx_enable = uart_stm32_async_rx_enable,
	.rx_disable = uart_stm32_async_rx_disable,
	.rx_buf_rsp = uart_stm32_async_rx_buf_rsp,
#endif  /* CONFIG_UART_ASYNC_API */
};

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
static int uart_stm32_init(const struct device *dev)
{
	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	uint32_t ll_parity;
	uint32_t ll_datawidth;
	int err;

	__uart_stm32_get_clock(dev);
	/* enable clock */
	if (clock_control_on(data->clock,
			(clock_control_subsys_t *)&config->pclken) != 0) {
		return -EIO;
	}

	/* Configure dt provided device signals when available */
	err = stm32_dt_pinctrl_configure(config->pinctrl_list,
					 config->pinctrl_list_size,
					 (uint32_t)UART_STRUCT(dev));
	if (err < 0) {
		return err;
	}

	LL_USART_Disable(UartInstance);

	/* TX/RX direction */
	LL_USART_SetTransferDirection(UartInstance,
				      LL_USART_DIRECTION_TX_RX);

	/* Determine the datawidth and parity. If we use other parity than
	 * 'none' we must use datawidth = 9 (to get 8 databit + 1 parity bit).
	 */
	if (config->parity == 2) {
		/* 8 databit, 1 parity bit, parity even */
		ll_parity = LL_USART_PARITY_EVEN;
		ll_datawidth = LL_USART_DATAWIDTH_9B;
	} else if (config->parity == 1) {
		/* 8 databit, 1 parity bit, parity odd */
		ll_parity = LL_USART_PARITY_ODD;
		ll_datawidth = LL_USART_DATAWIDTH_9B;
	} else {  /* Default to 8N0, but show warning if invalid value */
		if (config->parity != 0) {
			LOG_WRN("Invalid parity setting '%d'."
				"Defaulting to 'none'.", config->parity);
		}
		/* 8 databit, parity none */
		ll_parity = LL_USART_PARITY_NONE;
		ll_datawidth = LL_USART_DATAWIDTH_8B;
	}

	/* Set datawidth and parity, 1 start bit, 1 stop bit  */
	LL_USART_ConfigCharacter(UartInstance,
				 ll_datawidth,
				 ll_parity,
				 LL_USART_STOPBITS_1);

	if (config->hw_flow_control) {
		uart_stm32_set_hwctrl(dev, LL_USART_HWCONTROL_RTS_CTS);
	}

	/* Set the default baudrate */
	uart_stm32_set_baudrate(dev, data->baud_rate);

	LL_USART_Enable(UartInstance);

#ifdef USART_ISR_TEACK
	/* Wait until TEACK flag is set */
	while (!(LL_USART_IsActiveFlag_TEACK(UartInstance))) {
	}
#endif /* !USART_ISR_TEACK */

#ifdef USART_ISR_REACK
	/* Wait until REACK flag is set */
	while (!(LL_USART_IsActiveFlag_REACK(UartInstance))) {
	}
#endif /* !USART_ISR_REACK */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	config->uconf.irq_config_func(dev);
#endif
#ifdef CONFIG_PM_DEVICE
	data->pm_state = DEVICE_PM_ACTIVE_STATE;
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_UART_ASYNC_API
	return uart_stm32_async_init(dev);
#else
	return 0;
#endif
}

#ifdef CONFIG_PM_DEVICE
static int uart_stm32_set_power_state(const struct device *dev,
					      uint32_t new_state)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);

	/* setting a low power mode */
	if (new_state != DEVICE_PM_ACTIVE_STATE) {
#ifdef USART_ISR_BUSY
		/* Make sure that no USART transfer is on-going */
		while (LL_USART_IsActiveFlag_BUSY(UartInstance) == 1) {
		}
#endif
		while (LL_USART_IsActiveFlag_TC(UartInstance) == 0) {
		}
#ifdef USART_ISR_REACK
		/* Make sure that USART is ready for reception */
		while (LL_USART_IsActiveFlag_REACK(UartInstance) == 0) {
		}
#endif
		/* Clear OVERRUN flag */
		LL_USART_ClearFlag_ORE(UartInstance);
		/* Leave UartInstance unchanged */
	}
	data->pm_state = new_state;
	/* UartInstance returning to active mode has nothing special to do */
	return 0;
}

/**
 * @brief disable the UART channel
 *
 * This routine is called to put the device in low power mode.
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_stm32_pm_control(const struct device *dev,
					 uint32_t ctrl_command,
					 void *context, device_pm_cb cb,
					 void *arg)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		uint32_t new_state = *((const uint32_t *)context);

		if (new_state != data->pm_state) {
			uart_stm32_set_power_state(dev, new_state);
		}
	} else {
		__ASSERT_NO_MSG(ctrl_command == DEVICE_PM_GET_POWER_STATE);
		*((uint32_t *)context) = data->pm_state;
	}

	if (cb) {
		cb(dev, 0, context, arg);
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_UART_ASYNC_API
#define DMA_CHANNEL_CONFIG(id, dir)					\
	DT_INST_DMAS_CELL_BY_NAME(id, dir, channel_config)
#define DMA_FEATURES(id, dir)						\
	DT_INST_DMAS_CELL_BY_NAME(id, dir, features)
#define DMA_CTLR(id, dir)						\
	DT_INST_DMAS_CTLR_BY_NAME(id, dir)

/* src_dev and dest_dev should be 'MEMORY' or 'PERIPHERAL'. */
#define UART_DMA_CHANNEL_INIT(index, dir, dir_cap, src_dev, dest_dev)	\
	.dma_dev = DEVICE_DT_GET(DMA_CTLR(index, dir)),			\
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),	\
	.dma_cfg = {							\
		.dma_slot = DT_INST_DMAS_CELL_BY_NAME(index, dir, slot),\
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(	\
					DMA_CHANNEL_CONFIG(index, dir)),\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(		\
				DMA_CHANNEL_CONFIG(index, dir)),	\
		.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(\
					DMA_CHANNEL_CONFIG(index, dir)),\
		.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(\
				DMA_CHANNEL_CONFIG(index, dir)),\
		.source_burst_length = 1, /* SINGLE transfer */		\
		.dest_burst_length = 1,					\
		.block_count = 1,					\
		.dma_callback = uart_stm32_dma_##dir##_cb,		\
	},								\
	.src_addr_increment = STM32_DMA_CONFIG_##src_dev##_ADDR_INC(	\
				DMA_CHANNEL_CONFIG(index, dir)),	\
	.dst_addr_increment = STM32_DMA_CONFIG_##dest_dev##_ADDR_INC(	\
				DMA_CHANNEL_CONFIG(index, dir)),	\
	.fifo_threshold = STM32_DMA_FEATURES_FIFO_THRESHOLD(		\
				DMA_FEATURES(index, dir)),		\

#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define STM32_UART_IRQ_HANDLER_DECL(index)				\
	static void uart_stm32_irq_config_func_##index(const struct device *dev)
#define STM32_UART_IRQ_HANDLER_FUNC(index)				\
	.irq_config_func = uart_stm32_irq_config_func_##index,
#define STM32_UART_IRQ_HANDLER(index)					\
static void uart_stm32_irq_config_func_##index(const struct device *dev)	\
{									\
	IRQ_CONNECT(DT_INST_IRQN(index),				\
		DT_INST_IRQ(index, priority),				\
		uart_stm32_isr, DEVICE_DT_INST_GET(index),		\
		0);							\
	irq_enable(DT_INST_IRQN(index));				\
}
#else
#define STM32_UART_IRQ_HANDLER_DECL(index)
#define STM32_UART_IRQ_HANDLER_FUNC(index)
#define STM32_UART_IRQ_HANDLER(index)
#endif

#ifdef CONFIG_UART_ASYNC_API
#define UART_DMA_CHANNEL(index, dir, DIR, src, dest)			\
.dma_##dir = {				\
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),			\
		 (UART_DMA_CHANNEL_INIT(index, dir, DIR, src, dest)),	\
		 (NULL))				\
	},

#else
#define UART_DMA_CHANNEL(index, dir, DIR, src, dest)
#endif

#define STM32_UART_INIT(index)						\
STM32_UART_IRQ_HANDLER_DECL(index);					\
									\
static const struct soc_gpio_pinctrl uart_pins_##index[] =		\
				ST_STM32_DT_INST_PINCTRL(index, 0);	\
									\
static const struct uart_stm32_config uart_stm32_cfg_##index = {	\
	.uconf = {							\
		.base = (uint8_t *)DT_INST_REG_ADDR(index),		\
		STM32_UART_IRQ_HANDLER_FUNC(index)			\
	},								\
	.pclken = { .bus = DT_INST_CLOCKS_CELL(index, bus),		\
		    .enr = DT_INST_CLOCKS_CELL(index, bits)		\
	},								\
	.hw_flow_control = DT_INST_PROP(index, hw_flow_control),	\
	.parity = DT_INST_PROP_OR(index, parity, UART_CFG_PARITY_NONE),	\
	.pinctrl_list = uart_pins_##index,				\
	.pinctrl_list_size = ARRAY_SIZE(uart_pins_##index),		\
};									\
									\
static struct uart_stm32_data uart_stm32_data_##index = {		\
	.baud_rate = DT_INST_PROP(index, current_speed),		\
	UART_DMA_CHANNEL(index, rx, RX, PERIPHERAL, MEMORY)		\
	UART_DMA_CHANNEL(index, tx, TX, MEMORY, PERIPHERAL)		\
};									\
									\
DEVICE_DT_INST_DEFINE(index,						\
		    &uart_stm32_init,					\
		    &uart_stm32_pm_control,				\
		    &uart_stm32_data_##index, &uart_stm32_cfg_##index,	\
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &uart_stm32_driver_api);				\
									\
STM32_UART_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(STM32_UART_INIT)
