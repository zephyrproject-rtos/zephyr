/*
 * Copyright (c) 2020 Mark Olsson <mark@markolsson.se>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>
#include <zephyr/drivers/kscan.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

const struct device *const kscan_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_keyboard_scan));

static void k_callback(const struct device *dev, uint32_t row, uint32_t col,
		       bool pressed)
{
	ARG_UNUSED(dev);
	if (pressed) {
		printk("row = %u col = %u\n", row, col);
	}
}

void main(void)
{
	printk("Kscan touch panel sample application\n");

	if (!device_is_ready(kscan_dev)) {
		LOG_ERR("kscan device %s not ready", kscan_dev->name);
		return;
	}

	kscan_config(kscan_dev, k_callback);
	kscan_enable_callback(kscan_dev);
}
