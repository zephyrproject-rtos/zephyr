/*
 * Copyright (C) 2017 Intel Deutschland GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Watchdog (WDT) Driver for Atmel SAM MCUs
 *
 * Note:
 * - Once the watchdog disable bit is set, it cannot be cleared till next
 *   power reset, i.e, the watchdog cannot be started once stopped.
 * - Since the MCU boots with WDT enabled, only basic feature (disable) is
 *   currently implemented for systems that don't need watchdog functionality.
 *
 */

#include <watchdog.h>
#include <soc.h>

#define SYS_LOG_DOMAIN "dev/wdt_sam"
#include <logging/sys_log.h>

/* Device constant configuration parameters */
struct wdt_sam_dev_cfg {
	Wdt *regs;
};

#define DEV_CFG(dev) \
	((const struct wdt_sam_dev_cfg *const)(dev)->config->config_info)

static void wdt_sam_enable(struct device *dev)
{
	ARG_UNUSED(dev);

	SYS_LOG_ERR("Function not implemented!");
}

static int wdt_sam_disable(struct device *dev)
{
	Wdt *const wdt = DEV_CFG(dev)->regs;

	wdt->WDT_MR |= WDT_MR_WDDIS;

	return 0;
}

static int wdt_sam_set_config(struct device *dev, struct wdt_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);

	SYS_LOG_ERR("Function not implemented!");

	return -ENOTSUP;
}

static void wdt_sam_get_config(struct device *dev, struct wdt_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);

	SYS_LOG_ERR("Function not implemented!");
}

static void wdt_sam_reload(struct device *dev)
{
	ARG_UNUSED(dev);

	SYS_LOG_ERR("Function not implemented!");
}

static const struct wdt_driver_api wdt_sam_api = {
	.enable = wdt_sam_enable,
	.disable = wdt_sam_disable,
	.get_config = wdt_sam_get_config,
	.set_config = wdt_sam_set_config,
	.reload = wdt_sam_reload
};

static int wdt_sam_init(struct device *dev)
{
#ifdef CONFIG_WDT_SAM_DISABLE_AT_BOOT
	wdt_sam_disable(dev);
#endif
	return 0;
}

static const struct wdt_sam_dev_cfg wdt_sam_config = {
	.regs = WDT
};

DEVICE_AND_API_INIT(wdt_sam, CONFIG_WDT_0_NAME, wdt_sam_init,
		    NULL, &wdt_sam_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdt_sam_api);
