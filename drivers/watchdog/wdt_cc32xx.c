/*
 * Copyright (c) 2021 Pavlo Hamov <pasha.gamov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc32xx_watchdog

#include <zephyr/drivers/watchdog.h>
#include <soc.h>
#include <errno.h>

/* Driverlib includes */
#include <inc/hw_types.h>
#include <inc/hw_wdt.h>
#include <driverlib/pin.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/prcm.h>
#include <driverlib/wdt.h>

#define MAX_RELOAD_VALUE        0xFFFFFFFF

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(wdt_cc32xx);

struct wdt_cc32xx_data {
	int reload;
	wdt_callback_t cb;
	uint8_t flags;
};

struct wdt_cc32xx_cfg {
	const unsigned long reg;
	void (*irq_cfg_func)(void);
};

static uint32_t wdt_cc32xx_msToTicks(uint32_t ms)
{
	static const uint32_t ratio = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000;
	static const uint32_t maxMs = MAX_RELOAD_VALUE / ratio;

	if (ms > maxMs) {
		return maxMs;
	}

	return ms * ratio;
}

static int wdt_cc32xx_enable(const struct device *dev)
{
	struct wdt_cc32xx_data *data = dev->data;
	const struct wdt_cc32xx_cfg *config = dev->config;
	const uint32_t reload = wdt_cc32xx_msToTicks(data->reload);

	MAP_WatchdogIntClear(config->reg);
	MAP_WatchdogReloadSet(config->reg, reload);
	MAP_WatchdogEnable(config->reg);
	LOG_DBG("Enabled");
	return 0;
}

static int wdt_cc32xx_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_cc32xx_cfg *config = dev->config;
	int rv;

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		return -ENOTSUP;
	}

	MAP_WatchdogUnlock(config->reg);
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		MAP_WatchdogStallEnable(config->reg);
	} else {
		MAP_WatchdogStallDisable(config->reg);
	}
	rv = wdt_cc32xx_enable(dev);
	MAP_WatchdogLock(config->reg);
	return rv;
}

static int wdt_cc32xx_disable(const struct device *dev)
{
	return -ENOTSUP;
}

static int wdt_cc32xx_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *cfg)
{
	struct wdt_cc32xx_data *data = dev->data;

	if (COND_CODE_1(CONFIG_WDT_MULTISTAGE, (cfg->next), (0))) {
		return -ENOTSUP;
	}

	data->reload = cfg->window.max;
	data->cb = cfg->callback;
	data->flags = cfg->flags;
	LOG_DBG("Reload time %d", data->reload);
	return 0;
}

static int wdt_cc32xx_feed(const struct device *dev, int channel_id)
{
	struct wdt_cc32xx_data *data = dev->data;
	const struct wdt_cc32xx_cfg *config = dev->config;
	const uint32_t reload = wdt_cc32xx_msToTicks(data->reload);
	bool inIsr = k_is_in_isr();

	if (!inIsr) {
		MAP_WatchdogUnlock(config->reg);
	}
	MAP_WatchdogIntClear(config->reg);
	MAP_WatchdogReloadSet(config->reg, reload);
	if (!inIsr) {
		MAP_WatchdogLock(config->reg);
	}
	LOG_DBG("Feed");
	return 0;
}

static void wdt_cc32xx_isr(const struct device *dev)
{
	struct wdt_cc32xx_data *data = dev->data;

	LOG_DBG("ISR");
	if (data->cb) {
		data->cb(dev, 0);
	}
	if (data->flags != WDT_FLAG_RESET_NONE) {
		LOG_PANIC();
		MAP_PRCMMCUReset(data->flags & WDT_FLAG_RESET_SOC);
		while (1) {
		}
	}
}

static int wdt_cc32xx_init(const struct device *dev)
{
	const struct wdt_cc32xx_cfg *config = dev->config;
	int rv;

	LOG_DBG("init");
	config->irq_cfg_func();

	MAP_PRCMPeripheralClkEnable(PRCM_WDT, PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);
	while (!MAP_PRCMPeripheralStatusGet(PRCM_WDT)) {
	}

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		return 0;
	}

	MAP_WatchdogUnlock(config->reg);
	rv = wdt_cc32xx_enable(dev);
	MAP_WatchdogLock(config->reg);
	return rv;
}

static const struct wdt_driver_api wdt_cc32xx_api = {
	.setup = wdt_cc32xx_setup,
	.disable = wdt_cc32xx_disable,
	.install_timeout = wdt_cc32xx_install_timeout,
	.feed = wdt_cc32xx_feed,
};

#define cc32xx_WDT_INIT(index)							 \
										 \
	static void wdt_cc32xx_irq_cfg_##index(void)				 \
	{									 \
		IRQ_CONNECT(DT_INST_IRQN(index),				 \
			    DT_INST_IRQ(index, priority),			 \
			    wdt_cc32xx_isr, DEVICE_DT_INST_GET(index), 0);	 \
		irq_enable(DT_INST_IRQN(index));				 \
	}									 \
										 \
	static struct wdt_cc32xx_data wdt_cc32xx_data_##index = {		 \
		.reload = CONFIG_WDT_CC32XX_INITIAL_TIMEOUT,			 \
		.cb = NULL,							 \
		.flags = 0,							 \
	};									 \
										 \
	static struct wdt_cc32xx_cfg wdt_cc32xx_cfg_##index = {			 \
		.reg = (unsigned long)DT_INST_REG_ADDR(index),			 \
		.irq_cfg_func = wdt_cc32xx_irq_cfg_##index,			 \
	};									 \
										 \
	DEVICE_DT_INST_DEFINE(index,						 \
			      &wdt_cc32xx_init, NULL,				 \
			      &wdt_cc32xx_data_##index, &wdt_cc32xx_cfg_##index, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	 \
			      &wdt_cc32xx_api);

DT_INST_FOREACH_STATUS_OKAY(cc32xx_WDT_INIT)
