/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_pwm

#include <errno.h>

#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>

#include <gd32_rcu.h>
#include <gd32_timer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_gd32, CONFIG_PWM_LOG_LEVEL);

/** PWM data. */
struct pwm_gd32_data {
	/** Timer clock (Hz). */
	uint32_t tim_clk;
};

/** PWM configuration. */
struct pwm_gd32_config {
	/** Timer register. */
	uint32_t reg;
	/** Number of channels */
	uint8_t channels;
	/** Flag to indicate if timer has 32-bit counter */
	bool is_32bit;
	/** Flag to indicate if timer is advanced */
	bool is_advanced;
	/** Prescaler. */
	uint16_t prescaler;
	/** RCU peripheral clock. */
	uint32_t rcu_periph_clock;
	/** RCU peripheral reset. */
	uint32_t rcu_periph_reset;
	/** pinctrl configurations. */
	const struct pinctrl_dev_config *pcfg;
};

/** Obtain channel enable bit for the given channel */
#define TIMER_CHCTL2_CHXEN(ch) BIT(4U * (ch))
/** Obtain polarity bit for the given channel */
#define TIMER_CHCTL2_CHXP(ch) BIT(1U + (4U * (ch)))
/** Obtain CHCTL0/1 mask for the given channel (0 or 1) */
#define TIMER_CHCTLX_MSK(ch) (0xFU << (8U * (ch)))

/** Obtain RCU register offset from RCU clock value */
#define RCU_CLOCK_OFFSET(rcu_clock) ((rcu_clock) >> 6U)

/**
 * Obtain the timer clock.
 *
 * @param dev Device instance.
 *
 * @return Timer clock (Hz).
 */
static uint32_t pwm_gd32_get_tim_clk(const struct device *dev)
{
	const struct pwm_gd32_config *config = dev->config;
	uint32_t apb_psc, apb_clk;

	/* obtain APB prescaler value */
	if (RCU_CLOCK_OFFSET(config->rcu_periph_clock) == APB1EN_REG_OFFSET) {
		apb_psc = RCU_CFG0 & RCU_CFG0_APB1PSC;
	} else {
		apb_psc = RCU_CFG0 & RCU_CFG0_APB2PSC;
	}

	switch (apb_psc) {
	case RCU_APB1_CKAHB_DIV2:
		apb_psc = 2U;
		break;
	case RCU_APB1_CKAHB_DIV4:
		apb_psc = 4U;
		break;
	case RCU_APB1_CKAHB_DIV8:
		apb_psc = 8U;
		break;
	case RCU_APB1_CKAHB_DIV16:
		apb_psc = 16U;
		break;
	default:
		apb_psc = 1U;
		break;
	}

	apb_clk = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / apb_psc;

#ifdef RCU_CFG1_TIMERSEL
	/*
	 * The TIMERSEL bit in RCU_CFG1 controls the clock frequency of all the
	 * timers connected to the APB1 and APB2 domains.
	 *
	 * Up to a certain threshold value of APB{1,2} prescaler, timer clock
	 * equals to CK_AHB. This threshold value depends on TIMERSEL setting
	 * (2 if TIMERSEL=0, 4 if TIMERSEL=1). Above threshold, timer clock is
	 * set to a multiple of the APB domain clock CK_APB{1,2} (2 if
	 * TIMERSEL=0, 4 if TIMERSEL=1).
	 */

	/* TIMERSEL = 0 */
	if ((RCU_CFG1 & RCU_CFG1_TIMERSEL) == 0U) {
		if (apb_psc <= 2U) {
			return CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
		}

		return apb_clk * 2U;
	}

	/* TIMERSEL = 1 */
	if (apb_psc <= 4U) {
		return CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	}

	return apb_clk * 4U;
#else
	/*
	 * If the APB prescaler equals 1, the timer clock frequencies are set to
	 * the same frequency as that of the APB domain. Otherwise, they are set
	 * to twice the frequency of the APB domain.
	 */
	if (apb_psc == 1U) {
		return apb_clk;
	}

	return apb_clk * 2U;
#endif /* RCU_CFG1_TIMERSEL */
}

static int pwm_gd32_set_cycles(const struct device *dev, uint32_t channel,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	const struct pwm_gd32_config *config = dev->config;

	if (channel >= config->channels) {
		return -EINVAL;
	}

	/* 16-bit timers can count up to UINT16_MAX */
	if (!config->is_32bit && (period_cycles > UINT16_MAX)) {
		return -ENOTSUP;
	}

	/* disable channel output if period is zero */
	if (period_cycles == 0U) {
		TIMER_CHCTL2(config->reg) &= ~TIMER_CHCTL2_CHXEN(channel);
		return 0;
	}

	/* update polarity */
	if ((flags & PWM_POLARITY_INVERTED) != 0U) {
		TIMER_CHCTL2(config->reg) |= TIMER_CHCTL2_CHXP(channel);
	} else {
		TIMER_CHCTL2(config->reg) &= ~TIMER_CHCTL2_CHXP(channel);
	}

	/* update pulse */
	switch (channel) {
	case 0U:
		TIMER_CH0CV(config->reg) = pulse_cycles;
		break;
	case 1U:
		TIMER_CH1CV(config->reg) = pulse_cycles;
		break;
	case 2U:
		TIMER_CH2CV(config->reg) = pulse_cycles;
		break;
	case 3U:
		TIMER_CH3CV(config->reg) = pulse_cycles;
		break;
	default:
		__ASSERT_NO_MSG(NULL);
		break;
	}

	/* update period */
	TIMER_CAR(config->reg) = period_cycles;

	/* channel not enabled: configure it */
	if ((TIMER_CHCTL2(config->reg) & TIMER_CHCTL2_CHXEN(channel)) == 0U) {
		volatile uint32_t *chctl;

		/* select PWM1 mode, enable OC shadowing */
		if (channel < 2U) {
			chctl = &TIMER_CHCTL0(config->reg);
		} else {
			chctl = &TIMER_CHCTL1(config->reg);
		}

		*chctl &= ~TIMER_CHCTLX_MSK(channel);
		*chctl |= (TIMER_OC_MODE_PWM1 | TIMER_OC_SHADOW_ENABLE) <<
			  (8U * (channel % 2U));

		/* enable channel output */
		TIMER_CHCTL2(config->reg) |= TIMER_CHCTL2_CHXEN(channel);

		/* generate update event (to load shadow values) */
		TIMER_SWEVG(config->reg) |= TIMER_SWEVG_UPG;
	}

	return 0;
}

static int pwm_gd32_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	struct pwm_gd32_data *data = dev->data;
	const struct pwm_gd32_config *config = dev->config;

	*cycles = (uint64_t)(data->tim_clk / (config->prescaler + 1U));

	return 0;
}

static const struct pwm_driver_api pwm_gd32_driver_api = {
	.set_cycles = pwm_gd32_set_cycles,
	.get_cycles_per_sec = pwm_gd32_get_cycles_per_sec,
};

static int pwm_gd32_init(const struct device *dev)
{
	const struct pwm_gd32_config *config = dev->config;
	struct pwm_gd32_data *data = dev->data;
	int ret;

	rcu_periph_clock_enable(config->rcu_periph_clock);

	/* reset timer to its default state */
	rcu_periph_reset_enable(config->rcu_periph_reset);
	rcu_periph_reset_disable(config->rcu_periph_reset);

	/* apply pin configuration */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* cache timer clock value */
	data->tim_clk = pwm_gd32_get_tim_clk(dev);

	/* basic timer operation: edge aligned, up counting, shadowed CAR */
	TIMER_CTL0(config->reg) = TIMER_CKDIV_DIV1 | TIMER_COUNTER_EDGE |
				  TIMER_COUNTER_UP | TIMER_CTL0_ARSE;
	TIMER_PSC(config->reg) = config->prescaler;

	/* enable primary output for advanced timers */
	if (config->is_advanced) {
		TIMER_CCHP(config->reg) |= TIMER_CCHP_POEN;
	}

	/* enable timer counter */
	TIMER_CTL0(config->reg) |= TIMER_CTL0_CEN;

	return 0;
}

#define PWM_GD32_DEFINE(i)						       \
	static struct pwm_gd32_data pwm_gd32_data_##i;			       \
									       \
	PINCTRL_DT_INST_DEFINE(i);					       \
									       \
	static const struct pwm_gd32_config pwm_gd32_config_##i = {	       \
		.reg = DT_REG_ADDR(DT_INST_PARENT(i)),			       \
		.rcu_periph_clock = DT_PROP(DT_INST_PARENT(i),		       \
					    rcu_periph_clock),		       \
		.rcu_periph_reset = DT_PROP(DT_INST_PARENT(i),		       \
					    rcu_periph_reset),		       \
		.prescaler = DT_PROP(DT_INST_PARENT(i), prescaler),	       \
		.channels = DT_PROP(DT_INST_PARENT(i), channels),	       \
		.is_32bit = DT_PROP(DT_INST_PARENT(i), is_32bit),	       \
		.is_advanced = DT_PROP(DT_INST_PARENT(i), is_advanced),	       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),		       \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(i, &pwm_gd32_init, NULL, &pwm_gd32_data_##i,     \
			      &pwm_gd32_config_##i, POST_KERNEL,	       \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	       \
			      &pwm_gd32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_GD32_DEFINE)
