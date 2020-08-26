/*
 * Copyright (c) 2018 Justin Watson
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_usart

/** @file
 * @brief USART driver for Atmel SAM MCU family.
 *
 * Note:
 * - Only basic USART features sufficient to support printf functionality
 *   are currently implemented.
 */

#include <errno.h>
#include <sys/__assert.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/uart.h>

/* Device constant configuration parameters */
struct usart_sam_dev_cfg {
	Usart *regs;
	uint32_t periph_id;
	struct soc_gpio_pin pin_rx;
	struct soc_gpio_pin pin_tx;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t	irq_config_func;
#endif
};

/* Device run time data */
struct usart_sam_dev_data {
	uint32_t baud_rate;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;	/* Interrupt Callback */
	void *cb_data;	/* Interrupt Callback Arg */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define DEV_CFG(dev) \
	((const struct usart_sam_dev_cfg *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct usart_sam_dev_data *const)(dev)->data)


static int baudrate_set(Usart *const usart, uint32_t baudrate,
			uint32_t mck_freq_hz);


static int usart_sam_init(struct device *dev)
{
	int retval;
	const struct usart_sam_dev_cfg *const cfg = DEV_CFG(dev);
	struct usart_sam_dev_data *const dev_data = DEV_DATA(dev);
	Usart *const usart = cfg->regs;

	/* Enable USART clock in PMC */
	soc_pmc_peripheral_enable(cfg->periph_id);

	/* Connect pins to the peripheral */
	soc_gpio_configure(&cfg->pin_rx);
	soc_gpio_configure(&cfg->pin_tx);

	/* Reset and disable USART */
	usart->US_CR =   US_CR_RSTRX | US_CR_RSTTX
		       | US_CR_RXDIS | US_CR_TXDIS | US_CR_RSTSTA;

	/* Disable Interrupts */
	usart->US_IDR = 0xFFFFFFFF;

	/* 8 bits of data, no parity, 1 stop bit in normal mode */
	usart->US_MR =   US_MR_NBSTOP_1_BIT
		       | US_MR_PAR_NO
		       | US_MR_CHRL_8_BIT
		       | US_MR_USCLKS_MCK
		       | US_MR_CHMODE_NORMAL;

	/* Set baud rate */
	retval = baudrate_set(usart, dev_data->baud_rate,
			      SOC_ATMEL_SAM_MCK_FREQ_HZ);
	if (retval != 0) {
		return retval;
	}

	/* Enable receiver and transmitter */
	usart->US_CR = US_CR_RXEN | US_CR_TXEN;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

static int usart_sam_poll_in(struct device *dev, unsigned char *c)
{
	Usart *const usart = DEV_CFG(dev)->regs;

	if (!(usart->US_CSR & US_CSR_RXRDY)) {
		return -EBUSY;
	}

	/* got a character */
	*c = (unsigned char)usart->US_RHR;

	return 0;
}

static void usart_sam_poll_out(struct device *dev, unsigned char c)
{
	Usart *const usart = DEV_CFG(dev)->regs;

	/* Wait for transmitter to be ready */
	while (!(usart->US_CSR & US_CSR_TXRDY)) {
	}

	/* send a character */
	usart->US_THR = (uint32_t)c;
}

static int usart_sam_err_check(struct device *dev)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;
	int errors = 0;

	if (usart->US_CSR & US_CSR_OVRE) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (usart->US_CSR & US_CSR_PARE) {
		errors |= UART_ERROR_PARITY;
	}

	if (usart->US_CSR & US_CSR_FRAME) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

static int baudrate_set(Usart *const usart, uint32_t baudrate,
			uint32_t mck_freq_hz)
{
	uint32_t divisor;

	__ASSERT(baudrate,
		 "baud rate has to be bigger than 0");
	__ASSERT(mck_freq_hz/16U >= baudrate,
		 "MCK frequency is too small to set required baud rate");

	divisor = mck_freq_hz / 16U / baudrate;

	if (divisor > 0xFFFF) {
		return -EINVAL;
	}

	usart->US_BRGR = US_BRGR_CD(divisor);

	return 0;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

static int usart_sam_fifo_fill(struct device *dev, const uint8_t *tx_data,
			       int size)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;

	/* Wait for transmitter to be ready. */
	while ((usart->US_CSR & US_CSR_TXRDY) == 0) {
	}

	usart->US_THR = *tx_data;

	return 1;
}

static int usart_sam_fifo_read(struct device *dev, uint8_t *rx_data,
			       const int size)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;
	int bytes_read;

	bytes_read = 0;

	while (bytes_read < size) {
		if (usart->US_CSR & US_CSR_RXRDY) {
			rx_data[bytes_read] = usart->US_RHR;
			bytes_read++;
		} else {
			break;
		}
	}

	return bytes_read;
}

static void usart_sam_irq_tx_enable(struct device *dev)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;

	usart->US_IER = US_IER_TXRDY;
}

static void usart_sam_irq_tx_disable(struct device *dev)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;

	usart->US_IDR = US_IDR_TXRDY;
}

static int usart_sam_irq_tx_ready(struct device *dev)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;

	return (usart->US_CSR & US_CSR_TXRDY);
}

static void usart_sam_irq_rx_enable(struct device *dev)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;

	usart->US_IER = US_IER_RXRDY;
}

static void usart_sam_irq_rx_disable(struct device *dev)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;

	usart->US_IDR = US_IDR_RXRDY;
}

static int usart_sam_irq_tx_complete(struct device *dev)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;

	return !(usart->US_CSR & US_CSR_TXRDY);
}

static int usart_sam_irq_rx_ready(struct device *dev)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;

	return (usart->US_CSR & US_CSR_RXRDY);
}

static void usart_sam_irq_err_enable(struct device *dev)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;

	usart->US_IER = US_IER_OVRE | US_IER_FRAME | US_IER_PARE;
}

static void usart_sam_irq_err_disable(struct device *dev)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;

	usart->US_IDR = US_IDR_OVRE | US_IDR_FRAME | US_IDR_PARE;
}

static int usart_sam_irq_is_pending(struct device *dev)
{
	volatile Usart * const usart = DEV_CFG(dev)->regs;

	return (usart->US_IMR & (US_IMR_TXRDY | US_IMR_RXRDY)) &
		(usart->US_CSR & (US_CSR_TXRDY | US_CSR_RXRDY));
}

static int usart_sam_irq_update(struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void usart_sam_irq_callback_set(struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct usart_sam_dev_data *const dev_data = DEV_DATA(dev);

	dev_data->irq_cb = cb;
	dev_data->cb_data = cb_data;
}

static void usart_sam_isr(void *arg)
{
	struct device *dev = arg;
	struct usart_sam_dev_data *const dev_data = DEV_DATA(dev);

	if (dev_data->irq_cb) {
		dev_data->irq_cb(dev, dev_data->cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api usart_sam_driver_api = {
	.poll_in = usart_sam_poll_in,
	.poll_out = usart_sam_poll_out,
	.err_check = usart_sam_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = usart_sam_fifo_fill,
	.fifo_read = usart_sam_fifo_read,
	.irq_tx_enable = usart_sam_irq_tx_enable,
	.irq_tx_disable = usart_sam_irq_tx_disable,
	.irq_tx_ready = usart_sam_irq_tx_ready,
	.irq_rx_enable = usart_sam_irq_rx_enable,
	.irq_rx_disable = usart_sam_irq_rx_disable,
	.irq_tx_complete = usart_sam_irq_tx_complete,
	.irq_rx_ready = usart_sam_irq_rx_ready,
	.irq_err_enable = usart_sam_irq_err_enable,
	.irq_err_disable = usart_sam_irq_err_disable,
	.irq_is_pending = usart_sam_irq_is_pending,
	.irq_update = usart_sam_irq_update,
	.irq_callback_set = usart_sam_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define USART_SAM_DECLARE_CFG(n, IRQ_FUNC_INIT)				\
	static const struct usart_sam_dev_cfg usart##n##_sam_config = {	\
		.regs = (Usart *)DT_INST_REG_ADDR(n),			\
		.periph_id = DT_INST_PROP(n, peripheral_id),		\
									\
		.pin_rx = ATMEL_SAM_DT_PIN(n, 0),			\
		.pin_tx = ATMEL_SAM_DT_PIN(n, 1),			\
									\
		IRQ_FUNC_INIT						\
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define USART_SAM_CONFIG_FUNC(n)					\
	static void usart##n##_sam_irq_config_func(struct device *port)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    usart_sam_isr,				\
			    DEVICE_GET(usart##n##_sam), 0);		\
		irq_enable(DT_INST_IRQN(n));				\
	}
#define USART_SAM_IRQ_CFG_FUNC_INIT(n)					\
	.irq_config_func = usart##n##_sam_irq_config_func
#define USART_SAM_INIT_CFG(n)						\
	USART_SAM_DECLARE_CFG(n, USART_SAM_IRQ_CFG_FUNC_INIT(n))
#else
#define USART_SAM_CONFIG_FUNC(n)
#define USART_SAM_IRQ_CFG_FUNC_INIT
#define USART_SAM_INIT_CFG(n)						\
	USART_SAM_DECLARE_CFG(n, USART_SAM_IRQ_CFG_FUNC_INIT)
#endif

#define USART_SAM_INIT(n)						\
	static struct usart_sam_dev_data usart##n##_sam_data = {	\
		.baud_rate = DT_INST_PROP(n, current_speed),		\
	};								\
									\
	static const struct usart_sam_dev_cfg usart##n##_sam_config;	\
									\
	DEVICE_AND_API_INIT(usart##n##_sam, DT_INST_LABEL(n),		\
			    &usart_sam_init, &usart##n##_sam_data,	\
			    &usart##n##_sam_config, PRE_KERNEL_1,	\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &usart_sam_driver_api);			\
									\
	USART_SAM_CONFIG_FUNC(n)					\
									\
	USART_SAM_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(USART_SAM_INIT)
