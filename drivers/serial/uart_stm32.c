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
#include <drivers/clock_control.h>

#ifdef CONFIG_UART_ASYNC_API
#include <dt-bindings/dma/stm32_dma.h>
#include <drivers/dma.h>
#endif

#include <linker/sections.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include "uart_stm32.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(uart_stm32);

#define HAS_LPUART_1 (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpuart1), \
					 st_stm32_lpuart, okay))

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct uart_stm32_config * const)(dev)->config_info)
#define DEV_DATA(dev)							\
	((struct uart_stm32_data * const)(dev)->driver_data)
#define UART_STRUCT(dev)					\
	((USART_TypeDef *)(DEV_CFG(dev))->uconf.base)

#define TIMEOUT 1000

static inline void uart_stm32_set_baudrate(struct device *dev, uint32_t baud_rate)
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

static inline void uart_stm32_set_parity(struct device *dev, uint32_t parity)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_SetParity(UartInstance, parity);
}

static inline uint32_t uart_stm32_get_parity(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_GetParity(UartInstance);
}

static inline void uart_stm32_set_stopbits(struct device *dev, uint32_t stopbits)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_SetStopBitsLength(UartInstance, stopbits);
}

static inline uint32_t uart_stm32_get_stopbits(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_GetStopBitsLength(UartInstance);
}

static inline void uart_stm32_set_databits(struct device *dev, uint32_t databits)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_SetDataWidth(UartInstance, databits);
}

static inline uint32_t uart_stm32_get_databits(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_GetDataWidth(UartInstance);
}

static inline void uart_stm32_set_hwctrl(struct device *dev, uint32_t hwctrl)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_SetHWFlowCtrl(UartInstance, hwctrl);
}

static inline uint32_t uart_stm32_get_hwctrl(struct device *dev)
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

static int uart_stm32_configure(struct device *dev,
				const struct uart_config *cfg)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	const uint32_t parity = uart_stm32_cfg2ll_parity(cfg->parity);
	const uint32_t stopbits = uart_stm32_cfg2ll_stopbits(cfg->stop_bits);
	const uint32_t databits = uart_stm32_cfg2ll_databits(cfg->data_bits);
	const uint32_t flowctrl = uart_stm32_cfg2ll_hwctrl(cfg->flow_ctrl);

	/* Hardware doesn't support mark or space parity */
	if ((UART_CFG_PARITY_MARK == cfg->parity) ||
	    (UART_CFG_PARITY_SPACE == cfg->parity)) {
		return -ENOTSUP;
	}

#if defined(LL_USART_STOPBITS_0_5) && HAS_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance) &&
	    UART_CFG_STOP_BITS_0_5 == cfg->stop_bits) {
		return -ENOTSUP;
	}
#else
	if (UART_CFG_STOP_BITS_0_5 == cfg->stop_bits) {
		return -ENOTSUP;
	}
#endif

#if defined(LL_USART_STOPBITS_1_5) && HAS_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance) &&
	    UART_CFG_STOP_BITS_1_5 == cfg->stop_bits) {
		return -ENOTSUP;
	}
#else
	if (UART_CFG_STOP_BITS_1_5 == cfg->stop_bits) {
		return -ENOTSUP;
	}
#endif

	/* Driver doesn't support 5 or 6 databits and potentially 7 or 9 */
	if ((UART_CFG_DATA_BITS_5 == cfg->data_bits) ||
	    (UART_CFG_DATA_BITS_6 == cfg->data_bits)
#ifndef LL_USART_DATAWIDTH_7B
	    || (UART_CFG_DATA_BITS_7 == cfg->data_bits)
#endif /* LL_USART_DATAWIDTH_7B */
#ifndef LL_USART_DATAWIDTH_9B
	    || (UART_CFG_DATA_BITS_9 == cfg->data_bits)
#endif /* LL_USART_DATAWIDTH_9B */
		) {
		return -ENOTSUP;
	}

	/* Driver supports only RTS CTS flow control */
	if (UART_CFG_FLOW_CTRL_NONE != cfg->flow_ctrl) {
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

static int uart_stm32_config_get(struct device *dev, struct uart_config *cfg)
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

static int uart_stm32_poll_in(struct device *dev, unsigned char *c)
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

static void uart_stm32_poll_out(struct device *dev,
					unsigned char c)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Wait for TXE flag to be raised */
	while (!LL_USART_IsActiveFlag_TXE(UartInstance)) {
	}

	LL_USART_ClearFlag_TC(UartInstance);

	LL_USART_TransmitData8(UartInstance, (uint8_t)c);
}

static int uart_stm32_err_check(struct device *dev)
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

static inline void __uart_stm32_get_clock(struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	struct device *clk =
		device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(clk);

	data->clock = clk;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_stm32_fifo_fill(struct device *dev, const uint8_t *tx_data,
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

static int uart_stm32_fifo_read(struct device *dev, uint8_t *rx_data,
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

static void uart_stm32_irq_tx_enable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_EnableIT_TC(UartInstance);
}

static void uart_stm32_irq_tx_disable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_DisableIT_TC(UartInstance);
}

static int uart_stm32_irq_tx_ready(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_TXE(UartInstance);
}

static int uart_stm32_irq_tx_complete(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_TC(UartInstance);
}

static void uart_stm32_irq_rx_enable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_EnableIT_RXNE(UartInstance);
}

static void uart_stm32_irq_rx_disable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_DisableIT_RXNE(UartInstance);
}

static int uart_stm32_irq_rx_ready(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_RXNE(UartInstance);
}

static void uart_stm32_irq_err_enable(struct device *dev)
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

static void uart_stm32_irq_err_disable(struct device *dev)
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

static int uart_stm32_irq_is_pending(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return ((LL_USART_IsActiveFlag_RXNE(UartInstance) &&
		 LL_USART_IsEnabledIT_RXNE(UartInstance)) ||
		(LL_USART_IsActiveFlag_TC(UartInstance) &&
		 LL_USART_IsEnabledIT_TC(UartInstance)));
}

static int uart_stm32_irq_update(struct device *dev)
{
	return 1;
}

static void uart_stm32_irq_callback_set(struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	data->user_cb = cb;
	data->user_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)

#ifdef CONFIG_UART_ASYNC_API
static void uart_stm32_dma_rx_cb(void *client_data, uint32_t id, int ret_code);
#endif

static void uart_stm32_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_stm32_data *data = DEV_DATA(dev);
#ifdef CONFIG_UART_ASYNC_API
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	if (LL_USART_IsEnabledIT_IDLE(UartInstance)
			&& LL_USART_IsActiveFlag_IDLE(UartInstance)) {
		LL_USART_ClearFlag_IDLE(UartInstance);
		printk("idle\n");
		uart_stm32_dma_rx_cb(dev, data->rx.dma_channel, 0);
	}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->user_cb) {
		data->user_cb(data->user_data);
	}
#endif
}

#endif /* (CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API) */

#ifdef CONFIG_UART_ASYNC_API

static void async_user_callback(struct uart_stm32_data *data,
			  struct uart_event *event)
{
	if (data != NULL && data->async_cb) {
		data->async_cb(event, data->async_user_data);
	}
}

static int uart_stm32_async_callback_set(struct device *dev,
					 uart_callback_t callback,
					 void *user_data)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	data->async_cb = callback;
	data->async_user_data = user_data;

	return 0;
}

static void async_evt_rx_rdy(void *priv_data)
{
	struct uart_stm32_data *data = priv_data;

	struct uart_event event;

	printk("rx_rdy: (%d %d)\n", data->rx.offset, data->rx.counter);
	/* send event only for new data */
	event.type = UART_RX_RDY;
	event.data.rx.buf = data->rx.buffer;
	event.data.rx.len = data->rx.counter - data->rx.offset;
	event.data.rx.offset = data->rx.offset;

	if (event.data.rx.len > 0) {
		async_user_callback(priv_data, &event);
	}
}

static void async_evt_rx_err(void *priv_data, int err_code)
{
	struct uart_stm32_data *data = priv_data;
	struct uart_event event = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = err_code,
		.data.rx_stop.data.len = data->rx.counter,
		.data.rx_stop.data.offset = 0,
		.data.rx_stop.data.buf = data->rx.buffer
	};

	async_user_callback(data, &event);
}

static void async_evt_tx_done(void *priv_data)
{
	struct uart_stm32_data *data = priv_data;

	struct uart_event event = {
		.type = UART_TX_DONE,
		.data.tx.buf = data->tx.buffer,
		.data.tx.len = data->tx.counter
	};

	async_user_callback(data, &event);

	printk("tx done:%d\n", event.data.tx.len);
	/* Reset tx buffer */
	data->tx.buffer_length = 0;
	data->tx.counter = 0;
}

static void async_evt_rx_buf_request(void *priv_data)
{
	struct uart_stm32_data *data = priv_data;
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(data, &evt);
}

static int uart_stm32_dma_tx_enable(struct device *dev, bool enable)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	bool isEnabled;

	if (enable) {
		LL_USART_EnableDMAReq_TX(UartInstance);
	} else {
		LL_USART_DisableDMAReq_TX(UartInstance);
	}

	isEnabled = LL_USART_IsEnabledDMAReq_TX(UartInstance);

	return isEnabled == enable;
}

static inline int is_dma_rx_enabled(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsEnabledDMAReq_RX(UartInstance);
}

static int uart_stm32_dma_rx_enable(struct device *dev, bool enable)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);
	bool isEnabled;

	if (enable) {
		LL_USART_EnableDMAReq_RX(UartInstance);
	} else {
		LL_USART_DisableDMAReq_RX(UartInstance);
	}

	isEnabled = is_dma_rx_enabled(dev);

	data->rx.enabled = isEnabled;
	return isEnabled == enable;
}

static int uart_stm32_async_rx_disable(struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	struct uart_event disabled_event = {
		.type = UART_RX_DISABLED
	};

	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->rx.buffer
	};


	if (data->rx.buffer_length == 0) {
		async_user_callback(data, &disabled_event);
		return -EFAULT;
	}

	uart_stm32_dma_rx_enable(dev, false);

	k_delayed_work_cancel(&data->rx.timeout_work);

	dma_stop(data->dev_dma_rx, data->rx.dma_channel);
	async_user_callback(data, &disabled_event);
	async_user_callback(data, &event);

	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	printk("rx: disabled\n");
	return 0;
}

static void uart_stm32_dma_tx_cb(void *client_data, uint32_t id, int ret_code)
{
	struct device *dev = client_data;
	struct uart_stm32_data *data = DEV_DATA(dev);
	struct dma_status stat;
	unsigned int key = irq_lock();

	/* Disable TX */
	uart_stm32_dma_tx_enable(dev, false);

	k_delayed_work_cancel(&data->tx.timeout_work);

	data->tx.buffer_length = 0;

	if (!dma_get_status(data->dev_dma_tx, data->tx.dma_channel, &stat)) {
		data->tx.counter = stat.pending_length; /* remaining data */
	}

	async_evt_tx_done(data);

	irq_unlock(key);
}

static void uart_stm32_dma_replace_buffer(struct device *dev)
{

	struct uart_stm32_data *data = DEV_DATA(dev);
	/* Replace the buffer and relod the DMA */

	/* Disable UART RX DMA */
	uart_stm32_dma_rx_enable(dev, false);

	printk("Replacing RX buffer: %d\n", data->rx_next_buffer_len);

	/* reload DMA */
	data->rx.offset = 0;
	data->rx.counter = 0;
	data->rx.buffer = data->rx_next_buffer;
	data->rx.buffer_length = data->rx_next_buffer_len;
	data->rx.blk_cfg.block_size = data->rx.buffer_length;
	data->rx.blk_cfg.dest_address = (uint32_t)data->rx.buffer;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;
	dma_reload(data->dev_dma_rx, data->rx.dma_channel,
			data->rx.blk_cfg.source_address,
			data->rx.blk_cfg.dest_address,
			data->rx.blk_cfg.block_size);

	dma_start(data->dev_dma_rx, data->rx.dma_channel);

	/* Request next buffer */
	async_evt_rx_buf_request(data);

	/* Enable RX DMA requests again */
	uart_stm32_dma_rx_enable(dev, true);

}

static inline void async_timer_start(struct k_delayed_work *work,
				     k_timeout_t timeout)
{
	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
			!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		/* start RX timer */
		k_delayed_work_submit(work, timeout);
	}
}

static void uart_stm32_dma_rx_cb(void *client_data, uint32_t id, int ret_code)
{
	struct device *dev = client_data;
	struct uart_stm32_data *data = DEV_DATA(dev);
	int rx_rcv_len = 0;
	struct dma_status stat;
	unsigned int key;

	if (!data->rx.buffer_length || !is_dma_rx_enabled(dev)) {
		async_evt_rx_err(data, UART_ERROR_OVERRUN);
		return;
	}

	if (ret_code != 0) {
		async_evt_rx_err(data, ret_code);
		return;
	}

	key = irq_lock();
	k_delayed_work_cancel(&data->rx.timeout_work);

	if (!dma_get_status(data->dev_dma_rx, data->rx.dma_channel, &stat)) {
		rx_rcv_len = stat.pending_length;
	}

	if (rx_rcv_len < data->rx.counter) {
		/* circular buffer next cycle */
		data->rx.counter = 0;
		data->rx.offset = 0;
	}

	data->rx.counter = rx_rcv_len;
	async_evt_rx_rdy(data);

	/* update the current pos for new data */
	data->rx.offset = rx_rcv_len;

	if (data->rx.counter == data->rx.buffer_length &&
	    data->rx_next_buffer != NULL &&
	    data->rx_next_buffer != data->rx.buffer) {
		/* replace the buffer when the current
		 * is full and not the same as the next
		 * one.
		 */
		uart_stm32_dma_replace_buffer(dev);
	}

	/* Start the timer again */
	async_timer_start(&data->rx.timeout_work, data->rx.timeout);

	irq_unlock(key);
}

static int uart_stm32_async_tx(struct device *dev, const uint8_t *tx_data,
			       size_t buf_size, int32_t timeout)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	bool reload;
	int ret;

	if (data->dev_dma_tx == NULL) {
		return -ENODEV;
	}

	if (data->tx.buffer_length != 0) {
		return -EBUSY;
	}

	data->tx.buffer = (uint8_t *)tx_data;
	data->tx.buffer_length = buf_size;
	data->tx.timeout = K_MSEC(timeout);

	printk("tx: l=%d\n", data->tx.buffer_length);

	/* disable TX interrupt since DMA will handle it */
	LL_USART_DisableIT_TC(UartInstance);

	reload = data->tx.blk_cfg.source_address != 0 ? true : false;

	/* set source address */
	data->tx.blk_cfg.source_address = (uint32_t)data->tx.buffer;
	data->tx.blk_cfg.block_size = data->tx.buffer_length;

	if (reload) {
		ret = dma_reload(data->dev_dma_tx, data->tx.dma_channel,
				 data->tx.blk_cfg.source_address,
				 data->tx.blk_cfg.dest_address,
				 data->tx.blk_cfg.block_size);
	} else {
		ret = dma_config(data->dev_dma_tx, data->tx.dma_channel,
				 &data->tx.dma_cfg);
	}

	if (ret != 0) {
		LOG_ERR("dma config/reload error!");
		return -EINVAL;
	}

	if (dma_start(data->dev_dma_tx, data->tx.dma_channel)) {
		LOG_ERR("UART err: TX DMA start failed!");
		return -1;
	}

	/* Start TX timer */
	async_timer_start(&data->tx.timeout_work, data->tx.timeout);

	/* Enable TX DMA requests */
	return uart_stm32_dma_tx_enable(dev, true);
}

static int uart_stm32_async_rx_enable(struct device *dev, uint8_t *rx_buf,
				      size_t buf_size, int32_t timeout)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	bool reload;
	int ret;

	if (data->dev_dma_rx == NULL) {
		return -ENODEV;
	}

	if (data->rx.enabled) {
		LOG_INF("RX was already enabled");
		return -EBUSY;

	}

	data->rx.offset = 0;
	data->rx.buffer = rx_buf;
	data->rx.buffer_length = buf_size;
	data->rx.counter = 0;
	data->rx.timeout = K_MSEC(timeout);

	/* Disable RX inerrupts to let DMA to handle it */
	LL_USART_DisableIT_RXNE(UartInstance);

	/* Enable IRQ IDLE to define the end of a
	 * RX DMA transaction.
	 */
	LL_USART_EnableIT_IDLE(UartInstance);

	reload = data->rx.blk_cfg.dest_address != 0 ? true : false;

	data->rx.blk_cfg.block_size = buf_size;
	data->rx.blk_cfg.dest_address = (uint32_t)data->rx.buffer;

	if (reload) {
		ret = dma_reload(data->dev_dma_rx, data->rx.dma_channel,
				  data->rx.blk_cfg.source_address,
				  data->rx.blk_cfg.dest_address,
				  data->rx.blk_cfg.block_size);
	} else {
		ret = dma_config(data->dev_dma_rx, data->rx.dma_channel,
				 &data->rx.dma_cfg);
	}

	if (ret != 0) {
		LOG_ERR("UART ERR: RX DMA config/reload failed!");
		return -EINVAL;
	}

	if (dma_start(data->dev_dma_rx, data->rx.dma_channel)) {
		LOG_ERR("UART ERR: RX DMA start failed!");
		return -1;
	}

	/* Enable RX DMA requests */
	uart_stm32_dma_rx_enable(dev, true);

	/* Request next buffer */
	async_evt_rx_buf_request(data);

	/* Start the RX timer */
	async_timer_start(&data->rx.timeout_work, data->rx.timeout);

	return ret;
}

static int uart_stm32_async_tx_abort(struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	struct dma_status stat;

	if (data->tx.buffer_length == 0) {
		return -EFAULT;
	}

	k_delayed_work_cancel(&data->tx.timeout_work);
	if (!dma_get_status(data->dev_dma_tx, data->tx.dma_channel, &stat)) {
		data->tx.counter = stat.pending_length;
	}

	dma_stop(data->dev_dma_tx, data->tx.dma_channel);
	async_evt_tx_done(data);

	return 0;
}

static void uart_stm32_async_rx_timeout(struct k_work *work)
{
	struct uart_dma_stream *rx_stream = CONTAINER_OF(work,
			struct uart_dma_stream, timeout_work);

	struct uart_stm32_data *data = CONTAINER_OF(rx_stream,
						    struct uart_stm32_data, rx);

	struct device *dev = data->uart_dev;

	uart_stm32_async_rx_disable(dev);
}

static void uart_stm32_async_tx_timeout(struct k_work *work)
{
	struct uart_dma_stream *tx_stream = CONTAINER_OF(work,
			struct uart_dma_stream, timeout_work);

	struct uart_stm32_data *data = CONTAINER_OF(tx_stream,
						    struct uart_stm32_data, tx);

	struct device *dev = data->uart_dev;

	uart_stm32_async_tx_abort(dev);
}

static int uart_stm32_async_rx_buf_rsp(struct device *dev, uint8_t *buf,
				       size_t len)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	printk("replace buffer (%d)\n", len);
	data->rx_next_buffer = buf;
	data->rx_next_buffer_len = len;
	return 0;
}

static int uart_stm32_async_init(struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	data->uart_dev = dev;

	data->dev_dma_rx = device_get_binding(data->rx.dma_name);
	if (data->dev_dma_rx == NULL) {
		LOG_ERR("%s device not found", data->rx.dma_name);
		return -ENODEV;
	}

	data->dev_dma_tx = device_get_binding(data->tx.dma_name);
	if (data->dev_dma_tx == NULL) {
		LOG_ERR("%s device not found", data->tx.dma_name);
		return -ENODEV;
	}

	/* Disable both TX and RX DMA requests */
	uart_stm32_dma_rx_enable(dev, false);
	uart_stm32_dma_tx_enable(dev, false);

	k_delayed_work_init(&data->rx.timeout_work,
			    uart_stm32_async_rx_timeout);
	k_delayed_work_init(&data->tx.timeout_work,
			    uart_stm32_async_tx_timeout);

	/* Configure dma rx config */
	memset(&data->rx.blk_cfg, 0, sizeof(data->rx.blk_cfg));
	data->rx.blk_cfg.source_address = LL_USART_DMA_GetRegAddr(UartInstance);
	data->rx.blk_cfg.dest_address = 0; /* dest not ready */
	data->rx.blk_cfg.source_addr_adj = data->rx.src_addr_increment ?
					   DMA_ADDR_ADJ_INCREMENT :
					   DMA_ADDR_ADJ_NO_CHANGE;

	data->rx.blk_cfg.dest_addr_adj = data->rx.dst_addr_increment ?
					 DMA_ADDR_ADJ_INCREMENT :
					 DMA_ADDR_ADJ_NO_CHANGE;
	/* RX enable circular buffer */
	data->rx.blk_cfg.source_reload_en  = 1;
	data->rx.blk_cfg.dest_reload_en = 1;
	data->rx.blk_cfg.fifo_mode_control = data->rx.fifo_threshold;

	data->rx.dma_cfg.head_block = &data->rx.blk_cfg;
	data->rx.dma_cfg.callback_arg = dev;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	/* Configure dma tx config */
	memset(&data->tx.blk_cfg, 0, sizeof(data->tx.blk_cfg));
	data->tx.blk_cfg.dest_address = LL_USART_DMA_GetRegAddr(UartInstance);
	data->tx.blk_cfg.source_address = 0; /* not ready */
	data->tx.blk_cfg.source_addr_adj = data->tx.src_addr_increment ?
		DMA_ADDR_ADJ_INCREMENT : DMA_ADDR_ADJ_NO_CHANGE;
	data->tx.blk_cfg.dest_addr_adj = data->tx.dst_addr_increment ?
		DMA_ADDR_ADJ_INCREMENT : DMA_ADDR_ADJ_NO_CHANGE;
	data->tx.blk_cfg.fifo_mode_control = data->tx.fifo_threshold;

	data->tx.dma_cfg.head_block = &data->tx.blk_cfg;
	data->tx.dma_cfg.callback_arg = dev;

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
static int uart_stm32_init(struct device *dev)
{
	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	uint32_t ll_parity;
	uint32_t ll_datawidth;

	__uart_stm32_get_clock(dev);
	/* enable clock */
	if (clock_control_on(data->clock,
			(clock_control_subsys_t *)&config->pclken) != 0) {
		return -EIO;
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

#ifdef CONFIG_UART_ASYNC_API
	return uart_stm32_async_init(dev);
#else

	return 0;
#endif
}

#ifdef CONFIG_UART_ASYNC_API
#define DMA_CHANNEL_CONFIG(id, dir)					\
	DT_INST_DMAS_CELL_BY_NAME(id, dir, channel_config)
#define DMA_FEATURES(id, dir)						\
	DT_INST_DMAS_CELL_BY_NAME(id, dir, features)

/* src_dev and dest_dev should be 'MEMORY' or 'PERIPHERAL'. */
#define UART_DMA_CHANNEL_INIT(index, dir, dir_cap, src_dev, dest_dev)	\
{									\
	.dma_name = DT_INST_DMAS_LABEL_BY_NAME(index, dir),		\
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),	\
	.dma_cfg = {							\
		.block_count = 1,					\
		.dma_slot = DT_INST_DMAS_CELL_BY_NAME(index, dir, slot),\
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(	\
					DMA_CHANNEL_CONFIG(index, dir)),\
		.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(\
					DMA_CHANNEL_CONFIG(index, dir)),\
		.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(\
				DMA_CHANNEL_CONFIG(index, dir)),\
		.source_burst_length = 1, /* SINGLE transfer */		\
		.dest_burst_length = 1,					\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(		\
				DMA_CHANNEL_CONFIG(index, dir)),	\
		.dma_callback = uart_stm32_dma_##dir##_cb,		\
	},								\
	.src_addr_increment = STM32_DMA_CONFIG_##src_dev##_ADDR_INC(	\
				DMA_CHANNEL_CONFIG(index, dir)),	\
	.dst_addr_increment = STM32_DMA_CONFIG_##dest_dev##_ADDR_INC(	\
				DMA_CHANNEL_CONFIG(index, dir)),	\
	.fifo_threshold = STM32_DMA_FEATURES_FIFO_THRESHOLD(		\
				DMA_FEATURES(index, dir)),		\
}
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define STM32_UART_IRQ_HANDLER_DECL(index)				\
	static void uart_stm32_irq_config_func_##index(struct device *dev)
#define STM32_UART_IRQ_HANDLER_FUNC(index)				\
	.irq_config_func = uart_stm32_irq_config_func_##index,
#define STM32_UART_IRQ_HANDLER(index)					\
static void uart_stm32_irq_config_func_##index(struct device *dev)	\
{									\
	IRQ_CONNECT(DT_INST_IRQN(index),		\
		DT_INST_IRQ(index, priority),		\
		uart_stm32_isr, DEVICE_GET(uart_stm32_##index),		\
		0);							\
	irq_enable(DT_INST_IRQN(index));		\
}
#else
#define STM32_UART_IRQ_HANDLER_DECL(index)
#define STM32_UART_IRQ_HANDLER_FUNC(index)
#define STM32_UART_IRQ_HANDLER(index)
#endif

#ifdef CONFIG_UART_ASYNC_API
#define STM32_UART_DMA_DATA(index, dir, DIR, src, dest)			\
.dir = COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),			\
		 (UART_DMA_CHANNEL_INIT(index, dir, DIR, src, dest)),	\
		 NULL),
#else
#define STM32_UART_DMA_DATA(index, dir, DIR, src, dest)
#endif

#define STM32_UART_INIT(index)						\
STM32_UART_IRQ_HANDLER_DECL(index);					\
									\
static const struct uart_stm32_config uart_stm32_cfg_##index = {	\
	.uconf = {							\
		.base = (uint8_t *)DT_INST_REG_ADDR(index),\
		STM32_UART_IRQ_HANDLER_FUNC(index)			\
	},								\
	.pclken = { .bus = DT_INST_CLOCKS_CELL(index, bus),	\
		    .enr = DT_INST_CLOCKS_CELL(index, bits)	\
	},								\
	.hw_flow_control = DT_INST_PROP(index, hw_flow_control),\
	.parity = DT_INST_PROP(index, parity)\
};									\
									\
static struct uart_stm32_data uart_stm32_data_##index = {		\
	.baud_rate = DT_INST_PROP(index, current_speed),		\
	STM32_UART_DMA_DATA(index, rx, RX, PERIPHERAL, MEMORY)		\
	STM32_UART_DMA_DATA(index, tx, TX, MEMORY, PERIPHERAL)		\
};									\
									\
DEVICE_AND_API_INIT(uart_stm32_##index, DT_INST_LABEL(index),\
		    &uart_stm32_init,					\
		    &uart_stm32_data_##index, &uart_stm32_cfg_##index,	\
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &uart_stm32_driver_api);				\
									\
STM32_UART_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(STM32_UART_INIT)
