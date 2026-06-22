/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_cmsdk_watchdog

/**
 * @brief Driver for CMSDK APB Watchdog.
 */

#include <errno.h>
#include <soc.h>
#include <zephyr/arch/arm/nmi.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>

struct wdog_cmsdk_apb {
	/* offset: 0x000 (r/w) watchdog load register */
	volatile uint32_t  load;
	/* offset: 0x004 (r/ ) watchdog value register */
	volatile uint32_t  value;
	/* offset: 0x008 (r/w) watchdog control register */
	volatile uint32_t  ctrl;
	/* offset: 0x00c ( /w) watchdog clear interrupt register */
	volatile uint32_t  intclr;
	/* offset: 0x010 (r/ ) watchdog raw interrupt status register */
	volatile uint32_t  rawintstat;
	/* offset: 0x014 (r/ ) watchdog interrupt status register */
	volatile uint32_t  maskintstat;
	volatile uint32_t  reserved0[762];
	/* offset: 0xc00 (r/w) watchdog lock register */
	volatile uint32_t  lock;
	volatile uint32_t  reserved1[191];
	/* offset: 0xf00 (r/w) watchdog integration test control register */
	volatile uint32_t  itcr;
	/* offset: 0xf04 ( /w) watchdog integration test output set register */
	volatile uint32_t  itop;
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
	((volatile struct wdog_cmsdk_apb *)(DT_INST_REG_ADDR(0)))

/* Keep reference of the device to pass it to the callback */
const struct device *wdog_r;

/* watchdog reload value in clock cycles */
static unsigned int reload_cycles = CMSDK_APB_WDOG_RELOAD;
static uint8_t assigned_channels;
static uint8_t flags;
static bool enabled;

static void (*user_cb)(const struct device *dev, int channel_id);

static void wdog_cmsdk_apb_unlock(const struct device *dev)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);

	wdog->lock = CMSDK_APB_WDOG_UNLOCK_VALUE;
}

static int wdog_cmsdk_apb_setup(const struct device *dev, uint8_t options)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);
	ARG_UNUSED(options);

	/* Check if watchdog is already running */
	if (enabled) {
		return -EBUSY;
	}

	/* Reset pending interrupts before starting */
	wdog->intclr = CMSDK_APB_WDOG_INTCLR;
	wdog->load = reload_cycles;

	/* Start the watchdog counter with INTEN bit */
	wdog->ctrl = (CMSDK_APB_WDOG_CTRL_RESEN | CMSDK_APB_WDOG_CTRL_INTEN);

	enabled = true;
	return 0;
}

static int wdog_cmsdk_apb_disable(const struct device *dev)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);

	/* Stop the watchdog counter with INTEN bit */
	wdog->ctrl = ~(CMSDK_APB_WDOG_CTRL_RESEN | CMSDK_APB_WDOG_CTRL_INTEN);

	enabled = false;
	assigned_channels = 0;

	return 0;
}

static int wdog_cmsdk_apb_install_timeout(const struct device *dev,
					  const struct wdt_timeout_cfg *config)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;
	uint32_t clk_freq_khz = DT_INST_PROP_BY_PHANDLE(0, clocks, clock_frequency) / 1000;

	ARG_UNUSED(dev);

	if (config->window.max == 0) {
		return -EINVAL;
	}
	if (enabled == true) {
		return -EBUSY;
	}
	if (assigned_channels == 1) {
		return -ENOMEM;
	}

	/* Reload value */
	reload_cycles = config->window.max * clk_freq_khz;
	flags = config->flags;

	wdog->load = reload_cycles;

	/* Configure only the callback */
	user_cb = config->callback;

	assigned_channels++;
	return 0;
}

static int wdog_cmsdk_apb_feed(const struct device *dev, int channel_id)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	/* Clear the interrupt */
	wdog->intclr = CMSDK_APB_WDOG_INTCLR;

	/* Reload */
	wdog->load = reload_cycles;

	return 0;
}

static DEVICE_API(wdt, wdog_cmsdk_apb_api) = {
	.setup = wdog_cmsdk_apb_setup,
	.disable = wdog_cmsdk_apb_disable,
	.install_timeout = wdog_cmsdk_apb_install_timeout,
	.feed = wdog_cmsdk_apb_feed,
};

#ifdef CONFIG_RUNTIME_NMI

static int wdog_cmsdk_apb_has_fired(void)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	return (wdog->maskintstat & CMSDK_APB_WDOG_MASKINTSTAT) != 0U;
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
			user_cb(wdog_r, 0);
		}
	}
}
#endif /* CONFIG_RUNTIME_NMI */

static int wdog_cmsdk_apb_init(const struct device *dev)
{
	volatile struct wdog_cmsdk_apb *wdog = WDOG_STRUCT;

	wdog_r = dev;

	/* unlock access to configuration registers */
	wdog_cmsdk_apb_unlock(dev);

	/* set default reload value */
	wdog->load = reload_cycles;

#ifdef CONFIG_RUNTIME_NMI
	/* Configure the interrupts */
	z_arm_nmi_set_handler(wdog_cmsdk_apb_isr);
#endif

#ifdef CONFIG_WDOG_CMSDK_APB_START_AT_BOOT
	wdog_cmsdk_apb_setup(dev, 0);
#endif

	return 0;
}

DEVICE_DT_INST_DEFINE(0,
		    wdog_cmsdk_apb_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdog_cmsdk_apb_api);
