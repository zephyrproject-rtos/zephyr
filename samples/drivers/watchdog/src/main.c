/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <device.h>
#include <watchdog.h>
#include "board.h"
#include <misc/printk.h>

/* WDT Requires a callback, there is no interrupt enable / disable. */
void wdt_example_cb(struct device *dev)
{
	printk("watchdog fired\n");
	wdt_reload(dev);
}

void main(void)
{
	struct wdt_config wr_cfg;
	struct wdt_config cfg;
	struct device *wdt_dev;

	printk("Start watchdog test\n");
	wr_cfg.timeout = WDT_2_27_CYCLES;
	wr_cfg.mode = WDT_MODE_INTERRUPT_RESET;
	wr_cfg.interrupt_fn = wdt_example_cb;

	wdt_dev = device_get_binding("WATCHDOG_0");

	wdt_enable(wdt_dev);
	wdt_set_config(wdt_dev, &wr_cfg);

	wdt_get_config(wdt_dev, &cfg);
	printk("timeout: %d\n", cfg.timeout);
	printk("mode: %d\n", cfg.mode);
}
