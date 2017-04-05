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

struct uart_gecko_config {
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

struct uart_gecko_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_t callback;
#endif
};

static int uart_gecko_poll_in(struct device *dev, unsigned char *c)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	u32_t flags = USART_StatusGet(config->base);

	if (flags & USART_STATUS_RXDATAV) {
		*c = USART_Rx(config->base);
		return 0;
	}

	return -1;
}

static unsigned char uart_gecko_poll_out(struct device *dev, unsigned char c)
{
	const struct uart_gecko_config *config = dev->config->config_info;

	USART_Tx(config->base, c);

	return c;
}

static int uart_gecko_err_check(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;
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
static int uart_gecko_fifo_fill(struct device *dev, const u8_t *tx_data,
			       int len)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	u8_t num_tx = 0;

	while ((len - num_tx > 0) &&
	       (config->base->STATUS & USART_STATUS_TXBL)) {

		config->base->TXDATA = (u32_t)tx_data[num_tx++];
	}

	return num_tx;
}

static int uart_gecko_fifo_read(struct device *dev, u8_t *rx_data,
			       const int len)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	u8_t num_rx = 0;

	while ((len - num_rx > 0) &&
	       (config->base->STATUS & USART_STATUS_RXDATAV)) {

		rx_data[num_rx++] = (u8_t)config->base->RXDATA;
	}

	return num_rx;
}

static void uart_gecko_irq_tx_enable(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	u32_t mask = USART_IEN_TXBL | USART_IEN_TXC;

	USART_IntEnable(config->base, mask);
}

static void uart_gecko_irq_tx_disable(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	u32_t mask = USART_IEN_TXBL | USART_IEN_TXC;

	USART_IntDisable(config->base, mask);
}

static int uart_gecko_irq_tx_complete(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	u32_t flags = USART_IntGet(config->base);

	USART_IntClear(config->base, USART_IF_TXC);

	return (flags & USART_IF_TXC) != 0;
}

static int uart_gecko_irq_tx_ready(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	u32_t flags = USART_IntGet(config->base);

	return (flags & USART_IF_TXBL) != 0;
}

static void uart_gecko_irq_rx_enable(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	u32_t mask = USART_IEN_RXDATAV;

	USART_IntEnable(config->base, mask);
}

static void uart_gecko_irq_rx_disable(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	u32_t mask = USART_IEN_RXDATAV;

	USART_IntDisable(config->base, mask);
}

static int uart_gecko_irq_rx_full(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	u32_t flags = USART_IntGet(config->base);

	return (flags & USART_IF_RXDATAV) != 0;
}

static int uart_gecko_irq_rx_ready(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	u32_t mask = USART_IEN_RXDATAV;

	return (config->base->IEN & mask)
		&& uart_gecko_irq_rx_full(dev);
}

static void uart_gecko_irq_err_enable(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;

	USART_IntEnable(config->base, USART_IF_RXOF |
			 USART_IF_PERR |
			 USART_IF_FERR);
}

static void uart_gecko_irq_err_disable(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;

	USART_IntDisable(config->base, USART_IF_RXOF |
			 USART_IF_PERR |
			 USART_IF_FERR);
}

static int uart_gecko_irq_is_pending(struct device *dev)
{
	return uart_gecko_irq_tx_ready(dev) || uart_gecko_irq_rx_ready(dev);
}

static int uart_gecko_irq_update(struct device *dev)
{
	return 1;
}

static void uart_gecko_irq_callback_set(struct device *dev,
				       uart_irq_callback_t cb)
{
	struct uart_gecko_data *data = dev->driver_data;

	data->callback = cb;
}

static void uart_gecko_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_gecko_data *data = dev->driver_data;

	if (data->callback) {
		data->callback(dev);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static void uart_gecko_init_pins(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;

	soc_gpio_configure(&config->pin_rx);
	soc_gpio_configure(&config->pin_tx);

	config->base->ROUTE = USART_ROUTE_RXPEN | USART_ROUTE_TXPEN
		| (config->loc << 8);
}

static int uart_gecko_init(struct device *dev)
{
	const struct uart_gecko_config *config = dev->config->config_info;
	USART_InitAsync_TypeDef usartInit = USART_INITASYNC_DEFAULT;

	/* The peripheral and gpio clock are already enabled from soc and gpio
	 * driver
	 */

	usartInit.baudrate = config->baud_rate;

	/* Enable USART clock */
	CMU_ClockEnable(config->clock, true);

	/* Init USART */
	USART_InitAsync(config->base, &usartInit);

	/* Initialize USART pins */
	uart_gecko_init_pins(dev);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

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

#ifdef CONFIG_UART_GECKO_0

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_gecko_config_func_0(struct device *dev);
#endif

static const struct uart_gecko_config uart_gecko_0_config = {
	.base = UART0,
	.clock = cmuClock_UART0,
	.baud_rate = CONFIG_UART_GECKO_0_BAUD_RATE,
	.pin_rx = PIN_UART0_RXD,
	.pin_tx = PIN_UART0_TXD,
	.loc = CONFIG_UART_GECKO_0_GPIO_LOC,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_gecko_config_func_0,
#endif
};

static struct uart_gecko_data uart_gecko_0_data;

DEVICE_AND_API_INIT(uart_0, CONFIG_UART_GECKO_0_NAME,
		    &uart_gecko_init,
		    &uart_gecko_0_data, &uart_gecko_0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_gecko_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_gecko_config_func_0(struct device *dev)
{
	IRQ_CONNECT(UART0_RX_IRQn, CONFIG_UART_GECKO_0_IRQ_PRI,
		    uart_gecko_isr, DEVICE_GET(uart_0), 0);
	IRQ_CONNECT(UART0_TX_IRQn, CONFIG_UART_GECKO_0_IRQ_PRI,
		    uart_gecko_isr, DEVICE_GET(uart_0), 0);

	irq_enable(UART0_TX_IRQn);
	irq_enable(UART0_RX_IRQn);
}
#endif

#endif /* CONFIG_UART_GECKO_0 */

#ifdef CONFIG_UART_GECKO_1

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_gecko_config_func_1(struct device *dev);
#endif

static const struct uart_gecko_config uart_gecko_1_config = {
	.base = UART1,
	.clock = cmuClock_UART1,
	.baud_rate = CONFIG_UART_GECKO_1_BAUD_RATE,
	.pin_rx = PIN_UART1_RXD,
	.pin_tx = PIN_UART1_TXD,
	.loc = CONFIG_UART_GECKO_1_GPIO_LOC,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart_gecko_config_func_1,
#endif
};

static struct uart_gecko_data uart_gecko_1_data;

DEVICE_AND_API_INIT(uart_1, CONFIG_UART_GECKO_1_NAME,
		    &uart_gecko_init,
		    &uart_gecko_1_data, &uart_gecko_1_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_gecko_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_gecko_config_func_1(struct device *dev)
{
	IRQ_CONNECT(UART1_RX_IRQn, CONFIG_UART_GECKO_1_IRQ_PRI,
		    uart_gecko_isr, DEVICE_GET(uart_1), 0);
	IRQ_CONNECT(UART1_TX_IRQn, CONFIG_UART_GECKO_1_IRQ_PRI,
		    uart_gecko_isr, DEVICE_GET(uart_1), 0);

	irq_enable(UART1_RX_IRQn);
	irq_enable(UART1_TX_IRQn);
}
#endif

#endif /* CONFIG_UART_GECKO_1 */
