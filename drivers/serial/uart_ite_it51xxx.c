/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_uart

#include <soc.h>
#include <soc_dt.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/wuc_ite_it51xxx.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

LOG_MODULE_REGISTER(uart_ite_it51xxx, CONFIG_UART_LOG_LEVEL);

struct it51xxx_uart_wuc_map_cfg {
	/* WUC control device structure */
	const struct device *wucs;
	/* WUC pin mask */
	uint8_t mask;
};

struct uart_it51xxx_config {
	/* UART wake-up input source configuration list */
	const struct it51xxx_uart_wuc_map_cfg *wuc_map_list;
	const struct device *clk_dev;
	/* clock configuration */
	struct ite_clk_cfg clk_cfg;
	/* UART interrupt */
	uint8_t irq;
};

struct uart_it51xxx_data {
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	struct k_work_delayable rx_refresh_timeout_work;
#endif
};

static void it51xxx_uart_wui_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct uart_it51xxx_config *const config = dev->config;

	/* Disable interrupts on UART RX pin to avoid repeated interrupts. */
	irq_disable(config->irq);
	/* W/C wakeup interrupt status of UART pin */
	it51xxx_wuc_clear_status(config->wuc_map_list[0].wucs, config->wuc_map_list[0].mask);

	/* Refresh console expired time if got UART Rx wake-up event */
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	struct uart_it51xxx_data *data = dev->data;
	k_timeout_t delay = K_MSEC(CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT);

	/*
	 * The pm state of it51xxx chip only supports standby, so here we
	 * can directly set the constraint for standby.
	 */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	k_work_reschedule(&data->rx_refresh_timeout_work, delay);
#endif
}

static inline int uart_it51xxx_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct uart_it51xxx_config *const config = dev->config;

	switch (action) {
	/* Next device power state is in active. */
	case PM_DEVICE_ACTION_RESUME:
		/* Nothing to do. */
		break;
	/* Next device power state is deep doze mode */
	case PM_DEVICE_ACTION_SUSPEND:
		/* W/C wake-up interrupt status of UART pin */
		it51xxx_wuc_clear_status(config->wuc_map_list[0].wucs,
					 config->wuc_map_list[0].mask);
		/* W/C interrupt status of UART pin */
		ite_intc_isr_clear(config->irq);
		/* Enable UART interrupt */
		irq_enable(config->irq);

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
static void uart_it51xxx_rx_refresh_timeout(struct k_work *work)
{
	ARG_UNUSED(work);

	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}
#endif

static int uart_it51xxx_init(const struct device *dev)
{
	const struct uart_it51xxx_config *const config = dev->config;
	int ret;

	/* Enable clock to specified peripheral */
	ret = clock_control_on(config->clk_dev, (clock_control_subsys_t *)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on clock fail %d", ret);
		return ret;
	}

	/* Select wakeup interrupt falling-edge triggered of UART pin */
	it51xxx_wuc_set_polarity(config->wuc_map_list[0].wucs, config->wuc_map_list[0].mask,
				 WUC_TYPE_EDGE_FALLING);
	/* W/C wakeup interrupt status of UART pin */
	it51xxx_wuc_clear_status(config->wuc_map_list[0].wucs, config->wuc_map_list[0].mask);
	/* Enable wakeup interrupt of UART pin */
	it51xxx_wuc_enable(config->wuc_map_list[0].wucs, config->wuc_map_list[0].mask);

	/*
	 * We need to configure UART Rx interrupt as wakeup source and initialize
	 * a delayable work for console expired time.
	 */
#ifdef CONFIG_UART_CONSOLE_INPUT_EXPIRED
	struct uart_it51xxx_data *data = dev->data;

	k_work_init_delayable(&data->rx_refresh_timeout_work, uart_it51xxx_rx_refresh_timeout);
#endif
	/*
	 * When the system enters deep doze, all clocks are gated only the
	 * 32.768k clock is active. We need to wakeup EC by configuring
	 * UART Rx interrupt as a wakeup source. When the interrupt of UART
	 * Rx falling, EC will be woken.
	 */
	irq_connect_dynamic(config->irq, 0, it51xxx_uart_wui_isr, dev, 0);

	return 0;
}

#define UART_ITE_IT51XXX_INIT(inst)                                                                \
	static const struct it51xxx_uart_wuc_map_cfg                                               \
		it51xxx_uart_wuc_##inst[IT8XXX2_DT_INST_WUCCTRL_LEN(inst)] =                       \
			IT8XXX2_DT_WUC_ITEMS_LIST(inst);                                           \
	static struct uart_it51xxx_data uart_it51xxx_data_##inst;                                  \
	static const struct uart_it51xxx_config uart_it51xxx_cfg_##inst = {                        \
		.wuc_map_list = it51xxx_uart_wuc_##inst,                                           \
		.clk_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, clocks)),                           \
		.clk_cfg = {.ctrl = DT_INST_CLOCKS_CELL(inst, ctrl),                               \
			    .bits = DT_INST_CLOCKS_CELL(inst, bits)},                              \
		.irq = DT_INST_IRQN(inst),                                                         \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, uart_it51xxx_pm_action);                                    \
	DEVICE_DT_INST_DEFINE(inst, uart_it51xxx_init, PM_DEVICE_DT_INST_GET(inst),                \
			      &uart_it51xxx_data_##inst, &uart_it51xxx_cfg_##inst, PRE_KERNEL_1,   \
			      CONFIG_SERIAL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(UART_ITE_IT51XXX_INIT)
