/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * PWM driver for TI CC27XX Low Power General Purpose Timer (LGPT)
 *
 * Each PWM device instance represents one output channel of an LGPT timer.
 * The output index (0, 1, or 2) is determined from the device tree node name.
 */

#define DT_DRV_COMPAT ti_cc27xx_lgpt_pwm

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

#ifdef CONFIG_PWM_CAPTURE
#include <zephyr/irq.h>
#endif

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME pwm_cc27xx_timer_lgpt
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_PWM_LOG_LEVEL);

#define LGPT_CLK_PRESCALE(pres) ((pres) << 8)
#define LGPT_MAX_CHANNELS       3

/* Output enable bits for CxCFG registers */
#define LGPT_CxCFG_OUT0 0x100
#define LGPT_CxCFG_OUT1 0x200
#define LGPT_CxCFG_OUT2 0x400

/*
 * Per-channel state tracking for proper handling of start/stop/modify sequences.
 */
struct pwm_channel_state {
	uint32_t period;
	uint32_t pulse;
	pwm_flags_t flags;
	bool is_running;
};

struct pwm_cc27xx_timer_data {
	uint32_t prescale;
	uint32_t base_clk;
	struct k_spinlock lock;
	struct pwm_channel_state channels[LGPT_MAX_CHANNELS];
#ifdef CONFIG_PWM_CAPTURE
	struct {
		pwm_capture_callback_handler_t callback;
		void *user_data;
		pwm_flags_t flags;
		bool is_capturing;
		bool is_continuous;
		bool first_capture;
	} capture;
#endif
};

struct pwm_cc27xx_timer_config {
	const uint32_t base;
	const struct pinctrl_dev_config *pcfg;
	uint8_t lgpt_id;
	uint8_t output_idx; /* Which output (0, 1, or 2) this PWM controls */
#ifdef CONFIG_PWM_CAPTURE
	uint32_t irq_num;
	uint32_t irq_priority;
	uint32_t max_top_value;
#endif
};

#ifdef CONFIG_PWM_CAPTURE
/* One slot per LGPT instance (0-3); tracks which device is currently capturing */
static const struct device *lgpt_capture_dev[4];
#endif

static inline void pwm_cc27xx_timer_pm_policy_state_lock_get(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
}

static inline void pwm_cc27xx_timer_pm_policy_state_lock_put(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
#endif
}

static void pwm_cc27xx_timer_stop(const struct pwm_cc27xx_timer_config *config)
{
	HWREG(config->base + LGPT_O_CTL) = LGPT_CTL_MODE_DIS;
	barrier_dsync_fence_full();
	k_busy_wait(10);
}

static void pwm_cc27xx_timer_start(const struct pwm_cc27xx_timer_config *config)
{
	HWREG(config->base + LGPT_O_CTL) = LGPT_CTL_MODE_UP_PER;
	HWREG(config->base + LGPT_O_STARTCFG) = 0x1;
	barrier_dsync_fence_full();
}

static void pwm_cc27xx_timer_set_initial_target(const struct pwm_cc27xx_timer_config *config,
						uint32_t period)
{
	HWREG(config->base + LGPT_O_TGT) = period;
}

/*
 * Get the output enable bit for the given output index
 */
static uint32_t pwm_cc27xx_timer_get_output_enable(uint8_t output_idx)
{
	switch (output_idx) {
	case 0:
		return LGPT_CxCFG_OUT0;
	case 1:
		return LGPT_CxCFG_OUT1;
	case 2:
		return LGPT_CxCFG_OUT2;
	default:
		return LGPT_CxCFG_OUT0;
	}
}

/*
 * Set initial channel compare value.
 * Uses channel 0 registers but enables the correct output based on output_idx.
 */
static void pwm_cc27xx_timer_set_initial_compare(const struct pwm_cc27xx_timer_config *config,
						 uint32_t pulse, uint8_t capture_compare_action)
{
	uint32_t output_enable = pwm_cc27xx_timer_get_output_enable(config->output_idx);

	/* Always use channel 0 registers (C0CC, C0CFG) but enable correct output */
	HWREG(config->base + LGPT_O_C0CC) = pulse;
	HWREG(config->base + LGPT_O_C0CFG) = output_enable | capture_compare_action;

	LOG_DBG("C0CFG = 0x%x (output_idx=%d, ccact=0x%x)", output_enable | capture_compare_action,
		config->output_idx, capture_compare_action);
}

static void pwm_cc27xx_timer_set_next_target(const struct pwm_cc27xx_timer_config *config,
					     uint32_t period)
{
	HWREG(config->base + LGPT_O_PTGT) = period;
}

static void pwm_cc27xx_timer_set_next_compare(const struct pwm_cc27xx_timer_config *config,
					      uint32_t pulse)
{
	/* Always use channel 0 pipeline register */
	HWREG(config->base + LGPT_O_PC0CC) = pulse;
}

/*
 * Set channel output level directly (used when PWM is stopped)
 */
static void pwm_cc27xx_timer_set_output_level(const struct pwm_cc27xx_timer_config *config,
					      bool level_high)
{
	uint32_t outctl_val;

	/*
	 * OUTCTL register: each output uses 2 bits: bit0=CLR (low), bit1=SET (high)
	 * Shift by (output_idx * 2) to get the correct bit position
	 */
	if (level_high) {
		outctl_val = 0x02U << (config->output_idx * 2);
	} else {
		outctl_val = 0x01U << (config->output_idx * 2);
	}

	HWREG(config->base + LGPT_O_OUTCTL) = outctl_val;
}

static int pwm_cc27xx_timer_set_cycles(const struct device *dev, uint32_t channel, uint32_t period,
				       uint32_t pulse, pwm_flags_t flags)
{
	const struct pwm_cc27xx_timer_config *config = dev->config;
	struct pwm_cc27xx_timer_data *data = dev->data;
	struct pwm_channel_state *ch_state;
	k_spinlock_key_t key;
	uint8_t capture_compare_action;
	bool was_running;
	int ret;

	LOG_DBG("set cycles output=%u period=%u pulse=%u flags=0x%x", config->output_idx, period,
		pulse, flags);

	/*
	 * Note: The 'channel' parameter from pwms is ignored.
	 * Each PWM device instance controls one specific output determined by output_idx.
	 */
	(void)channel;

	/* Validate parameters based on timer width */
	if ((config->base != LGPT3_BASE) && (pulse > 0xffff || period > 0xffff || pulse > period)) {
		LOG_ERR("Period or pulse out of range for 16-bit timer");
		return -EINVAL;
	} else if (pulse > 0xffffff || period > 0xffffff || pulse > period) {
		LOG_ERR("Period or pulse out of range for 24-bit timer");
		return -EINVAL;
	}

	/*
	 * Capture/Compare Action values:
	 * 0x0B = SET_ON_0_TGL_ON_CMP (output HIGH at counter=0, toggle at compare)
	 * 0x0A = CLR_ON_0_TGL_ON_CMP (output LOW at counter=0, toggle at compare)
	 *
	 * PWM_POLARITY_NORMAL: Active HIGH → use 0x0B (starts HIGH)
	 * PWM_POLARITY_INVERTED: Active LOW → use 0x0A (starts LOW)
	 */
	capture_compare_action = (flags & PWM_POLARITY_INVERTED) ? 0xA : 0xB;

	key = k_spin_lock(&data->lock);

	/* Use output_idx to index into channels array */
	ch_state = &data->channels[config->output_idx];
	was_running = ch_state->is_running;

	if (pulse == 0) {
		/*
		 * STOP/IDLE OPERATION
		 *
		 * PWM_POLARITY_NORMAL: Active HIGH, Idle LOW (idle_high = false)
		 * PWM_POLARITY_INVERTED: Active LOW, Idle HIGH (idle_high = true)
		 *
		 * For positive output pins (Cx), OUTCTL directly controls the pin level.
		 * For negative output pins (CxN), hardware inverts the OUTCTL output.
		 * Since we use positive pins (C1, C2), no software inversion is needed.
		 */
		bool idle_high = (flags & PWM_POLARITY_INVERTED) != 0;

		pwm_cc27xx_timer_stop(config);

		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Failed to apply pinctrl");
			k_spin_unlock(&data->lock, key);
			return ret;
		}

		pwm_cc27xx_timer_set_output_level(config, idle_high);

		if (was_running) {
			ch_state->is_running = false;
			pwm_cc27xx_timer_pm_policy_state_lock_put();
		}

		ch_state->period = period;
		ch_state->pulse = 0;
		ch_state->flags = flags;

	} else if (!was_running) {
		/*
		 * START OPERATION
		 */
		pwm_cc27xx_timer_pm_policy_state_lock_get();

		pwm_cc27xx_timer_stop(config);

		/* Reset counter */
		HWREG(config->base + LGPT_O_CNTR) = 0;

		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Failed to apply pinctrl");
			k_spin_unlock(&data->lock, key);
			pwm_cc27xx_timer_pm_policy_state_lock_put();
			return ret;
		}

		pwm_cc27xx_timer_set_initial_target(config, period);
		pwm_cc27xx_timer_set_initial_compare(config, pulse, capture_compare_action);

		barrier_dsync_fence_full();

		pwm_cc27xx_timer_start(config);

		ch_state->period = period;
		ch_state->pulse = pulse;
		ch_state->flags = flags;
		ch_state->is_running = true;

	} else {
		/*
		 * MODIFY OPERATION (while running)
		 */
		if (period != ch_state->period) {
			pwm_cc27xx_timer_set_next_target(config, period);
		}

		if (pulse != ch_state->pulse) {
			pwm_cc27xx_timer_set_next_compare(config, pulse);
		}

		ch_state->period = period;
		ch_state->pulse = pulse;
		ch_state->flags = flags;
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int pwm_cc27xx_timer_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					       uint64_t *cycles)
{
	struct pwm_cc27xx_timer_data *data = dev->data;

	/*
	 * On CC27XX, the LGPT reference clock is half of SVTCLK.
	 */
	uint32_t lgpt_base_clk = data->base_clk / 2;

	*cycles = lgpt_base_clk / (data->prescale + 1);

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE

/*
 * ISR registered dynamically via irq_connect_dynamic() when capture is enabled.
 * The driver always uses the channel-0 hardware registers (C0CCNC / PC0CCNC)
 * regardless of output_idx; output_idx only selects which OUTx_EN bit is set.
 */
static void pwm_cc27xx_timer_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct pwm_cc27xx_timer_config *cfg = dev->config;
	struct pwm_cc27xx_timer_data *data = dev->data;
	uint32_t base = cfg->base;

	uint32_t mis = HWREG(base + LGPT_O_MIS);

	HWREG(base + LGPT_O_ICLR) = mis;
	barrier_dsync_fence_full();

	/*
	 * Only react to capture (C0CC). TGT wraps are intentionally not
	 * unmasked — when no input edge arrives, the OS-level timeout in
	 * pwm_capture_cycles() returns -EAGAIN. Firing -ERANGE here would
	 * race that timeout and break test_capture_timeout.
	 */
	if (mis & LGPT_RIS_C0CC) {
		uint32_t period = HWREG(base + LGPT_O_C0CCNC);
		uint32_t low_time = HWREG(base + LGPT_O_PC0CCNC);
		uint32_t pulse;

		/*
		 * The first capture event after enable can be a partial cycle
		 * (timer started mid-period). Discard it and wait for the next
		 * complete cycle. Apply to both single and continuous modes so
		 * callers never see a transient value.
		 */
		if (data->capture.first_capture) {
			data->capture.first_capture = false;
			return;
		}

		/*
		 * Empirically, the LGPT in PER_PULSE_WIDTH_MEAS mode reports:
		 *   C0CCNC  = full period
		 *   PC0CCNC = duration of the LOW phase of the physical input
		 * Zephyr's pulse_cycles is the duration of the *active* state:
		 *   PWM_POLARITY_NORMAL   → active = HIGH → pulse = period - low
		 *   PWM_POLARITY_INVERTED → active = LOW  → pulse = low
		 */
		if (data->capture.flags & PWM_POLARITY_INVERTED) {
			pulse = low_time;
		} else {
			pulse = (period > low_time) ? (period - low_time) : 0;
		}

		if (!data->capture.is_continuous) {
			HWREG(base + LGPT_O_IMCLR) = LGPT_RIS_C0CC;
			HWREG(base + LGPT_O_CTL) = LGPT_CTL_MODE_DIS;
			barrier_dsync_fence_full();
			data->capture.is_capturing = false;
			lgpt_capture_dev[cfg->lgpt_id] = NULL;
			irq_disable(cfg->irq_num);
		}
		data->capture.callback(dev, 0, period, pulse, 0, data->capture.user_data);
	}
}

static int pwm_cc27xx_timer_configure_capture(const struct device *dev, uint32_t channel,
					      pwm_flags_t flags, pwm_capture_callback_handler_t cb,
					      void *user_data)
{
	const struct pwm_cc27xx_timer_config *cfg = dev->config;
	struct pwm_cc27xx_timer_data *data = dev->data;
	uint32_t ccfg;

	ARG_UNUSED(channel);

	if (!(flags & (PWM_CAPTURE_TYPE_PERIOD | PWM_CAPTURE_TYPE_PULSE))) {
		return -EINVAL;
	}
	if (data->capture.is_capturing) {
		return -EBUSY;
	}

	/* C0CFG must be written while CTL.MODE = DIS per TRM */
	pwm_cc27xx_timer_stop(cfg);

	ccfg = LGPT_C0CFG_CCACT_PER_PULSE_WIDTH_MEAS | LGPT_C0CFG_INPUT_IO |
	       pwm_cc27xx_timer_get_output_enable(cfg->output_idx);
	ccfg |= (flags & PWM_POLARITY_INVERTED) ? LGPT_C0CFG_EDGE_FALL : LGPT_C0CFG_EDGE_RISE;

	HWREG(cfg->base + LGPT_O_C0CFG) = ccfg;
	HWREG(cfg->base + LGPT_O_TGT) = cfg->max_top_value;
	barrier_dsync_fence_full();

	data->capture.callback = cb;
	data->capture.user_data = user_data;
	data->capture.flags = flags;
	data->capture.is_continuous = !!(flags & PWM_CAPTURE_MODE_CONTINUOUS);

	LOG_DBG("configure_capture output_idx=%d ccfg=0x%x top=0x%x", cfg->output_idx, ccfg,
		cfg->max_top_value);

	return 0;
}

static int pwm_cc27xx_timer_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_cc27xx_timer_config *cfg = dev->config;
	struct pwm_cc27xx_timer_data *data = dev->data;

	ARG_UNUSED(channel);

	if (data->capture.callback == NULL) {
		return -EINVAL;
	}
	if (data->capture.is_capturing) {
		return -EBUSY;
	}
	if (lgpt_capture_dev[cfg->lgpt_id] != NULL && lgpt_capture_dev[cfg->lgpt_id] != dev) {
		return -EBUSY;
	}

	lgpt_capture_dev[cfg->lgpt_id] = dev;
	irq_connect_dynamic(cfg->irq_num, cfg->irq_priority, pwm_cc27xx_timer_isr, dev, 0);

	HWREG(cfg->base + LGPT_O_ICLR) = LGPT_RIS_C0CC | LGPT_RIS_TGT;
	HWREG(cfg->base + LGPT_O_IMSET) = LGPT_RIS_C0CC;
	barrier_dsync_fence_full();

	data->capture.is_capturing = true;
	data->capture.first_capture = true;
	pwm_cc27xx_timer_start(cfg);
	irq_enable(cfg->irq_num);

	LOG_DBG("enable_capture lgpt_id=%d irq=%d", cfg->lgpt_id, cfg->irq_num);

	return 0;
}

static int pwm_cc27xx_timer_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_cc27xx_timer_config *cfg = dev->config;
	struct pwm_cc27xx_timer_data *data = dev->data;

	ARG_UNUSED(channel);

	irq_disable(cfg->irq_num);
	HWREG(cfg->base + LGPT_O_IMCLR) = LGPT_RIS_C0CC;
	pwm_cc27xx_timer_stop(cfg);
	HWREG(cfg->base + LGPT_O_ICLR) = LGPT_RIS_C0CC | LGPT_RIS_TGT;
	barrier_dsync_fence_full();

	data->capture.is_capturing = false;
	lgpt_capture_dev[cfg->lgpt_id] = NULL;

	LOG_DBG("disable_capture lgpt_id=%d", cfg->lgpt_id);

	return 0;
}

#endif /* CONFIG_PWM_CAPTURE */

static DEVICE_API(pwm, pwm_cc27xx_timer_driver_api) = {
	.set_cycles = pwm_cc27xx_timer_set_cycles,
	.get_cycles_per_sec = pwm_cc27xx_timer_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_cc27xx_timer_configure_capture,
	.enable_capture = pwm_cc27xx_timer_enable_capture,
	.disable_capture = pwm_cc27xx_timer_disable_capture,
#endif
};

static int pwm_cc27xx_timer_clock_action(const struct device *dev, bool activate)
{
	const struct pwm_cc27xx_timer_config *config = dev->config;
	struct pwm_cc27xx_timer_data *data = dev->data;
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

static int pwm_cc27xx_timer_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		pwm_cc27xx_timer_clock_action(dev, false);
		return 0;
	case PM_DEVICE_ACTION_RESUME:
		pwm_cc27xx_timer_clock_action(dev, true);
		return 0;
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_PM_DEVICE */

/*
 * Get output index from the ti,output-index device tree property.
 */
#define DT_TIMER(idx)           DT_INST_PARENT(idx)
#define DT_TIMER_BASE_ADDR(idx) (DT_REG_ADDR(DT_TIMER(idx)))

/* Get output index from ti,output-index property, default to 0 */
#define PWM_OUTPUT_IDX(idx) DT_INST_PROP_OR(idx, ti_output_index, 0)

#define PWM_CC27XX_TIMER_INIT_FUNC(idx)                                                            \
	static int pwm_cc27xx_timer_init##idx(const struct device *dev)                            \
	{                                                                                          \
		const struct pwm_cc27xx_timer_config *config = dev->config;                        \
		int ret;                                                                           \
                                                                                                   \
		LOG_DBG("PWM cc27xx_timer base=[%x] output_idx=%d", config->base,                  \
			config->output_idx);                                                       \
                                                                                                   \
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);                    \
		if (ret < 0) {                                                                     \
			LOG_ERR("[ERR] failed to setup PWM pinctrl");                              \
			return ret;                                                                \
		}                                                                                  \
                                                                                                   \
		pwm_cc27xx_timer_clock_action(dev, true);                                          \
                                                                                                   \
		return 0;                                                                          \
	}

#define PWM_DEVICE_INIT(idx)                                                                       \
	PM_DEVICE_DT_INST_DEFINE(idx, pwm_cc27xx_timer_pm_action);                                 \
	PWM_CC27XX_TIMER_INIT_FUNC(idx);                                                           \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_PWM_LOG_LEVEL);                         \
                                                                                                   \
	static const struct pwm_cc27xx_timer_config pwm_cc27xx_timer_##idx##_config = {            \
		.base = DT_TIMER_BASE_ADDR(idx),                                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.lgpt_id = (DT_TIMER_BASE_ADDR(idx) - LGPT0_BASE) >> 12,                           \
		.output_idx = PWM_OUTPUT_IDX(idx),                                                 \
		IF_ENABLED(CONFIG_PWM_CAPTURE, (					\
		.irq_num      = DT_IRQN(DT_INST_PARENT(idx)),				\
		.irq_priority = DT_IRQ(DT_INST_PARENT(idx), priority),			\
		.max_top_value = DT_PROP(DT_INST_PARENT(idx), max_top_value),		\
		)) };      \
                                                                                                   \
	static struct pwm_cc27xx_timer_data pwm_cc27xx_timer_##idx##_data = {                      \
		.prescale = DT_PROP(DT_INST_PARENT(idx), clk_prescale),                            \
		.base_clk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency),                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, pwm_cc27xx_timer_init##idx, PM_DEVICE_DT_INST_GET(idx),         \
			      &pwm_cc27xx_timer_##idx##_data, &pwm_cc27xx_timer_##idx##_config,    \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &pwm_cc27xx_timer_driver_api)

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT);
