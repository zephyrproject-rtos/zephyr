/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_letimer_pwm

#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <sl_hal_letimer.h>

LOG_MODULE_REGISTER(pwm_silabs_letimer, CONFIG_PWM_LOG_LEVEL);

struct silabs_letimer_pwm_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	LETIMER_TypeDef *base;
	int clock_div;
	bool run_in_debug;
};

static bool silabs_letimer_channel_is_pwm(const struct silabs_letimer_pwm_config *config,
					  uint32_t channel)
{
	uint32_t mask = (channel == 0) ? _LETIMER_CTRL_UFOA0_MASK : _LETIMER_CTRL_UFOA1_MASK;

	return FIELD_GET(mask, config->base->CTRL) == _LETIMER_CTRL_UFOA0_PWM;
}

static int silabs_letimer_pwm_set_cycles(const struct device *dev, uint32_t channel,
					 uint32_t period_cycles, uint32_t pulse_cycles,
					 pwm_flags_t flags)
{
	bool invert_polarity = (flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED;
	const struct silabs_letimer_pwm_config *config = dev->config;

	if (period_cycles >= BIT(24) || pulse_cycles >= BIT(24)) {
		return -ENOTSUP;
	}

	/* The hardware is not capable of driving a constant active level.
	 * Convert it into a constant inactive level with opposite polarity.
	 */
	if (pulse_cycles > 0 && pulse_cycles == period_cycles) {
		invert_polarity = !invert_polarity;
		pulse_cycles = 0;
	}

	if (invert_polarity) {
		sys_set_bit((mem_addr_t)&config->base->CTRL, channel + _LETIMER_CTRL_OPOL0_SHIFT);
	} else {
		sys_clear_bit((mem_addr_t)&config->base->CTRL, channel + _LETIMER_CTRL_OPOL0_SHIFT);
	}

	if (!silabs_letimer_channel_is_pwm(config, channel)) {
		config->base->CTRL_SET =
			(channel == 0) ? LETIMER_CTRL_UFOA0_PWM : LETIMER_CTRL_UFOA1_PWM;
	}

	sl_hal_letimer_set_compare(config->base, channel, pulse_cycles);
	sl_hal_letimer_set_top(config->base, period_cycles);

	if (!(config->base->STATUS & LETIMER_STATUS_RUNNING)) {
		sl_hal_letimer_start(config->base);
	}

	return 0;
}

static int silabs_letimer_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
						 uint64_t *cycles)
{
	const struct silabs_letimer_pwm_config *config = dev->config;
	uint32_t clock_rate;
	int err;

	err = clock_control_get_rate(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg,
				     &clock_rate);
	if (err < 0) {
		return err;
	}

	*cycles = clock_rate / BIT(config->clock_div);

	return 0;
}

static int silabs_letimer_pwm_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct silabs_letimer_pwm_config *config = dev->config;
	int err;

	if (action == PM_DEVICE_ACTION_RESUME) {
		err = clock_control_on(config->clock_dev,
				       (clock_control_subsys_t)&config->clock_cfg);
		if (err < 0 && err != -EALREADY) {
			return err;
		}

		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0 && err != -ENOENT) {
			return err;
		}

		sl_hal_letimer_enable(config->base);
	} else if (IS_ENABLED(CONFIG_PM_DEVICE) && (action == PM_DEVICE_ACTION_SUSPEND)) {
		sl_hal_letimer_disable(config->base);

		err = clock_control_off(config->clock_dev,
					(clock_control_subsys_t)&config->clock_cfg);
		if (err < 0) {
			return err;
		}

		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			return err;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int silabs_letimer_pwm_init(const struct device *dev)
{
	sl_hal_letimer_config_t letimer_config = SL_HAL_LETIMER_CONFIG_DEFAULT;
	const struct silabs_letimer_pwm_config *config = dev->config;
	int err;

	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	letimer_config.prescaler = config->clock_div;
	letimer_config.debug_run = config->run_in_debug;
	letimer_config.enable_top = true;
	sl_hal_letimer_init(config->base, &letimer_config);

	return pm_device_driver_init(dev, silabs_letimer_pwm_pm_action);
}

static DEVICE_API(pwm, silabs_letimer_pwm_api) = {
	.set_cycles = silabs_letimer_pwm_set_cycles,
	.get_cycles_per_sec = silabs_letimer_pwm_get_cycles_per_sec,
};

#define LETIMER_PWM_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	PM_DEVICE_DT_INST_DEFINE(inst, silabs_letimer_pwm_pm_action);                              \
                                                                                                   \
	static const struct silabs_letimer_pwm_config letimer_pwm_config_##inst = {                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(inst))),                  \
		.clock_cfg = SILABS_DT_CLOCK_CFG(DT_INST_PARENT(inst)),                            \
		.base = (LETIMER_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(inst)),                      \
		.run_in_debug = DT_PROP(DT_INST_PARENT(inst), run_in_debug),                       \
		.clock_div = DT_ENUM_IDX(DT_INST_PARENT(inst), clock_div),                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &silabs_letimer_pwm_init, PM_DEVICE_DT_INST_GET(inst), NULL,   \
			      &letimer_pwm_config_##inst, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,   \
			      &silabs_letimer_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(LETIMER_PWM_INIT)
