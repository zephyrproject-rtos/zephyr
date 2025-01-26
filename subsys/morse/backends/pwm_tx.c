/*
 * Copyright (c) 2025-2026 Freedom Veiculos Eletricos
 * Copyright (c) 2025-2026 O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_morse_pwm_tx

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/morse/morse_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(morse_pwm_tx, CONFIG_MORSE_LOG_LEVEL);

struct morse_pwm_tx_config {
	const struct pwm_dt_spec pwm;
};

int morse_pwm_tx_bit_state(const struct device *dev,
			   enum morse_bit_state state)
{
	const struct morse_pwm_tx_config *const cfg = dev->config;
	const uint32_t period = cfg->pwm.period;

	LOG_DBG("state: %d", state);

	return pwm_set_dt(&cfg->pwm, period, state ? (period >> 2) : 0);
}

static int morse_pwm_tx_init(const struct device *dev)
{
	const struct morse_pwm_tx_config *const cfg = dev->config;
	int ret;

	LOG_DBG("PWM");
	if (!pwm_is_ready_dt(&cfg->pwm)) {
		LOG_ERR("Error: PWM device %s is not ready", cfg->pwm.dev->name);
		return -ENODEV;
	}

	ret = pwm_set_dt(&cfg->pwm, cfg->pwm.period, 0);
	if (ret < 0) {
		LOG_ERR("Error: PWM device %s do not configure", cfg->pwm.dev->name);
		return -EFAULT;
	}

	return 0;
}

static DEVICE_API(morse, morse_pwm_tx_api) = {
	.tx_bit_state = morse_pwm_tx_bit_state,
};

#define MORSE_PWM_TX_INIT(n)							\
	static const struct morse_pwm_tx_config morse_pwm_tx_cfg_##n = {	\
		.pwm = PWM_DT_SPEC_GET(DT_DRV_INST(n)),				\
	};									\
	DEVICE_DT_INST_DEFINE(							\
		n, morse_pwm_tx_init,						\
		NULL,								\
		NULL,								\
		&morse_pwm_tx_cfg_##n,						\
		POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,				\
		&morse_pwm_tx_api);

DT_INST_FOREACH_STATUS_OKAY(MORSE_PWM_TX_INIT)
