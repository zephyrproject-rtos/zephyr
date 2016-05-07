/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
