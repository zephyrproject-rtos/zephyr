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

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/policy.h>

#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#endif

#include <zephyr/linker/sections.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include "uart_stm32.h"

#include <stm32_ll_usart.h>
#include <stm32_ll_lpuart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_stm32, CONFIG_UART_LOG_LEVEL);

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with an optional clock */
#if STM32_DT_INST_DEV_OPT_CLOCK_SUPPORT
#define STM32_UART_OPT_CLOCK_SUPPORT 1
#else
#define STM32_UART_OPT_CLOCK_SUPPORT 0
#endif

#define HAS_LPUART_1 (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(lpuart1), \
					 st_stm32_lpuart, okay))

#if HAS_LPUART_1
#ifdef USART_PRESC_PRESCALER
uint32_t lpuartdiv_calc(const uint64_t clock_rate, const uint16_t presc_idx,
			const uint32_t baud_rate)
{
	uint64_t lpuartdiv;

	lpuartdiv = clock_rate / LPUART_PRESCALER_TAB[presc_idx];
	lpuartdiv *= LPUART_LPUARTDIV_FREQ_MUL;
	lpuartdiv += baud_rate / 2;
	lpuartdiv /= baud_rate;

	return (uint32_t)lpuartdiv;
}
#else
uint32_t lpuartdiv_calc(const uint64_t clock_rate, const uint32_t baud_rate)
{
	uint64_t lpuartdiv;

	lpuartdiv = clock_rate * LPUART_LPUARTDIV_FREQ_MUL;
	lpuartdiv += baud_rate / 2;
	lpuartdiv /= baud_rate;

	return (uint32_t)lpuartdiv;
}
#endif /* USART_PRESC_PRESCALER */
#endif /* HAS_LPUART_1 */

#ifdef CONFIG_PM
static void uart_stm32_pm_policy_state_lock_get(const struct device *dev)
{
	struct uart_stm32_data *data = dev->data;

	if (!data->pm_policy_state_on) {
		data->pm_policy_state_on = true;
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

static void uart_stm32_pm_policy_state_lock_put(const struct device *dev)
{
	struct uart_stm32_data *data = dev->data;

	if (data->pm_policy_state_on) {
		data->pm_policy_state_on = false;
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}
#endif /* CONFIG_PM */

static inline void uart_stm32_set_baudrate(const struct device *dev, uint32_t baud_rate)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;

	uint32_t clock_rate;

	/* Get clock rate */
	if (IS_ENABLED(STM32_UART_OPT_CLOCK_SUPPORT) && (config->pclk_len > 1)) {
		if (clock_control_get_rate(data->clock,
					   (clock_control_subsys_t)&config->pclken[1],
					   &clock_rate) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclken[1])");
			return;
		}
	} else {
		if (clock_control_get_rate(data->clock,
					   (clock_control_subsys_t)&config->pclken[0],
					   &clock_rate) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclken[0])");
			return;
		}
	}

#if HAS_LPUART_1
	if (IS_LPUART_INSTANCE(config->usart)) {
		uint32_t lpuartdiv;
#ifdef USART_PRESC_PRESCALER
		uint8_t presc_idx;
		uint32_t presc_val;

		for (presc_idx = 0; presc_idx < ARRAY_SIZE(LPUART_PRESCALER_TAB); presc_idx++) {
			lpuartdiv = lpuartdiv_calc(clock_rate, presc_idx, baud_rate);
			if (lpuartdiv >= LPUART_BRR_MIN_VALUE && lpuartdiv <= LPUART_BRR_MASK) {
				break;
			}
		}

		if (presc_idx == ARRAY_SIZE(LPUART_PRESCALER_TAB)) {
			LOG_ERR("Unable to set %s to %d", dev->name, baud_rate);
			return;
		}

		presc_val = presc_idx << USART_PRESC_PRESCALER_Pos;

		LL_LPUART_SetPrescaler(config->usart, presc_val);
#else
		lpuartdiv = lpuartdiv_calc(clock_rate, baud_rate);
		if (lpuartdiv < LPUART_BRR_MIN_VALUE || lpuartdiv > LPUART_BRR_MASK) {
			LOG_ERR("Unable to set %s to %d", dev->name, baud_rate);
			return;
		}
#endif /* USART_PRESC_PRESCALER */
		LL_LPUART_SetBaudRate(config->usart,
				      clock_rate,
#ifdef USART_PRESC_PRESCALER
				      presc_val,
#endif
				      baud_rate);
	} else {
#endif /* HAS_LPUART_1 */
#ifdef USART_CR1_OVER8
		LL_USART_SetOverSampling(config->usart,
					 LL_USART_OVERSAMPLING_16);
#endif
		LL_USART_SetBaudRate(config->usart,
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
	const struct uart_stm32_config *config = dev->config;

	LL_USART_SetParity(config->usart, parity);
}

static inline uint32_t uart_stm32_get_parity(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	return LL_USART_GetParity(config->usart);
}

static inline void uart_stm32_set_stopbits(const struct device *dev,
					   uint32_t stopbits)
{
	const struct uart_stm32_config *config = dev->config;

	LL_USART_SetStopBitsLength(config->usart, stopbits);
}

static inline uint32_t uart_stm32_get_stopbits(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	return LL_USART_GetStopBitsLength(config->usart);
}

static inline void uart_stm32_set_databits(const struct device *dev,
					   uint32_t databits)
{
	const struct uart_stm32_config *config = dev->config;

	LL_USART_SetDataWidth(config->usart, databits);
}

static inline uint32_t uart_stm32_get_databits(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	return LL_USART_GetDataWidth(config->usart);
}

static inline void uart_stm32_set_hwctrl(const struct device *dev,
					 uint32_t hwctrl)
{
	const struct uart_stm32_config *config = dev->config;

	LL_USART_SetHWFlowCtrl(config->usart, hwctrl);
}

static inline uint32_t uart_stm32_get_hwctrl(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	return LL_USART_GetHWFlowCtrl(config->usart);
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

static inline uint32_t uart_stm32_cfg2ll_databits(enum uart_config_data_bits db,
						  enum uart_config_parity p)
{
	switch (db) {
/* Some MCU's don't support 7B or 9B datawidth */
#ifdef LL_USART_DATAWIDTH_7B
	case UART_CFG_DATA_BITS_7:
		if (p == UART_CFG_PARITY_NONE) {
			return LL_USART_DATAWIDTH_7B;
		} else {
			return LL_USART_DATAWIDTH_8B;
		}
#endif	/* LL_USART_DATAWIDTH_7B */
#ifdef LL_USART_DATAWIDTH_9B
	case UART_CFG_DATA_BITS_9:
		return LL_USART_DATAWIDTH_9B;
#endif	/* LL_USART_DATAWIDTH_9B */
	case UART_CFG_DATA_BITS_8:
	default:
		if (p == UART_CFG_PARITY_NONE) {
			return LL_USART_DATAWIDTH_8B;
#ifdef LL_USART_DATAWIDTH_9B
		} else {
			return LL_USART_DATAWIDTH_9B;
#endif
		}
		return LL_USART_DATAWIDTH_8B;
	}
}

static inline enum uart_config_data_bits uart_stm32_ll2cfg_databits(uint32_t db,
								    uint32_t p)
{
	switch (db) {
/* Some MCU's don't support 7B or 9B datawidth */
#ifdef LL_USART_DATAWIDTH_7B
	case LL_USART_DATAWIDTH_7B:
		if (p == LL_USART_PARITY_NONE) {
			return UART_CFG_DATA_BITS_7;
		} else {
			return UART_CFG_DATA_BITS_6;
		}
#endif	/* LL_USART_DATAWIDTH_7B */
#ifdef LL_USART_DATAWIDTH_9B
	case LL_USART_DATAWIDTH_9B:
		if (p == LL_USART_PARITY_NONE) {
			return UART_CFG_DATA_BITS_9;
		} else {
			return UART_CFG_DATA_BITS_8;
		}
#endif	/* LL_USART_DATAWIDTH_9B */
	case LL_USART_DATAWIDTH_8B:
	default:
		if (p == LL_USART_PARITY_NONE) {
			return UART_CFG_DATA_BITS_8;
		} else {
			return UART_CFG_DATA_BITS_7;
		}
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

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_stm32_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;
	const uint32_t parity = uart_stm32_cfg2ll_parity(cfg->parity);
	const uint32_t stopbits = uart_stm32_cfg2ll_stopbits(cfg->stop_bits);
	const uint32_t databits = uart_stm32_cfg2ll_databits(cfg->data_bits,
							     cfg->parity);
	const uint32_t flowctrl = uart_stm32_cfg2ll_hwctrl(cfg->flow_ctrl);

	/* Hardware doesn't support mark or space parity */
	if ((cfg->parity == UART_CFG_PARITY_MARK) ||
	    (cfg->parity == UART_CFG_PARITY_SPACE)) {
		return -ENOTSUP;
	}

	/* Driver does not supports parity + 9 databits */
	if ((cfg->parity != UART_CFG_PARITY_NONE) &&
	    (cfg->data_bits == UART_CFG_DATA_BITS_9)) {
		return -ENOTSUP;
	}

#if defined(LL_USART_STOPBITS_0_5) && HAS_LPUART_1
	if (IS_LPUART_INSTANCE(config->usart) &&
	    (cfg->stop_bits == UART_CFG_STOP_BITS_0_5)) {
		return -ENOTSUP;
	}
#else
	if (cfg->stop_bits == UART_CFG_STOP_BITS_0_5) {
		return -ENOTSUP;
	}
#endif

#if defined(LL_USART_STOPBITS_1_5) && HAS_LPUART_1
	if (IS_LPUART_INSTANCE(config->usart) &&
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
		if (!IS_UART_HWFLOW_INSTANCE(config->usart) ||
		    UART_CFG_FLOW_CTRL_RTS_CTS != cfg->flow_ctrl) {
			return -ENOTSUP;
		}
	}

	LL_USART_Disable(config->usart);

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

	LL_USART_Enable(config->usart);
	return 0;
};

static int uart_stm32_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct uart_stm32_data *data = dev->data;

	cfg->baudrate = data->baud_rate;
	cfg->parity = uart_stm32_ll2cfg_parity(uart_stm32_get_parity(dev));
	cfg->stop_bits = uart_stm32_ll2cfg_stopbits(
		uart_stm32_get_stopbits(dev));
	cfg->data_bits = uart_stm32_ll2cfg_databits(
		uart_stm32_get_databits(dev), uart_stm32_get_parity(dev));
	cfg->flow_ctrl = uart_stm32_ll2cfg_hwctrl(
		uart_stm32_get_hwctrl(dev));
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_stm32_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_stm32_config *config = dev->config;

	/* Clear overrun error flag */
	if (LL_USART_IsActiveFlag_ORE(config->usart)) {
		LL_USART_ClearFlag_ORE(config->usart);
	}

	/*
	 * On stm32 F4X, F1X, and F2X, the RXNE flag is affected (cleared) by
	 * the uart_err_check function call (on errors flags clearing)
	 */
	if (!LL_USART_IsActiveFlag_RXNE(config->usart)) {
		return -1;
	}

	*c = (unsigned char)LL_USART_ReceiveData8(config->usart);

	return 0;
}

static void uart_stm32_poll_out(const struct device *dev,
					unsigned char c)
{
	const struct uart_stm32_config *config = dev->config;
#ifdef CONFIG_PM
	struct uart_stm32_data *data = dev->data;
#endif
	unsigned int key;

	/* Wait for TXE flag to be raised
	 * When TXE flag is raised, we lock interrupts to prevent interrupts (notably that of usart)
	 * or thread switch. Then, we can safely send our character. The character sent will be
	 * interlaced with the characters potentially send with interrupt transmission API
	 */
	while (1) {
		if (LL_USART_IsActiveFlag_TXE(config->usart)) {
			key = irq_lock();
			if (LL_USART_IsActiveFlag_TXE(config->usart)) {
				break;
			}
			irq_unlock(key);
		}
	}

#ifdef CONFIG_PM

	/* If an interrupt transmission is in progress, the pm constraint is already managed by the
	 * call of uart_stm32_irq_tx_[en|dis]able
	 */
	if (!data->tx_poll_stream_on && !data->tx_int_stream_on) {
		data->tx_poll_stream_on = true;

		/* Don't allow system to suspend until stream
		 * transmission has completed
		 */
		uart_stm32_pm_policy_state_lock_get(dev);

		/* Enable TC interrupt so we can release suspend
		 * constraint when done
		 */
		LL_USART_EnableIT_TC(config->usart);
	}
#endif /* CONFIG_PM */

	LL_USART_TransmitData8(config->usart, (uint8_t)c);
	irq_unlock(key);
}

static int uart_stm32_err_check(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;
	uint32_t err = 0U;

	/* Check for errors, then clear them.
	 * Some SoC clear all error flags when at least
	 * one is cleared. (e.g. F4X, F1X, and F2X).
	 * The stm32 F4X, F1X, and F2X also reads the usart DR when clearing Errors
	 */
	if (LL_USART_IsActiveFlag_ORE(config->usart)) {
		err |= UART_ERROR_OVERRUN;
	}

	if (LL_USART_IsActiveFlag_PE(config->usart)) {
		err |= UART_ERROR_PARITY;
	}

	if (LL_USART_IsActiveFlag_FE(config->usart)) {
		err |= UART_ERROR_FRAMING;
	}

#if !defined(CONFIG_SOC_SERIES_STM32F0X) || defined(USART_LIN_SUPPORT)
	if (LL_USART_IsActiveFlag_LBD(config->usart)) {
		err |= UART_BREAK;
	}

	if (err & UART_BREAK) {
		LL_USART_ClearFlag_LBD(config->usart);
	}
#endif
	/* Clearing error :
	 * the stm32 F4X, F1X, and F2X sw sequence is reading the usart SR
	 * then the usart DR to clear the Error flags ORE, PE, FE, NE
	 * --> so is the RXNE flag also cleared !
	 */
	if (err & UART_ERROR_OVERRUN) {
		LL_USART_ClearFlag_ORE(config->usart);
	}

	if (err & UART_ERROR_PARITY) {
		LL_USART_ClearFlag_PE(config->usart);
	}

	if (err & UART_ERROR_FRAMING) {
		LL_USART_ClearFlag_FE(config->usart);
	}
	/* Clear noise error as well,
	 * it is not represented by the errors enum
	 */
	LL_USART_ClearFlag_NE(config->usart);

	return err;
}

static inline void __uart_stm32_get_clock(const struct device *dev)
{
	struct uart_stm32_data *data = dev->data;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	data->clock = clk;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_stm32_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int size)
{
	const struct uart_stm32_config *config = dev->config;
	uint8_t num_tx = 0U;
	unsigned int key;

	if (!LL_USART_IsActiveFlag_TXE(config->usart)) {
		return num_tx;
	}

	/* Lock interrupts to prevent nested interrupts or thread switch */
	key = irq_lock();

	while ((size - num_tx > 0) &&
	       LL_USART_IsActiveFlag_TXE(config->usart)) {
		/* TXE flag will be cleared with byte write to DR|RDR register */

		/* Send a character (8bit , parity none) */
		LL_USART_TransmitData8(config->usart, tx_data[num_tx++]);
	}

	irq_unlock(key);

	return num_tx;
}

static int uart_stm32_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	const struct uart_stm32_config *config = dev->config;
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) &&
	       LL_USART_IsActiveFlag_RXNE(config->usart)) {
		/* RXNE flag will be cleared upon read from DR|RDR register */

		/* Receive a character (8bit , parity none) */
		rx_data[num_rx++] = LL_USART_ReceiveData8(config->usart);

		/* Clear overrun error flag */
		if (LL_USART_IsActiveFlag_ORE(config->usart)) {
			LL_USART_ClearFlag_ORE(config->usart);
		/*
		 * On stm32 F4X, F1X, and F2X, the RXNE flag is affected (cleared) by
		 * the uart_err_check function call (on errors flags clearing)
		 */
		}
	}

	return num_rx;
}

static void uart_stm32_irq_tx_enable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;
#ifdef CONFIG_PM
	struct uart_stm32_data *data = dev->data;
	unsigned int key;
#endif

#ifdef CONFIG_PM
	key = irq_lock();
	data->tx_poll_stream_on = false;
	data->tx_int_stream_on = true;
	uart_stm32_pm_policy_state_lock_get(dev);
#endif
	LL_USART_EnableIT_TC(config->usart);

#ifdef CONFIG_PM
	irq_unlock(key);
#endif
}

static void uart_stm32_irq_tx_disable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;
#ifdef CONFIG_PM
	struct uart_stm32_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
#endif

	LL_USART_DisableIT_TC(config->usart);

#ifdef CONFIG_PM
	data->tx_int_stream_on = false;
	uart_stm32_pm_policy_state_lock_put(dev);
#endif

#ifdef CONFIG_PM
	irq_unlock(key);
#endif
}

static int uart_stm32_irq_tx_ready(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	return LL_USART_IsActiveFlag_TXE(config->usart) &&
		LL_USART_IsEnabledIT_TC(config->usart);
}

static int uart_stm32_irq_tx_complete(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	return LL_USART_IsActiveFlag_TC(config->usart);
}

static void uart_stm32_irq_rx_enable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	LL_USART_EnableIT_RXNE(config->usart);
}

static void uart_stm32_irq_rx_disable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	LL_USART_DisableIT_RXNE(config->usart);
}

static int uart_stm32_irq_rx_ready(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;
	/*
	 * On stm32 F4X, F1X, and F2X, the RXNE flag is affected (cleared) by
	 * the uart_err_check function call (on errors flags clearing)
	 */
	return LL_USART_IsActiveFlag_RXNE(config->usart);
}

static void uart_stm32_irq_err_enable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	/* Enable FE, ORE interruptions */
	LL_USART_EnableIT_ERROR(config->usart);
#if !defined(CONFIG_SOC_SERIES_STM32F0X) || defined(USART_LIN_SUPPORT)
	/* Enable Line break detection */
	if (IS_UART_LIN_INSTANCE(config->usart)) {
		LL_USART_EnableIT_LBD(config->usart);
	}
#endif
	/* Enable parity error interruption */
	LL_USART_EnableIT_PE(config->usart);
}

static void uart_stm32_irq_err_disable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	/* Disable FE, ORE interruptions */
	LL_USART_DisableIT_ERROR(config->usart);
#if !defined(CONFIG_SOC_SERIES_STM32F0X) || defined(USART_LIN_SUPPORT)
	/* Disable Line break detection */
	if (IS_UART_LIN_INSTANCE(config->usart)) {
		LL_USART_DisableIT_LBD(config->usart);
	}
#endif
	/* Disable parity error interruption */
	LL_USART_DisableIT_PE(config->usart);
}

static int uart_stm32_irq_is_pending(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	return ((LL_USART_IsActiveFlag_RXNE(config->usart) &&
		 LL_USART_IsEnabledIT_RXNE(config->usart)) ||
		(LL_USART_IsActiveFlag_TC(config->usart) &&
		 LL_USART_IsEnabledIT_TC(config->usart)));
}

static int uart_stm32_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_stm32_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_stm32_data *data = dev->data;

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

static inline void async_timer_start(struct k_work_delayable *work,
				     int32_t timeout)
{
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		/* start timer */
		LOG_DBG("async timer started for %d us", timeout);
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static void uart_stm32_dma_rx_flush(const struct device *dev)
{
	struct dma_status stat;
	struct uart_stm32_data *data = dev->data;

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

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || \
	defined(CONFIG_UART_ASYNC_API) || \
	defined(CONFIG_PM)

static void uart_stm32_isr(const struct device *dev)
{
	struct uart_stm32_data *data = dev->data;
#if defined(CONFIG_PM) || defined(CONFIG_UART_ASYNC_API)
	const struct uart_stm32_config *config = dev->config;
#endif

#ifdef CONFIG_PM
	if (LL_USART_IsEnabledIT_TC(config->usart) &&
		LL_USART_IsActiveFlag_TC(config->usart)) {

		if (data->tx_poll_stream_on) {
			/* A poll stream transmission just completed,
			 * allow system to suspend
			 */
			LL_USART_DisableIT_TC(config->usart);
			data->tx_poll_stream_on = false;
			uart_stm32_pm_policy_state_lock_put(dev);
		}
		/* Stream transmission was either async or IRQ based,
		 * constraint will be released at the same time TC IT
		 * is disabled
		 */
	}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	if (LL_USART_IsEnabledIT_IDLE(config->usart) &&
			LL_USART_IsActiveFlag_IDLE(config->usart)) {

		LL_USART_ClearFlag_IDLE(config->usart);

		LOG_DBG("idle interrupt occurred");

		/* Start the RX timer */
		async_timer_start(&data->dma_rx.timeout_work,
							data->dma_rx.timeout);

		if (data->dma_rx.timeout == 0) {
			uart_stm32_dma_rx_flush(dev);
		}
	} else if (LL_USART_IsEnabledIT_TC(config->usart) &&
			LL_USART_IsActiveFlag_TC(config->usart)) {

		LL_USART_DisableIT_TC(config->usart);
		LL_USART_ClearFlag_TC(config->usart);
		/* Generate TX_DONE event when transmission is done */
		async_evt_tx_done(data);

#ifdef CONFIG_PM
		uart_stm32_pm_policy_state_lock_put(dev);
#endif
	} else if (LL_USART_IsEnabledIT_RXNE(config->usart) &&
			LL_USART_IsActiveFlag_RXNE(config->usart)) {
#ifdef USART_SR_RXNE
		/* clear the RXNE flag, because Rx data was not read */
		LL_USART_ClearFlag_RXNE(config->usart);
#else
		/* clear the RXNE by flushing the fifo, because Rx data was not read */
		LL_USART_RequestRxDataFlush(config->usart);
#endif /* USART_SR_RXNE */
	}

	/* Clear errors */
	uart_stm32_err_check(dev);
#endif /* CONFIG_UART_ASYNC_API */
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API || CONFIG_PM */

#ifdef CONFIG_UART_ASYNC_API

static int uart_stm32_async_callback_set(const struct device *dev,
					 uart_callback_t callback,
					 void *user_data)
{
	struct uart_stm32_data *data = dev->data;

	data->async_cb = callback;
	data->async_user_data = user_data;

	return 0;
}

static inline void uart_stm32_dma_tx_enable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	LL_USART_EnableDMAReq_TX(config->usart);
}

static inline void uart_stm32_dma_tx_disable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	LL_USART_DisableDMAReq_TX(config->usart);
}

static inline void uart_stm32_dma_rx_enable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;

	LL_USART_EnableDMAReq_RX(config->usart);

	data->dma_rx.enabled = true;
}

static inline void uart_stm32_dma_rx_disable(const struct device *dev)
{
	struct uart_stm32_data *data = dev->data;

	data->dma_rx.enabled = false;
}

static int uart_stm32_async_rx_disable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;
	struct uart_event disabled_event = {
		.type = UART_RX_DISABLED
	};

	if (!data->dma_rx.enabled) {
		async_user_callback(data, &disabled_event);
		return -EFAULT;
	}

	LL_USART_DisableIT_IDLE(config->usart);

	uart_stm32_dma_rx_flush(dev);

	async_evt_rx_buf_release(data);

	uart_stm32_dma_rx_disable(dev);

	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);

	dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	/* When async rx is disabled, enable interruptible instance of uart to function normally */
	LL_USART_EnableIT_RXNE(config->usart);

	LOG_DBG("rx: disabled");

	async_user_callback(data, &disabled_event);

	return 0;
}

void uart_stm32_dma_tx_cb(const struct device *dma_dev, void *user_data,
			       uint32_t channel, int status)
{
	const struct device *uart_dev = user_data;
	struct uart_stm32_data *data = uart_dev->data;
	struct dma_status stat;
	unsigned int key = irq_lock();

	/* Disable TX */
	uart_stm32_dma_tx_disable(uart_dev);

	(void)k_work_cancel_delayable(&data->dma_tx.timeout_work);

	if (!dma_get_status(data->dma_tx.dma_dev,
				data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = data->dma_tx.buffer_length -
					stat.pending_length;
	}

	data->dma_tx.buffer_length = 0;

	irq_unlock(key);
}

static void uart_stm32_dma_replace_buffer(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;

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

	LL_USART_ClearFlag_IDLE(config->usart);

	/* Request next buffer */
	async_evt_rx_buf_request(data);
}

void uart_stm32_dma_rx_cb(const struct device *dma_dev, void *user_data,
			       uint32_t channel, int status)
{
	const struct device *uart_dev = user_data;
	struct uart_stm32_data *data = uart_dev->data;

	if (status != 0) {
		async_evt_rx_err(data, status);
		return;
	}

	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);

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
		k_work_reschedule(&data->dma_rx.timeout_work, K_TICKS(1));
	}
}

static int uart_stm32_async_tx(const struct device *dev,
		const uint8_t *tx_data, size_t buf_size, int32_t timeout)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;
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

	/* Clear TC flag */
	LL_USART_ClearFlag_TC(config->usart);

	/* Enable TC interrupt so we can signal correct TX done */
	LL_USART_EnableIT_TC(config->usart);

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

#ifdef CONFIG_PM

	/* Do not allow system to suspend until transmission has completed */
	uart_stm32_pm_policy_state_lock_get(dev);
#endif

	/* Enable TX DMA requests */
	uart_stm32_dma_tx_enable(dev);

	return 0;
}

static int uart_stm32_async_rx_enable(const struct device *dev,
		uint8_t *rx_buf, size_t buf_size, int32_t timeout)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;
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
	LL_USART_DisableIT_RXNE(config->usart);

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
	LL_USART_ClearFlag_IDLE(config->usart);
	LL_USART_EnableIT_IDLE(config->usart);

	LL_USART_EnableIT_ERROR(config->usart);

	/* Request next buffer */
	async_evt_rx_buf_request(data);

	LOG_DBG("async rx enabled");

	return ret;
}

static int uart_stm32_async_tx_abort(const struct device *dev)
{
	struct uart_stm32_data *data = dev->data;
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

	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	async_evt_tx_abort(data);

	return 0;
}

static void uart_stm32_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_stream *rx_stream = CONTAINER_OF(dwork,
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
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_stream *tx_stream = CONTAINER_OF(dwork,
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
	struct uart_stm32_data *data = dev->data;

	LOG_DBG("replace buffer (%d)", len);
	data->rx_next_buffer = buf;
	data->rx_next_buffer_len = len;

	return 0;
}

static int uart_stm32_async_init(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;

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
	uart_stm32_dma_rx_disable(dev);
	uart_stm32_dma_tx_disable(dev);

	k_work_init_delayable(&data->dma_rx.timeout_work,
			    uart_stm32_async_rx_timeout);
	k_work_init_delayable(&data->dma_tx.timeout_work,
			    uart_stm32_async_tx_timeout);

	/* Configure dma rx config */
	memset(&data->dma_rx.blk_cfg, 0, sizeof(data->dma_rx.blk_cfg));

#if defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32L1X)
	data->dma_rx.blk_cfg.source_address =
				LL_USART_DMA_GetRegAddr(config->usart);
#else
	data->dma_rx.blk_cfg.source_address =
				LL_USART_DMA_GetRegAddr(config->usart,
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
			LL_USART_DMA_GetRegAddr(config->usart);
#else
	data->dma_tx.blk_cfg.dest_address =
			LL_USART_DMA_GetRegAddr(config->usart,
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
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_stm32_configure,
	.config_get = uart_stm32_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
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
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;
	uint32_t ll_parity;
	uint32_t ll_datawidth;
	int err;

	__uart_stm32_get_clock(dev);
	/* enable clock */
	err = clock_control_on(data->clock, (clock_control_subsys_t)&config->pclken[0]);
	if (err != 0) {
		LOG_ERR("Could not enable (LP)UART clock");
		return err;
	}

	if (IS_ENABLED(STM32_UART_OPT_CLOCK_SUPPORT) && (config->pclk_len > 1)) {
		err = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					      (clock_control_subsys_t) &config->pclken[1],
					      NULL);
		if (err != 0) {
			LOG_ERR("Could not select UART source clock");
			return err;
		}
	}

	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	LL_USART_Disable(config->usart);

	/* TX/RX direction */
	LL_USART_SetTransferDirection(config->usart,
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
	LL_USART_ConfigCharacter(config->usart,
				 ll_datawidth,
				 ll_parity,
				 LL_USART_STOPBITS_1);

	if (config->hw_flow_control) {
		uart_stm32_set_hwctrl(dev, LL_USART_HWCONTROL_RTS_CTS);
	}

	/* Set the default baudrate */
	uart_stm32_set_baudrate(dev, data->baud_rate);

	/* Enable the single wire / half-duplex mode */
	if (config->single_wire) {
		LL_USART_EnableHalfDuplex(config->usart);
	}

#ifdef LL_USART_TXRX_SWAPPED
	if (config->tx_rx_swap) {
		LL_USART_SetTXRXSwap(config->usart, LL_USART_TXRX_SWAPPED);
	}
#endif

#ifdef LL_USART_RXPIN_LEVEL_INVERTED
	if (config->rx_invert) {
		LL_USART_SetRXPinLevel(config->usart, LL_USART_RXPIN_LEVEL_INVERTED);
	}
#endif

#ifdef LL_USART_TXPIN_LEVEL_INVERTED
	if (config->tx_invert) {
		LL_USART_SetTXPinLevel(config->usart, LL_USART_TXPIN_LEVEL_INVERTED);
	}
#endif

	LL_USART_Enable(config->usart);

#ifdef USART_ISR_TEACK
	/* Wait until TEACK flag is set */
	while (!(LL_USART_IsActiveFlag_TEACK(config->usart))) {
	}
#endif /* !USART_ISR_TEACK */

#ifdef USART_ISR_REACK
	/* Wait until REACK flag is set */
	while (!(LL_USART_IsActiveFlag_REACK(config->usart))) {
	}
#endif /* !USART_ISR_REACK */

#if defined(CONFIG_PM) || \
	defined(CONFIG_UART_INTERRUPT_DRIVEN) || \
	defined(CONFIG_UART_ASYNC_API)
	config->irq_config_func(dev);
#endif /* CONFIG_PM || CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_ASYNC_API
	return uart_stm32_async_init(dev);
#else
	return 0;
#endif
}

#ifdef CONFIG_UART_ASYNC_API

/* src_dev and dest_dev should be 'MEMORY' or 'PERIPHERAL'. */
#define UART_DMA_CHANNEL_INIT(index, dir, dir_cap, src_dev, dest_dev)	\
	.dma_dev = DEVICE_DT_GET(STM32_DMA_CTLR(index, dir)),			\
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),	\
	.dma_cfg = {							\
		.dma_slot = STM32_DMA_SLOT(index, dir, slot),\
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(	\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(		\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
		.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(\
					STM32_DMA_CHANNEL_CONFIG(index, dir)),\
		.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),\
		.source_burst_length = 1, /* SINGLE transfer */		\
		.dest_burst_length = 1,					\
		.block_count = 1,					\
		.dma_callback = uart_stm32_dma_##dir##_cb,		\
	},								\
	.src_addr_increment = STM32_DMA_CONFIG_##src_dev##_ADDR_INC(	\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
	.dst_addr_increment = STM32_DMA_CONFIG_##dest_dev##_ADDR_INC(	\
				STM32_DMA_CHANNEL_CONFIG(index, dir)),	\
	.fifo_threshold = STM32_DMA_FEATURES_FIFO_THRESHOLD(		\
				STM32_DMA_FEATURES(index, dir)),		\

#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API) || \
	defined(CONFIG_PM)
#define STM32_UART_IRQ_HANDLER_DECL(index)				\
	static void uart_stm32_irq_config_func_##index(const struct device *dev);
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
#define STM32_UART_IRQ_HANDLER_DECL(index) /* Not used */
#define STM32_UART_IRQ_HANDLER(index) /* Not used */
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API) || \
	defined(CONFIG_PM)
#define STM32_UART_IRQ_HANDLER_FUNC(index)				\
	.irq_config_func = uart_stm32_irq_config_func_##index,
#else
#define STM32_UART_IRQ_HANDLER_FUNC(index) /* Not used */
#endif

#ifdef CONFIG_UART_ASYNC_API
#define UART_DMA_CHANNEL(index, dir, DIR, src, dest)			\
.dma_##dir = {								\
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),			\
		 (UART_DMA_CHANNEL_INIT(index, dir, DIR, src, dest)),	\
		 (NULL))						\
	},

#else
#define UART_DMA_CHANNEL(index, dir, DIR, src, dest)
#endif

#define STM32_UART_INIT(index)						\
STM32_UART_IRQ_HANDLER_DECL(index)					\
									\
PINCTRL_DT_INST_DEFINE(index);						\
									\
static const struct stm32_pclken pclken_##index[] =			\
					    STM32_DT_INST_CLOCKS(index);\
									\
static const struct uart_stm32_config uart_stm32_cfg_##index = {	\
	.usart = (USART_TypeDef *)DT_INST_REG_ADDR(index),		\
	.pclken = pclken_##index,					\
	.pclk_len = DT_INST_NUM_CLOCKS(index),				\
	.hw_flow_control = DT_INST_PROP(index, hw_flow_control),	\
	.parity = DT_INST_ENUM_IDX_OR(index, parity, UART_CFG_PARITY_NONE),	\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
	.single_wire = DT_INST_PROP_OR(index, single_wire, false),	\
	.tx_rx_swap = DT_INST_PROP_OR(index, tx_rx_swap, false),	\
	.rx_invert = DT_INST_PROP(index, rx_invert),	\
	.tx_invert = DT_INST_PROP(index, tx_invert),	\
	STM32_UART_IRQ_HANDLER_FUNC(index)				\
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
		    NULL,						\
		    &uart_stm32_data_##index, &uart_stm32_cfg_##index,	\
		    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,		\
		    &uart_stm32_driver_api);				\
									\
STM32_UART_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(STM32_UART_INIT)
