/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_watchdog

/**
 * @file
 * @brief Nuvoton NPCM Timer and Watchdog (TWD) driver
 *
 * The watchdog counter is clocked from LFCLK through two prescalers:
 *
 *            +---------------------+    +-----------------+
 *  LFCLK --->| T0 Prescale Counter |-+->| 16-Bit T0 Timer |
 * (32kHz)    |     (TWCP 1:32)     | |  |     (TWDT0)     |
 *            +---------------------+ |  +-----------------+
 *  +---------------------------------+
 *  |
 *  |    +-------------------+    +-----------------+
 *  +--->| Watchdog Prescale |--->| 8-Bit Watchdog  |-----> Watchdog Reset
 *       |    (WDCP 1:32)    |    | Counter (WDCNT) |       after n clocks
 *       +-------------------+    +-----------------+
 */

#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "wdt_npcm_registers.h"

LOG_MODULE_REGISTER(wdt_npcm, CONFIG_WDT_LOG_LEVEL);

/* Watchdog operating frequency is fixed to LFCLK (32.768 kHz) */
#define NPCM_WDT_CLK 32768

/* T0 timer and watchdog prescalers (TWCP/WDCP value 5 selects 1:32) */
#define NPCM_WDT_PRESCALER_DIV32 0x05
#define NPCM_WDT_PRESCALER       32

/*
 * Maximum watchdog window time. The watchdog counter is 8 bits, so the
 * maximum timeout is 256 * (32 * 32) / 32768 = 8 sec.
 */
#define NPCM_WDT_MAX_WND_TIME_MS 8000UL

/*
 * Minimum time between touching and stopping the watchdog: at least 3
 * watchdog clocks, i.e. 3 / (32768 / 1024) Hz = 93.75 ms.
 */
#define NPCM_WDT_MIN_WND_TIME_MS 100

/* Timeout for reloading and restarting Timer 0 */
#define NPCM_T0CSR_RST_TIMEOUT_MS 2

/* Timeout for stopping the watchdog */
#define NPCM_WDT_STOP_TIMEOUT_MS 1

/* Watchdog service data match values */
#define NPCM_WDSDM_FEED  0x5c
#define NPCM_WDSDM_STOP1 0x87
#define NPCM_WDSDM_STOP2 0x61
#define NPCM_WDSDM_STOP3 0x63

struct wdt_npcm_config {
	struct twd_reg *base;
	mm_reg_t scfg_base;
};

struct wdt_npcm_data {
	/* Timestamp of the last watchdog touch */
	int64_t last_watchdog_touch;
	/* Watchdog timeout in milliseconds */
	uint32_t timeout;
	/* Indicate whether a watchdog timeout is installed */
	bool timeout_installed;
};

static int wdt_npcm_t0_reload(const struct device *dev)
{
	const struct wdt_npcm_config *config = dev->config;
	struct twd_reg *const inst = config->base;
	k_timepoint_t end;

	/* Reload and restart T0 timer */
	inst->t0csr = inst->t0csr | BIT(NPCM_T0CSR_WDRST_STS) | BIT(NPCM_T0CSR_RST);

	end = sys_timepoint_calc(K_MSEC(NPCM_T0CSR_RST_TIMEOUT_MS));
	while (IS_BIT_SET(inst->t0csr, NPCM_T0CSR_RST)) {
		if (sys_timepoint_expired(end)) {
			if (IS_BIT_SET(inst->t0csr, NPCM_T0CSR_RST)) {
				LOG_ERR("Timeout: reload T0 timer");
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

static int wdt_npcm_wait_stopped(const struct device *dev)
{
	const struct wdt_npcm_config *config = dev->config;
	struct twd_reg *const inst = config->base;
	k_timepoint_t end;

	end = sys_timepoint_calc(K_MSEC(NPCM_WDT_STOP_TIMEOUT_MS));
	while (IS_BIT_SET(inst->t0csr, NPCM_T0CSR_WD_RUN)) {
		if (sys_timepoint_expired(end)) {
			if (IS_BIT_SET(inst->t0csr, NPCM_T0CSR_WD_RUN)) {
				LOG_ERR("Timeout: stop watchdog timer");
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

/*
 * Configure the Automatic Freeze mode. When enabled, module clocks and
 * counters (including the watchdog) are stopped while the core is halted.
 */
static void wdt_npcm_dbg_freeze_enable(const struct device *dev, bool enable)
{
	const struct wdt_npcm_config *config = dev->config;
	mm_reg_t reg = config->scfg_base + NPCM_SCFG_DBGFRZEN3_OFFSET;
	uint8_t val = sys_read8(reg);

	WRITE_BIT(val, NPCM_DBGFRZEN3_GLBL_FRZ_DIS, !enable);
	sys_write8(val, reg);
}

static int wdt_npcm_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_npcm_config *config = dev->config;
	struct wdt_npcm_data *const data = dev->data;
	struct twd_reg *const inst = config->base;

	if (IS_BIT_SET(inst->t0csr, NPCM_T0CSR_WD_RUN)) {
		return -EBUSY;
	}

	/* No window watchdog support */
	if (cfg->window.min != 0) {
		data->timeout_installed = false;
		return -EINVAL;
	}

	if (cfg->window.max > NPCM_WDT_MAX_WND_TIME_MS || cfg->window.max == 0) {
		data->timeout_installed = false;
		return -EINVAL;
	}

	/* Watchdog event callbacks are not supported, only reset */
	if (cfg->callback != NULL) {
		data->timeout_installed = false;
		return -ENOTSUP;
	}

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		data->timeout_installed = false;
		return -ENOTSUP;
	}

	data->timeout = cfg->window.max;
	data->timeout_installed = true;

	return 0;
}

static int wdt_npcm_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_npcm_config *config = dev->config;
	struct wdt_npcm_data *const data = dev->data;
	struct twd_reg *const inst = config->base;

	if (!data->timeout_installed) {
		LOG_ERR("No valid WDT timeout installed");
		return -EINVAL;
	}

	if (IS_BIT_SET(inst->t0csr, NPCM_T0CSR_WD_RUN)) {
		LOG_ERR("WDT timer is busy");
		return -EBUSY;
	}

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0) {
		LOG_ERR("WDT_OPT_PAUSE_IN_SLEEP is not supported");
		return -ENOTSUP;
	}

	/* Stall the WDT counter when halted by debugger */
	wdt_npcm_dbg_freeze_enable(dev, (options & WDT_OPT_PAUSE_HALTED_BY_DBG) != 0);

	/*
	 * One T0 timer clock period is 32 / 32.768 kHz = 0.976 ms, so the
	 * preset value is timeout / 0.976 - 1.
	 */
	inst->twdt0 =
		MAX(DIV_ROUND_UP(data->timeout * NPCM_WDT_CLK, NPCM_WDT_PRESCALER * 1000) - 1, 1);

	/* One watchdog clock period is 32 * 32 / 32.768 kHz = 31.25 ms */
	inst->wdcnt = MIN(DIV_ROUND_UP(data->timeout * NPCM_WDT_CLK,
				       NPCM_WDT_PRESCALER * NPCM_WDT_PRESCALER * 1000),
			  UINT8_MAX);

	LOG_DBG("WDT setup: TWDT0 %d, WDCNT %d", inst->twdt0, inst->wdcnt);

	data->last_watchdog_touch = k_uptime_get();

	return wdt_npcm_t0_reload(dev);
}

static int wdt_npcm_disable(const struct device *dev)
{
	const struct wdt_npcm_config *config = dev->config;
	struct wdt_npcm_data *const data = dev->data;
	struct twd_reg *const inst = config->base;

	/*
	 * Wait at least 3 watchdog clocks after the last touch before stopping
	 * the watchdog. Skip the wait if it was not touched by this driver: at
	 * PRE_KERNEL_1 (CONFIG_WDT_DISABLE_AT_BOOT) the system clock is not
	 * running yet, and the time spent in boot ROM already exceeds it.
	 */
	if (data->last_watchdog_touch != 0) {
		while (k_uptime_get() - data->last_watchdog_touch < NPCM_WDT_MIN_WND_TIME_MS) {
			continue;
		}
	}

	/* Stop and unlock the watchdog with the WDSDM stop sequence */
	inst->wdsdm = NPCM_WDSDM_STOP1;
	inst->wdsdm = NPCM_WDSDM_STOP2;
	inst->wdsdm = NPCM_WDSDM_STOP3;

	data->timeout_installed = false;

	return wdt_npcm_wait_stopped(dev);
}

static int wdt_npcm_feed(const struct device *dev, int channel_id)
{
	const struct wdt_npcm_config *config = dev->config;
	struct wdt_npcm_data *const data = dev->data;
	struct twd_reg *const inst = config->base;

	if (channel_id != 0) {
		return -EINVAL;
	}

	inst->wdsdm = NPCM_WDSDM_FEED;
	data->last_watchdog_touch = k_uptime_get();

	return wdt_npcm_t0_reload(dev);
}

static DEVICE_API(wdt, wdt_npcm_driver_api) = {
	.setup = wdt_npcm_setup,
	.disable = wdt_npcm_disable,
	.install_timeout = wdt_npcm_install_timeout,
	.feed = wdt_npcm_feed,
};

static int wdt_npcm_init(const struct device *dev)
{
	const struct wdt_npcm_config *config = dev->config;
	struct twd_reg *const inst = config->base;

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		wdt_npcm_disable(dev);
	}

	/*
	 * Feed the watchdog by writing 5Ch to WDSDM and select T0IN as the
	 * watchdog prescaler clock. Registers are left unlocked.
	 */
	inst->twcfg = BIT(NPCM_TWCFG_WDSDME) | BIT(NPCM_TWCFG_WDCT0I);

	/* Disable early touch functionality */
	inst->t0csr = inst->t0csr | BIT(NPCM_T0CSR_WDRST_STS) | BIT(NPCM_T0CSR_TESDIS);

	/*
	 * T0 timer runs at LFCLK / 32 and the watchdog counter at
	 * T0IN / 32 (i.e. LFCLK / 1024).
	 */
	inst->twcp = NPCM_WDT_PRESCALER_DIV32;
	inst->wdcp = NPCM_WDT_PRESCALER_DIV32;

	return 0;
}

static const struct wdt_npcm_config wdt_npcm_cfg_0 = {
	.base = (struct twd_reg *)DT_INST_REG_ADDR(0),
	.scfg_base = DT_REG_ADDR(DT_INST_PHANDLE(0, nuvoton_scfg)),
};

static struct wdt_npcm_data wdt_npcm_data_0;

DEVICE_DT_INST_DEFINE(0, wdt_npcm_init, NULL, &wdt_npcm_data_0, &wdt_npcm_cfg_0, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wdt_npcm_driver_api);
