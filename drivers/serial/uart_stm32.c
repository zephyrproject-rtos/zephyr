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

#include <linker/sections.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include "uart_stm32.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(uart_stm32);

#define HAS_LPUART_1 (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpuart1), \
					 st_stm32_lpuart, okay))

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct uart_stm32_config * const)(dev)->config)
#define DEV_DATA(dev)							\
	((struct uart_stm32_data * const)(dev)->data)
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

static void uart_stm32_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_stm32_data *data = DEV_DATA(dev);

	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->uconf.irq_config_func(dev);
#endif
	return 0;
}


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
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
	.baud_rate = DT_INST_PROP(index, current_speed)	\
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
