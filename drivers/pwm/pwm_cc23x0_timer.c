/*
 * Copyright (c) 2024 BayLibre, SAS
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * PWM driver for TI CC23x0 Low Power General Purpose Timer (LGPT)
 *
 * Provides PWM output and (optional) input capture using LGPT hardware.
 *   - Each instance represents one LGPT channel (C0/C1/C2).
 *   - Output uses CxCFG with the matching OUTx_EN bit.
 *   - Capture uses PER_PULSE_WIDTH_MEAS on the same channel; period and
 *     low-time are read from CxCCNC and PCxCCNC respectively.
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

#ifdef CONFIG_PWM_CAPTURE
#include <zephyr/irq.h>
#endif

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME pwm_cc23x0_lgpt
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_PWM_LOG_LEVEL);

#define LGPT_CLK_PRESCALE(pres) ((pres) << 8)
#define LGPT_MAX_CHANNELS       3

/* Output enable bits in CxCFG */
#define LGPT_CxCFG_OUT0         0x100
#define LGPT_CxCFG_OUT1         0x200
#define LGPT_CxCFG_OUT2         0x400

struct pwm_channel_state {
	uint32_t period;
	uint32_t pulse;
	pwm_flags_t flags;
	bool is_running;
};

struct pwm_cc23x0_data {
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

struct pwm_cc23x0_config {
	const uint32_t base;
	const struct pinctrl_dev_config *pcfg;
	uint8_t lgpt_id;
#ifdef CONFIG_PWM_CAPTURE
	uint32_t irq_num;
	uint32_t irq_priority;
	uint32_t max_top_value;
#endif
};

#ifdef CONFIG_PWM_CAPTURE
/* One slot per LGPT instance (0..3); tracks active capture device + channel. */
static struct {
	const struct device *dev;
	uint8_t channel;
} lgpt_capture_active[4];
#endif

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

static void pwm_cc23x0_stop_timer(const struct pwm_cc23x0_config *config)
{
	HWREG(config->base + LGPT_O_CTL) = LGPT_CTL_MODE_DIS;
	barrier_dsync_fence_full();
	k_busy_wait(10);
}

static void pwm_cc23x0_start_timer(const struct pwm_cc23x0_config *config)
{
	HWREG(config->base + LGPT_O_CTL) = LGPT_CTL_MODE_UP_PER;
	HWREG(config->base + LGPT_O_STARTCFG) = 0x1;
	barrier_dsync_fence_full();
}

static void pwm_cc23x0_set_initial_target(const struct pwm_cc23x0_config *config,
					  uint32_t period)
{
	HWREG(config->base + LGPT_O_TGT) = period;
}

static uint32_t pwm_cc23x0_cxcfg_offset(uint32_t channel)
{
	switch (channel) {
	case 0: return LGPT_O_C0CFG;
	case 1: return LGPT_O_C1CFG;
	case 2: return LGPT_O_C2CFG;
	default: return LGPT_O_C0CFG;
	}
}

static uint32_t pwm_cc23x0_cxcc_offset(uint32_t channel)
{
	switch (channel) {
	case 0: return LGPT_O_C0CC;
	case 1: return LGPT_O_C1CC;
	case 2: return LGPT_O_C2CC;
	default: return LGPT_O_C0CC;
	}
}

static uint32_t pwm_cc23x0_pcxcc_offset(uint32_t channel)
{
	switch (channel) {
	case 0: return LGPT_O_PC0CC;
	case 1: return LGPT_O_PC1CC;
	case 2: return LGPT_O_PC2CC;
	default: return LGPT_O_PC0CC;
	}
}

static uint32_t pwm_cc23x0_out_enable(uint32_t channel)
{
	switch (channel) {
	case 0: return LGPT_CxCFG_OUT0;
	case 1: return LGPT_CxCFG_OUT1;
	case 2: return LGPT_CxCFG_OUT2;
	default: return LGPT_CxCFG_OUT0;
	}
}

/*
 * Configure IOCTL so the complementary output (CxN) pin inverts OUTx.
 *
 * On CC23x0 the LGPT_O_IOCTL register has, per output, an OUTx field (bits
 * 1:0/5:4/9:8) and a COUTx field (bits 3:2/7:6/11:10). After reset both fields
 * are 0 (NRM) which makes the CxN pin track OUTx with the same phase, not
 * inverted - so a board that pinmuxes a CxN pin sees the wrong signal (or no
 * toggle at all, depending on initial state). Setting COUTx = INV (b11) makes
 * CxN truly complementary. The default pinctrl entries on this SoC all map
 * the negative pin (CxN), so we set COUTx_INV for the channel in use.
 */
static void pwm_cc23x0_enable_complementary_output(const struct pwm_cc23x0_config *config,
						   uint32_t channel)
{
	uint32_t mask, inv;

	switch (channel) {
	case 0:
		mask = LGPT_IOCTL_COUT0_M;
		inv  = LGPT_IOCTL_COUT0_INV;
		break;
	case 1:
		mask = LGPT_IOCTL_COUT1_M;
		inv  = LGPT_IOCTL_COUT1_INV;
		break;
	case 2:
		mask = LGPT_IOCTL_COUT2_M;
		inv  = LGPT_IOCTL_COUT2_INV;
		break;
	default:
		return;
	}

	uint32_t val = HWREG(config->base + LGPT_O_IOCTL);

	val = (val & ~mask) | inv;
	HWREG(config->base + LGPT_O_IOCTL) = val;
}

static void pwm_cc23x0_set_initial_compare(const struct pwm_cc23x0_config *config,
					   uint32_t channel, uint32_t pulse,
					   uint8_t capture_compare_action)
{
	HWREG(config->base + pwm_cc23x0_cxcc_offset(channel)) = pulse;
	HWREG(config->base + pwm_cc23x0_cxcfg_offset(channel)) =
		pwm_cc23x0_out_enable(channel) | capture_compare_action;
	pwm_cc23x0_enable_complementary_output(config, channel);
}

static void pwm_cc23x0_set_next_target(const struct pwm_cc23x0_config *config,
				       uint32_t period)
{
	HWREG(config->base + LGPT_O_PTGT) = period;
}

static void pwm_cc23x0_set_next_compare(const struct pwm_cc23x0_config *config,
					uint32_t channel, uint32_t pulse)
{
	HWREG(config->base + pwm_cc23x0_pcxcc_offset(channel)) = pulse;
}

/*
 * Override channel output level via OUTCTL while the timer is stopped.
 * CC23x0 PWM output pins are typically the complementary CxN signals;
 * caller supplies the OUTCTL bit polarity, not the pin polarity.
 */
static void pwm_cc23x0_set_output_level(const struct pwm_cc23x0_config *config,
					uint32_t channel, bool level_high)
{
	uint32_t outctl_val;

	if (level_high) {
		outctl_val = 0x02U << (channel * 2);
	} else {
		outctl_val = 0x01U << (channel * 2);
	}

	HWREG(config->base + LGPT_O_OUTCTL) = outctl_val;
}

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

	if (channel >= LGPT_MAX_CHANNELS) {
		LOG_ERR("Invalid channel ID: %u", channel);
		return -ENOTSUP;
	}

	if ((config->base != LGPT3_BASE) &&
	    (pulse > 0xffff || period > 0xffff || pulse > period)) {
		LOG_ERR("Period or pulse out of range for 16-bit timer");
		return -EINVAL;
	} else if (pulse > 0xffffff || period > 0xffffff || pulse > period) {
		LOG_ERR("Period or pulse out of range for 24-bit timer");
		return -EINVAL;
	}

	capture_compare_action = (flags & PWM_POLARITY_INVERTED) ? 0xB : 0xA;

	key = k_spin_lock(&data->lock);

	ch_state = &data->channels[channel];
	was_running = ch_state->is_running;

	if (pulse == 0) {
		/* Stop / idle on the configured channel pin. */
		bool idle_high = (flags & PWM_POLARITY_INVERTED) != 0;
		/* Default pin maps to CxN, so invert OUTCTL bit to drive the pad correctly. */
		bool outctl_level = !idle_high;

		pwm_cc23x0_stop_timer(config);

		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Failed to apply pinctrl");
			k_spin_unlock(&data->lock, key);
			return ret;
		}

		pwm_cc23x0_set_output_level(config, channel, outctl_level);

		if (was_running) {
			ch_state->is_running = false;
			pwm_cc23x0_pm_policy_state_lock_put();
		}

		ch_state->period = period;
		ch_state->pulse = 0;
		ch_state->flags = flags;

	} else if (!was_running) {
		pwm_cc23x0_pm_policy_state_lock_get();

		pwm_cc23x0_stop_timer(config);
		HWREG(config->base + LGPT_O_CNTR) = 0;

		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Failed to apply pinctrl");
			k_spin_unlock(&data->lock, key);
			pwm_cc23x0_pm_policy_state_lock_put();
			return ret;
		}

		pwm_cc23x0_set_initial_target(config, period);
		pwm_cc23x0_set_initial_compare(config, channel, pulse,
					       capture_compare_action);

		barrier_dsync_fence_full();

		pwm_cc23x0_start_timer(config);

		ch_state->period = period;
		ch_state->pulse = pulse;
		ch_state->flags = flags;
		ch_state->is_running = true;

	} else {
		if (period != ch_state->period) {
			pwm_cc23x0_set_next_target(config, period);
		}
		if (pulse != ch_state->pulse) {
			pwm_cc23x0_set_next_compare(config, channel, pulse);
		}
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

#ifdef CONFIG_PWM_CAPTURE

/* Per-channel capture-register and IRQ-mask helpers. */
static uint32_t pwm_cc23x0_cxccnc_offset(uint32_t channel)
{
	switch (channel) {
	case 0: return LGPT_O_C0CCNC;
	case 1: return LGPT_O_C1CCNC;
	case 2: return LGPT_O_C2CCNC;
	default: return LGPT_O_C0CCNC;
	}
}

static uint32_t pwm_cc23x0_pcxccnc_offset(uint32_t channel)
{
	switch (channel) {
	case 0: return LGPT_O_PC0CCNC;
	case 1: return LGPT_O_PC1CCNC;
	case 2: return LGPT_O_PC2CCNC;
	default: return LGPT_O_PC0CCNC;
	}
}

static uint32_t pwm_cc23x0_cxcc_irq_mask(uint32_t channel)
{
	switch (channel) {
	case 0: return LGPT_RIS_C0CC;
	case 1: return LGPT_RIS_C1CC;
	case 2: return LGPT_RIS_C2CC;
	default: return LGPT_RIS_C0CC;
	}
}

static uint32_t pwm_cc23x0_cxcfg_per_pulse_meas(uint32_t channel)
{
	switch (channel) {
	case 0: return LGPT_C0CFG_CCACT_PER_PULSE_WIDTH_MEAS;
	case 1: return LGPT_C1CFG_CCACT_PER_PULSE_WIDTH_MEAS;
	case 2: return LGPT_C2CFG_CCACT_PER_PULSE_WIDTH_MEAS;
	default: return LGPT_C0CFG_CCACT_PER_PULSE_WIDTH_MEAS;
	}
}

static uint32_t pwm_cc23x0_cxcfg_input_io(uint32_t channel)
{
	switch (channel) {
	case 0: return LGPT_C0CFG_INPUT_IO;
	case 1: return LGPT_C1CFG_INPUT_IO;
	case 2: return LGPT_C2CFG_INPUT_IO;
	default: return LGPT_C0CFG_INPUT_IO;
	}
}

static uint32_t pwm_cc23x0_cxcfg_edge(uint32_t channel, bool falling)
{
	switch (channel) {
	case 0:
		return falling ? LGPT_C0CFG_EDGE_FALL : LGPT_C0CFG_EDGE_RISE;
	case 1:
		return falling ? LGPT_C1CFG_EDGE_FALL : LGPT_C1CFG_EDGE_RISE;
	case 2:
		return falling ? LGPT_C2CFG_EDGE_FALL : LGPT_C2CFG_EDGE_RISE;
	default:
		return falling ? LGPT_C0CFG_EDGE_FALL : LGPT_C0CFG_EDGE_RISE;
	}
}

static void pwm_cc23x0_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct pwm_cc23x0_config *cfg = dev->config;
	struct pwm_cc23x0_data *data = dev->data;
	uint32_t base = cfg->base;
	uint8_t channel = lgpt_capture_active[cfg->lgpt_id].channel;
	uint32_t cxcc_mask = pwm_cc23x0_cxcc_irq_mask(channel);

	uint32_t mis = HWREG(base + LGPT_O_MIS);

	HWREG(base + LGPT_O_ICLR) = mis;
	barrier_dsync_fence_full();

	if (mis & cxcc_mask) {
		uint32_t period   = HWREG(base + pwm_cc23x0_cxccnc_offset(channel));
		uint32_t low_time = HWREG(base + pwm_cc23x0_pcxccnc_offset(channel));
		uint32_t pulse;

		/* Discard first capture (partial cycle). */
		if (data->capture.first_capture) {
			data->capture.first_capture = false;
			return;
		}

		if (data->capture.flags & PWM_POLARITY_INVERTED) {
			pulse = low_time;
		} else {
			pulse = (period > low_time) ? (period - low_time) : 0;
		}

		if (!data->capture.is_continuous) {
			HWREG(base + LGPT_O_IMCLR) = cxcc_mask;
			HWREG(base + LGPT_O_CTL) = LGPT_CTL_MODE_DIS;
			barrier_dsync_fence_full();
			data->capture.is_capturing = false;
			lgpt_capture_active[cfg->lgpt_id].dev = NULL;
			irq_disable(cfg->irq_num);
		}
		data->capture.callback(dev, channel, period, pulse, 0,
				       data->capture.user_data);
	}
}

static int pwm_cc23x0_configure_capture(const struct device *dev, uint32_t channel,
					pwm_flags_t flags,
					pwm_capture_callback_handler_t cb,
					void *user_data)
{
	const struct pwm_cc23x0_config *cfg = dev->config;
	struct pwm_cc23x0_data *data = dev->data;
	uint32_t ccfg;
	bool falling;

	if (channel >= LGPT_MAX_CHANNELS) {
		return -EINVAL;
	}
	if (!(flags & (PWM_CAPTURE_TYPE_PERIOD | PWM_CAPTURE_TYPE_PULSE))) {
		return -EINVAL;
	}
	if (data->capture.is_capturing) {
		return -EBUSY;
	}

	pwm_cc23x0_stop_timer(cfg);

	falling = (flags & PWM_POLARITY_INVERTED) != 0;
	/* SimpleLink LGPTimerLPF3 reference always sets OUTx_EN even for capture. */
	ccfg = pwm_cc23x0_cxcfg_per_pulse_meas(channel)
	     | pwm_cc23x0_cxcfg_input_io(channel)
	     | pwm_cc23x0_out_enable(channel)
	     | pwm_cc23x0_cxcfg_edge(channel, falling);

	HWREG(cfg->base + pwm_cc23x0_cxcfg_offset(channel)) = ccfg;
	HWREG(cfg->base + LGPT_O_TGT) = cfg->max_top_value;
	barrier_dsync_fence_full();

	data->capture.callback      = cb;
	data->capture.user_data     = user_data;
	data->capture.flags         = flags;
	data->capture.is_continuous = !!(flags & PWM_CAPTURE_MODE_CONTINUOUS);

	LOG_DBG("configure_capture ch=%u ccfg=0x%x top=0x%x",
		channel, ccfg, cfg->max_top_value);

	return 0;
}

static int pwm_cc23x0_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_cc23x0_config *cfg = dev->config;
	struct pwm_cc23x0_data *data = dev->data;
	uint32_t cxcc_mask;

	if (channel >= LGPT_MAX_CHANNELS) {
		return -EINVAL;
	}
	if (data->capture.callback == NULL) {
		return -EINVAL;
	}
	if (data->capture.is_capturing) {
		return -EBUSY;
	}
	if (lgpt_capture_active[cfg->lgpt_id].dev != NULL &&
	    lgpt_capture_active[cfg->lgpt_id].dev != dev) {
		return -EBUSY;
	}

	cxcc_mask = pwm_cc23x0_cxcc_irq_mask(channel);

	lgpt_capture_active[cfg->lgpt_id].dev = dev;
	lgpt_capture_active[cfg->lgpt_id].channel = channel;
	irq_connect_dynamic(cfg->irq_num, cfg->irq_priority,
			    pwm_cc23x0_isr, dev, 0);

	HWREG(cfg->base + LGPT_O_ICLR)  = cxcc_mask | LGPT_RIS_TGT;
	HWREG(cfg->base + LGPT_O_IMSET) = cxcc_mask;
	barrier_dsync_fence_full();

	data->capture.is_capturing = true;
	data->capture.first_capture = true;
	pwm_cc23x0_start_timer(cfg);
	irq_enable(cfg->irq_num);

	LOG_DBG("enable_capture lgpt_id=%d ch=%u irq=%d",
		cfg->lgpt_id, channel, cfg->irq_num);

	return 0;
}

static int pwm_cc23x0_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_cc23x0_config *cfg = dev->config;
	struct pwm_cc23x0_data *data = dev->data;
	uint32_t cxcc_mask;

	if (channel >= LGPT_MAX_CHANNELS) {
		return -EINVAL;
	}

	cxcc_mask = pwm_cc23x0_cxcc_irq_mask(channel);

	irq_disable(cfg->irq_num);
	HWREG(cfg->base + LGPT_O_IMCLR) = cxcc_mask;
	pwm_cc23x0_stop_timer(cfg);
	HWREG(cfg->base + LGPT_O_ICLR)  = cxcc_mask | LGPT_RIS_TGT;
	barrier_dsync_fence_full();

	data->capture.is_capturing = false;
	lgpt_capture_active[cfg->lgpt_id].dev = NULL;

	LOG_DBG("disable_capture lgpt_id=%d ch=%u", cfg->lgpt_id, channel);

	return 0;
}

#endif /* CONFIG_PWM_CAPTURE */

static const struct pwm_driver_api pwm_cc23x0_driver_api = {
	.set_cycles = pwm_cc23x0_set_cycles,
	.get_cycles_per_sec = pwm_cc23x0_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_cc23x0_configure_capture,
	.enable_capture    = pwm_cc23x0_enable_capture,
	.disable_capture   = pwm_cc23x0_disable_capture,
#endif
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
		IF_ENABLED(CONFIG_PWM_CAPTURE, (					\
		.irq_num      = DT_IRQN(DT_INST_PARENT(idx)),				\
		.irq_priority = DT_IRQ(DT_INST_PARENT(idx), priority),			\
		.max_top_value = DT_PROP(DT_INST_PARENT(idx), max_top_value),		\
		))									\
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
