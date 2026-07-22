/*
 * Copyright (c) 2025, Linumiz GmbH
 * Copyright (c) 2026, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_timer_pwm

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_mspm0, CONFIG_PWM_LOG_LEVEL);

/* GPTIMER register map — TRM Chapter 34 */

struct mspm0_gptimer_gprcm {
	volatile uint32_t pwren;
	volatile uint32_t rstctl;
	uint32_t reserved[3];
	volatile uint32_t stat;
};

struct mspm0_gptimer_int {
	volatile uint32_t iidx;
	uint32_t reserved0;
	volatile uint32_t imask;
	uint32_t reserved1;
	volatile uint32_t ris;
	uint32_t reserved2;
	volatile uint32_t mis;
	uint32_t reserved3;
	volatile uint32_t iset;
	uint32_t reserved4;
	volatile uint32_t iclr;
};

struct mspm0_gptimer_common {
	volatile uint32_t ccpd;
	volatile uint32_t odis;
	volatile uint32_t cclkctl;
	volatile uint32_t cps;
	volatile uint32_t cpsv;
	volatile uint32_t cttrigctl;
	uint32_t reserved0;
	volatile uint32_t cttrig;
	volatile uint32_t fsctl;
	volatile uint32_t gctl;
};

/* Extended COUNTERREGS: covers CC control/action/filter registers for PWM */
struct mspm0_gptimer_counter_regs {
	volatile uint32_t ctr;         /* +0x00 (0x1800) */
	volatile uint32_t ctrctl;      /* +0x04 */
	volatile uint32_t load;        /* +0x08 */
	uint32_t reserved0;            /* +0x0C */
	volatile uint32_t cc_01[2];    /* +0x10 — CC ch0/ch1 */
	volatile uint32_t cc_23[2];    /* +0x18 — CC ch2/ch3 */
	volatile uint32_t cc_45[2];    /* +0x20 */
	uint32_t reserved1[2];         /* +0x28 */
	volatile uint32_t ccctl_01[2]; /* +0x30 (0x1830) */
	volatile uint32_t ccctl_23[2]; /* +0x38 */
	volatile uint32_t ccctl_45[2]; /* +0x40 */
	uint32_t reserved2[2];         /* +0x48 */
	volatile uint32_t octl_01[2];  /* +0x50 (0x1850) */
	volatile uint32_t octl_23[2];  /* +0x58 */
	uint32_t reserved3[4];         /* +0x60 */
	volatile uint32_t ccact_01[2]; /* +0x70 (0x1870) */
	volatile uint32_t ccact_23[2]; /* +0x78 */
	volatile uint32_t ifctl_01[2]; /* +0x80 (0x1880) */
	volatile uint32_t ifctl_23[2]; /* +0x88 */
};

struct mspm0_gptimer_regs {
	uint32_t reserved0[256];
	volatile uint32_t fsub_0;
	volatile uint32_t fsub_1;
	uint32_t reserved1[15];
	volatile uint32_t fpub_0;
	volatile uint32_t fpub_1;
	uint32_t reserved2[237];
	struct mspm0_gptimer_gprcm gprcm; /* 0x800 */
	uint32_t reserved3[506];
	volatile uint32_t clkdiv; /* 0x1000 */
	uint32_t reserved4;
	volatile uint32_t clksel; /* 0x1008 */
	uint32_t reserved5[3];
	volatile uint32_t pdbgctl;
	uint32_t reserved6;
	struct mspm0_gptimer_int cpu_int; /* 0x1020 */
	uint32_t reserved7;
	struct mspm0_gptimer_int gen_event0; /* 0x1050 */
	uint32_t reserved8;
	struct mspm0_gptimer_int gen_event1; /* 0x1080 */
	uint32_t reserved9[13];
	volatile uint32_t evt_mode;
	uint32_t reserved10[6];
	volatile uint32_t desc;
	struct mspm0_gptimer_common commonregs; /* 0x1100 */
	uint32_t reserved11[438];
	struct mspm0_gptimer_counter_regs counterregs; /* 0x1800 */
};

BUILD_ASSERT(offsetof(struct mspm0_gptimer_regs, gprcm) == 0x0800U);
BUILD_ASSERT(offsetof(struct mspm0_gptimer_regs, clkdiv) == 0x1000U);
BUILD_ASSERT(offsetof(struct mspm0_gptimer_regs, cpu_int) == 0x1020U);
BUILD_ASSERT(offsetof(struct mspm0_gptimer_regs, commonregs) == 0x1100U);
BUILD_ASSERT(offsetof(struct mspm0_gptimer_regs, counterregs) == 0x1800U);
BUILD_ASSERT(offsetof(struct mspm0_gptimer_counter_regs, ccctl_01) == 0x30U);
BUILD_ASSERT(offsetof(struct mspm0_gptimer_counter_regs, ccact_01) == 0x70U);
BUILD_ASSERT(offsetof(struct mspm0_gptimer_counter_regs, ifctl_01) == 0x80U);

#ifndef CONFIG_HAS_MSPM0_SDK

/* GPRCM.PWREN */
#define GPTIMER_PWREN_KEY_UNLOCK_W      0x26000000U
#define GPTIMER_PWREN_ENABLE_ENABLE     BIT(0)

/* GPRCM.RSTCTL */
#define GPTIMER_RSTCTL_KEY_UNLOCK_W         0xB1000000U
#define GPTIMER_RSTCTL_RESETSTKYCLR_CLR     BIT(1)
#define GPTIMER_RSTCTL_RESETASSERT_ASSERT   BIT(0)

/* COUNTERREGS.CTRCTL */
#define GPTIMER_CTRCTL_EN_ENABLED       BIT(0)
#define GPTIMER_CTRCTL_EN_MASK          BIT(0)
#define GPTIMER_CTRCTL_REPEAT_REPEAT_1  BIT(1)
#define GPTIMER_CTRCTL_CM_DOWN          0x00000000U
#define GPTIMER_CTRCTL_CM_UP_DOWN       BIT(4)
#define GPTIMER_CTRCTL_CM_UP            BIT(5)
#define GPTIMER_CTRCTL_CVAE_ZEROVAL     BIT(29)

/* COMMONREGS */
#define GPTIMER_CCLKCTL_CLKEN_ENABLED   BIT(0)

/* CCCTL_01/23 */
#define GPTIMER_CCCTL_01_COC_COMPARE            0x00000000U
#define GPTIMER_CCCTL_01_COC_CAPTURE            BIT(17)
#define GPTIMER_CCCTL_01_CCUPD_ZERO_EVT         BIT(18)
#define GPTIMER_CCCTL_01_CCUPD_ZERO_LOAD_EVT    BIT(20)
#define GPTIMER_CCCTL_01_CCOND_CC_TRIG_RISE     BIT(0)
#define GPTIMER_CCCTL_01_CCOND_CC_TRIG_FALL     BIT(1)

/* CCACT_01/23 — output pin actions */
#define GPTIMER_CCACT_01_ZACT_CCP_HIGH  BIT(0)
#define GPTIMER_CCACT_01_ZACT_CCP_LOW   BIT(1)
#define GPTIMER_CCACT_01_LACT_CCP_HIGH  BIT(3)
#define GPTIMER_CCACT_01_LACT_CCP_LOW   BIT(4)
#define GPTIMER_CCACT_01_CDACT_CCP_LOW  BIT(7)
#define GPTIMER_CCACT_01_CDACT_CCP_HIGH BIT(6)
#define GPTIMER_CCACT_01_CUACT_CCP_LOW  BIT(10)
#define GPTIMER_CCACT_01_CUACT_CCP_HIGH BIT(9)

/* IFCTL_01/23 ISEL */
#define GPTIMER_IFCTL_01_ISEL_CCPX_INPUT        0x00000000U
#define GPTIMER_IFCTL_01_ISEL_CCPX_INPUT_PAIR   BIT(0)
#define GPTIMER_IFCTL_01_ISEL_CCP0_INPUT        BIT(1)

#endif /* CONFIG_HAS_MSPM0_SDK */

#define GPTIMER_CCPD_OUTPUT_MASK(ch) BIT(ch)

/* CPU_INT interrupt bits — same position in IMASK, RIS, ICLR */
#define GPTIMER_INT_ZERO_BIT     BIT(0) /* Z: zero/underflow event */
#define GPTIMER_INT_CCD_MASK(ch) BIT(4U + (ch))

#define MSPM0_TIMER_CC_MAX 4U

enum pwm_mspm0_mode {
	PWM_MSPM0_EDGE_ALIGN,    /* down-count: LACT=LOW, CDACT=HIGH */
	PWM_MSPM0_EDGE_ALIGN_UP, /* up-count:   ZACT=HIGH, CUACT=LOW */
	PWM_MSPM0_CENTER_ALIGN,  /* up-down:    CUACT=HIGH, CDACT=LOW */
};

#ifdef CONFIG_PWM_CAPTURE
enum mspm0_capture_mode {
	CMODE_EDGE_TIME,
	CMODE_PULSE_WIDTH
};
#endif

struct pwm_mspm0_config {
	struct mspm0_gptimer_regs *base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	struct mspm0_sys_clock clock_subsys;
	uint32_t clk_sel;
	uint32_t clk_div_reg; /* CLKDIV value: ti_clk_div - 1 */
	uint8_t prescaler;
	enum pwm_mspm0_mode mode;
#ifdef CONFIG_PWM_CAPTURE
	void (*irq_config_func)(const struct device *dev);
#endif
	uint8_t	cc_idx[MSPM0_TIMER_CC_MAX];
	uint8_t cc_idx_cnt;
	bool is_capture;
};

struct pwm_mspm0_data {
	uint32_t pulse_cycle[MSPM0_TIMER_CC_MAX];
	uint32_t period;
	uint32_t freq_hz;
	struct k_mutex lock;
#ifdef CONFIG_PWM_CAPTURE
	uint32_t last_sample;
	enum mspm0_capture_mode cmode;
	pwm_capture_callback_handler_t callback;
	pwm_flags_t flags;
	void *user_data;
	bool is_synced;
	bool armed;
#endif
};

static inline void mspm0_pwm_write_cc(struct mspm0_gptimer_regs *base, uint8_t ch, uint32_t val)
{
	if (ch < 2U) {
		base->counterregs.cc_01[ch] = val;
	} else {
		base->counterregs.cc_23[ch - 2U] = val;
	}
}

static inline uint32_t mspm0_pwm_read_cc(struct mspm0_gptimer_regs *base, uint8_t ch)
{
	if (ch < 2U) {
		return base->counterregs.cc_01[ch];
	}
	return base->counterregs.cc_23[ch - 2U];
}

static inline void mspm0_pwm_write_ccctl(struct mspm0_gptimer_regs *base, uint8_t ch, uint32_t val)
{
	if (ch < 2U) {
		base->counterregs.ccctl_01[ch] = val;
	} else {
		base->counterregs.ccctl_23[ch - 2U] = val;
	}
}

static inline void mspm0_pwm_write_ccact(struct mspm0_gptimer_regs *base, uint8_t ch, uint32_t val)
{
	if (ch < 2U) {
		base->counterregs.ccact_01[ch] = val;
	} else {
		base->counterregs.ccact_23[ch - 2U] = val;
	}
}

static inline void mspm0_pwm_write_octl(struct mspm0_gptimer_regs *base, uint8_t ch, uint32_t val)
{
	if (ch < 2U) {
		base->counterregs.octl_01[ch] = val;
	} else {
		base->counterregs.octl_23[ch - 2U] = val;
	}
}

static inline void mspm0_pwm_write_ifctl(struct mspm0_gptimer_regs *base, uint8_t ch, uint32_t val)
{
	if (ch < 2U) {
		base->counterregs.ifctl_01[ch] = val;
	} else {
		base->counterregs.ifctl_23[ch - 2U] = val;
	}
}

/*
 * EDGE_ALIGN (down): LACT/CDACT toggle HIGH for `pulse` ticks. At 0%/100%
 * both events coincide — use a fixed action to avoid a 1-tick glitch.
 */
static inline uint32_t mspm0_pwm_edge_align_ccact(uint32_t pulse, uint32_t load)
{
	if (pulse == 0U) {
		return GPTIMER_CCACT_01_LACT_CCP_LOW;
	}
	if (pulse >= load) {
		return GPTIMER_CCACT_01_LACT_CCP_HIGH;
	}
	return GPTIMER_CCACT_01_LACT_CCP_LOW | GPTIMER_CCACT_01_CDACT_CCP_HIGH;
}

/*
 * EDGE_ALIGN (up): ZACT sets HIGH, CUACT sets LOW. At 0%/100% both events
 * coincide — use a fixed action.
 */
static inline uint32_t mspm0_pwm_edge_align_up_ccact(uint32_t pulse, uint32_t load)
{
	if (pulse == 0U) {
		return GPTIMER_CCACT_01_ZACT_CCP_LOW;
	}
	if (pulse >= load) {
		return GPTIMER_CCACT_01_ZACT_CCP_HIGH;
	}
	return GPTIMER_CCACT_01_ZACT_CCP_HIGH | GPTIMER_CCACT_01_CUACT_CCP_LOW;
}

/*
 * CENTER_ALIGN (up-down): LOAD = period/2; CC = load - pulse/2 gives
 * `pulse` HIGH ticks symmetrically around the peak.
 */
static inline uint32_t mspm0_pwm_center_align_cc(uint32_t load, uint32_t pulse)
{
	return load - (pulse / 2U);
}

static void mspm0_pwm_setup_cc_chan(struct mspm0_gptimer_regs *base, uint8_t ch,
				    enum pwm_mspm0_mode mode, uint32_t pulse)
{
	uint32_t ccact;
	uint32_t ccupd;
	uint32_t cc_val = pulse;

	switch (mode) {
	case PWM_MSPM0_EDGE_ALIGN:
		ccact = mspm0_pwm_edge_align_ccact(pulse, base->counterregs.load);
		ccupd = GPTIMER_CCCTL_01_CCUPD_ZERO_EVT;
		break;
	case PWM_MSPM0_EDGE_ALIGN_UP:
		ccact = mspm0_pwm_edge_align_up_ccact(pulse, base->counterregs.load);
		ccupd = GPTIMER_CCCTL_01_CCUPD_ZERO_EVT;
		break;
	default: /* PWM_MSPM0_CENTER_ALIGN */
		ccact = GPTIMER_CCACT_01_CUACT_CCP_HIGH | GPTIMER_CCACT_01_CDACT_CCP_LOW;
		ccupd = GPTIMER_CCCTL_01_CCUPD_ZERO_LOAD_EVT;
		cc_val = mspm0_pwm_center_align_cc(base->counterregs.load, pulse);
		break;
	}

	mspm0_pwm_write_ccact(base, ch, ccact);
	mspm0_pwm_write_ccctl(base, ch, GPTIMER_CCCTL_01_COC_COMPARE | ccupd);
	mspm0_pwm_write_octl(base, ch, 0U);
	mspm0_pwm_write_ifctl(base, ch, GPTIMER_IFCTL_01_ISEL_CCPX_INPUT);
	mspm0_pwm_write_cc(base, ch, cc_val);
}

static void mspm0_setup_pwm_out(const struct pwm_mspm0_config *config,
				struct pwm_mspm0_data *data)
{
	int i;
	struct mspm0_gptimer_regs *base = config->base;
	uint32_t ctrctl;
	uint32_t ccpd_mask = 0U;

	switch (config->mode) {
	case PWM_MSPM0_EDGE_ALIGN:
		ctrctl = GPTIMER_CTRCTL_CM_DOWN;
		break;
	case PWM_MSPM0_EDGE_ALIGN_UP:
		ctrctl = GPTIMER_CTRCTL_CM_UP | GPTIMER_CTRCTL_CVAE_ZEROVAL;
		break;
	default: /* PWM_MSPM0_CENTER_ALIGN */
		ctrctl = GPTIMER_CTRCTL_CM_UP_DOWN;
		break;
	}

	/* CENTER_ALIGN: LOAD = period/2; keep data->period intact to avoid
	 * double-halving on reinit.
	 */
	uint32_t load = (config->mode == PWM_MSPM0_CENTER_ALIGN) ? (data->period >> 1)
								 : data->period;

	base->counterregs.load = load;

	for (i = 0; i < config->cc_idx_cnt; i++) {
		uint8_t ch = config->cc_idx[i];

		mspm0_pwm_setup_cc_chan(base, ch, config->mode, data->pulse_cycle[i]);
		ccpd_mask |= GPTIMER_CCPD_OUTPUT_MASK(ch);
	}

	base->commonregs.ccpd |= ccpd_mask;
	base->commonregs.cclkctl = GPTIMER_CCLKCTL_CLKEN_ENABLED;
	base->counterregs.ctrctl =
		ctrctl | GPTIMER_CTRCTL_REPEAT_REPEAT_1 | GPTIMER_CTRCTL_EN_ENABLED;
}

static int mspm0_pwm_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	struct mspm0_gptimer_regs *base = config->base;
	uint32_t period;

	if (channel >= config->cc_idx_cnt) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (period_cycles == 0U || period_cycles > UINT16_MAX) {
		LOG_ERR("period cycles %u out of range [1, %u]", period_cycles, UINT16_MAX);
		return -ENOTSUP;
	}

	if ((flags & PWM_POLARITY_INVERTED) && pulse_cycles > period_cycles) {
		LOG_ERR("pulse_cycles %u > period_cycles %u", pulse_cycles, period_cycles);
		return -EINVAL;
	}

	period = (config->mode == PWM_MSPM0_CENTER_ALIGN) ? (period_cycles >> 1) : period_cycles;

	if (flags & PWM_POLARITY_INVERTED) {
		pulse_cycles = period_cycles - pulse_cycles;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->pulse_cycle[channel] = pulse_cycles;
	data->period = period;

	base->counterregs.load = period;

	if (config->mode == PWM_MSPM0_CENTER_ALIGN) {
		/* LOAD is shared; recompute all channels' CC on period change. */
		for (int i = 0; i < config->cc_idx_cnt; i++) {
			mspm0_pwm_write_cc(base, config->cc_idx[i],
					   mspm0_pwm_center_align_cc(period, data->pulse_cycle[i]));
		}
	} else {
		mspm0_pwm_write_cc(base, config->cc_idx[channel], pulse_cycles);
	}

	if (config->mode == PWM_MSPM0_EDGE_ALIGN) {
		mspm0_pwm_write_ccact(base, config->cc_idx[channel],
				      mspm0_pwm_edge_align_ccact(pulse_cycles, period));
	}

	if (config->mode == PWM_MSPM0_EDGE_ALIGN_UP) {
		mspm0_pwm_write_ccact(base, config->cc_idx[channel],
				      mspm0_pwm_edge_align_up_ccact(pulse_cycles, period));
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mspm0_pwm_get_cycles_per_sec(const struct device *dev,
					uint32_t channel, uint64_t *cycles)
{
	ARG_UNUSED(channel);
	const struct pwm_mspm0_data *data = dev->data;

	if (cycles == NULL) {
		return -EINVAL;
	}

	*cycles = data->freq_hz;

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE

/*
 * Interrupt channel depends on cap_mode, not capture TYPE. In PULSE_WIDTH
 * mode only cc_idx[0]^1 (the rise channel) fires; using TYPE flags instead
 * would arm the wrong channel for EDGE_TIME + PERIOD/PULSE/BOTH.
 */
static uint32_t mspm0_pwm_cap_intr_mask(const struct pwm_mspm0_config *config,
					const struct pwm_mspm0_data *data)
{
	if (data->cmode == CMODE_PULSE_WIDTH) {
		return GPTIMER_INT_CCD_MASK(config->cc_idx[0] ^ 1U) | GPTIMER_INT_ZERO_BIT;
	}
	return GPTIMER_INT_CCD_MASK(config->cc_idx[0]);
}

static void mspm0_set_combined_mode(const struct pwm_mspm0_config *config)
{
	struct mspm0_gptimer_regs *base = config->base;
	uint8_t ch_fall = config->cc_idx[0];
	uint8_t ch_rise = ch_fall ^ 1U;
	uint32_t isel_fall = (ch_fall & 1U) ? GPTIMER_IFCTL_01_ISEL_CCPX_INPUT
					    : GPTIMER_IFCTL_01_ISEL_CCP0_INPUT;

	mspm0_pwm_write_ccctl(base, ch_fall,
			      GPTIMER_CCCTL_01_COC_CAPTURE |
				     GPTIMER_CCCTL_01_CCOND_CC_TRIG_FALL);
	mspm0_pwm_write_ifctl(base, ch_fall, isel_fall);

	mspm0_pwm_write_ccctl(base, ch_rise,
			      GPTIMER_CCCTL_01_COC_CAPTURE |
				     GPTIMER_CCCTL_01_CCOND_CC_TRIG_RISE);
	mspm0_pwm_write_ifctl(base, ch_rise, GPTIMER_IFCTL_01_ISEL_CCPX_INPUT_PAIR);

	base->commonregs.ccpd &=
		~(GPTIMER_CCPD_OUTPUT_MASK(ch_fall) | GPTIMER_CCPD_OUTPUT_MASK(ch_rise));
	base->counterregs.ctrctl = GPTIMER_CTRCTL_CM_DOWN | GPTIMER_CTRCTL_REPEAT_REPEAT_1;
}

static void mspm0_setup_capture(const struct device *dev,
				const struct pwm_mspm0_config *config,
				struct pwm_mspm0_data *data)
{
	struct mspm0_gptimer_regs *base = config->base;

	base->counterregs.load = data->period;

	if (data->cmode == CMODE_EDGE_TIME) {
		uint8_t ch = config->cc_idx[0];
		uint32_t isel = (ch & 1U) ? GPTIMER_IFCTL_01_ISEL_CCPX_INPUT
					  : GPTIMER_IFCTL_01_ISEL_CCP0_INPUT;

		mspm0_pwm_write_ccctl(base, ch,
				      GPTIMER_CCCTL_01_COC_CAPTURE |
					     GPTIMER_CCCTL_01_CCOND_CC_TRIG_RISE);
		mspm0_pwm_write_ifctl(base, ch, isel);
		base->commonregs.ccpd &= ~GPTIMER_CCPD_OUTPUT_MASK(ch);
		base->counterregs.ctrctl = GPTIMER_CTRCTL_CM_DOWN | GPTIMER_CTRCTL_REPEAT_REPEAT_1;
	} else {
		mspm0_set_combined_mode(config);
	}

	base->commonregs.cclkctl = GPTIMER_CCLKCTL_CLKEN_ENABLED;
	config->irq_config_func(dev);
}

static int mspm0_capture_configure(const struct device *dev,
				   uint32_t channel,
				   pwm_flags_t flags,
				   pwm_capture_callback_handler_t cb,
				   void *user_data)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	uint32_t intr_mask;

	if (config->is_capture != true ||
	    channel != 0) {
		LOG_ERR("Invalid channel %d", channel);
		return -EINVAL;
	}

	intr_mask = mspm0_pwm_cap_intr_mask(config, data);

	k_mutex_lock(&data->lock, K_FOREVER);

	/* If interrupt is enabled --> channel is on-going */
	if (config->base->cpu_int.imask & intr_mask) {
		LOG_ERR("Channel %d is busy", channel);
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	data->flags = flags;
	data->callback = cb;
	data->user_data = user_data;

	if (data->cmode == CMODE_PULSE_WIDTH) {
		struct mspm0_gptimer_regs *base = config->base;
		uint8_t ch_fall = config->cc_idx[0];
		uint8_t ch_rise = ch_fall ^ 1U;
		bool inverted = flags & PWM_POLARITY_INVERTED;

		/*
		 * Inverted polarity: swap captured edges (ch_fall→RISE,
		 * ch_rise→FALL) so the same formula yields the LOW duration.
		 */
		mspm0_pwm_write_ccctl(base, ch_fall,
				      GPTIMER_CCCTL_01_COC_CAPTURE |
					     (inverted ? GPTIMER_CCCTL_01_CCOND_CC_TRIG_RISE
						       : GPTIMER_CCCTL_01_CCOND_CC_TRIG_FALL));
		mspm0_pwm_write_ccctl(base, ch_rise,
				      GPTIMER_CCCTL_01_COC_CAPTURE |
					     (inverted ? GPTIMER_CCCTL_01_CCOND_CC_TRIG_FALL
						       : GPTIMER_CCCTL_01_CCOND_CC_TRIG_RISE));
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mspm0_capture_enable(const struct device *dev, uint32_t channel)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	struct mspm0_gptimer_regs *base = config->base;
	uint32_t intr_mask;

	if (config->is_capture != true ||
	    channel != 0) {
		LOG_ERR("Invalid capture mode or channel");
		return -EINVAL;
	}

	if (!data->callback) {
		LOG_ERR("Callback is not configured");
		return -EINVAL;
	}

	intr_mask = mspm0_pwm_cap_intr_mask(config, data);

	k_mutex_lock(&data->lock, K_FOREVER);

	/* If interrupt is enabled --> channel is on-going */
	if (base->cpu_int.imask & intr_mask) {
		LOG_ERR("Channel %d is busy", channel);
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	/* Re-baseline on every arm; CC may hold stale data from a previous capture. */
	data->is_synced = false;
	data->armed = false;

	base->counterregs.ctr = data->period;
	base->counterregs.ctrctl |= GPTIMER_CTRCTL_EN_ENABLED;
	base->cpu_int.iclr = intr_mask;
	base->cpu_int.imask |= intr_mask;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int mspm0_capture_disable(const struct device *dev, uint32_t channel)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	struct mspm0_gptimer_regs *base = config->base;
	uint32_t intr_mask;

	if (config->is_capture != true ||
	    channel != 0) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	intr_mask = mspm0_pwm_cap_intr_mask(config, data);

	k_mutex_lock(&data->lock, K_FOREVER);

	base->cpu_int.imask &= ~intr_mask;
	base->counterregs.ctrctl &= ~GPTIMER_CTRCTL_EN_MASK;
	data->is_synced = false;
	data->armed = false;

	k_mutex_unlock(&data->lock);

	return 0;
}
#endif /* CONFIG_PWM_CAPTURE */

static int pwm_mspm0_init(const struct device *dev)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	struct mspm0_gptimer_regs *base = config->base;
	uint32_t clock_rate;
	int err;

	k_mutex_init(&data->lock);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	struct mspm0_sys_clock clock_subsys = config->clock_subsys;

	err = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)&clock_subsys, &clock_rate);
	if (err != 0) {
		LOG_ERR("clk get rate err %d", err);
		return err;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	base->gprcm.rstctl = GPTIMER_RSTCTL_KEY_UNLOCK_W | GPTIMER_RSTCTL_RESETSTKYCLR_CLR |
			     GPTIMER_RSTCTL_RESETASSERT_ASSERT;
	base->gprcm.pwren = GPTIMER_PWREN_KEY_UNLOCK_W | GPTIMER_PWREN_ENABLE_ENABLE;
	msp_delay_peripheral_startup();

	base->clksel = config->clk_sel;
	base->clkdiv = config->clk_div_reg;
	base->commonregs.cps = config->prescaler;

	data->freq_hz =
		clock_rate / ((config->clk_div_reg + 1U) * ((uint32_t)config->prescaler + 1U));

	base->cpu_int.imask = 0U;
	base->cpu_int.iclr = 0xFFFFFFFFU;

	if (config->is_capture) {
#ifdef CONFIG_PWM_CAPTURE
		mspm0_setup_capture(dev, config, data);
#endif
	} else {
		mspm0_setup_pwm_out(config, data);
	}

	return 0;
}

static DEVICE_API(pwm, pwm_mspm0_driver_api) = {
	.set_cycles = mspm0_pwm_set_cycles,
	.get_cycles_per_sec = mspm0_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = mspm0_capture_configure,
	.enable_capture = mspm0_capture_enable,
	.disable_capture = mspm0_capture_disable,
#endif
};

#ifdef CONFIG_PWM_CAPTURE
static void mspm0_cc_isr(const struct device *dev)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	struct mspm0_gptimer_regs *base = config->base;
	uint32_t raw_ris;
	uint32_t ris;
	uint32_t cc1 = 0;
	uint32_t cc0 = 0;
	uint32_t period = 0;
	uint32_t pulse = 0;

	/* Clear all pending bits to avoid stale compare-match bits from unused CC channels. */
	raw_ris = base->cpu_int.ris;
	ris = raw_ris & mspm0_pwm_cap_intr_mask(config, data);
	base->cpu_int.iclr = raw_ris;

	if (!ris) {
		return;
	}

	if (ris & GPTIMER_INT_ZERO_BIT) {
		if (!(data->flags & PWM_CAPTURE_MODE_CONTINUOUS)) {
			if (data->callback) {
				data->callback(dev, 0, 0, 0, -ERANGE, data->user_data);
				base->counterregs.ctrctl &= ~GPTIMER_CTRCTL_EN_MASK;
			}
			return;
		}
		/* Continuous: ZERO is a counter wrap, not an error;
		 * fall through to process any CC edge.
		 */
		if (!(ris & ~GPTIMER_INT_ZERO_BIT)) {
			return;
		}
	}

	if (data->cmode == CMODE_PULSE_WIDTH) {
		/*
		 * Read cc1 unconditionally: it anchors period/pulse for all
		 * capture types. Gating on PERIOD flag breaks PULSE-only mode.
		 */
		cc1 = mspm0_pwm_read_cc(base, config->cc_idx[0] ^ 1U);
	}

	if (!data->is_synced && data->cmode != CMODE_EDGE_TIME) {
		data->last_sample = cc1;
		data->is_synced = true;
		return;
	}

	if ((data->flags & PWM_CAPTURE_TYPE_PULSE) || data->cmode == CMODE_EDGE_TIME) {
		cc0 = mspm0_pwm_read_cc(base, config->cc_idx[0]);
	}

	if (data->cmode == CMODE_PULSE_WIDTH && !data->armed) {
		/* First interval may straddle a pre-arm edge; discard and re-baseline. */
		data->armed = true;
		data->last_sample = cc1;
		return;
	}

	if (!(data->flags & PWM_CAPTURE_MODE_CONTINUOUS)) {
		uint32_t mask = mspm0_pwm_cap_intr_mask(config, data);

		base->cpu_int.imask &= ~mask;
		base->counterregs.ctrctl &= ~GPTIMER_CTRCTL_EN_MASK;
		data->is_synced = false;
		data->armed = false;
	}

	if (data->cmode == CMODE_EDGE_TIME) {
		/* Edge-time: cc0 = current, last_sample = previous;
		 * down-counter so earlier > later.
		 */
		period = (data->last_sample - cc0) & 0xFFFFU;
		pulse = 0U;
		data->last_sample = cc0;
	} else {
		/* cc0 = fall, cc1 = rise; period = last_sample - cc1, pulse = last_sample - cc0. */
		period = (data->last_sample - cc1) & 0xFFFFU;
		pulse = (data->last_sample - cc0) & 0xFFFFU;

		/* Guard: pulse > period means stale fall edge; armed-discard should prevent this.
		 */
		if (pulse > period) {
			pulse -= period;
		}

		if (!(data->flags & PWM_CAPTURE_TYPE_PULSE)) {
			/* PERIOD-only: caller did not ask for pulse width. */
			pulse = 0U;
		}

		data->last_sample = cc1;
	}

	if (data->callback && period) {
		data->callback(dev, 0, period, pulse, 0, data->user_data);
	}
}
#endif /* CONFIG_PWM_CAPTURE */

/* Device instantiation */

#ifdef CONFIG_PWM_CAPTURE
#define MSP_CC_IRQ_REGISTER(n)							\
	static void mspm0_pwm_## n ##_irq_register(const struct device *dev)                       \
	{									\
		const struct pwm_mspm0_config *config = dev->config;		\
		if (!config->is_capture) {					\
			return;							\
		}								\
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)),				\
			    DT_IRQ(DT_INST_PARENT(n), priority), mspm0_cc_isr,	\
			    DEVICE_DT_INST_GET(n), 0);				\
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));				\
	}
#else
#define MSP_CC_IRQ_REGISTER(n)
#endif

/* Only output-capable instances (no ti,cc-mode) skip ISR registration. */
#define MSP_CC_IRQ_REGISTER_IF_CAPTURE(n)                                                          \
	COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_cc_mode), (MSP_CC_IRQ_REGISTER(n)), ())

#define MSPM0_PWM_MODE(tok)     _CONCAT(PWM_MSPM0_, tok)
#define MSPM0_CAPTURE_MODE(tok) _CONCAT(CMODE_, tok)

#define MSPM0_CC_IDX_ARRAY(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx),

#define PWM_DEVICE_INIT_MSPM0(n)                                                                   \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, ti_cc_index) <= MSPM0_TIMER_CC_MAX,                       \
		     "ti,cc-index exceeds hardware maximum of 4");                                 \
	BUILD_ASSERT(DT_PROP(DT_INST_PARENT(n), ti_clk_div) >= 1,                                  \
		     "ti,clk-div must be >= 1 (0 underflows clk_div_reg)");                        \
                                                                                                   \
	static struct pwm_mspm0_data pwm_mspm0_data_ ## n = {			\
		.period = DT_PROP(DT_DRV_INST(n), ti_period),			\
		IF_ENABLED(CONFIG_PWM_CAPTURE,                                                     \
		(COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_cc_mode),                         \
			(.cmode = MSPM0_CAPTURE_MODE(                                              \
				DT_STRING_TOKEN(DT_DRV_INST(n), ti_cc_mode)),),                    \
			()))) };                                                                   \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	MSP_CC_IRQ_REGISTER_IF_CAPTURE(n)                                                          \
                                                                                                   \
	static const struct pwm_mspm0_config pwm_mspm0_config_##n = {                              \
		.base = (struct mspm0_gptimer_regs *)DT_REG_ADDR(DT_INST_PARENT(n)),               \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(DT_INST_PARENT(n), 0)),           \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock_subsys =                                                                    \
			{                                                                          \
				.clk = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, clk),           \
			},                                                                         \
		.clk_sel = MSPM0_CLOCK_PERIPH_REG_MASK(                                            \
			DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, clk)),                         \
		.clk_div_reg = DT_PROP(DT_INST_PARENT(n), ti_clk_div) - 1U,                        \
		.prescaler = DT_PROP(DT_INST_PARENT(n), ti_clk_prescaler),                         \
		.mode = COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n),                               \
						     ti_pwm_mode),                                 \
			(MSPM0_PWM_MODE(                                                           \
				DT_STRING_TOKEN(DT_DRV_INST(n), ti_pwm_mode))),		           \
			(PWM_MSPM0_EDGE_ALIGN)),                                                   \
			 .cc_idx = {DT_INST_FOREACH_PROP_ELEM(n, ti_cc_index,                      \
							      MSPM0_CC_IDX_ARRAY)},                \
			 .cc_idx_cnt = DT_INST_PROP_LEN(n, ti_cc_index),                           \
		.is_capture = DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_cc_mode),                        \
		IF_ENABLED(CONFIG_PWM_CAPTURE,                                                     \
		(.irq_config_func = COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_cc_mode),	   \
						(mspm0_pwm_## n ##_irq_register), (NULL))))        \
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      pwm_mspm0_init,					\
			      NULL,						\
			      &pwm_mspm0_data_ ## n,				\
			      &pwm_mspm0_config_ ## n,				\
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,		\
			      &pwm_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT_MSPM0)
