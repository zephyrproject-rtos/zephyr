/*
 * Copyright (c) 2017 Piotr Mienkowski
 * Copyright (c) 2018 Justin Watson
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief UART driver for Atmel SAM MCU family.
 *
 * Note:
 * - Error handling is not implemented.
 * - The driver works only in polling mode, interrupt mode is not implemented.
 */

#include <errno.h>
#include <sys/__assert.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/uart.h>

/*
 * Verify Kconfig configuration
 */

#if CONFIG_UART_SAM_PORT_0 == 1

#if DT_UART_SAM_PORT_0_BAUD_RATE == 0
#error "DT_UART_SAM_PORT_0_BAUD_RATE has to be bigger than 0"
#endif

#endif

#if CONFIG_UART_SAM_PORT_1 == 1

#if DT_UART_SAM_PORT_1_BAUD_RATE == 0
#error "DT_UART_SAM_PORT_1_BAUD_RATE has to be bigger than 0"
#endif

#endif

#if CONFIG_UART_SAM_PORT_2 == 1

#if DT_UART_SAM_PORT_2_BAUD_RATE == 0
#error "DT_UART_SAM_PORT_2_BAUD_RATE has to be bigger than 0"
#endif

#endif

#if CONFIG_UART_SAM_PORT_3 == 1

#if DT_UART_SAM_PORT_3_BAUD_RATE == 0
#error "DT_UART_SAM_PORT_3_BAUD_RATE has to be bigger than 0"
#endif

#endif

#if CONFIG_UART_SAM_PORT_4 == 1

#if DT_UART_SAM_PORT_4_BAUD_RATE == 0
#error "DT_UART_SAM_PORT_4_BAUD_RATE has to be bigger than 0"
#endif

#endif

/* Device constant configuration parameters */
struct uart_sam_dev_cfg {
	Uart *regs;
	u32_t periph_id;
	struct soc_gpio_pin pin_rx;
	struct soc_gpio_pin pin_tx;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t	irq_config_func;
#endif
};

/* Device run time data */
struct uart_sam_dev_data {
	u32_t baud_rate;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;	/* Interrupt Callback */
	void *irq_cb_data;	/* Interrupt Callback Arg */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define DEV_CFG(dev) \
	((const struct uart_sam_dev_cfg *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_sam_dev_data *const)(dev)->driver_data)


static int baudrate_set(Uart *const uart, u32_t baudrate,
			u32_t mck_freq_hz);


static int uart_sam_init(struct device *dev)
{
	int retval;
	const struct uart_sam_dev_cfg *const cfg = DEV_CFG(dev);
	struct uart_sam_dev_data *const dev_data = DEV_DATA(dev);
	Uart *const uart = cfg->regs;

	/* Enable UART clock in PMC */
	soc_pmc_peripheral_enable(cfg->periph_id);

	/* Connect pins to the peripheral */
	soc_gpio_configure(&cfg->pin_rx);
	soc_gpio_configure(&cfg->pin_tx);

	/* Reset and disable UART */
	uart->UART_CR =   UART_CR_RSTRX | UART_CR_RSTTX
			| UART_CR_RXDIS | UART_CR_TXDIS | UART_CR_RSTSTA;

	/* Disable Interrupts */
	uart->UART_IDR = 0xFFFFFFFF;

	/* 8 bits of data, no parity, 1 stop bit in normal mode,  baud rate
	 * driven by the peripheral clock, UART does not filter the receive line
	 */
	uart->UART_MR =   UART_MR_PAR_NO
			| UART_MR_CHMODE_NORMAL;

	/* Set baud rate */
	retval = baudrate_set(uart, dev_data->baud_rate,
			      SOC_ATMEL_SAM_MCK_FREQ_HZ);
	if (retval != 0) {
		return retval;
	}

	/* Enable receiver and transmitter */
	uart->UART_CR = UART_CR_RXEN | UART_CR_TXEN;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

static int uart_sam_poll_in(struct device *dev, unsigned char *c)
{
	Uart *const uart = DEV_CFG(dev)->regs;

	if (!(uart->UART_SR & UART_SR_RXRDY)) {
		return -EBUSY;
	}

	/* got a character */
	*c = (unsigned char)uart->UART_RHR;

	return 0;
}

static void uart_sam_poll_out(struct device *dev, unsigned char c)
{
	Uart *const uart = DEV_CFG(dev)->regs;

	/* Wait for transmitter to be ready */
	while (!(uart->UART_SR & UART_SR_TXRDY)) {
	}

	/* send a character */
	uart->UART_THR = (u32_t)c;
}

static int uart_sam_err_check(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;
	int errors = 0;

	if (uart->UART_SR & UART_SR_OVRE) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (uart->UART_SR & UART_SR_PARE) {
		errors |= UART_ERROR_PARITY;
	}

	if (uart->UART_SR & UART_SR_FRAME) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

static int baudrate_set(Uart *const uart, u32_t baudrate,
			u32_t mck_freq_hz)
{
	u32_t divisor;

	__ASSERT(baudrate,
		 "baud rate has to be bigger than 0");
	__ASSERT(mck_freq_hz/16U >= baudrate,
		 "MCK frequency is too small to set required baud rate");

	divisor = mck_freq_hz / 16U / baudrate;

	if (divisor > 0xFFFF) {
		return -EINVAL;
	}

	uart->UART_BRGR = UART_BRGR_CD(divisor);

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_sam_fifo_fill(struct device *dev, const uint8_t *tx_data,
			      int size)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	/* Wait for transmitter to be ready. */
	while ((uart->UART_SR & UART_SR_TXRDY) == 0) {
	}

	uart->UART_THR = *tx_data;

	return 1;
}

static int uart_sam_fifo_read(struct device *dev, uint8_t *rx_data,
			      const int size)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;
	int bytes_read;

	bytes_read = 0;

	while (bytes_read < size) {
		if (uart->UART_SR & UART_SR_RXRDY) {
			rx_data[bytes_read] = uart->UART_RHR;
			bytes_read++;
		} else {
			break;
		}
	}

	return bytes_read;
}

static void uart_sam_irq_tx_enable(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	uart->UART_IER = UART_IER_TXRDY;
}

static void uart_sam_irq_tx_disable(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	uart->UART_IDR = UART_IDR_TXRDY;
}

static int uart_sam_irq_tx_ready(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	return (uart->UART_SR & UART_SR_TXRDY);
}

static void uart_sam_irq_rx_enable(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	uart->UART_IER = UART_IER_RXRDY;
}

static void uart_sam_irq_rx_disable(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	uart->UART_IDR = UART_IDR_RXRDY;
}

static int uart_sam_irq_tx_complete(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	return !(uart->UART_SR & UART_SR_TXRDY);
}

static int uart_sam_irq_rx_ready(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	return (uart->UART_SR & UART_SR_RXRDY);
}

static void uart_sam_irq_err_enable(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	uart->UART_IER = UART_IER_OVRE | UART_IER_FRAME | UART_IER_PARE;
}

static void uart_sam_irq_err_disable(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	uart->UART_IDR = UART_IDR_OVRE | UART_IDR_FRAME | UART_IDR_PARE;
}

static int uart_sam_irq_is_pending(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	return (uart->UART_IMR & (UART_IMR_TXRDY | UART_IMR_RXRDY)) &
		(uart->UART_SR & (UART_SR_TXRDY | UART_SR_RXRDY));
}

static int uart_sam_irq_update(struct device *dev)
{
	volatile Uart * const uart = DEV_CFG(dev)->regs;

	return (uart->UART_SR & UART_SR_TXEMPTY);
}

static void uart_sam_irq_callback_set(struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_sam_dev_data *const dev_data = DEV_DATA(dev);

	dev_data->irq_cb = cb;
	dev_data->irq_cb_data = cb_data;
}

static void uart_sam_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_sam_dev_data *const dev_data = DEV_DATA(dev);

	if (dev_data->irq_cb) {
		dev_data->irq_cb(dev_data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_sam_driver_api = {
	.poll_in = uart_sam_poll_in,
	.poll_out = uart_sam_poll_out,
	.err_check = uart_sam_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_sam_fifo_fill,
	.fifo_read = uart_sam_fifo_read,
	.irq_tx_enable = uart_sam_irq_tx_enable,
	.irq_tx_disable = uart_sam_irq_tx_disable,
	.irq_tx_ready = uart_sam_irq_tx_ready,
	.irq_rx_enable = uart_sam_irq_rx_enable,
	.irq_rx_disable = uart_sam_irq_rx_disable,
	.irq_tx_complete = uart_sam_irq_tx_complete,
	.irq_rx_ready = uart_sam_irq_rx_ready,
	.irq_err_enable = uart_sam_irq_err_enable,
	.irq_err_disable = uart_sam_irq_err_disable,
	.irq_is_pending = uart_sam_irq_is_pending,
	.irq_update = uart_sam_irq_update,
	.irq_callback_set = uart_sam_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */
};

/* UART0 */

#ifdef CONFIG_UART_SAM_PORT_0

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/* Forward declare function */
static void uart0_sam_irq_config_func(struct device *port);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_sam_dev_cfg uart0_sam_config = {
	.regs = UART0,
	.periph_id = ID_UART0,
	.pin_rx = PIN_UART0_RXD,
	.pin_tx = PIN_UART0_TXD,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart0_sam_irq_config_func,
#endif
};

static struct uart_sam_dev_data uart0_sam_data = {
	.baud_rate = DT_UART_SAM_PORT_0_BAUD_RATE,
};

DEVICE_AND_API_INIT(uart0_sam, DT_UART_SAM_PORT_0_NAME, &uart_sam_init,
		    &uart0_sam_data, &uart0_sam_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uart_sam_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart0_sam_irq_config_func(struct device *port)
{
	IRQ_CONNECT(DT_UART_SAM_PORT_0_IRQ,
		    DT_UART_SAM_PORT_0_IRQ_PRIO,
		    uart_sam_isr,
		    DEVICE_GET(uart0_sam), 0);
	irq_enable(DT_UART_SAM_PORT_0_IRQ);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#endif /* CONFIG_UART_SAM_PORT_0 */

/* UART1 */

#ifdef CONFIG_UART_SAM_PORT_1

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/* Forward declare function */
static void uart1_sam_irq_config_func(struct device *port);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_sam_dev_cfg uart1_sam_config = {
	.regs = UART1,
	.periph_id = ID_UART1,
	.pin_rx = PIN_UART1_RXD,
	.pin_tx = PIN_UART1_TXD,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart1_sam_irq_config_func,
#endif
};

static struct uart_sam_dev_data uart1_sam_data = {
	.baud_rate = DT_UART_SAM_PORT_1_BAUD_RATE,
};

DEVICE_AND_API_INIT(uart1_sam, DT_UART_SAM_PORT_1_NAME, &uart_sam_init,
		    &uart1_sam_data, &uart1_sam_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uart_sam_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart1_sam_irq_config_func(struct device *port)
{
	IRQ_CONNECT(DT_UART_SAM_PORT_1_IRQ,
		    DT_UART_SAM_PORT_1_IRQ_PRIO,
		    uart_sam_isr,
		    DEVICE_GET(uart1_sam), 0);
	irq_enable(DT_UART_SAM_PORT_1_IRQ);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#endif /* CONFIG_UART_SAM_PORT_1 */

/* UART2 */

#ifdef CONFIG_UART_SAM_PORT_2

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/* Forward declare function */
static void uart2_sam_irq_config_func(struct device *port);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_sam_dev_cfg uart2_sam_config = {
	.regs = UART2,
	.periph_id = ID_UART2,
	.pin_rx = PIN_UART2_RXD,
	.pin_tx = PIN_UART2_TXD,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart2_sam_irq_config_func,
#endif
};

static struct uart_sam_dev_data uart2_sam_data = {
	.baud_rate = DT_UART_SAM_PORT_2_BAUD_RATE,
};

DEVICE_AND_API_INIT(uart2_sam, DT_UART_SAM_PORT_2_NAME, &uart_sam_init,
		    &uart2_sam_data, &uart2_sam_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uart_sam_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart2_sam_irq_config_func(struct device *port)
{
	IRQ_CONNECT(DT_UART_SAM_PORT_2_IRQ,
		    DT_UART_SAM_PORT_2_IRQ_PRIO,
		    uart_sam_isr,
		    DEVICE_GET(uart2_sam), 0);
	irq_enable(DT_UART_SAM_PORT_2_IRQ);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#endif

/* UART3 */

#ifdef CONFIG_UART_SAM_PORT_3

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/* Forward declare function */
static void uart3_sam_irq_config_func(struct device *port);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_sam_dev_cfg uart3_sam_config = {
	.regs = UART3,
	.periph_id = ID_UART3,
	.pin_rx = PIN_UART3_RXD,
	.pin_tx = PIN_UART3_TXD,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart3_sam_irq_config_func,
#endif
};

static struct uart_sam_dev_data uart3_sam_data = {
	.baud_rate = DT_UART_SAM_PORT_3_BAUD_RATE,
};

DEVICE_AND_API_INIT(uart3_sam, DT_UART_SAM_PORT_3_NAME, &uart_sam_init,
		    &uart3_sam_data, &uart3_sam_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uart_sam_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart3_sam_irq_config_func(struct device *port)
{
	IRQ_CONNECT(DT_UART_SAM_PORT_3_IRQ,
		    DT_UART_SAM_PORT_3_IRQ_PRIO,
		    uart_sam_isr,
		    DEVICE_GET(uart3_sam), 0);
	irq_enable(DT_UART_SAM_PORT_3_IRQ);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#endif

/* UART4 */

#ifdef CONFIG_UART_SAM_PORT_4

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/* Forward declare function */
static void uart4_sam_irq_config_func(struct device *port);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_sam_dev_cfg uart4_sam_config = {
	.regs = UART4,
	.periph_id = ID_UART4,
	.pin_rx = PIN_UART4_RXD,
	.pin_tx = PIN_UART4_TXD,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uart4_sam_irq_config_func,
#endif
};

static struct uart_sam_dev_data uart4_sam_data = {
	.baud_rate = DT_UART_SAM_PORT_4_BAUD_RATE,
};

DEVICE_AND_API_INIT(uart4_sam, DT_UART_SAM_PORT_4_NAME, &uart_sam_init,
		    &uart4_sam_data, &uart4_sam_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uart_sam_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart4_sam_irq_config_func(struct device *port)
{
	IRQ_CONNECT(DT_UART_SAM_PORT_4_IRQ,
		    DT_UART_SAM_PORT_4_IRQ_PRIO,
		    uart_sam_isr,
		    DEVICE_GET(uart4_sam), 0);
	irq_enable(DT_UART_SAM_PORT_4_IRQ);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#endif
