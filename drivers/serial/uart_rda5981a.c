/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <board.h>
#include <init.h>
#include <uart.h>
#include <clock_control.h>
#include <gpio.h>
#include <pinmux.h>
#include <pinmux/pinmux.h>
#include <pinmux/rda5981a/pinmux_rda5981a.h>

#include <sections.h>
#include "uart_rda5981a.h"
#include "soc_registers.h"

#define UART1_CLKEN_MASK        (0x01UL << 21)
#define RX_FIFO_DATA_RDY       (0x01UL << 0)

#define TX_FIFO_EMPTY        (0x01UL << 18)
#define TX_FIFO_FULL        (0x01UL << 19)
#define AFCE_MASK               (0x01UL << 5)

#define ENABLE_IRQ_RX	(1 << 0)
#define ENABLE_IRQ_TX	(1 << 1)
#define ENABLE_IRQ_LINE_STATUS	(1 << 2)

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct uart_rda5981a_config * const)(dev)->config->config_info)
#define DEV_DATA(dev)							\
	((struct uart_rda5981a_data * const)(dev)->driver_data)
#define UART_STRUCT(dev)					\
	((volatile struct uart_rda5981a *)(DEV_CFG(dev))->uconf.base)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_irq_config_func_0(struct device *dev);
#endif

static const struct pin_config uart_pinconf[] = {
	{UART0_RX, 0},
	{UART0_TX, 0},
};

#if 0
static void set_baud_rate(struct device *dev, uint32_t rate)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);
	const struct uart_rda5981a_config *conf = DEV_CFG(dev);
	uint32_t baud_divisor, baud_mod;

	baud_divisor  = (conf->ahb_bus_clk / rate) >> 4;
	baud_mod      = (conf->ahb_bus_clk / rate) & 0x0F;

	/* enable load devisor register */
	uart->lcr |= (1 << 7);

	uart->dll = (baud_divisor >> 0) & 0xFF;
	uart->dlh = (baud_divisor >> 8) & 0xFF;
	uart->dl2 = (baud_mod>>1) + ((baud_mod - (baud_mod>>1))<<4);
	/* after loading, disable load devisor register */
	uart->lcr &= ~(1 << 7);
}
#endif

static uint32_t serial_format(int data_bits, serial_parity parity,
			int stop_bits)
{
	int parity_enable, parity_select;
	uint32_t s_format;

	stop_bits -= 1;
	data_bits -= 5;

	switch (parity) {
	case PN:
		parity_enable = 0; parity_select = 0;
		break;
	case PO:
		parity_enable = 1; parity_select = 0;
		break;
	case PE:
		parity_enable = 1; parity_select = 1;
		break;
	case PF1:
		parity_enable = 1; parity_select = 2;
		break;
	case PF0:
		parity_enable = 1; parity_select = 3;
		break;
	default:
		parity_enable = 0; parity_select = 0;
		break;
	}

	s_format = (data_bits << 0
		| stop_bits << 2
		| parity_enable << 3
		| parity_select  << 4);

	return s_format;
}

static int uart_rda5981a_init(struct device *dev)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);
	struct uart_rda5981a_data *data = DEV_DATA(dev);
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_rda5981a_config *cfg = DEV_CFG(dev);
#endif
	if (data->uart_index == 1) {
	}

	uart->ier = 0 << 0 /* rx data available irq enable */
		| 0 << 1 /* tx fifo empty irq enable */
		| 0 << 2; /* rxline status irq enable */

	/* enable fifos and default rx trigger level */
	uart->fcr = 0 << 0 /* fifo Enable - 0 = disables, 1 = enabled */
		| 0 << 1 /* rx fifo reset */
		| 0 << 2 /* rx fifo reset */
		| 0 << 6; /* rx irq trigger level - 0 = 1 char, 1 = 4 chars, 2 = 8 chars, 3 = 14 chars */

	uart->mcr = 1 << 8; /* select clock */
	uart->frr = (0x10 << 9) | (0x1 << 0); /* tx_trigger = 0x10,rx_triger = 0x01 */

	/* set default baud rate and format */
	/* set_baud_rate(dev, data->baud_rate); */
	uart->lcr = serial_format(8, PN, 1);

	rda5981a_setup_pins(uart_pinconf, ARRAY_SIZE(uart_pinconf));

	/* todo pinmode */
	/* todo flow control */

	uart->ier = 1 << 2;

	/* clear */
	uart->fcr = 1 << 0
		| 1 << 1
		| 1 << 2;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->uconf.irq_config_func(dev);
#endif

	return 0;
}


static int serial_readable(struct device *dev)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);

	return (uart->lsr & RX_FIFO_DATA_RDY);
}

static int serial_writable(struct device *dev)
{
	struct uart_rda5981a_data *data = DEV_DATA(dev);
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);

	if (data->uart_index == 0) {
		/* uart0 not have flow control */
		return (uart->fsr & TX_FIFO_FULL)
	} else {
		/* todo uart1 */
		return 0;
	}
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_rda5981a_fifo_fill(struct device *dev, const uint8_t *tx_data,
				  int size)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);
	uint8_t num_tx = 0;

	while ((size - num_tx > 0) && !serial_writable(dev)) {
		/* Send a character */
		uart->thr = (uint8_t)tx_data[num_tx++];
	}

	return (int)num_tx;
}

static int uart_rda5981a_fifo_read(struct device *dev, uint8_t *rx_data,
				  const int size)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);
	uint8_t num_rx = 0;

	while ((size - num_rx > 0) && serial_readable(dev)) {
		rx_data[num_rx++] = (uint8_t)uart->rbr;
	}

	return num_rx;
}

static void uart_rda5981a_irq_tx_enable(struct device *dev)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);

	uart->ier |= ENABLE_IRQ_TX;
}

static void uart_rda5981a_irq_tx_disable(struct device *dev)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);

	uart->ier &= ~(ENABLE_IRQ_TX);
}

static int uart_rda5981a_irq_tx_ready(struct device *dev)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);

	return (uart->fsr & TX_FIFO_FULL);  /* uart0 not have flow control */
}

static int uart_rda5981a_irq_tx_empty(struct device *dev)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);

	return (uart->fsr & TX_FIFO_EMPTY);
}

static void uart_rda5981a_irq_rx_enable(struct device *dev)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);

	uart->ier |= ENABLE_IRQ_RX;
}

static void uart_rda5981a_irq_rx_disable(struct device *dev)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);

	uart->ier &= ~(ENABLE_IRQ_RX);
}

static int uart_rda5981a_irq_rx_ready(struct device *dev)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);

	return (uart->lsr & RX_FIFO_DATA_RDY);
}

static void uart_rda5981a_irq_err_enable(struct device *dev)
{
}

static void uart_rda5981a_irq_err_disable(struct device *dev)
{
}

static int uart_rda5981a_irq_is_pending(struct device *dev)
{
	return (uart_rda5981a_irq_tx_ready(dev)
				|| uart_rda5981a_irq_rx_ready(dev));
}

static int uart_rda5981a_irq_update(struct device *dev)
{
	return 1;
}

static void uart_rda5981a_irq_callback_set(struct device *dev,
				       uart_irq_callback_t cb)
{
	struct uart_rda5981a_data *data = DEV_DATA(dev);

	data->user_cb = cb;
}

static void uart_rda5981a_isr(void *arg)
{

	struct device *dev = arg;
	struct uart_rda5981a_data *data = DEV_DATA(dev);

	if (data->user_cb) {
		data->user_cb(dev);
	}
}
#endif

static int serial_getc(struct device *dev)
{
	int data;
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);

	while (!serial_readable(dev)) {
		;
	}
	data = uart->rbr;

	return data;
}

static void serial_putc(struct device *dev, int c)
{
	volatile struct uart_rda5981a *uart = UART_STRUCT(dev);

	while (serial_writable(dev)) {
		;
	}
	uart->thr = c;
}

static int uart_rda5981a_poll_in(struct device *dev, unsigned char *c)
{
	*c = (unsigned char)serial_getc(dev);

	return 0;
}

static unsigned char uart_rda5981a_poll_out(struct device *dev,
					unsigned char c)
{
	serial_putc(dev, c);

	return c;
}

static const struct uart_driver_api uart_rda5981a_driver_api = {
	.poll_in = uart_rda5981a_poll_in,
	.poll_out = uart_rda5981a_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_rda5981a_fifo_fill,
	.fifo_read = uart_rda5981a_fifo_read,
	.irq_tx_enable = uart_rda5981a_irq_tx_enable,
	.irq_tx_disable = uart_rda5981a_irq_tx_disable,
	.irq_tx_ready = uart_rda5981a_irq_tx_ready,
	.irq_tx_empty = uart_rda5981a_irq_tx_empty,
	.irq_rx_enable = uart_rda5981a_irq_rx_enable,
	.irq_rx_disable = uart_rda5981a_irq_rx_disable,
	.irq_rx_ready = uart_rda5981a_irq_rx_ready,
	.irq_err_enable = uart_rda5981a_irq_err_enable,
	.irq_err_disable = uart_rda5981a_irq_err_disable,
	.irq_is_pending = uart_rda5981a_irq_is_pending,
	.irq_update = uart_rda5981a_irq_update,
	.irq_callback_set = uart_rda5981a_irq_callback_set,
#endif
};

static struct uart_rda5981a_data uart_rda5981a_dev_data_0 = {
	.uart_index = CONFIG_UART_RDA5981A_PORT_N,/*default uart0 */
	.baud_rate = CONFIG_UART_RDA5981A_PORT_0_BAUD_RATE,
};

static const struct uart_rda5981a_config uart_rda5981a_dev_cfg_0 = {
	.ahb_bus_clk = 80000000UL,
	.uconf = {
		.base = (uint8_t *)RDA_UART0_BASE,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
		.irq_config_func = uart_irq_config_func_0,
#endif
	}
};

DEVICE_AND_API_INIT(uart_rda5981a_0, CONFIG_UART_RDA5981A_PORT_0_NAME,
			&uart_rda5981a_init,
		    &uart_rda5981a_dev_data_0, &uart_rda5981a_dev_cfg_0,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_rda5981a_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_irq_config_func_0(struct device *dev)
{
	IRQ_CONNECT(UART0_IRQ,
		CONFIG_UART_RDA5981A_PORT_0_IRQ_PRI,
		uart_rda5981a_isr, DEVICE_GET(uart_rda5981a_0),
		0);

	irq_enable(UART0_IRQ);
}
#endif

