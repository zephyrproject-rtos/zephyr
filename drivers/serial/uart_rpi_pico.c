/*
 * Copyright (c) 2021, Yonatan Schachter
 * Copyright (c) 2022, Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

/* pico-sdk includes */
#include <hardware/uart.h>

#define DT_DRV_COMPAT raspberrypi_pico_uart

struct uart_rpi_config {
	uart_hw_t *const uart_regs;
	const struct pinctrl_dev_config *pcfg;
	const struct reset_dt_spec reset;
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
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

static int uart_rpi_set_baudrate(const struct device *dev, uint32_t input_baudrate,
				 uint32_t *output_baudrate)
{
	const struct uart_rpi_config *cfg = dev->config;
	uart_hw_t * const uart_hw = cfg->uart_regs;
	uint32_t baudrate_frac;
	uint32_t baudrate_int;
	uint32_t baudrate_div;
	uint32_t pclk;
	int ret;

	if (input_baudrate == 0) {
		return -EINVAL;
	}

	ret = clock_control_get_rate(cfg->clk_dev, cfg->clk_id, &pclk);
	if (ret < 0 || pclk == 0) {
		return -EINVAL;
	}

	baudrate_div = (8 * pclk / input_baudrate);
	baudrate_int = baudrate_div >> 7;
	baudrate_frac = (baudrate_int == 0) || (baudrate_int >= UINT16_MAX) ? 0 :
		((baudrate_div & 0x7f) + 1) / 2;
	baudrate_int = (baudrate_int == 0) ? 1 :
		(baudrate_int >= UINT16_MAX) ? UINT16_MAX : baudrate_int;

	uart_hw->ibrd = baudrate_int;
	uart_hw->fbrd = baudrate_frac;

	uart_hw->lcr_h |= 0;

	*output_baudrate = (4 * pclk) / (64 * baudrate_int + baudrate_frac);

	return 0;
}

static int uart_rpi_set_format(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_rpi_config *config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;
	uint32_t data_bits;
	uint32_t stop_bits;
	uint32_t lcr_value;
	uint32_t lcr_mask;

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

	lcr_mask = UART_UARTLCR_H_WLEN_BITS | UART_UARTLCR_H_STP2_BITS |
		   UART_UARTLCR_H_PEN_BITS | UART_UARTLCR_H_EPS_BITS;

	lcr_value = ((data_bits - 5) << UART_UARTLCR_H_WLEN_LSB) |
		    ((stop_bits - 1) << UART_UARTLCR_H_STP2_LSB) |
		    (!!(cfg->parity != UART_CFG_PARITY_NONE) << UART_UARTLCR_H_PEN_LSB) |
		    (!!(cfg->parity == UART_CFG_PARITY_EVEN) << UART_UARTLCR_H_EPS_LSB);

	uart_hw->lcr_h = (uart_hw->lcr_h & ~lcr_mask) | (lcr_value & lcr_mask);

	return 0;
}

static int uart_rpi_init(const struct device *dev)
{
	const struct uart_rpi_config *config = dev->config;
	uart_hw_t * const uart_hw = config->uart_regs;
	struct uart_rpi_data * const data = dev->data;
	uint32_t baudrate;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_on(config->clk_dev, config->clk_id);
	if (ret < 0) {
		return ret;
	}

	ret = reset_line_toggle(config->reset.dev, config->reset.id);
	if (ret < 0) {
		return ret;
	}

	ret = uart_rpi_set_baudrate(dev, data->uart_config.baudrate, &baudrate);
	if (ret < 0) {
		return ret;
	}

	uart_rpi_set_format(dev, &data->uart_config);

	uart_hw->cr = UART_UARTCR_UARTEN_BITS | UART_UARTCR_TXE_BITS | UART_UARTCR_RXE_BITS;
	uart_hw->lcr_h |= UART_UARTLCR_H_FEN_BITS;

	uart_hw->dmacr = UART_UARTDMACR_TXDMAE_BITS | UART_UARTDMACR_RXDMAE_BITS;

	/*
	 * initialize uart_config with hardware reset values
	 * https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf#tab-registerlist_uart page:431
	 * data bits set default to 8 instead of hardware reset 5 to increase compatibility.
	 */
	data->uart_config.data_bits = UART_CFG_DATA_BITS_8;
	data->uart_config.parity = UART_CFG_PARITY_NONE;
	data->uart_config.stop_bits = UART_CFG_STOP_BITS_1;
	uart_rpi_set_format(dev, &data->uart_config);
	uart_hw->dr = 0U;

	if (data->uart_config.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		uart_set_hw_flow((uart_inst_t *)uart_hw, true, true);
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

static int uart_rpi_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_rpi_data *data = dev->data;

	*cfg = data->uart_config;

	return 0;
}

static int uart_rpi_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct uart_rpi_data *data = dev->data;
	uint32_t baudrate = 0;
	int ret;

	ret = uart_rpi_set_baudrate(dev, cfg->baudrate, &baudrate);
	if (ret < 0) {
		return ret;
	}

	ret = uart_rpi_set_format(dev, cfg);
	if (ret < 0) {
		return ret;
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

	return !!(uart_hw->fr & UART_UARTFR_TXFE_BITS) && !(uart_hw->fr & UART_UARTFR_BUSY_BITS);
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

#define RPI_UART_INIT(idx)									   \
	PINCTRL_DT_INST_DEFINE(idx);								   \
												   \
	static void uart##idx##_rpi_irq_config_func(const struct device *port)			   \
	{											   \
		IRQ_CONNECT(DT_INST_IRQN(idx),							   \
			    DT_INST_IRQ(idx, priority),						   \
			    uart_rpi_isr,							   \
			    DEVICE_DT_INST_GET(idx), 0);					   \
		irq_enable(DT_INST_IRQN(idx));							   \
	}											   \
												   \
	static const struct uart_rpi_config uart##idx##_rpi_config = {				   \
		.uart_regs = (uart_hw_t *const)DT_INST_REG_ADDR(idx),				   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),					   \
		.reset = RESET_DT_SPEC_INST_GET(idx),						   \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),				   \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(idx, clocks, 0, clk_id),	   \
		RPI_UART_IRQ_CONFIG_INIT(idx),							   \
	};											   \
												   \
	static struct uart_rpi_data uart##idx##_rpi_data = {					   \
		.uart_config.baudrate = DT_INST_PROP(idx, current_speed),			   \
		.uart_config.flow_ctrl = DT_INST_PROP(idx, hw_flow_control)			   \
				       ? UART_CFG_FLOW_CTRL_RTS_CTS				   \
				       : UART_CFG_FLOW_CTRL_NONE,				   \
	};											   \
												   \
	DEVICE_DT_INST_DEFINE(idx, &uart_rpi_init,						   \
			    NULL, &uart##idx##_rpi_data,					   \
			    &uart##idx##_rpi_config, PRE_KERNEL_1,				   \
			    CONFIG_SERIAL_INIT_PRIORITY,					   \
			    &uart_rpi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RPI_UART_INIT)
