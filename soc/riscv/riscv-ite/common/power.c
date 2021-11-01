/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <kernel.h>
#include <pm/pm.h>
#include <soc.h>
#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(power, CONFIG_PM_LOG_LEVEL);

__weak void uart1_wui_isr_late(void) { }
__weak void uart2_wui_isr_late(void) { }

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
static const struct device *uart1_dev;

void uart1_wui_isr(const struct device *gpio, struct gpio_callback *cb,
		   uint32_t pins)
{
	/* Disable interrupts on UART1 RX pin to avoid repeated interrupts. */
	gpio_pin_interrupt_configure(gpio, (find_msb_set(pins) - 1),
				     GPIO_INT_DISABLE);

	if (uart1_wui_isr_late) {
		uart1_wui_isr_late();
	}
}
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
static const struct device *uart2_dev;

void uart2_wui_isr(const struct device *gpio, struct gpio_callback *cb,
		   uint32_t pins)
{
	/* Disable interrupts on UART2 RX pin to avoid repeated interrupts. */
	gpio_pin_interrupt_configure(gpio, (find_msb_set(pins) - 1),
				     GPIO_INT_DISABLE);

	if (uart2_wui_isr_late) {
		uart2_wui_isr_late();
	}
}
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) */

/* Handle when enter deep doze mode. */
static void ite_power_soc_deep_doze(void)
{
	int ret;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	/* Enable UART1 WUI */
	ret = gpio_pin_interrupt_configure(DEVICE_DT_GET(DT_NODELABEL(gpiob)),
					   0, GPIO_INT_TRIG_LOW);
	if (ret) {
		LOG_ERR("Failed to configure UART1 WUI (ret %d)", ret);
		return;
	}
	/* If nothing remains to be transmitted. */
	while (!uart_irq_tx_complete(uart1_dev))
		;
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	/* Enable UART2 WUI */
	ret = gpio_pin_interrupt_configure(DEVICE_DT_GET(DT_NODELABEL(gpioh)),
					   1, GPIO_INT_TRIG_LOW);
	if (ret) {
		LOG_ERR("Failed to configure UART2 WUI (ret %d)", ret);
		return;
	}
	/* If nothing remains to be transmitted. */
	while (!uart_irq_tx_complete(uart2_dev))
		;
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) */

	/* Enter deep doze mode */
	riscv_idle(CHIP_PLL_DEEP_DOZE, MSTATUS_IEN);
}

/* Invoke Low Power/System Off specific Tasks */
__weak void pm_power_state_set(struct pm_state_info info)
{
	switch (info.state) {
	/* Deep doze mode */
	case PM_STATE_STANDBY:
		ite_power_soc_deep_doze();
		break;
	default:
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	ARG_UNUSED(info);
}

static int power_it8xxx2_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	int ret;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	static struct gpio_callback uart1_wui_cb;

	uart1_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart1)));
	if (!uart1_dev) {
		LOG_ERR("Fail to find %s", DT_LABEL(DT_NODELABEL(uart1)));
		return -ENODEV;
	}

	gpio_init_callback(&uart1_wui_cb, uart1_wui_isr, BIT(0));

	ret = gpio_add_callback(DEVICE_DT_GET(DT_NODELABEL(gpiob)),
				&uart1_wui_cb);
	if (ret) {
		LOG_ERR("Failed to add callback (err %d)", ret);
		return ret;
	}
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	static struct gpio_callback uart2_wui_cb;

	uart2_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart2)));
	if (!uart2_dev) {
		LOG_ERR("Fail to find %s", DT_LABEL(DT_NODELABEL(uart2)));
		return -ENODEV;
	}

	gpio_init_callback(&uart2_wui_cb, uart2_wui_isr, BIT(1));

	ret = gpio_add_callback(DEVICE_DT_GET(DT_NODELABEL(gpioh)),
				&uart2_wui_cb);
	if (ret) {
		LOG_ERR("Failed to add callback (err %d)", ret);
		return ret;
	}
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) */

	return 0;
}
SYS_INIT(power_it8xxx2_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
