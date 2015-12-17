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
#include <clock_control.h>
#include "wdt_dw.h"

#ifdef WDT_DW_INT_MASK
static inline void _wdt_dw_int_unmask(void)
{
	sys_write32(sys_read32(WDT_DW_INT_MASK) & INT_UNMASK_IA,
						WDT_DW_INT_MASK);
}
#else
#define _wdt_dw_int_unmask()
#endif

#ifdef CONFIG_WDT_DW_CLOCK_GATE
static inline void _wdt_dw_clock_config(struct device *dev)
{
	char *drv = CONFIG_WDT_DW_CLOCK_GATE_DRV_NAME;
	struct device *clk;

	clk = device_get_binding(drv);
	if (clk) {
		struct wdt_dw_runtime *context = dev->driver_data;

		context->clock = clk;
	}
}

static inline void _wdt_dw_clock_on(struct device *dev)
{
	struct wdt_dw_dev_config *config = dev->config->config_info;
	struct wdt_dw_runtime *context = dev->driver_data;

	clock_control_on(context->clock, config->clock_data);
}

static inline void _wdt_dw_clock_off(struct device *dev)
{
	struct wdt_dw_dev_config *config = dev->config->config_info;
	struct wdt_dw_runtime *context = dev->driver_data;

	clock_control_off(context->clock, config->clock_data);
}
#else
#define _wdt_dw_clock_config(...)
#define _wdt_dw_clock_on(...)
#define _wdt_dw_clock_off(...)
#endif

/**
 * Enables the clock for the peripheral watchdog
 */
static void wdt_dw_enable(struct device *dev)
{
	_wdt_dw_clock_on(dev);

#if defined(CONFIG_SOC_QUARK_SE) || defined(CONFIG_SOC_QUARK_D2000)
	sys_set_bit(SCSS_PERIPHERAL_BASE + SCSS_PERIPH_CFG0, 1);
#endif
}

static void wdt_dw_disable(struct device *dev)
{
	_wdt_dw_clock_off(dev);

#if defined(CONFIG_SOC_QUARK_SE) || defined(CONFIG_SOC_QUARK_D2000)
	sys_clear_bit(SCSS_PERIPHERAL_BASE + SCSS_PERIPH_CFG0, 1);
#endif
}

void wdt_dw_isr(void *arg)
{
	struct device *dev = arg;
	struct wdt_dw_runtime *context = dev->driver_data;

	if (context->cb_fn) {
		context->cb_fn(dev);
	}
}

static void wdt_dw_get_config(struct device *dev, struct wdt_config *config)
{
	struct wdt_dw_dev_config *wdt_dev = dev->config->config_info;
	struct wdt_dw_runtime *context = dev->driver_data;

	config->timeout = sys_read32(wdt_dev->base_address + WDT_TORR) &
							WDT_TIMEOUT_MASK;
	config->mode = (sys_read32(wdt_dev->base_address + WDT_CR) & WDT_MODE)
							>> WDT_MODE_OFFSET;
	config->interrupt_fn = context->cb_fn;
}

static void wdt_dw_reload(struct device *dev)
{
	struct wdt_dw_dev_config *wdt_dev = dev->config->config_info;

	sys_write32(WDT_CRR_VAL, wdt_dev->base_address + WDT_CRR);
}

static int wdt_dw_set_config(struct device *dev, struct wdt_config *config)
{
	struct wdt_dw_dev_config *wdt_dev = dev->config->config_info;
	struct wdt_dw_runtime *context = dev->driver_data;

	sys_write32(config->timeout, wdt_dev->base_address + WDT_TORR);

	/* Set response mode */
	if (WDT_MODE_RESET == config->mode) {
		sys_clear_bit(wdt_dev->base_address + WDT_CR, 1);
	} else {
		if (!config->interrupt_fn) {
			return DEV_FAIL;
		}

		context->cb_fn = config->interrupt_fn;
		sys_set_bit(wdt_dev->base_address + WDT_CR, 1);
	}

	/* Enable WDT, cannot be disabled until soc reset */
	sys_set_bit(wdt_dev->base_address + WDT_CR, 0);

	wdt_dw_reload(dev);

	return DEV_OK;
}

static struct wdt_driver_api wdt_dw_funcs = {
	.set_config = wdt_dw_set_config,
	.get_config = wdt_dw_get_config,
	.enable = wdt_dw_enable,
	.disable = wdt_dw_disable,
	.reload = wdt_dw_reload,
};

/* IRQ_CONFIG needs the flags variable declared by IRQ_CONNECT_STATIC */
IRQ_CONNECT_STATIC(wdt_dw, CONFIG_WDT_DW_IRQ,
		   CONFIG_WDT_DW_IRQ_PRI, wdt_dw_isr, 0, 0);

int wdt_dw_init(struct device *dev)
{
	dev->driver_api = &wdt_dw_funcs;

	IRQ_CONFIG(wdt_dw, CONFIG_WDT_DW_IRQ);
	irq_enable(CONFIG_WDT_DW_IRQ);

	_wdt_dw_int_unmask();

	_wdt_dw_clock_config(dev);

	return 0;
}

struct wdt_dw_runtime wdt_runtime;

struct wdt_dw_dev_config wdt_dev = {
	.base_address = CONFIG_WDT_DW_BASE_ADDR,
#ifdef CONFIG_WDT_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_WDT_DW_CLOCK_GATE_SUBSYS),
#endif
};

DECLARE_DEVICE_INIT_CONFIG(wdt, CONFIG_WDT_DW_DRV_NAME,
			   &wdt_dw_init, &wdt_dev);

SYS_DEFINE_DEVICE(wdt, &wdt_runtime, SECONDARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

struct device *wdt_dw_isr_dev = SYS_GET_DEVICE(wdt);
