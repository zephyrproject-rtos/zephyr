/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for CMSDK APB Watchdog.
 */

#include <errno.h>
#include <soc.h>
#include <watchdog.h>
#include <misc/printk.h>
#include <misc/reboot.h>

struct wdog_cmsdk_apb {
	/* offset: 0x000 (r/w) watchdog load register */
	volatile u32_t  load;
	/* offset: 0x004 (r/ ) watchdog value register */
	volatile u32_t  value;
	/* offset: 0x008 (r/w) watchdog control register */
	volatile u32_t  ctrl;
	/* offset: 0x00c ( /w) watchdog clear interrupt register */
	volatile u32_t  intclr;
	/* offset: 0x010 (r/ ) watchdog raw interrupt status register */
	volatile u32_t  rawintstat;
	/* offset: 0x014 (r/ ) watchdog interrupt status register */
	volatile u32_t  maskintstat;
	volatile u32_t  reserved0[762];
	/* offset: 0xc00 (r/w) watchdog lock register */
	volatile u32_t  lock;
	volatile u32_t  reserved1[191];
	/* offset: 0xf00 (r/w) watchdog integration test control register */
	volatile u32_t  itcr;
	/* offset: 0xf04 ( /w) watchdog integration test output set register */
	volatile u32_t  itop;
};

#define CMSDK_APB_WDOG_LOAD		(0xFFFFFFFF << 0)
#define CMSDK_APB_WDOG_RELOAD		(0xE4E1C00 << 0)
#define CMSDK_APB_WDOG_VALUE		(0xFFFFFFFF << 0)
#define CMSDK_APB_WDOG_CTRL_RESEN	(0x1 << 1)
#define CMSDK_APB_WDOG_CTRL_INTEN	(0x1 << 0)
#define CMSDK_APB_WDOG_INTCLR		(0x1 << 0)
#define CMSDK_APB_WDOG_RAWINTSTAT	(0x1 << 0)
#define CMSDK_APB_WDOG_MASKINTSTAT	(0x1 << 0)
#define CMSDK_APB_WDOG_LOCK		(0x1 << 0)
#define CMSDK_APB_WDOG_INTEGTESTEN	(0x1 << 0)
#define CMSDK_APB_WDOG_INTEGTESTOUTSET	(0x1 << 1)

/*
 * Value written to the LOCK register to lock or unlock the write access
 * to all of the registers of the watchdog (except the LOCK register)
 */
#define CMSDK_APB_WDOG_UNLOCK_VALUE (0x1ACCE551)
#define CMSDK_APB_WDOG_LOCK_VALUE (0x2BDDF662)

#define WDOG_STRUCT \
	((volatile struct wdog_cmsdk_apb *)(CMSDK_APB_WDOG))

/* Keep reference of the device to pass it to the callback */
struct device *wdog_r;

/* watchdog reload value in sec */
static unsigned int reload_s = CMSDK_APB_WDOG_RELOAD;
static unsigned int mode;

static void (*user_cb)(struct device *dev);

static void wdog_cmsdk_apb_unlock(struct device *dev)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);

	wdog->lock = CMSDK_APB_WDOG_UNLOCK_VALUE;
}

static void wdog_cmsdk_apb_enable(struct device *dev)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);

	/* Start the watchdog counter with INTEN bit */
	wdog->ctrl = (CMSDK_APB_WDOG_CTRL_RESEN | CMSDK_APB_WDOG_CTRL_INTEN);
}

static int wdog_cmsdk_apb_disable(struct device *dev)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);

	/* Stop the watchdog counter with INTEN bit */
	wdog->ctrl = ~(CMSDK_APB_WDOG_CTRL_RESEN | CMSDK_APB_WDOG_CTRL_INTEN);

	return 0;
}

static int wdog_cmsdk_apb_set_config(struct device *dev,
				     struct wdt_config *config)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);

	/* Reload value */
	reload_s = config->timeout * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	mode = config->mode;

	wdog->load = reload_s;

	/* Configure only the callback */
	user_cb = config->interrupt_fn;

	return 0;
}

static void wdog_cmsdk_apb_get_config(struct device *dev,
				      struct wdt_config *config)
{
	ARG_UNUSED(dev);

	/* Return stored configuration */
	config->timeout = reload_s / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	config->mode = mode;
	config->interrupt_fn = user_cb;
}

static void wdog_cmsdk_apb_reload(struct device *dev)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);

	/* Clear the interrupt */
	wdog->intclr = CMSDK_APB_WDOG_INTCLR;

	/* Reload */
	wdog->load = reload_s;
}

static const struct wdt_driver_api wdog_cmsdk_apb_api = {
	.enable = wdog_cmsdk_apb_enable,
	.disable = wdog_cmsdk_apb_disable,
	.get_config = wdog_cmsdk_apb_get_config,
	.set_config = wdog_cmsdk_apb_set_config,
	.reload = wdog_cmsdk_apb_reload,
};

#ifdef CONFIG_RUNTIME_NMI
extern void _NmiHandlerSet(void (*pHandler)(void));

static int wdog_cmsdk_apb_has_fired(void)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	return (wdog->maskintstat & CMSDK_APB_WDOG_MASKINTSTAT) != 0;
}

static void wdog_cmsdk_apb_isr(void)
{
	/*
	 * Check if the watchdog was the reason of the NMI interrupt
	 * and if not, exit immediately
	 */
	if (!wdog_cmsdk_apb_has_fired()) {
		printk("NMI received! Rebooting...\n");
		/* In ARM implementation sys_reboot ignores the parameter */
		sys_reboot(0);
	} else {
		if (user_cb != NULL) {
			user_cb(wdog_r);
		}
	}
}
#endif /* CONFIG_RUNTIME_NMI */

static int wdog_cmsdk_apb_init(struct device *dev)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	wdog_r = dev;

	/* unlock access to configuration registers */
	wdog_cmsdk_apb_unlock(dev);

	/* set default reload value */
	wdog->load = reload_s;

#ifdef CONFIG_RUNTIME_NMI
	/* Configure the interrupts */
	_NmiHandlerSet(wdog_cmsdk_apb_isr);
#endif

#ifdef CONFIG_WDOG_CMSDK_APB_START_AT_BOOT
	wdog_cmsdk_apb_enable(dev);
#endif

	return 0;
}

DEVICE_AND_API_INIT(wdog_cmsdk_apb, CONFIG_WDT_0_NAME,
		    wdog_cmsdk_apb_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdog_cmsdk_apb_api);
