/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_watchdog

/**
 * @file
 * @brief Nuvoton NCT watchdog modules driver
 *
 * This file contains the drivers of NCT Watchdog module that generates the
 * clocks and interrupts (T0 Timer) used for its callback functions in the
 * system. It also provides watchdog reset signal generation in response to a
 * failure detection. Please refer the block diagram for more detail.
 *
 *            +---------------------+    +-----------------+
 *  LFCLK --->| T0 Prescale Counter |-+->| 16-Bit T0 Timer |--------> T0 Timer
 * (32kHz)    |     (TWCP 1:32)     | |  |     (TWDT0)     |           Event
 *            +---------------------+ |  +-----------------+
 *  +---------------------------------+
 *  |
 *  |    +-------------------+    +-----------------+
 *  +--->| Watchdog Prescale |--->| 8-Bit Watchdog  |-----> Watchdog Event/Reset
 *       |    (WDCP 1:32)    |    | Counter (WDCNT) |       after n clocks
 *       +-------------------+    +-----------------+
 *
 */

#include "soc_miwu.h"

#include <assert.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <soc.h>
#include "soc_dbg.h"
LOG_MODULE_REGISTER(wdt_nct, CONFIG_WDT_LOG_LEVEL);

/* Watchdog operating frequency is fixed to LFCLK (32.768) kHz */
#define NCT_WDT_CLK 32768

/*
 * Maximum watchdog window time. Since the watchdog counter is 8-bits, maximum
 * time supported by nct watchdog is 256 * (32 * 128) / 32768 = 32 sec.
 */
#define NCT_WDT_MAX_WND_TIME 32000UL

/*
 * Minimum watchdog window time. Ensure we have waited at least 3 watchdog
 * clocks since touching WD timer. 3 / (32768 / 1024) HZ = 93.75 ms
 */
#define NCT_WDT_MIN_WND_TIME 100UL

/* Timeout for reloading and restarting Timer 0. (Unit:ms) */
#define NCT_T0CSR_RST_TIMEOUT 2

/* Timeout for stopping watchdog. (Unit:ms) */
#define NCT_WATCHDOG_STOP_TIMEOUT 1

/* Device config */
struct wdt_nct_config {
	/* wdt controller base address */
	uintptr_t base;
	/* t0 timer wake-up input source configuration */
	const struct nct_wui t0out;
};

/* Driver data */
struct wdt_nct_data {
	/* Timestamp of touching watchdog last time */
	int64_t last_watchdog_touch;
	/* Timeout callback used to handle watchdog event */
	wdt_callback_t cb;
	/* Watchdog feed timeout in milliseconds */
	uint32_t timeout;
	/* Indicate whether a watchdog timeout is installed */
	bool timeout_installed;
};

struct miwu_callback miwu_cb;

/* Driver convenience defines */
#define HAL_INSTANCE(dev) ((struct twd_reg *)((const struct wdt_nct_config *)(dev)->config)->base)

/* WDT local inline functions */
static inline int wdt_t0out_reload(const struct device *dev)
{
	struct twd_reg *const inst = HAL_INSTANCE(dev);
	uint64_t st;

	/* Reload and restart T0 timer */
	inst->T0CSR = (inst->T0CSR & ~BIT(NCT_T0CSR_WDRST_STS)) |
		      BIT(NCT_T0CSR_RST);
	/* Wait for timer is loaded and restart */
	st = k_uptime_get();
	while (IS_BIT_SET(inst->T0CSR, NCT_T0CSR_RST)) {
		if (k_uptime_get() - st > NCT_T0CSR_RST_TIMEOUT) {
			/* RST bit is still set? */
			if (IS_BIT_SET(inst->T0CSR, NCT_T0CSR_RST)) {
				LOG_ERR("Timeout: reload T0 timer!");
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

static inline int wdt_wait_stopped(const struct device *dev)
{
	struct twd_reg *const inst = HAL_INSTANCE(dev);
	uint64_t st;

	st = k_uptime_get();
	/* If watchdog is still running? */
	while (IS_BIT_SET(inst->T0CSR, NCT_T0CSR_WD_RUN)) {
		if (k_uptime_get() - st > NCT_WATCHDOG_STOP_TIMEOUT) {
			/* WD_RUN bit is still set? */
			if (IS_BIT_SET(inst->T0CSR, NCT_T0CSR_WD_RUN)) {
				LOG_ERR("Timeout: stop watchdog timer!");
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

/* WDT local functions */
static void wdt_t0out_isr(const struct device *dev, struct nct_wui *wui)
{
	struct wdt_nct_data *const data = dev->data;
	ARG_UNUSED(wui);

	LOG_DBG("WDT reset will issue after %d delay cycle! WUI(%d %d %d)",
		CONFIG_WDT_NCT_DELAY_CYCLES, wui->table, wui->group, wui->bit);

	/* Handle watchdog event here. */
	if (data->cb) {
		data->cb(dev, 0);
	}
}

static void wdt_config_t0out_interrupt(const struct device *dev)
{
	struct twd_reg *const inst = HAL_INSTANCE(dev);
	const struct wdt_nct_config *const config = dev->config;

	/* Enable t0out */
	inst->T0CSR |= BIT(NCT_T0CSR_T0EN);

	/* Initialize a miwu device input and its callback function */
	nct_miwu_init_dev_callback(&miwu_cb, &config->t0out, wdt_t0out_isr,
			dev);
	nct_miwu_manage_callback(&miwu_cb, true);

	/*
	 * Configure the T0 wake-up event triggered from a rising edge
	 * on T0OUT signal.
	 */
	nct_miwu_interrupt_configure(&config->t0out,
			NCT_MIWU_MODE_EDGE, NCT_MIWU_TRIG_HIGH);
}

/* WDT api functions */
static int wdt_nct_install_timeout(const struct device *dev,
				   const struct wdt_timeout_cfg *cfg)
{
	struct wdt_nct_data *const data = dev->data;
	struct twd_reg *const inst = HAL_INSTANCE(dev);

	/* If watchdog is already running */
	if (IS_BIT_SET(inst->T0CSR, NCT_T0CSR_WD_RUN)) {
		return -EBUSY;
	}

	/* No window watchdog support */
	if (cfg->window.min != 0) {
		data->timeout_installed = false;
		return -EINVAL;
	}

	/*
	 * Since the watchdog counter in nct series is 8-bits, maximum time
	 * supported by it is 256 * (32 * 32) / 32768 = 8 sec. This makes the
	 * allowed range of 1-8000 in milliseconds. Check if the provided value
	 * is within this range.
	 */
	if (cfg->window.max > NCT_WDT_MAX_WND_TIME || cfg->window.max == 0) {
		data->timeout_installed = false;
		return -EINVAL;
	}

	/* Save watchdog timeout */
	data->timeout = cfg->window.max;

	/* Install user timeout isr */
	data->cb = cfg->callback;
	data->timeout_installed = true;

	return 0;
}

static int wdt_nct_setup(const struct device *dev, uint8_t options)
{
	struct twd_reg *const inst = HAL_INSTANCE(dev);
	const struct wdt_nct_config *const config = dev->config;
	struct wdt_nct_data *const data = dev->data;
	int rv;

	/* Disable irq of t0-out expired event first */
	nct_miwu_irq_disable(&config->t0out);

	if (!data->timeout_installed) {
		LOG_ERR("No valid WDT timeout installed");
		return -EINVAL;
	}

	if (IS_BIT_SET(inst->T0CSR, NCT_T0CSR_WD_RUN)) {
		LOG_ERR("WDT timer is busy");
		return -EBUSY;
	}

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0) {
		LOG_ERR("WDT_OPT_PAUSE_IN_SLEEP is not supported");
		return -ENOTSUP;
	}

	/* Stall the WDT counter when halted by debugger */
	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) != 0) {
		nct_dbg_freeze_enable(true);
	} else {
		nct_dbg_freeze_enable(false);
	}

	/*
	 * One clock period of T0 timer is 32/32.768 KHz = 0.976 ms.
	 * Then the counter value is timeout/0.976 - 1.
	 */

	inst->TWDT0 = MAX(DIV_ROUND_UP(data->timeout * NCT_WDT_CLK,
				32 * 1000) - 1, 1);

	/* Configure 8-bit watchdog counter */
	inst->WDCNT = MIN(DIV_ROUND_UP(data->timeout, 128) +
					CONFIG_WDT_NCT_DELAY_CYCLES, 0xff);

	LOG_DBG("WDT setup: TWDT0, WDCNT are %d, %d", inst->TWDT0, inst->WDCNT);

	/* Reload and restart T0 timer */
	rv = wdt_t0out_reload(dev);

	/* Configure t0 timer interrupt and its isr. */
	wdt_config_t0out_interrupt(dev);

	/* Enable irq of t0-out expired event */
	nct_miwu_irq_enable(&config->t0out);

	return rv;
}

static int wdt_nct_disable(const struct device *dev)
{
	const struct wdt_nct_config *const config = dev->config;
	struct wdt_nct_data *const data = dev->data;
	struct twd_reg *const inst = HAL_INSTANCE(dev);

	/*
	 * Ensure we have waited at least 3 watchdog ticks before
	 * stopping watchdog
	 */
	while (k_uptime_get() - data->last_watchdog_touch < NCT_WDT_MIN_WND_TIME) {
		continue;
	}

	/*
	 * Stop and unlock watchdog by writing 87h, 61h and 63h
	 * sequence bytes to WDSDM register
	 */
	inst->WDSDM = 0x87;
	inst->WDSDM = 0x61;
	inst->WDSDM = 0x63;

	/* Disable irq of t0-out expired event and mark it uninstalled */
	nct_miwu_irq_disable(&config->t0out);
	data->timeout_installed = false;

	/* Wait until watchdog is stopped. */
	return wdt_wait_stopped(dev);
}

static int wdt_nct_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);
	struct wdt_nct_data *const data = dev->data;
	struct twd_reg *const inst = HAL_INSTANCE(dev);

	/* Feed watchdog by writing 5Ch to WDSDM */
	inst->WDSDM = 0x5C;
	data->last_watchdog_touch = k_uptime_get();

	/* Reload and restart T0 timer */
	return wdt_t0out_reload(dev);
}

/* WDT driver registration */
static const struct wdt_driver_api wdt_nct_driver_api = {
	.setup = wdt_nct_setup,
	.disable = wdt_nct_disable,
	.install_timeout = wdt_nct_install_timeout,
	.feed = wdt_nct_feed,
};

static int wdt_nct_init(const struct device *dev)
{
	struct twd_reg *const inst = HAL_INSTANCE(dev);

#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	wdt_nct_disable(dev);
#endif

	/*
	 * TWCFG (Timer Watchdog Configuration) setting
	 * [7:6]- Reserved = 0
	 * [5]  - WDSDME   = 1: Feed watchdog by writing 5Ch to WDSDM
	 * [4]  - WDCT0I   = 1: Select T0IN as watchdog prescaler clock
	 * [3]  - LWDCNT   = 0: Don't lock WDCNT register
	 * [2]  - LTWDT0   = 0: Don't lock TWDT0 register
	 * [1]  - LTWCP    = 0: Don't lock TWCP register
	 * [0]  - LTWCFG   = 0: Don't lock TWCFG register
	 */
	inst->TWCFG = BIT(NCT_TWCFG_WDSDME) | BIT(NCT_TWCFG_WDCT0I);

	/* Disable early touch functionality */
	inst->T0CSR = (inst->T0CSR & ~BIT(NCT_T0CSR_WDRST_STS)) |
		      BIT(NCT_T0CSR_TESDIS);
	/*
	 * Plan clock frequency of T0 timer and watchdog timer as below:
	 * - T0 Timer freq is LFCLK/32 Hz
	 * - Watchdog freq is T0CLK/32 Hz (ie. LFCLK/1024 Hz)
	 */
	inst->TWCP = 0x05; /* Prescaler is 32 in T0 Timer */
	inst->WDCP = 0x07; /* Prescaler is 128 in Watchdog Timer */

	return 0;
}

static const struct wdt_nct_config wdt_nct_cfg_0 = {
	.base = DT_INST_REG_ADDR(0),
	.t0out = NCT_DT_WUI_ITEM_BY_NAME(0, t0_out)
};

static struct wdt_nct_data wdt_nct_data_0;

DEVICE_DT_INST_DEFINE(0, wdt_nct_init, NULL,
			&wdt_nct_data_0, &wdt_nct_cfg_0,
			PRE_KERNEL_1,
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&wdt_nct_driver_api);
