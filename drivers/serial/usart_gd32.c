/*
 * Copyright (c) 2021, ATL Electronics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_usart

#include <drivers/pinctrl.h>
#include <drivers/uart.h>

struct gd32_usart_config {
	uint32_t reg;
	uint32_t rcu_periph_clock;
	const struct pinctrl_dev_config *pcfg;
};

struct gd32_usart_data {
	uint32_t baud_rate;
};

static int usart_gd32_init(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;
	struct gd32_usart_data *const data = dev->data;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	rcu_periph_clock_enable(cfg->rcu_periph_clock);
	usart_deinit(cfg->reg);
	usart_baudrate_set(cfg->reg, data->baud_rate);
	usart_word_length_set(cfg->reg, USART_WL_8BIT);
	usart_parity_config(cfg->reg, USART_PM_NONE);
	usart_stop_bit_set(cfg->reg, USART_STB_1BIT);
	usart_parity_config(cfg->reg, USART_PM_NONE);
	usart_receive_config(cfg->reg, USART_RECEIVE_ENABLE);
	usart_transmit_config(cfg->reg, USART_TRANSMIT_ENABLE);
	usart_enable(cfg->reg);

	return 0;
}

static int usart_gd32_poll_in(const struct device *dev, unsigned char *c)
{
	const struct gd32_usart_config *const cfg = dev->config;
	uint32_t status;

	status = usart_flag_get(cfg->reg, USART_FLAG_RBNE);

	if (!status) {
		return -EPERM;
	}

	*c = usart_data_receive(cfg->reg);

	return 0;
}

static void usart_gd32_poll_out(const struct device *dev, unsigned char c)
{
	const struct gd32_usart_config *const cfg = dev->config;

	usart_data_transmit(cfg->reg, c);

	while (usart_flag_get(cfg->reg, USART_FLAG_TBE) == RESET) {
		;
	}
}

static int usart_gd32_err_check(const struct device *dev)
{
	const struct gd32_usart_config *const cfg = dev->config;
	uint32_t status = USART_STAT0(cfg->reg);
	int errors = 0;

	if (status & USART_FLAG_ORERR) {
		usart_flag_clear(cfg->reg, USART_FLAG_ORERR);

		errors |= UART_ERROR_OVERRUN;
	}

	if (status & USART_FLAG_PERR) {
		usart_flag_clear(cfg->reg, USART_FLAG_PERR);

		errors |= UART_ERROR_PARITY;
	}

	if (status & USART_FLAG_FERR) {
		usart_flag_clear(cfg->reg, USART_FLAG_FERR);

		errors |= UART_ERROR_FRAMING;
	}

	usart_flag_clear(cfg->reg, USART_FLAG_NERR);

	return errors;
}

static const struct uart_driver_api usart_gd32_driver_api = {
	.poll_in = usart_gd32_poll_in,
	.poll_out = usart_gd32_poll_out,
	.err_check = usart_gd32_err_check,
};

#define GD32_USART_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n)						\
	static struct gd32_usart_data usart##n##_gd32_data = {			\
		.baud_rate = DT_INST_PROP(n, current_speed),			\
	};									\
	static const struct gd32_usart_config usart##n##_gd32_config = {	\
		.reg = DT_INST_REG_ADDR(n),					\
		.rcu_periph_clock = DT_INST_PROP(n, rcu_periph_clock),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	};									\
	DEVICE_DT_INST_DEFINE(n, &usart_gd32_init,				\
			      NULL,						\
			      &usart##n##_gd32_data,				\
			      &usart##n##_gd32_config, PRE_KERNEL_1,		\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &usart_gd32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GD32_USART_INIT)
