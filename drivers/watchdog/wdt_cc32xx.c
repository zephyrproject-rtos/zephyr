/*
 * Copyright (C) 2017 Matthias Boesl  <matthias.boesl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Watchdog (WDT) Driver for CC32xx devices
 *
 */

#include <watchdog.h>
#include <soc.h>
#include <misc/printk.h>

/* Driverlib includes */
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_ints.h>
#include <driverlib/rom.h>
#include <driverlib/pin.h>
#include <driverlib/rom_map.h>

#ifdef CONFIG_CC3200SDK_ROM_DRIVERLIB
#include <inc/hw_wdt.h>
#endif

#define SYS_LOG_DOMAIN "dev/wdt_cc32xx"
#include <logging/sys_log.h>

/* Note: Zephyr uses exception numbers, vs the IRQ #s used by the CC3200 SDK */
#define EXCEPTION_WDT0  18  /* (INT_WDT - 16) = (34-16) */

static void (*user_cb)(struct device *dev);

/* Keep reference of the device to pass it to the callback */
struct device *wdog_r;

static void wdt_cc32xx_enable(struct device *dev)
{
	ARG_UNUSED(dev);
	MAP_WatchdogEnable(WDT_BASE);
}

static void wdt_cc32xx_disable(struct device *dev)
{
	ARG_UNUSED(dev);
}

static void wdt_cc32xx_isr(void)
{
	MAP_WatchdogIntClear(WDT_BASE);

	if (user_cb != NULL) {
		user_cb(wdog_r);
	}
}

static int wdt_cc32xx_set_config(struct device *dev, struct wdt_config *config)
{
	ARG_UNUSED(dev);
	/* Configure only the callback */
	user_cb = config->interrupt_fn;
	return 0;
}

static void wdt_cc32xx_get_config(struct device *dev, struct wdt_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(config);
	SYS_LOG_ERR("Function not implemented!");
}

static void wdt_cc32xx_reload(struct device *dev)
{
	ARG_UNUSED(dev);
	MAP_WatchdogUnlock(WDT_BASE);
	MAP_WatchdogReloadSet(WDT_BASE, CONFIG_WDT_CC32XX_RELOAD_TIME *
			      CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	MAP_WatchdogLock(WDT_BASE);
}

static const struct wdt_driver_api wdt_cc32xx_api = {
	.enable = wdt_cc32xx_enable,
	.disable = wdt_cc32xx_disable,
	.get_config = wdt_cc32xx_get_config,
	.set_config = wdt_cc32xx_set_config,
	.reload = wdt_cc32xx_reload
};

static struct device DEVICE_NAME_GET(wdt_cc32xx);

static int wdt_cc32xx_init(struct device *dev)
{
	/* store ref for callback */
	wdog_r = dev;

	MAP_PRCMPeripheralClkEnable(PRCM_WDT,
				    PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);
	/* spin until status returns TRUE */
	while(!MAP_PRCMPeripheralStatusGet(PRCM_WDT)) {
	}

	MAP_WatchdogUnlock(WDT_BASE);
	MAP_WatchdogReloadSet(WDT_BASE, CONFIG_WDT_CC32XX_RELOAD_TIME *
			      CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	MAP_WatchdogIntClear(WDT_BASE);

	/* Set debug stall mode */
#ifdef CONFIG_WDT_CC32XX_DEBUGGER_STALL_MODE
	MAP_WatchdogStallEnable(WDT_BASE);
#endif

	IRQ_CONNECT(EXCEPTION_WDT0, CONFIG_WDT_CC32XX_IRQ_PRI,
		    wdt_cc32xx_isr, DEVICE_GET(wdt_cc32xx), 0);

	MAP_IntPendClear(INT_WDT);
	irq_enable(EXCEPTION_WDT0);

	MAP_WatchdogLock(WDT_BASE);

	return 0;
}

DEVICE_AND_API_INIT(wdt_cc32xx, CONFIG_WDT_0_NAME, wdt_cc32xx_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdt_cc32xx_api);
