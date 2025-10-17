/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#if DT_HAS_COMPAT_STATUS_OKAY(renesas_rx_lvd)
#define LVD_DEV DT_INST(0, renesas_rx_lvd)
#else
#error "Please set the correct device"
#endif

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
static const struct device *lvd_dev = DEVICE_DT_GET(LVD_DEV);

#define VREF_MV (DT_PROP(LVD_DEV, voltage_level))

static void lvd_callback(const struct device *dev, void *user_data)
{
	int ret;

	printk("[WARNING] Voltage instability detected! Check power supply.\n");
	gpio_pin_set_dt(&led, 1);

	ret = comparator_get_output(lvd_dev);
	if (ret < 0) {
		printk("Error: failed to get comparator output\n");
		return;
	}
	printk("Comparator output is %s Vref (%.2fV)\n", ret ? "ABOVE" : "BELOW",
	       ((double)VREF_MV / 100));
}

int main(void)
{
	int ret;

	if (!device_is_ready(lvd_dev)) {
		printk("Comparator device not ready\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure LED\n");
		return -EINVAL;
	}

	gpio_pin_set_dt(&led, 0);

	ret = comparator_get_output(lvd_dev);
	if (ret < 0) {
		printk("Error: failed to get comparator output\n");
		return -EINVAL;
	}

	printk("Comparator output is %s Vref (%.2fV)\n", ret ? "ABOVE" : "BELOW",
	       ((double)VREF_MV / 100));

	ret = comparator_set_trigger(lvd_dev, COMPARATOR_TRIGGER_RISING_EDGE);
	if (ret < 0) {
		printk("Error: failed to set comparator trigger\n");
		return -EINVAL;
	}

	ret = comparator_set_trigger_callback(lvd_dev, lvd_callback, NULL);
	if (ret < 0) {
		printk("Error: failed to set comparator callback\n");
		return -EINVAL;
	}

	while (1) {
		k_sleep(K_MSEC(100));
	}
}
