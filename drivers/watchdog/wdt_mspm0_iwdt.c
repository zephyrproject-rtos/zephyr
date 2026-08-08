/*
 * Copyright (c) 2026 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT ti_mspm0_iwdt

LOG_MODULE_REGISTER(wdt_mspm0_iwdt, CONFIG_WDT_LOG_LEVEL);

/*
 * IWDT register map — LFSS.IPSPECIFIC_WDT sub-block, TRM "Low-Frequency
 * SubSystem" chapter. Not a standalone peripheral: lives at a fixed offset
 * inside the LFSS block that also hosts the RTC.
 */

struct lfss_iwdt_regs {
	volatile uint32_t WDTEN;     /* 0x00 */
	volatile uint32_t WDTDBGCTL; /* 0x04 */
	volatile uint32_t WDTCTL;    /* 0x08 */
	volatile uint32_t WDTCNTRST; /* 0x0C — feed: write exactly 0x03A7 */
	volatile uint32_t WDTSTAT;   /* 0x10 — RUN bit (read-only) */
	uint32_t RESERVED[58];
	volatile uint32_t WDTLOCK; /* 0xFC */
};

BUILD_ASSERT(offsetof(struct lfss_iwdt_regs, WDTCNTRST) == 0x0CU);
BUILD_ASSERT(offsetof(struct lfss_iwdt_regs, WDTLOCK) == 0xFCU);

/*
 * WDTEN/WDTCTL/WDTLOCK need the exact key in the top byte on every write.
 * Wrong key = POR reset, not just a rejected write.
 */
#define IWDT_WDTEN_KEY    0xEE000000U
#define IWDT_WDTEN_ENABLE 0x00000001U

#define IWDT_WDTDBGCTL_STOP 0x00000000U /* halt with core */
#define IWDT_WDTDBGCTL_FREE 0x00000001U /* keep counting  */

#define IWDT_WDTCTL_KEY         0xC6000000U
/* CLKDIV field [2:0] — divide clock source by CLKDIV+1 (÷1..÷8) */
#define IWDT_WDTCTL_CLKDIV_MASK 0x00000007U
/* PER field [6:4] — total watchdog counter width */
#define IWDT_WDTCTL_PER_25      0x00000000U
#define IWDT_WDTCTL_PER_21      0x00000010U
#define IWDT_WDTCTL_PER_18      0x00000020U
#define IWDT_WDTCTL_PER_15      0x00000030U
#define IWDT_WDTCTL_PER_12      0x00000040U
#define IWDT_WDTCTL_PER_10      0x00000050U
#define IWDT_WDTCTL_PER_8       0x00000060U
#define IWDT_WDTCTL_PER_6       0x00000070U

/* Feed value — writing anything else to WDTCNTRST is ignored. */
#define IWDT_WDTCNTRST_KEY 0x000003A7U

#define IWDT_WDTSTAT_RUN 0x00000001U

#define IWDT_WDTLOCK_KEY     0xBD000000U
#define IWDT_WDTLOCK_PROTECT 0x00000001U

/* IWDT counts directly off LFOSC (TRM 38.1) — no clock_control API for it. */
#define IWDT_NOMINAL_CLOCK_HZ DT_PROP(DT_NODELABEL(lfosc), clock_frequency)

/* WDTSTAT.RUN lags an ENABLE write by up to a couple of LFOSC cycles. */
#define IWDT_WDTSTAT_SETTLE_US ((2U * USEC_PER_SEC) / IWDT_NOMINAL_CLOCK_HZ)

struct wwdt_mspm_iwdt_config {
	struct lfss_iwdt_regs *base;
};

struct wwdt_mspm_iwdt_data {
	uint8_t per;           /* WDTCTL.PER raw value */
	uint8_t clock_divider; /* WDTCTL.CLKDIV raw 0-7 */
	atomic_t timeout_valid; /* true after install_timeout(), false after disable() */
	atomic_t is_setup; /* true after setup(), false after disable(); read in feed() ISR path */
	struct k_mutex lock; /* guards setup()/disable()/install_timeout() only */
};

struct iwdt_period_lut {
	uint32_t per;        /* WDTCTL.PER raw value */
	uint32_t per_counts; /* 2^n counter counts */
};

static int wwdt_mspm_iwdt_calculate_timeout(const struct wdt_timeout_cfg *cfg,
					    struct wwdt_mspm_iwdt_data *data)
{
	static const struct iwdt_period_lut period_lut[] = {
		{IWDT_WDTCTL_PER_6, 64},       {IWDT_WDTCTL_PER_8, 256},
		{IWDT_WDTCTL_PER_10, 1024},    {IWDT_WDTCTL_PER_12, 4096},
		{IWDT_WDTCTL_PER_15, 32768},   {IWDT_WDTCTL_PER_18, 262144},
		{IWDT_WDTCTL_PER_21, 2097152}, {IWDT_WDTCTL_PER_25, 33554432},
	};
	uint32_t max_ms = cfg->window.max;
	uint32_t abs_max_ms;
	uint32_t lut_idx = 0;

	/* IWDT has no windowing hardware — a minimum feed window can't be enforced. */
	if (cfg->window.min != 0) {
		LOG_ERR("Install timeout failed. IWDT cannot enforce window.min");
		return -EINVAL;
	}

	/* No interrupt capability at all — no callback path exists. */
	if (cfg->callback) {
		LOG_ERR("Install timeout failed. Callback not supported");
		return -ENOTSUP;
	}

	abs_max_ms = (uint32_t)(((uint64_t)8 * period_lut[7].per_counts * 1000U) /
				IWDT_NOMINAL_CLOCK_HZ);

	if (max_ms == 0U || max_ms > abs_max_ms) {
		LOG_ERR("Invalid window timing (max=%u ms, abs_max=%u ms)", max_ms, abs_max_ms);
		return -EINVAL;
	}

	for (uint32_t i = 0; i < ARRAY_SIZE(period_lut); i++) {
		uint32_t max_timeout_ms =
			(uint32_t)(((uint64_t)8 * period_lut[i].per_counts * 1000U) /
				   IWDT_NOMINAL_CLOCK_HZ);

		if (max_ms <= max_timeout_ms) {
			lut_idx = i;
			break;
		}
	}

	data->per = period_lut[lut_idx].per;

	/* Walk CLKDIV 0..7 to find the smallest divider whose timeout >= max_ms. */
	for (data->clock_divider = 0; data->clock_divider < 8; data->clock_divider++) {
		uint32_t actual_timeout = (uint32_t)(((uint64_t)(data->clock_divider + 1U) *
						      period_lut[lut_idx].per_counts * 1000U) /
						     IWDT_NOMINAL_CLOCK_HZ);

		if (max_ms <= actual_timeout) {
			break;
		}
	}
	data->clock_divider = MIN(data->clock_divider, 7U);

	return 0;
}

static int wwdt_mspm_iwdt_setup(const struct device *dev, uint8_t options)
{
	const struct wwdt_mspm_iwdt_config *config = dev->config;
	struct wwdt_mspm_iwdt_data *data = dev->data;
	struct lfss_iwdt_regs *base = config->base;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!atomic_get(&data->timeout_valid)) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	if (atomic_get(&data->is_setup)) {
		LOG_ERR("IWDT already running — call wdt_disable() first");
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	/*
	 * IWDT lives in the always-on VBAT domain — no documented sleep-pause
	 * concept for this block. Reject rather than silently ignore.
	 */
	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		k_mutex_unlock(&data->lock);
		return -ENOTSUP;
	}

	base->WDTLOCK = IWDT_WDTLOCK_KEY; /* PROTECT=0: unlock WDTEN/WDTCTL */

	base->WDTDBGCTL =
		(options & WDT_OPT_PAUSE_HALTED_BY_DBG) ? IWDT_WDTDBGCTL_STOP : IWDT_WDTDBGCTL_FREE;

	base->WDTCTL =
		IWDT_WDTCTL_KEY | (data->clock_divider & IWDT_WDTCTL_CLKDIV_MASK) | data->per;

	base->WDTEN = IWDT_WDTEN_KEY | IWDT_WDTEN_ENABLE;
	base->WDTLOCK = IWDT_WDTLOCK_KEY | IWDT_WDTLOCK_PROTECT;

	atomic_set(&data->is_setup, 1);
	k_mutex_unlock(&data->lock);
	return 0;
}

static int wwdt_mspm_iwdt_disable(const struct device *dev)
{
	const struct wwdt_mspm_iwdt_config *config = dev->config;
	struct wwdt_mspm_iwdt_data *data = dev->data;
	struct lfss_iwdt_regs *base = config->base;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!atomic_get(&data->is_setup)) {
		k_mutex_unlock(&data->lock);
		return -EFAULT;
	}

	/* Unlike WWDT, WDTEN isn't documented as irrevocable — clear it and
	 * confirm via WDTSTAT.RUN instead of assuming.
	 */
	base->WDTLOCK = IWDT_WDTLOCK_KEY; /* PROTECT=0: unlock WDTEN/WDTCTL */
	base->WDTEN = IWDT_WDTEN_KEY;

	/* WDTSTAT.RUN needs a couple of LFOSC cycles to catch up. */
	k_busy_wait(IWDT_WDTSTAT_SETTLE_US);

	if (base->WDTSTAT & IWDT_WDTSTAT_RUN) {
		ret = -EPERM;
	} else {
		atomic_set(&data->is_setup, 0);
		atomic_set(&data->timeout_valid, 0);
		ret = 0;
	}

	base->WDTLOCK = IWDT_WDTLOCK_KEY | IWDT_WDTLOCK_PROTECT;

	k_mutex_unlock(&data->lock);
	return ret;
}

static int wwdt_mspm_iwdt_install_timeout(const struct device *dev,
					  const struct wdt_timeout_cfg *cfg)
{
	struct wwdt_mspm_iwdt_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (atomic_get(&data->is_setup)) {
		LOG_ERR("Install timeout failed. IWDT is already running");
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	/* Single channel: slot freed by disable(). Second install before disable -> -ENOMEM. */
	if (atomic_get(&data->timeout_valid)) {
		k_mutex_unlock(&data->lock);
		return -ENOMEM;
	}

	/* Hardware always PORs on timeout — no other reset action exists. */
	if ((cfg->flags & WDT_FLAG_RESET_MASK) != WDT_FLAG_RESET_SOC) {
		LOG_ERR("Install timeout failed. Unsupported reset flags 0x%x", cfg->flags);
		k_mutex_unlock(&data->lock);
		return -ENOTSUP;
	}

	ret = wwdt_mspm_iwdt_calculate_timeout(cfg, data);
	if (ret == 0) {
		atomic_set(&data->timeout_valid, 1);
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int wwdt_mspm_iwdt_feed(const struct device *dev, int channel_id)
{
	const struct wwdt_mspm_iwdt_config *config = dev->config;
	struct wwdt_mspm_iwdt_data *data = dev->data;

	/* Single channel (0) only. */
	if (channel_id != 0) {
		return -EINVAL;
	}

	if (!atomic_get(&data->is_setup)) {
		return -EINVAL;
	}

	/* Any value other than 0x03A7 is ignored by hardware. */
	config->base->WDTCNTRST = IWDT_WDTCNTRST_KEY;

	return 0;
}

static int wwdt_mspm_iwdt_init(const struct device *dev)
{
	struct wwdt_mspm_iwdt_data *data = dev->data;

	/* LFSS is always-on — no RSTCTL/PWREN sequence needed, unlike WWDT0. */
	k_mutex_init(&data->lock);

	return 0;
}

static DEVICE_API(wdt, wwdt_mspm_iwdt_driver_api) = {
	.setup = wwdt_mspm_iwdt_setup,
	.disable = wwdt_mspm_iwdt_disable,
	.install_timeout = wwdt_mspm_iwdt_install_timeout,
	.feed = wwdt_mspm_iwdt_feed,
};

#define WWDT_MSPM_IWDT_INIT(index)                                                                 \
	static const struct wwdt_mspm_iwdt_config wwdt_mspm_iwdt_cfg_##index = {                   \
		.base = (struct lfss_iwdt_regs *)DT_INST_REG_ADDR(index),                          \
	};                                                                                         \
                                                                                                   \
	static struct wwdt_mspm_iwdt_data wwdt_mspm_iwdt_data_##index;                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, wwdt_mspm_iwdt_init, NULL, &wwdt_mspm_iwdt_data_##index,      \
			      &wwdt_mspm_iwdt_cfg_##index, POST_KERNEL,                            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wwdt_mspm_iwdt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(WWDT_MSPM_IWDT_INIT)
