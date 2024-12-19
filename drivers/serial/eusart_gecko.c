/*
 * Copyright (c) 2024, Yishai Jaffe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_eusart

#include <errno.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <em_eusart.h>
#include <em_cmu.h>
#include <soc.h>

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#else
#include <em_gpio.h>
#endif /* CONFIG_PINCTRL */

#ifdef CONFIG_CLOCK_CONTROL
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#define GET_GECKO_EUSART_CLOCK(idx)                            \
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)), \
	.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),
#elif DT_NODE_HAS_PROP(id, peripheral_id)
#define EUSART_PREFIX cmuClock_EUSART
#define CLOCK_EUSART(id) _CONCAT(EUSART_PREFIX, id)
#define GET_GECKO_EUSART_CLOCK(id) \
	.clock = CLOCK_EUSART(DT_INST_PROP(id, peripheral_id)),
#else
#if (EUSART_COUNT == 1)
#define CLOCK_EUSART(ref)	(((ref) == EUSART0) ? cmuClock_EUSART0 \
			       : -1)
#elif (EUSART_COUNT == 2)
#define CLOCK_EUSART(ref)	(((ref) == EUSART0) ? cmuClock_EUSART0 \
			       : ((ref) == EUSART1) ? cmuClock_EUSART1 \
			       : -1)
#elif (EUSART_COUNT == 3)
#define CLOCK_EUSART(ref)	(((ref) == EUSART0) ? cmuClock_EUSART0 \
			       : ((ref) == EUSART1) ? cmuClock_EUSART1 \
			       : ((ref) == EUSART2) ? cmuClock_EUSART2 \
			       : -1)
#else
#error "Undefined number of RUSARTs."
#endif /* EUSART_COUNT */

#define GET_GECKO_EUSART_CLOCK(id) \
	.clock = CLOCK_EUSART((EUSART_TypeDef *)DT_INST_REG_ADDR(id)),
#endif /* DT_NODE_HAS_PROP(id, peripheral_id) */

/* Helper define to determine if SOC supports hardware flow control */
#if ((_SILICON_LABS_32B_SERIES > 0) ||					\
	(defined(_USART_ROUTEPEN_RTSPEN_MASK) &&			\
	defined(_USART_ROUTEPEN_CTSPEN_MASK)))
#define HW_FLOWCONTROL_IS_SUPPORTED_BY_SOC
#endif

#define HAS_HFC_OR(inst) DT_INST_PROP(inst, hw_flow_control) ||

/* Has any enabled uart instance hw-flow-control enabled? */
#define EUSART_GECKO_HW_FLOW_CONTROL				\
	DT_INST_FOREACH_STATUS_OKAY(HAS_HFC_OR) 0

/* Sanity check for hardware flow control */
#if defined(EUSART_GECKO_HW_FLOW_CONTROL) &&				\
	(!(defined(HW_FLOWCONTROL_IS_SUPPORTED_BY_SOC)))
#error "Hardware flow control is activated for at least one EUSART, but	\
not supported by this SOC"
#endif

#if defined(EUSART_GECKO_HW_FLOW_CONTROL) &&				\
	(!defined(CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION) &&	\
	 !defined(GPIO_EUSART_ROUTEEN_RTSPEN))
#error "Driver not supporting hardware flow control for this SOC"
#endif

#define DEV_BASE(dev) \
	((EUSART_TypeDef *) \
	 ((const struct eusart_gecko_config * const)(dev)->config)->base)


/**
 * @brief Config struct for UART
 */

struct eusart_gecko_config {
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pcfg;
#endif /* CONFIG_PINCTRL */
	EUSART_TypeDef *base;
#ifdef CONFIG_CLOCK_CONTROL
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
#else
	CMU_Clock_TypeDef clock;
#endif
	uint32_t baud_rate;
/* #ifndef CONFIG_PINCTRL FIXME: ? */
#ifdef EUSART_GECKO_HW_FLOW_CONTROL
	bool hw_flowcontrol;
#endif /* EUSART_GECKO_HW_FLOW_CONTROL */
/* #endif */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifndef CONFIG_PINCTRL
	struct soc_gpio_pin pin_rx;
	struct soc_gpio_pin pin_tx;
#ifdef EUSART_GECKO_HW_FLOW_CONTROL
	struct soc_gpio_pin pin_rts;
	struct soc_gpio_pin pin_cts;
#endif /* EUSART_GECKO_HW_FLOW_CONTROL */
#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
	uint8_t loc_rx;
	uint8_t loc_tx;
#ifdef EUSART_GECKO_HW_FLOW_CONTROL
	uint8_t loc_rts;
	uint8_t loc_cts;
#endif /* EUSART_GECKO_HW_FLOW_CONTROL */
#else /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */
	uint8_t loc;
#endif /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */
#endif
};

struct eusart_gecko_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int eusart_gecko_poll_in(const struct device *dev, unsigned char *c)
{
	EUSART_TypeDef *base = DEV_BASE(dev);
	uint32_t flags = EUSART_StatusGet(base);

	if (flags & USART_STATUS_RXDATAV) { /* FIXME: no EUSART_STATUS_RXDATAV */
		*c = EUSART_Rx(base);
		return 0;
	}

	return -1;
}

static void eusart_gecko_poll_out(const struct device *dev, unsigned char c)
{
	EUSART_TypeDef *base = DEV_BASE(dev);

	/* EUSART_Tx function already waits for the transmit buffer being empty
	 * and waits for the bus to be free to transmit.
	 */
	EUSART_Tx(base, c);
}

static int eusart_gecko_err_check(const struct device *dev)
{
	EUSART_TypeDef *base = DEV_BASE(dev);
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
	EUSART_TypeDef *base = DEV_BASE(dev);
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
	EUSART_TypeDef *base = DEV_BASE(dev);
	int num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (base->STATUS & USART_STATUS_RXDATAV)) { /* FIXME: no EUSART_STATUS_RXDATAV */

		rx_data[num_rx++] = (uint8_t)base->RXDATA;
	}

	return num_rx;
}

static void eusart_gecko_irq_tx_enable(const struct device *dev)
{
	EUSART_TypeDef *base = DEV_BASE(dev);
	uint32_t mask = USART_IEN_TXBL | EUSART_IEN_TXC; /* FIXME: no EUSART_STATUS_RXDATAV */

	EUSART_IntEnable(base, mask);
}

static void eusart_gecko_irq_tx_disable(const struct device *dev)
{
	EUSART_TypeDef *base = DEV_BASE(dev);
	uint32_t mask = USART_IEN_TXBL | EUSART_IEN_TXC; /* FIXME: no EUSART_STATUS_RXDATAV */

	EUSART_IntDisable(base, mask);
}

static int eusart_gecko_irq_tx_complete(const struct device *dev)
{
	EUSART_TypeDef *base = DEV_BASE(dev);
	uint32_t flags = EUSART_IntGet(base);

	EUSART_IntClear(base, EUSART_IF_TXC); /* FIXME: Necessary? compare usart/leuart */

	return (flags & EUSART_IF_TXC) != 0U;
}

static int eusart_gecko_irq_tx_ready(const struct device *dev)
{
	EUSART_TypeDef *base = DEV_BASE(dev);
	uint32_t flags = EUSART_IntGet(base); /* FIXME: EUSART_IntGetEnabled or EUSART_IntGet*/

	return (flags & USART_IF_TXBL) != 0U; /* FIXME: no EUSART_IF_TXBL*/
}

static void eusart_gecko_irq_rx_enable(const struct device *dev)
{
	EUSART_TypeDef *base = DEV_BASE(dev);
	uint32_t mask = USART_IEN_RXDATAV; /* FIXME: no EUSART_IEN_RXDATAV */

	EUSART_IntEnable(base, mask);
}

static void eusart_gecko_irq_rx_disable(const struct device *dev)
{
	EUSART_TypeDef *base = DEV_BASE(dev);
	uint32_t mask = USART_IEN_RXDATAV; /* FIXME: no EUSART_IEN_RXDATAV */

	EUSART_IntDisable(base, mask);
}

static int eusart_gecko_irq_rx_full(const struct device *dev)
{
	EUSART_TypeDef *base = DEV_BASE(dev);
	uint32_t flags = EUSART_IntGet(base);

	return (flags & USART_IF_RXDATAV) != 0U; /* FIXME: no EUSART_IF_RXDATAV */
}

static int eusart_gecko_irq_rx_ready(const struct device *dev)
{
	EUSART_TypeDef *base = DEV_BASE(dev);
	uint32_t mask = USART_IEN_RXDATAV; /* FIXME: no EUSART_IEN_RXDATAV */

	return (base->IEN & mask)
		&& eusart_gecko_irq_rx_full(dev);
}

static void eusart_gecko_irq_err_enable(const struct device *dev)
{
	EUSART_TypeDef *base = DEV_BASE(dev);

	EUSART_IntEnable(base, EUSART_IF_RXOF |
			 EUSART_IF_PERR |
			 EUSART_IF_FERR);
}

static void eusart_gecko_irq_err_disable(const struct device *dev)
{
	EUSART_TypeDef *base = DEV_BASE(dev);

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

/**
 * @brief Subroutine initializer of UART pins
 *
 * @param dev UART device to configure
 */
 #ifndef CONFIG_PINCTRL
static void eusart_gecko_init_pins(const struct device *dev)
{
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = DEV_BASE(dev);

	/* Configure RX and TX */
	GPIO_PinModeSet(config->pin_rx.port, config->pin_rx.pin,
			 config->pin_rx.mode, config->pin_rx.out);
	GPIO_PinModeSet(config->pin_tx.port, config->pin_tx.pin,
			 config->pin_tx.mode, config->pin_tx.out);

#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
	/* For SOCs with configurable pin locations (set in SOC Kconfig) */
	base->ROUTEPEN = EUSART_ROUTEPEN_RXPEN | EUSART_ROUTEPEN_TXPEN;
	base->ROUTELOC0 =
		(config->loc_tx << _EUSART_ROUTELOC0_TXLOC_SHIFT) |
		(config->loc_rx << _EUSART_ROUTELOC0_RXLOC_SHIFT);
	base->ROUTELOC1 = _USART_ROUTELOC1_RESETVALUE;
#elif defined(EUSART_ROUTE_RXPEN) && defined(EUSART_ROUTE_TXPEN)
	/* For olders SOCs with only one pin location */
	base->ROUTE = EUSART_ROUTE_RXPEN | EUSART_ROUTE_TXPEN
		| (config->loc << 8);
#elif defined(GPIO_EUSART_ROUTEEN_RXPEN) && defined(GPIO_EUSART_ROUTEEN_TXPEN)
	GPIO->EUSARTROUTE[EUSART_NUM(base)].ROUTEEN =
		GPIO_EUSART_ROUTEEN_TXPEN | GPIO_EUSART_ROUTEEN_RXPEN;
	GPIO->EUSARTROUTE[EUSART_NUM(base)].TXROUTE =
		(config->pin_tx.pin << _GPIO_EUSART_TXROUTE_PIN_SHIFT) |
		(config->pin_tx.port << _GPIO_EUSART_TXROUTE_PORT_SHIFT);
	GPIO->EUSARTROUTE[EUSART_NUM(base)].RXROUTE =
		(config->pin_rx.pin << _GPIO_EUSART_RXROUTE_PIN_SHIFT) |
		(config->pin_rx.port << _GPIO_EUSART_RXROUTE_PORT_SHIFT);
#endif /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */

#ifdef EUSART_GECKO_HW_FLOW_CONTROL
	/* Configure HW flow control (RTS, CTS) */
	if (config->hw_flowcontrol) {
		GPIO_PinModeSet(config->pin_rts.port, config->pin_rts.pin,
				config->pin_rts.mode, config->pin_rts.out);
		GPIO_PinModeSet(config->pin_cts.port, config->pin_cts.pin,
				config->pin_cts.mode, config->pin_cts.out);

#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
		base->ROUTEPEN =
			EUSART_ROUTEPEN_RXPEN  |
			EUSART_ROUTEPEN_TXPEN  |
			EUSART_ROUTEPEN_RTSPEN |
			EUSART_ROUTEPEN_CTSPEN;

		base->ROUTELOC1 =
			(config->loc_rts << _EUSART_ROUTELOC1_RTSLOC_SHIFT) |
			(config->loc_cts << _EUSART_ROUTELOC1_CTSLOC_SHIFT);
#elif defined(GPIO_EUSART_ROUTEEN_RTSPEN) && defined(GPIO_EUSART_ROUTEEN_CTSPEN)
	GPIO->EUSARTROUTE[EUSART_NUM(base)].ROUTEEN =
		GPIO_EUSART_ROUTEEN_TXPEN |
		GPIO_EUSART_ROUTEEN_RXPEN |
		GPIO_EUSART_ROUTEPEN_RTSPEN |
		GPIO_EUSART_ROUTEPEN_CTSPEN;

	GPIO->EUSARTROUTE[EUSART_NUM(base)].RTSROUTE =
		(config->pin_rts.pin << _GPIO_EUSART_RTSROUTE_PIN_SHIFT) |
		(config->pin_rts.port << _GPIO_EUSART_RTSROUTE_PORT_SHIFT);
	GPIO->EUSARTROUTE[EUSART_NUM(base)].CTSROUTE =
		(config->pin_cts.pin << _GPIO_EUSART_CTSROUTE_PIN_SHIFT) |
		(config->pin_cts.port << _GPIO_EUSART_CTSROUTE_PORT_SHIFT);
#endif /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */
	}
#endif /* EUSART_GECKO_HW_FLOW_CONTROL */
}
#endif /* !CONFIG_PINCTRL */

/**
 * @brief Main initializer for UART
 *
 * @param dev UART device to be initialized
 * @return int 0
 */
static int eusart_gecko_init(const struct device *dev)
{
#ifdef CONFIG_PINCTRL
	int err;
#endif /* CONFIG_PINCTRL */
	const struct eusart_gecko_config *config = dev->config;
	EUSART_TypeDef *base = DEV_BASE(dev);

	EUSART_UartInit_TypeDef eusartInit = EUSART_UART_INIT_DEFAULT_HF;

	/* The peripheral and gpio clock are already enabled from soc and gpio
	 * driver
	 */
	/* Enable EUSART clock */
#ifdef CONFIG_CLOCK_CONTROL
	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0) {
		return err;
	}
#else
	CMU_ClockEnable(config->clock, true);
#endif

	/* Init EUSART */
	eusartInit.baudrate = config->baud_rate;
#ifdef EUSART_GECKO_HW_FLOW_CONTROL
	EUSART_AdvancedInit_TypeDef advancedSettings = EUSART_ADVANCED_INIT_DEFAULT;

	eusartInit.advancedSettings = &advancedSettings;
	eusartInit.advancedSettings->hwFlowControl = config->hw_flowcontrol ?
		eusartHwFlowControlCtsAndRts : eusartHwFlowControlNone;
#endif
	EUSART_UartInitHf(base, &eusartInit);

#ifdef CONFIG_PINCTRL
		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			return err;
		}
#else
	/* Initialize EUSART pins */
	eusart_gecko_init_pins(dev);
#endif /* CONFIG_PINCTRL */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int eusart_gecko_pm_action(const struct device *dev, enum pm_device_action action)
{
	__maybe_unused const struct eseusart_gecko_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
#ifdef EUSART_STATUS_TXIDLE
		/* Wait for TX FIFO to flush before suspending */
		while (!(EUSART_StatusGet(config->base) & EUSART_STATUS_TXIDLE)) {
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
	static void eusart_gecko_config_func_##idx(const struct device *dev)    \
	{								       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, rx, irq),		       \
			    DT_INST_IRQ_BY_NAME(idx, rx, priority),	       \
			    eusart_gecko_isr, DEVICE_DT_INST_GET(idx), 0);       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, tx, irq),		       \
			    DT_INST_IRQ_BY_NAME(idx, tx, priority),	       \
			    eusart_gecko_isr, DEVICE_DT_INST_GET(idx), 0);       \
									       \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, rx, irq));		       \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, tx, irq));		       \
	}
#else
#define GECKO_EUSART_IRQ_HANDLER_DECL(idx)
#define GECKO_EUSART_IRQ_HANDLER_FUNC(idx)
#define GECKO_EUSART_IRQ_HANDLER(idx)
#endif

#ifdef CONFIG_PINCTRL
#define GECKO_EUSART_INIT(idx)						       \
	PINCTRL_DT_INST_DEFINE(idx);					       \
	GECKO_EUSART_IRQ_HANDLER_DECL(idx);				       \
	PM_DEVICE_DT_INST_DEFINE(idx, eusart_gecko_pm_action);		       \
									       \
	static const struct eusart_gecko_config eusart_gecko_cfg_##idx = {        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),		       \
		.base = (EUSART_TypeDef *)DT_INST_REG_ADDR(idx),		       \
		GET_GECKO_EUSART_CLOCK(idx)				       \
		.baud_rate = DT_INST_PROP(idx, current_speed),		       \
		GECKO_EUSART_IRQ_HANDLER_FUNC(idx)			       \
		};							       \
									       \
	static struct eusart_gecko_data eusart_gecko_data_##idx;		       \
									       \
	DEVICE_DT_INST_DEFINE(idx, eusart_gecko_init, PM_DEVICE_DT_INST_GET(idx),\
			    &eusart_gecko_data_##idx,			       \
			    &eusart_gecko_cfg_##idx, PRE_KERNEL_1,	       \
			    CONFIG_SERIAL_INIT_PRIORITY,		       \
			    &eusart_gecko_driver_api);			       \
									       \
	GECKO_EUSART_IRQ_HANDLER(idx)
#else
#define GECKO_EUSART_INIT(idx)						       \
	VALIDATE_GECKO_EUSART_RX_TX_PIN_LOCATIONS(idx);			       \
	VALIDATE_GECKO_EUSART_RTS_CTS_PIN_LOCATIONS(idx);			       \
									       \
	GECKO_EUSART_IRQ_HANDLER_DECL(idx);				       \
	PM_DEVICE_DT_INST_DEFINE(idx, eusart_gecko_pm_action);		       \
									       \
	static const struct eusart_gecko_config eusart_gecko_cfg_##idx = {        \
		.base = (EUSART_TypeDef *)DT_INST_REG_ADDR(idx),		       \
		GET_GECKO_EUSART_CLOCK(idx)				       \
		.baud_rate = DT_INST_PROP(idx, current_speed),		       \
		GECKO_EUSART_HW_FLOW_CONTROL(idx)				       \
		GECKO_EUSART_RX_TX_PINS(idx)				       \
		GECKO_EUSART_RTS_CTS_PINS(idx)				       \
		GECKO_EUSART_RX_TX_PIN_LOCATIONS(idx)			       \
		GECKO_EUSART_RTS_CTS_PIN_LOCATIONS(idx)			       \
		GECKO_EUSART_IRQ_HANDLER_FUNC(idx)			       \
	};								       \
									       \
	static struct eusart_gecko_data eusart_gecko_data_##idx;		       \
									       \
	DEVICE_DT_INST_DEFINE(idx, eusart_gecko_init, PM_DEVICE_DT_INST_GET(idx),\
			    &eusart_gecko_data_##idx,			       \
			    &eusart_gecko_cfg_##idx, PRE_KERNEL_1,	       \
			    CONFIG_SERIAL_INIT_PRIORITY,		       \
			    &eusart_gecko_driver_api);			       \
									       \
	GECKO_EUSART_IRQ_HANDLER(idx)
#endif

DT_INST_FOREACH_STATUS_OKAY(GECKO_EUSART_INIT)
