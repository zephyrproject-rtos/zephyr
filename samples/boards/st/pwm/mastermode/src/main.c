/*
 * Copyright (c) 2025 Matthias Plöger <matze.ploeger@gmx.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

#define PWM_PERIOD1_US    400 /* PWM period duration */
#define PWM_PERIOD2_US    300 /* PWM period duration */
#define PWM_PERIOD3_US    200 /* PWM period duration */
#define PWM_PULSE_TIME_US 100 /* PWM on/pulse/high time*/

static const struct device *pwm_master = DEVICE_DT_GET(DT_NODELABEL(pwm1));
static struct pwm_dt_spec pwm_slave = PWM_DT_SPEC_GET(DT_PATH(zephyr_user));

int main(void)
{
	int ret;

	if (!device_is_ready(pwm_master)) {
		LOG_ERR("%s: pwm device not ready", pwm_master->name);
		return 0;
	}
	if (!device_is_ready(pwm_slave.dev)) {
		LOG_ERR("%s: pwm device not ready", pwm_slave.dev->name);
		return 0;
	}

	LOG_INF("slave PWM T=%dus, Ton=%dus", PWM_PERIOD1_US, PWM_PULSE_TIME_US);
	ret = pwm_set_dt(&pwm_slave, PWM_USEC(PWM_PERIOD1_US), PWM_USEC(PWM_PULSE_TIME_US));
	if (ret < 0) {
		LOG_ERR("pwm_set_dt() failed: %d", ret);
		return 0;
	}

	while (true) {
		k_sleep(K_MSEC(2000));

		LOG_INF("Set period of master PWM to %dus", PWM_PERIOD2_US);
		ret = pwm_set(pwm_master, 1, PWM_USEC(PWM_PERIOD2_US), 0, PWM_POLARITY_NORMAL);
		if (ret < 0) {
			LOG_ERR("pwm_set() failed: %d", ret);
			return 0;
		}
		LOG_INF("--> slave PWM T=%dus, Ton=%dus\n", PWM_PERIOD2_US, PWM_PULSE_TIME_US);

		k_sleep(K_MSEC(2000));

		LOG_INF("Set period of master PWM to %dus", PWM_PERIOD3_US);
		ret = pwm_set(pwm_master, 1, PWM_USEC(PWM_PERIOD3_US), 0, PWM_POLARITY_NORMAL);
		if (ret < 0) {
			LOG_ERR("pwm_set() failed: %d", ret);
			return 0;
		}
		LOG_INF("--> slave PWM T=%dus, Ton=%dus\n", PWM_PERIOD3_US, PWM_PULSE_TIME_US);
	}

	return 0;
}
