/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_pwm

#include <errno.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>

#include "bee_timer_common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_bee, CONFIG_PWM_LOG_LEVEL);

#define HW_TIMER_SRC_CLOCK_HZ 40000000U

struct pwm_bee_data {
	const struct bee_timer_ops *ops;
};

struct pwm_bee_config {
	uint32_t reg;
	bool enhanced;
	uint8_t prescaler;
	uint8_t clock_div;
	uint16_t clkid;
	const struct pinctrl_dev_config *pcfg;
};

static int pwm_bee_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
			      uint32_t pulse_cycles, pwm_flags_t flags)
{
	ARG_UNUSED(channel);
	const struct pwm_bee_config *config = dev->config;
	struct pwm_bee_data *data = dev->data;

	LOG_DBG("channel=%d, period_cycles=%x, pulse_cycles=%x, flags=%x", channel, period_cycles,
		pulse_cycles, flags);

	if (!data->ops) {
		return -ENOTSUP;
	}

	data->ops->set_pwm_duty(config->reg, period_cycles, pulse_cycles,
				(flags & PWM_POLARITY_INVERTED));

	data->ops->stop(config->reg);
	data->ops->start(config->reg);

	return 0;
}

static int pwm_bee_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	const struct pwm_bee_config *config = dev->config;

	*cycles = (uint64_t)(HW_TIMER_SRC_CLOCK_HZ / config->prescaler);

	LOG_DBG("channel=%u, cycles=%llu", channel, *cycles);

	return 0;
}

static int pwm_bee_init(const struct device *dev)
{
	const struct pwm_bee_config *config = dev->config;
	struct pwm_bee_data *data = dev->data;
	int ret;

	data->ops = bee_timer_get_ops(config->enhanced);
	if (!data->ops) {
		LOG_ERR("PWM Driver: HAL not enabled for enhanced=%d", config->enhanced);
		return -ENOTSUP;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&config->clkid);

	data->ops->init(config->reg, config->clock_div, UINT32_MAX, BEE_TIMER_MODE_PWM);

	return 0;
}

static DEVICE_API(pwm, pwm_bee_driver_api) = {
	.set_cycles = pwm_bee_set_cycles,
	.get_cycles_per_sec = pwm_bee_get_cycles_per_sec,
};

#define PWM_BEE_PARENT_NODE(n) DT_INST_PARENT(n)

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#define TIMER_DIV_CONFIG(index)                                                                    \
	.clock_div = CONCAT(CLOCK_DIV_, DT_PROP(PWM_BEE_PARENT_NODE(index), prescaler))
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#define TIMER_DIV_CONFIG(index)                                                                    \
	.clock_div = CONCAT(TIM_CLOCK_DIVIDER_, DT_PROP(PWM_BEE_PARENT_NODE(index), prescaler))
#endif

#define PWM_BEE_INIT(index)                                                                        \
	static struct pwm_bee_data pwm_bee_data_##index;                                           \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct pwm_bee_config pwm_bee_config_##index = {                              \
		.reg = DT_REG_ADDR(PWM_BEE_PARENT_NODE(index)),                                    \
		.clkid = DT_CLOCKS_CELL(PWM_BEE_PARENT_NODE(index), id),                           \
		.prescaler = DT_PROP(PWM_BEE_PARENT_NODE(index), prescaler),                       \
		.enhanced = DT_NODE_HAS_COMPAT(PWM_BEE_PARENT_NODE(index),                         \
					       realtek_bee_enhanced_timer),                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		TIMER_DIV_CONFIG(index),                                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &pwm_bee_init, NULL, &pwm_bee_data_##index,                   \
			      &pwm_bee_config_##index, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,      \
			      &pwm_bee_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_BEE_INIT)
