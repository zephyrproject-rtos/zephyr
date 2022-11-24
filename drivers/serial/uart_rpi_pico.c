/*
 * Copyright (c) 2021, Yonatan Schachter
 * Copyright (c) 2022, Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <string.h>

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
	struct uart_config uart_config;
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

static int uart_rpi_set_format(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_rpi_config *config = dev->config;
	uart_inst_t * const uart_inst = config->uart_dev;
	uart_parity_t parity = 0;
	uint data_bits = 0;
	uint stop_bits = 0;

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		data_bits = 5;
		break;
	case UART_CFG_DATA_BITS_6:
		data_bits = 6;
		break;
	case UART_CFG_DATA_BITS_7:
		data_bits = 7;
		break;
	case UART_CFG_DATA_BITS_8:
		data_bits = 8;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		stop_bits = 1;
		break;
	case UART_CFG_STOP_BITS_2:
		stop_bits = 2;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		parity = UART_PARITY_NONE;
		break;
	case UART_CFG_PARITY_EVEN:
		parity = UART_PARITY_EVEN;
		break;
	case UART_CFG_PARITY_ODD:
		parity = UART_PARITY_ODD;
		break;
	default:
		return -EINVAL;
	}

	uart_set_format(uart_inst, data_bits, stop_bits, parity);
	return 0;
}

static int uart_rpi_init(const struct device *dev)
{
	const struct uart_rpi_config *config = dev->config;
	uart_inst_t * const uart_inst = config->uart_dev;
	uart_hw_t * const uart_hw = config->uart_regs;
	struct uart_rpi_data * const data = dev->data;
	uint baudrate;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/*
	 * uart_init() may be replaced by register based API once rpi-pico platform
	 * has a clock controller driver
	 */
	baudrate = uart_init(uart_inst, data->uart_config.baudrate);
	/* Check if baudrate adjustment returned by 'uart_init' function is a positive value */
	if (baudrate == 0) {
		return -EINVAL;
	}
	/*
	 * initialize uart_config with hardware reset values
	 * https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf#tab-registerlist_uart page:431
	 * data bits set default to 8 instaed of hardware reset 5 to increase compatibility.
	 */
	data->uart_config = (struct uart_config){
		.baudrate = baudrate,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1
	};
	uart_rpi_set_format(dev, &data->uart_config);
	hw_clear_bits(&uart_hw->lcr_h, UART_UARTLCR_H_FEN_BITS);
	uart_hw->dr = 0U;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

static int uart_rpi_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_rpi_data *data = dev->data;

	memcpy(cfg, &data->uart_config, sizeof(struct uart_config));
	return 0;
}

static int uart_rpi_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_rpi_config *config = dev->config;
	uart_inst_t * const uart_inst = config->uart_dev;
	struct uart_rpi_data *data = dev->data;
	uint baudrate = 0;

	baudrate = uart_set_baudrate(uart_inst, cfg->baudrate);
	if (baudrate == 0) {
		return -EINVAL;
	}

	if (uart_rpi_set_format(dev, cfg) != 0) {
		return -EINVAL;
	}

	data->uart_config = *cfg;
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
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_rpi_configure,
	.config_get = uart_rpi_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
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
		.uart_config.baudrate = DT_INST_PROP(idx, current_speed),	\
	};									\
										\
	DEVICE_DT_INST_DEFINE(idx, &uart_rpi_init,				\
			    NULL, &uart##idx##_rpi_data,			\
			    &uart##idx##_rpi_config, PRE_KERNEL_1,		\
			    CONFIG_SERIAL_INIT_PRIORITY,			\
			    &uart_rpi_driver_api);				\

DT_INST_FOREACH_STATUS_OKAY(RPI_UART_INIT)
