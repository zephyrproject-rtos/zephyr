/*
 * Copyright (c) 2022 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/arch/arm/nmi.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wdog_smartbond, CONFIG_WDT_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_smartbond_watchdog

/* driver data */
struct wdog_smartbond_data {
	/* Reload value calculated in setup */
	uint32_t reload_val;
#ifdef CONFIG_WDT_SMARTBOND_NMI
	const struct device *wdog_device;
	wdt_callback_t callback;
#endif
};

static struct wdog_smartbond_data wdog_smartbond_dev_data = {};

static int wdg_smartbond_setup(const struct device *dev, uint8_t options)
{
	ARG_UNUSED(dev);

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		LOG_ERR("Watchdog pause in sleep is not supported");
		return -ENOTSUP;
	}
	return 0;
}

static int wdg_smartbond_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (SYS_WDOG->WATCHDOG_CTRL_REG & SYS_WDOG_WATCHDOG_CTRL_REG_NMI_RST_Msk) {
		/* watchdog cannot be stopped once started when NMI_RST is 1 */
		return -EPERM;
	}

	GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_SYS_WDOG_Msk;
	return 0;
}

#ifdef CONFIG_WDT_SMARTBOND_NMI
static void wdog_smartbond_nmi_isr(void)
{
	if (wdog_smartbond_dev_data.callback) {
		wdog_smartbond_dev_data.callback(wdog_smartbond_dev_data.wdog_device, 0);
	}
}
#endif /* CONFIG_RUNTIME_NMI */

static int wdg_smartbond_install_timeout(const struct device *dev,
					 const struct wdt_timeout_cfg *config)
{
	struct wdog_smartbond_data *data = (struct wdog_smartbond_data *)(dev)->data;
	uint32_t reload_val;

#ifndef CONFIG_WDT_SMARTBOND_NMI
	if (config->callback != NULL) {
		return -ENOTSUP;
	}
#endif
	/* For RC32K timer ticks every ~10ms, for RCX ~21ms */
	if (CRG_TOP->CLK_RCX_REG & CRG_TOP_CLK_RCX_REG_RCX_ENABLE_Msk) {
		reload_val = config->window.max / 21;
	} else {
		reload_val = config->window.max / 10;
	}

	if (reload_val < 1 || reload_val >= 0x2000 || config->window.min != 0) {
		/* Out of range supported by watchdog */
		LOG_ERR("Watchdog timeout out of range");
		return -EINVAL;
	}
#if CONFIG_WDT_SMARTBOND_NMI
	data->callback = config->callback;
	data->wdog_device = dev;
	z_arm_nmi_set_handler(wdog_smartbond_nmi_isr);
	SYS_WDOG->WATCHDOG_CTRL_REG = 2;
#else
	SYS_WDOG->WATCHDOG_CTRL_REG = 2 | SYS_WDOG_WATCHDOG_CTRL_REG_NMI_RST_Msk;
#endif

	data->reload_val = reload_val;
	while (SYS_WDOG->WATCHDOG_CTRL_REG & SYS_WDOG_WATCHDOG_CTRL_REG_WRITE_BUSY_Msk) {
		/* wait */
	}
	SYS_WDOG->WATCHDOG_REG = reload_val;

	return 0;
}

static int wdg_smartbond_feed(const struct device *dev, int channel_id)
{
	struct wdog_smartbond_data *data = (struct wdog_smartbond_data *)(dev)->data;

	ARG_UNUSED(channel_id);

	while (SYS_WDOG->WATCHDOG_CTRL_REG & SYS_WDOG_WATCHDOG_CTRL_REG_WRITE_BUSY_Msk) {
		/* wait */
	}
	SYS_WDOG->WATCHDOG_REG = data->reload_val;

	return 0;
}

static DEVICE_API(wdt, wdg_smartbond_api) = {
	.setup = wdg_smartbond_setup,
	.disable = wdg_smartbond_disable,
	.install_timeout = wdg_smartbond_install_timeout,
	.feed = wdg_smartbond_feed,
};

static int wdg_smartbond_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_SYS_WDOG_Msk;
#endif

	return 0;
}

DEVICE_DT_INST_DEFINE(0, wdg_smartbond_init, NULL, &wdog_smartbond_dev_data, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdg_smartbond_api);
