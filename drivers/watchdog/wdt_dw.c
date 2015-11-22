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

void (*cb_fn)(void);

/**
 * Enables the clock for the peripheral watchdog
 */
static void wdt_dw_enable(void)
{
	SCSS_PERIPHERAL->periph_cfg0 |= SCSS_PERIPH_CFG0_WDT_ENABLE;
}

static void wdt_dw_disable(void)
{
	/* Disable the clock for the peripheral watchdog */
	SCSS_PERIPHERAL->periph_cfg0 &= ~SCSS_PERIPH_CFG0_WDT_ENABLE;
}

void wdt_dw_isr(void)
{
	if (cb_fn) {
		(*cb_fn)();
	}
}

static void wdt_dw_get_config(struct wdt_config *config)
{

}

IRQ_CONNECT_STATIC(wdt_dw, INT_WDT_IRQ, INT_WDT_IRQ_PRI, wdt_dw_isr, 0);

static void wdt_dw_reload(void) { WDT_DW->wdt_crr = WDT_CRR_VAL; }

static int wdt_dw_set_config(struct wdt_config *config)
{
	int ret = 0;

	wdt_dw_enable();
	/*  Set timeout value
	 *  [7:4] TOP_INIT - the initial timeout value is hardcoded in silicon,
	 *  only bits [3:0] TOP are relevant.
	 *  Once tickled TOP is loaded at the next expiration.
	 */
	uint32_t i;
	uint32_t ref = (1 << 16) / (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC /
				    1000000); /* 2^16/FREQ_CPU */
	uint32_t timeout = config->timeout * 1000;

	for (i = 0; i < 16; i++) {
		if (timeout <= ref)
			break;
		ref = ref << 1;
	}
	if (i > 15) {
		ret = -1;
		i = 15;
	}
	WDT_DW->wdt_torr = i;

	/* Set response mode */
	if (WDT_MODE_RESET == config->mode) {
		WDT_DW->wdt_cr &= ~WDT_CR_INT_ENABLE;
	} else {
		if (config->interrupt_fn) {
			cb_fn = config->interrupt_fn;
		} else {
			return -1;
		}

		WDT_DW->wdt_cr |= WDT_CR_INT_ENABLE;

		IRQ_CONFIG(wdt_dw, INT_WDT_IRQ, 0);
		irq_enable(INT_WDT_IRQ);

		/* unmask WDT interrupts to lmt  */
		SCSS_INTERRUPT->int_watchdog_mask &= INT_UNMASK_IA;
	}

	/* Enable WDT, cannot be disabled until soc reset */
	WDT_DW->wdt_cr |= WDT_CR_ENABLE;

	wdt_dw_reload();
	return ret;
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

micro_early_init(wdt, NULL);
