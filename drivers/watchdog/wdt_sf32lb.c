/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT sifli_sf32lb_wdt

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

#include <register.h>

LOG_MODULE_REGISTER(wdt_sf32lb, CONFIG_WDT_LOG_LEVEL);

#define WDT_CVR0        offsetof(WDT_TypeDef, WDT_CVR0)
#define WDT_CR          offsetof(WDT_TypeDef, WDT_CR)
#define WDT_CCR         offsetof(WDT_TypeDef, WDT_CCR)

#define WDT_CMD_START 0x00000076U
#define WDT_CMD_STOP  0x00000034U

#define WDT_CVR0_MAX 0xFFFFFF

#define WDT_WDT_CR_RESPONSE_MODE1 0U

/* Assume LRC10 clocks WDT (LRC32 support to be added in the future) */
#define WDT_CLK_KHZ 10

#define WDT_WINDOW_MS_MAX (WDT_CVR0_MAX / WDT_CLK_KHZ)

struct wdt_sf32lb_config {
	uintptr_t base;
};

static int wdt_sf32lb_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_sf32lb_config *config = dev->config;

	if (options != 0U) {
		LOG_ERR("Options not supported");
		return -ENOTSUP;
	}

	sys_write32(WDT_CMD_START, config->base + WDT_CCR);

	return 0;
}

static int wdt_sf32lb_disable(const struct device *dev)
{
	const struct wdt_sf32lb_config *config = dev->config;

	sys_write32(WDT_CMD_STOP, config->base + WDT_CCR);

	return 0;
}

static int wdt_sf32lb_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *wdt_cfg)
{
	const struct wdt_sf32lb_config *config = dev->config;

	if (wdt_cfg->flags != WDT_FLAG_RESET_SOC) {
		LOG_ERR("Only SoC reset supported");
		return -ENOTSUP;
	}

	if (wdt_cfg->callback != NULL) {
		LOG_ERR("Callback not supported");
		return -ENOTSUP;
	}

	if (wdt_cfg->window.min != 0U) {
		LOG_ERR("Window mode not supported!");
		return -ENOTSUP;
	};

	if (wdt_cfg->window.max > WDT_WINDOW_MS_MAX) {
		return -EINVAL;
	}

	sys_write32(wdt_cfg->window.max * WDT_CLK_KHZ, config->base + WDT_CVR0);

	return 0;
}

static int wdt_sf32lb_feed(const struct device *dev, int channel_id)
{
	const struct wdt_sf32lb_config *config = dev->config;

	sys_write32(WDT_CMD_START, config->base + WDT_CCR);

	return 0;
}

static DEVICE_API(wdt, wdt_sf32lb_api) = {
	.setup = wdt_sf32lb_setup,
	.disable = wdt_sf32lb_disable,
	.install_timeout = wdt_sf32lb_install_timeout,
	.feed = wdt_sf32lb_feed,
};

static int wdt_sf32lb_init(const struct device *dev)
{
	const struct wdt_sf32lb_config *config = dev->config;
	uint32_t cr;

	cr = sys_read32(config->base + WDT_CR);
	cr &= ~WDT_WDT_CR_RESPONSE_MODE_Msk;
	cr |= WDT_WDT_CR_RESPONSE_MODE1;
	sys_write32(cr, config->base + WDT_CR);

	return 0;
}

#define WDT_SF32LB_INIT(index)                                                                     \
	static const struct wdt_sf32lb_config wdt_sf32lb_config_##index = {                        \
		.base = DT_INST_REG_ADDR(index),                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, wdt_sf32lb_init, NULL, NULL,                                  \
			      &wdt_sf32lb_config_##index, POST_KERNEL,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_sf32lb_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_SF32LB_INIT)
