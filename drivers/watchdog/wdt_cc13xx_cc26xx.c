/*
 * Copyright (c) 2021 Florin Stancu <niflostancu@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_watchdog

#include <drivers/watchdog.h>
#include <soc.h>
#include <errno.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <logging/log.h>
#include <logging/log_ctrl.h>
LOG_MODULE_REGISTER(wdt_cc13xx_cc26xx);

/* Driverlib includes */
#include <inc/hw_types.h>
#include <inc/hw_wdt.h>
#include <driverlib/rom.h>
#include <driverlib/prcm.h>
#include <driverlib/watchdog.h>

#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

#define WDT_INST_IDX            0
#define MAX_RELOAD_VALUE        0xFFFFFFFF
#define WATCHDOG_DIV_RATIO      32            /* Watchdog division ratio */

struct wdt_cc13xx_cc26xx_data {
	uint32_t reload;
	wdt_callback_t cb;
	uint8_t flags;
} wdt_cc13xx_cc26xx_data = {
	.reload = CONFIG_WDT_CC13XX_CC26XX_INITIAL_TIMEOUT,
	.cb = NULL,
	.flags = 0,
};

struct wdt_cc13xx_cc26xx_cfg {
	const unsigned long reg; /* note: unused */
} wdt_cc13xx_cc26xx_cfg = {
	.reg = (unsigned long)DT_INST_REG_ADDR(WDT_INST_IDX),
};

static uint32_t wdt_cc13xx_cc26xx_msToTicks(uint32_t ms)
{
	const uint32_t ratio = CPU_FREQ / WATCHDOG_DIV_RATIO / 1000;
	const uint32_t maxMs = MAX_RELOAD_VALUE / ratio;

	if (ms > maxMs) {
		return maxMs;
	}

	return ms * ratio;
}

static int wdt_cc13xx_cc26xx_enable(const struct device *dev)
{
	struct wdt_cc13xx_cc26xx_data *data = dev->data;
	const uint32_t reload = wdt_cc13xx_cc26xx_msToTicks(data->reload);

	WatchdogIntClear();
	WatchdogReloadSet(reload);

	switch ((data->flags & WDT_FLAG_RESET_MASK)) {
	case WDT_FLAG_RESET_CPU_CORE:
	case WDT_FLAG_RESET_SOC:
		LOG_DBG("reset enabled");
		WatchdogResetEnable();
		break;
	case WDT_FLAG_RESET_NONE:
		LOG_DBG("reset disabled");
		WatchdogResetDisable();
		break;
	default:
		return -ENOTSUP;
	}

	WatchdogEnable();
	LOG_DBG("Enabled");
	return 0;
}

static int wdt_cc13xx_cc26xx_setup(const struct device *dev, uint8_t options)
{
	int rv;

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		return -ENOTSUP;
	}

	WatchdogUnlock();
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		WatchdogStallEnable();
	} else {
		WatchdogStallDisable();
	}
	rv = wdt_cc13xx_cc26xx_enable(dev);
	WatchdogLock();
	return rv;
}

static int wdt_cc13xx_cc26xx_disable(const struct device *dev)
{
	return -ENOTSUP;
}

static int wdt_cc13xx_cc26xx_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *cfg)
{
	struct wdt_cc13xx_cc26xx_data *data = dev->data;

	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}
	if (COND_CODE_1(CONFIG_WDT_MULTISTAGE, (cfg->next), (0))) {
		return -ENOTSUP;
	}

	data->reload = cfg->window.max;
	data->cb = cfg->callback;
	data->flags = cfg->flags;
	LOG_DBG("Reload time %d", data->reload);
	return 0;
}

static int wdt_cc13xx_cc26xx_feed(const struct device *dev, int channel_id)
{
	struct wdt_cc13xx_cc26xx_data *data = dev->data;
	const uint32_t reload = wdt_cc13xx_cc26xx_msToTicks(data->reload);
	bool inIsr = k_is_in_isr();

	if (!inIsr) {
		WatchdogUnlock();
	}
	WatchdogIntClear();
	WatchdogReloadSet(reload);
	if (!inIsr) {
		WatchdogLock();
	}
	LOG_DBG("Feed %i", reload);
	return 0;
}

static void wdt_cc13xx_cc26xx_isr(const struct device *dev)
{
	struct wdt_cc13xx_cc26xx_data *data = dev->data;

	LOG_DBG("ISR");
	if (data->cb) {
		data->cb(dev, 0);
	}
}

static int wdt_cc13xx_cc26xx_init(const struct device *dev)
{
	int rv;

	LOG_DBG("init");
	IRQ_CONNECT(DT_INST_IRQN(WDT_INST_IDX), DT_INST_IRQ(WDT_INST_IDX, priority),
			wdt_cc13xx_cc26xx_isr, DEVICE_DT_INST_GET(WDT_INST_IDX), 0);
	irq_enable(DT_INST_IRQN(WDT_INST_IDX));

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		return 0;
	}

	WatchdogUnlock();
	rv = wdt_cc13xx_cc26xx_enable(dev);
	WatchdogLock();
	return rv;
}

static const struct wdt_driver_api wdt_cc13xx_cc26xx_api = {
	.setup = wdt_cc13xx_cc26xx_setup,
	.disable = wdt_cc13xx_cc26xx_disable,
	.install_timeout = wdt_cc13xx_cc26xx_install_timeout,
	.feed = wdt_cc13xx_cc26xx_feed,
};

#if DT_NODE_HAS_STATUS(DT_DRV_INST(WDT_INST_IDX), okay)
DEVICE_DT_INST_DEFINE(WDT_INST_IDX,
			wdt_cc13xx_cc26xx_init, NULL,
			&wdt_cc13xx_cc26xx_data, &wdt_cc13xx_cc26xx_cfg,
			POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&wdt_cc13xx_cc26xx_api);
#endif
