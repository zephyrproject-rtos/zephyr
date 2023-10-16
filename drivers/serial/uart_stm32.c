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
#include <zephyr/drivers/interrupt_controller/exti_stm32.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>

#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#endif

#include <zephyr/linker/sections.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include "uart_stm32.h"

#include <stm32_ll_usart.h>
#include <stm32_ll_lpuart.h>
#if defined(CONFIG_PM) && defined(IS_UART_WAKEUP_FROMSTOP_INSTANCE)
#include <stm32_ll_exti.h>
#endif /* CONFIG_PM */

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(uart_stm32, CONFIG_UART_LOG_LEVEL);

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32_UART_DOMAIN_CLOCK_SUPPORT 1
#else
#define STM32_UART_DOMAIN_CLOCK_SUPPORT 0
#endif

#define HAS_LPUART DT_HAS_COMPAT_STATUS_OKAY(st_stm32_lpuart)

/* Available everywhere except l1, f1, f2, f4. */
#ifdef USART_CR3_DEM
#define HAS_DRIVER_ENABLE 1
#else
#define HAS_DRIVER_ENABLE 0
#endif

#if HAS_LPUART
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
#endif /* HAS_LPUART */

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
	if (IS_ENABLED(STM32_UART_DOMAIN_CLOCK_SUPPORT) && (config->pclk_len > 1)) {
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

#if HAS_LPUART
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
		/* Check BRR is greater than or equal to 0x300 */
		__ASSERT(LL_LPUART_ReadReg(config->usart, BRR) >= 0x300U,
			 "BaudRateReg >= 0x300");

		/* Check BRR is lower than or equal to 0xFFFFF */
		__ASSERT(LL_LPUART_ReadReg(config->usart, BRR) < 0x000FFFFFU,
			 "BaudRateReg < 0xFFFF");
	} else {
#endif /* HAS_LPUART */
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
		/* Check BRR is greater than or equal to 16d */
		__ASSERT(LL_USART_ReadReg(config->usart, BRR) >= 16,
			 "BaudRateReg >= 16");

#if HAS_LPUART
	}
#endif /* HAS_LPUART */
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

#if HAS_DRIVER_ENABLE
static inline void uart_stm32_set_driver_enable(const struct device *dev,
						bool driver_enable)
{
	const struct uart_stm32_config *config = dev->config;

	if (driver_enable) {
		LL_USART_EnableDEMode(config->usart);
	} else {
		LL_USART_DisableDEMode(config->usart);
	}
}

static inline bool uart_stm32_get_driver_enable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	return LL_USART_IsEnabledDEMode(config->usart);
}
#endif

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

static inline uint32_t uart_stm32_cfg2ll_stopbits(const struct uart_stm32_config *config,
							enum uart_config_stop_bits sb)
{
	switch (sb) {
/* Some MCU's don't support 0.5 stop bits */
#ifdef LL_USART_STOPBITS_0_5
	case UART_CFG_STOP_BITS_0_5:
#if HAS_LPUART
		if (IS_LPUART_INSTANCE(config->usart)) {
			/* return the default */
			return LL_USART_STOPBITS_1;
		}
#endif /* HAS_LPUART */
		return LL_USART_STOPBITS_0_5;
#endif	/* LL_USART_STOPBITS_0_5 */
	case UART_CFG_STOP_BITS_1:
		return LL_USART_STOPBITS_1;
/* Some MCU's don't support 1.5 stop bits */
#ifdef LL_USART_STOPBITS_1_5
	case UART_CFG_STOP_BITS_1_5:
#if HAS_LPUART
		if (IS_LPUART_INSTANCE(config->usart)) {
			/* return the default */
			return LL_USART_STOPBITS_2;
		}
#endif
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
 * @note   Supports only UART_CFG_FLOW_CTRL_RTS_CTS and UART_CFG_FLOW_CTRL_RS485.
 * @param  fc: Zephyr hardware flow control option.
 * @retval LL_USART_HWCONTROL_RTS_CTS, or LL_USART_HWCONTROL_NONE.
 */
static inline uint32_t uart_stm32_cfg2ll_hwctrl(enum uart_config_flow_control fc)
{
	if (fc == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return LL_USART_HWCONTROL_RTS_CTS;
	} else if (fc == UART_CFG_FLOW_CTRL_RS485) {
		/* Driver Enable is handled separately */
		return LL_USART_HWCONTROL_NONE;
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

static void uart_stm32_parameters_set(const struct device *dev,
				      const struct uart_config *cfg)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;
	const uint32_t parity = uart_stm32_cfg2ll_parity(cfg->parity);
	const uint32_t stopbits = uart_stm32_cfg2ll_stopbits(config, cfg->stop_bits);
	const uint32_t databits = uart_stm32_cfg2ll_databits(cfg->data_bits,
							     cfg->parity);
	const uint32_t flowctrl = uart_stm32_cfg2ll_hwctrl(cfg->flow_ctrl);
#if HAS_DRIVER_ENABLE
	bool driver_enable = cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485;
#endif

	if (cfg == uart_cfg) {
		/* Called via (re-)init function, so the SoC either just booted,
		 * or is returning from a low-power state where it lost register
		 * contents
		 */
		LL_USART_ConfigCharacter(config->usart,
					 databits,
					 parity,
					 stopbits);
		uart_stm32_set_hwctrl(dev, flowctrl);
		uart_stm32_set_baudrate(dev, cfg->baudrate);
	} else {
		/* Called from application/subsys via uart_configure syscall */
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

#if HAS_DRIVER_ENABLE
		if (driver_enable != uart_stm32_get_driver_enable(dev)) {
			uart_stm32_set_driver_enable(dev, driver_enable);
		}
#endif

		if (cfg->baudrate != uart_cfg->baudrate) {
			uart_stm32_set_baudrate(dev, cfg->baudrate);
			uart_cfg->baudrate = cfg->baudrate;
		}
	}
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_stm32_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;
	const uint32_t parity = uart_stm32_cfg2ll_parity(cfg->parity);
	const uint32_t stopbits = uart_stm32_cfg2ll_stopbits(config, cfg->stop_bits);
	const uint32_t databits = uart_stm32_cfg2ll_databits(cfg->data_bits,
							     cfg->parity);

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

	/* When the transformed ll stop bits don't match with what was requested, then it's not
	 * supported
	 */
	if (uart_stm32_ll2cfg_stopbits(stopbits) != cfg->stop_bits) {
		return -ENOTSUP;
	}

	/* When the transformed ll databits don't match with what was requested, then it's not
	 * supported
	 */
	if (uart_stm32_ll2cfg_databits(databits, parity) != cfg->data_bits) {
		return -ENOTSUP;
	}

	/* Driver supports only RTS/CTS and RS485 flow control */
	if (!(cfg->flow_ctrl == UART_CFG_FLOW_CTRL_NONE
		|| (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS &&
			IS_UART_HWFLOW_INSTANCE(config->usart))
#if HAS_DRIVER_ENABLE
		|| (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485 &&
			IS_UART_DRIVER_ENABLE_INSTANCE(config->usart))
#endif
		)) {
		return -ENOTSUP;
	}

	LL_USART_Disable(config->usart);

	/* Set basic parmeters, such as data-/stop-bit, parity, and baudrate */
	uart_stm32_parameters_set(dev, cfg);

	LL_USART_Enable(config->usart);

	/* Upon successful configuration, persist the syscall-passed
	 * uart_config.
	 * This allows restoring it, should the device return from a low-power
	 * mode in which register contents are lost.
	 */
	*uart_cfg = *cfg;

	return 0;
};

static int uart_stm32_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct uart_stm32_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;

	cfg->baudrate = uart_cfg->baudrate;
	cfg->parity = uart_stm32_ll2cfg_parity(uart_stm32_get_parity(dev));
	cfg->stop_bits = uart_stm32_ll2cfg_stopbits(
		uart_stm32_get_stopbits(dev));
	cfg->data_bits = uart_stm32_ll2cfg_databits(
		uart_stm32_get_databits(dev), uart_stm32_get_parity(dev));
	cfg->flow_ctrl = uart_stm32_ll2cfg_hwctrl(
		uart_stm32_get_hwctrl(dev));
#if HAS_DRIVER_ENABLE
	if (uart_stm32_get_driver_enable(dev)) {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_RS485;
	}
#endif
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

typedef void (*poll_in_fn)(
	const struct uart_stm32_config *config,
	void *in);

static int uart_stm32_poll_in_visitor(const struct device *dev, void *in, poll_in_fn get_fn)
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

	get_fn(config, in);

	return 0;
}

typedef void (*poll_out_fn)(
	const struct uart_stm32_config *config, void *out);

static void uart_stm32_poll_out_visitor(const struct device *dev, void *out, poll_out_fn set_fn)
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

	set_fn(config, out);
	irq_unlock(key);
}

static void poll_in_u8(const struct uart_stm32_config *config, void *in)
{
	*((unsigned char *)in) = (unsigned char)LL_USART_ReceiveData8(config->usart);
}

static void poll_out_u8(const struct uart_stm32_config *config, void *out)
{
	LL_USART_TransmitData8(config->usart, *((uint8_t *)out));
}

static int uart_stm32_poll_in(const struct device *dev, unsigned char *c)
{
	return uart_stm32_poll_in_visitor(dev, (void *)c, poll_in_u8);
}

static void uart_stm32_poll_out(const struct device *dev, unsigned char c)
{
	uart_stm32_poll_out_visitor(dev, (void *)&c, poll_out_u8);
}

#ifdef CONFIG_UART_WIDE_DATA

static void poll_out_u9(const struct uart_stm32_config *config, void *out)
{
	LL_USART_TransmitData9(config->usart, *((uint16_t *)out));
}

static void poll_in_u9(const struct uart_stm32_config *config, void *in)
{
	*((uint16_t *)in) = LL_USART_ReceiveData9(config->usart);
}

static int uart_stm32_poll_in_u16(const struct device *dev, uint16_t *in_u16)
{
	return uart_stm32_poll_in_visitor(dev, (void *)in_u16, poll_in_u9);
}

static void uart_stm32_poll_out_u16(const struct device *dev, uint16_t out_u16)
{
	uart_stm32_poll_out_visitor(dev, (void *)&out_u16, poll_out_u9);
}

#endif

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

	if (LL_USART_IsActiveFlag_NE(config->usart)) {
		err |= UART_ERROR_NOISE;
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

	if (err & UART_ERROR_NOISE) {
		LL_USART_ClearFlag_NE(config->usart);
	}

	return err;
}

static inline void __uart_stm32_get_clock(const struct device *dev)
{
	struct uart_stm32_data *data = dev->data;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	data->clock = clk;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

typedef void (*fifo_fill_fn)(const struct uart_stm32_config *config, const void *tx_data,
			     const uint8_t offset);

static int uart_stm32_fifo_fill_visitor(const struct device *dev, const void *tx_data, int size,
					fifo_fill_fn fill_fn)
{
	const struct uart_stm32_config *config = dev->config;
	uint8_t num_tx = 0U;
	unsigned int key;

	if (!LL_USART_IsActiveFlag_TXE(config->usart)) {
		return num_tx;
	}

	/* Lock interrupts to prevent nested interrupts or thread switch */
	key = irq_lock();

	while ((size - num_tx > 0) && LL_USART_IsActiveFlag_TXE(config->usart)) {
		/* TXE flag will be cleared with byte write to DR|RDR register */

		/* Send a character */
		fill_fn(config, tx_data, num_tx);
		num_tx++;
	}

	irq_unlock(key);

	return num_tx;
}

static void fifo_fill_with_u8(const struct uart_stm32_config *config,
					 const void *tx_data, const uint8_t offset)
{
	const uint8_t *data = (const uint8_t *)tx_data;
	/* Send a character (8bit) */
	LL_USART_TransmitData8(config->usart, data[offset]);
}

static int uart_stm32_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	if (uart_stm32_ll2cfg_databits(uart_stm32_get_databits(dev), uart_stm32_get_parity(dev)) ==
	    UART_CFG_DATA_BITS_9) {
		return -ENOTSUP;
	}
	return uart_stm32_fifo_fill_visitor(dev, (const void *)tx_data, size,
					    fifo_fill_with_u8);
}

typedef void (*fifo_read_fn)(const struct uart_stm32_config *config, void *rx_data,
			     const uint8_t offset);

static int uart_stm32_fifo_read_visitor(const struct device *dev, void *rx_data, const int size,
					fifo_read_fn read_fn)
{
	const struct uart_stm32_config *config = dev->config;
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) && LL_USART_IsActiveFlag_RXNE(config->usart)) {
		/* RXNE flag will be cleared upon read from DR|RDR register */

		read_fn(config, rx_data, num_rx);
		num_rx++;

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

static void fifo_read_with_u8(const struct uart_stm32_config *config, void *rx_data,
					 const uint8_t offset)
{
	uint8_t *data = (uint8_t *)rx_data;

	data[offset] = LL_USART_ReceiveData8(config->usart);
}

static int uart_stm32_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	if (uart_stm32_ll2cfg_databits(uart_stm32_get_databits(dev), uart_stm32_get_parity(dev)) ==
	    UART_CFG_DATA_BITS_9) {
		return -ENOTSUP;
	}
	return uart_stm32_fifo_read_visitor(dev, (void *)rx_data, size,
					    fifo_read_with_u8);
}

#ifdef CONFIG_UART_WIDE_DATA

static void fifo_fill_with_u16(const struct uart_stm32_config *config,
					  const void *tx_data, const uint8_t offset)
{
	const uint16_t *data = (const uint16_t *)tx_data;

	/* Send a character (9bit) */
	LL_USART_TransmitData9(config->usart, data[offset]);
}

static int uart_stm32_fifo_fill_u16(const struct device *dev, const uint16_t *tx_data, int size)
{
	if (uart_stm32_ll2cfg_databits(uart_stm32_get_databits(dev), uart_stm32_get_parity(dev)) !=
	    UART_CFG_DATA_BITS_9) {
		return -ENOTSUP;
	}
	return uart_stm32_fifo_fill_visitor(dev, (const void *)tx_data, size,
					    fifo_fill_with_u16);
}

static void fifo_read_with_u16(const struct uart_stm32_config *config, void *rx_data,
					  const uint8_t offset)
{
	uint16_t *data = (uint16_t *)rx_data;

	data[offset] = LL_USART_ReceiveData9(config->usart);
}

static int uart_stm32_fifo_read_u16(const struct device *dev, uint16_t *rx_data, const int size)
{
	if (uart_stm32_ll2cfg_databits(uart_stm32_get_databits(dev), uart_stm32_get_parity(dev)) !=
	    UART_CFG_DATA_BITS_9) {
		return -ENOTSUP;
	}
	return uart_stm32_fifo_read_visitor(dev, (void *)rx_data, size,
					    fifo_read_with_u16);
}

#endif

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

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->async_cb = NULL;
	data->async_user_data = NULL;
#endif
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

		if (data->dma_rx.timeout == 0) {
			uart_stm32_dma_rx_flush(dev);
		} else {
			/* Start the RX timer not null */
			async_timer_start(&data->dma_rx.timeout_work,
								data->dma_rx.timeout);
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

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->user_cb = NULL;
	data->user_data = NULL;
#endif

	return 0;
}

static inline void uart_stm32_dma_tx_enable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

	LL_USART_EnableDMAReq_TX(config->usart);
}

static inline void uart_stm32_dma_tx_disable(const struct device *dev)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_dma)
	ARG_UNUSED(dev);

	/*
	 * Errata Sheet ES0499 : STM32U575xx and STM32U585xx device errata
	 * USART does not generate DMA requests after setting/clearing DMAT bit
	 * (also seen on stm32H5 serie)
	 */
#else
	const struct uart_stm32_config *config = dev->config;

	LL_USART_DisableDMAReq_TX(config->usart);
#endif /* ! st_stm32u5_dma */
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

	if (data->rx_next_buffer) {
		struct uart_event rx_next_buf_release_evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = data->rx_next_buffer,
		};
		async_user_callback(data, &rx_next_buf_release_evt);
	}

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

	if (status < 0) {
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

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_dma)
	dma_suspend(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
#endif /* st_stm32u5_dma */
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

#ifdef CONFIG_UART_WIDE_DATA

static int uart_stm32_async_tx_u16(const struct device *dev, const uint16_t *tx_data,
				   size_t buf_size, int32_t timeout)
{
	return uart_stm32_async_tx(dev, (const uint8_t *)tx_data, buf_size * 2, timeout);
}

static int uart_stm32_async_rx_enable_u16(const struct device *dev, uint16_t *buf, size_t len,
					  int32_t timeout)
{
	return uart_stm32_async_rx_enable(dev, (uint8_t *)buf, len * 2, timeout);
}

static int uart_stm32_async_rx_buf_rsp_u16(const struct device *dev, uint16_t *buf, size_t len)
{
	return uart_stm32_async_rx_buf_rsp(dev, (uint8_t *)buf, len * 2);
}

#endif

#endif /* CONFIG_UART_ASYNC_API */

static const struct uart_driver_api uart_stm32_driver_api = {
	.poll_in = uart_stm32_poll_in,
	.poll_out = uart_stm32_poll_out,
#ifdef CONFIG_UART_WIDE_DATA
	.poll_in_u16 = uart_stm32_poll_in_u16,
	.poll_out_u16 = uart_stm32_poll_out_u16,
#endif
	.err_check = uart_stm32_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_stm32_configure,
	.config_get = uart_stm32_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_stm32_fifo_fill,
	.fifo_read = uart_stm32_fifo_read,
#ifdef CONFIG_UART_WIDE_DATA
	.fifo_fill_u16 = uart_stm32_fifo_fill_u16,
	.fifo_read_u16 = uart_stm32_fifo_read_u16,
#endif
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
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_stm32_async_callback_set,
	.tx = uart_stm32_async_tx,
	.tx_abort = uart_stm32_async_tx_abort,
	.rx_enable = uart_stm32_async_rx_enable,
	.rx_disable = uart_stm32_async_rx_disable,
	.rx_buf_rsp = uart_stm32_async_rx_buf_rsp,
#ifdef CONFIG_UART_WIDE_DATA
	.tx_u16 = uart_stm32_async_tx_u16,
	.rx_enable_u16 = uart_stm32_async_rx_enable_u16,
	.rx_buf_rsp_u16 = uart_stm32_async_rx_buf_rsp_u16,
#endif
#endif /* CONFIG_UART_ASYNC_API */
};

static int uart_stm32_clocks_enable(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;
	int err;

	__uart_stm32_get_clock(dev);

	if (!device_is_ready(data->clock)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* enable clock */
	err = clock_control_on(data->clock, (clock_control_subsys_t)&config->pclken[0]);
	if (err != 0) {
		LOG_ERR("Could not enable (LP)UART clock");
		return err;
	}

	if (IS_ENABLED(STM32_UART_DOMAIN_CLOCK_SUPPORT) && (config->pclk_len > 1)) {
		err = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					      (clock_control_subsys_t) &config->pclken[1],
					      NULL);
		if (err != 0) {
			LOG_ERR("Could not select UART domain clock");
			return err;
		}
	}

	return 0;
}

static int uart_stm32_registers_configure(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;

	LL_USART_Disable(config->usart);

	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	/* Reset UART to default state using RCC */
	(void)reset_line_toggle_dt(&config->reset);

	/* TX/RX direction */
	LL_USART_SetTransferDirection(config->usart,
				      LL_USART_DIRECTION_TX_RX);

	/* Set basic parmeters, such as data-/stop-bit, parity, and baudrate */
	uart_stm32_parameters_set(dev, uart_cfg);

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

#if HAS_DRIVER_ENABLE
	if (config->de_enable) {
		if (!IS_UART_DRIVER_ENABLE_INSTANCE(config->usart)) {
			LOG_ERR("%s does not support driver enable", dev->name);
			return -EINVAL;
		}

		uart_stm32_set_driver_enable(dev, true);
		LL_USART_SetDEAssertionTime(config->usart, config->de_assert_time);
		LL_USART_SetDEDeassertionTime(config->usart, config->de_deassert_time);

		if (config->de_invert) {
			LL_USART_SetDESignalPolarity(config->usart, LL_USART_DE_POLARITY_LOW);
		}
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
static int uart_stm32_init(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;
	int err;

	err = uart_stm32_clocks_enable(dev);
	if (err < 0) {
		return err;
	}

	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	err = uart_stm32_registers_configure(dev);
	if (err < 0) {
		return err;
	}

#if defined(CONFIG_PM) || \
	defined(CONFIG_UART_INTERRUPT_DRIVEN) || \
	defined(CONFIG_UART_ASYNC_API)
	config->irq_config_func(dev);
#endif /* CONFIG_PM || CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */

#if defined(CONFIG_PM) && defined(IS_UART_WAKEUP_FROMSTOP_INSTANCE)
	if (config->wakeup_source) {
		/* Enable ability to wakeup device in Stop mode
		 * Effect depends on CONFIG_PM_DEVICE status:
		 * CONFIG_PM_DEVICE=n : Always active
		 * CONFIG_PM_DEVICE=y : Controlled by pm_device_wakeup_enable()
		 */
		LL_USART_EnableInStopMode(config->usart);
		if (config->wakeup_line != STM32_EXTI_LINE_NONE) {
			/* Prepare the WAKEUP with the expected EXTI line */
			LL_EXTI_EnableIT_0_31(BIT(config->wakeup_line));
		}
	}
#endif /* CONFIG_PM */

#ifdef CONFIG_UART_ASYNC_API
	return uart_stm32_async_init(dev);
#else
	return 0;
#endif
}

#ifdef CONFIG_PM_DEVICE
static void uart_stm32_suspend_setup(const struct device *dev)
{
	const struct uart_stm32_config *config = dev->config;

#ifdef USART_ISR_BUSY
	/* Make sure that no USART transfer is on-going */
	while (LL_USART_IsActiveFlag_BUSY(config->usart) == 1) {
	}
#endif
	while (LL_USART_IsActiveFlag_TC(config->usart) == 0) {
	}
#ifdef USART_ISR_REACK
	/* Make sure that USART is ready for reception */
	while (LL_USART_IsActiveFlag_REACK(config->usart) == 0) {
	}
#endif
	/* Clear OVERRUN flag */
	LL_USART_ClearFlag_ORE(config->usart);
}

static int uart_stm32_pm_action(const struct device *dev,
			       enum pm_device_action action)
{
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;
	int err;


	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Set pins to active state */
		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			return err;
		}

		/* enable clock */
		err = clock_control_on(data->clock, (clock_control_subsys_t)&config->pclken[0]);
		if (err != 0) {
			LOG_ERR("Could not enable (LP)UART clock");
			return err;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		uart_stm32_suspend_setup(dev);
		/* Stop device clock. Note: fixed clocks are not handled yet. */
		err = clock_control_off(data->clock, (clock_control_subsys_t)&config->pclken[0]);
		if (err != 0) {
			LOG_ERR("Could not enable (LP)UART clock");
			return err;
		}

		/* Move pins to sleep state */
		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if ((err < 0) && (err != -ENOENT)) {
			/*
			 * If returning -ENOENT, no pins where defined for sleep mode :
			 * Do not output on console (might sleep already) when going to sleep,
			 * "(LP)UART pinctrl sleep state not available"
			 * and don't block PM suspend.
			 * Else return the error.
			 */
			return err;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

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

#ifdef CONFIG_PM
#define STM32_UART_PM_WAKEUP(index)						\
	.wakeup_source = DT_INST_PROP(index, wakeup_source),			\
	.wakeup_line = COND_CODE_1(DT_INST_NODE_HAS_PROP(index, wakeup_line),	\
			(DT_INST_PROP(index, wakeup_line)),			\
			(STM32_EXTI_LINE_NONE)),
#else
#define STM32_UART_PM_WAKEUP(index) /* Not used */
#endif

/* Ensure DTS doesn't present an incompatible parity configuration.
 * Mark/space parity isn't supported on the STM32 family.
 * If 9 data bits are configured, ensure that a parity bit isn't set.
 */
#define STM32_UART_CHECK_DT_PARITY(index)				\
BUILD_ASSERT(								\
	!(DT_INST_ENUM_IDX_OR(index, parity, STM32_UART_DEFAULT_PARITY)	\
					== UART_CFG_PARITY_MARK ||	\
	DT_INST_ENUM_IDX_OR(index, parity, STM32_UART_DEFAULT_PARITY)	\
					== UART_CFG_PARITY_SPACE),	\
	"Node " DT_NODE_PATH(DT_DRV_INST(index))			\
	" has unsupported parity configuration");			\
BUILD_ASSERT(								\
	!(DT_INST_ENUM_IDX_OR(index, parity, STM32_UART_DEFAULT_PARITY)	\
					!= UART_CFG_PARITY_NONE	&&	\
	DT_INST_ENUM_IDX_OR(index, data_bits,				\
			    STM32_UART_DEFAULT_DATA_BITS)		\
					== UART_CFG_DATA_BITS_9),	\
	"Node " DT_NODE_PATH(DT_DRV_INST(index))			\
		" has unsupported parity + data bits combination");

/* Ensure DTS doesn't present an incompatible data bits configuration
 * The STM32 family doesn't support 5 data bits, or 6 data bits without parity.
 * Only some series support 7 data bits.
 */
#ifdef LL_USART_DATAWIDTH_7B
#define STM32_UART_CHECK_DT_DATA_BITS(index)				\
BUILD_ASSERT(								\
	!(DT_INST_ENUM_IDX_OR(index, data_bits,				\
			      STM32_UART_DEFAULT_DATA_BITS)		\
					== UART_CFG_DATA_BITS_5 ||	\
		(DT_INST_ENUM_IDX_OR(index, data_bits,			\
				    STM32_UART_DEFAULT_DATA_BITS)	\
					== UART_CFG_DATA_BITS_6 &&	\
		DT_INST_ENUM_IDX_OR(index, parity,			\
				    STM32_UART_DEFAULT_PARITY)		\
					== UART_CFG_PARITY_NONE)),	\
	"Node " DT_NODE_PATH(DT_DRV_INST(index))			\
		" has unsupported data bits configuration");
#else
#define STM32_UART_CHECK_DT_DATA_BITS(index)				\
BUILD_ASSERT(								\
	!(DT_INST_ENUM_IDX_OR(index, data_bits,				\
			      STM32_UART_DEFAULT_DATA_BITS)		\
					== UART_CFG_DATA_BITS_5 ||	\
	DT_INST_ENUM_IDX_OR(index, data_bits,				\
			    STM32_UART_DEFAULT_DATA_BITS)		\
					== UART_CFG_DATA_BITS_6 ||	\
		(DT_INST_ENUM_IDX_OR(index, data_bits,			\
			    STM32_UART_DEFAULT_DATA_BITS)		\
					== UART_CFG_DATA_BITS_7 &&	\
		DT_INST_ENUM_IDX_OR(index, parity,			\
				    STM32_UART_DEFAULT_PARITY)		\
					== UART_CFG_PARITY_NONE)),	\
	"Node " DT_NODE_PATH(DT_DRV_INST(index))			\
		" has unsupported data bits configuration");
#endif

/* Ensure DTS doesn't present an incompatible stop bits configuration.
 * Some STM32 series USARTs don't support 0.5 stop bits, and it generally isn't
 * supported for LPUART.
 */
#ifndef LL_USART_STOPBITS_0_5
#define STM32_UART_CHECK_DT_STOP_BITS_0_5(index)			\
BUILD_ASSERT(								\
	!(DT_INST_ENUM_IDX_OR(index, stop_bits,				\
			      STM32_UART_DEFAULT_STOP_BITS)		\
					== UART_CFG_STOP_BITS_0_5),	\
	"Node " DT_NODE_PATH(DT_DRV_INST(index))			\
		" has unsupported stop bits configuration");
/* LPUARTs don't support 0.5 stop bits configurations */
#else
#define STM32_UART_CHECK_DT_STOP_BITS_0_5(index)			\
BUILD_ASSERT(								\
	!(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_lpuart) &&			\
	  DT_INST_ENUM_IDX_OR(index, stop_bits,				\
			      STM32_UART_DEFAULT_STOP_BITS)		\
					== UART_CFG_STOP_BITS_0_5),	\
	"Node " DT_NODE_PATH(DT_DRV_INST(index))			\
		" has unsupported stop bits configuration");
#endif

/* Ensure DTS doesn't present an incompatible stop bits configuration.
 * Some STM32 series USARTs don't support 1.5 stop bits, and it generally isn't
 * supported for LPUART.
 */
#ifndef LL_USART_STOPBITS_1_5
#define STM32_UART_CHECK_DT_STOP_BITS_1_5(index)			\
BUILD_ASSERT(								\
	DT_INST_ENUM_IDX_OR(index, stop_bits,				\
			    STM32_UART_DEFAULT_STOP_BITS)		\
					!= UART_CFG_STOP_BITS_1_5,	\
	"Node " DT_NODE_PATH(DT_DRV_INST(index))			\
		" has unsupported stop bits configuration");
/* LPUARTs don't support 1.5 stop bits configurations */
#else
#define STM32_UART_CHECK_DT_STOP_BITS_1_5(index)			\
BUILD_ASSERT(								\
	!(DT_HAS_COMPAT_STATUS_OKAY(st_stm32_lpuart) &&			\
	  DT_INST_ENUM_IDX_OR(index, stop_bits,				\
			      STM32_UART_DEFAULT_STOP_BITS)		\
					== UART_CFG_STOP_BITS_1_5),	\
	"Node " DT_NODE_PATH(DT_DRV_INST(index))			\
		" has unsupported stop bits configuration");
#endif

#define STM32_UART_INIT(index)						\
STM32_UART_IRQ_HANDLER_DECL(index)					\
									\
PINCTRL_DT_INST_DEFINE(index);						\
									\
static const struct stm32_pclken pclken_##index[] =			\
					    STM32_DT_INST_CLOCKS(index);\
									\
static struct uart_config uart_cfg_##index = {				\
	.baudrate  = DT_INST_PROP_OR(index, current_speed,		\
				     STM32_UART_DEFAULT_BAUDRATE),	\
	.parity    = DT_INST_ENUM_IDX_OR(index, parity,			\
					 STM32_UART_DEFAULT_PARITY),	\
	.stop_bits = DT_INST_ENUM_IDX_OR(index, stop_bits,		\
					 STM32_UART_DEFAULT_STOP_BITS),	\
	.data_bits = DT_INST_ENUM_IDX_OR(index, data_bits,		\
					 STM32_UART_DEFAULT_DATA_BITS),	\
	.flow_ctrl = DT_INST_PROP(index, hw_flow_control)		\
					? UART_CFG_FLOW_CTRL_RTS_CTS	\
					: UART_CFG_FLOW_CTRL_NONE,	\
};									\
									\
static const struct uart_stm32_config uart_stm32_cfg_##index = {	\
	.usart = (USART_TypeDef *)DT_INST_REG_ADDR(index),		\
	.reset = RESET_DT_SPEC_GET(DT_DRV_INST(index)),			\
	.pclken = pclken_##index,					\
	.pclk_len = DT_INST_NUM_CLOCKS(index),				\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
	.single_wire = DT_INST_PROP_OR(index, single_wire, false),	\
	.tx_rx_swap = DT_INST_PROP_OR(index, tx_rx_swap, false),	\
	.rx_invert = DT_INST_PROP(index, rx_invert),			\
	.tx_invert = DT_INST_PROP(index, tx_invert),			\
	.de_enable = DT_INST_PROP(index, de_enable),			\
	.de_assert_time = DT_INST_PROP(index, de_assert_time),		\
	.de_deassert_time = DT_INST_PROP(index, de_deassert_time),	\
	.de_invert = DT_INST_PROP(index, de_invert),			\
	STM32_UART_IRQ_HANDLER_FUNC(index)				\
	STM32_UART_PM_WAKEUP(index)					\
};									\
									\
static struct uart_stm32_data uart_stm32_data_##index = {		\
	.uart_cfg = &uart_cfg_##index,					\
	UART_DMA_CHANNEL(index, rx, RX, PERIPHERAL, MEMORY)		\
	UART_DMA_CHANNEL(index, tx, TX, MEMORY, PERIPHERAL)		\
};									\
									\
PM_DEVICE_DT_INST_DEFINE(index, uart_stm32_pm_action);		        \
									\
DEVICE_DT_INST_DEFINE(index,						\
		    &uart_stm32_init,					\
		    PM_DEVICE_DT_INST_GET(index),			\
		    &uart_stm32_data_##index, &uart_stm32_cfg_##index,	\
		    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,		\
		    &uart_stm32_driver_api);				\
									\
STM32_UART_IRQ_HANDLER(index)						\
									\
STM32_UART_CHECK_DT_PARITY(index)					\
STM32_UART_CHECK_DT_DATA_BITS(index)					\
STM32_UART_CHECK_DT_STOP_BITS_0_5(index)				\
STM32_UART_CHECK_DT_STOP_BITS_1_5(index)

DT_INST_FOREACH_STATUS_OKAY(STM32_UART_INIT)
