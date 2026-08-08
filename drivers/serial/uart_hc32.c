/*
 * Copyright (c) 2026 Liu Changjie <liucj1228@outlook.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xhsc_hc32_usart

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>

#include <hc32_ll.h>
#include <hc32_ll_fcg.h>
#include <hc32_ll_usart.h>

struct uart_hc32_config {
	CM_USART_TypeDef *base;
	uint32_t baudrate;
	uint32_t fcg3_periph;
	const struct pinctrl_dev_config *pcfg;
};

static int uart_hc32_init(const struct device *dev)
{
	const struct uart_hc32_config *cfg = dev->config;
	stc_usart_uart_init_t init;
	int ret;

	LL_PERIPH_WE(LL_PERIPH_FCG);
	FCG_Fcg3PeriphClockCmd(cfg->fcg3_periph, ENABLE);
	LL_PERIPH_WP(LL_PERIPH_FCG);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (USART_UART_StructInit(&init) != LL_OK) {
		return -EIO;
	}

	init.u32Baudrate = cfg->baudrate;
	init.u32HWFlowControl = USART_HW_FLOWCTRL_NONE;

	if (USART_UART_Init(cfg->base, &init, NULL) != LL_OK) {
		return -EIO;
	}

	USART_FuncCmd(cfg->base, USART_TX | USART_RX, ENABLE);

	return 0;
}

static int uart_hc32_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_hc32_config *cfg = dev->config;

	if (USART_GetStatus(cfg->base, USART_FLAG_RX_FULL) == RESET) {
		return -1;
	}

	*c = (unsigned char)USART_ReadData(cfg->base);

	return 0;
}

static void uart_hc32_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_hc32_config *cfg = dev->config;

	while (USART_GetStatus(cfg->base, USART_FLAG_TX_EMPTY) == RESET) {
	}

	USART_WriteData(cfg->base, c);
}

static int uart_hc32_err_check(const struct device *dev)
{
	const struct uart_hc32_config *cfg = dev->config;
	uint32_t status = cfg->base->SR;
	int errors = 0;

	if ((status & USART_FLAG_OVERRUN) != 0U) {
		errors |= UART_ERROR_OVERRUN;
	}

	if ((status & USART_FLAG_PARITY_ERR) != 0U) {
		errors |= UART_ERROR_PARITY;
	}

	if ((status & USART_FLAG_FRAME_ERR) != 0U) {
		errors |= UART_ERROR_FRAMING;
	}

	if (errors != 0) {
		USART_ClearStatus(cfg->base, USART_FLAG_OVERRUN | USART_FLAG_PARITY_ERR |
						     USART_FLAG_FRAME_ERR);
	}

	return errors;
}

static DEVICE_API(uart, uart_hc32_driver_api) = {
	.poll_in = uart_hc32_poll_in,
	.poll_out = uart_hc32_poll_out,
	.err_check = uart_hc32_err_check,
};

#define UART_HC32_INIT(inst)                                                                       \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct uart_hc32_config uart_hc32_config_##inst = {                           \
		.base = (CM_USART_TypeDef *)DT_INST_REG_ADDR(inst),                                \
		.baudrate = DT_INST_PROP(inst, current_speed),                                     \
		.fcg3_periph = DT_INST_PROP(inst, fcg3_periph),                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, uart_hc32_init, NULL, NULL, &uart_hc32_config_##inst,          \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_hc32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_HC32_INIT)
