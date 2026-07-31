/*
 * Copyright (c) 2024 Texas Instruments Inc.
 * Copyright (c) 2026 Linumiz.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
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
#define WWDT_CTL0_KEY           0xC9000000U
/* PER field [6:4] */
#define WWDT_CTL0_PER_25        0x00000000U
#define WWDT_CTL0_PER_21        0x00000010U
#define WWDT_CTL0_PER_18        0x00000020U
#define WWDT_CTL0_PER_15        0x00000030U
#define WWDT_CTL0_PER_12        0x00000040U
#define WWDT_CTL0_PER_10        0x00000050U
#define WWDT_CTL0_PER_8         0x00000060U
#define WWDT_CTL0_PER_6         0x00000070U
/* WINDOW0 [10:8] and WINDOW1 [14:12] field offsets */
#define WWDT_CTL0_WINDOW0_OFS   8U
#define WWDT_CTL0_WINDOW1_OFS   12U
/* MODE [16] */
#define WWDT_CTL0_MODE_WINDOW   0x00000000U
#define WWDT_CTL0_MODE_INTERVAL 0x00010000U

/* WWDTCTL1 */
#define WWDT_CTL1_KEY 0xBE000000U

/* WWDTCNTRST — write exactly this magic; anything else → ESM error */
#define WWDT_CNTRST_KEY 0x00A7U

/* WWDTSTAT */
#define WWDT_STAT_RUN 0x00000001U

/* PDBGCTL — debug halt behavior */
#define WWDT_PDBGCTL_STOP 0x00000000U /* halt with core */
#define WWDT_PDBGCTL_FREE 0x00000001U /* keep counting  */

/*
 * CPU_INT — single source: INTTIM (bit 0).
 * Fires only in interval-timer mode (WWDTCTL0.MODE = MODE_INTERVAL).
 * In window-watchdog mode violations go to the ESM, not this interrupt.
 */
#define WWDT_INT_INTTIM 0x00000001U

/* True when the DT node has an interrupts property (interval mode capable) */
#define WWDT_MSPM_HAS_IRQ(n) DT_INST_NODE_HAS_PROP(n, interrupts)

struct wwdt_mspm_config {
	struct wwdt_mspm_regs *base;
	uint8_t closed_window;
	/* Zephyr flag expected by install_timeout() — WWDT always does SYSRST. */
	uint8_t reset_action;
	const struct device *clock_dev;
	struct mspm0_sys_clock clock_subsys;
	/* NULL on instances without an interrupts DT property (window-mode only). */
	void (*irq_config_func)(const struct device *dev);
	int irq_num;
};

struct wwdt_mspm_data {
	uint8_t period_count;  /* WWDTCTL0.PER field raw bits */
	uint8_t clock_divider; /* WWDTCTL0.CLKDIV raw 0–7 */
	uint16_t window_count; /* WINDOW0 field raw bits (shifted to [10:8]) */
	bool timeout_valid;    /* true after install_timeout(), false after disable() */
	bool is_interval_mode; /* true when WDT_FLAG_RESET_NONE path selected */
	/*
	 * Authoritative "driver active" flag — set by setup(), cleared by disable().
	 * Cannot use WWDTSTAT.RUN: it stays 1 after interval-mode disable() since
	 * the hardware counter cannot be stopped. volatile: written by thread
	 * (disable()), read by INTTIM ISR.
	 */
	volatile bool is_setup;
	volatile wdt_callback_t callback; /* volatile: written by thread, read by ISR */
	struct k_mutex lock;
};

struct wwdt_period_lut {
	uint32_t period_count; /* WWDTCTL0.PER raw value */
	uint32_t per_counts;   /* 2^n counter counts      */
};

static int wwdt_mspm_calculate_timeout_periods(const struct device *dev,
					       const struct wdt_timeout_cfg *cfg)
{
	/* PER field values and their corresponding counter bit-widths */
	static const struct wwdt_period_lut period_lut[] = {
		{WWDT_CTL0_PER_6, 64},       {WWDT_CTL0_PER_8, 256},
		{WWDT_CTL0_PER_10, 1024},    {WWDT_CTL0_PER_12, 4096},
		{WWDT_CTL0_PER_15, 32768},   {WWDT_CTL0_PER_18, 262144},
		{WWDT_CTL0_PER_21, 2097152}, {WWDT_CTL0_PER_25, 33554432},
	};
	const struct wwdt_mspm_config *config = dev->config;
	struct wwdt_mspm_data *data = dev->data;
	struct mspm0_sys_clock clock_subsys = config->clock_subsys;
	uint32_t max_ms = cfg->window.max;
	uint32_t min_ms = cfg->window.min;
	uint32_t clock_freq;
	uint8_t window_idx;
	int ret;

	ret = clock_control_get_rate(config->clock_dev, (clock_control_subsys_t)&clock_subsys,
				     &clock_freq);
	if (ret != 0) {
		LOG_ERR("Failed to get LFCLK rate: %d", ret);
		return ret;
	}

	if (clock_freq == 0U) {
		LOG_ERR("LFCLK rate is zero");
		return -EINVAL;
	}

	/*
	 * Closed-window fractions as n/16 of the timeout period.
	 * Maps to: 0%, 12.5%, 18.75%, 25%, 50%, 75%, 81.25%, 87.5%.
	 */
	static const uint8_t window_sixteenths[] = {0, 2, 3, 4, 8, 12, 13, 14};

	/* Compute max achievable timeout: 8 × 2^25 / clock_freq × 1000 ms */
	uint32_t abs_max_ms =
		(uint32_t)(((uint64_t)8 * period_lut[7].per_counts * 1000U) / clock_freq);

	if (max_ms > abs_max_ms || min_ms >= max_ms) {
		LOG_ERR("Invalid window timing (max=%u ms, abs_max=%u ms)", max_ms, abs_max_ms);
		return -EINVAL;
	}

	/* Find the smallest PER where max_timeout_ms (at CLKDIV=7) >= max_ms */
	uint32_t lut_idx = 0;

	for (uint32_t i = 0; i < ARRAY_SIZE(period_lut); i++) {
		uint32_t max_timeout_ms =
			(uint32_t)(((uint64_t)8 * period_lut[i].per_counts * 1000U) / clock_freq);

		if (max_ms <= max_timeout_ms) {
			lut_idx = i;
			break;
		}
	}

	data->period_count = period_lut[lut_idx].period_count;

	/*
	 * Walk CLKDIV 0→7 to find the smallest divider where the timeout
	 * is >= max_ms (always rounds up, never under-programs the watchdog).
	 */
	uint32_t actual_timeout = 0;

	for (data->clock_divider = 0; data->clock_divider < 8; data->clock_divider++) {
		actual_timeout = (uint32_t)(((uint64_t)(data->clock_divider + 1U) *
					     period_lut[lut_idx].per_counts * 1000U) /
					    clock_freq);
		if (max_ms <= actual_timeout) {
			break;
		}
	}
	data->clock_divider = MIN(data->clock_divider, 7U);

	/* Find the smallest closed-window fraction that enforces min_ms */
	for (window_idx = 0; window_idx < ARRAY_SIZE(window_sixteenths); window_idx++) {
		uint32_t window_ms = actual_timeout * window_sixteenths[window_idx] / 16U;

		if (min_ms <= window_ms) {
			break;
		}
	}

	/*
	 * If no available fraction enforces the requested min_ms, the hardware
	 * cannot satisfy the constraint — return an error rather than silently
	 * programming a wider window than requested.
	 */
	if (window_idx >= ARRAY_SIZE(window_sixteenths)) {
		LOG_ERR("min_ms %u cannot be enforced; max available closed "
			"window is %u ms",
			min_ms,
			actual_timeout * window_sixteenths[ARRAY_SIZE(window_sixteenths) - 1] /
				16U);
		return -EINVAL;
	}

	data->window_count = (window_idx << WWDT_CTL0_WINDOW0_OFS);

	return 0;
}

static void wwdt_mspm_isr(const struct device *dev)
{
	struct wwdt_mspm_data *data = dev->data;
	struct wwdt_mspm_regs *base = ((const struct wwdt_mspm_config *)dev->config)->base;

	base->ICLR = WWDT_INT_INTTIM;

	/*
	 * The hardware counter keeps running after disable() — no stop register
	 * exists. is_setup=false (set by disable() before any IRQ operations)
	 * is the software gate against spurious fires.
	 */
	if (!data->is_setup) {
		return;
	}

	if (data->callback) {
		data->callback(dev, 0);
	}
}

static int wwdt_mspm_setup(const struct device *dev, uint8_t options)
{
	const struct wwdt_mspm_config *config = dev->config;
	struct wwdt_mspm_data *data = dev->data;
	struct wwdt_mspm_regs *base = config->base;

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

	/*
	 * hw_wwdt.h: STISM "has no effect for the global Window Watchdog as
	 * Sleep Mode is not supported." Return -ENOTSUP rather than silently
	 * ignoring the request.
	 */
	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		k_mutex_unlock(&data->lock);
		return -ENOTSUP;
	}

	/* STOP halts with core; FREE keeps counting under debugger (default). */
	base->PDBGCTL =
		(options & WDT_OPT_PAUSE_HALTED_BY_DBG) ? WWDT_PDBGCTL_STOP : WWDT_PDBGCTL_FREE;

	if (data->is_interval_mode) {
		/* Interval-timer mode: INTTIM fires on each expiry, auto-reloads, no reset.
		 */
		uint32_t ctl0 = WWDT_CTL0_KEY | (uint32_t)data->clock_divider |
				(uint32_t)data->period_count | WWDT_CTL0_MODE_INTERVAL;

		base->WWDTCTL0 = ctl0;
		/*
		 * WWDTCNTRST restarts the counter. WWDTCTL0 updates config but
		 * does not reset a running counter — hardware keeps counting across
		 * disable() calls (no stop register on MSPM0 WWDT). Without this
		 * the interval fires at the inherited counter position.
		 * No closed-window constraint in interval mode so write is safe.
		 */
		base->WWDTCNTRST = WWDT_CNTRST_KEY;
		/* Clear stale RIS before enabling IMASK to prevent immediate ISR. */
		base->ICLR = WWDT_INT_INTTIM;
		base->IMASK = WWDT_INT_INTTIM;

		/*
		 * Release mutex before irq_enable(): avoids holding the lock
		 * across IRQ_CONNECT/irq_enable(). feed() itself skips the
		 * lock entirely when called from ISR context (see
		 * wwdt_mspm_feed()), so this isn't required for that reason,
		 * but keeping the critical section short is still good
		 * practice.
		 */
		data->is_setup = true;
		k_mutex_unlock(&data->lock);
		config->irq_config_func(dev);
		return 0;
	}

	/* Window-watchdog mode */
	uint32_t window0_closed;
	uint32_t window1_closed;

	/* Select active window slot — must write WWDTCTL1 before WWDTCTL0 */
	base->WWDTCTL1 = WWDT_CTL1_KEY | (config->closed_window & 1U);

	if (config->closed_window) {
		window0_closed = 0U;
		window1_closed = data->window_count;
	} else {
		window0_closed = data->window_count;
		window1_closed = 0U;
	}

	/*
	 * Writing WWDTCTL0 starts the watchdog immediately.
	 * WINDOW0 encoding is at bits [10:8]; WINDOW1 uses the same encoding
	 * but at bits [14:12] — shift left by (12 - 8) = 4.
	 */
	base->WWDTCTL0 = WWDT_CTL0_KEY | (uint32_t)data->clock_divider |
			 (uint32_t)data->period_count | window0_closed |
			 (window1_closed << (WWDT_CTL0_WINDOW1_OFS - WWDT_CTL0_WINDOW0_OFS));

	data->is_setup = true;
	k_mutex_unlock(&data->lock);
	return 0;
}

static int wwdt_mspm_disable(const struct device *dev)
{
	const struct wwdt_mspm_config *config = dev->config;
	struct wwdt_mspm_data *data = dev->data;
	struct wwdt_mspm_regs *base = config->base;
	bool is_interval;

	k_mutex_lock(&data->lock, K_FOREVER);
	is_interval = data->is_interval_mode;

	if (is_interval) {
		/*
		 * Hardware counter cannot be stopped (no stop register). Mask the
		 * interrupt and gate the ISR via is_setup — that is the only reliable
		 * "logically stopped" indicator.
		 * is_setup cleared FIRST so any pending ISR sees it before irq_disable.
		 */
		int ret = data->is_setup ? 0 : -EFAULT;

		data->is_setup = false;
		data->is_interval_mode = false;
		data->callback = NULL;
		data->timeout_valid = false;

		base->IMASK = 0U;
		base->ICLR = WWDT_INT_INTTIM;

		if (config->irq_num >= 0) {
			irq_disable(config->irq_num);
			NVIC_ClearPendingIRQ(config->irq_num);
		}
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/*
	 * Window-watchdog mode: hw_wwdt.h: "For safety devices a watchdog reset
	 * by software is not possible." Once started, the WDT runs until SoC
	 * reset. -EFAULT if never started (also frees the install slot),
	 * -EPERM if started.
	 */
	if (data->is_setup) {
		k_mutex_unlock(&data->lock);
		return -EPERM;
	}

	data->timeout_valid = false;
	k_mutex_unlock(&data->lock);
	return -EFAULT;
}

static int wwdt_mspm_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wwdt_mspm_config *config = dev->config;
	struct wwdt_mspm_data *data = dev->data;
	wdt_callback_t cb = NULL;
	bool interval_mode = false;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->is_setup) {
		LOG_ERR("Install timeout failed. WWDT is already running");
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	/* Single channel: slot freed by disable(). Second install before disable →
	 * -ENOMEM.
	 */
	if (data->timeout_valid) {
		k_mutex_unlock(&data->lock);
		return -ENOMEM;
	}

	if ((cfg->flags & WDT_FLAG_RESET_MASK) == WDT_FLAG_RESET_NONE) {
		/* Interval-timer mode: INTTIM fires on expiry, auto-reloads, no reset. */
		if (!config->irq_config_func) {
			LOG_ERR("Instance has no interrupt line (missing DT interrupts property)");
			k_mutex_unlock(&data->lock);
			return -ENOTSUP;
		}
		cb = cfg->callback;
		interval_mode = true;
	} else if (cfg->callback) {
		/* Window-watchdog violations route to ESM, not NVIC — no pre-reset
		 * callback.
		 */
		LOG_ERR("Callback requires WDT_FLAG_RESET_NONE");
		k_mutex_unlock(&data->lock);
		return -ENOTSUP;
	} else {
		/*
		 * Window-watchdog mode. WWDT always generates SYSRST on violation
		 * regardless of ti,watchdog-reset-action; the DT property is used
		 * only to validate that the requested flag matches this instance.
		 */
		if ((cfg->flags & WDT_FLAG_RESET_MASK) > WDT_FLAG_RESET_SOC) {
			LOG_ERR("Unsupported reset flags 0x%x", cfg->flags);
			k_mutex_unlock(&data->lock);
			return -ENOTSUP;
		}
		if ((cfg->flags & WDT_FLAG_RESET_MASK) != config->reset_action) {
			LOG_ERR("Unsupported reset flag %u on this instance "
				"(ti,watchdog-reset-action configures %u)",
				cfg->flags & WDT_FLAG_RESET_MASK, config->reset_action);
			k_mutex_unlock(&data->lock);
			return -ENOTSUP;
		}
		interval_mode = false;
	}

	/* Calculate periods before committing — leave state intact on failure. */
	ret = wwdt_mspm_calculate_timeout_periods(dev, cfg);
	if (ret == 0) {
		data->callback = cb;
		data->is_interval_mode = interval_mode;
		data->timeout_valid = true;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int wwdt_mspm_feed(const struct device *dev, int channel_id)
{
	/* Single channel (0) only. */
	if (channel_id != 0) {
		return -EINVAL;
	}

	const struct wwdt_mspm_config *config = dev->config;
	struct wwdt_mspm_data *data = dev->data;
	struct wwdt_mspm_regs *base = config->base;

	/*
	 * May be called from the interval-timer ISR's callback (wwdt_mspm_isr()).
	 * k_mutex_lock() is illegal in ISR context, so skip the lock there — the
	 * register write is safe unsynchronized; worst case is racing a
	 * concurrent disable(), which is harmless since WWDTCNTRST can be
	 * written regardless of software state.
	 */
	if (k_is_in_isr()) {
		if (!data->is_setup) {
			return -EINVAL;
		}
		base->WWDTCNTRST = WWDT_CNTRST_KEY;
		return 0;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->is_setup) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	/* Any value other than 0x00A7 triggers an ESM error. */
	base->WWDTCNTRST = WWDT_CNTRST_KEY;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int wwdt_mspm_init(const struct device *dev)
{
	const struct wwdt_mspm_config *config = dev->config;
	struct wwdt_mspm_data *data = dev->data;
	struct wwdt_mspm_regs *base = config->base;

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
	.feed = wwdt_mspm_feed,
};

/* clang-format off */
#define WWDT_MSPM_IRQ_DEFINE(n)                                                                    \
	static void wwdt_mspm_##n##_irq_config(const struct device *dev)                           \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), wwdt_mspm_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		/* Belt-and-suspenders: clear any pending NVIC bit before enable. */               \
		NVIC_ClearPendingIRQ(DT_INST_IRQN(n));                                            \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

/* Gate IRQ definition per instance on whether the DT node has interrupts */
#define WWDT_MSPM_IRQ_DEFINE_IF_PRESENT(n)                                                         \
	COND_CODE_1(WWDT_MSPM_HAS_IRQ(n), (WWDT_MSPM_IRQ_DEFINE(n)), ())

#define WWDT_MSPM_INIT(index)                                                                      \
	WWDT_MSPM_IRQ_DEFINE_IF_PRESENT(index)                                                     \
                                                                                                   \
	static struct wwdt_mspm_data wwdt_mspm_data_##index;                                       \
                                                                                                   \
	static const struct wwdt_mspm_config wwdt_mspm_cfg_##index = {                             \
		.base = (struct wwdt_mspm_regs *)DT_INST_REG_ADDR(index),                          \
		.closed_window = DT_INST_PROP(index, closed_window),                               \
		.reset_action = COND_CODE_1(                                                       \
			DT_INST_PROP(index, ti_watchdog_reset_action),                             \
			(WDT_FLAG_RESET_SOC), (WDT_FLAG_RESET_CPU_CORE)),                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(index, 0)),                  \
		.clock_subsys = {                                                                  \
			.clk = DT_INST_CLOCKS_CELL_BY_IDX(index, 0, clk),                         \
		},                                                                                 \
		.irq_config_func = COND_CODE_1(WWDT_MSPM_HAS_IRQ(index),\
			(wwdt_mspm_##index##_irq_config), (NULL)),                           \
		.irq_num = COND_CODE_1(WWDT_MSPM_HAS_IRQ(index),                                  \
			(DT_INST_IRQN(index)), (-1)),                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, wwdt_mspm_init, NULL, &wwdt_mspm_data_##index,                \
			      &wwdt_mspm_cfg_##index, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wwdt_mspm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(WWDT_MSPM_INIT)
/* clang-format on */
