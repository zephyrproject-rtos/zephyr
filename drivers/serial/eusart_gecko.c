/*
 * Copyright (c) 2024, Yishai Jaffe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_eusart

#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <em_eusart.h>
#include <em_cmu.h>
#include <soc.h>

#define GECKO_UART_DEFAULT_BAUDRATE	115200
#define GECKO_UART_DEFAULT_PARITY	UART_CFG_PARITY_NONE
#define GECKO_UART_DEFAULT_STOP_BITS	UART_CFG_STOP_BITS_1
#define GECKO_UART_DEFAULT_DATA_BITS	UART_CFG_DATA_BITS_8

#define GET_GECKO_EUSART_CLOCK(idx)                            \
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)), \
	.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),

#define HAS_HFC_OR(inst) DT_INST_PROP(inst, hw_flow_control) ||

/* Has any enabled uart instance hw-flow-control enabled? */
#define EUSART_GECKO_HW_FLOW_CONTROL				\
	DT_INST_FOREACH_STATUS_OKAY(HAS_HFC_OR) 0

#if defined(EUSART_GECKO_HW_FLOW_CONTROL) && !defined(GPIO_EUSART_ROUTEEN_RTSPEN)
#error "Driver not supporting hardware flow control for this SOC"
#endif

/**
 * @brief Config struct for UART
 */
struct eusart_gecko_config {
	EUSART_TypeDef *eusart;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

struct eusart_gecko_data {
	struct uart_config *uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int eusart_gecko_poll_in(const struct device *dev, unsigned char *c)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	uint32_t flags = EUSART_StatusGet(base);

	if (flags & USART_STATUS_RXDATAV) { /* FIXME: no EUSART_STATUS_RXDATAV */
		*c = EUSART_Rx(base);
		return 0;
	}

	return -1;
}

static void eusart_gecko_poll_out(const struct device *dev, unsigned char c)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;

	/* EUSART_Tx function already waits for the transmit buffer being empty
	 * and waits for the bus to be free to transmit.
	 */
	EUSART_Tx(base, c);
}

static int eusart_gecko_err_check(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	uint32_t flags = EUSART_IntGet(base);
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

	EUSART_IntClear(base, EUSART_IF_RXOF |
		       EUSART_IF_PERR |
		       EUSART_IF_FERR);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int eusart_gecko_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int len)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	int num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (base->STATUS & USART_STATUS_TXBL)) { /* FIXME: no EUSART_STATUS_TXBL */

		base->TXDATA = (uint32_t)tx_data[num_tx++];
	}

	return num_tx;
}

static int eusart_gecko_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int len)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	int num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (base->STATUS & USART_STATUS_RXDATAV)) { /* FIXME: no EUSART_STATUS_RXDATAV */

		rx_data[num_rx++] = (uint8_t)base->RXDATA;
	}

	return num_rx;
}

static void eusart_gecko_irq_tx_enable(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	uint32_t mask = USART_IEN_TXBL | EUSART_IEN_TXC; /* FIXME: no EUSART_STATUS_RXDATAV */

	EUSART_IntEnable(base, mask);
}

static void eusart_gecko_irq_tx_disable(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	uint32_t mask = USART_IEN_TXBL | EUSART_IEN_TXC; /* FIXME: no EUSART_STATUS_RXDATAV */

	EUSART_IntDisable(base, mask);
}

static int eusart_gecko_irq_tx_complete(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	uint32_t flags = EUSART_IntGet(base);

	EUSART_IntClear(base, EUSART_IF_TXC); /* FIXME: Necessary? compare usart/leuart */

	return (flags & EUSART_IF_TXC) != 0U;
}

static int eusart_gecko_irq_tx_ready(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	uint32_t flags = EUSART_IntGet(base); /* FIXME: EUSART_IntGetEnabled or EUSART_IntGet*/

	return (flags & USART_IF_TXBL) != 0U; /* FIXME: no EUSART_IF_TXBL*/
}

static void eusart_gecko_irq_rx_enable(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	uint32_t mask = USART_IEN_RXDATAV; /* FIXME: no EUSART_IEN_RXDATAV */

	EUSART_IntEnable(base, mask);
}

static void eusart_gecko_irq_rx_disable(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	uint32_t mask = USART_IEN_RXDATAV; /* FIXME: no EUSART_IEN_RXDATAV */

	EUSART_IntDisable(base, mask);
}

static int eusart_gecko_irq_rx_full(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	uint32_t flags = EUSART_IntGet(base);

	return (flags & USART_IF_RXDATAV) != 0U; /* FIXME: no EUSART_IF_RXDATAV */
}

static int eusart_gecko_irq_rx_ready(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;
	uint32_t mask = USART_IEN_RXDATAV; /* FIXME: no EUSART_IEN_RXDATAV */

	return (base->IEN & mask)
		&& eusart_gecko_irq_rx_full(dev);
}

static void eusart_gecko_irq_err_enable(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;

	EUSART_IntEnable(base, EUSART_IF_RXOF |
			 EUSART_IF_PERR |
			 EUSART_IF_FERR);
}

static void eusart_gecko_irq_err_disable(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = config->eusart;

	EUSART_IntDisable(base, EUSART_IF_RXOF |
			 EUSART_IF_PERR |
			 EUSART_IF_FERR);
}

static int eusart_gecko_irq_is_pending(const struct device *dev)
{
	return eusart_gecko_irq_tx_ready(dev) || eusart_gecko_irq_rx_ready(dev);
}

static int eusart_gecko_irq_update(const struct device *dev)
{
	return 1;
}

static void eusart_gecko_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb,
					  void *cb_data)
{
	struct eusart_gecko_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void eusart_gecko_isr(const struct device *dev)
{
	struct eusart_gecko_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static inline EUSART_Parity_TypeDef eusart_gecko_cfg2ll_parity(enum uart_config_parity parity)
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

static inline enum uart_config_parity eusart_gecko_ll2cfg_parity(EUSART_Parity_TypeDef parity)
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

static inline EUSART_Stopbits_TypeDef eusart_gecko_cfg2ll_stopbits(enum uart_config_stop_bits sb)
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

static inline enum uart_config_stop_bits eusart_gecko_ll2cfg_stopbits(EUSART_Stopbits_TypeDef sb)
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

static inline EUSART_Databits_TypeDef eusart_gecko_cfg2ll_databits(enum uart_config_data_bits db,
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

static inline enum uart_config_data_bits eusart_gecko_ll2cfg_databits(EUSART_Databits_TypeDef db,
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

/**
 * @brief  Get LL hardware flow control define from
 *         Zephyr hardware flow control option.
 * @note   Supports only UART_CFG_FLOW_CTRL_RTS_CTS and UART_CFG_FLOW_CTRL_RS485.
 * @param  fc: Zephyr hardware flow control option.
 * @retval eusartHwFlowControlCtsAndRts, or eusartHwFlowControlNone.
 */
static inline EUSART_HwFlowControl_TypeDef eusart_gecko_cfg2ll_hwctrl(
	enum uart_config_flow_control fc)
{
	if (fc == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return eusartHwFlowControlCtsAndRts;
	}

	return eusartHwFlowControlNone;
}

/**
 * @brief  Get Zephyr hardware flow control option from
 *         LL hardware flow control define.
 * @note   Supports only eusartHwFlowControlCtsAndRts.
 * @param  fc: LL hardware flow control definition.
 * @retval UART_CFG_FLOW_CTRL_RTS_CTS, or UART_CFG_FLOW_CTRL_NONE.
 */
static inline enum uart_config_flow_control eusart_gecko_ll2cfg_hwctrl(
	EUSART_HwFlowControl_TypeDef fc)
{
	if (fc == eusartHwFlowControlCtsAndRts) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_FLOW_CTRL_NONE;
}

/**
 * @brief Main initializer for UART
 *
 * @param dev UART device to be initialized
 * @return int 0
 */
static int eusart_gecko_init(const struct device *dev)
{
	int err;
	const struct eusart_gecko_config *config = dev->config;
	struct eusart_gecko_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;
	EUSART_TypeDef *base = config->eusart;

	EUSART_UartInit_TypeDef eusartInit = EUSART_UART_INIT_DEFAULT_HF;
	EUSART_AdvancedInit_TypeDef advancedSettings = EUSART_ADVANCED_INIT_DEFAULT;

	/* The peripheral and gpio clock are already enabled from soc and gpio
	 * driver
	 */
	/* Enable EUSART clock */
	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0) {
		return err;
	}

	/* Init EUSART */
	eusartInit.baudrate = uart_cfg->baudrate;
	eusartInit.parity = eusart_gecko_cfg2ll_parity(uart_cfg->parity);
	eusartInit.stopbits = eusart_gecko_cfg2ll_stopbits(uart_cfg->stop_bits);
	eusartInit.databits = eusart_gecko_cfg2ll_databits(uart_cfg->data_bits, uart_cfg->parity);
	advancedSettings.hwFlowControl = eusart_gecko_cfg2ll_hwctrl(uart_cfg->flow_ctrl);
	eusartInit.advancedSettings = &advancedSettings;

	EUSART_UartInitHf(base, &eusartInit);

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int eusart_gecko_pm_action(const struct device *dev, enum pm_device_action action)
{
	__maybe_unused const struct eusart_gecko_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
#ifdef EUSART_STATUS_TXIDLE
		/* Wait for TX FIFO to flush before suspending */
		while (!(EUSART_StatusGet(config->eusart) & EUSART_STATUS_TXIDLE)) {
		}
#endif
		break;

	case PM_DEVICE_ACTION_RESUME:
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static DEVICE_API(uart, eusart_gecko_driver_api) = {
	.poll_in = eusart_gecko_poll_in,
	.poll_out = eusart_gecko_poll_out,
	.err_check = eusart_gecko_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = eusart_gecko_fifo_fill,
	.fifo_read = eusart_gecko_fifo_read,
	.irq_tx_enable = eusart_gecko_irq_tx_enable,
	.irq_tx_disable = eusart_gecko_irq_tx_disable,
	.irq_tx_complete = eusart_gecko_irq_tx_complete,
	.irq_tx_ready = eusart_gecko_irq_tx_ready,
	.irq_rx_enable = eusart_gecko_irq_rx_enable,
	.irq_rx_disable = eusart_gecko_irq_rx_disable,
	.irq_rx_ready = eusart_gecko_irq_rx_ready,
	.irq_err_enable = eusart_gecko_irq_err_enable,
	.irq_err_disable = eusart_gecko_irq_err_disable,
	.irq_is_pending = eusart_gecko_irq_is_pending,
	.irq_update = eusart_gecko_irq_update,
	.irq_callback_set = eusart_gecko_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define GECKO_EUSART_IRQ_HANDLER_DECL(idx)				       \
	static void eusart_gecko_config_func_##idx(const struct device *dev)
#define GECKO_EUSART_IRQ_HANDLER_FUNC(idx)				       \
	.irq_config_func = eusart_gecko_config_func_##idx,
#define GECKO_EUSART_IRQ_HANDLER(idx)					       \
	static void eusart_gecko_config_func_##idx(const struct device *dev)   \
	{								       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, rx, irq),		       \
			    DT_INST_IRQ_BY_NAME(idx, rx, priority),	       \
			    eusart_gecko_isr, DEVICE_DT_INST_GET(idx), 0);     \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, tx, irq),		       \
			    DT_INST_IRQ_BY_NAME(idx, tx, priority),	       \
			    eusart_gecko_isr, DEVICE_DT_INST_GET(idx), 0);     \
									       \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, rx, irq));		       \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, tx, irq));		       \
	}
#else
#define GECKO_EUSART_IRQ_HANDLER_DECL(idx)
#define GECKO_EUSART_IRQ_HANDLER_FUNC(idx)
#define GECKO_EUSART_IRQ_HANDLER(idx)
#endif

#define GECKO_EUSART_INIT(idx)							\
GECKO_EUSART_IRQ_HANDLER_DECL(idx);						\
										\
PINCTRL_DT_INST_DEFINE(idx);							\
										\
static struct uart_config uart_cfg_##idx = {					\
	.baudrate  = DT_INST_PROP_OR(idx, current_speed,			\
				     GECKO_UART_DEFAULT_BAUDRATE),		\
	.parity    = DT_INST_ENUM_IDX_OR(idx, parity,				\
					 GECKO_UART_DEFAULT_PARITY),		\
	.stop_bits = DT_INST_ENUM_IDX_OR(idx, stop_bits,			\
					 GECKO_UART_DEFAULT_STOP_BITS),		\
	.data_bits = DT_INST_ENUM_IDX_OR(idx, data_bits,			\
					 GECKO_UART_DEFAULT_DATA_BITS),		\
	.flow_ctrl = DT_INST_PROP(idx, hw_flow_control)				\
					? UART_CFG_FLOW_CTRL_RTS_CTS		\
					: UART_CFG_FLOW_CTRL_NONE,		\
};										\
										\
static const struct eusart_gecko_config eusart_gecko_cfg_##idx = {		\
	.eusart = (EUSART_TypeDef *)DT_INST_REG_ADDR(idx),			\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),				\
	GET_GECKO_EUSART_CLOCK(idx)						\
	GECKO_EUSART_IRQ_HANDLER_FUNC(idx)					\
	};									\
										\
static struct eusart_gecko_data eusart_gecko_data_##idx = {			\
	.uart_cfg = &uart_cfg_##idx,						\
};										\
										\
PM_DEVICE_DT_INST_DEFINE(idx, eusart_gecko_pm_action);				\
										\
DEVICE_DT_INST_DEFINE(idx, eusart_gecko_init, PM_DEVICE_DT_INST_GET(idx),	\
		      &eusart_gecko_data_##idx,					\
		      &eusart_gecko_cfg_##idx, PRE_KERNEL_1,			\
		      CONFIG_SERIAL_INIT_PRIORITY,				\
		      &eusart_gecko_driver_api);				\
										\
GECKO_EUSART_IRQ_HANDLER(idx)

DT_INST_FOREACH_STATUS_OKAY(GECKO_EUSART_INIT)
