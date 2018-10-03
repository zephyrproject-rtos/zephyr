/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <errno.h>
#include <init.h>
#include <misc/__assert.h>
#include <soc.h>
#include <uart.h>
#include <board.h>

/* Device constant configuration parameters */
struct uart_sam0_dev_cfg {
	SercomUsart *regs;
	u32_t baudrate;
	u32_t pads;
	u32_t pm_apbcmask;
	u16_t gclk_clkctrl_id;
#if CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(struct device *dev);
#endif
};

/* Device run time data */
struct uart_sam0_dev_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;
	void *cb_data;
#endif
};

#define DEV_CFG(dev) \
	((const struct uart_sam0_dev_cfg *const)(dev)->config->config_info)
#define DEV_DATA(dev) ((struct uart_sam0_dev_data * const)(dev)->driver_data)

static void wait_synchronization(SercomUsart *const usart)
{
#if defined(SERCOM_USART_SYNCBUSY_MASK)
	/* SYNCBUSY is a register */
	while ((usart->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_MASK) != 0) {
	}
#elif defined(SERCOM_USART_STATUS_SYNCBUSY)
	/* SYNCBUSY is a bit */
	while ((usart->STATUS.reg & SERCOM_USART_STATUS_SYNCBUSY) != 0) {
	}
#else
#error Unsupported device
#endif
}

static int uart_sam0_set_baudrate(SercomUsart *const usart, u32_t baudrate,
				  u32_t clk_freq_hz)
{
	u64_t tmp;
	u16_t baud;

	tmp = (u64_t)baudrate << 20;
	tmp = (tmp + (clk_freq_hz >> 1)) / clk_freq_hz;

	/* Verify that the calculated result is within range */
	if (tmp < 1 || tmp > UINT16_MAX) {
		return -ERANGE;
	}

	baud = 65536 - (u16_t)tmp;
	usart->BAUD.reg = baud;
	wait_synchronization(usart);

	return 0;
}

static int uart_sam0_init(struct device *dev)
{
	int retval;
	const struct uart_sam0_dev_cfg *const cfg = DEV_CFG(dev);
	SercomUsart *const usart = cfg->regs;

	/* Enable the GCLK */
	GCLK->CLKCTRL.reg =
	    cfg->gclk_clkctrl_id | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN;

	/* Enable SERCOM clock in PM */
	PM->APBCMASK.reg |= cfg->pm_apbcmask;

	/* Disable all USART interrupts */
	usart->INTENCLR.reg = SERCOM_USART_INTENCLR_MASK;
	wait_synchronization(usart);

	/* 8 bits of data, no parity, 1 stop bit in normal mode */
	usart->CTRLA.reg =
	    cfg->pads |
	    /* Internal clock */
	    SERCOM_USART_CTRLA_MODE_USART_INT_CLK
#if defined(SERCOM_USART_CTRLA_SAMPR)
	    /* 16x oversampling with arithmetic baud rate generation */
	    | SERCOM_USART_CTRLA_SAMPR(0)
#endif
	    | SERCOM_USART_CTRLA_FORM(0) |
	    SERCOM_USART_CTRLA_CPOL | SERCOM_USART_CTRLA_DORD;
	wait_synchronization(usart);

	/* Enable receiver and transmitter */
	usart->CTRLB.reg = SERCOM_SPI_CTRLB_CHSIZE(0) |
			   SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN;
	wait_synchronization(usart);

	retval = uart_sam0_set_baudrate(usart, cfg->baudrate,
					SOC_ATMEL_SAM0_GCLK0_FREQ_HZ);
	if (retval != 0) {
		return retval;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config_func(dev);
#endif

	usart->CTRLA.bit.ENABLE = 1;
	wait_synchronization(usart);

	return 0;
}

static int uart_sam0_poll_in(struct device *dev, unsigned char *c)
{
	SercomUsart *const usart = DEV_CFG(dev)->regs;

	if (!usart->INTFLAG.bit.RXC) {
		return -EBUSY;
	}

	*c = (unsigned char)usart->DATA.reg;
	return 0;
}

static unsigned char uart_sam0_poll_out(struct device *dev, unsigned char c)
{
	SercomUsart *const usart = DEV_CFG(dev)->regs;

	while (!usart->INTFLAG.bit.DRE) {
	}

	/* send a character */
	usart->DATA.reg = c;
	return c;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

static void uart_sam0_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);

	if (dev_data->cb) {
		dev_data->cb(dev_data->cb_data);
	}
}

static int uart_sam0_fifo_fill(struct device *dev, const u8_t *tx_data, int len)
{
	SercomUsart *regs = DEV_CFG(dev)->regs;

	if (regs->INTFLAG.bit.DRE && len >= 1) {
		regs->DATA.reg = tx_data[0];
		return 1;
	} else {
		return 0;
	}
}

static void uart_sam0_irq_tx_enable(struct device *dev)
{
	SercomUsart *regs = DEV_CFG(dev)->regs;

	regs->INTENSET.reg = SERCOM_USART_INTENCLR_DRE;
}

static void uart_sam0_irq_tx_disable(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	regs->INTENCLR.reg = SERCOM_USART_INTENCLR_DRE;
}

static int uart_sam0_irq_tx_ready(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	return regs->INTFLAG.bit.DRE != 0;
}

static void uart_sam0_irq_rx_enable(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	regs->INTENSET.reg = SERCOM_USART_INTENSET_RXC;
}

static void uart_sam0_irq_rx_disable(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	regs->INTENCLR.reg = SERCOM_USART_INTENCLR_RXC;
}

static int uart_sam0_irq_rx_ready(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	return regs->INTFLAG.bit.RXC != 0;
}

static int uart_sam0_fifo_read(struct device *dev, u8_t *rx_data,
			       const int size)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	if (regs->INTFLAG.bit.RXC) {
		u8_t ch = regs->DATA.reg;

		if (size >= 1) {
			*rx_data = ch;
			return 1;
		} else {
			return -EINVAL;
		}
	}
	return 0;
}

static int uart_sam0_irq_is_pending(struct device *dev)
{
	SercomUsart *const regs = DEV_CFG(dev)->regs;

	return (regs->INTENSET.reg & regs->INTFLAG.reg) != 0;
}

static int uart_sam0_irq_update(struct device *dev) { return 1; }

static void uart_sam0_irq_callback_set(struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct uart_sam0_dev_data *const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}
#endif

static const struct uart_driver_api uart_sam0_driver_api = {
	.poll_in = uart_sam0_poll_in,
	.poll_out = uart_sam0_poll_out,
#if CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_sam0_fifo_fill,
	.fifo_read = uart_sam0_fifo_read,
	.irq_tx_enable = uart_sam0_irq_tx_enable,
	.irq_tx_disable = uart_sam0_irq_tx_disable,
	.irq_tx_ready = uart_sam0_irq_tx_ready,
	.irq_rx_enable = uart_sam0_irq_rx_enable,
	.irq_rx_disable = uart_sam0_irq_rx_disable,
	.irq_rx_ready = uart_sam0_irq_rx_ready,
	.irq_is_pending = uart_sam0_irq_is_pending,
	.irq_update = uart_sam0_irq_update,
	.irq_callback_set = uart_sam0_irq_callback_set,
#endif
};

#if CONFIG_UART_INTERRUPT_DRIVEN
#define UART_SAM0_IRQ_HANDLER_DECL(n)					\
static void uart_sam0_irq_config_##n(struct device *dev)
#define UART_SAM0_IRQ_HANDLER_FUNC(n)					\
	.irq_config_func = uart_sam0_irq_config_##n,
#define UART_SAM0_IRQ_HANDLER(n)					\
static void uart_sam0_irq_config_##n(struct device *dev)		\
{									\
	IRQ_CONNECT(CONFIG_UART_SAM0_SERCOM##n##_IRQ,			\
		    CONFIG_UART_SAM0_SERCOM##n##_IRQ_PRIORITY,		\
		    uart_sam0_isr, DEVICE_GET(uart_sam0_##n),		\
		    0);							\
	irq_enable(CONFIG_UART_SAM0_SERCOM##n##_IRQ);			\
}
#else
#define UART_SAM0_IRQ_HANDLER_DECL(n)
#define UART_SAM0_IRQ_HANDLER_FUNC(n)
#define UART_SAM0_IRQ_HANDLER(n)
#endif

#define UART_SAM0_CONFIG_DEFN(n)					       \
static const struct uart_sam0_dev_cfg uart_sam0_config_##n = {		       \
	.regs = (SercomUsart *)CONFIG_UART_SAM0_SERCOM##n##_BASE_ADDRESS,      \
	.baudrate = CONFIG_UART_SAM0_SERCOM##n##_CURRENT_SPEED,		       \
	.pm_apbcmask = PM_APBCMASK_SERCOM##n,				       \
	.gclk_clkctrl_id = GCLK_CLKCTRL_ID_SERCOM##n##_CORE,		       \
	.pads = CONFIG_UART_SAM0_SERCOM##n##_PADS,			       \
	UART_SAM0_IRQ_HANDLER_FUNC(n)					       \
}

#define UART_SAM0_DEVICE_INIT(n)					       \
static struct uart_sam0_dev_data uart_sam0_data_##n;			       \
UART_SAM0_IRQ_HANDLER_DECL(n);						       \
UART_SAM0_CONFIG_DEFN(n);						       \
DEVICE_AND_API_INIT(uart_sam0_##n, CONFIG_UART_SAM0_SERCOM##n##_LABEL,	       \
		    uart_sam0_init, &uart_sam0_data_##n,		       \
		    &uart_sam0_config_##n, PRE_KERNEL_1,		       \
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			       \
		    &uart_sam0_driver_api);				       \
UART_SAM0_IRQ_HANDLER(n)

#if CONFIG_UART_SAM0_SERCOM0_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(0)
#endif

#if CONFIG_UART_SAM0_SERCOM1_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(1)
#endif

#if CONFIG_UART_SAM0_SERCOM2_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(2)
#endif

#if CONFIG_UART_SAM0_SERCOM3_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(3)
#endif

#if CONFIG_UART_SAM0_SERCOM4_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(4)
#endif

#if CONFIG_UART_SAM0_SERCOM5_BASE_ADDRESS
UART_SAM0_DEVICE_INIT(5)
#endif
