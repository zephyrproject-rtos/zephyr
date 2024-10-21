/*
 * Copyright (c) 2017, Christian Taedcke
 * Copyright (c) 2020 Lemonbeat GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <em_usart.h>
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
#define GET_GECKO_USART_CLOCK(idx)                            \
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)), \
	.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),
#elif DT_NODE_HAS_PROP(id, peripheral_id)
#define USART_PREFIX cmuClock_USART
#define UART_PREFIX cmuClock_UART
#define CLOCK_USART(id) _CONCAT(USART_PREFIX, id)
#define CLOCK_UART(id) _CONCAT(UART_PREFIX, id)
#define GET_GECKO_USART_CLOCK(id) \
	.clock = CLOCK_USART(DT_INST_PROP(id, peripheral_id)),
#define GET_GECKO_UART_CLOCK(id) \
	.clock = CLOCK_UART(DT_INST_PROP(id, peripheral_id)),
#else
#if (USART_COUNT == 1)
#define CLOCK_USART(ref)	(((ref) == USART0) ? cmuClock_USART0 \
			       : -1)
#elif (USART_COUNT == 2)
#define CLOCK_USART(ref)	(((ref) == USART0) ? cmuClock_USART0 \
			       : ((ref) == USART1) ? cmuClock_USART1 \
			       : -1)
#elif (USART_COUNT == 3)
#define CLOCK_USART(ref)	(((ref) == USART0) ? cmuClock_USART0 \
			       : ((ref) == USART1) ? cmuClock_USART1 \
			       : ((ref) == USART2) ? cmuClock_USART2 \
			       : -1)
#elif (USART_COUNT == 4)
#define CLOCK_USART(ref)	(((ref) == USART0) ? cmuClock_USART0 \
			       : ((ref) == USART1) ? cmuClock_USART1 \
			       : ((ref) == USART2) ? cmuClock_USART2 \
			       : ((ref) == USART3) ? cmuClock_USART3 \
			       : -1)
#elif (USART_COUNT == 5)
#define CLOCK_USART(ref)	(((ref) == USART0) ? cmuClock_USART0 \
			       : ((ref) == USART1) ? cmuClock_USART1 \
			       : ((ref) == USART2) ? cmuClock_USART2 \
			       : ((ref) == USART3) ? cmuClock_USART3 \
			       : ((ref) == USART4) ? cmuClock_USART4 \
			       : -1)
#elif (USART_COUNT == 6)
#define CLOCK_USART(ref)	(((ref) == USART0) ? cmuClock_USART0 \
			       : ((ref) == USART1) ? cmuClock_USART1 \
			       : ((ref) == USART2) ? cmuClock_USART2 \
			       : ((ref) == USART3) ? cmuClock_USART3 \
			       : ((ref) == USART4) ? cmuClock_USART4 \
			       : ((ref) == USART5) ? cmuClock_USART5 \
			       : -1)
#else
#error "Undefined number of USARTs."
#endif /* USART_COUNT */

#define CLOCK_UART(ref)		(((ref) == UART0) ? cmuClock_UART0  \
			       : ((ref) == UART1) ? cmuClock_UART1  \
			       : -1)
#define GET_GECKO_USART_CLOCK(id) \
	.clock = CLOCK_USART((USART_TypeDef *)DT_INST_REG_ADDR(id)),
#define GET_GECKO_UART_CLOCK(id) \
	.clock = CLOCK_UART((USART_TypeDef *)DT_INST_REG_ADDR(id)),
#endif /* DT_NODE_HAS_PROP(id, peripheral_id) */

/* Helper define to determine if SOC supports hardware flow control */
#if ((_SILICON_LABS_32B_SERIES > 0) ||					\
	(defined(_USART_ROUTEPEN_RTSPEN_MASK) &&			\
	defined(_USART_ROUTEPEN_CTSPEN_MASK)))
#define HW_FLOWCONTROL_IS_SUPPORTED_BY_SOC
#endif

#define HAS_HFC_OR(inst) DT_INST_PROP(inst, hw_flow_control) ||

#define DT_DRV_COMPAT silabs_gecko_uart

/* Has any enabled uart instance hw-flow-control enabled? */
#define UART_GECKO_UART_HW_FLOW_CONTROL_ENABLED				\
	DT_INST_FOREACH_STATUS_OKAY(HAS_HFC_OR) 0

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT silabs_gecko_usart

/* Has any enabled usart instance hw-flow-control enabled? */
#define UART_GECKO_USART_HW_FLOW_CONTROL_ENABLED			\
	DT_INST_FOREACH_STATUS_OKAY(HAS_HFC_OR) 0

#if UART_GECKO_USART_HW_FLOW_CONTROL_ENABLED ||				\
	UART_GECKO_UART_HW_FLOW_CONTROL_ENABLED
#define UART_GECKO_HW_FLOW_CONTROL
#endif

/* Sanity check for hardware flow control */
#if defined(UART_GECKO_HW_FLOW_CONTROL) &&				\
	(!(defined(HW_FLOWCONTROL_IS_SUPPORTED_BY_SOC)))
#error "Hardware flow control is activated for at least one UART/USART,	\
but not supported by this SOC"
#endif

#if defined(UART_GECKO_HW_FLOW_CONTROL) &&				\
	(!defined(CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION) &&	\
	 !defined(GPIO_USART_ROUTEEN_RTSPEN))
#error "Driver not supporting hardware flow control for this SOC"
#endif

/**
 * @brief Config struct for UART
 */

struct uart_gecko_config {
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pcfg;
#endif /* CONFIG_PINCTRL */
	USART_TypeDef *base;
#ifdef CONFIG_CLOCK_CONTROL
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
#else
	CMU_Clock_TypeDef clock;
#endif
	uint32_t baud_rate;
#ifndef CONFIG_PINCTRL
#ifdef UART_GECKO_HW_FLOW_CONTROL
	bool hw_flowcontrol;
#endif /* UART_GECKO_HW_FLOW_CONTROL */
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifndef CONFIG_PINCTRL
	struct soc_gpio_pin pin_rx;
	struct soc_gpio_pin pin_tx;
#ifdef UART_GECKO_HW_FLOW_CONTROL
	struct soc_gpio_pin pin_rts;
	struct soc_gpio_pin pin_cts;
#endif /* UART_GECKO_HW_FLOW_CONTROL */
#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
	uint8_t loc_rx;
	uint8_t loc_tx;
#ifdef UART_GECKO_HW_FLOW_CONTROL
	uint8_t loc_rts;
	uint8_t loc_cts;
#endif /* UART_GECKO_HW_FLOW_CONTROL */
#else /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */
	uint8_t loc;
#endif /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */
#endif
};

struct uart_gecko_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int uart_gecko_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_gecko_config *config = dev->config;
	uint32_t flags = USART_StatusGet(config->base);

	if (flags & USART_STATUS_RXDATAV) {
		*c = USART_Rx(config->base);
		return 0;
	}

	return -1;
}

static void uart_gecko_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_gecko_config *config = dev->config;

	USART_Tx(config->base, c);
}

static int uart_gecko_err_check(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;
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

	USART_IntClear(config->base, USART_IF_RXOF |
		       USART_IF_PERR |
		       USART_IF_FERR);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_gecko_fifo_fill(const struct device *dev, const uint8_t *tx_data,
			       int len)
{
	const struct uart_gecko_config *config = dev->config;
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (config->base->STATUS & USART_STATUS_TXBL)) {

		config->base->TXDATA = (uint32_t)tx_data[num_tx++];
	}

	return num_tx;
}

static int uart_gecko_fifo_read(const struct device *dev, uint8_t *rx_data,
			       const int len)
{
	const struct uart_gecko_config *config = dev->config;
	uint8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (config->base->STATUS & USART_STATUS_RXDATAV)) {

		rx_data[num_rx++] = (uint8_t)config->base->RXDATA;
	}

	return num_rx;
}

static void uart_gecko_irq_tx_enable(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;
	uint32_t mask = USART_IEN_TXBL | USART_IEN_TXC;

	USART_IntEnable(config->base, mask);
}

static void uart_gecko_irq_tx_disable(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;
	uint32_t mask = USART_IEN_TXBL | USART_IEN_TXC;

	USART_IntDisable(config->base, mask);
}

static int uart_gecko_irq_tx_complete(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;
	uint32_t flags = USART_IntGet(config->base);

	USART_IntClear(config->base, USART_IF_TXC);

	return (flags & USART_IF_TXC) != 0U;
}

static int uart_gecko_irq_tx_ready(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;
	uint32_t flags = USART_IntGetEnabled(config->base);

	return (flags & USART_IF_TXBL) != 0U;
}

static void uart_gecko_irq_rx_enable(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;
	uint32_t mask = USART_IEN_RXDATAV;

	USART_IntEnable(config->base, mask);
}

static void uart_gecko_irq_rx_disable(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;
	uint32_t mask = USART_IEN_RXDATAV;

	USART_IntDisable(config->base, mask);
}

static int uart_gecko_irq_rx_full(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;
	uint32_t flags = USART_IntGet(config->base);

	return (flags & USART_IF_RXDATAV) != 0U;
}

static int uart_gecko_irq_rx_ready(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;
	uint32_t mask = USART_IEN_RXDATAV;

	return (config->base->IEN & mask)
		&& uart_gecko_irq_rx_full(dev);
}

static void uart_gecko_irq_err_enable(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;

	USART_IntEnable(config->base, USART_IF_RXOF |
			 USART_IF_PERR |
			 USART_IF_FERR);
}

static void uart_gecko_irq_err_disable(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;

	USART_IntDisable(config->base, USART_IF_RXOF |
			 USART_IF_PERR |
			 USART_IF_FERR);
}

static int uart_gecko_irq_is_pending(const struct device *dev)
{
	return uart_gecko_irq_tx_ready(dev) || uart_gecko_irq_rx_ready(dev);
}

static int uart_gecko_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_gecko_irq_callback_set(const struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_gecko_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_gecko_isr(const struct device *dev)
{
	struct uart_gecko_data *data = dev->data;

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
static void uart_gecko_init_pins(const struct device *dev)
{
	const struct uart_gecko_config *config = dev->config;

	/* Configure RX and TX */
	GPIO_PinModeSet(config->pin_rx.port, config->pin_rx.pin,
			config->pin_rx.mode, config->pin_rx.out);
	GPIO_PinModeSet(config->pin_tx.port, config->pin_tx.pin,
			config->pin_tx.mode, config->pin_tx.out);

#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
	/* For SOCs with configurable pin locations (set in SOC Kconfig) */
	config->base->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
	config->base->ROUTELOC0 =
		(config->loc_tx << _USART_ROUTELOC0_TXLOC_SHIFT) |
		(config->loc_rx << _USART_ROUTELOC0_RXLOC_SHIFT);
	config->base->ROUTELOC1 = _USART_ROUTELOC1_RESETVALUE;
#elif defined(USART_ROUTE_RXPEN) && defined(USART_ROUTE_TXPEN)
	/* For olders SOCs with only one pin location */
	config->base->ROUTE = USART_ROUTE_RXPEN | USART_ROUTE_TXPEN
		| (config->loc << 8);
#elif defined(GPIO_USART_ROUTEEN_RXPEN) && defined(GPIO_USART_ROUTEEN_TXPEN)
	GPIO->USARTROUTE[USART_NUM(config->base)].ROUTEEN =
		GPIO_USART_ROUTEEN_TXPEN | GPIO_USART_ROUTEEN_RXPEN;
	GPIO->USARTROUTE[USART_NUM(config->base)].TXROUTE =
		(config->pin_tx.pin << _GPIO_USART_TXROUTE_PIN_SHIFT) |
		(config->pin_tx.port << _GPIO_USART_TXROUTE_PORT_SHIFT);
	GPIO->USARTROUTE[USART_NUM(config->base)].RXROUTE =
		(config->pin_rx.pin << _GPIO_USART_RXROUTE_PIN_SHIFT) |
		(config->pin_rx.port << _GPIO_USART_RXROUTE_PORT_SHIFT);
#endif /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */

#ifdef UART_GECKO_HW_FLOW_CONTROL
	/* Configure HW flow control (RTS, CTS) */
	if (config->hw_flowcontrol) {
		GPIO_PinModeSet(config->pin_rts.port, config->pin_rts.pin,
				config->pin_rts.mode, config->pin_rts.out);
		GPIO_PinModeSet(config->pin_cts.port, config->pin_cts.pin,
				config->pin_cts.mode, config->pin_cts.out);

#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
		config->base->ROUTEPEN =
			USART_ROUTEPEN_RXPEN  |
			USART_ROUTEPEN_TXPEN  |
			USART_ROUTEPEN_RTSPEN |
			USART_ROUTEPEN_CTSPEN;

		config->base->ROUTELOC1 =
			(config->loc_rts << _USART_ROUTELOC1_RTSLOC_SHIFT) |
			(config->loc_cts << _USART_ROUTELOC1_CTSLOC_SHIFT);
#elif defined(GPIO_USART_ROUTEEN_RTSPEN) && defined(GPIO_USART_ROUTEEN_CTSPEN)
	GPIO->USARTROUTE[USART_NUM(config->base)].ROUTEEN =
		GPIO_USART_ROUTEEN_TXPEN |
		GPIO_USART_ROUTEEN_RXPEN |
		GPIO_USART_ROUTEPEN_RTSPEN |
		GPIO_USART_ROUTEPEN_CTSPEN;

	GPIO->USARTROUTE[USART_NUM(config->base)].RTSROUTE =
		(config->pin_rts.pin << _GPIO_USART_RTSROUTE_PIN_SHIFT) |
		(config->pin_rts.port << _GPIO_USART_RTSROUTE_PORT_SHIFT);
	GPIO->USARTROUTE[USART_NUM(config->base)].CTSROUTE =
		(config->pin_cts.pin << _GPIO_USART_CTSROUTE_PIN_SHIFT) |
		(config->pin_cts.port << _GPIO_USART_CTSROUTE_PORT_SHIFT);
#endif /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */
	}
#endif /* UART_GECKO_HW_FLOW_CONTROL */
}
#endif /* !CONFIG_PINCTRL */

/**
 * @brief Main initializer for UART
 *
 * @param dev UART device to be initialized
 * @return int 0
 */
static int uart_gecko_init(const struct device *dev)
{
#ifdef CONFIG_PINCTRL
	int err;
#endif /* CONFIG_PINCTRL */
	const struct uart_gecko_config *config = dev->config;

	USART_InitAsync_TypeDef usartInit = USART_INITASYNC_DEFAULT;

	/* The peripheral and gpio clock are already enabled from soc and gpio driver */
	/* Enable USART clock */
#ifdef CONFIG_CLOCK_CONTROL
	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0) {
		return err;
	}
#else
	CMU_ClockEnable(config->clock, true);
#endif

	/* Init USART */
	usartInit.baudrate = config->baud_rate;
#ifdef UART_GECKO_HW_FLOW_CONTROL
	usartInit.hwFlowControl = config->hw_flowcontrol ?
		usartHwFlowControlCtsAndRts : usartHwFlowControlNone;
#endif
	USART_InitAsync(config->base, &usartInit);

#ifdef CONFIG_PINCTRL
		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			return err;
		}
#else
	/* Initialize USART pins */
	uart_gecko_init_pins(dev);
#endif /* CONFIG_PINCTRL */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int uart_gecko_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct uart_gecko_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* Wait for TX FIFO to flush before suspending */
		while (!(USART_StatusGet(config->base) & USART_STATUS_TXIDLE)) {
		}
		break;

	case PM_DEVICE_ACTION_RESUME:
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static const struct uart_driver_api uart_gecko_driver_api = {
	.poll_in = uart_gecko_poll_in,
	.poll_out = uart_gecko_poll_out,
	.err_check = uart_gecko_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_gecko_fifo_fill,
	.fifo_read = uart_gecko_fifo_read,
	.irq_tx_enable = uart_gecko_irq_tx_enable,
	.irq_tx_disable = uart_gecko_irq_tx_disable,
	.irq_tx_complete = uart_gecko_irq_tx_complete,
	.irq_tx_ready = uart_gecko_irq_tx_ready,
	.irq_rx_enable = uart_gecko_irq_rx_enable,
	.irq_rx_disable = uart_gecko_irq_rx_disable,
	.irq_rx_ready = uart_gecko_irq_rx_ready,
	.irq_err_enable = uart_gecko_irq_err_enable,
	.irq_err_disable = uart_gecko_irq_err_disable,
	.irq_is_pending = uart_gecko_irq_is_pending,
	.irq_update = uart_gecko_irq_update,
	.irq_callback_set = uart_gecko_irq_callback_set,
#endif
};

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT silabs_gecko_uart

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define GECKO_UART_IRQ_HANDLER_DECL(idx)				       \
	static void uart_gecko_config_func_##idx(const struct device *dev)
#define GECKO_UART_IRQ_HANDLER_FUNC(idx)				       \
	.irq_config_func = uart_gecko_config_func_##idx,
#define GECKO_UART_IRQ_HANDLER(idx)					       \
	static void uart_gecko_config_func_##idx(const struct device *dev)     \
	{								       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, rx, irq),		       \
			    DT_INST_IRQ_BY_NAME(idx, rx, priority),	       \
			    uart_gecko_isr, DEVICE_DT_INST_GET(idx), 0);       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, tx, irq),		       \
			    DT_INST_IRQ_BY_NAME(idx, tx, priority),	       \
			    uart_gecko_isr, DEVICE_DT_INST_GET(idx), 0);       \
									       \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, rx, irq));		       \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, tx, irq));		       \
	}
#else /* CONFIG_UART_INTERRUPT_DRIVEN */
#define GECKO_UART_IRQ_HANDLER_DECL(idx)
#define GECKO_UART_IRQ_HANDLER_FUNC(idx)
#define GECKO_UART_IRQ_HANDLER(idx)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
#define GECKO_UART_RX_TX_PIN_LOCATIONS(idx)				       \
	.loc_rx = DT_INST_PROP_BY_IDX(idx, location_rx, 0),		       \
	.loc_tx = DT_INST_PROP_BY_IDX(idx, location_tx, 0),
#define VALIDATE_GECKO_UART_RX_TX_PIN_LOCATIONS(idx)
#else
#define GECKO_UART_RX_TX_PIN_LOCATIONS(idx)				       \
	.loc = DT_INST_PROP_BY_IDX(idx, location_rx, 0),
#define VALIDATE_GECKO_UART_RX_TX_PIN_LOCATIONS(idx)			       \
	BUILD_ASSERT(DT_INST_PROP_BY_IDX(idx, location_rx, 0) ==	       \
			     DT_INST_PROP_BY_IDX(idx, location_tx, 0),	       \
		     "DTS location-* properties must have identical value")
#endif

#define PIN_UART_RXD(idx)						       \
	{								       \
		DT_INST_PROP_BY_IDX(idx, location_rx, 1),		       \
			DT_INST_PROP_BY_IDX(idx, location_rx, 2),	       \
			gpioModeInput, 1				       \
	}
#define PIN_UART_TXD(idx)						       \
	{								       \
		DT_INST_PROP_BY_IDX(idx, location_tx, 1),		       \
			DT_INST_PROP_BY_IDX(idx, location_tx, 2),	       \
			gpioModePushPull, 1				       \
	}

#define GECKO_UART_RX_TX_PINS(idx)					       \
	.pin_rx = PIN_UART_RXD(idx),					       \
	.pin_tx = PIN_UART_TXD(idx),

#ifdef UART_GECKO_HW_FLOW_CONTROL

#ifdef CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION
#define GECKO_UART_RTS_CTS_PIN_LOCATIONS(idx)				       \
	.loc_rts = COND_CODE_1(DT_INST_PROP(idx, hw_flow_control),	       \
		   (DT_INST_PROP_BY_IDX(idx, location_rts, 0)),		       \
		   (0)),						       \
	.loc_cts = COND_CODE_1(DT_INST_PROP(idx, hw_flow_control),	       \
		   (DT_INST_PROP_BY_IDX(idx, location_cts, 0)),		       \
		   (0)),
#define VALIDATE_GECKO_UART_RTS_CTS_PIN_LOCATIONS(idx)			       \
	COND_CODE_1(DT_INST_PROP(idx, hw_flow_control),			       \
		    (BUILD_ASSERT(DT_INST_NODE_HAS_PROP(idx, location_rts) &&  \
				  DT_INST_NODE_HAS_PROP(idx, location_cts),    \
			"DTS location-rts and location-cts are mandatory")),   \
		    ())
#else /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */
/* Hardware flow control not supported for these SOCs */
#define GECKO_UART_RTS_CTS_PIN_LOCATIONS(idx)
#define VALIDATE_GECKO_UART_RTS_CTS_PIN_LOCATIONS(idx)
#endif /* CONFIG_SOC_GECKO_HAS_INDIVIDUAL_PIN_LOCATION */

#define PIN_UART_RTS(idx)						       \
	COND_CODE_1(DT_INST_PROP(idx, hw_flow_control),			       \
	({								       \
		DT_INST_PROP_BY_IDX(idx, location_rts, 1),		       \
			DT_INST_PROP_BY_IDX(idx, location_rts, 2),	       \
			gpioModePushPull, 1				       \
	}),								       \
	({0}))

#define PIN_UART_CTS(idx)						       \
	COND_CODE_1(DT_INST_PROP(idx, hw_flow_control),			       \
	({								       \
		DT_INST_PROP_BY_IDX(idx, location_cts, 1),		       \
			DT_INST_PROP_BY_IDX(idx, location_cts, 2),	       \
			gpioModeInput, 1				       \
	}),								       \
	({0}))

#define GECKO_UART_RTS_CTS_PINS(idx)					       \
	.pin_rts = PIN_UART_RTS(idx),					       \
	.pin_cts = PIN_UART_CTS(idx),

#define GECKO_UART_HW_FLOW_CONTROL(idx)					       \
	.hw_flowcontrol = DT_INST_PROP(idx, hw_flow_control),

#else /* UART_GECKO_HW_FLOW_CONTROL */

#define GECKO_UART_RTS_CTS_PIN_LOCATIONS(idx)
#define VALIDATE_GECKO_UART_RTS_CTS_PIN_LOCATIONS(idx)
#define GECKO_UART_RTS_CTS_PINS(idx)
#define GECKO_UART_HW_FLOW_CONTROL(idx)

#endif /* UART_GECKO_HW_FLOW_CONTROL */

#define GECKO_UART_INIT(idx)						       \
	VALIDATE_GECKO_UART_RX_TX_PIN_LOCATIONS(idx);			       \
	VALIDATE_GECKO_UART_RTS_CTS_PIN_LOCATIONS(idx);			       \
									       \
	GECKO_UART_IRQ_HANDLER_DECL(idx);				       \
									       \
	static const struct uart_gecko_config uart_gecko_cfg_##idx = {	       \
		.base = (USART_TypeDef *)DT_INST_REG_ADDR(idx),		       \
		GET_GECKO_UART_CLOCK(idx)				       \
		.baud_rate = DT_INST_PROP(idx, current_speed),		       \
		GECKO_UART_HW_FLOW_CONTROL(idx)				       \
		GECKO_UART_RX_TX_PINS(idx)				       \
		GECKO_UART_RTS_CTS_PINS(idx)				       \
		GECKO_UART_RX_TX_PIN_LOCATIONS(idx)			       \
		GECKO_UART_RTS_CTS_PIN_LOCATIONS(idx)			       \
		GECKO_UART_IRQ_HANDLER_FUNC(idx)			       \
	};								       \
									       \
	static struct uart_gecko_data uart_gecko_data_##idx;		       \
									       \
	DEVICE_DT_INST_DEFINE(idx, uart_gecko_init,			       \
			    NULL, &uart_gecko_data_##idx,		       \
			    &uart_gecko_cfg_##idx, PRE_KERNEL_1,	       \
			    CONFIG_SERIAL_INIT_PRIORITY,		       \
			    &uart_gecko_driver_api);			       \
									       \
									       \
	GECKO_UART_IRQ_HANDLER(idx)

DT_INST_FOREACH_STATUS_OKAY(GECKO_UART_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT silabs_gecko_usart

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define GECKO_USART_IRQ_HANDLER_DECL(idx)				       \
	static void usart_gecko_config_func_##idx(const struct device *dev)
#define GECKO_USART_IRQ_HANDLER_FUNC(idx)				       \
	.irq_config_func = usart_gecko_config_func_##idx,
#define GECKO_USART_IRQ_HANDLER(idx)					       \
	static void usart_gecko_config_func_##idx(const struct device *dev)    \
	{								       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, rx, irq),		       \
			    DT_INST_IRQ_BY_NAME(idx, rx, priority),	       \
			    uart_gecko_isr, DEVICE_DT_INST_GET(idx), 0);       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, tx, irq),		       \
			    DT_INST_IRQ_BY_NAME(idx, tx, priority),	       \
			    uart_gecko_isr, DEVICE_DT_INST_GET(idx), 0);       \
									       \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, rx, irq));		       \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, tx, irq));		       \
	}
#else
#define GECKO_USART_IRQ_HANDLER_DECL(idx)
#define GECKO_USART_IRQ_HANDLER_FUNC(idx)
#define GECKO_USART_IRQ_HANDLER(idx)
#endif

#ifdef CONFIG_PINCTRL
#define GECKO_USART_INIT(idx)						       \
	PINCTRL_DT_INST_DEFINE(idx);					       \
	GECKO_USART_IRQ_HANDLER_DECL(idx);				       \
	PM_DEVICE_DT_INST_DEFINE(idx, uart_gecko_pm_action);		       \
									       \
	static const struct uart_gecko_config usart_gecko_cfg_##idx = {        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),		       \
		.base = (USART_TypeDef *)DT_INST_REG_ADDR(idx),		       \
		GET_GECKO_USART_CLOCK(idx)				       \
		.baud_rate = DT_INST_PROP(idx, current_speed),		       \
		GECKO_USART_IRQ_HANDLER_FUNC(idx)			       \
		};							       \
									       \
	static struct uart_gecko_data usart_gecko_data_##idx;		       \
									       \
	DEVICE_DT_INST_DEFINE(idx, uart_gecko_init, PM_DEVICE_DT_INST_GET(idx),\
			    &usart_gecko_data_##idx,			       \
			    &usart_gecko_cfg_##idx, PRE_KERNEL_1,	       \
			    CONFIG_SERIAL_INIT_PRIORITY,		       \
			    &uart_gecko_driver_api);			       \
									       \
	GECKO_USART_IRQ_HANDLER(idx)
#else
#define GECKO_USART_INIT(idx)						       \
	VALIDATE_GECKO_UART_RX_TX_PIN_LOCATIONS(idx);			       \
	VALIDATE_GECKO_UART_RTS_CTS_PIN_LOCATIONS(idx);			       \
									       \
	GECKO_USART_IRQ_HANDLER_DECL(idx);				       \
	PM_DEVICE_DT_INST_DEFINE(idx, uart_gecko_pm_action);		       \
									       \
	static const struct uart_gecko_config usart_gecko_cfg_##idx = {        \
		.base = (USART_TypeDef *)DT_INST_REG_ADDR(idx),		       \
		GET_GECKO_USART_CLOCK(idx)				       \
		.baud_rate = DT_INST_PROP(idx, current_speed),		       \
		GECKO_UART_HW_FLOW_CONTROL(idx)				       \
		GECKO_UART_RX_TX_PINS(idx)				       \
		GECKO_UART_RTS_CTS_PINS(idx)				       \
		GECKO_UART_RX_TX_PIN_LOCATIONS(idx)			       \
		GECKO_UART_RTS_CTS_PIN_LOCATIONS(idx)			       \
		GECKO_USART_IRQ_HANDLER_FUNC(idx)			       \
	};								       \
									       \
	static struct uart_gecko_data usart_gecko_data_##idx;		       \
									       \
	DEVICE_DT_INST_DEFINE(idx, uart_gecko_init, PM_DEVICE_DT_INST_GET(idx),\
			    &usart_gecko_data_##idx,			       \
			    &usart_gecko_cfg_##idx, PRE_KERNEL_1,	       \
			    CONFIG_SERIAL_INIT_PRIORITY,		       \
			    &uart_gecko_driver_api);			       \
									       \
	GECKO_USART_IRQ_HANDLER(idx)
#endif

DT_INST_FOREACH_STATUS_OKAY(GECKO_USART_INIT)
