/*
 * Copyright (c) 2017, Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <uart.h>
#include <em_usart.h>
#include <em_gpio.h>
#include <em_cmu.h>
#include <soc.h>

struct usart_gecko_config {
	USART_TypeDef *base;
	CMU_Clock_TypeDef clock;
	u32_t baud_rate;
	struct soc_gpio_pin pin_rx;
	struct soc_gpio_pin pin_tx;
	unsigned int loc;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(struct device *dev);
#endif
};

struct usart_gecko_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_t callback;
#endif
};

static int usart_gecko_poll_in(struct device *dev, unsigned char *c)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u32_t flags = USART_StatusGet(config->base);

	if (flags & USART_STATUS_RXDATAV) {
		*c = USART_Rx(config->base);
		return 0;
	}

	return -1;
}

static unsigned char usart_gecko_poll_out(struct device *dev, unsigned char c)
{
	const struct usart_gecko_config *config = dev->config->config_info;

	USART_Tx(config->base, c);

	return c;
}

static int usart_gecko_err_check(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u32_t flags = USART_IntGet(config->base);
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
static int usart_gecko_fifo_fill(struct device *dev, const u8_t *tx_data,
			       int len)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u8_t num_tx = 0;

	while ((len - num_tx > 0) &&
	       (config->base->STATUS & USART_STATUS_TXBL)) {

		config->base->TXDATA = (u32_t)tx_data[num_tx++];
	}

	return num_tx;
}

static int usart_gecko_fifo_read(struct device *dev, u8_t *rx_data,
			       const int len)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u8_t num_rx = 0;

	while ((len - num_rx > 0) &&
	       (config->base->STATUS & USART_STATUS_RXDATAV)) {

		rx_data[num_rx++] = (u8_t)config->base->RXDATA;
	}

	return num_rx;
}

static void usart_gecko_irq_tx_enable(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u32_t mask = USART_IEN_TXBL | USART_IEN_TXC;

	USART_IntEnable(config->base, mask);
}

static void usart_gecko_irq_tx_disable(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u32_t mask = USART_IEN_TXBL | USART_IEN_TXC;

	USART_IntDisable(config->base, mask);
}

static int usart_gecko_irq_tx_complete(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u32_t flags = USART_IntGet(config->base);

	USART_IntClear(config->base, USART_IF_TXC);

	return (flags & USART_IF_TXC) != 0;
}

static int usart_gecko_irq_tx_ready(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u32_t flags = USART_IntGet(config->base);

	return (flags & USART_IF_TXBL) != 0;
}

static void usart_gecko_irq_rx_enable(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u32_t mask = USART_IEN_RXDATAV;

	USART_IntEnable(config->base, mask);
}

static void usart_gecko_irq_rx_disable(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u32_t mask = USART_IEN_RXDATAV;

	USART_IntDisable(config->base, mask);
}

static int usart_gecko_irq_rx_full(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u32_t flags = USART_IntGet(config->base);

	return (flags & USART_IF_RXDATAV) != 0;
}

static int usart_gecko_irq_rx_ready(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	u32_t mask = USART_IEN_RXDATAV;

	return (config->base->IEN & mask)
		&& usart_gecko_irq_rx_full(dev);
}

static void usart_gecko_irq_err_enable(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;

	USART_IntEnable(config->base, USART_IF_RXOF |
			 USART_IF_PERR |
			 USART_IF_FERR);
}

static void usart_gecko_irq_err_disable(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;

	USART_IntDisable(config->base, USART_IF_RXOF |
			 USART_IF_PERR |
			 USART_IF_FERR);
}

static int usart_gecko_irq_is_pending(struct device *dev)
{
	return usart_gecko_irq_tx_ready(dev) || usart_gecko_irq_rx_ready(dev);
}

static int usart_gecko_irq_update(struct device *dev)
{
	return 1;
}

static void usart_gecko_irq_callback_set(struct device *dev,
				       uart_irq_callback_t cb)
{
	struct usart_gecko_data *data = dev->driver_data;

	data->callback = cb;
}

static void usart_gecko_isr(void *arg)
{
	struct device *dev = arg;
	struct usart_gecko_data *data = dev->driver_data;

	if (data->callback) {
		data->callback(dev);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static void usart_gecko_init_pins(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;

	soc_gpio_configure(&config->pin_rx);
	soc_gpio_configure(&config->pin_tx);

#if defined(USART_ROUTEPEN_TXPEN)
	config->base->ROUTEPEN = USART_ROUTEPEN_TXPEN
		| USART_ROUTEPEN_RXPEN;
	config->base->ROUTELOC0 = (config->base->ROUTELOC0
		& ~(_USART_ROUTELOC0_TXLOC_MASK
			| _USART_ROUTELOC0_RXLOC_MASK))
		| (config->loc
			<< _USART_ROUTELOC0_TXLOC_SHIFT)
		| (config->loc
			<< _USART_ROUTELOC0_RXLOC_SHIFT);
#else
	config->base->ROUTE = USART_ROUTE_RXPEN | USART_ROUTE_TXPEN
		| (config->loc << 8);
#endif

}

static int usart_gecko_init(struct device *dev)
{
	const struct usart_gecko_config *config = dev->config->config_info;
	USART_InitAsync_TypeDef usartInit = USART_INITASYNC_DEFAULT;

	/* The peripheral and gpio clock are already enabled from soc and gpio
	 * driver
	 */

	usartInit.baudrate = config->baud_rate;

	/* Enable USART clock */
	CMU_ClockEnable(config->clock, true);

	/* Init USART */
	usartInit.enable = usartDisable;
	USART_InitAsync(config->base, &usartInit);

	/* Initialize USART pins */
	usart_gecko_init_pins(dev);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	USART_Enable(config->base, usartEnable);

	return 0;
}

static const struct uart_driver_api usart_gecko_driver_api = {
	.poll_in = usart_gecko_poll_in,
	.poll_out = usart_gecko_poll_out,
	.err_check = usart_gecko_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = usart_gecko_fifo_fill,
	.fifo_read = usart_gecko_fifo_read,
	.irq_tx_enable = usart_gecko_irq_tx_enable,
	.irq_tx_disable = usart_gecko_irq_tx_disable,
	.irq_tx_complete = usart_gecko_irq_tx_complete,
	.irq_tx_ready = usart_gecko_irq_tx_ready,
	.irq_rx_enable = usart_gecko_irq_rx_enable,
	.irq_rx_disable = usart_gecko_irq_rx_disable,
	.irq_rx_ready = usart_gecko_irq_rx_ready,
	.irq_err_enable = usart_gecko_irq_err_enable,
	.irq_err_disable = usart_gecko_irq_err_disable,
	.irq_is_pending = usart_gecko_irq_is_pending,
	.irq_update = usart_gecko_irq_update,
	.irq_callback_set = usart_gecko_irq_callback_set,
#endif
};

#ifdef CONFIG_USART_GECKO_0

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void usart_gecko_config_func_0(struct device *dev);
#endif

static const struct usart_gecko_config usart_gecko_0_config = {
	.base = USART0,
	.clock = cmuClock_USART0,
	.baud_rate = CONFIG_USART_GECKO_0_BAUD_RATE,
	.pin_rx = PIN_USART0_RXD,
	.pin_tx = PIN_USART0_TXD,
	.loc = CONFIG_USART_GECKO_0_GPIO_LOC,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = usart_gecko_config_func_0,
#endif
};

static struct usart_gecko_data usart_gecko_0_data;

DEVICE_AND_API_INIT(usart_0, CONFIG_USART_GECKO_0_NAME,
		    &usart_gecko_init,
		    &usart_gecko_0_data, &usart_gecko_0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &usart_gecko_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void usart_gecko_config_func_0(struct device *dev)
{
	IRQ_CONNECT(USART0_RX_IRQn, CONFIG_USART_GECKO_0_IRQ_PRI,
		    usart_gecko_isr, DEVICE_GET(usart_0), 0);
	IRQ_CONNECT(USART0_TX_IRQn, CONFIG_USART_GECKO_0_IRQ_PRI,
		    usart_gecko_isr, DEVICE_GET(usart_0), 0);

	irq_enable(USART0_TX_IRQn);
	irq_enable(USART0_RX_IRQn);
}
#endif

#endif /* CONFIG_USART_GECKO_0 */

#ifdef CONFIG_USART_GECKO_1

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void usart_gecko_config_func_1(struct device *dev);
#endif

static const struct usart_gecko_config usart_gecko_1_config = {
	.base = USART1,
	.clock = cmuClock_USART1,
	.baud_rate = CONFIG_USART_GECKO_1_BAUD_RATE,
	.pin_rx = PIN_USART1_RXD,
	.pin_tx = PIN_USART1_TXD,
	.loc = CONFIG_USART_GECKO_1_GPIO_LOC,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = usart_gecko_config_func_1,
#endif
};

static struct usart_gecko_data usart_gecko_1_data;

DEVICE_AND_API_INIT(usart_1, CONFIG_USART_GECKO_1_NAME,
		    &usart_gecko_init,
		    &usart_gecko_1_data, &uart_gecko_1_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &usart_gecko_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void usart_gecko_config_func_1(struct device *dev)
{
	IRQ_CONNECT(USART1_RX_IRQn, CONFIG_USART_GECKO_1_IRQ_PRI,
		    usart_gecko_isr, DEVICE_GET(usart_1), 0);
	IRQ_CONNECT(USART1_TX_IRQn, CONFIG_USART_GECKO_1_IRQ_PRI,
		    usart_gecko_isr, DEVICE_GET(usart_1), 0);

	irq_enable(USART1_RX_IRQn);
	irq_enable(USART1_TX_IRQn);
}
#endif

#endif /* CONFIG_USART_GECKO_1 */
