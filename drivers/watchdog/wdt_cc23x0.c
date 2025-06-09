/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_wdt

#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <errno.h>

#include <inc/hw_ckmd.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wdt_cc23x0);

#define CC23X0_WDT_UNLOCK(_base)        (HWREG((_base) + CKMD_O_WDTLOCK) = 0x1ACCE551)
#define CC23X0_WDT_LOCK(_base)          (HWREG((_base) + CKMD_O_WDTLOCK) = 0x1)
#define CC23X0_WDT_FEED(_base, _value)  (HWREG((_base) + CKMD_O_WDTCNT) = (_value))
#define CC23X0_WDT_STALL_ENABLE(_base)  (HWREG((_base) + CKMD_O_WDTTEST) = 0x1)
#define CC23X0_WDT_STALL_DISABLE(_base) (HWREG((_base) + CKMD_O_WDTTEST) = 0x0)

#define WDT_SOURCE_FREQ      32768
#define WDT_MS_RATIO         (WDT_SOURCE_FREQ / 1000)
#define WDT_MAX_RELOAD_MS    0xffffffff / WDT_MS_RATIO
#define WDT_MS_TO_TICKS(_ms) ((_ms) * WDT_MS_RATIO)

struct wdt_cc23x0_data {
	uint8_t enabled;
	uint32_t reload;
	uint8_t flags;
};

struct wdt_cc23x0_config {
	uint32_t base;
};

static int wdt_cc23x0_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct wdt_cc23x0_data *data = dev->data;

	/* window watchdog not supported */
	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	if (cfg->window.max > WDT_MAX_RELOAD_MS) {
		return -EINVAL;
	}
	data->reload = WDT_MS_TO_TICKS(cfg->window.max);
	data->flags = cfg->flags;

	LOG_DBG("raw reload value: %d", data->reload);

	return 0;
}

static int wdt_cc23x0_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_cc23x0_config *config = dev->config;

	/* Stall the WDT counter when halted by debugger */
	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) != 0U) {
		CC23X0_WDT_STALL_ENABLE(config->base);
	} else {
		CC23X0_WDT_STALL_DISABLE(config->base);
	}

	return 0;
}

static int wdt_cc23x0_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static int wdt_cc23x0_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);

	const struct wdt_cc23x0_config *config = dev->config;
	struct wdt_cc23x0_data *data = dev->data;

	CC23X0_WDT_UNLOCK(config->base);
	CC23X0_WDT_FEED(config->base, data->reload);

	return 0;
}

static int wdt_cc23x0_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static DEVICE_API(wdt, wdt_cc23x0_api) = {
	.setup = wdt_cc23x0_setup,
	.disable = wdt_cc23x0_disable,
	.install_timeout = wdt_cc23x0_install_timeout,
	.feed = wdt_cc23x0_feed,
};

#define CC23X0_WDT_INIT(index)                                                                     \
	static struct wdt_cc23x0_data wdt_cc23x0_data_##index = {                                  \
		.reload = WDT_MS_TO_TICKS(CONFIG_WDT_CC23X0_INITIAL_TIMEOUT),                      \
	};                                                                                         \
	static struct wdt_cc23x0_config wdt_cc23x0_config_##index = {                              \
		.base = DT_INST_REG_ADDR(index),                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, wdt_cc23x0_init, NULL, &wdt_cc23x0_data_##index,              \
			      &wdt_cc23x0_config_##index, POST_KERNEL,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wdt_cc23x0_api);

DT_INST_FOREACH_STATUS_OKAY(CC23X0_WDT_INIT)
