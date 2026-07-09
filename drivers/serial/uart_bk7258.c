/*
 * Copyright (c) 2026 Embracecactus
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT beken_bk7258_uart

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/sys/util.h>

#define BK7258_UART_CFG_OFFSET         0x10U
#define BK7258_UART_FIFO_STATUS_OFFSET 0x18U
#define BK7258_UART_FIFO_PORT_OFFSET   0x1cU

#define BK7258_UART_TX_FULL_BIT  BIT(16)
#define BK7258_UART_RX_READY_BIT BIT(21)

#define BK7258_UART_XTAL_FREQ         26000000U
#define BK7258_UART_CFG_CLK_DIV_SHIFT 8U

struct uart_bk7258_config {
	mem_addr_t base;
	uint32_t current_speed;
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
	const struct pinctrl_dev_config *pcfg;
};

static void uart_bk7258_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_bk7258_config *cfg = dev->config;

	while (sys_read32(cfg->base + BK7258_UART_FIFO_STATUS_OFFSET) & BK7258_UART_TX_FULL_BIT) {
	}

	sys_write32((uint32_t)c & 0xffU, cfg->base + BK7258_UART_FIFO_PORT_OFFSET);
}

static int uart_bk7258_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_bk7258_config *cfg = dev->config;

	if (sys_read32(cfg->base + BK7258_UART_FIFO_STATUS_OFFSET) & BK7258_UART_RX_READY_BIT) {
		*c = (unsigned char)((sys_read32(cfg->base + BK7258_UART_FIFO_PORT_OFFSET) >> 8) &
				     0xffU);
		return 0;
	}

	return -1;
}

static int uart_bk7258_err_check(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int uart_bk7258_init(const struct device *dev)
{
	const struct uart_bk7258_config *cfg = dev->config;
	uint32_t clk_div;
	uint32_t val;
	int ret;

	if (cfg->current_speed == 0U) {
		return -EINVAL;
	}

	if (!device_is_ready(cfg->clk_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clk_dev, cfg->clk_id);
	if (ret != 0) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	clk_div = BK7258_UART_XTAL_FREQ / cfg->current_speed - 1U;
	val = (clk_div << BK7258_UART_CFG_CLK_DIV_SHIFT) | (3U << 3) | BIT(1) | BIT(0);
	sys_write32(val, cfg->base + BK7258_UART_CFG_OFFSET);

	return 0;
}

static DEVICE_API(uart, uart_bk7258_driver_api) = {
	.poll_in = uart_bk7258_poll_in,
	.poll_out = uart_bk7258_poll_out,
	.err_check = uart_bk7258_err_check,
};

#define UART_BK7258_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct uart_bk7258_config uart_bk7258_config_##inst = {                       \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.current_speed = DT_INST_PROP(inst, current_speed),                                \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
		.clk_id = (clock_control_subsys_t)(uintptr_t)DT_INST_CLOCKS_CELL(inst, id),        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, uart_bk7258_init, NULL, NULL, &uart_bk7258_config_##inst,      \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_bk7258_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_BK7258_INIT)
