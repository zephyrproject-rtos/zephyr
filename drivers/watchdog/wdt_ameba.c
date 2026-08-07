/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_ameba_watchdog

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_ameba, CONFIG_WDT_LOG_LEVEL);

struct wdt_ameba_data {
	uint32_t timeout;
	wdt_callback_t callback;
	bool started;
};

struct wdt_ameba_config {
	DEVICE_MMIO_ROM;
	void (*irq_config_func)(const struct device *dev);
	uint32_t eicnt;
	bool auto_enabled;
};

static inline WDG_TypeDef *wdt_ameba_get_base(const struct device *dev)
{
	return (WDG_TypeDef *)DEVICE_MMIO_GET(dev);
}

static void wdt_ameba_isr(const struct device *dev);

static int wdt_ameba_disable(const struct device *dev)
{
	struct wdt_ameba_data *data = dev->data;

	if (!data->started) {
		return -EFAULT;
	}

	/*
	 * The WDG on Ameba SoCs uses a magic-key mechanism; once enabled via
	 * WDG_FUNC_EN (0x3C3C) there is no documented key to disable it.
	 * Hardware cannot be stopped by software.
	 */
	return -ENOTSUP;
}

static int wdt_ameba_feed(const struct device *dev, int channel_id)
{
	struct wdt_ameba_data *data = dev->data;
	WDG_TypeDef *wdg = wdt_ameba_get_base(dev);

	if (channel_id != 0) {
		return -EINVAL;
	}

	if (!data->started) {
		return -EINVAL;
	}

	WDG_Refresh(wdg);

	return 0;
}

static int wdt_ameba_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_ameba_config *config = dev->config;
	struct wdt_ameba_data *data = dev->data;
	WDG_TypeDef *wdg = wdt_ameba_get_base(dev);
	WDG_InitTypeDef wdg_initstruct;

	if (options != 0) {
		return -ENOTSUP;
	}

	if (data->started) {
		return -EBUSY;
	}

	if (data->timeout == 0) {
		LOG_ERR("no timeout installed, call wdt_install_timeout() first");
		return -EINVAL;
	}

	WDG_StructInit(&wdg_initstruct);
	wdg_initstruct.Window  = 0;
	wdg_initstruct.Timeout = (u16)data->timeout;
	wdg_initstruct.EICNT   = (u16)config->eicnt;
	wdg_initstruct.EIMOD   = ENABLE;
	WDG_Init(wdg, &wdg_initstruct);
	WDG_Enable(wdg);

	data->started = true;

	return 0;
}

static int wdt_ameba_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct wdt_ameba_data *data = dev->data;

	if (data->started) {
		return -EBUSY;
	}

	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	if (cfg->flags == WDT_FLAG_RESET_NONE) {
		return -ENOTSUP;
	}

	data->timeout = cfg->window.max;
	data->callback = cfg->callback;

	return 0;
}

static int wdt_ameba_init(const struct device *dev)
{
	const struct wdt_ameba_config *config = dev->config;
	struct wdt_ameba_data *data = dev->data;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (config->auto_enabled) {
		data->started = true;
	}

	config->irq_config_func(dev);

	return 0;
}

static void wdt_ameba_isr(const struct device *dev)
{
	struct wdt_ameba_data *data = dev->data;
	WDG_TypeDef *wdg = wdt_ameba_get_base(dev);
	wdt_callback_t cb = data->callback;

	if (cb != NULL) {
		cb(dev, 0);
	}

	WDG_INTConfig(wdg, WDG_BIT_EIE, DISABLE);
	WDG_ClearINT(wdg, WDG_BIT_EIC);
}

static DEVICE_API(wdt, wdt_ameba_api) = {
	.setup = wdt_ameba_setup,
	.disable = wdt_ameba_disable,
	.install_timeout = wdt_ameba_install_timeout,
	.feed = wdt_ameba_feed,
};

#define WDT_IRQ_CONFIG(n)                                                                          \
	static void irq_config_##n(const struct device *dev)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), wdt_ameba_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define AMEBA_WDT_INIT(n)                                                                          \
	WDT_IRQ_CONFIG(n)                                                                          \
	static struct wdt_ameba_data wdt##n##_data;                                                \
	static const struct wdt_ameba_config wdt_ameba_config##n = {                               \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.irq_config_func = irq_config_##n,                                                 \
		.eicnt = DT_INST_PROP(n, early_int_cnt_ms),                                        \
		.auto_enabled = DT_INST_PROP(n, realtek_auto_enabled),                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, wdt_ameba_init, NULL, &wdt##n##_data, &wdt_ameba_config##n,       \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_ameba_api)

DT_INST_FOREACH_STATUS_OKAY(AMEBA_WDT_INIT)
