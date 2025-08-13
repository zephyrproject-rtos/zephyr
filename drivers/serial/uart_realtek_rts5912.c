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

/** Device config structure */
struct uart_ns16550_dev_config {
	union {
		DEVICE_MMIO_ROM;
		uint32_t port;
	};
	uint32_t sys_clk_freq;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t irq_config_func;
#endif
#if UART_NS16550_PCP_ENABLED
	uint32_t pcp;
#endif
	uint8_t reg_interval;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
	struct pcie_dev *pcie;
#endif
#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pincfg;
#endif
#if UART_NS16550_IOPORT_ENABLED
	bool io_map;
#endif
#if UART_NS16550_RESET_ENABLED
	struct reset_dt_spec reset_spec;
#endif
};

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_DT_INST_DEFINE);
#if defined(CONFIG_PM)
#define REG_IIR  0x08 /* Interrupt ID reg.              */
#define REG_LSR  0x14 /* Line status reg.               */
#define REG_USR  0x7C /* UART status reg. (DW8250)      */
#define IIR_NIP  0x01 /* no interrupt pending    */
#define IIR_THRE 0x02 /* transmit holding register empty interrupt */
#define IIR_RBRF 0x04 /* receiver buffer register full interrupt */
#define IIR_LS   0x06 /* receiver line status interrupt */
#define IIR_MASK 0x07 /* interrupt id bits mask  */
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
	const struct device *uart_device = dev_cfg->uart_dev;
	const struct uart_ns16550_dev_config *uart_cfg = uart_device->config;
	uint32_t iir = *(volatile uint32_t *)(uart_cfg->port + REG_IIR) & 0x0f;
	uint32_t temp = 0;

	while (iir != IIR_NIP) {
		switch (iir) {
		case IIR_LS: /* Receiver line status */
			temp = *(volatile uint32_t *)(uart_cfg->port + REG_LSR);
			break;
		case IIR_RBRF: /* Received data available*/
			*(volatile uint32_t *)(uart_cfg->port + REG_IIR) |= IIR_THRE;
			break;
		case IIR_MASK:
			while (1) {
				uint32_t usr = *(volatile uint32_t *)(uart_cfg->port + REG_USR);

				if ((usr & BIT(0)) == 0) {
					break;
				}
			}
			break;
		}

		iir = *(volatile uint32_t *)(uart_cfg->port + REG_IIR) & 0x0f;
	}

	k_work_init_delayable(&rx_refresh_timeout_work, uart_rts5912_rx_refresh_timeout);
	pm_notifier_register(&dev_data->pm_handles);
	uart_irq_callback_set(DEVICE_DT_GET(DT_CHOSEN(zephyr_console)), uart_rx_wait);
	dev_data->rx_wakeup_pin_num = gpio_rts5912_get_pin_num(&dev_cfg->uart_rx_wakeup);
	dev_data->rts5912_rx_wake_reg =
		(uint32_t *)gpio_rts5912_get_port_address(&dev_cfg->uart_rx_wakeup);
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
