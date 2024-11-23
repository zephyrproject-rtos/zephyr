/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#define DT_DRV_COMPAT realtek_rts5912_uart

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_rts5912, CONFIG_UART_LOG_LEVEL);

BUILD_ASSERT(CONFIG_UART_RTS5912_INIT_PRIORITY < CONFIG_SERIAL_INIT_PRIORITY,
	     "The uart_realtek_rts5912 driver must be initialized before the uart_ns16550 driver");

/* device config */
struct uart_rts5912_device_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	struct rts5912_sccon_subsys sccon_cfg;
};

/** Device data structure */
struct uart_rts5912_dev_data {
};

static int rts5912_uart_init(const struct device *dev)
{
	const struct uart_rts5912_device_config *const dev_cfg = dev->config;
	int rc;

	if (!device_is_ready(dev_cfg->clk_dev)) {
		return -ENODEV;
	}

	rc = clock_control_on(dev_cfg->clk_dev, (clock_control_subsys_t)&dev_cfg->sccon_cfg);
	if (rc != 0) {
		return rc;
	}

	rc = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc != 0) {
		return rc;
	}

	return 0;
}

#define UART_RTS5912_DEVICE_INIT(n)                                                                \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct uart_rts5912_device_config uart_rts5912_dev_cfg_##n = {                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                  \
		.sccon_cfg =                                                                       \
			{                                                                          \
				.clk_grp = DT_INST_CLOCKS_CELL_BY_NAME(n, uart##n, clk_grp),       \
				.clk_idx = DT_INST_CLOCKS_CELL_BY_NAME(n, uart##n, clk_idx),       \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct uart_rts5912_dev_data uart_rts5912_dev_data_##n;                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &rts5912_uart_init, NULL, &uart_rts5912_dev_data_##n,             \
			      &uart_rts5912_dev_cfg_##n, PRE_KERNEL_1,                             \
			      CONFIG_UART_RTS5912_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(UART_RTS5912_DEVICE_INIT)
