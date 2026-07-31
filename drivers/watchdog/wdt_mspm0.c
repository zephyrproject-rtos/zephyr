/*
 * Copyright (c) 2024 Texas Instruments Inc.
 * Copyright (c) 2026 Linumiz.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT ti_mspm0_watchdog

LOG_MODULE_REGISTER(wdt_mspm0, CONFIG_WDT_LOG_LEVEL);

/* WWDT register map — TRM "Windowed Watchdog Timer" chapter */

struct wwdt_mspm_gprcm {
	volatile uint32_t PWREN;
	volatile uint32_t RSTCTL;
	uint32_t RESERVED[3];
	volatile uint32_t STAT;
};

struct wwdt_mspm_regs {
	uint32_t RESERVED0[512];      /* 0x000–0x7FF */
	struct wwdt_mspm_gprcm GPRCM; /* 0x800       */
	uint32_t RESERVED1[512];
	volatile uint32_t PDBGCTL; /* 0x1018 — debug halt control      */
	uint32_t RESERVED2;
	volatile uint32_t IIDX; /* 0x1020 */
	uint32_t RESERVED3;
	volatile uint32_t IMASK; /* 0x1028 */
	uint32_t RESERVED4;
	volatile uint32_t RIS; /* 0x1030 */
	uint32_t RESERVED5;
	volatile uint32_t MIS; /* 0x1038 */
	uint32_t RESERVED6;
	volatile uint32_t ISET; /* 0x1040 */
	uint32_t RESERVED7;
	volatile uint32_t ICLR; /* 0x1048 */
	uint32_t RESERVED8[37];
	volatile uint32_t EVT_MODE; /* 0x10E0 */
	uint32_t RESERVED9[6];
	volatile uint32_t DESC;       /* 0x10FC */
	volatile uint32_t WWDTCTL0;   /* 0x1100 — starts watchdog on write */
	volatile uint32_t WWDTCTL1;   /* 0x1104 — window slot select       */
	volatile uint32_t WWDTCNTRST; /* 0x1108 — feed: write 0x00A7       */
	volatile uint32_t WWDTSTAT;   /* 0x110C — RUN bit (read-only)      */
};

BUILD_ASSERT(offsetof(struct wwdt_mspm_regs, GPRCM) == 0x0800U);
BUILD_ASSERT(offsetof(struct wwdt_mspm_regs, PDBGCTL) == 0x1018U);
BUILD_ASSERT(offsetof(struct wwdt_mspm_regs, IMASK) == 0x1028U);
BUILD_ASSERT(offsetof(struct wwdt_mspm_regs, ICLR) == 0x1048U);
BUILD_ASSERT(offsetof(struct wwdt_mspm_regs, WWDTCTL0) == 0x1100U);
BUILD_ASSERT(offsetof(struct wwdt_mspm_regs, WWDTSTAT) == 0x110CU);

/* GPRCM.PWREN */
#define WWDT_PWREN_KEY    0x26000000U
#define WWDT_PWREN_ENABLE 0x00000001U

/* GPRCM.RSTCTL */
#define WWDT_RSTCTL_KEY     0xB1000000U
#define WWDT_RSTCTL_STKYCLR 0x00000002U
#define WWDT_RSTCTL_ASSERT  0x00000001U

/* WWDTCTL0 — key required on every write; wrong key → ESM error */
#define WWDT_CTL0_KEY         0xC9000000U
/* PER field [6:4] */
#define WWDT_CTL0_PER_25      0x00000000U
#define WWDT_CTL0_PER_21      0x00000010U
#define WWDT_CTL0_PER_18      0x00000020U
#define WWDT_CTL0_PER_15      0x00000030U
#define WWDT_CTL0_PER_12      0x00000040U
#define WWDT_CTL0_PER_10      0x00000050U
#define WWDT_CTL0_PER_8       0x00000060U
#define WWDT_CTL0_PER_6       0x00000070U
/* WINDOW0 [10:8] and WINDOW1 [14:12] field offsets */
#define WWDT_CTL0_WINDOW0_OFS 8U
#define WWDT_CTL0_WINDOW1_OFS 12U

/* WWDTCTL1 */
#define WWDT_CTL1_KEY 0xBE000000U

/* WWDTCNTRST — write exactly this magic; anything else → ESM error */
#define WWDT_CNTRST_KEY 0x00A7U

/* WWDTSTAT */
#define WWDT_STAT_RUN 0x00000001U

/* PDBGCTL — debug halt behavior */
#define WWDT_PDBGCTL_FREE 0x00000001U /* keep counting under debugger */

struct wwdt_mspm_config {
	struct wwdt_mspm_regs *base;
	uint8_t reset_action;
	uint8_t closed_window;
};

struct wwdt_mspm_data {
	uint8_t period_count;
	uint8_t clock_divider;
	uint16_t window_count;
	bool timeout_valid; /* true after install_timeout(), false after disable() */
	bool is_setup;      /* true after setup(), false after disable() */
	struct k_mutex lock;
};

struct wwdt_period_lut {
	uint8_t period_count;
	uint32_t max_msec;
	uint32_t interval;
};

static int wwdt_mspm_calculate_timeout_periods(const struct device *dev,
						const struct wdt_timeout_cfg *cfg)
{
	struct wwdt_mspm_data *data = dev->data;
	struct wwdt_period_lut *lut_entry = NULL;
	uint32_t max_ms = cfg->window.max;
	uint32_t min_ms = cfg->window.min;
	uint32_t actual_timeout;
	uint8_t window_idx;

	struct wwdt_period_lut period_lut[] = {
		/* Timer_max_period_count,	 max_timeout(ms),   interval(ms) */
		{WWDT_CTL0_PER_6, 16, 2},          {WWDT_CTL0_PER_8, 64, 8},
		{WWDT_CTL0_PER_10, 256, 32},       {WWDT_CTL0_PER_12, 1000, 125},
		{WWDT_CTL0_PER_15, 8000, 1000},    {WWDT_CTL0_PER_18, 64000, 8000},
		{WWDT_CTL0_PER_21, 512000, 64000}, {WWDT_CTL0_PER_25, 8192000, 1024000}};

	uint8_t window_sixteenths[] = {0, 2, 3, 4, 8, 12, 13, 14};

	if (max_ms > period_lut[7].max_msec || min_ms >= max_ms) {
		LOG_ERR("Install timeout failed. Invalid window timing");
		return -EINVAL;
	}

	/* Find appropriate period count (PER_count) */
	for (uint8_t i = 0; i < 8; i++) {
		if (max_ms <= period_lut[i].max_msec) {
			lut_entry = &period_lut[i];
			break;
		}
	}

	data->period_count = lut_entry->period_count;

	/*
	 * Determine clock divider based on the period count. Since rounding up
	 * is the defined behavior, walking up and checking for when the value is
	 * equal or underneath will yield the rounded up value
	 */
	actual_timeout = lut_entry->interval;
	for (data->clock_divider = 0; data->clock_divider < 8; data->clock_divider++) {
		if (max_ms <= actual_timeout) {
			break;
		}
		actual_timeout += lut_entry->interval;
	}

	/* Determine closed window as per the requested lower limit of watchdog feed timeout */
	for (window_idx = 0; window_idx < ARRAY_SIZE(window_sixteenths); window_idx++) {
		if (min_ms <= (actual_timeout * window_sixteenths[window_idx] / 16)) {
			break;
		}
	}

	if (window_idx >= ARRAY_SIZE(window_sixteenths)) {
		LOG_ERR("Install timeout failed. min_ms %u cannot be enforced", min_ms);
		return -EINVAL;
	}

	data->window_count = (window_idx << WWDT_CTL0_WINDOW0_OFS);

	return 0;
}

static int wwdt_mspm_setup(const struct device *dev, uint8_t options)
{
	const struct wwdt_mspm_config *config = dev->config;
	struct wwdt_mspm_data *data = dev->data;
	struct wwdt_mspm_regs *base = config->base;
	uint32_t window0_closed;
	uint32_t window1_closed;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->timeout_valid) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	if (data->is_setup) {
		LOG_ERR("WWDT already running — call wdt_disable() first");
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		/* hw_wwdt.h: STISM has no effect for the global Window Watchdog. */
		k_mutex_unlock(&data->lock);
		return -ENOTSUP;
	}

	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) != WDT_OPT_PAUSE_HALTED_BY_DBG) {
		/* On reset the MSPM0 is set to halt with the core halting */
		base->PDBGCTL = WWDT_PDBGCTL_FREE;
	}

	/* This call enables the Watchdog */
	base->WWDTCTL1 = WWDT_CTL1_KEY | (config->closed_window & 1U);

	if (config->closed_window) {
		window0_closed = 0;
		window1_closed = data->window_count;
	} else {
		window0_closed = data->window_count;
		window1_closed = 0;
	}

	base->WWDTCTL0 = WWDT_CTL0_KEY | data->clock_divider | data->period_count |
			 window0_closed |
			 (window1_closed << (WWDT_CTL0_WINDOW1_OFS - WWDT_CTL0_WINDOW0_OFS));

	data->is_setup = true;
	k_mutex_unlock(&data->lock);
	return 0;
}

static int wwdt_mspm_disable(const struct device *dev)
{
	struct wwdt_mspm_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	/*
	 * hw_wwdt.h: "For safety devices a watchdog reset by software is not
	 * possible." Once started, the WDT runs until SoC reset. -EFAULT if
	 * never started (also frees the install slot), -EPERM if started.
	 */
	if (data->is_setup) {
		ret = -EPERM;
	} else {
		data->timeout_valid = false;
		ret = -EFAULT;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int wwdt_mspm_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wwdt_mspm_config *config = dev->config;
	struct wwdt_mspm_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Cannot install timeout if the WWDT is already running */
	if (data->is_setup) {
		LOG_ERR("Install timeout failed. WWDT is already running");
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	/* Single channel: slot freed by disable(). Second install before disable -> -ENOMEM. */
	if (data->timeout_valid) {
		k_mutex_unlock(&data->lock);
		return -ENOMEM;
	}

	if (cfg->callback) {
		LOG_ERR("Install timeout failed. Callback not supported");
		k_mutex_unlock(&data->lock);
		return -ENOTSUP;
	}

	if (cfg->flags != config->reset_action) {
		LOG_ERR("Install timeout failed. Reset action mismatch");
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	/*
	 * To calculate the timeout period as per :
	 * TIMEOUT = (CLKDIV + 1) * PER_count / 32768 (LFCLK Frequency)
	 */
	ret = wwdt_mspm_calculate_timeout_periods(dev, cfg);
	if (ret == 0) {
		data->timeout_valid = true;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int wwdt_mspm_feed(const struct device *dev, int channel_id)
{
	const struct wwdt_mspm_config *config = dev->config;
	struct wwdt_mspm_data *data = dev->data;

	ARG_UNUSED(channel_id);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->is_setup) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	config->base->WWDTCNTRST = WWDT_CNTRST_KEY;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int wwdt_mspm_init(const struct device *dev)
{
	struct wwdt_mspm_regs *base = ((const struct wwdt_mspm_config *)dev->config)->base;
	struct wwdt_mspm_data *data = dev->data;

	k_mutex_init(&data->lock);

	/* Reset peripheral and enable power — matches counter/PWM init sequence. */
	base->GPRCM.RSTCTL = WWDT_RSTCTL_KEY | WWDT_RSTCTL_STKYCLR | WWDT_RSTCTL_ASSERT;
	base->GPRCM.PWREN = WWDT_PWREN_KEY | WWDT_PWREN_ENABLE;
	k_busy_wait(1U);

	return 0;
}

static DEVICE_API(wdt, wwdt_mspm_driver_api) = {
	.setup = wwdt_mspm_setup,
	.disable = wwdt_mspm_disable,
	.install_timeout = wwdt_mspm_install_timeout,
	.feed = wwdt_mspm_feed
};

#define WWDT_MSPM_INIT(index)								\
static const struct wwdt_mspm_config wwdt_mspm_cfg_##index = {			\
	.base = (struct wwdt_mspm_regs *)DT_INST_REG_ADDR(index),			\
	.reset_action = COND_CODE_1(DT_INST_PROP(index, ti_watchdog_reset_action),	\
				    (WDT_FLAG_RESET_SOC), (WDT_FLAG_RESET_CPU_CORE)),	\
	.closed_window = DT_INST_PROP(index, closed_window),				\
};											\
											\
static struct wwdt_mspm_data wwdt_mspm_data_##index;					\
											\
DEVICE_DT_INST_DEFINE(index, wwdt_mspm_init, NULL, &wwdt_mspm_data_##index,		\
		      &wwdt_mspm_cfg_##index, POST_KERNEL,				\
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,				\
		      &wwdt_mspm_driver_api);						\

DT_INST_FOREACH_STATUS_OKAY(WWDT_MSPM_INIT)
