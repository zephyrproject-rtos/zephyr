/*
 * Copyright (c) 2021, ATL Electronics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT artery_at32_usart

#include <errno.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/at32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

#include <at32_usart.h>

#define AT32_USART(reg) ((usart_type *) reg)

struct at32_usart_config {
	uint32_t reg;
	uint16_t clkid;
	struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pcfg;
	uint32_t parity;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

struct at32_usart_data {
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void usart_at32_isr(const struct device *dev)
{
	struct at32_usart_data *const data = dev->data;

	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int usart_at32_init(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	struct at32_usart_data *const data = dev->data;
	usart_data_bit_num_type word_length;
	usart_parity_selection_type parity;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/**
	 * In order to keep the transfer data size to 8 bits(1 byte),
	 * append word length to 9BIT if parity bit enabled.
	 */
	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		parity = USART_PARITY_NONE;
		word_length = USART_DATA_8BITS;
		break;
	case UART_CFG_PARITY_ODD:
		parity = USART_PARITY_ODD;
		word_length = USART_DATA_9BITS;
		break;
	case UART_CFG_PARITY_EVEN:
		parity = USART_PARITY_EVEN;
		word_length = USART_DATA_9BITS;
		break;
	default:
		return -ENOTSUP;
	}

	(void)clock_control_on(AT32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clkid);

	(void)reset_line_toggle_dt(&cfg->reset);

	usart_init(AT32_USART(cfg->reg), data->baud_rate,
			word_length, USART_STOP_1_BIT);
	usart_parity_selection_config(AT32_USART(cfg->reg), parity);
	usart_receiver_enable(AT32_USART(cfg->reg), TRUE);
	usart_transmitter_enable(AT32_USART(cfg->reg), TRUE);
	usart_enable((AT32_USART(cfg->reg)), TRUE);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
	return 0;
}

static int usart_at32_poll_in(const struct device *dev, unsigned char *c)
{
	const struct at32_usart_config *const cfg = dev->config;
	uint32_t status;

	status = (uint32_t)usart_flag_get(AT32_USART(cfg->reg), USART_RDBF_FLAG);

	if (!status) {
		return -EPERM;
	}

	*c = usart_data_receive(AT32_USART(cfg->reg));

	return 0;
}

static void usart_at32_poll_out(const struct device *dev, unsigned char c)
{
	const struct at32_usart_config *const cfg = dev->config;

	usart_data_transmit(AT32_USART(cfg->reg), c);

	while (usart_flag_get(AT32_USART(cfg->reg), USART_TDBE_FLAG) == RESET) {
		;
	}
}

static int usart_at32_err_check(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);
	uint32_t status = usart->sts;
	int errors = 0;

	if (status & USART_ROERR_FLAG) {
		usart_flag_clear(usart, USART_ROERR_FLAG);

		errors |= UART_ERROR_OVERRUN;
	}

	if (status & USART_PERR_FLAG) {
		usart_flag_clear(usart, USART_PERR_FLAG);

		errors |= UART_ERROR_PARITY;
	}

	if (status & USART_FERR_FLAG) {
		usart_flag_clear(usart, USART_FERR_FLAG);

		errors |= UART_ERROR_FRAMING;
	}

	usart_flag_clear(usart, USART_NERR_FLAG);

	return errors;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
int usart_at32_fifo_fill(const struct device *dev, const uint8_t *tx_data,
			 int len)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       usart_flag_get(usart, USART_TDBE_FLAG)) {
		usart_data_transmit(usart, tx_data[num_tx++]);
	}
	return num_tx;
}

int usart_at32_fifo_read(const struct device *dev, uint8_t *rx_data,
			 const int size)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) &&
	       usart_flag_get(usart, USART_RDBF_FLAG)) {
		rx_data[num_rx++] = usart_data_receive(usart);
	}
	return num_rx;
}

void usart_at32_irq_tx_enable(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);

	usart_interrupt_enable(usart, USART_TDC_INT, TRUE);
}

void usart_at32_irq_tx_disable(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);

	usart_interrupt_enable(usart, USART_TDC_INT, FALSE);
}

int usart_at32_irq_tx_ready(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);

	return usart_flag_get(usart, USART_TDBE_FLAG) &&
	       usart_interrupt_flag_get(usart, USART_TDC_FLAG);
}

int usart_at32_irq_tx_complete(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);

	return (int)usart_flag_get(usart, USART_TDC_FLAG);
}

void usart_at32_irq_rx_enable(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);

	usart_interrupt_enable(usart, USART_RDBF_INT, TRUE);
}

void usart_at32_irq_rx_disable(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);

	usart_interrupt_enable(usart, USART_RDBF_INT, FALSE);
}

int usart_at32_irq_rx_ready(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);

	return usart_flag_get(usart, USART_RDBF_FLAG);
}

void usart_at32_irq_err_enable(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);

	usart_interrupt_enable(usart, USART_ERR_INT, TRUE);
	usart_interrupt_enable(usart, USART_PERR_INT, TRUE);
}

void usart_at32_irq_err_disable(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);

	usart_interrupt_enable(usart, USART_ERR_INT, FALSE);
	usart_interrupt_enable(usart, USART_PERR_INT, FALSE);
}

int usart_at32_irq_is_pending(const struct device *dev)
{
	const struct at32_usart_config *const cfg = dev->config;
	usart_type *usart = AT32_USART(cfg->reg);

	return ((usart_flag_get(usart, USART_RDBF_FLAG) &&
		 usart_interrupt_flag_get(usart, USART_RDBF_FLAG)) ||
		(usart_flag_get(usart, USART_TDC_FLAG) &&
		 usart_interrupt_flag_get(usart, USART_TDC_FLAG)));
}

void usart_at32_irq_callback_set(const struct device *dev,
				 uart_irq_callback_user_data_t cb,
				 void *user_data)
{
	struct at32_usart_data *const data = dev->data;

	data->user_cb = cb;
	data->user_data = user_data;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, usart_at32_driver_api) = {
	.poll_in = usart_at32_poll_in,
	.poll_out = usart_at32_poll_out,
	.err_check = usart_at32_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = usart_at32_fifo_fill,
	.fifo_read = usart_at32_fifo_read,
	.irq_tx_enable = usart_at32_irq_tx_enable,
	.irq_tx_disable = usart_at32_irq_tx_disable,
	.irq_tx_ready = usart_at32_irq_tx_ready,
	.irq_tx_complete = usart_at32_irq_tx_complete,
	.irq_rx_enable = usart_at32_irq_rx_enable,
	.irq_rx_disable = usart_at32_irq_rx_disable,
	.irq_rx_ready = usart_at32_irq_rx_ready,
	.irq_err_enable = usart_at32_irq_err_enable,
	.irq_err_disable = usart_at32_irq_err_disable,
	.irq_is_pending = usart_at32_irq_is_pending,
	.irq_callback_set = usart_at32_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define AT32_USART_IRQ_HANDLER(n)						\
	static void usart_at32_config_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    usart_at32_isr,					\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
		irq_enable(DT_INST_IRQN(n));					\
	}
#define AT32_USART_IRQ_HANDLER_FUNC_INIT(n)					\
	.irq_config_func = usart_at32_config_func_##n
#else /* CONFIG_UART_INTERRUPT_DRIVEN */
#define AT32_USART_IRQ_HANDLER(n)
#define AT32_USART_IRQ_HANDLER_FUNC_INIT(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define AT32_USART_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);						\
	AT32_USART_IRQ_HANDLER(n)						\
	static struct at32_usart_data usart_at32_data_##n = {			\
		.baud_rate = DT_INST_PROP(n, current_speed),			\
	};									\
	static const struct at32_usart_config usart_at32_config_##n = {		\
		.reg = DT_INST_REG_ADDR(n),					\
		.clkid = DT_INST_CLOCKS_CELL(n, id),				\
		.reset = RESET_DT_SPEC_INST_GET(n),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),	\
		 AT32_USART_IRQ_HANDLER_FUNC_INIT(n)				\
	};									\
	DEVICE_DT_INST_DEFINE(n, usart_at32_init,				\
			      NULL,						\
			      &usart_at32_data_##n,				\
			      &usart_at32_config_##n, PRE_KERNEL_1,		\
			      CONFIG_SERIAL_INIT_PRIORITY,			\
			      &usart_at32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AT32_USART_INIT)
