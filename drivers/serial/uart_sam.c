/*
 * Copyright (c) 2017 Piotr Mienkowski
 * Copyright (c) 2018 Justin Watson
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_uart

/** @file
 * @brief UART driver for Atmel SAM MCU family.
 *
 * Note:
 * - Error handling is not implemented.
 * - The driver works only in polling mode, interrupt mode is not implemented.
 */

#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>

/* Device constant configuration parameters */
struct uart_sam_dev_cfg {
	Uart *regs;
	uint32_t periph_id;
	const struct pinctrl_dev_config *pcfg;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t	irq_config_func;
#endif
};

/* Device run time data */
struct uart_sam_dev_data {
	uint32_t baud_rate;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;	/* Interrupt Callback */
	void *irq_cb_data;	/* Interrupt Callback Arg */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_sam_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	Uart * const uart = cfg->regs;

	if (!(uart->UART_SR & UART_SR_RXRDY)) {
		return -EBUSY;
	}

	/* got a character */
	*c = (unsigned char)uart->UART_RHR;

	return 0;
}

static void uart_sam_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	Uart * const uart = cfg->regs;

	/* Wait for transmitter to be ready */
	while (!(uart->UART_SR & UART_SR_TXRDY)) {
	}

	/* send a character */
	uart->UART_THR = (uint32_t)c;
}

static int uart_sam_err_check(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;
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

static int uart_sam_baudrate_set(const struct device *dev, uint32_t baudrate)
{
	struct uart_sam_dev_data *const dev_data = dev->data;

	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	uint32_t divisor;

	__ASSERT(baudrate,
		 "baud rate has to be bigger than 0");
	__ASSERT(SOC_ATMEL_SAM_MCK_FREQ_HZ/16U >= baudrate,
		 "MCK frequency is too small to set required baud rate");

	divisor = SOC_ATMEL_SAM_MCK_FREQ_HZ / 16U / baudrate;

	if (divisor > 0xFFFF) {
		return -EINVAL;
	}

	uart->UART_BRGR = UART_BRGR_CD(divisor);
	dev_data->baud_rate = baudrate;

	return 0;
}

static uint32_t uart_sam_cfg2sam_parity(uint8_t parity)
{
	switch (parity) {
	case UART_CFG_PARITY_EVEN:
		return UART_MR_PAR_EVEN;
	case UART_CFG_PARITY_ODD:
		return UART_MR_PAR_ODD;
	case UART_CFG_PARITY_SPACE:
		return UART_MR_PAR_SPACE;
	case UART_CFG_PARITY_MARK:
		return UART_MR_PAR_MARK;
	case UART_CFG_PARITY_NONE:
	default:
		return UART_MR_PAR_NO;
	}
}

static uint8_t uart_sam_get_parity(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	switch (uart->UART_MR & UART_MR_PAR_Msk) {
	case UART_MR_PAR_EVEN:
		return UART_CFG_PARITY_EVEN;
	case UART_MR_PAR_ODD:
		return UART_CFG_PARITY_ODD;
	case UART_MR_PAR_SPACE:
		return UART_CFG_PARITY_SPACE;
	case UART_MR_PAR_MARK:
		return UART_CFG_PARITY_MARK;
	case UART_MR_PAR_NO:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static int uart_sam_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	int retval;

	const struct uart_sam_dev_cfg *const config = dev->config;

	volatile Uart * const uart = config->regs;

	/* Driver only supports 8 data bits, 1 stop bit, and no flow control */
	if (cfg->stop_bits != UART_CFG_STOP_BITS_1 ||
		cfg->data_bits != UART_CFG_DATA_BITS_8 ||
		cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	/* Reset and disable UART */
	uart->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX
		      | UART_CR_RXDIS | UART_CR_TXDIS
		      | UART_CR_RSTSTA;

	/* baud rate driven by the peripheral clock, UART does not filter
	 * the receive line, parity chosen by config
	 */
	uart->UART_MR = UART_MR_CHMODE_NORMAL
		      | uart_sam_cfg2sam_parity(cfg->parity);

	/* Set baud rate */
	retval = uart_sam_baudrate_set(dev, cfg->baudrate);
	if (retval != 0) {
		return retval;
	}

	/* Enable receiver and transmitter */
	uart->UART_CR = UART_CR_RXEN | UART_CR_TXEN;

	return 0;
}

static int uart_sam_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct uart_sam_dev_data *const dev_data = dev->data;

	cfg->baudrate = dev_data->baud_rate;
	cfg->parity = uart_sam_get_parity(dev);
	/* only supported mode for this peripheral */
	cfg->stop_bits = UART_CFG_STOP_BITS_1;
	cfg->data_bits = UART_CFG_DATA_BITS_8;
	cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_sam_fifo_fill(const struct device *dev,
			      const uint8_t *tx_data,
			      int size)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	/* Wait for transmitter to be ready. */
	while ((uart->UART_SR & UART_SR_TXRDY) == 0) {
	}

	uart->UART_THR = *tx_data;

	return 1;
}

static int uart_sam_fifo_read(const struct device *dev, uint8_t *rx_data,
			      const int size)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;
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

static void uart_sam_irq_tx_enable(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	uart->UART_IER = UART_IER_TXRDY;
}

static void uart_sam_irq_tx_disable(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	uart->UART_IDR = UART_IDR_TXRDY;
}

static int uart_sam_irq_tx_ready(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	/* Check that the transmitter is ready but only
	 * return true if the interrupt is also enabled
	 */
	return (uart->UART_SR & UART_SR_TXRDY &&
		uart->UART_IMR & UART_IMR_TXRDY);
}

static void uart_sam_irq_rx_enable(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	uart->UART_IER = UART_IER_RXRDY;
}

static void uart_sam_irq_rx_disable(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	uart->UART_IDR = UART_IDR_RXRDY;
}

static int uart_sam_irq_tx_complete(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	return (uart->UART_SR & UART_SR_TXRDY &&
		uart->UART_IMR & UART_IMR_TXEMPTY);
}

static int uart_sam_irq_rx_ready(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	return (uart->UART_SR & UART_SR_RXRDY);
}

static void uart_sam_irq_err_enable(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	uart->UART_IER = UART_IER_OVRE | UART_IER_FRAME | UART_IER_PARE;
}

static void uart_sam_irq_err_disable(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	uart->UART_IDR = UART_IDR_OVRE | UART_IDR_FRAME | UART_IDR_PARE;
}

static int uart_sam_irq_is_pending(const struct device *dev)
{
	const struct uart_sam_dev_cfg *const cfg = dev->config;

	volatile Uart * const uart = cfg->regs;

	return (uart->UART_IMR & (UART_IMR_TXRDY | UART_IMR_RXRDY)) &
		(uart->UART_SR & (UART_SR_TXRDY | UART_SR_RXRDY));
}

static int uart_sam_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void uart_sam_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_sam_dev_data *const dev_data = dev->data;

	dev_data->irq_cb = cb;
	dev_data->irq_cb_data = cb_data;
}

static void uart_sam_isr(const struct device *dev)
{
	struct uart_sam_dev_data *const dev_data = dev->data;

	if (dev_data->irq_cb) {
		dev_data->irq_cb(dev, dev_data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_sam_init(const struct device *dev)
{
	int retval;

	const struct uart_sam_dev_cfg *const cfg = dev->config;

	struct uart_sam_dev_data *const dev_data = dev->data;

	Uart * const uart = cfg->regs;

	/* Enable UART clock in PMC */
	soc_pmc_peripheral_enable(cfg->periph_id);

	/* Connect pins to the peripheral */
	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	/* Disable Interrupts */
	uart->UART_IDR = 0xFFFFFFFF;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	struct uart_config uart_config = {
		.baudrate = dev_data->baud_rate,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};
	return uart_sam_configure(dev, &uart_config);
}

static const struct uart_driver_api uart_sam_driver_api = {
	.poll_in = uart_sam_poll_in,
	.poll_out = uart_sam_poll_out,
	.err_check = uart_sam_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_sam_configure,
	.config_get = uart_sam_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
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

#define UART_SAM_DECLARE_CFG(n, IRQ_FUNC_INIT)				\
	static const struct uart_sam_dev_cfg uart##n##_sam_config = {	\
		.regs = (Uart *)DT_INST_REG_ADDR(n),			\
		.periph_id = DT_INST_PROP(n, peripheral_id),		\
									\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
									\
		IRQ_FUNC_INIT						\
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_SAM_CONFIG_FUNC(n)						\
	static void uart##n##_sam_irq_config_func(const struct device *port)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    uart_sam_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}
#define UART_SAM_IRQ_CFG_FUNC_INIT(n)					\
	.irq_config_func = uart##n##_sam_irq_config_func
#define UART_SAM_INIT_CFG(n)						\
	UART_SAM_DECLARE_CFG(n, UART_SAM_IRQ_CFG_FUNC_INIT(n))
#else
#define UART_SAM_CONFIG_FUNC(n)
#define UART_SAM_IRQ_CFG_FUNC_INIT
#define UART_SAM_INIT_CFG(n)						\
	UART_SAM_DECLARE_CFG(n, UART_SAM_IRQ_CFG_FUNC_INIT)
#endif

#define UART_SAM_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	static struct uart_sam_dev_data uart##n##_sam_data = {		\
		.baud_rate = DT_INST_PROP(n, current_speed),		\
	};								\
									\
	static const struct uart_sam_dev_cfg uart##n##_sam_config;	\
									\
	DEVICE_DT_INST_DEFINE(n, &uart_sam_init,			\
			    NULL, &uart##n##_sam_data,			\
			    &uart##n##_sam_config, PRE_KERNEL_1,	\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &uart_sam_driver_api);			\
									\
	UART_SAM_CONFIG_FUNC(n)						\
									\
	UART_SAM_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(UART_SAM_INIT)
