/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Local Includes */

#include <zephyr/sys/util.h>
#include <zephyr/drivers/misc/renesas_rx_lvd/renesas_rx_lvd.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* LVD node & config from Device Tree */
#if DT_HAS_COMPAT_STATUS_OKAY(renesas_rx_lvd)
#define LVD_NODE DT_INST(0, renesas_rx_lvd)
#endif

#define LVD_ACTION        DT_PROP(LVD_NODE, lvd_action)
#define VOLTAGE_THRESHOLD DT_PROP(LVD_NODE, voltage_level)

/* GPIO LEDs */
#define LED_BELOW_NODE DT_ALIAS(led0)
#define LED_ABOVE_NODE DT_ALIAS(led1)

static const struct gpio_dt_spec led_below = GPIO_DT_SPEC_GET(LED_BELOW_NODE, gpios);
static const struct gpio_dt_spec led_above = GPIO_DT_SPEC_GET(LED_ABOVE_NODE, gpios);

/* Semaphore for LVD interrupt */
static struct k_sem lvd_sem;

#if LVD_ACTION == 2
#define LVD_INT 1
#else
#define LVD_INT 0
#endif

#if LVD_INT
void user_callback(void *user_data)
{
	ARG_UNUSED(user_data);
	printk("[LVD] Interrupt: Voltage level crossing detected!\n");
	printk("[WARNING] Voltage instability detected! Check power supply.\n");
	k_sem_give(&lvd_sem);
}
#endif

/* Handle voltage status: get, print, update LEDs */
static void handle_voltage_status(const struct device *lvd_dev, const char *tag)
{
	int ret;
	lvd_status_position_t position;
	lvd_status_cross_t cross;

	ret = renesas_rx_lvd_get_status(lvd_dev, &position, &cross);
	if (ret != 0) {
		printk("[%s] Error: Failed to get LVD status.\n", tag);
		return;
	}

	switch (position) {
	case LVD_STATUS_POSITION_ABOVE:
		printk("[%s] Voltage is ABOVE threshold (%d mV)\n", tag, VOLTAGE_THRESHOLD);
		gpio_pin_set_dt(&led_above, 1);
		gpio_pin_set_dt(&led_below, 0);
		break;
	case LVD_STATUS_POSITION_BELOW:
		printk("[%s] Voltage is BELOW threshold (%d mV)\n", tag, VOLTAGE_THRESHOLD);
		gpio_pin_set_dt(&led_below, 1);
		gpio_pin_set_dt(&led_above, 0);
		break;
	default:
		printk("[%s] Unknown voltage position\n", tag);
		break;
	}

	ret = renesas_rx_lvd_clear_status(lvd_dev);
	if (ret != 0) {
		printk("[%s] Warning: Failed to clear LVD status\n", tag);
	}
}

int main(void)
{
	int ret;
	const struct device *const lvd_dev = DEVICE_DT_GET(LVD_NODE);

	k_sem_init(&lvd_sem, 0, 1);

	if (!device_is_ready(lvd_dev)) {
		printk("[LVD] Error: LVD device not ready.\n");
		return -EINVAL;
	}

#if LVD_INT
	ret = renesas_rx_lvd_register_callback(lvd_dev, user_callback, NULL);
	if (ret != 0) {
		printk("[LVD] Error: Failed to register callback.\n");
		return -EINVAL;
	}
#endif

	if (!gpio_is_ready_dt(&led_below) || !gpio_is_ready_dt(&led_above)) {
		printk("[GPIO] Error: LED GPIOs not ready.\n");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led_below, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&led_above, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	printk("===========================================================\n");
	printk("LVD Voltage Monitoring Sample Started\n");
	printk("Threshold Voltage Level : %d mV\n", VOLTAGE_THRESHOLD);
	printk("===========================================================\n");

	handle_voltage_status(lvd_dev, "LVD Init");

	while (1) {
		k_sem_take(&lvd_sem, K_FOREVER);
		handle_voltage_status(lvd_dev, "LVD");
	}

	return 0;
}
