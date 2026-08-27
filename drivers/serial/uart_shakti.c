/*
 * Copyright (c) 2026 RISE Lab, IIT Madras
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT shakti_uart

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

/* Status register bits */
#define STS_TX_EMPTY		BIT(0)	/* transmitter empty */
#define STS_TX_FULL			BIT(1)	/* transmitter full */
#define STS_RX_NOT_EMPTY	BIT(2)	/* receiver has data */
#define STS_RX_FULL			BIT(3)	/* receiver full */

/* Interrupt enable register bits */
#define IEN_TX_EMPTY		BIT(0)	/* fire when TX can accept data */
#define IEN_RX_NOT_EMPTY	BIT(2)	/* fire when a byte has arrived */

struct uart_shakti_regs {
	uint16_t baud;			/* baud divisor */
	uint16_t _reserved0;
	uint32_t tx_reg;		/* transmit data */
	uint32_t rx_reg;		/* receive data */
	uint16_t status;		/* status flags */
	uint16_t _reserved1;
	uint16_t delay;			/* tx delay */
	uint16_t _reserved2;
	uint16_t control;		/* control */
	uint16_t _reserved3;
	uint16_t ien;			/* interrupt enable */
};

struct uart_shakti_device_config {
	uintptr_t base;
	uint32_t clk_freq;
	uint32_t baud_rate;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
struct uart_shakti_data {
	uart_irq_callback_user_data_t callback;
	void *cb_data;
};
#endif

#define DEV_UART(dev) \
	((volatile struct uart_shakti_regs *) \
	 ((const struct uart_shakti_device_config *)(dev)->config)->base)

static void uart_shakti_poll_out(const struct device *dev, unsigned char c)
{
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);
	/* wait for room in the TX fifo */
	while (uart->status & STS_TX_FULL) {
	}

	uart->tx_reg = (uint32_t)c;
}

static int uart_shakti_poll_in(const struct device *dev, unsigned char *c)
{
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);

	if (uart->status & STS_RX_NOT_EMPTY) {
		*c = (unsigned char)(uart->rx_reg & 0xFF);
		return 0;
	}

	return -1;
}

static int uart_shakti_init(const struct device *dev)
{
	const struct uart_shakti_device_config *cfg = dev->config;
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);

	if (cfg->baud_rate == 0) {
		return -EINVAL;
	}
	/* baud divisor, UART oversamples each bit 16x */
	uart->baud = (uint16_t)(cfg->clk_freq / (16 * cfg->baud_rate));

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config_func(dev);
#endif

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_shakti_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);
	int i = 0;

	while (i < size && !(uart->status & STS_TX_FULL)) {
		uart->tx_reg = (uint32_t)tx_data[i];
		i++;
	}

	return i;
}

static int uart_shakti_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);
	int i = 0;

	while (i < size && (uart->status & STS_RX_NOT_EMPTY)) {
		rx_data[i] = (uint8_t)(uart->rx_reg & 0xFF);
		i++;
	}

	return i;
}

static void uart_shakti_irq_tx_enable(const struct device *dev)
{
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);

	uart->ien |= IEN_TX_EMPTY;
}

static void uart_shakti_irq_tx_disable(const struct device *dev)
{
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);

	uart->ien &= ~IEN_TX_EMPTY;
}

static int uart_shakti_irq_tx_ready(const struct device *dev)
{
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);

	return !(uart->status & STS_TX_FULL);
}

static void uart_shakti_irq_rx_enable(const struct device *dev)
{
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);

	uart->ien |= IEN_RX_NOT_EMPTY;
}

static void uart_shakti_irq_rx_disable(const struct device *dev)
{
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);

	uart->ien &= ~IEN_RX_NOT_EMPTY;
}

static int uart_shakti_irq_rx_ready(const struct device *dev)
{
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);

	return !!(uart->status & STS_RX_NOT_EMPTY);
}

static int uart_shakti_irq_is_pending(const struct device *dev)
{
	volatile struct uart_shakti_regs *uart = DEV_UART(dev);

	if (uart->status & STS_RX_NOT_EMPTY) {
		return 1;
	}

	if ((uart->ien & IEN_TX_EMPTY) && !(uart->status & STS_TX_FULL)) {
		return 1;
	}

	return 0;
}

static void uart_shakti_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_shakti_irq_callback_set(const struct device *dev,
					 uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct uart_shakti_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_shakti_isr(const struct device *dev)
{
	struct uart_shakti_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_shakti_driver_api) = {
	.poll_in = uart_shakti_poll_in,
	.poll_out = uart_shakti_poll_out,
	.err_check = NULL,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_shakti_fifo_fill,
	.fifo_read = uart_shakti_fifo_read,
	.irq_tx_enable = uart_shakti_irq_tx_enable,
	.irq_tx_disable = uart_shakti_irq_tx_disable,
	.irq_tx_ready = uart_shakti_irq_tx_ready,
	.irq_rx_enable = uart_shakti_irq_rx_enable,
	.irq_rx_disable = uart_shakti_irq_rx_disable,
	.irq_rx_ready = uart_shakti_irq_rx_ready,
	.irq_is_pending = uart_shakti_irq_is_pending,
	.irq_update = uart_shakti_irq_update,
	.irq_callback_set = uart_shakti_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_SHAKTI_IRQ_CONFIG(n)                                                 \
	static void uart_shakti_irq_config_##n(const struct device *dev)              \
	{                                                                             \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                    \
			    uart_shakti_isr, DEVICE_DT_INST_GET(n), 0);                       \
		irq_enable(DT_INST_IRQN(n));                                              \
	}
#define UART_SHAKTI_IRQ_FUNC_INIT(n) .irq_config_func = uart_shakti_irq_config_##n,
#define UART_SHAKTI_DATA(n) static struct uart_shakti_data uart_shakti_data_##n;
#define UART_SHAKTI_DATA_PTR(n) (&uart_shakti_data_##n)
#else
#define UART_SHAKTI_IRQ_CONFIG(n)
#define UART_SHAKTI_IRQ_FUNC_INIT(n)
#define UART_SHAKTI_DATA(n)
#define UART_SHAKTI_DATA_PTR(n) NULL
#endif

#define UART_SHAKTI_INIT(n)                                                      \
	UART_SHAKTI_DATA(n)                                                          \
	UART_SHAKTI_IRQ_CONFIG(n)                                                    \
	static const struct uart_shakti_device_config uart_shakti_cfg_##n = {        \
		.base = DT_INST_REG_ADDR(n),                                             \
		.clk_freq = DT_INST_PROP(n, clock_frequency),                            \
		.baud_rate = DT_INST_PROP(n, current_speed),                             \
		UART_SHAKTI_IRQ_FUNC_INIT(n)                                             \
	};                                                                           \
	DEVICE_DT_INST_DEFINE(n, uart_shakti_init, NULL,                             \
			      UART_SHAKTI_DATA_PTR(n), &uart_shakti_cfg_##n,                 \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,                     \
			      &uart_shakti_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_SHAKTI_INIT)
