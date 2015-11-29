/*
 * Copyright (c) 2015 Intel Corporation.
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

#include <nanokernel.h>
#include <init.h>
#include "wdt_dw.h"

static void (*cb_fn)(void);

/**
 * Enables the clock for the peripheral watchdog
 */
static void wdt_dw_enable(void)
{
	sys_set_bit(WDT_BASE_ADDR + WDT_CR, 0);
	sys_set_bit(CLOCK_PERIPHERAL_BASE_ADDR, 1);
	sys_set_bit(SCSS_PERIPHERAL_BASE + SCSS_PERIPH_CFG0, 1);
}

static void wdt_dw_disable(void)
{
	/* Disable the clock for the peripheral watchdog */
	sys_clear_bit(SCSS_PERIPHERAL_BASE + SCSS_PERIPH_CFG0, 1);
}

void wdt_dw_isr(void)
{
	if (cb_fn) {
		(*cb_fn)();
	}
}

static void wdt_dw_get_config(struct wdt_config *config)
{
	config->timeout = sys_read32(WDT_BASE_ADDR + WDT_TORR) & WDT_TIMEOUT_MASK;
	config->mode = (sys_read32(WDT_BASE_ADDR + WDT_CR) & WDT_MODE) >> WDT_MODE_OFFSET;
	config->interrupt_fn = cb_fn;
}

IRQ_CONNECT_STATIC(wdt_dw, INT_WDT_IRQ, INT_WDT_IRQ_PRI, wdt_dw_isr, 0, 0);

static void wdt_dw_reload(void)
{
	sys_write32(WDT_CRR_VAL, WDT_BASE_ADDR + WDT_CRR);
}

static int wdt_dw_set_config(struct wdt_config *config)
{
	sys_write32(config->timeout, WDT_BASE_ADDR + WDT_TORR);

	/* Set response mode */
	if (WDT_MODE_RESET == config->mode) {
		sys_clear_bit(WDT_BASE_ADDR + WDT_CR, 1);
	} else {
		if (config->interrupt_fn) {
			cb_fn = config->interrupt_fn;
		} else {
			return DEV_FAIL;
		}

		sys_set_bit(WDT_BASE_ADDR + WDT_CR, 1);

		IRQ_CONFIG(wdt_dw, INT_WDT_IRQ, 0);
		irq_enable(INT_WDT_IRQ);

		/* unmask WDT interrupts to lmt  */
		SCSS_INTERRUPT->int_watchdog_mask &= INT_UNMASK_IA;
	}

	/* Enable WDT, cannot be disabled until soc reset */
	sys_set_bit(WDT_BASE_ADDR + WDT_CR, 0);

	wdt_dw_reload();
	return DEV_OK;
}

static struct wdt_driver_api wdt_dw_funcs = {
	.set_config = wdt_dw_set_config,
	.get_config = wdt_dw_get_config,
	.enable = wdt_dw_enable,
	.disable = wdt_dw_disable,
	.reload = wdt_dw_reload,
};

int wdt_dw_init(struct device *dev)
{
	dev->driver_api = &wdt_dw_funcs;
	return 0;
}

struct wdt_dw_dev_config wdt_dev = {
	.base_address = WDT_BASE_ADDR,
};

DECLARE_DEVICE_INIT_CONFIG(wdt, WDT_DRV_NAME, &wdt_dw_init, &wdt_dev);

SYS_DEFINE_DEVICE(wdt, NULL, SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
