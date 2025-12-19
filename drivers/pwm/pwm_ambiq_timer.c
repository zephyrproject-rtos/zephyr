/*
 * Copyright (c) 2025 Ambiq
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_timer_pwm

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
/* ambiq-sdk includes */
#include <soc.h>

LOG_MODULE_REGISTER(ambiq_timer_pwm, CONFIG_PWM_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_APOLLO4X) || defined(CONFIG_SOC_SERIES_APOLLO5X)
typedef am_hal_timer_config_t pwm_timer_config_t;
#endif

struct pwm_ambiq_timer_data {
	uint32_t cycles;
};

struct pwm_ambiq_timer_config {
	uint32_t timer_num;
	uint32_t clock_sel;
	const struct pinctrl_dev_config *pincfg;
};

static uint32_t get_clock_cycles(uint32_t clock_sel)
{
	uint32_t ret = 0;

	switch (clock_sel) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = 96000000 / (4 << (clock_sel * 2));
		break;
	case 6:
		ret = 1000;
		break;
#if defined(CONFIG_SOC_SERIES_APOLLO4X) ||                                                         \
	defined(CONFIG_SOC_APOLLO510) || defined(CONFIG_SOC_APOLLO510B)
	case 7:
		ret = 500;
		break;
	case 8:
		ret = 31;
		break;
	case 9:
		ret = 1;
		break;
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = 32768 / (1 << (clock_sel - 10));
		break;
	case 16:
		ret = 256;
		break;
	case 17:
		ret = 100;
		break;
#endif /* CONFIG_SOC_SERIES_APOLLO4X || CONFIG_SOC_APOLLO510/B */

#if defined(CONFIG_SOC_APOLLO510) || defined(CONFIG_SOC_APOLLO510B)
	case 18:
		ret = 512;
		break;
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
		ret = 125000000 / (8 << (clock_sel - 19));
		break;
	case 25:
	case 26:
	case 27:
		ret = 48000000 / (1 << (clock_sel - 25));
		break;
#endif /* CONFIG_SOC_APOLLO510/B */

	default:
		ret = 24000000;
		break;
	}

	return ret;
}

static int ambiq_timer_pwm_set_cycles(const struct device *dev, uint32_t channel,
				      uint32_t period_cycles, uint32_t pulse_cycles,
				      pwm_flags_t flags)
{
	const struct pwm_ambiq_timer_config *config = dev->config;

	if (period_cycles == 0) {
		LOG_ERR("period_cycles can not be set to zero");
		return -ENOTSUP;
	}

	if (channel > 1) {
		LOG_ERR("A timer has at most 2 channels");
		return -ENOTSUP;
	}

	if ((channel == 1) && (config->pincfg->states->pin_cnt == 2)) {
		am_hal_timer_output_config(config->pincfg->states->pins[1].pin_num,
					   AM_HAL_TIMER_OUTPUT_TMR0_OUT1 + config->timer_num * 2);
	} else {
		am_hal_timer_output_config(config->pincfg->states->pins[0].pin_num,
					   AM_HAL_TIMER_OUTPUT_TMR0_OUT0 + config->timer_num * 2);
	}

	if (flags & PWM_POLARITY_INVERTED) {
		if (pulse_cycles == 0) {
			/* make pulse cycles greater than period so event never occurs */
			pulse_cycles = period_cycles + 1;
		} else {
			pulse_cycles = period_cycles - pulse_cycles;
		}
	} else {
		if (pulse_cycles == period_cycles) {
			--pulse_cycles;
		} else if (pulse_cycles == 0) {
			pulse_cycles = 1;
		}
	}

	am_hal_timer_clear(config->timer_num);
	am_hal_timer_compare0_set(config->timer_num, period_cycles);
	am_hal_timer_compare1_set(config->timer_num, pulse_cycles);
	am_hal_timer_enable(config->timer_num);
	return 0;
}

static int ambiq_timer_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					      uint64_t *cycles)
{
	struct pwm_ambiq_timer_data *data = dev->data;

	if (channel > 1) {
		LOG_ERR("A timer has at most 2 channels");
		return -ENOTSUP;
	}

	/* cycles of the timer clock */
	*cycles = (uint64_t)data->cycles;

	return 0;
}

static int ambiq_timer_pwm_init(const struct device *dev)
{
	const struct pwm_ambiq_timer_config *config = dev->config;
	struct pwm_ambiq_timer_data *data = dev->data;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	pwm_timer_config_t pwm_timer_config;

	am_hal_timer_default_config_set(&pwm_timer_config);

	pwm_timer_config.eFunction = AM_HAL_TIMER_FN_PWM;
	pwm_timer_config.eInputClock = config->clock_sel;
	data->cycles = get_clock_cycles(config->clock_sel);

	am_hal_timer_config(config->timer_num, &pwm_timer_config);
	am_hal_timer_clear_stop(config->timer_num);
	am_hal_timer_disable(config->timer_num);
	return 0;
}

static DEVICE_API(pwm, pwm_ambiq_timer_driver_api) = {
	.set_cycles = ambiq_timer_pwm_set_cycles,
	.get_cycles_per_sec = ambiq_timer_pwm_get_cycles_per_sec,
};

#define PWM_AMBIQ_TIMER_DEVICE_INIT(n)                                                             \
	BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(DT_INST_PARENT(n)) == 1,                             \
		     "Too many children for Timer!");                                              \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct pwm_ambiq_timer_data pwm_ambiq_timer_data_##n = {                            \
		.cycles = 0,                                                                       \
	};                                                                                         \
	static const struct pwm_ambiq_timer_config pwm_ambiq_timer_config_##n = {                  \
		.timer_num = (DT_REG_ADDR(DT_INST_PARENT(n)) - TIMER_BASE) /                       \
			     DT_REG_SIZE(DT_INST_PARENT(n)),                                       \
		.clock_sel = DT_ENUM_IDX(DT_INST_PARENT(n), clk_source),                           \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n)};                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ambiq_timer_pwm_init, NULL, &pwm_ambiq_timer_data_##n,            \
			      &pwm_ambiq_timer_config_##n, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,  \
			      &pwm_ambiq_timer_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_AMBIQ_TIMER_DEVICE_INIT)
