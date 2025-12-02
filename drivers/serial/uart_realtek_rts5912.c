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
#include <zephyr/drivers/serial/uart_ns16550.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include "zephyr/drivers/gpio/gpio_rts5912.h"

LOG_MODULE_REGISTER(uart_rts5912, CONFIG_UART_LOG_LEVEL);

/* device config */
struct uart_rts5912_device_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	struct rts5912_sccon_subsys sccon_cfg;
	struct gpio_dt_spec uart_rx_wakeup;
	const struct device *uart_dev;
};

/** Device data structure */
struct uart_rts5912_dev_data {
#if defined(CONFIG_PM)
	struct pm_notifier pm_handles;
	uint32_t *rts5912_rx_wake_reg;
	uint32_t rx_wakeup_pin_num;
#endif
};

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_DT_INST_DEFINE);
#if defined(CONFIG_PM)
#define REG_IIR                              0x08 /* Interrupt ID reg.              */
#define REG_LSR                              0x14 /* Line status reg.               */
#define REG_USR                              0x7C /* UART status reg. (DW8250)      */
#define IIR_NIP                              0x01 /* no interrupt pending    */
#define IIR_THRE                             0x02 /* transmit holding register empty interrupt */
#define IIR_RBRF                             0x04 /* receiver buffer register full interrupt */
#define IIR_LS                               0x06 /* receiver line status interrupt */
#define IIR_MASK                             0x07 /* interrupt id bits mask  */
#define IIR_BUSY                             0x07 /* iir busy mask  */
#define USR_BUSY_CHECK                       BIT(0)
#define RTS5912_MAXIMUM_UART_POLLING_TIME_US (50 * USEC_PER_MSEC)
static struct k_work_delayable rx_refresh_timeout_work;

static void uart_rts5912_rx_refresh_timeout(struct k_work *work)
{
	ARG_UNUSED(work);

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
}
/* Hook to count entry/exit */
static void uart_rts5912_pm_state_entry(const struct device *dev, enum pm_state state)
{
	const struct uart_rts5912_dev_data *const dev_data = dev->data;

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		irq_enable(dev_data->rx_wakeup_pin_num);
		gpio_rts5912_set_wakeup_pin(dev_data->rx_wakeup_pin_num);
		break;
	default:
		break;
	}
}
/* Hook to count entry/exit */
static void uart_rts5912_pm_state_exit(const struct device *dev, enum pm_state state)
{
	const struct uart_rts5912_device_config *const dev_cfg = dev->config;
	const struct uart_rts5912_dev_data *const dev_data = dev->data;

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		uint32_t interrupt_pin = gpio_rts5912_get_intr_pin(dev_data->rts5912_rx_wake_reg);

		pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (IS_ENABLED(CONFIG_UART_CONSOLE_INPUT_EXPIRED) &&
		    interrupt_pin == dev_cfg->uart_rx_wakeup.pin) {
			k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

			pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
			k_work_reschedule(&rx_refresh_timeout_work, delay);
		}
		NVIC_ClearPendingIRQ(dev_data->rx_wakeup_pin_num);
		irq_disable(dev_data->rx_wakeup_pin_num);
		break;
	default:
		break;
	}
}

static void uart_rx_wait(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);
	if (IS_ENABLED(CONFIG_UART_CONSOLE_INPUT_EXPIRED) && uart_irq_rx_ready(dev)) {
		k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		k_work_reschedule(&rx_refresh_timeout_work, delay);
	}
}
#endif

static int rts5912_uart_init(const struct device *dev)
{
	const struct uart_rts5912_device_config *const dev_cfg = dev->config;
	struct uart_rts5912_dev_data *const dev_data = dev->data;
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
	const uint32_t uart_reg = uart_ns16550_get_port(dev_cfg->uart_dev);
	uint32_t wf_cycle_count = k_us_to_cyc_ceil32(RTS5912_MAXIMUM_UART_POLLING_TIME_US);
	uint32_t wf_start = k_cycle_get_32();
	uint32_t wf_now = wf_start;

	/*	For RTS5912 UART, if enable UART wake up function, it will
	 *	change GPIO pin from uart funciton to gpio function before WFI.
	 *	System need to ensure this register is cleared in every time doing init function.
	 */
	while (wf_cycle_count > (wf_now - wf_start)) {
		uint32_t iir = sys_read32(uart_reg + REG_IIR) & IIR_MASK;

		if (iir == IIR_NIP) {
			break;
		}
		switch (iir) {
		case IIR_LS: /* Receiver line status */
			sys_read32(uart_reg + REG_LSR);
			break;
		case IIR_RBRF: /* Received data available*/
			sys_write32(sys_read32(uart_reg + REG_IIR) | IIR_THRE,
				    (uart_reg + REG_IIR));
			break;
		case IIR_BUSY:
			while (wf_cycle_count > (wf_now - wf_start)) {
				uint32_t usr = sys_read32(uart_reg + REG_USR);

				if ((usr & USR_BUSY_CHECK) == 0) {
					break;
				}
				wf_now = k_cycle_get_32();
			}
			break;
		}
		iir = sys_read32(uart_reg + REG_IIR) & IIR_MASK;
		if (iir == IIR_NIP) {
			break;
		}
		k_busy_wait(10);
		wf_now = k_cycle_get_32();
	}

	if (wf_cycle_count <= (wf_now - wf_start)) {
		LOG_ERR("Uart reset iir reach timeout");
		return -EIO;
	}

	k_work_init_delayable(&rx_refresh_timeout_work, uart_rts5912_rx_refresh_timeout);
	pm_notifier_register(&dev_data->pm_handles);
	uart_irq_callback_set(DEVICE_DT_GET(DT_CHOSEN(zephyr_console)), uart_rx_wait);
	dev_data->rx_wakeup_pin_num = gpio_rts5912_get_pin_num(&dev_cfg->uart_rx_wakeup);
	dev_data->rts5912_rx_wake_reg = gpio_rts5912_get_port_address(&dev_cfg->uart_rx_wakeup);
#endif

	return 0;
}

#if defined(CONFIG_PM)
#define UART_REALTEK_RTS5912_PM_HANDLES_DEFINE(n)                                                  \
	static void uart_rts5912_##n##_pm_entry(enum pm_state state)                               \
	{                                                                                          \
		uart_rts5912_pm_state_entry(DEVICE_DT_INST_GET(n), state);                         \
	}                                                                                          \
                                                                                                   \
	static void uart_rts5912_##n##_pm_exit(enum pm_state state)                                \
	{                                                                                          \
		uart_rts5912_pm_state_exit(DEVICE_DT_INST_GET(n), state);                          \
	}
#define UART_REALTEK_RTS5912_PM_HANDLES_BIND(n)                                                    \
	.pm_handles = {                                                                            \
		.state_entry = uart_rts5912_##n##_pm_entry,                                        \
		.state_exit = uart_rts5912_##n##_pm_exit,                                          \
	},
#else
#define UART_REALTEK_RTS5912_PM_HANDLES_DEFINE(n)
#define UART_REALTEK_RTS5912_PM_HANDLES_BIND(n)
#endif

#define UART_RTS5912_DEVICE_INIT(n)                                                                \
	UART_REALTEK_RTS5912_PM_HANDLES_DEFINE(n)                                                  \
                                                                                                   \
	static const struct uart_rts5912_device_config uart_rts5912_dev_cfg_##n = {                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                  \
		.sccon_cfg =                                                                       \
			{                                                                          \
				.clk_grp = DT_INST_CLOCKS_CELL_BY_NAME(n, uart##n, clk_grp),       \
				.clk_idx = DT_INST_CLOCKS_CELL_BY_NAME(n, uart##n, clk_idx),       \
			},                                                                         \
		.uart_rx_wakeup = GPIO_DT_SPEC_INST_GET(n, rx_gpios),                              \
		.uart_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, uart_dev)),                           \
	};                                                                                         \
                                                                                                   \
	static struct uart_rts5912_dev_data uart_rts5912_dev_data_##n = {                          \
		UART_REALTEK_RTS5912_PM_HANDLES_BIND(n)};                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &rts5912_uart_init, NULL, &uart_rts5912_dev_data_##n,             \
			      &uart_rts5912_dev_cfg_##n, PRE_KERNEL_1,                             \
			      CONFIG_UART_RTS5912_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(UART_RTS5912_DEVICE_INIT)
