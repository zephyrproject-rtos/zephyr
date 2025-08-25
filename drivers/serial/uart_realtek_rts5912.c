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
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>

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

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_DT_INST_DEFINE);
#if defined(CONFIG_PM)
#include <reg/reg_gpio.h>
#include "zephyr/drivers/gpio/gpio_rts5912.h"
uint32_t rx_wakeup_pin_num;
uint32_t *rts5912_rx_wake_reg;
struct gpio_dt_spec uart_rx_wakeup = GPIO_DT_SPEC_GET(DT_NODELABEL(uart0_wrapper), gpios);
struct k_work_delayable rx_refresh_timeout_work;

static void uart_rts5912_rx_refresh_timeout(struct k_work *work)
{
	ARG_UNUSED(work);

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
}
/* Hook to count entry/exit */
static void notify_pm_state_entry(enum pm_state state)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		irq_enable(rx_wakeup_pin_num);
		gpio_rts5912_set_wakeup_pin(rx_wakeup_pin_num);
		break;
	default:
		break;
	}
}
/* Hook to count entry/exit */
static void notify_pm_state_exit(enum pm_state state)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		uint32_t interrupt_pin = gpio_rts5912_get_intr_pin(rts5912_rx_wake_reg);

		pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(uart0_wrapper)),
				    PINCTRL_STATE_DEFAULT);
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
		if (interrupt_pin == uart_rx_wakeup.pin) {
			k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

			pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
			k_work_reschedule(&rx_refresh_timeout_work, delay);
		}
#endif
		NVIC_ClearPendingIRQ(rx_wakeup_pin_num);
		irq_disable(rx_wakeup_pin_num);
		break;
	default:
		break;
	}
}
static struct pm_notifier notifier = {
	.state_entry = notify_pm_state_entry,
	.state_exit = notify_pm_state_exit,
};

static void uart_rx_wait(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	if (uart_irq_rx_ready(dev)) {
		k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		k_work_reschedule(&rx_refresh_timeout_work, delay);
	}
#endif
}
#endif

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

#if defined(CONFIG_PM)
	k_work_init_delayable(&rx_refresh_timeout_work, uart_rts5912_rx_refresh_timeout);
	pm_notifier_register(&notifier);
	uart_irq_callback_set(DEVICE_DT_GET(DT_CHOSEN(zephyr_console)), uart_rx_wait);
	rx_wakeup_pin_num = gpio_rts5912_get_pin_num(&uart_rx_wakeup);
	rts5912_rx_wake_reg = (uint32_t *)gpio_rts5912_get_port_address(&uart_rx_wakeup);
#endif

	return 0;
}

#define UART_RTS5912_DEVICE_INIT(n)                                                                \
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
