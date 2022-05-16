/*
 * Copyright (c) 2021, Yonatan Schachter
 * Copyright (c) 2022, Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>

/* pico-sdk includes */
#include <hardware/uart.h>

#define DT_DRV_COMPAT raspberrypi_pico_uart

struct uart_rpi_config {
	uart_inst_t *const uart_dev;
	uart_hw_t *const uart_regs;
	const struct pinctrl_dev_config *pcfg;
	const struct reset_dt_spec reset;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

struct uart_rpi_data {
	uint32_t baudrate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_rpi_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_rpi_config *config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;

	if (uart_hw->fr & UART_UARTFR_RXFE_BITS) {
		return -1;
	}

	*c = (unsigned char)uart_hw->dr;
	return 0;
}

static void uart_rpi_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_rpi_config *config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;

	while (uart_hw->fr & UART_UARTFR_TXFF_BITS) {
		/* Wait */
	}

	uart_hw->dr = c;
}

static int uart_rpi_init(const struct device *dev)
{
	const struct uart_rpi_config *config = dev->config;
	uart_inst_t * const uart_inst = config->uart_dev;
	uart_hw_t * const uart_hw = config->uart_regs;
	struct uart_rpi_data * const data = dev->data;
	int ret, baudrate;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/*
	 * uart_init() may be replaced by register based API once rpi-pico platform
	 * has a clock controller driver
	 */
	baudrate = uart_init(uart_inst, data->baudrate);
	/* Check if baudrate adjustment returned by 'uart_init' function is a positive value */
	if (baudrate <= 0) {
		return -EINVAL;
	}

	hw_clear_bits(&uart_hw->lcr_h, UART_UARTLCR_H_FEN_BITS);
	uart_hw->dr = 0U;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

static int uart_rpi_err_check(const struct device *dev)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;
	uint32_t data_reg = uart_hw->dr;
	int errors = 0;

	if (data_reg & UART_UARTDR_OE_BITS) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (data_reg & UART_UARTDR_BE_BITS) {
		errors |= UART_BREAK;
	}

	if (data_reg & UART_UARTDR_PE_BITS) {
		errors |= UART_ERROR_PARITY;
	}

	if (data_reg & UART_UARTDR_FE_BITS) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_rpi_fifo_fill(const struct device *dev,
			      const uint8_t *tx_data, int len)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;
	int tx_len = 0;

	while (!(uart_hw->fr & UART_UARTFR_TXFF_BITS) && (len - tx_len) > 0) {
		uart_hw->dr = tx_data[tx_len++];
	}

	return tx_len;
}

static int uart_rpi_fifo_read(const struct device *dev,
			      uint8_t *rx_data, const int len)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;
	int rx_len = 0;

	while (!(uart_hw->fr & UART_UARTFR_RXFE_BITS) && (len - rx_len) > 0) {
		rx_data[rx_len++] = (uint8_t)uart_hw->dr;
	}

	return rx_len;
}

static void uart_rpi_irq_tx_enable(const struct device *dev)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;

	uart_hw->imsc |= UART_UARTIMSC_TXIM_BITS;
	uart_hw->ifls &= ~UART_UARTIFLS_TXIFLSEL_BITS;
}

static void uart_rpi_irq_tx_disable(const struct device *dev)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;

	uart_hw->imsc &= ~UART_UARTIMSC_TXIM_BITS;
}

static int uart_rpi_irq_tx_ready(const struct device *dev)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;

	return (uart_hw->mis & UART_UARTMIS_TXMIS_BITS) == UART_UARTMIS_TXMIS_BITS;
}

static void uart_rpi_irq_rx_enable(const struct device *dev)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;

	uart_hw->imsc |= UART_UARTIMSC_RXIM_BITS;
	uart_hw->ifls &= ~UART_UARTIFLS_RXIFLSEL_BITS;
}

static void uart_rpi_irq_rx_disable(const struct device *dev)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;

	uart_hw->imsc &= ~UART_UARTIMSC_RXIM_BITS;
}

static int uart_rpi_irq_tx_complete(const struct device *dev)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;

	return !!(uart_hw->fr & UART_UARTFR_TXFE_BITS);
}

static int uart_rpi_irq_rx_ready(const struct device *dev)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;

	return (uart_hw->mis & UART_UARTMIS_RXMIS_BITS) == UART_UARTMIS_RXMIS_BITS;
}

static void uart_rpi_irq_err_enable(const struct device *dev)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;

	uart_hw->imsc |= (UART_UARTIMSC_OEIM_BITS |
			  UART_UARTIMSC_BEIM_BITS |
			  UART_UARTIMSC_PEIM_BITS |
			  UART_UARTIMSC_FEIM_BITS |
			  UART_UARTIMSC_RTIM_BITS);
}

static void uart_rpi_irq_err_disable(const struct device *dev)
{
	const struct uart_rpi_config * const config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;

	uart_hw->imsc &= ~(UART_UARTIMSC_OEIM_BITS |
			   UART_UARTIMSC_BEIM_BITS |
			   UART_UARTIMSC_PEIM_BITS |
			   UART_UARTIMSC_FEIM_BITS |
			   UART_UARTIMSC_RTIM_BITS);
}

static int uart_rpi_irq_is_pending(const struct device *dev)
{
	return !!(uart_rpi_irq_rx_ready(dev) || uart_rpi_irq_tx_ready(dev));
}

static int uart_rpi_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_rpi_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_rpi_data * const data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}

static void uart_rpi_isr(const struct device *dev)
{
	struct uart_rpi_data * const data = dev->data;

	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_rpi_driver_api = {
	.poll_in = uart_rpi_poll_in,
	.poll_out = uart_rpi_poll_out,
	.err_check = uart_rpi_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_rpi_fifo_fill,
	.fifo_read = uart_rpi_fifo_read,
	.irq_tx_enable = uart_rpi_irq_tx_enable,
	.irq_tx_disable = uart_rpi_irq_tx_disable,
	.irq_tx_ready = uart_rpi_irq_tx_ready,
	.irq_rx_enable = uart_rpi_irq_rx_enable,
	.irq_rx_disable = uart_rpi_irq_rx_disable,
	.irq_tx_complete = uart_rpi_irq_tx_complete,
	.irq_rx_ready = uart_rpi_irq_rx_ready,
	.irq_err_enable = uart_rpi_irq_err_enable,
	.irq_err_disable = uart_rpi_irq_err_disable,
	.irq_is_pending = uart_rpi_irq_is_pending,
	.irq_update = uart_rpi_irq_update,
	.irq_callback_set = uart_rpi_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define RPI_UART_IRQ_CONFIG_INIT(idx)	\
	.irq_config_func = uart##idx##_rpi_irq_config_func
#else
#define RPI_UART_IRQ_CONFIG_INIT(idx)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN*/

#define RPI_UART_INIT(idx)							\
	PINCTRL_DT_INST_DEFINE(idx);						\
										\
	static void uart##idx##_rpi_irq_config_func(const struct device *port)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(idx),					\
			    DT_INST_IRQ(idx, priority),				\
			    uart_rpi_isr,					\
			    DEVICE_DT_INST_GET(idx), 0);			\
		irq_enable(DT_INST_IRQN(idx));					\
	}									\
										\
	static const struct uart_rpi_config uart##idx##_rpi_config = {		\
		.uart_dev = uart##idx,						\
		.uart_regs = (uart_hw_t *)uart##idx,				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),			\
		.reset = RESET_DT_SPEC_INST_GET(idx),				\
		RPI_UART_IRQ_CONFIG_INIT(idx),					\
	};									\
										\
	static struct uart_rpi_data uart##idx##_rpi_data = {			\
		.baudrate = DT_INST_PROP(idx, current_speed),			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(idx, &uart_rpi_init,				\
			    NULL, &uart##idx##_rpi_data,			\
			    &uart##idx##_rpi_config, PRE_KERNEL_1,		\
			    CONFIG_SERIAL_INIT_PRIORITY,			\
			    &uart_rpi_driver_api);				\

DT_INST_FOREACH_STATUS_OKAY(RPI_UART_INIT)
