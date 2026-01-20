/*
 * Copyright (c) 2025 BayLibre, SAS
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This driver handles both CC23X0 and CC27XX watchdog controllers
 * in a single implementation.
 */

#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <errno.h>

/* Define compatible string for the unified driver */
#define DT_DRV_COMPAT ti_lpf3_wdt

/* Include appropriate header files based on device family */
#include <driverlib/pmctl.h>
#include <inc/hw_ckmd.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wdt_ti_lpf3, CONFIG_WDT_LOG_LEVEL);

/* Register access macros for each chip family */
#if defined(CONFIG_SOC_SERIES_CC23X0)
/* CC23X0 register access macros */
#define WDT_UNLOCK(_base)        (HWREG((_base) + CKMD_O_WDTLOCK) = 0x1ACCE551)
#define WDT_LOCK(_base)          (HWREG((_base) + CKMD_O_WDTLOCK) = 0x1)
#define WDT_FEED(_base, _value)  (HWREG((_base) + CKMD_O_WDTCNT) = (_value))
#define WDT_STALL_ENABLE(_base)  (HWREG((_base) + CKMD_O_WDTTEST) = 0x1)
#define WDT_STALL_DISABLE(_base) (HWREG((_base) + CKMD_O_WDTTEST) = 0x0)

/* No explicit control register in CC23X0 */
#define WDT_HAS_CTL_REG 0

#elif defined(CONFIG_SOC_SERIES_CC27XX)
/* CC27XX register access macros */
#define WDT_UNLOCK(_base)            (HWREG((_base) + CKMD_O_LOCK) = 0x1ACCE551)
#define WDT_LOCK(_base)              (HWREG((_base) + CKMD_O_LOCK) = 0x1)
#define WDT_FEED(_base, _value)      (HWREG((_base) + CKMD_O_CNT) = (_value))
#define WDT_STALL_ENABLE(_base)      (HWREG((_base) + CKMD_O_TEST) = 0x1)
#define WDT_STALL_DISABLE(_base)     (HWREG((_base) + CKMD_O_TEST) = 0x0)

/* Define constants for reset bit in the watchdog control register */
#define WDT_HAS_CTL_REG              1
#define WDT_CTL_REG_OFFSET           CKMD_O_CTL
#define WDT_CTL_RSTEN_BIT            (1U << 1) /* Reset enable is bit 1 */
#define WDT_CTL_EN_BIT               (1U << 0) /* Watchdog enable is bit 0 */
#define WDT_WRITE_CTL(_base, _value) (HWREG((_base) + WDT_CTL_REG_OFFSET) = (_value))
#define WDT_GET_CTL(_base)           (HWREG((_base) + WDT_CTL_REG_OFFSET))
#endif

/* Common timing constants for both chip families */
#define WDT_SOURCE_FREQ      32768
/*
 * Convert milliseconds to watchdog ticks with proper rounding.
 * Formula:  ticks = (ms * 32768) / 1000
 * Use (ms * 32768 + 500) / 1000 for rounding to nearest integer.
 */
#define WDT_MS_TO_TICKS(_ms) (((_ms) * WDT_SOURCE_FREQ + 500) / 1000)
#define WDT_MAX_RELOAD_MS    (0xffffffffUL / WDT_SOURCE_FREQ * 1000)

/* Common data structures */
struct wdt_ti_lpf3_data {
	uint8_t enabled;
	uint32_t reload;
	uint8_t flags;
};

struct wdt_ti_lpf3_config {
	uint32_t base;
};

static int wdt_ti_lpf3_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct wdt_ti_lpf3_data *data = dev->data;

	/* Cannot install timeout if watchdog is already running */
	if (data->enabled) {
		return -EBUSY;
	}

	/* window watchdog not supported */
	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	if (cfg->window.max > WDT_MAX_RELOAD_MS) {
		return -EINVAL;
	}

	/* Callbacks are not supported - hardware does not generate interrupt before reset */
	if (cfg->callback != NULL) {
		return -ENOTSUP;
	}

	data->reload = WDT_MS_TO_TICKS(cfg->window.max);
	data->flags = cfg->flags;

	LOG_DBG("raw reload value: %d", data->reload);

	return 0;
}

static int wdt_ti_lpf3_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_ti_lpf3_config *config = dev->config;
	struct wdt_ti_lpf3_data *data = dev->data;

	/* Check if watchdog is already enabled (exclusive access) */
	if (data->enabled) {
		return -EBUSY;
	}

	/* Unlock the watchdog */
	WDT_UNLOCK(config->base);

	/* Configure stall behavior based on options */
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		WDT_STALL_ENABLE(config->base);
	} else {
		WDT_STALL_DISABLE(config->base);
	}

#if WDT_HAS_CTL_REG
	/* For devices with control register (CC27XX), configure reset behavior */
	uint32_t ctl_value = WDT_CTL_EN_BIT;

	if ((data->flags & WDT_FLAG_RESET_MASK) == WDT_FLAG_RESET_SOC) {
		ctl_value |= WDT_CTL_RSTEN_BIT;
		LOG_DBG("Reset enabled");
	} else {
		LOG_DBG("Reset disabled");
	}

	WDT_WRITE_CTL(config->base, ctl_value);
	LOG_DBG("Control register value: 0x%08lx", WDT_GET_CTL(config->base));
#endif

	/* Feed the watchdog to start the counter with the configured reload value */
	WDT_FEED(config->base, data->reload);

	/* Lock the watchdog */
	WDT_LOCK(config->base);

	/* Mark watchdog as enabled */
	data->enabled = 1;

	return 0;
}

static int wdt_ti_lpf3_disable(const struct device *dev)
{
	return -ENOTSUP;
}

static int wdt_ti_lpf3_feed(const struct device *dev, int channel_id)
{
	const struct wdt_ti_lpf3_config *config = dev->config;
	struct wdt_ti_lpf3_data *data = dev->data;

	WDT_UNLOCK(config->base);
	WDT_FEED(config->base, data->reload);

#if defined(CONFIG_SOC_SERIES_CC27XX)
	/* CC27XX requires explicit lock after feeding */
	WDT_LOCK(config->base);
#endif

	return 0;
}

#ifdef CONFIG_DEBUG
static void wdt_ti_lpf3_boot_reason(void)
{
	uint32_t rststa = PMCTLGetResetReason();
	uint32_t syssrc = rststa & PMCTL_RSTSTA_SYSSRC_M;

	LOG_DBG("[WDT] Boot reason rststa[%x] syssrc[%x]", rststa, syssrc);

	switch (syssrc) {
	case PMCTL_RSTSTA_SYSSRC_LFLOSSEV:
		LOG_DBG("LF clock loss event");
		break;
	case PMCTL_RSTSTA_SYSSRC_CPURSTEV:
		LOG_DBG("CPU reset event");
		break;
	case PMCTL_RSTSTA_SYSSRC_LOCKUPEV:
		LOG_DBG("CPU LOCKUP event");
		break;
	case PMCTL_RSTSTA_SYSSRC_WDTEV:
		LOG_DBG("Watchdog timeout event");
		break;
	case PMCTL_RSTSTA_SYSSRC_SYSRSTEV:
		LOG_DBG("System reset event");
		break;
	case PMCTL_RSTSTA_SYSSRC_SWDRSTEV:
		LOG_DBG("Serial Wire Debug reset event");
		break;
	case PMCTL_RSTSTA_SYSSRC_AFSMEV:
		LOG_DBG("Analog FSM timeout event");
		break;
	case PMCTL_RSTSTA_SYSSRC_AERREV:
		LOG_DBG("Analog Error reset event");
		break;
	case PMCTL_RSTSTA_SYSSRC_DERREV:
		LOG_DBG("Digital Error reset event");
		break;
	}
}
#endif

static int wdt_ti_lpf3_init(const struct device *dev)
{
	struct wdt_ti_lpf3_data *data = dev->data;

#ifdef CONFIG_DEBUG
	wdt_ti_lpf3_boot_reason();
#endif

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		return 0;
	}

	/* Store default initial values */
	data->enabled = 0;

	return 0;
}

static DEVICE_API(wdt, wdt_ti_lpf3_api) = {
	.setup = wdt_ti_lpf3_setup,
	.disable = wdt_ti_lpf3_disable,
	.install_timeout = wdt_ti_lpf3_install_timeout,
	.feed = wdt_ti_lpf3_feed,
};

#define WDT_INITIAL_TIMEOUT CONFIG_WDT_LPF3_INITIAL_TIMEOUT

#define WDT_TI_LPF3_INIT(index)                                                                    \
	static struct wdt_ti_lpf3_data wdt_ti_lpf3_data_##index = {                                \
		.reload = WDT_MS_TO_TICKS(WDT_INITIAL_TIMEOUT),                                    \
		.flags = 0,                                                                        \
	};                                                                                         \
	static struct wdt_ti_lpf3_config wdt_ti_lpf3_config_##index = {                            \
		.base = DT_INST_REG_ADDR(index),                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, wdt_ti_lpf3_init, NULL, &wdt_ti_lpf3_data_##index,            \
			      &wdt_ti_lpf3_config_##index, POST_KERNEL,                            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wdt_ti_lpf3_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_TI_LPF3_INIT)
