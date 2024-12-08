/*
 * Copyright (c) 2024 Google LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_usart

#include <errno.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <ch32fun.h>

struct usart_wch_config {
	USART_TypeDef *regs;
	const struct device *clock_dev;
	uint32_t current_speed;
	uint8_t parity;
	uint8_t clock_id;
	const struct pinctrl_dev_config *pin_cfg;
};

struct usart_wch_data {
};

static int usart_wch_init(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;
	uint32_t ctlr1 = USART_CTLR1_TE | USART_CTLR1_RE | USART_CTLR1_UE;
	uint32_t clock_rate;
	clock_control_subsys_t clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clock_id;
	uint32_t divn;
	int err;

	clock_control_on(config->clock_dev, clock_sys);

	err = clock_control_get_rate(config->clock_dev, clock_sys, &clock_rate);
	if (err != 0) {
		return err;
	}
	divn = (clock_rate + config->current_speed / 2) / config->current_speed;

	switch (config->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_ODD:
		ctlr1 |= USART_CTLR1_PCE | USART_CTLR1_PS;
		break;
	case UART_CFG_PARITY_EVEN:
		ctlr1 |= USART_CTLR1_PCE;
		break;
	default:
		return -EINVAL;
	}

	regs->BRR = divn;
	regs->CTLR1 = ctlr1;
	regs->CTLR2 = 0;
	regs->CTLR3 = 0;

	err = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	return 0;
}

static int usart_wch_poll_in(const struct device *dev, unsigned char *ch)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	if ((regs->STATR & USART_STATR_RXNE) == 0) {
		return -1;
	}

	*ch = regs->DATAR;
	return 0;
}

static void usart_wch_poll_out(const struct device *dev, unsigned char ch)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;

	while ((regs->STATR & USART_STATR_TXE) == 0) {
	}

	regs->DATAR = ch;
}

static int usart_wch_err_check(const struct device *dev)
{
	const struct usart_wch_config *config = dev->config;
	USART_TypeDef *regs = config->regs;
	uint32_t statr = regs->STATR;
	enum uart_rx_stop_reason errors = 0;

	if ((statr & USART_STATR_PE) != 0) {
		errors |= UART_ERROR_PARITY;
	}
	if ((statr & USART_STATR_FE) != 0) {
		errors |= UART_ERROR_FRAMING;
	}
	if ((statr & USART_STATR_NE) != 0) {
		errors |= UART_ERROR_NOISE;
	}
	if ((statr & USART_STATR_ORE) != 0) {
		errors |= UART_ERROR_OVERRUN;
	}

	return errors;
}

static DEVICE_API(uart, usart_wch_driver_api) = {
	.poll_in = usart_wch_poll_in,
	.poll_out = usart_wch_poll_out,
	.err_check = usart_wch_err_check,
};

#define USART_WCH_INIT(idx)                                                                        \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static struct usart_wch_data usart_wch_##idx##_data;                                       \
	static const struct usart_wch_config usart_wch_##idx##_config = {                          \
		.regs = (USART_TypeDef *)DT_INST_REG_ADDR(idx),                                    \
		.current_speed = DT_INST_PROP(idx, current_speed),                                 \
		.parity = DT_INST_ENUM_IDX_OR(idx, parity, UART_CFG_PARITY_NONE),                  \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_id = DT_INST_CLOCKS_CELL(idx, id),                                          \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, &usart_wch_init, NULL, &usart_wch_##idx##_data,                 \
			      &usart_wch_##idx##_config, PRE_KERNEL_1,                             \
			      CONFIG_SERIAL_INIT_PRIORITY, &usart_wch_driver_api);

DT_INST_FOREACH_STATUS_OKAY(USART_WCH_INIT)
