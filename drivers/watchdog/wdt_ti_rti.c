/*
 * Copyright (c) 2025 Ayush Singh, BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_j7_rti_wdt

#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <stdint.h>

#define WDENABLE_KEY  0xa98559da
#define WDDISABLE_KEY 0x5312aced

#define WDKEY_SEQ0 0xe51a
#define WDKEY_SEQ1 0xa35c

#define WDT_PRELOAD_SHIFT 13

#define WDT_PRELOAD_MAX 0xfff

#define RTIWWDRX_NMI   0xa
#define RTIWWDRX_RESET 0x5

#define RTIGCTRL_HALT_BY_DBG 0
#define RTIGCTRL_RUN_BY_DBG  BIT(15)

#define RTIWWDSIZE_100P  0x5
#define RTIWWDSIZE_50P   0x50
#define RTIWWDSIZE_25P   0x500
#define RTIWWDSIZE_12P5  0x5000
#define RTIWWDSIZE_6P25  0x50000
#define RTIWWDSIZE_3P125 0x500000

#define DEV_CFG(dev)  ((const struct wdt_ti_rti_config *)(dev)->config)
#define DEV_DATA(dev) ((struct wdt_ti_rti_data *)(dev)->data)
#define DEV_REGS(dev) ((struct wdt_ti_rti_regs *)DEVICE_MMIO_GET(dev))

struct wdt_ti_rti_regs {
	/* RTI Global Control Register, offset: 0x00 */
	volatile uint32_t GCTRL;
	uint32_t pad[35];
	/* Digital Watchdog Control Register, offset: 0x90 */
	volatile uint32_t DWDCTRL;
	/* Digital Watchdog Preload Register, offset: 0x94 */
	volatile uint32_t DWDPRLD;
	/* Watchdog Status Register, offset: 0x98 */
	volatile uint32_t WDSTATUS;
	/* Watchdog Key Register, offset: 0x9C */
	volatile uint32_t WDKEY;
	/* Digital Watchdog Down Counter, offset: 0xA0 */
	volatile uint32_t DWDCNTR;
	/* Digital Windowed Watchdog Reaction Control, offset: 0xA4 */
	volatile uint32_t WWDRXNCTRL;
	/* Digital Windowed Watchdog Window Size Control, offset: 0xA8 */
	volatile uint32_t WWDSIZECTRL;
};

struct wdt_ti_rti_data {
	DEVICE_MMIO_RAM;
};

struct wdt_ti_rti_config {
	DEVICE_MMIO_ROM;

	uint64_t freq;
};

static int wdt_ti_rti_setup(const struct device *dev, uint8_t options)
{
	ARG_UNUSED(options);

	struct wdt_ti_rti_regs *regs = DEV_REGS(dev);

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		regs->GCTRL = RTIGCTRL_HALT_BY_DBG;
	} else {
		regs->GCTRL = RTIGCTRL_RUN_BY_DBG;
	}

	regs->DWDCTRL = WDENABLE_KEY;

	return 0;
}

static int wdt_ti_rti_disable(const struct device *dev)
{
	struct wdt_ti_rti_regs *regs = DEV_REGS(dev);

	regs->DWDCTRL = WDDISABLE_KEY;
	regs->WDKEY = 0;

	return 0;
}

static int wdt_ti_rti_window_size(const struct wdt_window window)
{
	const int window_sizes[] = {RTIWWDSIZE_100P, RTIWWDSIZE_50P,  RTIWWDSIZE_25P,
				    RTIWWDSIZE_12P5, RTIWWDSIZE_6P25, RTIWWDSIZE_3P125};

	if (window.max < window.min || window.max == 0) {
		return -EINVAL;
	}

	for (uint32_t idx = 0; idx < ARRAY_SIZE(window_sizes); idx++) {
		uint32_t temp = (window.max - window.min) << idx;

		if (temp == window.max) {
			return window_sizes[idx];
		} else if (temp > window.max) {
			break;
		}
	}

	return -EINVAL;
}

static int wdt_ti_rti_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_ti_rti_config *config = DEV_CFG(dev);
	struct wdt_ti_rti_regs *regs = DEV_REGS(dev);
	uint32_t timer_margin;
	int window_size;

	window_size = wdt_ti_rti_window_size(cfg->window);
	if (window_size < 0) {
		return window_size;
	}

	timer_margin = (cfg->window.max * config->freq) / MSEC_PER_SEC;
	timer_margin >>= WDT_PRELOAD_SHIFT;
	if (timer_margin > WDT_PRELOAD_MAX) {
		return -EINVAL;
	}

	if (cfg->flags == WDT_FLAG_RESET_SOC) {
		regs->WWDRXNCTRL = RTIWWDRX_NMI;
	}

	regs->DWDPRLD = timer_margin;
	regs->WWDSIZECTRL = (uint32_t)window_size;

	return 0;
}

static int wdt_ti_rti_feed(const struct device *dev, int channel_id)
{
	struct wdt_ti_rti_regs *regs = DEV_REGS(dev);

	if (channel_id != 0) {
		return -EINVAL;
	}

	regs->WDKEY = WDKEY_SEQ0;
	regs->WDKEY = WDKEY_SEQ1;

	return 0;
}

static int wdt_ti_rti_init(const struct device *dev)
{
	struct wdt_ti_rti_regs *regs;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	regs = DEV_REGS(dev);
	if (!regs) {
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(wdt, wdt_ti_rti_api) = {
	.setup = wdt_ti_rti_setup,
	.disable = wdt_ti_rti_disable,
	.feed = wdt_ti_rti_feed,
	.install_timeout = wdt_ti_rti_timeout,
};

#define WDT_TI_RTI_INIT(i)                                                                         \
	static struct wdt_ti_rti_data wdt_ti_rti_data_##i = {};                                    \
                                                                                                   \
	static struct wdt_ti_rti_config wdt_ti_rti_config_##i = {                                  \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(i)),                                              \
		.freq = DT_INST_PROP(i, clock_frequency),                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, wdt_ti_rti_init, NULL, &wdt_ti_rti_data_##i,                      \
			      &wdt_ti_rti_config_##i, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wdt_ti_rti_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_TI_RTI_INIT)
