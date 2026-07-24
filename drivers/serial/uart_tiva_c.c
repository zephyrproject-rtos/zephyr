/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Linumiz
 * Author: Sri Surya <srisurya@linumiz.com>
 */

/* uart_tiva_c.c - TI Tiva C Series UART driver */

#define DT_DRV_COMPAT ti_tiva_c_uart

/**
 * UART driver for TI Tiva C Series (TM4C123G / TM4C129x)
 *
 * This driver consumes the TivaWare HAL (driverlib/uart.c) for
 * configuration, baud-rate setup, and data transfer.  Clock gating
 * is performed via TivaWare SysCtlPeripheralEnable().
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>

/* TivaWare HAL headers */
#include <driverlib/uart.h>
#include <driverlib/sysctl.h>

/*
 * SYSCTL_PERIPH_UARTn values from sysctl.h follow the pattern:
 *   SYSCTL_PERIPH_UART0 = 0xf0001800, UART1 = 0xf0001801, etc.
 * We derive the peripheral ID from the UART index.
 */
#define TIVA_C_SYSCTL_PERIPH_UART(idx) (SYSCTL_PERIPH_UART0 + (idx))


struct uart_tiva_c_config {
	uint32_t base;         /* UART base address (e.g. UART0_BASE) */
	uint32_t sys_clk_freq;
	uint8_t  uart_index;   /* 0-7 */
	const struct pinctrl_dev_config *pcfg;
};

struct uart_tiva_c_data {
	uint32_t baud_rate;
};

static int uart_tiva_c_init(const struct device *dev)
{
	const struct uart_tiva_c_config *cfg = dev->config;
	struct uart_tiva_c_data *data = dev->data;
	int ret;

	/* Enable the UART peripheral clock via TivaWare */
	SysCtlPeripheralEnable(TIVA_C_SYSCTL_PERIPH_UART(cfg->uart_index));

	/* Wait for peripheral to be ready */
	if (!WAIT_FOR(SysCtlPeripheralReady(
				TIVA_C_SYSCTL_PERIPH_UART(cfg->uart_index)),
		      1000, NULL)) {
		return -ETIMEDOUT;
	}

	/* Apply pin configuration from device tree */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Configure UART: baud rate, 8-N-1 */
	UARTConfigSetExpClk(cfg->base, cfg->sys_clk_freq,
			    data->baud_rate,
			    (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
			     UART_CONFIG_PAR_NONE));

	UARTEnable(cfg->base);

	return 0;
}

static int uart_tiva_c_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_tiva_c_config *cfg = dev->config;

	if (!UARTCharsAvail(cfg->base)) {
		return -1;
	}

	*c = (unsigned char)UARTCharGetNonBlocking(cfg->base);
	return 0;
}

static void uart_tiva_c_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_tiva_c_config *cfg = dev->config;

	UARTCharPut(cfg->base, c);
}


static DEVICE_API(uart, uart_tiva_c_driver_api) = {
	.poll_in = uart_tiva_c_poll_in,
	.poll_out = uart_tiva_c_poll_out,
};

/* Device instantiation macros */

#define TIVA_C_UART_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static const struct uart_tiva_c_config uart_tiva_c_cfg_##n = {	\
		.base = DT_INST_REG_ADDR(n),				\
		.sys_clk_freq = DT_PROP(DT_INST_CLOCKS_CTLR(n), clock_frequency),\
		.uart_index = DT_INST_PROP(n, peripheral_id),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
									\
	static struct uart_tiva_c_data uart_tiva_c_data_##n = {		\
		.baud_rate = DT_INST_PROP(n, current_speed),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      uart_tiva_c_init,				\
			      NULL,					\
			      &uart_tiva_c_data_##n,			\
			      &uart_tiva_c_cfg_##n,			\
			      PRE_KERNEL_1,				\
			      CONFIG_SERIAL_INIT_PRIORITY,		\
			      &uart_tiva_c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TIVA_C_UART_INIT)
