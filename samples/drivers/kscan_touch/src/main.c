/*
 * Copyright (c) 2020 Mark Olsson <mark@markolsson.se>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <drivers/kscan.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>

LOG_MODULE_REGISTER(main);

const struct device *kscan_dev;

#define TOUCH_CONTROLLER_LABEL DT_LABEL(DT_ALIAS(kscan0))

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

	kscan_dev = device_get_binding(TOUCH_CONTROLLER_LABEL);
	if (!kscan_dev) {
		LOG_ERR("kscan device %s not found", TOUCH_CONTROLLER_LABEL);
		return;
	}

	kscan_config(kscan_dev, k_callback);
	kscan_enable_callback(kscan_dev);
}
