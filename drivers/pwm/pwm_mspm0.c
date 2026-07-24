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

struct mspm_gptimer_gprcm {
	volatile uint32_t PWREN;
	volatile uint32_t RSTCTL;
	uint32_t RESERVED[3];
	volatile uint32_t STAT;
};

struct mspm_gptimer_int {
	volatile uint32_t IIDX;
	uint32_t RESERVED0;
	volatile uint32_t IMASK;
	uint32_t RESERVED1;
	volatile uint32_t RIS;
	uint32_t RESERVED2;
	volatile uint32_t MIS;
	uint32_t RESERVED3;
	volatile uint32_t ISET;
	uint32_t RESERVED4;
	volatile uint32_t ICLR;
};

struct mspm_gptimer_common {
	volatile uint32_t CCPD;
	volatile uint32_t ODIS;
	volatile uint32_t CCLKCTL;
	volatile uint32_t CPS;
	volatile uint32_t CPSV;
	volatile uint32_t CTTRIGCTL;
	uint32_t RESERVED0;
	volatile uint32_t CTTRIG;
	volatile uint32_t FSCTL;
	volatile uint32_t GCTL;
};

/* Extended COUNTERREGS: covers CC control/action/filter registers for PWM */
struct mspm_gptimer_counter_regs {
	volatile uint32_t CTR;         /* +0x00 (0x1800) */
	volatile uint32_t CTRCTL;      /* +0x04 */
	volatile uint32_t LOAD;        /* +0x08 */
	uint32_t RESERVED0;            /* +0x0C */
	volatile uint32_t CC_01[2];    /* +0x10 — CC ch0/ch1 */
	volatile uint32_t CC_23[2];    /* +0x18 — CC ch2/ch3 */
	volatile uint32_t CC_45[2];    /* +0x20 */
	uint32_t RESERVED1[2];         /* +0x28 */
	volatile uint32_t CCCTL_01[2]; /* +0x30 (0x1830) */
	volatile uint32_t CCCTL_23[2]; /* +0x38 */
	volatile uint32_t CCCTL_45[2]; /* +0x40 */
	uint32_t RESERVED2[2];         /* +0x48 */
	volatile uint32_t OCTL_01[2];  /* +0x50 (0x1850) */
	volatile uint32_t OCTL_23[2];  /* +0x58 */
	uint32_t RESERVED3[4];         /* +0x60 */
	volatile uint32_t CCACT_01[2]; /* +0x70 (0x1870) */
	volatile uint32_t CCACT_23[2]; /* +0x78 */
	volatile uint32_t IFCTL_01[2]; /* +0x80 (0x1880) */
	volatile uint32_t IFCTL_23[2]; /* +0x88 */
};

struct mspm_gptimer_regs {
	uint32_t RESERVED0[256];
	volatile uint32_t FSUB_0;
	volatile uint32_t FSUB_1;
	uint32_t RESERVED1[15];
	volatile uint32_t FPUB_0;
	volatile uint32_t FPUB_1;
	uint32_t RESERVED2[237];
	struct mspm_gptimer_gprcm GPRCM; /* 0x800 */
	uint32_t RESERVED3[506];
	volatile uint32_t CLKDIV; /* 0x1000 */
	uint32_t RESERVED4;
	volatile uint32_t CLKSEL; /* 0x1008 */
	uint32_t RESERVED5[3];
	volatile uint32_t PDBGCTL;
	uint32_t RESERVED6;
	struct mspm_gptimer_int CPU_INT; /* 0x1020 */
	uint32_t RESERVED7;
	struct mspm_gptimer_int GEN_EVENT0; /* 0x1050 */
	uint32_t RESERVED8;
	struct mspm_gptimer_int GEN_EVENT1; /* 0x1080 */
	uint32_t RESERVED9[13];
	volatile uint32_t EVT_MODE;
	uint32_t RESERVED10[6];
	volatile uint32_t DESC;
	struct mspm_gptimer_common COMMONREGS; /* 0x1100 */
	uint32_t RESERVED11[438];
	struct mspm_gptimer_counter_regs COUNTERREGS; /* 0x1800 */
};

BUILD_ASSERT(offsetof(struct mspm_gptimer_regs, GPRCM) == 0x0800U);
BUILD_ASSERT(offsetof(struct mspm_gptimer_regs, CLKDIV) == 0x1000U);
BUILD_ASSERT(offsetof(struct mspm_gptimer_regs, CPU_INT) == 0x1020U);
BUILD_ASSERT(offsetof(struct mspm_gptimer_regs, COMMONREGS) == 0x1100U);
BUILD_ASSERT(offsetof(struct mspm_gptimer_regs, COUNTERREGS) == 0x1800U);
BUILD_ASSERT(offsetof(struct mspm_gptimer_counter_regs, CCCTL_01) == 0x30U);
BUILD_ASSERT(offsetof(struct mspm_gptimer_counter_regs, CCACT_01) == 0x70U);
BUILD_ASSERT(offsetof(struct mspm_gptimer_counter_regs, IFCTL_01) == 0x80U);

/* GPRCM.PWREN */
#ifndef GPTIMER_PWREN_KEY_UNLOCK_W
#define GPTIMER_PWREN_KEY_UNLOCK_W 0x26000000U
#endif
#ifndef GPTIMER_PWREN_ENABLE_ENABLE
#define GPTIMER_PWREN_ENABLE_ENABLE 0x00000001U
#endif

/* GPRCM.RSTCTL */
#ifndef GPTIMER_RSTCTL_KEY_UNLOCK_W
#define GPTIMER_RSTCTL_KEY_UNLOCK_W 0xB1000000U
#endif
#ifndef GPTIMER_RSTCTL_RESETSTKYCLR_CLR
#define GPTIMER_RSTCTL_RESETSTKYCLR_CLR 0x00000002U
#endif
#ifndef GPTIMER_RSTCTL_RESETASSERT_ASSERT
#define GPTIMER_RSTCTL_RESETASSERT_ASSERT 0x00000001U
#endif

/* COUNTERREGS.CTRCTL */
#ifndef GPTIMER_CTRCTL_EN_ENABLED
#define GPTIMER_CTRCTL_EN_ENABLED 0x00000001U
#endif
#ifndef GPTIMER_CTRCTL_EN_MASK
#define GPTIMER_CTRCTL_EN_MASK 0x00000001U
#endif
#ifndef GPTIMER_CTRCTL_REPEAT_REPEAT_1
#define GPTIMER_CTRCTL_REPEAT_REPEAT_1 0x00000002U
#endif
#ifndef GPTIMER_CTRCTL_CM_DOWN
#define GPTIMER_CTRCTL_CM_DOWN 0x00000000U
#endif
#ifndef GPTIMER_CTRCTL_CM_UP_DOWN
#define GPTIMER_CTRCTL_CM_UP_DOWN 0x00000010U
#endif
#ifndef GPTIMER_CTRCTL_CM_UP
#define GPTIMER_CTRCTL_CM_UP 0x00000020U
#endif
#ifndef GPTIMER_CTRCTL_CVAE_ZEROVAL
#define GPTIMER_CTRCTL_CVAE_ZEROVAL 0x20000000U
#endif

/* COMMONREGS */
#ifndef GPTIMER_CCLKCTL_CLKEN_ENABLED
#define GPTIMER_CCLKCTL_CLKEN_ENABLED 0x00000001U
#endif
#define GPTIMER_CCPD_OUTPUT_MASK(ch) BIT(ch)

/* CCCTL_01/23 */
#ifndef GPTIMER_CCCTL_01_COC_COMPARE
#define GPTIMER_CCCTL_01_COC_COMPARE 0x00000000U
#endif
#ifndef GPTIMER_CCCTL_01_COC_CAPTURE
#define GPTIMER_CCCTL_01_COC_CAPTURE 0x00020000U
#endif
#ifndef GPTIMER_CCCTL_01_CCUPD_ZERO_EVT
#define GPTIMER_CCCTL_01_CCUPD_ZERO_EVT 0x00040000U
#endif
#ifndef GPTIMER_CCCTL_01_CCUPD_ZERO_LOAD_EVT
#define GPTIMER_CCCTL_01_CCUPD_ZERO_LOAD_EVT 0x00100000U
#endif
#ifndef GPTIMER_CCCTL_01_CCOND_CC_TRIG_RISE
#define GPTIMER_CCCTL_01_CCOND_CC_TRIG_RISE 0x00000001U
#endif
#ifndef GPTIMER_CCCTL_01_CCOND_CC_TRIG_FALL
#define GPTIMER_CCCTL_01_CCOND_CC_TRIG_FALL 0x00000002U
#endif

/* CCACT_01/23 — output pin actions */
#ifndef GPTIMER_CCACT_01_ZACT_CCP_HIGH
#define GPTIMER_CCACT_01_ZACT_CCP_HIGH 0x00000001U
#endif
#ifndef GPTIMER_CCACT_01_LACT_CCP_HIGH
#define GPTIMER_CCACT_01_LACT_CCP_HIGH 0x00000008U
#endif
#ifndef GPTIMER_CCACT_01_LACT_CCP_LOW
#define GPTIMER_CCACT_01_LACT_CCP_LOW 0x00000010U
#endif
#ifndef GPTIMER_CCACT_01_CDACT_CCP_LOW
#define GPTIMER_CCACT_01_CDACT_CCP_LOW 0x00000080U
#endif
#ifndef GPTIMER_CCACT_01_CDACT_CCP_HIGH
#define GPTIMER_CCACT_01_CDACT_CCP_HIGH 0x00000040U
#endif
#ifndef GPTIMER_CCACT_01_CUACT_CCP_LOW
#define GPTIMER_CCACT_01_CUACT_CCP_LOW 0x00000400U
#endif
#ifndef GPTIMER_CCACT_01_CUACT_CCP_HIGH
#define GPTIMER_CCACT_01_CUACT_CCP_HIGH 0x00000200U
#endif

/* IFCTL_01/23 ISEL */
#ifndef GPTIMER_IFCTL_01_ISEL_CCPX_INPUT
#define GPTIMER_IFCTL_01_ISEL_CCPX_INPUT 0x00000000U
#endif
#ifndef GPTIMER_IFCTL_01_ISEL_CCPX_INPUT_PAIR
#define GPTIMER_IFCTL_01_ISEL_CCPX_INPUT_PAIR 0x00000001U
#endif
#ifndef GPTIMER_IFCTL_01_ISEL_CCP0_INPUT
#define GPTIMER_IFCTL_01_ISEL_CCP0_INPUT 0x00000002U
#endif

/* CPU_INT interrupt bits — same position in IMASK, RIS, ICLR */
#define GPTIMER_INT_ZERO_BIT     BIT(0) /* Z: zero/underflow event */
#define GPTIMER_INT_CCD_MASK(ch) BIT(4U + (ch))

#define MSPM_PWM_CC_MAX 4U

/* Replaces DL_TIMER_PWM_MODE */
enum pwm_mspm_mode {
	PWM_MSPM_EDGE_ALIGN,    /* down-count: LACT=LOW, CDACT=HIGH */
	PWM_MSPM_EDGE_ALIGN_UP, /* up-count:   ZACT=HIGH, CUACT=LOW */
	PWM_MSPM_CENTER_ALIGN,  /* up-down:    CUACT=HIGH, CDACT=LOW */
};

#ifdef CONFIG_PWM_CAPTURE
enum pwm_mspm_cap_mode {
	PWM_MSPM_CAP_EDGE_TIME,
	PWM_MSPM_CAP_PULSE_WIDTH,
};
#endif

struct pwm_mspm_config {
	struct mspm_gptimer_regs *base;
	const struct device *clock_dev;
	const struct pinctrl_dev_config *pincfg;
	struct mspm0_sys_clock clock_subsys;
	uint32_t clk_sel;
	uint32_t clk_div_reg; /* CLKDIV value: ti_clk_div - 1 */
	uint8_t prescaler;
	enum pwm_mspm_mode mode;
	uint8_t cc_idx[MSPM_PWM_CC_MAX];
	uint8_t cc_idx_cnt;
	bool is_capture;
#ifdef CONFIG_PWM_CAPTURE
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct pwm_mspm_data {
	uint32_t period;
	uint32_t pulse[MSPM_PWM_CC_MAX];
	uint32_t freq_hz;
	struct k_mutex lock;
#ifdef CONFIG_PWM_CAPTURE
	uint32_t last_sample;
	enum pwm_mspm_cap_mode cap_mode;
	pwm_capture_callback_handler_t callback;
	pwm_flags_t flags;
	void *user_data;
	bool is_synced;
	bool armed;
#endif
};

/* Register helpers */

static inline void mspm_pwm_write_cc(struct mspm_gptimer_regs *base, uint8_t ch, uint32_t val)
{
	if (ch < 2U) {
		base->COUNTERREGS.CC_01[ch] = val;
	} else {
		base->COUNTERREGS.CC_23[ch - 2U] = val;
	}
}

static inline uint32_t mspm_pwm_read_cc(struct mspm_gptimer_regs *base, uint8_t ch)
{
	if (ch < 2U) {
		return base->COUNTERREGS.CC_01[ch];
	}
	return base->COUNTERREGS.CC_23[ch - 2U];
}

static inline void mspm_pwm_write_ccctl(struct mspm_gptimer_regs *base, uint8_t ch, uint32_t val)
{
	if (ch < 2U) {
		base->COUNTERREGS.CCCTL_01[ch] = val;
	} else {
		base->COUNTERREGS.CCCTL_23[ch - 2U] = val;
	}
}

static inline void mspm_pwm_write_ccact(struct mspm_gptimer_regs *base, uint8_t ch, uint32_t val)
{
	if (ch < 2U) {
		base->COUNTERREGS.CCACT_01[ch] = val;
	} else {
		base->COUNTERREGS.CCACT_23[ch - 2U] = val;
	}
}

static inline void mspm_pwm_write_octl(struct mspm_gptimer_regs *base, uint8_t ch, uint32_t val)
{
	if (ch < 2U) {
		base->COUNTERREGS.OCTL_01[ch] = val;
	} else {
		base->COUNTERREGS.OCTL_23[ch - 2U] = val;
	}
}

static inline void mspm_pwm_write_ifctl(struct mspm_gptimer_regs *base, uint8_t ch, uint32_t val)
{
	if (ch < 2U) {
		base->COUNTERREGS.IFCTL_01[ch] = val;
	} else {
		base->COUNTERREGS.IFCTL_23[ch - 2U] = val;
	}
}

/* PWM output helpers */

/*
 * DOWN-counting EDGE_ALIGN: LACT (reload) and CDACT (CC match) toggle the
 * pin so it's HIGH for exactly `pulse` ticks. At the 0% and 100% duty
 * boundaries CC coincides with LOAD/reload, so LACT and CDACT would fire
 * on the same tick — pick a single fixed action instead of toggling to
 * avoid a spurious 1-tick glitch at either extreme.
 */
static inline uint32_t mspm_pwm_edge_align_ccact(uint32_t pulse, uint32_t load)
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
 * UP-DOWN counting CENTER_ALIGN: LOAD holds period/2 (up-down counting
 * doubles the effective tick count). CC must be the symmetric offset from
 * the peak that gives `pulse` total ticks of HIGH time: load - pulse/2.
 */
static inline uint32_t mspm_pwm_center_align_cc(uint32_t load, uint32_t pulse)
{
	return load - (pulse / 2U);
}

static void mspm_pwm_setup_cc_chan(struct mspm_gptimer_regs *base, uint8_t ch,
				   enum pwm_mspm_mode mode, uint32_t pulse)
{
	uint32_t ccact;
	uint32_t ccupd;
	uint32_t cc_val = pulse;

	switch (mode) {
	case PWM_MSPM_EDGE_ALIGN:
		ccact = mspm_pwm_edge_align_ccact(pulse, base->COUNTERREGS.LOAD);
		ccupd = GPTIMER_CCCTL_01_CCUPD_ZERO_EVT;
		break;
	case PWM_MSPM_EDGE_ALIGN_UP:
		ccact = GPTIMER_CCACT_01_ZACT_CCP_HIGH | GPTIMER_CCACT_01_CUACT_CCP_LOW;
		ccupd = GPTIMER_CCCTL_01_CCUPD_ZERO_EVT;
		break;
	default: /* PWM_MSPM_CENTER_ALIGN */
		ccact = GPTIMER_CCACT_01_CUACT_CCP_HIGH | GPTIMER_CCACT_01_CDACT_CCP_LOW;
		ccupd = GPTIMER_CCCTL_01_CCUPD_ZERO_LOAD_EVT;
		cc_val = mspm_pwm_center_align_cc(base->COUNTERREGS.LOAD, pulse);
		break;
	}

	mspm_pwm_write_ccact(base, ch, ccact);
	mspm_pwm_write_ccctl(base, ch, GPTIMER_CCCTL_01_COC_COMPARE | ccupd);
	mspm_pwm_write_octl(base, ch, 0U);
	mspm_pwm_write_ifctl(base, ch, GPTIMER_IFCTL_01_ISEL_CCPX_INPUT);
	mspm_pwm_write_cc(base, ch, cc_val);
}

static void mspm_pwm_setup_output(const struct pwm_mspm_config *config, struct pwm_mspm_data *data)
{
	struct mspm_gptimer_regs *base = config->base;
	uint32_t ctrctl;
	uint32_t ccpd_mask = 0U;
	int i;

	switch (config->mode) {
	case PWM_MSPM_EDGE_ALIGN:
		ctrctl = GPTIMER_CTRCTL_CM_DOWN;
		break;
	case PWM_MSPM_EDGE_ALIGN_UP:
		ctrctl = GPTIMER_CTRCTL_CM_UP | GPTIMER_CTRCTL_CVAE_ZEROVAL;
		break;
	default: /* PWM_MSPM_CENTER_ALIGN */
		ctrctl = GPTIMER_CTRCTL_CM_UP_DOWN;
		break;
	}

	if (config->mode == PWM_MSPM_CENTER_ALIGN) {
		/* Up-down counting doubles the effective tick count; LOAD
		 * must hold period/2 to match pwm_mspm_set_cycles() and the
		 * CC offset math in mspm_pwm_setup_cc_chan().
		 */
		data->period >>= 1;
	}
	base->COUNTERREGS.LOAD = data->period;

	for (i = 0; i < config->cc_idx_cnt; i++) {
		uint8_t ch = config->cc_idx[i];

		mspm_pwm_setup_cc_chan(base, ch, config->mode, data->pulse[i]);
		ccpd_mask |= GPTIMER_CCPD_OUTPUT_MASK(ch);
	}

	base->COMMONREGS.CCPD |= ccpd_mask;
	base->COMMONREGS.CCLKCTL = GPTIMER_CCLKCTL_CLKEN_ENABLED;
	base->COUNTERREGS.CTRCTL =
		ctrctl | GPTIMER_CTRCTL_REPEAT_REPEAT_1 | GPTIMER_CTRCTL_EN_ENABLED;
}

/* Zephyr PWM API */

static int pwm_mspm_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
			       uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_mspm_config *config = dev->config;
	struct pwm_mspm_data *data = dev->data;
	struct mspm_gptimer_regs *base = config->base;
	uint32_t period;

	if (channel >= config->cc_idx_cnt) {
		LOG_ERR("Invalid channel %u", channel);
		return -EINVAL;
	}

	if (period_cycles > UINT16_MAX) {
		LOG_ERR("period cycles exceeds 16-bit timer limit");
		return -ENOTSUP;
	}

	period = (config->mode == PWM_MSPM_CENTER_ALIGN) ? (period_cycles >> 1) : period_cycles;

	if (flags & PWM_POLARITY_INVERTED) {
		pulse_cycles = period_cycles - pulse_cycles;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->pulse[channel] = pulse_cycles;
	data->period = period;

	base->COUNTERREGS.LOAD = period;

	if (config->mode == PWM_MSPM_CENTER_ALIGN) {
		mspm_pwm_write_cc(base, config->cc_idx[channel],
				  mspm_pwm_center_align_cc(period, pulse_cycles));
	} else {
		mspm_pwm_write_cc(base, config->cc_idx[channel], pulse_cycles);
	}

	if (config->mode == PWM_MSPM_EDGE_ALIGN) {
		/* Re-evaluate the 0%/100% CDACT special case; see
		 * mspm_pwm_edge_align_ccact() for why this is needed.
		 */
		mspm_pwm_write_ccact(base, config->cc_idx[channel],
				     mspm_pwm_edge_align_ccact(pulse_cycles, period));
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int pwm_mspm_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	ARG_UNUSED(channel);

	if (cycles == NULL) {
		return -EINVAL;
	}

	*cycles = ((const struct pwm_mspm_data *)dev->data)->freq_hz;
	return 0;
}

/* Capture support */

#ifdef CONFIG_PWM_CAPTURE

/*
 * Which channel triggers the capture interrupt depends on cap_mode, not on
 * the requested capture TYPE: PULSE_WIDTH always triggers on the rise
 * channel (cc_idx[0]^1) regardless of PERIOD/PULSE/BOTH, since that's the
 * only channel actually wired to fire an interrupt (mspm_pwm_setup_capture()
 * never arms cc_idx[0]'s own interrupt in PULSE_WIDTH mode). Gating on the
 * TYPE flags instead of cap_mode would arm the wrong, unconfigured channel
 * for EDGE_TIME mode whenever PERIOD/PULSE/BOTH is requested there.
 */
static uint32_t mspm_pwm_cap_intr_mask(const struct pwm_mspm_config *config,
				       const struct pwm_mspm_data *data)
{
	if (data->cap_mode == PWM_MSPM_CAP_PULSE_WIDTH) {
		return GPTIMER_INT_CCD_MASK(config->cc_idx[0] ^ 1U) | GPTIMER_INT_ZERO_BIT;
	}
	return GPTIMER_INT_CCD_MASK(config->cc_idx[0]);
}

static void mspm_pwm_setup_capture(const struct device *dev, const struct pwm_mspm_config *config,
				   struct pwm_mspm_data *data)
{
	struct mspm_gptimer_regs *base = config->base;

	base->COUNTERREGS.LOAD = data->period;

	if (data->cap_mode == PWM_MSPM_CAP_EDGE_TIME) {
		uint8_t ch = config->cc_idx[0];
		uint32_t isel = (ch & 1U) ? GPTIMER_IFCTL_01_ISEL_CCPX_INPUT
					  : GPTIMER_IFCTL_01_ISEL_CCP0_INPUT;

		mspm_pwm_write_ccctl(base, ch,
				     GPTIMER_CCCTL_01_COC_CAPTURE |
					     GPTIMER_CCCTL_01_CCOND_CC_TRIG_RISE);
		mspm_pwm_write_ifctl(base, ch, isel);
		base->COMMONREGS.CCPD &= ~GPTIMER_CCPD_OUTPUT_MASK(ch);
		base->COUNTERREGS.CTRCTL = GPTIMER_CTRCTL_CM_DOWN | GPTIMER_CTRCTL_REPEAT_REPEAT_1;
	} else {
		uint8_t ch_fall = config->cc_idx[0];
		uint8_t ch_rise = ch_fall ^ 1U;
		uint32_t isel_fall = (ch_fall & 1U) ? GPTIMER_IFCTL_01_ISEL_CCPX_INPUT
						    : GPTIMER_IFCTL_01_ISEL_CCP0_INPUT;

		mspm_pwm_write_ccctl(base, ch_fall,
				     GPTIMER_CCCTL_01_COC_CAPTURE |
					     GPTIMER_CCCTL_01_CCOND_CC_TRIG_FALL);
		mspm_pwm_write_ifctl(base, ch_fall, isel_fall);

		mspm_pwm_write_ccctl(base, ch_rise,
				     GPTIMER_CCCTL_01_COC_CAPTURE |
					     GPTIMER_CCCTL_01_CCOND_CC_TRIG_RISE);
		mspm_pwm_write_ifctl(base, ch_rise, GPTIMER_IFCTL_01_ISEL_CCPX_INPUT_PAIR);

		base->COMMONREGS.CCPD &=
			~(GPTIMER_CCPD_OUTPUT_MASK(ch_fall) | GPTIMER_CCPD_OUTPUT_MASK(ch_rise));
		base->COUNTERREGS.CTRCTL = GPTIMER_CTRCTL_CM_DOWN | GPTIMER_CTRCTL_REPEAT_REPEAT_1;
	}

	base->COMMONREGS.CCLKCTL = GPTIMER_CCLKCTL_CLKEN_ENABLED;
	config->irq_config_func(dev);
}

static int pwm_mspm_configure_capture(const struct device *dev, uint32_t channel, pwm_flags_t flags,
				      pwm_capture_callback_handler_t cb, void *user_data)
{
	const struct pwm_mspm_config *config = dev->config;
	struct pwm_mspm_data *data = dev->data;
	uint32_t intr_mask;

	if (!config->is_capture || channel != 0) {
		LOG_ERR("Invalid capture channel %u", channel);
		return -EINVAL;
	}

	intr_mask = mspm_pwm_cap_intr_mask(config, data);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (config->base->CPU_INT.IMASK & intr_mask) {
		LOG_ERR("Channel %u busy", channel);
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	data->flags = flags;
	data->callback = cb;
	data->user_data = user_data;

	if (data->cap_mode == PWM_MSPM_CAP_PULSE_WIDTH) {
		struct mspm_gptimer_regs *base = config->base;
		uint8_t ch_fall = config->cc_idx[0];
		uint8_t ch_rise = ch_fall ^ 1U;
		bool inverted = flags & PWM_POLARITY_INVERTED;

		/*
		 * "pulse" is always computed as (time of ch_rise's edge) -
		 * (time of ch_fall's edge). Under normal polarity that is
		 * RISE-to-FALL, i.e. the HIGH duration. Under inverted
		 * polarity the caller expects "pulse" to mean the LOW
		 * duration instead, so swap which physical edge each
		 * channel captures: ch_fall now triggers on RISE and
		 * ch_rise now triggers on FALL, making the same
		 * last_sample-minus-cc0 formula yield FALL-to-RISE (the
		 * LOW duration). Mirrors the edge-swap approach used by
		 * pwm_stm32's init_capture_channels().
		 */
		mspm_pwm_write_ccctl(base, ch_fall,
				     GPTIMER_CCCTL_01_COC_CAPTURE |
					     (inverted ? GPTIMER_CCCTL_01_CCOND_CC_TRIG_RISE
						       : GPTIMER_CCCTL_01_CCOND_CC_TRIG_FALL));
		mspm_pwm_write_ccctl(base, ch_rise,
				     GPTIMER_CCCTL_01_COC_CAPTURE |
					     (inverted ? GPTIMER_CCCTL_01_CCOND_CC_TRIG_FALL
						       : GPTIMER_CCCTL_01_CCOND_CC_TRIG_RISE));
	}

	k_mutex_unlock(&data->lock);
	return 0;
}

static int pwm_mspm_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mspm_config *config = dev->config;
	struct pwm_mspm_data *data = dev->data;
	struct mspm_gptimer_regs *base = config->base;
	uint32_t intr_mask;

	if (!config->is_capture || channel != 0) {
		LOG_ERR("Invalid capture channel %u", channel);
		return -EINVAL;
	}

	if (!data->callback) {
		LOG_ERR("No capture callback configured");
		return -EINVAL;
	}

	intr_mask = mspm_pwm_cap_intr_mask(config, data);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (base->CPU_INT.IMASK & intr_mask) {
		LOG_ERR("Channel %u busy", channel);
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	/*
	 * Re-baseline on every arm, not just after disable_capture(): a
	 * caller may legitimately re-enable without disabling first (e.g.
	 * back-to-back single-shot captures), and the CC registers may hold
	 * a stale capture from whatever ran before this arm.
	 */
	data->is_synced = false;
	data->armed = false;

	base->COUNTERREGS.CTR = data->period;
	base->COUNTERREGS.CTRCTL |= GPTIMER_CTRCTL_EN_ENABLED;
	base->CPU_INT.ICLR = intr_mask;
	base->CPU_INT.IMASK |= intr_mask;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int pwm_mspm_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mspm_config *config = dev->config;
	struct pwm_mspm_data *data = dev->data;
	struct mspm_gptimer_regs *base = config->base;
	uint32_t intr_mask;

	if (!config->is_capture || channel != 0) {
		LOG_ERR("Invalid capture channel %u", channel);
		return -EINVAL;
	}

	intr_mask = mspm_pwm_cap_intr_mask(config, data);

	k_mutex_lock(&data->lock, K_FOREVER);

	base->CPU_INT.IMASK &= ~intr_mask;
	base->COUNTERREGS.CTRCTL &= ~GPTIMER_CTRCTL_EN_MASK;
	data->is_synced = false;
	data->armed = false;

	k_mutex_unlock(&data->lock);
	return 0;
}

static void mspm_pwm_cc_isr(const struct device *dev)
{
	const struct pwm_mspm_config *config = dev->config;
	struct pwm_mspm_data *data = dev->data;
	struct mspm_gptimer_regs *base = config->base;
	uint32_t raw_ris;
	uint32_t ris;
	uint32_t cc0 = 0;
	uint32_t cc1 = 0;
	uint32_t period;
	uint32_t pulse;

	/*
	 * Clear every pending bit (raw_ris), not just the ones relevant to
	 * this capture (ris), so coincidental compare matches on unrelated
	 * CC channels (e.g. CC4/CC5 defaulting to COMPARE mode with CC=0)
	 * don't linger set in RIS indefinitely.
	 */
	raw_ris = base->CPU_INT.RIS;
	ris = raw_ris & mspm_pwm_cap_intr_mask(config, data);
	base->CPU_INT.ICLR = raw_ris;

	if (!ris) {
		return;
	}

	if (ris & GPTIMER_INT_ZERO_BIT) {
		if (!(data->flags & PWM_CAPTURE_MODE_CONTINUOUS)) {
			if (data->callback) {
				data->callback(dev, 0, 0, 0, -ERANGE, data->user_data);
				base->COUNTERREGS.CTRCTL &= ~GPTIMER_CTRCTL_EN_MASK;
			}
			return;
		}
		/*
		 * Continuous mode: ZERO is just the free-running counter's
		 * periodic wrap, not a capture error. A CC edge can land in
		 * the same ISR entry (the wrap period doesn't divide evenly
		 * into the PWM period) — fall through to process it instead
		 * of dropping it.
		 */
		if (!(ris & ~GPTIMER_INT_ZERO_BIT)) {
			return;
		}
	}

	if (data->cap_mode == PWM_MSPM_CAP_PULSE_WIDTH) {
		/*
		 * cc1 (rise-edge capture) is the interrupt trigger source for
		 * PULSE_WIDTH mode and the sync/period/pulse anchor for every
		 * capture type, not just PWM_CAPTURE_TYPE_PERIOD. Gating this
		 * read on the PERIOD flag left cc1/last_sample stuck at 0 for
		 * PULSE-only captures, making period compute to 0 and the
		 * capture callback never fire (timeout).
		 */
		cc1 = mspm_pwm_read_cc(base, config->cc_idx[0] ^ 1U);
	}

	if (!data->is_synced && data->cap_mode != PWM_MSPM_CAP_EDGE_TIME) {
		data->last_sample = cc1;
		data->is_synced = true;
		return;
	}

	if ((data->flags & PWM_CAPTURE_TYPE_PULSE) || data->cap_mode == PWM_MSPM_CAP_EDGE_TIME) {
		cc0 = mspm_pwm_read_cc(base, config->cc_idx[0]);
	}

	if (data->cap_mode == PWM_MSPM_CAP_PULSE_WIDTH && !data->armed) {
		/*
		 * First full interval after arming can straddle a leftover
		 * edge from whatever waveform ran before capture was armed
		 * (e.g. pwm_set() right before enable_capture(), no settle
		 * delay). Discard it and re-baseline instead of reporting
		 * a bogus period/pulse.
		 */
		data->armed = true;
		data->last_sample = cc1;
		return;
	}

	if (!(data->flags & PWM_CAPTURE_MODE_CONTINUOUS)) {
		base->COUNTERREGS.CTRCTL &= ~GPTIMER_CTRCTL_EN_MASK;
		data->is_synced = false;
	}

	period = (data->last_sample - cc1) & 0xFFFFU;
	pulse = (data->last_sample - cc0) & 0xFFFFU;

	/*
	 * Residual defensive guard: cc0/cc1 should structurally never
	 * produce pulse > period once armed (see the `armed` discard
	 * above, which handles the one confirmed cause of this). Kept as
	 * a graceful fallback for any unconfirmed capture-block timing
	 * edge case rather than reporting a nonsensical pulse width.
	 */
	if (pulse > period) {
		pulse -= period;
	}

	if (data->callback && period) {
		data->callback(dev, 0, period, pulse, 0, data->user_data);
	}

	data->last_sample = cc1;
}

#endif /* CONFIG_PWM_CAPTURE */

static int pwm_mspm_init(const struct device *dev)
{
	const struct pwm_mspm_config *config = dev->config;
	struct pwm_mspm_data *data = dev->data;
	struct mspm_gptimer_regs *base = config->base;
	uint32_t clock_rate;
	int ret;

	k_mutex_init(&data->lock);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)&config->clock_subsys, &clock_rate);
	if (ret != 0) {
		LOG_ERR("clk get rate err %d", ret);
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	base->GPRCM.RSTCTL = GPTIMER_RSTCTL_KEY_UNLOCK_W | GPTIMER_RSTCTL_RESETSTKYCLR_CLR |
			     GPTIMER_RSTCTL_RESETASSERT_ASSERT;
	base->GPRCM.PWREN = GPTIMER_PWREN_KEY_UNLOCK_W | GPTIMER_PWREN_ENABLE_ENABLE;
	k_busy_wait(1U);

	base->CLKSEL = config->clk_sel;
	base->CLKDIV = config->clk_div_reg;
	base->COMMONREGS.CPS = config->prescaler;

	data->freq_hz =
		clock_rate / ((config->clk_div_reg + 1U) * ((uint32_t)config->prescaler + 1U));

	base->CPU_INT.IMASK = 0U;
	base->CPU_INT.ICLR = 0xFFFFFFFFU;

	if (config->is_capture) {
#ifdef CONFIG_PWM_CAPTURE
		mspm_pwm_setup_capture(dev, config, data);
#endif
	} else {
		mspm_pwm_setup_output(config, data);
	}

	return 0;
}

static DEVICE_API(pwm, pwm_mspm_driver_api) = {
	.set_cycles = pwm_mspm_set_cycles,
	.get_cycles_per_sec = pwm_mspm_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_mspm_configure_capture,
	.enable_capture = pwm_mspm_enable_capture,
	.disable_capture = pwm_mspm_disable_capture,
#endif
};

/* Device instantiation */

#ifdef CONFIG_PWM_CAPTURE
#define MSPM_PWM_IRQ_REGISTER(n)                                                                   \
	static void mspm_pwm_##n##_irq_register(const struct device *dev)                          \
	{                                                                                          \
		const struct pwm_mspm_config *config = dev->config;                                \
		if (!config->is_capture) {                                                         \
			return;                                                                    \
		}                                                                                  \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)), DT_IRQ(DT_INST_PARENT(n), priority),       \
			    mspm_pwm_cc_isr, DEVICE_DT_INST_GET(n), 0);                            \
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));                                            \
	}
#else
#define MSPM_PWM_IRQ_REGISTER(n)
#endif

/* Only output-capable instances (no ti,cc-mode) skip ISR registration. */
#define MSPM_PWM_IRQ_REGISTER_IF_CAPTURE(n)                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_cc_mode), (MSPM_PWM_IRQ_REGISTER(n)), ())

#define MSPM_PWM_MODE(tok) _CONCAT(PWM_MSPM_, tok)
#define MSPM_CAP_MODE(tok) _CONCAT(PWM_MSPM_CAP_, tok)

#define MSPM_CC_IDX_ARRAY(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx),

#define PWM_DEVICE_INIT_MSPM(n)                                                                    \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, ti_cc_index) <= MSPM_PWM_CC_MAX,                          \
		     "ti,cc-index exceeds hardware maximum of 4");                                 \
	BUILD_ASSERT(DT_PROP(DT_INST_PARENT(n), ti_clk_div) >= 1,                                  \
		     "ti,clk-div must be >= 1 (0 underflows clk_div_reg)");                        \
                                                                                                   \
	static struct pwm_mspm_data pwm_mspm_data_##n = {                                          \
		.period = DT_PROP(DT_DRV_INST(n), ti_period),                                      \
		IF_ENABLED(CONFIG_PWM_CAPTURE,					\
		(COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_cc_mode),	\
			(.cap_mode = MSPM_CAP_MODE(				\
				DT_STRING_TOKEN(DT_DRV_INST(n), ti_cc_mode)),),\
			()))) };                                 \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	MSPM_PWM_IRQ_REGISTER_IF_CAPTURE(n)                                                        \
                                                                                                   \
	static const struct pwm_mspm_config pwm_mspm_config_##n = {                                \
		.base = (struct mspm_gptimer_regs *)DT_REG_ADDR(DT_INST_PARENT(n)),                \
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
		.mode = COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n),		\
						     ti_pwm_mode),		\
			(MSPM_PWM_MODE(					\
				DT_STRING_TOKEN(DT_DRV_INST(n), ti_pwm_mode))),\
			(PWM_MSPM_EDGE_ALIGN)),                                                   \
			 .cc_idx = {DT_INST_FOREACH_PROP_ELEM(n, ti_cc_index, MSPM_CC_IDX_ARRAY)}, \
			 .cc_idx_cnt = DT_INST_PROP_LEN(n, ti_cc_index),                           \
			 .is_capture = DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_cc_mode),               \
			 IF_ENABLED(CONFIG_PWM_CAPTURE,					\
		(.irq_config_func =						\
			COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n),		\
						     ti_cc_mode),		\
				(mspm_pwm_##n##_irq_register), (NULL)))) };            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, pwm_mspm_init, NULL, &pwm_mspm_data_##n, &pwm_mspm_config_##n,    \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &pwm_mspm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT_MSPM)
