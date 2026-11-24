/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2023 Nobleo Technology
 * Copyright (c) 2025 CodeWrights GmbH
 * Copyright (c) 2026 Synsquare S.r.l.
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * PWM on the low-power timers (LPTIM) of STM32 series with the LPTIM capture/compare channels.
 *
 * In PWM mode an LPTIM output is set once the counter exceeds the compare value and reset when
 * the counter matches the autoreload value. With ARR = period - 1 and CCR = period - pulse - 1
 * the output is therefore high for the last pulse cycles of the period, and the inverted output
 * polarity gives the inverted waveform. The pulse is at the end of the period so that the output
 * is at its inactive level while the counter does not run and before the first compare match.
 * The first pulse after a start then has the requested length.
 *
 * 0 % and 100 % duty cycle cannot be set through the compare alone. CCR = 0 still leaves one
 * inactive cycle, and no pulse at all needs a compare above the autoreload. Such a compare
 * disturbs the waveform of the other channel, and channel 1 held high by it drops low for one
 * cycle at regular intervals. The driver produces constant levels as follows.
 * - On instances with capture/compare channels, the counter is stopped while neither channel
 *   generates a waveform. An enabled timer whose counter does not run holds each output at the
 *   level of its polarity. While the other channel generates a waveform, a constant low level
 *   becomes 1 / period and a constant high level (period - 1) / period. A disabled channel
 *   output (CCxE = 0) is not driven, so the outputs stay enabled.
 * - Instances with a single output and no CCMR1 register (LPTIM4 on STM32U5, for instance) idle
 *   at the level of their waveform polarity while disabled, so the timer is stopped.
 *
 * A compare change takes effect at the end of the current period. A channel held low that starts
 * a waveform while the counter runs therefore shows a one-cycle pulse at the end of the current
 * period, before its first full pulse.
 */

#define DT_DRV_COMPAT st_stm32_lptim_pwm

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_lptim.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(pwm_stm32_lptim, CONFIG_PWM_LOG_LEVEL);

#if !defined(LPTIM_CCMR1_CC1E)
#error "The STM32 LPTIM PWM driver needs the LPTIM capture/compare channels"
#endif

/* Maximum number of timer channels. */
#define TIMER_MAX_CH 2U

/* Largest period, given by the largest autoreload. */
#define MAX_PERIOD_CYCLES (LPTIM_ARR_ARR + 1U)

enum channel_state {
	/* Output held low for the whole period. */
	CHANNEL_LOW = 0,
	/* Output held high for the whole period. */
	CHANNEL_HIGH,
	/* Output generates a waveform. */
	CHANNEL_WAVE,
};

struct pwm_stm32_lptim_config {
	LPTIM_TypeDef *timer;
	uint32_t prescaler;
	const struct stm32_pclken *pclken;
	struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pcfg;
};

struct pwm_stm32_lptim_data {
	/* Timer kernel clock (Hz). */
	uint32_t tim_clk;
	/* State of each channel. The state of a missing channel stays CHANNEL_LOW. */
	enum channel_state state[TIMER_MAX_CH];
	/* Whether the counter of an instance with channels runs. */
	bool counting;
	/* Serializes the calls, since a call can sleep while it waits for a register write. */
	struct k_sem lock;
};

/* Channel to LL mapping. */
static const uint32_t ch2ll[TIMER_MAX_CH] = {
	LL_LPTIM_CHANNEL_CH1,
	LL_LPTIM_CHANNEL_CH2,
};

/* Channel to compare set function mapping. */
static void (*const set_timer_compare[TIMER_MAX_CH])(LPTIM_TypeDef *, uint32_t) = {
	LL_LPTIM_OC_SetCompareCH1,
	LL_LPTIM_OC_SetCompareCH2,
};

/* Channel to compare get function mapping. */
static uint32_t (*const get_timer_compare[TIMER_MAX_CH])(const LPTIM_TypeDef *) = {
	LL_LPTIM_OC_GetCompareCH1,
	LL_LPTIM_OC_GetCompareCH2,
};

/* Channel to compare write completion flag mapping. */
static const uint32_t ch2cmpok[TIMER_MAX_CH] = {
	LPTIM_ISR_CMP1OK,
	LPTIM_ISR_CMP2OK,
};

/* Channel to compare write completion clear flag mapping. */
static const uint32_t ch2cmpokcf[TIMER_MAX_CH] = {
	LPTIM_ICR_CMP1OKCF,
	LPTIM_ICR_CMP2OKCF,
};

/*
 * Whether the instance has a CCMR1 register, with the polarity and the output enable of each
 * channel. The other instances have a single output, with its polarity in CFGR.
 */
static bool has_ccmr1(const LPTIM_TypeDef *timer)
{
	/*
	 * Input capture is configured in CCMR1 too. IS_LPTIM_CC1_INSTANCE() cannot tell, since most
	 * series count the instances without CCMR1 among those with capture/compare channels.
	 */
	return IS_LPTIM_INPUT_CAPTURE_INSTANCE(timer);
}

/* Let the timer keep its kernel clock in Stop modes, where the SoC supports it. */
static void enable_autonomous_mode(const LPTIM_TypeDef *timer)
{
#if defined(LL_SRDAMR_GRP1_PERIPH_LPTIM1AMEN) && defined(LL_SRDAMR_GRP1_PERIPH_LPTIM3AMEN) &&      \
	defined(LL_SRDAMR_GRP1_PERIPH_LPTIM4AMEN)
	if (timer == LPTIM1) {
		LL_SRDAMR_GRP1_EnableAutonomousClock(LL_SRDAMR_GRP1_PERIPH_LPTIM1AMEN);
	} else if (timer == LPTIM3) {
		LL_SRDAMR_GRP1_EnableAutonomousClock(LL_SRDAMR_GRP1_PERIPH_LPTIM3AMEN);
	} else if (timer == LPTIM4) {
		LL_SRDAMR_GRP1_EnableAutonomousClock(LL_SRDAMR_GRP1_PERIPH_LPTIM4AMEN);
	} else {
		/* The other instances have no autonomous mode. */
	}
#else
	ARG_UNUSED(timer);
#endif
}

static void pause_between_polls(bool sleep)
{
	if (sleep) {
		(void)k_sleep(K_TICKS(1));
	} else {
		k_busy_wait(1);
	}
}

/*
 * Wait until a write to ARR or CCRx completes, about 3 counter clock cycles or 5 right after the
 * timer is enabled, since a new write before that gives unpredictable results. Outside
 * interrupts, a wait longer than a system tick sleeps one tick per poll.
 */
static int wait_write(const struct device *dev, uint32_t flag)
{
	const struct pwm_stm32_lptim_config *cfg = dev->config;
	const struct pwm_stm32_lptim_data *data = dev->data;
	uint32_t tick_us = k_ticks_to_us_floor32(1U);
	uint32_t write_us = 3U * cfg->prescaler * USEC_PER_SEC / data->tim_clk;
	uint32_t timeout_us = 10U * cfg->prescaler * USEC_PER_SEC / data->tim_clk + 10U;
	bool sleep = !k_is_in_isr() && (write_us > tick_us);
	uint32_t max_polls = sleep ? (timeout_us / tick_us + 1U) : timeout_us;

	/*
	 * The timeout counts polls of at least 1 us or one tick. WAIT_FOR() reads the cycle
	 * counter of the system timer instead, which can step back during a long wait in an
	 * interrupt that holds off the system timer.
	 */
	for (uint32_t polls = 0U; (cfg->timer->ISR & flag) == 0U; polls++) {
		if (polls == max_polls) {
			LOG_ERR("Register write timeout");
			return -EIO;
		}

		pause_between_polls(sleep);
	}

	return 0;
}

static int set_autoreload(const struct device *dev, uint32_t autoreload)
{
	const struct pwm_stm32_lptim_config *cfg = dev->config;

	cfg->timer->ICR = LPTIM_ICR_ARROKCF;
	LL_LPTIM_SetAutoReload(cfg->timer, autoreload);

	return wait_write(dev, LPTIM_ISR_ARROK);
}

static int set_compare(const struct device *dev, uint32_t ch, uint32_t compare)
{
	const struct pwm_stm32_lptim_config *cfg = dev->config;

	if (get_timer_compare[ch](cfg->timer) == compare) {
		return 0;
	}

	cfg->timer->ICR = ch2cmpokcf[ch];
	set_timer_compare[ch](cfg->timer, compare);

	return wait_write(dev, ch2cmpok[ch]);
}

/*
 * Reset the timer and configure it, which leaves it disabled. The configuration register can
 * only be written while the timer is disabled.
 */
static int reset_timer(const struct device *dev)
{
	const struct pwm_stm32_lptim_config *cfg = dev->config;
	int ret;

	ret = reset_line_toggle_dt(&cfg->reset);
	if (ret < 0) {
		return ret;
	}

	LL_LPTIM_SetClockSource(cfg->timer, LL_LPTIM_CLK_SOURCE_INTERNAL);
	LL_LPTIM_SetPrescaler(cfg->timer, __CLZ(__RBIT(cfg->prescaler)) << LPTIM_CFGR_PRESC_Pos);
	LL_LPTIM_SetWaveform(cfg->timer, LL_LPTIM_OUTPUT_WAVEFORM_PWM);
	/* Update ARR and CCRx at the end of the period, so a change never cuts a pulse short. */
	LL_LPTIM_SetUpdateMode(cfg->timer, LL_LPTIM_UPDATE_MODE_ENDOFPERIOD);

	return 0;
}

/*
 * Compare of a channel held at a constant level while the counter runs. A stopped counter needs
 * none, since the output polarity alone then sets the level.
 */
static inline uint32_t get_constant_level_compare(uint32_t autoreload)
{
	/*
	 * A compare at or above the autoreload would disturb the other channel, so the level is
	 * off for one cycle per period.
	 */
	return autoreload - 1U;
}

/*
 * Update the compare of a channel to a new autoreload. A waveform keeps the length of its pulse,
 * which ends with the period, unless the new period is not longer than the pulse.
 */
static int refresh_compare(const struct device *dev, uint32_t ch, uint32_t old_autoreload,
			   uint32_t autoreload)
{
	const struct pwm_stm32_lptim_config *cfg = dev->config;
	const struct pwm_stm32_lptim_data *data = dev->data;

	if (data->state[ch] == CHANNEL_WAVE) {
		uint32_t pulse = old_autoreload - get_timer_compare[ch](cfg->timer);

		return set_compare(dev, ch, (pulse < autoreload) ? (autoreload - pulse) : 0U);
	}

	return set_compare(dev, ch, get_constant_level_compare(autoreload));
}

/* Whether a new autoreload must be written before the compares rather than after them. */
static inline bool must_write_autoreload_first(uint32_t old_autoreload, uint32_t autoreload)
{
	/*
	 * A compare above the autoreload in effect would disturb the outputs, so a longer
	 * autoreload is written before the compares and a shorter one after them. Like the
	 * compares, it takes effect at the end of the current period. When that end falls between
	 * the writes, one period combines old and new values, which lengthens its pulses.
	 */
	return autoreload > old_autoreload;
}

/* Set the polarity of a channel output and enable the output. */
static void set_channel_output(LPTIM_TypeDef *timer, uint32_t ll_channel, enum channel_state state,
			       bool inverted)
{
	/*
	 * A disabled output (CCxE = 0) is not driven and would let the pin float, so a channel held
	 * low keeps its output enabled with the regular polarity. With a prescaler above 1, a timer
	 * whose counter does not run applies a polarity change only while it is enabled.
	 */
	LL_LPTIM_OC_SetPolarity(timer, ll_channel,
				((state == CHANNEL_HIGH) || ((state == CHANNEL_WAVE) && inverted))
					? LL_LPTIM_OUTPUT_POLARITY_INVERSE
					: LL_LPTIM_OUTPUT_POLARITY_REGULAR);
	LL_LPTIM_CC_EnableChannel(timer, ll_channel);
}

/* Stop the counter by disabling the timer and enabling it again without starting it. */
static void reenable_timer(LPTIM_TypeDef *timer)
{
	/*
	 * The enable bit is cleared directly. On some series LL_LPTIM_Disable() also resets the
	 * timer, for an erratum about LPTIM interrupts, which this driver does not use, and the
	 * reset would disable the outputs.
	 */
	CLEAR_BIT(timer->CR, LPTIM_CR_ENABLE);
	SET_BIT(timer->CR, LPTIM_CR_ENABLE);
}

static int set_cycles_channels(const struct device *dev, uint32_t ch, uint32_t period_cycles,
			       uint32_t pulse_cycles, bool inverted)
{
	const struct pwm_stm32_lptim_config *cfg = dev->config;
	struct pwm_stm32_lptim_data *data = dev->data;
	LPTIM_TypeDef *timer = cfg->timer;
	uint32_t ll_channel = ch2ll[ch];
	uint32_t other_ch = 1U - ch;
	uint32_t old_autoreload = LL_LPTIM_GetAutoReload(timer);
	uint32_t autoreload = period_cycles - 1U;
	bool start = !data->counting;
	enum channel_state state;
	int ret;

	if ((pulse_cycles == 0U) || (pulse_cycles == period_cycles)) {
		state = ((pulse_cycles == 0U) == inverted) ? CHANNEL_HIGH : CHANNEL_LOW;
	} else {
		state = CHANNEL_WAVE;
	}

	data->state[ch] = state;

	/* Without a waveform the counter is stopped, which cuts a pulse in progress short. */
	if ((state != CHANNEL_WAVE) && (data->state[other_ch] != CHANNEL_WAVE)) {
		set_channel_output(timer, ll_channel, state, inverted);
		reenable_timer(timer);
		data->counting = false;
		return 0;
	}

	if (start) {
		/* The polarity goes first, so that it has settled when the counter starts. */
		set_channel_output(timer, ll_channel, state, inverted);
	}

	if (start || must_write_autoreload_first(old_autoreload, autoreload)) {
		ret = set_autoreload(dev, autoreload);
		if (ret < 0) {
			return ret;
		}
	}

	/*
	 * The compare of the other channel follows the period. After a stop it can be above the
	 * new autoreload, which would disturb this channel.
	 */
	if (IS_LPTIM_CC2_INSTANCE(timer)) {
		ret = refresh_compare(dev, other_ch, old_autoreload, autoreload);
		if (ret < 0) {
			return ret;
		}
	}

	ret = set_compare(dev, ch,
			  (state == CHANNEL_WAVE) ? (autoreload - pulse_cycles)
						  : get_constant_level_compare(autoreload));
	if (ret < 0) {
		return ret;
	}

	set_channel_output(timer, ll_channel, state, inverted);

	if (!start && (autoreload < old_autoreload)) {
		/* A shorter autoreload is written after the compares, not before. */
		ret = set_autoreload(dev, autoreload);
		if (ret < 0) {
			return ret;
		}
	}

	/* The counter starts once the compares are written, so that the first period is right. */
	if (start) {
		LL_LPTIM_StartCounter(timer, LL_LPTIM_OPERATING_MODE_CONTINUOUS);
		data->counting = true;
	}

	return 0;
}

/* Set the polarity of an instance with a single output. The timer must be disabled. */
static void set_single_output_polarity(LPTIM_TypeDef *timer, bool inverted)
{
#if defined(LPTIM_CFGR_WAVPOL)
	/* The LL driver of some series, like STM32U3, has no function for WAVPOL. */
	MODIFY_REG(timer->CFGR, LPTIM_CFGR_WAVPOL, inverted ? LPTIM_CFGR_WAVPOL : 0U);
#else
	/* Series without single-output instances have no WAVPOL bit. */
	ARG_UNUSED(timer);
	ARG_UNUSED(inverted);
#endif
}

static bool is_single_output_inverted(const LPTIM_TypeDef *timer)
{
#if defined(LPTIM_CFGR_WAVPOL)
	return READ_BIT(timer->CFGR, LPTIM_CFGR_WAVPOL) != 0U;
#else
	ARG_UNUSED(timer);

	return false;
#endif
}

static int set_cycles_single_output(const struct device *dev, uint32_t period_cycles,
				    uint32_t pulse_cycles, bool inverted)
{
	const struct pwm_stm32_lptim_config *cfg = dev->config;
	LPTIM_TypeDef *timer = cfg->timer;
	bool constant = (pulse_cycles == 0U) || (pulse_cycles == period_cycles);
	int ret;

	/*
	 * A running waveform that keeps its polarity only gets a new compare and autoreload. The
	 * timer is disabled while the output holds a constant level.
	 */
	if (!constant && (LL_LPTIM_IsEnabled(timer) != 0U) &&
	    (is_single_output_inverted(timer) == inverted)) {
		uint32_t old_autoreload = LL_LPTIM_GetAutoReload(timer);
		uint32_t autoreload = period_cycles - 1U;

		if (must_write_autoreload_first(old_autoreload, autoreload)) {
			ret = set_autoreload(dev, autoreload);
			if (ret < 0) {
				return ret;
			}
		}

		ret = set_compare(dev, 0U, autoreload - pulse_cycles);
		if ((ret == 0) && (autoreload < old_autoreload)) {
			/* A shorter autoreload is written after the compare, not before. */
			ret = set_autoreload(dev, autoreload);
		}

		return ret;
	}

	/*
	 * The polarity can only be changed while the timer is disabled, which also holds the
	 * output at the polarity level.
	 */
	ret = reset_timer(dev);
	if (ret < 0) {
		return ret;
	}

	if (constant) {
		bool high = (pulse_cycles == period_cycles) != inverted;

		set_single_output_polarity(timer, high);
		return 0;
	}

	set_single_output_polarity(timer, inverted);
	LL_LPTIM_Enable(timer);

	ret = set_compare(dev, 0U, period_cycles - pulse_cycles - 1U);
	if (ret < 0) {
		return ret;
	}

	ret = set_autoreload(dev, period_cycles - 1U);
	if (ret < 0) {
		return ret;
	}

	/* The counter starts once the compare is written, so that the first period is right. */
	LL_LPTIM_StartCounter(timer, LL_LPTIM_OPERATING_MODE_CONTINUOUS);

	return 0;
}

static int pwm_stm32_lptim_set_cycles(const struct device *dev, uint32_t channel,
				      uint32_t period_cycles, uint32_t pulse_cycles,
				      pwm_flags_t flags)
{
	const struct pwm_stm32_lptim_config *cfg = dev->config;
	struct pwm_stm32_lptim_data *data = dev->data;
	uint32_t num_channels = IS_LPTIM_CC2_INSTANCE(cfg->timer) ? 2U : 1U;
	bool inverted = (flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED;
	int ret;

	if ((channel < 1U) || (channel > num_channels)) {
		LOG_ERR("Invalid channel (%u)", channel);
		return -EINVAL;
	}

	if ((period_cycles < 2U) || (period_cycles > MAX_PERIOD_CYCLES)) {
		LOG_ERR("Period of %u cycles not supported", period_cycles);
		return -ENOTSUP;
	}

	if (pulse_cycles > period_cycles) {
		return -EINVAL;
	}

	/* A thread waits for a call in progress, an interrupt cannot. */
	if (k_sem_take(&data->lock, k_is_in_isr() ? K_NO_WAIT : K_FOREVER) != 0) {
		return -EBUSY;
	}

	if (has_ccmr1(cfg->timer)) {
		ret = set_cycles_channels(dev, channel - 1U, period_cycles, pulse_cycles, inverted);
	} else {
		ret = set_cycles_single_output(dev, period_cycles, pulse_cycles, inverted);
	}

	k_sem_give(&data->lock);

	return ret;
}

static int pwm_stm32_lptim_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					      uint64_t *cycles)
{
	const struct pwm_stm32_lptim_config *cfg = dev->config;
	struct pwm_stm32_lptim_data *data = dev->data;
	uint32_t rate = data->tim_clk / cfg->prescaler;

	ARG_UNUSED(channel);

	*cycles = rate;

	return 0;
}

static DEVICE_API(pwm, pwm_stm32_lptim_driver_api) = {
	.set_cycles = pwm_stm32_lptim_set_cycles,
	.get_cycles_per_sec = pwm_stm32_lptim_get_cycles_per_sec,
};

static int pwm_stm32_lptim_init(const struct device *dev)
{
	const struct pwm_stm32_lptim_config *cfg = dev->config;
	struct pwm_stm32_lptim_data *data = dev->data;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int ret;

	k_sem_init(&data->lock, 1, 1);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(cfg->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	ret = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("Could not initialize clock (%d)", ret);
		return ret;
	}

	ret = clock_control_configure(clk, (clock_control_subsys_t)&cfg->pclken[1], NULL);
	if (ret < 0) {
		LOG_ERR("Could not configure clock (%d)", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk, (clock_control_subsys_t)&cfg->pclken[1], &data->tim_clk);
	if (ret < 0) {
		LOG_ERR("Timer clock rate get error (%d)", ret);
		return ret;
	}

	enable_autonomous_mode(cfg->timer);

	ret = reset_timer(dev);
	if (ret < 0) {
		return ret;
	}

	/* The channel outputs are driven low until the first call. */
	if (has_ccmr1(cfg->timer)) {
		set_channel_output(cfg->timer, ch2ll[0], CHANNEL_LOW, false);
		if (IS_LPTIM_CC2_INSTANCE(cfg->timer)) {
			set_channel_output(cfg->timer, ch2ll[1], CHANNEL_LOW, false);
		}
		reenable_timer(cfg->timer);
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", ret);
		return ret;
	}

	return 0;
}

#define TIMER(index) DT_INST_PARENT(index)

#define PWM_DEVICE_INIT(index)                                                                     \
	BUILD_ASSERT(DT_NUM_CLOCKS(TIMER(index)) == 2, "Timer clock source is required");          \
	BUILD_ASSERT(!DT_HAS_CHOSEN(zephyr_system_timer) ||                                        \
			     !DT_SAME_NODE(DT_CHOSEN(zephyr_system_timer), TIMER(index)),          \
		     "The system timer LPTIM cannot generate PWM");                                \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct stm32_pclken pwm_stm32_lptim_pclken_##index[] =                        \
		STM32_DT_CLOCKS(TIMER(index));                                                     \
                                                                                                   \
	static const struct pwm_stm32_lptim_config pwm_stm32_lptim_config_##index = {              \
		.timer = (LPTIM_TypeDef *)DT_REG_ADDR(TIMER(index)),                               \
		.prescaler = DT_PROP(TIMER(index), st_prescaler),                                  \
		.pclken = pwm_stm32_lptim_pclken_##index,                                          \
		.reset = RESET_DT_SPEC_GET(TIMER(index)),                                          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
	};                                                                                         \
                                                                                                   \
	static struct pwm_stm32_lptim_data pwm_stm32_lptim_data_##index;                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &pwm_stm32_lptim_init, NULL, &pwm_stm32_lptim_data_##index,   \
			      &pwm_stm32_lptim_config_##index, POST_KERNEL,                        \
			      CONFIG_PWM_INIT_PRIORITY, &pwm_stm32_lptim_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT)
