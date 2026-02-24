/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * PWM driver for TI CC23x0 Low Power General Purpose Timer (LGPT)
 *
 * This driver provides PWM output using the LGPT hardware:
 * - Tracks per-channel running state
 * - Uses different sequences for start vs modify operations
 * - Re-initializes timer state on start for thread-safety
 */

#define DT_DRV_COMPAT ti_cc23x0_lgpt_pwm

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/barrier.h>

#include <driverlib/gpio.h>
#include <driverlib/clkctl.h>
#include <inc/hw_lgpt.h>
#include <inc/hw_lgpt1.h>
#include <inc/hw_lgpt3.h>
#include <inc/hw_types.h>
#include <inc/hw_evtsvt.h>
#include <inc/hw_memmap.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME pwm_cc23x0_lgpt
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_PWM_LOG_LEVEL);

#define LGPT_CLK_PRESCALE(pres) ((pres) << 8)
#define LGPT_MAX_CHANNELS       3

/*
 * Per-channel state tracking for proper handling of start/stop/modify sequences.
 */
struct pwm_channel_state {
	uint32_t period;        /* Current period in cycles */
	uint32_t pulse;         /* Current pulse width in cycles */
	pwm_flags_t flags;      /* Current polarity flags */
	bool is_running;        /* True if channel is outputting PWM */
};

struct pwm_cc23x0_data {
	uint32_t prescale;
	uint32_t base_clk;
	struct k_spinlock lock;
	struct pwm_channel_state channels[LGPT_MAX_CHANNELS];
};

struct pwm_cc23x0_config {
	const uint32_t base; /* LGPT register base address */
	const struct pinctrl_dev_config *pcfg;
};

static inline void pwm_cc23x0_pm_policy_state_lock_get(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
}

static inline void pwm_cc23x0_pm_policy_state_lock_put(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
#endif
}

/*
 * Stop the timer completely by disabling the timer mode.
 */
static void pwm_cc23x0_stop_timer(const struct pwm_cc23x0_config *config)
{
	/* Disable timer by setting mode to disabled */
	HWREG(config->base + LGPT_O_CTL) = LGPT_CTL_MODE_DIS;

	/* Memory barrier to ensure stop is committed */
	barrier_dsync_fence_full();

	/* Wait for timer to fully stop */
	k_busy_wait(10);
}

/*
 * Start the timer in UP_PER (up-periodic) mode.
 */
static void pwm_cc23x0_start_timer(const struct pwm_cc23x0_config *config)
{
	/* Set timer mode to up-periodic */
	HWREG(config->base + LGPT_O_CTL) = LGPT_CTL_MODE_UP_PER;

	/* Trigger timer start */
	HWREG(config->base + LGPT_O_STARTCFG) = 0x1;

	/* Memory barrier to ensure start is committed */
	barrier_dsync_fence_full();
}

/*
 * Set initial counter target (used when starting timer).
 */
static void pwm_cc23x0_set_initial_target(const struct pwm_cc23x0_config *config,
					  uint32_t period)
{
	HWREG(config->base + LGPT_O_TGT) = period;
}

/*
 * Set initial channel compare value (used when starting timer).
 */
static void pwm_cc23x0_set_initial_compare(const struct pwm_cc23x0_config *config,
					   uint32_t channel, uint32_t pulse,
					   uint8_t capture_compare_action)
{
	switch (channel) {
	case 0:
		HWREG(config->base + LGPT_O_C0CC) = pulse;
		HWREG(config->base + LGPT_O_C0CFG) = 0x100 | capture_compare_action;
		break;
	case 1:
		HWREG(config->base + LGPT_O_C1CC) = pulse;
		HWREG(config->base + LGPT_O_C1CFG) = 0x200 | capture_compare_action;
		break;
	case 2:
		HWREG(config->base + LGPT_O_C2CC) = pulse;
		HWREG(config->base + LGPT_O_C2CFG) = 0x400 | capture_compare_action;
		break;
	}
}

/*
 * Set next counter target (used when modifying running timer).
 */
static void pwm_cc23x0_set_next_target(const struct pwm_cc23x0_config *config,
				       uint32_t period)
{
	/* PTGT register sets target for next cycle without stopping timer */
	HWREG(config->base + LGPT_O_PTGT) = period;
}

/*
 * Set next channel compare value (used when modifying running timer).
 */
static void pwm_cc23x0_set_next_compare(const struct pwm_cc23x0_config *config,
					uint32_t channel, uint32_t pulse)
{
	/* PCnCC registers set compare for next cycle without glitches */
	switch (channel) {
	case 0:
		HWREG(config->base + LGPT_O_PC0CC) = pulse;
		break;
	case 1:
		HWREG(config->base + LGPT_O_PC1CC) = pulse;
		break;
	case 2:
		HWREG(config->base + LGPT_O_PC2CC) = pulse;
		break;
	}
}

/*
 * Set channel output level directly (used when PWM is stopped).
 *
 * This uses the OUTCTL register to manually override the channel output.
 * OUTCTL register format (per channel, 2 bits):
 *   - CLROUTn: Write 1 to clear (set LOW) the output
 *   - SETOUTn: Write 1 to set (HIGH) the output
 *
 * Channel bit positions:
 *   - Channel 0: CLROUT0=bit0 (0x01), SETOUT0=bit1 (0x02)
 *   - Channel 1: CLROUT1=bit2 (0x04), SETOUT1=bit3 (0x08)
 *   - Channel 2: CLROUT2=bit4 (0x10), SETOUT2=bit5 (0x20)
 */
static void pwm_cc23x0_set_output_level(const struct pwm_cc23x0_config *config,
					uint32_t channel, bool level_high)
{
	uint32_t outctl_val;

	/*
	 * Each channel uses 2 bits: bit0=CLR (low), bit1=SET (high)
	 * Shift by (channel * 2) to get the correct bit position
	 */
	if (level_high) {
		/* SETOUT: bit pattern 0b10 = 2 */
		outctl_val = 0x02U << (channel * 2);
	} else {
		/* CLROUT: bit pattern 0b01 = 1 */
		outctl_val = 0x01U << (channel * 2);
	}

	HWREG(config->base + LGPT_O_OUTCTL) = outctl_val;
}

/*
 * Main PWM set function - handles start, stop, and modify operations
 */
static int pwm_cc23x0_set_cycles(const struct device *dev, uint32_t channel,
				 uint32_t period, uint32_t pulse, pwm_flags_t flags)
{
	const struct pwm_cc23x0_config *config = dev->config;
	struct pwm_cc23x0_data *data = dev->data;
	struct pwm_channel_state *ch_state;
	k_spinlock_key_t key;
	uint8_t capture_compare_action;
	bool was_running;
	int ret;

	LOG_DBG("set cycles ch=%u period=%u pulse=%u flags=0x%x",
		channel, period, pulse, flags);

	/* Validate channel */
	if (channel >= LGPT_MAX_CHANNELS) {
		LOG_ERR("Invalid channel ID: %u", channel);
		return -ENOTSUP;
	}

	/* Validate parameters based on timer width */
	if ((config->base != LGPT3_BASE) &&
	    (pulse > 0xffff || period > 0xffff || pulse > period)) {
		/* LGPT0, LGPT1, LGPT2 - 16bit counters */
		LOG_ERR("Period or pulse out of range for 16-bit timer");
		return -EINVAL;
	} else if (pulse > 0xffffff || period > 0xffffff || pulse > period) {
		/* LGPT3 - 24bit counter */
		LOG_ERR("Period or pulse out of range for 24-bit timer");
		return -EINVAL;
	}

	capture_compare_action = (flags & PWM_POLARITY_INVERTED) ? 0xB : 0xA;

	/* Acquire spinlock for thread-safe operation */
	key = k_spin_lock(&data->lock);

	ch_state = &data->channels[channel];
	was_running = ch_state->is_running;

	if (pulse == 0) {
		/*
		 * STOP/IDLE OPERATION
		 *
		 * When pulse=0, we need to:
		 * 1. Stop the timer (if running)
		 * 2. Apply pin configuration (so pin is PWM output, not floating)
		 * 3. Explicitly set the pin to idle level using OUTCTL register
		 *
		 * Idle level is determined by polarity:
		 *   - PWM_POLARITY_NORMAL: idle LOW
		 *   - PWM_POLARITY_INVERTED: idle HIGH
		 *
		 * Note: CC23x0 PWM pins typically use the negative/complementary
		 * timer outputs (e.g., T2_C0N instead of T2_C0). This means the
		 * pin output is the inverse of the timer's internal output.
		 * OUTCTL controls the internal output, so we must invert the
		 * value to get the correct idle level on the pin.
		 */
		bool idle_high = (flags & PWM_POLARITY_INVERTED) != 0;
		/* Invert for negative output pins (C0N, C1N, C2N) */
		bool outctl_level = !idle_high;

		/* Stop the timer */
		pwm_cc23x0_stop_timer(config);

		/* Apply pin configuration so the pin is driven (not floating) */
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Failed to apply pinctrl");
			k_spin_unlock(&data->lock, key);
			return ret;
		}

		/* Explicitly set pin to idle level using OUTCTL register
		 * Use inverted value to account for negative output pins
		 */
		pwm_cc23x0_set_output_level(config, channel, outctl_level);

		if (was_running) {
			ch_state->is_running = false;
			/* Release PM lock since we're stopping */
			pwm_cc23x0_pm_policy_state_lock_put();
		}

		/* Update state */
		ch_state->period = period;
		ch_state->pulse = 0;
		ch_state->flags = flags;

	} else if (!was_running) {
		/*
		 * START OPERATION (pulse > 0, was not running)
		 *
		 * Always re-initialize the timer state before starting.
		 * This ensures thread-safety - each start is a clean initialization.
		 */

		/* Acquire PM lock before starting */
		pwm_cc23x0_pm_policy_state_lock_get();

		/* Stop timer if it was running from another channel */
		pwm_cc23x0_stop_timer(config);

		/* Reset counter to 0 for clean start */
		HWREG(config->base + LGPT_O_CNTR) = 0;

		/* Re-apply pin configuration for thread-safety */
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Failed to apply pinctrl");
			k_spin_unlock(&data->lock, key);
			pwm_cc23x0_pm_policy_state_lock_put();
			return ret;
		}

		/* Set initial period (before starting) */
		pwm_cc23x0_set_initial_target(config, period);

		/* Set initial compare value (before starting) */
		pwm_cc23x0_set_initial_compare(config, channel, pulse,
					       capture_compare_action);

		/* Memory barrier before starting timer */
		barrier_dsync_fence_full();

		/* Start the timer */
		pwm_cc23x0_start_timer(config);

		/* Update state */
		ch_state->period = period;
		ch_state->pulse = pulse;
		ch_state->flags = flags;
		ch_state->is_running = true;

	} else {
		/*
		 * MODIFY OPERATION (pulse > 0, already running)
		 *
		 * Use pipeline registers (PTGT, PCxCC) to update without glitches.
		 */

		/* Update period for next cycle if changed */
		if (period != ch_state->period) {
			pwm_cc23x0_set_next_target(config, period);
		}

		/* Update pulse for next cycle if changed */
		if (pulse != ch_state->pulse) {
			pwm_cc23x0_set_next_compare(config, channel, pulse);
		}

		/* Update state */
		ch_state->period = period;
		ch_state->pulse = pulse;
		ch_state->flags = flags;
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int pwm_cc23x0_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					 uint64_t *cycles)
{
	struct pwm_cc23x0_data *data = dev->data;

	*cycles = data->base_clk / (data->prescale + 1);

	return 0;
}

static DEVICE_API(pwm, pwm_cc23x0_driver_api) = {
	.set_cycles = pwm_cc23x0_set_cycles,
	.get_cycles_per_sec = pwm_cc23x0_get_cycles_per_sec,
};

static int pwm_cc23x0_clock_action(const struct device *dev, bool activate)
{
	const struct pwm_cc23x0_config *config = dev->config;
	struct pwm_cc23x0_data *data = dev->data;
	uint32_t lgpt_clk_id = 0;

	switch (config->base) {
	case LGPT0_BASE:
		lgpt_clk_id = CLKCTL_LGPT0;
		break;
	case LGPT1_BASE:
		lgpt_clk_id = CLKCTL_LGPT1;
		break;
	case LGPT2_BASE:
		lgpt_clk_id = CLKCTL_LGPT2;
		break;
	case LGPT3_BASE:
		lgpt_clk_id = CLKCTL_LGPT3;
		break;
	default:
		return -EINVAL;
	}

	if (activate) {
		CLKCTLEnable(CLKCTL_BASE, lgpt_clk_id);
		HWREG(config->base + LGPT_O_PRECFG) = LGPT_CLK_PRESCALE(data->prescale);
		HWREG(EVTSVT_BASE + EVTSVT_O_LGPTSYNCSEL) = EVTSVT_LGPTSYNCSEL_PUBID_SYSTIM0;
	} else {
		CLKCTLDisable(CLKCTL_BASE, lgpt_clk_id);
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int pwm_cc23x0_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		pwm_cc23x0_clock_action(dev, false);
		return 0;
	case PM_DEVICE_ACTION_RESUME:
		pwm_cc23x0_clock_action(dev, true);
		return 0;
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_PM_DEVICE */

#define DT_TIMER(idx)           DT_INST_PARENT(idx)
#define DT_TIMER_BASE_ADDR(idx) (DT_REG_ADDR(DT_TIMER(idx)))

#define PWM_CC23X0_INIT_FUNC(idx)							\
	static int pwm_cc23x0_init##idx(const struct device *dev)			\
	{										\
		const struct pwm_cc23x0_config *config = dev->config;			\
											\
		int ret;								\
											\
		LOG_DBG("PWM cc23x0 base=[%x]", config->base);				\
											\
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);		\
		if (ret < 0) {								\
			LOG_ERR("[ERR] failed to setup PWM pinctrl");			\
			return ret;							\
		}									\
											\
		pwm_cc23x0_clock_action(dev, true);					\
											\
		return 0;								\
	}

#define PWM_DEVICE_INIT(idx)								\
	PM_DEVICE_DT_INST_DEFINE(idx, pwm_cc23x0_pm_action);				\
	PWM_CC23X0_INIT_FUNC(idx);							\
	PINCTRL_DT_INST_DEFINE(idx);							\
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_PWM_LOG_LEVEL);		\
											\
	static const struct pwm_cc23x0_config pwm_cc23x0_##idx##_config = {		\
		.base = DT_TIMER_BASE_ADDR(idx),					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),				\
		.lgpt_id = (DT_TIMER_BASE_ADDR(idx) - LGPT0_BASE) >> 12,		\
	};										\
											\
	static struct pwm_cc23x0_data pwm_cc23x0_##idx##_data = {			\
		.prescale = DT_PROP(DT_INST_PARENT(idx), clk_prescale),			\
		.base_clk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency),		\
	};										\
											\
	DEVICE_DT_INST_DEFINE(idx, pwm_cc23x0_init##idx, PM_DEVICE_DT_INST_GET(idx),	\
			      &pwm_cc23x0_##idx##_data, &pwm_cc23x0_##idx##_config,	\
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,			\
			      &pwm_cc23x0_driver_api)

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT);
