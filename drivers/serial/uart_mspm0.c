/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_uart

/* Zephyr includes */
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

/* Driverlib includes */
#include <ti/driverlib/dl_uart_main.h>

struct uart_mspm0_config {
	UART_Regs *regs;
	uint32_t current_speed;
	const struct mspm0_sys_clock *clock_subsys;
	const struct pinctrl_dev_config *pinctrl;
};

struct uart_mspm0_data {
	/* UART clock structure */
	DL_UART_Main_ClockConfig uart_clockconfig;
	/* UART config structure */
	DL_UART_Main_Config uart_config;
};

static int uart_mspm0_init(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
	uint32_t clock_rate;
	int ret;

	/* Reset power */
	DL_UART_Main_reset(config->regs);
	DL_UART_Main_enablePower(config->regs);
	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);

	/* Init UART pins */
	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Set UART configs */
	DL_UART_Main_setClockConfig(config->regs,
				    &data->uart_clockconfig);
	DL_UART_Main_init(config->regs, &data->uart_config);

	/*
	 * Configure baud rate by setting oversampling and baud rate divisor
	 * from the device tree data current-speed
	 */
	ret = clock_control_get_rate(clk_dev,
				(struct mspm0_sys_clock *)config->clock_subsys,
				&clock_rate);
	if (ret < 0) {
		return ret;
	}

	DL_UART_Main_configBaudRate(config->regs,
				    clock_rate,
				    config->current_speed);

	/* Enable UART */
	DL_UART_Main_enable(config->regs);

	return 0;
}

static int uart_mspm0_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_mspm0_config *config = dev->config;

	if (DL_UART_Main_receiveDataCheck(config->regs, c) == false) {
		return -1;
	}

	return 0;
}

static void uart_mspm0_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_transmitDataBlocking(config->regs, c);
}

static const struct uart_driver_api uart_mspm0_driver_api = {
	.poll_in = uart_mspm0_poll_in,
	.poll_out = uart_mspm0_poll_out,
};

#define MSPM0_MAIN_CLK_DIV(n)  CONCAT(DL_UART_MAIN_CLOCK_DIVIDE_RATIO_, DT_INST_PROP(n, clk_div))

#define MSPM0_UART_INIT_FN(index)								\
												\
	PINCTRL_DT_INST_DEFINE(index);								\
												\
	static const struct mspm0_sys_clock mspm0_uart_sys_clock##index =			\
		MSPM0_CLOCK_SUBSYS_FN(index);							\
												\
	static const struct uart_mspm0_config uart_mspm0_cfg_##index = {			\
		.regs = (UART_Regs *)DT_INST_REG_ADDR(index),					\
		.current_speed = DT_INST_PROP(index, current_speed),				\
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(index),				\
		.clock_subsys = &mspm0_uart_sys_clock##index,					\
		};										\
												\
	static struct uart_mspm0_data uart_mspm0_data_##index = {				\
		.uart_clockconfig = {								\
			.clockSel = MSPM0_CLOCK_PERIPH_REG_MASK(DT_INST_CLOCKS_CELL(index, clk)), \
			.divideRatio = MSPM0_MAIN_CLK_DIV(index),				\
		 },										\
		.uart_config = {.mode = DL_UART_MAIN_MODE_NORMAL,				\
				.direction = DL_UART_MAIN_DIRECTION_TX_RX,			\
				.flowControl = (DT_INST_PROP(index, hw_flow_control)		\
						? DL_UART_MAIN_FLOW_CONTROL_RTS_CTS		\
						: DL_UART_MAIN_FLOW_CONTROL_NONE),		\
				.parity = DL_UART_MAIN_PARITY_NONE,				\
				.wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,			\
				.stopBits = DL_UART_MAIN_STOP_BITS_ONE,				\
				},								\
		};										\
												\
	DEVICE_DT_INST_DEFINE(index, &uart_mspm0_init, NULL, &uart_mspm0_data_##index,		\
			      &uart_mspm0_cfg_##index, PRE_KERNEL_1,				\
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_UART_INIT_FN)
