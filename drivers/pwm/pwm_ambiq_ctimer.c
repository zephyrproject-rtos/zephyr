/*
 * Copyright (c) 2025 Ambiq
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_ctimer_pwm

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <am_mcu_apollo.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ambiq_ctimer_pwm, CONFIG_PWM_LOG_LEVEL);

struct pwm_ambiq_ctimer_data {
	uint32_t cycles;
};

struct pwm_ambiq_ctimer_config {
	uint32_t timer_num;
	uint32_t timer_seg;
	uint32_t pwm_type;
	uint32_t clock_sel;
	const struct pinctrl_dev_config *pincfg;
};

static uint32_t get_clock_cycles(uint32_t clock_sel)
{
	uint32_t ret = 0;

	switch (clock_sel) {
	case 1:
		ret = 12000000;
		break;
	case 2:
		ret = 3000000;
		break;
	case 3:
		ret = 187500;
		break;
	case 4:
		ret = 47000;
		break;
	case 5:
		ret = 12000;
		break;
	case 6:
		ret = 32768;
		break;
	case 7:
		ret = 16384;
		break;
	case 8:
		ret = 2048;
		break;
	case 9:
		ret = 256;
		break;
	case 10:
		ret = 512;
		break;
	case 11:
		ret = 32;
		break;
	case 12:
		ret = 1000;
		break;
	case 13:
		ret = 116;
		break;
	case 14:
		ret = 100;
		break;
	case 15: /* todo: check on value */
		ret = 0;
		break;
	case 16:
		ret = 8192;
		break;
	case 17:
		ret = 4096;
		break;
	case 18:
		ret = 1024;
		break;
	default:
		ret = 12000000;
		break;
	}

	return ret;
}

static void start_clock(uint32_t clock_sel)
{
	switch (clock_sel) {
	default:
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);
		break;
	case 6:
	case 7:
	case 8:
	case 9:
	case 14: /* RTC will assume XTAL since LFRC isn't as accurate */
	case 16:
	case 17:
	case 18:
		am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_START, 0);
		break;
	case 10:
	case 11:
	case 12:
	case 13:
		am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_LFRC_START, 0);
		break;
	}
}

static int ambiq_ctimer_pwm_set_cycles(const struct device *dev, uint32_t channel,
				      uint32_t period_cycles, uint32_t pulse_cycles,
				      pwm_flags_t flags)
{
	const struct pwm_ambiq_ctimer_config *config = dev->config;

	if (period_cycles == 0) {
		LOG_ERR("period_cycles can not be set to zero");
		return -ENOTSUP;
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
			period_cycles = 0;
			pulse_cycles = 1;
		}
	}

	uint32_t seg = 0;

	if (config->timer_seg == 0) {
		seg = 0x0000FFFF;
	} else if (config->timer_seg == 1) {
		seg = 0xFFFF0000U;
	} else {
		seg = 0xFFFFFFFFU;
	}

	/* todo: need to check if all of this is required */
	am_hal_ctimer_clear(config->timer_num, seg);
	am_hal_ctimer_period_set(config->timer_num, seg, period_cycles, pulse_cycles);
	am_hal_ctimer_start(config->timer_num, seg);

	return 0;
}

static int ambiq_ctimer_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					      uint64_t *cycles)
{
	struct pwm_ambiq_ctimer_data *data = dev->data;

	/* cycles of the timer clock */
	*cycles = (uint64_t)data->cycles;

	return 0;
}

static int ambiq_ctimer_pwm_init(const struct device *dev)
{
	const struct pwm_ambiq_ctimer_config *config = dev->config;
	struct pwm_ambiq_ctimer_data *data = dev->data;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	uint32_t seg = 0;

	if (config->timer_seg == 0) {
		seg = 0x0000FFFF;
	} else if (config->timer_seg == 1) {
		seg = 0xFFFF0000U;
	} else {
		seg = 0xFFFFFFFFU;
	}

	data->cycles = get_clock_cycles(config->clock_sel);

	start_clock(config->clock_sel);

	am_hal_ctimer_output_config(config->timer_num, seg, config->pincfg->states->pins->pin_num,
				    AM_HAL_CTIMER_OUTPUT_NORMAL,
				    AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA);

	am_hal_ctimer_config_single(config->timer_num, seg,
				    (_VAL2FLD(CTIMER_CTRL0_TMRA0FN, config->pwm_type + 2) |
				     _VAL2FLD(CTIMER_CTRL0_TMRA0CLK, config->clock_sel) |
				     AM_HAL_CTIMER_INT_ENABLE));

	return 0;
}

static DEVICE_API(pwm, pwm_ambiq_ctimer_driver_api) = {
	.set_cycles = ambiq_ctimer_pwm_set_cycles,
	.get_cycles_per_sec = ambiq_ctimer_pwm_get_cycles_per_sec,
};

#define PWM_AMBIQ_CTIMER_DEVICE_INIT(n)                                                            \
	BUILD_ASSERT(DT_CHILD_NUM_STATUS_OKAY(DT_INST_PARENT(n)) == 1,                             \
		     "Too many children for Timer!");                                              \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct pwm_ambiq_ctimer_data pwm_ambiq_ctimer_data_##n = {                          \
		.cycles = 0,                                                                       \
	};                                                                                         \
	static const struct pwm_ambiq_ctimer_config pwm_ambiq_ctimer_config_##n = {                \
		.timer_num = (DT_REG_ADDR(DT_INST_PARENT(n)) - CTIMER_BASE) /                      \
			     DT_REG_SIZE(DT_INST_PARENT(n)),                                       \
		.timer_seg = DT_INST_ENUM_IDX(n, timer_segment),                                   \
		.clock_sel = DT_ENUM_IDX(DT_INST_PARENT(n), clk_source),                           \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.pwm_type = DT_INST_ENUM_IDX(n, pwm_type)};                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ambiq_ctimer_pwm_init, NULL, &pwm_ambiq_ctimer_data_##n,          \
			      &pwm_ambiq_ctimer_config_##n, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, \
			      &pwm_ambiq_ctimer_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_AMBIQ_CTIMER_DEVICE_INIT)
