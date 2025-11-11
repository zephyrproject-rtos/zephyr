/*
 * Copyright (c) 2022 Florin Stancu <niflostancu@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_watchdog

#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <errno.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_cc13xx_cc26xx);

/* Driverlib includes */
#include <driverlib/watchdog.h>


/*
 * TI CC13xx/CC26xx watchdog is a 32-bit timer that runs on the MCU clock
 * with a fixed 32 divider.
 *
 * For the default MCU frequency of 48MHz:
 * 1ms = (48e6 / 32 / 1000) = 1500 ticks
 * Max. value = 2^32 / 1500 ~= 2863311 ms
 *
 * The watchdog will issue reset only on second in turn time-out (if the timer
 * or the interrupt aren't reset after the first time-out). By default, regular
 * interrupt is generated but platform supports also NMI (can be enabled by
 * setting the `interrupt-nmi` boolean DT property).
 */

#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
#define WATCHDOG_DIV_RATIO	 32
#define WATCHDOG_MS_RATIO	 (CPU_FREQ / WATCHDOG_DIV_RATIO / 1000)
#define WATCHDOG_MAX_RELOAD_MS	 (0xFFFFFFFFu / WATCHDOG_MS_RATIO)
#define WATCHDOG_MS_TO_TICKS(_ms) ((_ms) * WATCHDOG_MS_RATIO)

struct wdt_cc13xx_cc26xx_data {
	uint8_t enabled;
	uint32_t reload;
	wdt_callback_t cb;
	uint8_t flags;
};

struct wdt_cc13xx_cc26xx_cfg {
	uint32_t reg;
	uint8_t irq_nmi;
	void (*irq_cfg_func)(void);
};

static int wdt_cc13xx_cc26xx_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *cfg)
{
	struct wdt_cc13xx_cc26xx_data *data = dev->data;

	/* window watchdog not supported */
	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}
	/*
	 * Note: since this SoC doesn't define CONFIG_WDT_MULTISTAGE, we don't need to
	 * specifically check for it and return ENOTSUP
	 */

	if (cfg->window.max > WATCHDOG_MAX_RELOAD_MS) {
		return -EINVAL;
	}
	data->reload = WATCHDOG_MS_TO_TICKS(cfg->window.max);
	data->cb = cfg->callback;
	data->flags = cfg->flags;
	LOG_DBG("raw reload value: %d", data->reload);
	return 0;
}

static int wdt_cc13xx_cc26xx_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_cc13xx_cc26xx_cfg *config = dev->config;
	struct wdt_cc13xx_cc26xx_data *data = dev->data;

	/*
	 * Note: don't check if watchdog is already enabled, an application might
	 * want to dynamically re-configure its options (e.g., decrease the reload
	 * value for critical sections).
	 */

	WatchdogUnlock();

	/* clear any previous interrupt flags */
	WatchdogIntClear();

	/* Stall the WDT counter when halted by debugger */
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		WatchdogStallEnable();
	} else {
		WatchdogStallDisable();
	}
	/*
	 * According to TI's datasheets, the WDT is paused in STANDBY mode,
	 * so we simply continue with the setup => don't do this check:
	 * > if (options & WDT_OPT_PAUSE_IN_SLEEP) {
	 * >	return -ENOTSUP;
	 * > }
	 */

	/* raw reload value was computed by `_install_timeout()` */
	WatchdogReloadSet(data->reload);

	/* use the Device Tree-configured interrupt type */
	if (config->irq_nmi) {
		LOG_DBG("NMI enabled");
		WatchdogIntTypeSet(WATCHDOG_INT_TYPE_NMI);
	} else {
		WatchdogIntTypeSet(WATCHDOG_INT_TYPE_INT);
	}

	switch ((data->flags & WDT_FLAG_RESET_MASK)) {
	case WDT_FLAG_RESET_NONE:
		LOG_DBG("reset disabled");
		WatchdogResetDisable();
		break;
	case WDT_FLAG_RESET_SOC:
		LOG_DBG("reset enabled");
		WatchdogResetEnable();
		break;
	default:
		WatchdogLock();
		return -ENOTSUP;
	}

	data->enabled = 1;
	WatchdogEnable();
	WatchdogLock();

	LOG_DBG("done");
	return 0;
}

static int wdt_cc13xx_cc26xx_disable(const struct device *dev)
{
	struct wdt_cc13xx_cc26xx_data *data = dev->data;

	if (!WatchdogRunning()) {
		return -EFAULT;
	}

	/*
	 * Node: once started, the watchdog timer cannot be stopped!
	 * All we can do is disable the timeout reset, but the interrupt
	 * will be triggered if it was enabled (though it won't trigger the
	 * user callback due to `enabled` being unsed)!
	 */
	data->enabled = 0;
	WatchdogUnlock();
	WatchdogResetDisable();
	WatchdogLock();

	return 0;
}

static int wdt_cc13xx_cc26xx_feed(const struct device *dev, int channel_id)
{
	struct wdt_cc13xx_cc26xx_data *data = dev->data;

	WatchdogUnlock();
	WatchdogIntClear();
	WatchdogReloadSet(data->reload);
	WatchdogLock();
	LOG_DBG("feed %i", data->reload);
	return 0;
}

static void wdt_cc13xx_cc26xx_isr(const struct device *dev)
{
	struct wdt_cc13xx_cc26xx_data *data = dev->data;

	/* Simulate the watchdog being disabled: don't call the handler. */
	if (!data->enabled) {
		return;
	}

	/*
	 * Note: don't clear the interrupt here, leave it for the callback
	 * to decide (by calling `_feed()`)
	 */

	LOG_DBG("ISR");
	if (data->cb) {
		data->cb(dev, 0);
	}
}

static int wdt_cc13xx_cc26xx_init(const struct device *dev)
{
	const struct wdt_cc13xx_cc26xx_cfg *config = dev->config;
	uint8_t options = 0;

	LOG_DBG("init");
	config->irq_cfg_func();

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		return 0;
	}

#ifdef CONFIG_DEBUG
	/* when CONFIG_DEBUG is enabled, pause the WDT during debugging */
	options = WDT_OPT_PAUSE_HALTED_BY_DBG;
#endif /* CONFIG_DEBUG */

	return wdt_cc13xx_cc26xx_setup(dev, options);
}

static DEVICE_API(wdt, wdt_cc13xx_cc26xx_api) = {
	.setup = wdt_cc13xx_cc26xx_setup,
	.disable = wdt_cc13xx_cc26xx_disable,
	.install_timeout = wdt_cc13xx_cc26xx_install_timeout,
	.feed = wdt_cc13xx_cc26xx_feed,
};

#define CC13XX_CC26XX_WDT_INIT(index)						 \
	static void wdt_cc13xx_cc26xx_irq_cfg_##index(void)			 \
	{									 \
		if (DT_INST_PROP(index, interrupt_nmi)) {			 \
			return; /* NMI interrupt is used */			 \
		}								 \
		IRQ_CONNECT(DT_INST_IRQN(index),				 \
			DT_INST_IRQ(index, priority),				 \
			wdt_cc13xx_cc26xx_isr, DEVICE_DT_INST_GET(index), 0);	 \
		irq_enable(DT_INST_IRQN(index));				 \
	}									 \
	static struct wdt_cc13xx_cc26xx_data wdt_cc13xx_cc26xx_data_##index = {	 \
		.reload = WATCHDOG_MS_TO_TICKS(					 \
			CONFIG_WDT_CC13XX_CC26XX_INITIAL_TIMEOUT),		 \
		.cb = NULL,							 \
		.flags = 0,							 \
	};									 \
	static struct wdt_cc13xx_cc26xx_cfg wdt_cc13xx_cc26xx_cfg_##index = {	 \
		.reg = DT_INST_REG_ADDR(index),					 \
		.irq_nmi = DT_INST_PROP(index, interrupt_nmi),			 \
		.irq_cfg_func = wdt_cc13xx_cc26xx_irq_cfg_##index,		 \
	};									 \
	DEVICE_DT_INST_DEFINE(index,						 \
		wdt_cc13xx_cc26xx_init, NULL,					 \
		&wdt_cc13xx_cc26xx_data_##index,				 \
		&wdt_cc13xx_cc26xx_cfg_##index,					 \
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,		 \
		&wdt_cc13xx_cc26xx_api);

DT_INST_FOREACH_STATUS_OKAY(CC13XX_CC26XX_WDT_INIT)
