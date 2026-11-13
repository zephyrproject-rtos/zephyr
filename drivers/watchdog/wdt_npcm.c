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
 * (32kHz)    |    (TWCP 1:2^n)     | |  |     (TWDT0)     |
 *            +---------------------+ |  +-----------------+
 *  +---------------------------------+
 *  |
 *  |    +-------------------+    +-----------------+
 *  +--->| Watchdog Prescale |--->| 8-Bit Watchdog  |-----> Watchdog Reset
 *       |   (WDCP 1:2^m)    |    | Counter (WDCNT) |       after n clocks
 *       +-------------------+    +-----------------+
 *
 * The prescalers are selected according to the timeout range.
 */

#include <cmsis_core.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "wdt_npcm_registers.h"

LOG_MODULE_REGISTER(wdt_npcm, CONFIG_WDT_LOG_LEVEL);

/* Watchdog operating frequency is fixed to LFCLK (32.768 kHz) */
#define NPCM_WDT_CLK 32768

/*
 * Maximum watchdog window time. The watchdog counter is 8 bits, so the
 * hardware limit is 255 * (1024 * 32768) / 32768 = 261120 sec. Use a
 * round value below it.
 */
#define NPCM_WDT_MAX_WND_TIME_MS 250000000UL

/*
 * Maximum timeout with a callback. TWDT0 is 16 bits, so with the largest
 * T0 prescaler the T0OUT event occurs at most every
 * 65536 * 1024 / 32768 = 2048 sec.
 */
#define NPCM_WDT_MAX_CB_TIME_MS 2048000UL

/*
 * Minimum time between touching and stopping the watchdog. The stop
 * sequence is not accepted right after the watchdog is configured or fed.
 */
#define NPCM_WDT_MIN_WND_TIME_MS 100

/*
 * T0CSR.RST is cleared on the 2nd rising edge of T0IN, so wait for up to
 * three T0IN periods (LFCLK / 2^TWCP) plus 1 ms margin.
 */
#define NPCM_T0CSR_RST_TIMEOUT_MS(twcp) (DIV_ROUND_UP(3U * BIT(twcp) * 1000U, NPCM_WDT_CLK) + 1)

/* Timeout for stopping the watchdog */
#define NPCM_WDT_STOP_TIMEOUT_MS 1

/* Watchdog service data match values */
#define NPCM_WDSDM_FEED  0x5c
#define NPCM_WDSDM_STOP1 0x87
#define NPCM_WDSDM_STOP2 0x61
#define NPCM_WDSDM_STOP3 0x63

/* T0 timer (TWCP) and watchdog (WDCP) prescalers for a timeout range */
struct wdt_npcm_prescaler {
	/* Upper limit (exclusive) of the timeout range in milliseconds */
	uint32_t limit_ms;
	/* T0 timer prescaler, divides LFCLK by 2^twcp */
	uint8_t twcp;
	/* Watchdog prescaler, divides T0IN by 2^wdcp */
	uint8_t wdcp;
};

static const struct wdt_npcm_prescaler wdt_npcm_prescalers[] = {
	/* Watchdog clock period is 32 * 32 / 32768 = 31.25 ms */
	{.limit_ms = 1000, .twcp = 5, .wdcp = 5},
	/* Watchdog clock period is 256 * 32 / 32768 = 250 ms */
	{.limit_ms = 60000, .twcp = 8, .wdcp = 5},
	/* Watchdog clock period is 1024 * 256 / 32768 = 8 sec */
	{.limit_ms = 1800000, .twcp = 10, .wdcp = 8},
	/* Watchdog clock period is 1024 * 32768 / 32768 = 1024 sec */
	{.limit_ms = UINT32_MAX, .twcp = 10, .wdcp = 15},
};

struct wdt_npcm_config {
	struct twd_reg *base;
	mm_reg_t scfg_base;
};

struct wdt_npcm_data {
	/* Timeout callback, called on the T0OUT event before the reset */
	wdt_callback_t cb;
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

	end = sys_timepoint_calc(K_MSEC(NPCM_T0CSR_RST_TIMEOUT_MS(inst->twcp)));
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
	/*
	 * Uptime does not advance at PRE_KERNEL_1, so also cap the iterations.
	 * With the core at 96 MHz and about 10 cycles per iteration (register
	 * read), 100000 iterations take about 10 ms, well above
	 * NPCM_WDT_STOP_TIMEOUT_MS.
	 */
	int spins = 100000;

	end = sys_timepoint_calc(K_MSEC(NPCM_WDT_STOP_TIMEOUT_MS));
	while (IS_BIT_SET(inst->t0csr, NPCM_T0CSR_WD_RUN)) {
		if (--spins <= 0 || sys_timepoint_expired(end)) {
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

/*
 * T0 keeps running after a timeout was reported, so a T0OUT event may be
 * pending: clear the terminal count status (cleared on read) and the
 * pending interrupt before enabling it, so that only a new timeout is
 * reported.
 */
static void wdt_npcm_t0out_irq_arm(const struct device *dev)
{
	const struct wdt_npcm_config *config = dev->config;
	struct twd_reg *const inst = config->base;

	(void)inst->t0csr;
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
	irq_enable(DT_INST_IRQN(0));
}

static int wdt_npcm_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_npcm_config *config = dev->config;
	struct wdt_npcm_data *const data = dev->data;
	struct twd_reg *const inst = config->base;

	if (IS_BIT_SET(inst->t0csr, NPCM_T0CSR_WD_RUN)) {
		return -EBUSY;
	}

	/* Only one timeout channel is supported */
	if (data->timeout_installed) {
		return -ENOMEM;
	}

	/* No window watchdog support */
	if (cfg->window.min != 0) {
		return -EINVAL;
	}

	if (cfg->window.max > NPCM_WDT_MAX_WND_TIME_MS || cfg->window.max == 0) {
		return -EINVAL;
	}

	if (cfg->callback != NULL && cfg->window.max > NPCM_WDT_MAX_CB_TIME_MS) {
		return -EINVAL;
	}

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	data->timeout = cfg->window.max;
	data->cb = cfg->callback;
	data->timeout_installed = true;

	return 0;
}

static int wdt_npcm_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_npcm_config *config = dev->config;
	struct wdt_npcm_data *const data = dev->data;
	struct twd_reg *const inst = config->base;
	const struct wdt_npcm_prescaler *prescaler = &wdt_npcm_prescalers[0];
	uint64_t timeout_ms;
	uint64_t ticks;
	int ret;

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

	while (data->timeout >= prescaler->limit_ms) {
		prescaler++;
	}

	inst->twcp = prescaler->twcp;
	inst->wdcp = prescaler->wdcp;

	/*
	 * T0 timer preset value: the T0OUT event (callback) occurs every
	 * (PRESET + 1) T0IN clocks, i.e. at the requested timeout.
	 */
	ticks = DIV_ROUND_UP((uint64_t)data->timeout * NPCM_WDT_CLK,
			     BIT(prescaler->twcp) * 1000ULL);
	inst->twdt0 = CLAMP(ticks - 1, 1, UINT16_MAX);

	/*
	 * Watchdog count value: timeout in watchdog clocks. With a callback,
	 * delay the reset by CONFIG_WDT_NPCM_WARNING_LEADING_TIME_MS so that
	 * the callback can run before the SoC is reset.
	 */
	timeout_ms = data->timeout;
	if (data->cb != NULL) {
		timeout_ms += CONFIG_WDT_NPCM_WARNING_LEADING_TIME_MS;
	}
	ticks = DIV_ROUND_UP(timeout_ms * NPCM_WDT_CLK,
			     BIT64(prescaler->twcp + prescaler->wdcp) * 1000ULL);
	inst->wdcnt = CLAMP(ticks, 1, UINT8_MAX);

	LOG_DBG("WDT setup: TWCP %d, WDCP %d, TWDT0 %d, WDCNT %d", inst->twcp, inst->wdcp,
		inst->twdt0, inst->wdcnt);

	ret = wdt_npcm_t0_reload(dev);
	if (ret != 0) {
		return ret;
	}

	data->last_watchdog_touch = k_uptime_get();

	if (data->cb != NULL) {
		wdt_npcm_t0out_irq_arm(dev);
	}

	/* Enable the T0OUT event */
	inst->t0csr = inst->t0csr | BIT(NPCM_T0CSR_T0EN);

	return 0;
}

static int wdt_npcm_disable(const struct device *dev)
{
	const struct wdt_npcm_config *config = dev->config;
	struct wdt_npcm_data *const data = dev->data;
	struct twd_reg *const inst = config->base;

	data->timeout_installed = false;
	irq_disable(DT_INST_IRQN(0));

	/* Stop the T0OUT event */
	inst->t0csr = inst->t0csr & ~BIT(NPCM_T0CSR_T0EN);

	if (!IS_BIT_SET(inst->t0csr, NPCM_T0CSR_WD_RUN)) {
		return -EFAULT;
	}

	/*
	 * Wait after the last touch before stopping the watchdog. Skip the
	 * wait if it was not touched by this driver: at PRE_KERNEL_1
	 * (CONFIG_WDT_DISABLE_AT_BOOT) the system clock is not running yet,
	 * and the time spent in boot ROM already exceeds it.
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

	return wdt_npcm_wait_stopped(dev);
}

static int wdt_npcm_feed(const struct device *dev, int channel_id)
{
	const struct wdt_npcm_config *config = dev->config;
	struct wdt_npcm_data *const data = dev->data;
	struct twd_reg *const inst = config->base;
	int ret;

	if (channel_id != 0 || !IS_BIT_SET(inst->t0csr, NPCM_T0CSR_WD_RUN)) {
		return -EINVAL;
	}

	inst->wdsdm = NPCM_WDSDM_FEED;
	data->last_watchdog_touch = k_uptime_get();

	ret = wdt_npcm_t0_reload(dev);

	/* Re-arm the timeout event after it was reported */
	if (data->cb != NULL) {
		wdt_npcm_t0out_irq_arm(dev);
	}

	return ret;
}

static void wdt_npcm_t0out_isr(const struct device *dev)
{
	const struct wdt_npcm_config *config = dev->config;
	struct wdt_npcm_data *const data = dev->data;
	struct twd_reg *const inst = config->base;

	/* Reading T0CSR clears the terminal count (TC) status */
	(void)inst->t0csr;

	/*
	 * T0OUT is periodic: report the timeout once, the SoC is reset after
	 * the remaining watchdog delay cycles.
	 */
	irq_disable(DT_INST_IRQN(0));

	if (data->cb != NULL) {
		data->cb(dev, 0);
	}
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

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), wdt_npcm_t0out_isr,
		    DEVICE_DT_INST_GET(0), 0);

	return 0;
}

static const struct wdt_npcm_config wdt_npcm_cfg_0 = {
	.base = (struct twd_reg *)DT_INST_REG_ADDR(0),
	.scfg_base = DT_REG_ADDR(DT_INST_PHANDLE(0, nuvoton_scfg)),
};

static struct wdt_npcm_data wdt_npcm_data_0;

DEVICE_DT_INST_DEFINE(0, wdt_npcm_init, NULL, &wdt_npcm_data_0, &wdt_npcm_cfg_0, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wdt_npcm_driver_api);
