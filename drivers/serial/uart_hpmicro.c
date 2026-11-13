/*
 * Copyright (c) 2022 HPMicro
 * Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hpmicro_hpm_uart

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>

#include <hpm_clock_drv.h>
#include <hpm_soc.h>
#include <hpm_uart_drv.h>

struct hpm_uart_dev_config {
	UART_Type *base;
	const struct device *clock_dev;
	clock_name_t clock_name;
	clk_src_t clock_source;
	uint32_t clock_divider;
	uint32_t baud_rate;
	uint8_t parity;
	uint8_t stop_bits;
	uint8_t data_bits;
	uint8_t flow_ctrl;
	const struct pinctrl_dev_config *pincfg;
};

struct hpm_uart_dev_data {
	struct uart_config uart_config;
};

static int hpm_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct hpm_uart_dev_config *config = dev->config;
	struct hpm_uart_dev_data *data = dev->data;
	uart_config_t uart_config;
	parity_setting_t parity;
	num_of_stop_bits_t stop_bits;
	word_length_t word_length;
	uint32_t clock_rate;
	hpm_stat_t status;
	int ret;

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		parity = parity_none;
		break;
	case UART_CFG_PARITY_ODD:
		parity = parity_odd;
		break;
	case UART_CFG_PARITY_EVEN:
		parity = parity_even;
		break;
	case UART_CFG_PARITY_MARK:
		parity = parity_always_1;
		break;
	case UART_CFG_PARITY_SPACE:
		parity = parity_always_0;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		word_length = word_length_5_bits;
		break;
	case UART_CFG_DATA_BITS_6:
		word_length = word_length_6_bits;
		break;
	case UART_CFG_DATA_BITS_7:
		word_length = word_length_7_bits;
		break;
	case UART_CFG_DATA_BITS_8:
		word_length = word_length_8_bits;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		stop_bits = stop_bits_1;
		break;
	case UART_CFG_STOP_BITS_1_5:
		stop_bits = stop_bits_1_5;
		break;
	case UART_CFG_STOP_BITS_2:
		stop_bits = stop_bits_2;
		break;
	default:
		return -ENOTSUP;
	}

	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	ret = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)(uintptr_t)config->clock_name,
				     &clock_rate);
	if (ret < 0) {
		return ret;
	}

	uart_default_config(config->base, &uart_config);
	uart_config.src_freq_in_hz = clock_rate;
	uart_config.baudrate = cfg->baudrate;
	uart_config.parity = parity;
	uart_config.word_length = word_length;
	uart_config.num_of_stop_bits = stop_bits;

	status = uart_init(config->base, &uart_config);
	if (status != status_success) {
		return -EINVAL;
	}

	data->uart_config = *cfg;

	return 0;
}

static int hpm_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct hpm_uart_dev_data *data = dev->data;

	*cfg = data->uart_config;

	return 0;
}

static int hpm_uart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct hpm_uart_dev_config *config = dev->config;

	if (!uart_check_status(config->base, uart_stat_data_ready)) {
		return -1;
	}

	*c = (unsigned char)(config->base->RBR & UART_RBR_RBR_MASK);

	return 0;
}

static void hpm_uart_poll_out(const struct device *dev, unsigned char c)
{
	const struct hpm_uart_dev_config *config = dev->config;

	while (!uart_check_status(config->base, uart_stat_tx_slot_avail)) {
	}

	config->base->THR = UART_THR_THR_SET(c);
}

static int hpm_uart_err_check(const struct device *dev)
{
	const struct hpm_uart_dev_config *config = dev->config;
	uint32_t status = uart_get_status(config->base);
	int err = 0;

	if ((status & UART_LSR_OE_MASK) != 0U) {
		err |= UART_ERROR_OVERRUN;
	}

	if ((status & UART_LSR_PE_MASK) != 0U) {
		err |= UART_ERROR_PARITY;
	}

	if ((status & UART_LSR_FE_MASK) != 0U) {
		err |= UART_ERROR_FRAMING;
	}

	return err;
}

static int hpm_uart_init(const struct device *dev)
{
	const struct hpm_uart_dev_config *config = dev->config;
	struct hpm_uart_dev_data *data = dev->data;
	hpm_stat_t status;
	int ret;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	status = clock_set_source_divider(config->clock_name, config->clock_source,
					  config->clock_divider);
	if (status != status_success) {
		return -EIO;
	}

	ret = clock_control_on(config->clock_dev,
			       (clock_control_subsys_t)(uintptr_t)config->clock_name);
	if (ret < 0) {
		return ret;
	}

	data->uart_config = (struct uart_config){
		.baudrate = config->baud_rate,
		.parity = config->parity,
		.stop_bits = config->stop_bits,
		.flow_ctrl = config->flow_ctrl,
	};

	switch (config->data_bits) {
	case 5:
		data->uart_config.data_bits = UART_CFG_DATA_BITS_5;
		break;
	case 6:
		data->uart_config.data_bits = UART_CFG_DATA_BITS_6;
		break;
	case 7:
		data->uart_config.data_bits = UART_CFG_DATA_BITS_7;
		break;
	case 8:
		data->uart_config.data_bits = UART_CFG_DATA_BITS_8;
		break;
	default:
		return -ENOTSUP;
	}

	return hpm_uart_configure(dev, &data->uart_config);
}

static DEVICE_API(uart, hpm_uart_driver_api) = {
	.poll_in = hpm_uart_poll_in,
	.poll_out = hpm_uart_poll_out,
	.err_check = hpm_uart_err_check,
	.configure = hpm_uart_configure,
	.config_get = hpm_uart_config_get,
};

#define HPM_UART_INIT(n)                                                                           \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct hpm_uart_dev_data hpm_uart_data_##n;                                         \
	static const struct hpm_uart_dev_config hpm_uart_config_##n = {                            \
		.base = (UART_Type *)DT_INST_REG_ADDR(n),                                          \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_name = (clock_name_t)DT_INST_CLOCKS_CELL(n, name),                          \
		.clock_source = (clk_src_t)DT_INST_CLOCKS_CELL(n, source),                         \
		.clock_divider = DT_INST_CLOCKS_CELL(n, divider),                                  \
		.baud_rate = DT_INST_PROP_OR(n, current_speed, 115200),                            \
		.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),                    \
		.stop_bits = DT_INST_ENUM_IDX_OR(n, stop_bits, UART_CFG_STOP_BITS_1),              \
		.data_bits = DT_INST_PROP_OR(n, data_bits, 8),                                     \
		.flow_ctrl = DT_INST_PROP(n, hw_flow_control) ? UART_CFG_FLOW_CTRL_RTS_CTS         \
							      : UART_CFG_FLOW_CTRL_NONE,           \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, hpm_uart_init, NULL, &hpm_uart_data_##n, &hpm_uart_config_##n,    \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &hpm_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HPM_UART_INIT)
