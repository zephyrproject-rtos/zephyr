/*
 * Copyright (c) 2016 Piotr Mienkowski
 * Copyright (c) 2018 Justin Watson
 * Copyright (c) 2023 Gerson Fernando Budke
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_usart

/** @file
 * @brief USART driver for Atmel SAM MCU family.
 */

#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/irq.h>

/* Device constant configuration parameters */
struct usart_sam_dev_cfg {
	Usart *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	bool hw_flow_control;

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

static int usart_sam_poll_in(const struct device *dev, unsigned char *c)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	Usart * const usart = config->regs;

	if (!(usart->US_CSR & US_CSR_RXRDY)) {
		return -1;
	}

	/* got a character */
	*c = (unsigned char)usart->US_RHR;

	return 0;
}

static void usart_sam_poll_out(const struct device *dev, unsigned char c)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	Usart * const usart = config->regs;

	/* Wait for transmitter to be ready */
	while (!(usart->US_CSR & US_CSR_TXRDY)) {
	}

	/* send a character */
	usart->US_THR = (uint32_t)c;
}

static int usart_sam_err_check(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;
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

static int usart_sam_baudrate_set(const struct device *dev, uint32_t baudrate)
{
	struct usart_sam_dev_data *const dev_data = dev->data;

	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

	uint32_t divisor;

	__ASSERT(baudrate,
		 "baud rate has to be bigger than 0");
	__ASSERT(SOC_ATMEL_SAM_MCK_FREQ_HZ/16U >= baudrate,
		 "MCK frequency is too small to set required baud rate");

	divisor = SOC_ATMEL_SAM_MCK_FREQ_HZ / 16U / baudrate;

	if (divisor > 0xFFFF) {
		return -EINVAL;
	}

	usart->US_BRGR = US_BRGR_CD(divisor);
	dev_data->baud_rate = baudrate;

	return 0;
}

static uint32_t usart_sam_cfg2sam_parity(uint8_t parity)
{
	switch (parity) {
	case UART_CFG_PARITY_EVEN:
		return US_MR_PAR_EVEN;
	case UART_CFG_PARITY_ODD:
		return US_MR_PAR_ODD;
	case UART_CFG_PARITY_SPACE:
		return US_MR_PAR_SPACE;
	case UART_CFG_PARITY_MARK:
		return US_MR_PAR_MARK;
	case UART_CFG_PARITY_NONE:
	default:
		return US_MR_PAR_NO;
	}
}

static uint8_t usart_sam_get_parity(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

	switch (usart->US_MR & US_MR_PAR_Msk) {
	case US_MR_PAR_EVEN:
		return UART_CFG_PARITY_EVEN;
	case US_MR_PAR_ODD:
		return UART_CFG_PARITY_ODD;
	case US_MR_PAR_SPACE:
		return UART_CFG_PARITY_SPACE;
	case US_MR_PAR_MARK:
		return UART_CFG_PARITY_MARK;
	case US_MR_PAR_NO:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static uint32_t usart_sam_cfg2sam_stop_bits(uint8_t stop_bits)
{
	switch (stop_bits) {
	case UART_CFG_STOP_BITS_1_5:
		return US_MR_NBSTOP_1_5_BIT;
	case UART_CFG_STOP_BITS_2:
		return US_MR_NBSTOP_2_BIT;
	case UART_CFG_STOP_BITS_1:
	default:
		return US_MR_NBSTOP_1_BIT;
	}
}

static uint8_t usart_sam_get_stop_bits(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

	switch (usart->US_MR & US_MR_NBSTOP_Msk) {
	case US_MR_NBSTOP_1_5_BIT:
		return UART_CFG_STOP_BITS_1_5;
	case US_MR_NBSTOP_2_BIT:
		return UART_CFG_STOP_BITS_2;
	case US_MR_NBSTOP_1_BIT:
	default:
		return UART_CFG_STOP_BITS_1;
	}
}

static uint32_t usart_sam_cfg2sam_data_bits(uint8_t data_bits)
{
	switch (data_bits) {
	case UART_CFG_DATA_BITS_5:
		return US_MR_CHRL_5_BIT;
	case UART_CFG_DATA_BITS_6:
		return US_MR_CHRL_6_BIT;
	case UART_CFG_DATA_BITS_7:
		return US_MR_CHRL_7_BIT;
	case UART_CFG_DATA_BITS_8:
	default:
		return US_MR_CHRL_8_BIT;
	}
}

static uint8_t usart_sam_get_data_bits(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

	switch (usart->US_MR & US_MR_CHRL_Msk) {
	case US_MR_CHRL_5_BIT:
		return UART_CFG_DATA_BITS_5;
	case US_MR_CHRL_6_BIT:
		return UART_CFG_DATA_BITS_6;
	case US_MR_CHRL_7_BIT:
		return UART_CFG_DATA_BITS_7;
	case US_MR_CHRL_8_BIT:
	default:
		return UART_CFG_DATA_BITS_8;
	}
}

static uint32_t usart_sam_cfg2sam_flow_ctrl(uint8_t flow_ctrl)
{
	switch (flow_ctrl) {
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		return US_MR_USART_MODE_HW_HANDSHAKING;
	case UART_CFG_FLOW_CTRL_NONE:
	default:
		return US_MR_USART_MODE_NORMAL;
	}
}

static uint8_t usart_sam_get_flow_ctrl(const struct device *dev)
{
	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

	switch (usart->US_MR & US_MR_USART_MODE_Msk) {
	case US_MR_USART_MODE_HW_HANDSHAKING:
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	case US_MR_USART_MODE_NORMAL:
	default:
		return UART_CFG_FLOW_CTRL_NONE;
	}
}

static int usart_sam_configure(const struct device *dev,
				const struct uart_config *cfg)
{
	int retval;

	const struct usart_sam_dev_cfg *const config = dev->config;

	volatile Usart * const usart = config->regs;

	/* Driver doesn't support 9 data bits, 0.5 stop bits, or DTR DSR flow control */
	if (cfg->data_bits == UART_CFG_DATA_BITS_9 ||
		cfg->stop_bits == UART_CFG_STOP_BITS_0_5 ||
		cfg->flow_ctrl == UART_CFG_FLOW_CTRL_DTR_DSR) {
		return -ENOTSUP;
	}

	/* Reset and disable USART */
	usart->US_CR = US_CR_RSTRX | US_CR_RSTTX
		     | US_CR_RXDIS | US_CR_TXDIS
		     | US_CR_RSTSTA;

	/* normal UART mode, baud rate driven by peripheral clock, all
	 * other values chosen by config
	 */
	usart->US_MR = US_MR_CHMODE_NORMAL
		     | US_MR_USCLKS_MCK
		     | usart_sam_cfg2sam_parity(cfg->parity)
		     | usart_sam_cfg2sam_stop_bits(cfg->stop_bits)
		     | usart_sam_cfg2sam_data_bits(cfg->data_bits)
		     | usart_sam_cfg2sam_flow_ctrl(cfg->flow_ctrl);

	/* Set baud rate */
	retval = usart_sam_baudrate_set(dev, cfg->baudrate);
	if (retval != 0) {
		return retval;
	}

	/* Enable receiver and transmitter */
	usart->US_CR = US_CR_RXEN | US_CR_TXEN;

	return 0;
}

static int usart_sam_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct usart_sam_dev_data *const dev_data = dev->data;

	cfg->baudrate = dev_data->baud_rate;
	cfg->parity = usart_sam_get_parity(dev);
	cfg->stop_bits = usart_sam_get_stop_bits(dev);
	cfg->data_bits = usart_sam_get_data_bits(dev);
	cfg->flow_ctrl = usart_sam_get_flow_ctrl(dev);

	return 0;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

static int usart_sam_fifo_fill(const struct device *dev,
			       const uint8_t *tx_data,
			       int size)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	/* Wait for transmitter to be ready. */
	while ((usart->US_CSR & US_CSR_TXRDY) == 0) {
	}

	usart->US_THR = *tx_data;

	return 1;
}

static int usart_sam_fifo_read(const struct device *dev, uint8_t *rx_data,
			       const int size)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;
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

static void usart_sam_irq_tx_enable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IER = US_IER_TXRDY;
}

static void usart_sam_irq_tx_disable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IDR = US_IDR_TXRDY;
}

static int usart_sam_irq_tx_ready(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	/* Check that the transmitter is ready but only
	 * return true if the interrupt is also enabled
	 */
	return (usart->US_CSR & US_CSR_TXRDY &&
		usart->US_IMR & US_IMR_TXRDY);
}

static void usart_sam_irq_rx_enable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IER = US_IER_RXRDY;
}

static void usart_sam_irq_rx_disable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IDR = US_IDR_RXRDY;
}

static int usart_sam_irq_tx_complete(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	return (usart->US_CSR & US_CSR_TXRDY &&
		usart->US_CSR & US_CSR_TXEMPTY);
}

static int usart_sam_irq_rx_ready(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	return (usart->US_CSR & US_CSR_RXRDY);
}

static void usart_sam_irq_err_enable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IER = US_IER_OVRE | US_IER_FRAME | US_IER_PARE;
}

static void usart_sam_irq_err_disable(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	usart->US_IDR = US_IDR_OVRE | US_IDR_FRAME | US_IDR_PARE;
}

static int usart_sam_irq_is_pending(const struct device *dev)
{
	const struct usart_sam_dev_cfg *config = dev->config;

	volatile Usart * const usart = config->regs;

	return (usart->US_IMR & (US_IMR_TXRDY | US_IMR_RXRDY)) &
		(usart->US_CSR & (US_CSR_TXRDY | US_CSR_RXRDY));
}

static int usart_sam_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void usart_sam_irq_callback_set(const struct device *dev,
				       uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct usart_sam_dev_data *const dev_data = dev->data;

	dev_data->irq_cb = cb;
	dev_data->cb_data = cb_data;
}

static void usart_sam_isr(const struct device *dev)
{
	struct usart_sam_dev_data *const dev_data = dev->data;

	if (dev_data->irq_cb) {
		dev_data->irq_cb(dev, dev_data->cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int usart_sam_init(const struct device *dev)
{
	int retval;

	const struct usart_sam_dev_cfg *const cfg = dev->config;

	struct usart_sam_dev_data *const dev_data = dev->data;

	Usart * const usart = cfg->regs;

	/* Enable USART clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clock_cfg);

	/* Connect pins to the peripheral */
	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	/* Disable Interrupts */
	usart->US_IDR = 0xFFFFFFFF;

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
	if (cfg->hw_flow_control) {
		uart_config.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
	}
	return usart_sam_configure(dev, &uart_config);
}

static DEVICE_API(uart, usart_sam_driver_api) = {
	.poll_in = usart_sam_poll_in,
	.poll_out = usart_sam_poll_out,
	.err_check = usart_sam_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = usart_sam_configure,
	.config_get = usart_sam_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
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
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),		\
		.hw_flow_control = DT_INST_PROP(n, hw_flow_control),	\
									\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
									\
		IRQ_FUNC_INIT						\
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define USART_SAM_CONFIG_FUNC(n)					\
	static void usart##n##_sam_irq_config_func(const struct device *port)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    usart_sam_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
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
	PINCTRL_DT_INST_DEFINE(n);					\
	static struct usart_sam_dev_data usart##n##_sam_data = {	\
		.baud_rate = DT_INST_PROP(n, current_speed),		\
	};								\
									\
	static const struct usart_sam_dev_cfg usart##n##_sam_config;	\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    usart_sam_init, NULL,			\
			    &usart##n##_sam_data,			\
			    &usart##n##_sam_config, PRE_KERNEL_1,	\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &usart_sam_driver_api);			\
									\
	USART_SAM_CONFIG_FUNC(n)					\
									\
	USART_SAM_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(USART_SAM_INIT)
