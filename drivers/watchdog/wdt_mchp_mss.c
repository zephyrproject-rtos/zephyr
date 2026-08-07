/*
 * Copyright (c) 2026 Donato Brusamento <donato.brus@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mpfs_watchdog

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_mchp_mss, CONFIG_WDT_LOG_LEVEL);

#define CORE_WDT_REFRESH (0x00)
#define CORE_WDT_CONTROL (0x04)
#define CORE_WDT_STATUS  (0x08)
#define CORE_WDT_TIME    (0x0C)
#define CORE_WDT_MSVP    (0x10)
#define CORE_WDT_TRIGGER (0x14)
#define CORE_WDT_FORCE   (0x18)

#define REFRESH_WDOGREFRESH_KEY (0xDEADC0DE)

#define CONTROL_ENABLE_FORBIDDEN_MASK BIT(4)
#define CONTROL_ACTIVE_SLEEP_MASK     BIT(3)
#define CONTROL_INTEN_SLEEP_MASK      BIT(2)
#define CONTROL_INTEN_TRIG_MASK       BIT(1)
#define CONTROL_INTEN_MSVP_MASK       BIT(0)

#define STATUS_DEVRST_MASK       BIT(5)
#define STATUS_LOCKED_MASK       BIT(4)
#define STATUS_TRIGGERED_MASK    BIT(3)
#define STATUS_FORBIDDEN_MASK    BIT(2)
#define STATUS_WDOG_TRIPPED_MASK BIT(1)
#define STATUS_MVRP_TRIPPED_MASK BIT(0)

#define TIME_WDOGTIME_MASK BIT_MASK(24)
#define TIME_WDOGTIME_MAX  (0x00FFFFF0)

#define MSVP_WDOGMVRP_MASK BIT_MASK(24)

#define TRIGGER_WDOGTRIGGER_MASK BIT_MASK(12)

#define FORCE_WDOGVALUE_IMMEDIATE_RESET (0x0000000C)

struct mss_wdt_config {
	bool reset_capable;
	uintptr_t wdt_base_addr;
	uint32_t wdt_clk;
};

struct mss_wdt_data {
	bool running;
	bool already_programmed;
};

static int wdt_mchp_mss_setup(const struct device *dev, uint8_t options)
{
	const struct mss_wdt_config *cfg = dev->config;
	struct mss_wdt_data *data = dev->data;
	uint32_t regval;

	if (data->running) {
		return -EBUSY;
	}

	if (!data->already_programmed) {
		/* The wdg, once started, can never be stopped.
		 * Refuse to start one that is still holding its
		 * reset-value timeout.
		 */
		return -EINVAL;
	}

	/* MSS WDG is always halted when in debug mode. */
	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0) {
		return -ENOTSUP;
	}

	regval = sys_read32(cfg->wdt_base_addr + CORE_WDT_CONTROL);
	regval &= ~(CONTROL_ENABLE_FORBIDDEN_MASK | CONTROL_INTEN_SLEEP_MASK |
		    CONTROL_INTEN_MSVP_MASK);

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0) {
		regval &= ~CONTROL_ACTIVE_SLEEP_MASK;
	} else {
		regval |= CONTROL_ACTIVE_SLEEP_MASK;
	}

	sys_write32(regval, cfg->wdt_base_addr + CORE_WDT_CONTROL);

	/* start the countdown */
	sys_write32(REFRESH_WDOGREFRESH_KEY, cfg->wdt_base_addr + CORE_WDT_REFRESH);

	data->running = true;

	return 0;
}

static int wdt_mchp_mss_disable(const struct device *dev)
{
	struct mss_wdt_data *data = dev->data;

	if (!data->running) {
		return -EINVAL;
	}

	return -EPERM;
}

static int wdt_mchp_mss_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct mss_wdt_data *data = dev->data;
	const struct mss_wdt_config *devcfg = dev->config;
	uint32_t wdog_time;
	uint64_t wdog_time_raw;
	uint32_t wdog_mvrp;
	uint64_t wdog_min_raw;

	if (data->already_programmed) {
		return -EBUSY;
	}

	if ((sys_read32(devcfg->wdt_base_addr + CORE_WDT_STATUS) & STATUS_LOCKED_MASK) != 0) {
		return -EBUSY;
	}

	if (cfg->callback != NULL) {
		/* Callback could be implemented if the watchdog was running
		 * with interrupts, but at this time this driver only supports
		 * WDT0 with its reset capabilities, and no interrupts.
		 */
		return -ENOTSUP;
	}
#if defined(CONFIG_WDT_MULTISTAGE)
	if (cfg->next != NULL) {
		return -ENOTSUP;
	}
#endif
	if ((cfg->flags & WDT_FLAG_RESET_MASK) == WDT_FLAG_RESET_NONE) {
		/* Same as cfg->callback, this mode is supported on WDT0 and is
		 * actually the only one available on WDT1-4, but it requires some
		 * additional machinery not supported at this time.
		 */
		return -ENOTSUP;
	}

	/* Watchdog that can only raise an NMI to the E51,
	 * are presently not implemented by this driver.
	 */
	if (!devcfg->reset_capable) {
		return -ENOTSUP;
	}

	if (cfg->window.min != 0U && (cfg->window.min > cfg->window.max)) {
		return -EINVAL;
	}

	/* cfg->window.max maps to the WDOG_TIME register */
	wdog_time_raw = ((uint64_t)cfg->window.max * devcfg->wdt_clk) / 1000;
	if ((wdog_time_raw > (uint64_t)TIME_WDOGTIME_MAX) || wdog_time_raw == 0) {
		return -EINVAL;
	}
	wdog_time = (uint32_t)wdog_time_raw;

	/* cfg->window.min can instead be mapped to MVRP value */
	if (cfg->window.min > 0) {
		wdog_min_raw = ((uint64_t)cfg->window.min * devcfg->wdt_clk) / 1000;
		if (wdog_min_raw == 0 || wdog_min_raw >= (uint64_t)wdog_time) {
			return -EINVAL;
		}
		wdog_mvrp = wdog_time - (uint32_t)wdog_min_raw;
	} else {
		wdog_mvrp = wdog_time;
	}

	/* MVRP and TRIGGER registers will be locked by writing to TIME, so they
	 * get programmed first. Trigger is left disabled since it's used for
	 * an (unsupported) INT -> RST pattern.
	 */
	sys_write32(wdog_mvrp, devcfg->wdt_base_addr + CORE_WDT_MSVP);
	sys_write32(0, devcfg->wdt_base_addr + CORE_WDT_TRIGGER);
	sys_write32(wdog_time, devcfg->wdt_base_addr + CORE_WDT_TIME);

	data->already_programmed = true;

	return 0;
}

static int wdt_mchp_mss_feed(const struct device *dev, int channel_id)
{
	struct mss_wdt_data *data = dev->data;
	const struct mss_wdt_config *cfg = dev->config;

	if (channel_id != 0) {
		return -EINVAL;
	}

	if (!data->running) {
		return -EINVAL;
	}

	sys_write32(REFRESH_WDOGREFRESH_KEY, cfg->wdt_base_addr + CORE_WDT_REFRESH);

	return 0;
}

static DEVICE_API(wdt, wdt_mchp_mss_driver_api) = {
	.setup = wdt_mchp_mss_setup,
	.disable = wdt_mchp_mss_disable,
	.install_timeout = wdt_mchp_mss_install_timeout,
	.feed = wdt_mchp_mss_feed,
};

static int wdt_mchp_mss_init(const struct device *dev)
{
	const struct mss_wdt_config *cfg = dev->config;
	struct mss_wdt_data *data = dev->data;
	uint32_t regval;

	if (cfg->wdt_clk == 0) {
		return -EINVAL;
	}

#if !defined(CONFIG_WDT_DISABLE_AT_BOOT)
	/* The WDG can only be programmed once, so that is left to the user
	 *	rather than automatically started.
	 */
	return -ENOTSUP;
#endif

	/* Check STATUS.LOCKED bit. The watchdog can only be programmed once,
	 * so installing timeouts from user code won't be effective.
	 */
	regval = sys_read32(cfg->wdt_base_addr + CORE_WDT_STATUS);
	data->already_programmed = (regval & STATUS_LOCKED_MASK) != 0;

	return 0;
}

#define WDT_MCHP_MSS_WDT_DEVICE(idx)                                                               \
	static const struct mss_wdt_config wdt_##idx##_config = {                                  \
		.reset_capable = DT_INST_PROP(idx, reset_capable),                                 \
		.wdt_base_addr = DT_INST_REG_ADDR(idx),                                            \
		.wdt_clk = DT_INST_PROP(idx, clock_frequency) >> 8};                               \
                                                                                                   \
	static struct mss_wdt_data wdt_##idx##_data = {.already_programmed = false,                \
						       .running = false};                          \
                                                                                                   \
	DEVICE_DT_DEFINE(DT_NODELABEL(wdt##idx), wdt_mchp_mss_init, NULL, &wdt_##idx##_data,       \
			 &wdt_##idx##_config, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,    \
			 &wdt_mchp_mss_driver_api)

DT_INST_FOREACH_STATUS_OKAY(WDT_MCHP_MSS_WDT_DEVICE);
