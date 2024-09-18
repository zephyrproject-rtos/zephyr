/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_uart

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_ite_it8xxx2, CONFIG_UART_LOG_LEVEL);

#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_UART_CONSOLE_INPUT_EXPIRED)
static struct uart_it8xxx2_data *uart_console_data;
#endif

struct uart_it8xxx2_config {
	uint8_t port;
	/* GPIO cells */
	struct gpio_dt_spec gpio_wui;
	/* UART handle */
	const struct device *uart_dev;
	/* UART alternate configuration */
	const struct pinctrl_dev_config *pcfg;
};

struct uart_it8xxx2_data {
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	struct k_work_delayable rx_refresh_timeout_work;
#endif
};

enum uart_port_num {
	UART1 = 1,
	UART2,
};

#ifdef CONFIG_PM_DEVICE
void uart1_wui_isr(const struct device *gpio, struct gpio_callback *cb,
		   uint32_t pins)
{
	/* Disable interrupts on UART1 RX pin to avoid repeated interrupts. */
	(void)gpio_pin_interrupt_configure(gpio, (find_msb_set(pins) - 1),
					   GPIO_INT_DISABLE);

	/* Refresh console expired time if got UART Rx wake-up event */
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

	/*
	 * The pm state of it8xxx2 chip only supports standby, so here we
	 * can directly set the constraint for standby.
	 */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	k_work_reschedule(&uart_console_data->rx_refresh_timeout_work, delay);
#endif
}

void uart2_wui_isr(const struct device *gpio, struct gpio_callback *cb,
		   uint32_t pins)
{
	/* Disable interrupts on UART2 RX pin to avoid repeated interrupts. */
	(void)gpio_pin_interrupt_configure(gpio, (find_msb_set(pins) - 1),
					   GPIO_INT_DISABLE);

	/* Refresh console expired time if got UART Rx wake-up event */
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

	/*
	 * The pm state of it8xxx2 chip only supports standby, so here we
	 * can directly set the constraint for standby.
	 */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	k_work_reschedule(&uart_console_data->rx_refresh_timeout_work, delay);
#endif
}

static inline int uart_it8xxx2_pm_action(const struct device *dev,
					 enum pm_device_action action)
{
	const struct uart_it8xxx2_config *const config = dev->config;
	int ret = 0;

	switch (action) {
	/* Next device power state is in active. */
	case PM_DEVICE_ACTION_RESUME:
		/* Nothing to do. */
		break;
	/* Next device power state is deep doze mode */
	case PM_DEVICE_ACTION_SUSPEND:
		/* Enable UART WUI */
		ret = gpio_pin_interrupt_configure_dt(&config->gpio_wui,
						      GPIO_INT_MODE_EDGE | GPIO_INT_TRIG_LOW);
		if (ret < 0) {
			LOG_ERR("Failed to configure UART%d WUI (ret %d)",
				config->port, ret);
			return ret;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
static void uart_it8xxx2_rx_refresh_timeout(struct k_work *work)
{
	ARG_UNUSED(work);

	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}
#endif
#endif /* CONFIG_PM_DEVICE */


static int uart_it8xxx2_init(const struct device *dev)
{
	const struct uart_it8xxx2_config *const config = dev->config;
	int status;

	/* Set the pin to UART alternate function. */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure UART pins");
		return status;
	}

#ifdef CONFIG_PM_DEVICE
	const struct device *uart_console_dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	int ret = 0;

	/*
	 * If the UART is used as a console device, we need to configure
	 * UART Rx interrupt as wakeup source and initialize a delayable
	 * work for console expired time.
	 */
	if (config->uart_dev == uart_console_dev) {
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
		uart_console_data = dev->data;
		k_work_init_delayable(&uart_console_data->rx_refresh_timeout_work,
				      uart_it8xxx2_rx_refresh_timeout);
#endif
		/*
		 * When the system enters deep doze, all clocks are gated only the
		 * 32.768k clock is active. We need to wakeup EC by configuring
		 * UART Rx interrupt as a wakeup source. When the interrupt of UART
		 * Rx falling, EC will be woken.
		 */
		if (config->port == UART1) {
			static struct gpio_callback uart1_wui_cb;

			gpio_init_callback(&uart1_wui_cb, uart1_wui_isr,
					   BIT(config->gpio_wui.pin));

			ret = gpio_add_callback(config->gpio_wui.port, &uart1_wui_cb);
		} else if (config->port == UART2) {
			static struct gpio_callback uart2_wui_cb;

			gpio_init_callback(&uart2_wui_cb, uart2_wui_isr,
					   BIT(config->gpio_wui.pin));

			ret = gpio_add_callback(config->gpio_wui.port, &uart2_wui_cb);
		}

		if (ret < 0) {
			LOG_ERR("Failed to add UART%d callback (err %d)",
				config->port, ret);
			return ret;
		}
	}
#endif /* CONFIG_PM_DEVICE */

	return 0;
}

#define UART_ITE_IT8XXX2_INIT(inst)                                            \
	PINCTRL_DT_INST_DEFINE(inst);                                          \
	static const struct uart_it8xxx2_config uart_it8xxx2_cfg_##inst = {    \
		.port = DT_INST_PROP(inst, port_num),                          \
		.gpio_wui = GPIO_DT_SPEC_INST_GET(inst, gpios),                \
		.uart_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, uart_dev)),    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                  \
	};                                                                     \
									       \
	static struct uart_it8xxx2_data uart_it8xxx2_data_##inst;              \
									       \
	PM_DEVICE_DT_INST_DEFINE(inst, uart_it8xxx2_pm_action);                \
	DEVICE_DT_INST_DEFINE(inst, uart_it8xxx2_init,                         \
			      PM_DEVICE_DT_INST_GET(inst),                     \
			      &uart_it8xxx2_data_##inst,                       \
			      &uart_it8xxx2_cfg_##inst,                        \
			      PRE_KERNEL_1,                                    \
			      CONFIG_UART_ITE_IT8XXX2_INIT_PRIORITY,           \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(UART_ITE_IT8XXX2_INIT)
