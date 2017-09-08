/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <watchdog.h>
#include <soc.h>
#include <errno.h>

#include "wdt_rda5981a.h"
#include "soc_registers.h"

#define RDA_WDT(__base_addr) \
	(struct wdt_rda5981a *)(__base_addr)

static void wdt_5981a_enable(struct device *dev)
{
	volatile struct wdt_rda5981a *wdt = RDA_WDT(RDA_WDT_BASE);

	wdt->wdt_cfg |= (0x01UL << WDT_EN_BIT);
}

static void wdt_5981a_disable(struct device *dev)
{
	volatile struct wdt_rda5981a *wdt = RDA_WDT(RDA_WDT_BASE);

	/* watchdog cannot be stopped once started */
	wdt->wdt_cfg &= ~(0x01UL << WDT_EN_BIT);
}

static int wdt_5981a_set_config(struct device *dev,
				struct wdt_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);

	/* no configuration */

	return -ENOTSUP;
}

static void wdt_5981a_get_config(struct device *dev,
				struct wdt_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);
}

static void wdt_5981a_reload(struct device *dev)
{
	volatile struct wdt_rda5981a *wdt = RDA_WDT(RDA_WDT_BASE);

	wdt->wdt_cfg |= (0x01UL << WDT_CLR_BIT);
}

static const struct wdt_driver_api wdt_api = {
	.enable = wdt_5981a_enable,
	.disable = wdt_5981a_disable,
	.get_config = wdt_5981a_get_config,
	.set_config = wdt_5981a_set_config,
	.reload = wdt_5981a_reload,
};

static int wdt_init(struct device *dev)
{
	uint32_t reg = 0;
	volatile struct wdt_rda5981a *wdt = RDA_WDT(RDA_WDT_BASE);

	reg = wdt->wdt_cfg &
		~(((0x01UL << WDT_TMRCNT_WIDTH) - 0x01UL) << WDT_TMRCNT_OFST);
	wdt->wdt_cfg = reg | (WDT_TIMER << WDT_TMRCNT_OFST);

	return 0;
}

DEVICE_AND_API_INIT(wdt, CONFIG_WDT_RDA5981A_DEVICE_NAME, wdt_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdt_api);
