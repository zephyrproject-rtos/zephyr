/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_wdog

#include <soc.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <em_wdog.h>
#include <em_cmu.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(wdt_gecko, CONFIG_WDT_LOG_LEVEL);

#ifdef cmuClock_CORELE
#define CLOCK_DEF(id) cmuClock_CORELE
#else
#define CLOCK_DEF(id) cmuClock_WDOG##id
#endif /* cmuClock_CORELE */
#define CLOCK_ID(id) CLOCK_DEF(id)

/* Defines maximum WDOG_CTRL.PERSEL value which is used by the watchdog module
 * to select its timeout period.
 */
#define WDT_GECKO_MAX_PERIOD_SELECT_VALUE 15

/* Device constant configuration parameters */
struct wdt_gecko_cfg {
	WDOG_TypeDef *base;
	CMU_Clock_TypeDef clock;
	void (*irq_cfg_func)(void);
};

struct wdt_gecko_data {
	wdt_callback_t callback;
	WDOG_Init_TypeDef wdog_config;
	bool timeout_installed;
};

static uint32_t wdt_gecko_get_timeout_from_persel(int perSel)
{
	return (8 << perSel) + 1;
}

/* Find the rounded up value of cycles for supplied timeout. When using ULFRCO
 * (default), 1 cycle is 1 ms +/- 12%.
 */
static int wdt_gecko_get_persel_from_timeout(uint32_t timeout)
{
	int idx;

	for (idx = 0; idx < WDT_GECKO_MAX_PERIOD_SELECT_VALUE; idx++) {
		if (wdt_gecko_get_timeout_from_persel(idx) >= timeout) {
			break;
		}
	}

	return idx;
}

static int wdt_gecko_convert_window(uint32_t window, uint32_t period)
{
	int idx = 0;
	uint32_t incr_val, comp_val;

	incr_val = period / 8;
	comp_val = 0; /* Initially 0, disable */

	/* Valid window settings range from 12.5% of the calculated
	 * timeout period up to 87.5% (= 7 * 12.5%)
	 */
	while (idx < 7) {
		if (window > comp_val) {
			comp_val += incr_val;
			idx++;
			continue;
		}

		break;
	}

	return idx;
}

static int wdt_gecko_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_gecko_cfg *config = dev->config;
	struct wdt_gecko_data *data = dev->data;
	WDOG_TypeDef *wdog = config->base;

	if (!data->timeout_installed) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

#if defined(_WDOG_CFG_EM1RUN_MASK)
	data->wdog_config.em1Run =
		(options & WDT_OPT_PAUSE_IN_SLEEP) == 0U;
#endif
	data->wdog_config.em2Run =
		(options & WDT_OPT_PAUSE_IN_SLEEP) == 0U;
	data->wdog_config.em3Run =
		(options & WDT_OPT_PAUSE_IN_SLEEP) == 0U;

	data->wdog_config.debugRun =
		(options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0U;

	if (data->callback != NULL) {
		/* Interrupt mode for window */
		/* Clear possible lingering interrupts */
		WDOGn_IntClear(wdog, WDOG_IEN_TOUT);
		/* Enable timeout interrupt */
		WDOGn_IntEnable(wdog, WDOG_IEN_TOUT);
	} else {
		/* Disable timeout interrupt */
		WDOGn_IntDisable(wdog, WDOG_IEN_TOUT);
	}

	/* Watchdog is started after initialization */
	WDOGn_Init(wdog, &data->wdog_config);
	LOG_DBG("Setup the watchdog");

	return 0;
}

static int wdt_gecko_disable(const struct device *dev)
{
	const struct wdt_gecko_cfg *config = dev->config;
	struct wdt_gecko_data *data = dev->data;
	WDOG_TypeDef *wdog = config->base;

	WDOGn_Enable(wdog, false);
	data->timeout_installed = false;
	LOG_DBG("Disabled the watchdog");

	return 0;
}

static int wdt_gecko_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *cfg)
{
	struct wdt_gecko_data *data = dev->data;
	data->wdog_config = (WDOG_Init_TypeDef)WDOG_INIT_DEFAULT;
	uint32_t installed_timeout;

	if (data->timeout_installed) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	if ((cfg->window.max < wdt_gecko_get_timeout_from_persel(0)) ||
		(cfg->window.max > wdt_gecko_get_timeout_from_persel(
			WDT_GECKO_MAX_PERIOD_SELECT_VALUE))) {
		LOG_ERR("Upper limit timeout out of range");
		return -EINVAL;
	}

#if defined(_WDOG_CTRL_CLKSEL_MASK)
	data->wdog_config.clkSel = wdogClkSelULFRCO;
#endif

	data->wdog_config.perSel = (WDOG_PeriodSel_TypeDef)
		wdt_gecko_get_persel_from_timeout(cfg->window.max);

	installed_timeout = wdt_gecko_get_timeout_from_persel(
		data->wdog_config.perSel);
	LOG_INF("Installed timeout value: %u", installed_timeout);

	if (cfg->window.min > 0) {
		/* Window mode. Use rounded up timeout value to
		 * calculate minimum window setting.
		 */
		data->wdog_config.winSel = (WDOG_WinSel_TypeDef)
			wdt_gecko_convert_window(cfg->window.min,
						installed_timeout);

		LOG_INF("Installed window value: %u",
			(installed_timeout / 8) * data->wdog_config.winSel);
	} else {
		/* Normal mode */
		data->wdog_config.winSel = wdogIllegalWindowDisable;
	}

	/* Set mode of watchdog and callback */
	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
	case WDT_FLAG_RESET_CPU_CORE:
		if (cfg->callback != NULL) {
			LOG_ERR("Reset mode with callback not supported\n");
			return -ENOTSUP;
		}
		data->wdog_config.resetDisable = false;
		LOG_DBG("Configuring reset CPU/SoC mode\n");
		break;

	case WDT_FLAG_RESET_NONE:
		data->wdog_config.resetDisable = true;
		data->callback = cfg->callback;
		LOG_DBG("Configuring non-reset mode\n");
		break;

	default:
		LOG_ERR("Unsupported watchdog config flag");
		return -EINVAL;
	}

	data->timeout_installed = true;

	return 0;
}

static int wdt_gecko_feed(const struct device *dev, int channel_id)
{
	const struct wdt_gecko_cfg *config = dev->config;
	WDOG_TypeDef *wdog = config->base;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	WDOGn_Feed(wdog);
	LOG_DBG("Fed the watchdog");

	return 0;
}

static void wdt_gecko_isr(const struct device *dev)
{
	const struct wdt_gecko_cfg *config = dev->config;
	struct wdt_gecko_data *data = dev->data;
	WDOG_TypeDef *wdog = config->base;
	uint32_t flags;

	/* Clear IRQ flags */
	flags = WDOGn_IntGet(wdog);
	WDOGn_IntClear(wdog, flags);

	if (data->callback != NULL) {
		data->callback(dev, 0);
	}
}

static int wdt_gecko_init(const struct device *dev)
{
	const struct wdt_gecko_cfg *config = dev->config;

#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	/* Ignore any errors */
	wdt_gecko_disable(dev);
#endif

	/* Enable ULFRCO (1KHz) oscillator */
	CMU_OscillatorEnable(cmuOsc_ULFRCO, true, false);

	/* Ensure LE modules are clocked */
	CMU_ClockEnable(config->clock, true);

#if defined(_SILICON_LABS_32B_SERIES_2)
	CMU_ClockSelectSet(config->clock, cmuSelect_ULFRCO);
	/* Enable Watchdog clock. */
	CMU_ClockEnable(cmuClock_WDOG0, true);
#endif

	/* Enable IRQs */
	config->irq_cfg_func();

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static DEVICE_API(wdt, wdt_gecko_driver_api) = {
	.setup = wdt_gecko_setup,
	.disable = wdt_gecko_disable,
	.install_timeout = wdt_gecko_install_timeout,
	.feed = wdt_gecko_feed,
};

#define GECKO_WDT_INIT(index)						\
									\
	static void wdt_gecko_cfg_func_##index(void);			\
									\
	static const struct wdt_gecko_cfg wdt_gecko_cfg_##index = {	\
		.base = (WDOG_TypeDef *)				\
			DT_INST_REG_ADDR(index),\
		.clock = CLOCK_ID(DT_INST_PROP(index, peripheral_id)),  \
		.irq_cfg_func = wdt_gecko_cfg_func_##index,		\
	};								\
	static struct wdt_gecko_data wdt_gecko_data_##index;		\
									\
	DEVICE_DT_INST_DEFINE(index,					\
				&wdt_gecko_init, NULL,			\
				&wdt_gecko_data_##index,		\
				&wdt_gecko_cfg_##index, POST_KERNEL,	\
				CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
				&wdt_gecko_driver_api);			\
									\
	static void wdt_gecko_cfg_func_##index(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(index),	\
			DT_INST_IRQ(index, priority),\
			wdt_gecko_isr, DEVICE_DT_INST_GET(index), 0);	\
		irq_enable(DT_INST_IRQN(index));	\
	}

DT_INST_FOREACH_STATUS_OKAY(GECKO_WDT_INIT)
