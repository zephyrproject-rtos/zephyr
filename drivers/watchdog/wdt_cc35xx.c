/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_watchdog

#include <soc.h>
#include <errno.h>
#include <stdint.h>

#include <zephyr/irq.h>
#include <zephyr/drivers/watchdog.h>

/* Driverlib includes */
#include <inc/hw_types.h>
#include <driverlib/watchdog.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(wdt_cc35xx);

#define LFCLK_FREQ_HZ                DT_INST_PROP_BY_PHANDLE(0, clocks, clock_frequency)
#define CC35XX_MAX_RELOAD_TICKS      0x7FFFFF
#define CC35XX_MIN_RELOAD_TICKS      0x2
#define CC35XX_WATCHDOG_TICK_FREQ_HZ (LFCLK_FREQ_HZ >> (HOSTMCU_AON_CFGWDT_THR_S))

struct wdt_cc35xx_data {
	uint32_t reload;
};

static uint32_t wdt_cc35xx_ms_to_ticks(uint32_t ms)
{
	uint32_t us_per_tick = 1000000l / CC35XX_WATCHDOG_TICK_FREQ_HZ;
	uint64_t us = ms * 1000ll;
	uint32_t ticks = us / us_per_tick;

	ticks = MAX(ticks, CC35XX_MIN_RELOAD_TICKS);
	ticks = MIN(ticks, CC35XX_MAX_RELOAD_TICKS);

	return ticks;
}

static int wdt_cc35xx_enable(const struct device *dev)
{
	struct wdt_cc35xx_data *data = dev->data;
	const uint32_t reload = wdt_cc35xx_ms_to_ticks(data->reload);

	WatchdogStopSequence();
	WatchdogDisableResetEvent();

	WatchdogSetResetThreshold(reload);
	WatchdogEnableResetEvent();

	WatchdogStartSequence();

	return 0;
}

static int wdt_cc35xx_setup(const struct device *dev, uint8_t options)
{
	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		return -ENOTSUP;
	}

	return wdt_cc35xx_enable(dev);
}

static int wdt_cc35xx_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	WatchdogStopSequence();
	return 0;
}

static int wdt_cc35xx_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct wdt_cc35xx_data *data = dev->data;

	if (COND_CODE_1(CONFIG_WDT_MULTISTAGE, (cfg->next), (0))) {
		return -ENOTSUP;
	}
	if (cfg->window.max == 0) {
		return -EINVAL;
	}
	if (!(cfg->flags & WDT_FLAG_RESET_SOC)) {
		/* Due to hardware bug, watchdog supports only hardware
		 * reset. Software interrupt with no reset is disabled.
		 */
		return -ENOTSUP;
	}

	data->reload = cfg->window.max;
	return 0;
}

static int wdt_cc35xx_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	WatchdogReset();
	return 0;
}

static void wdt_cc35xx_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int wdt_cc35xx_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), wdt_cc35xx_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		return 0;
	}

	WatchdogSetSwMode();
	WatchdogStopSequence();
	WatchdogDisableResetEvent();
	WatchdogClearWakeUpEvent();
	WatchdogDisableWakeUpEvent();

	return 0;
}

static DEVICE_API(wdt, wdt_cc35xx_api) = {
	.setup = wdt_cc35xx_setup,
	.disable = wdt_cc35xx_disable,
	.install_timeout = wdt_cc35xx_install_timeout,
	.feed = wdt_cc35xx_feed,
};

#define CC35XX_WDT_INIT(n)                                                                         \
	static struct wdt_cc35xx_data wdt_cc35xx_data_##n = {                                      \
		.reload = 0,                                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &wdt_cc35xx_init, NULL, &wdt_cc35xx_data_##n, NULL, POST_KERNEL,  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wdt_cc35xx_api);

DT_INST_FOREACH_STATUS_OKAY(CC35XX_WDT_INIT);
