/*
 * Copyright (c) 2025 Matthias Plöger <matze.ploeger@gmx.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

#define PWM_PERIOD_US     200 /* PWM period duration */
#define PWM_PULSE_TIME_US 100 /* PWM on/pulse/high time*/

static const struct device *pwm = DEVICE_DT_GET(DT_NODELABEL(pwm3));

int main(void)
{
	int ret;

	if (!device_is_ready(pwm)) {
		LOG_ERR("%s: pwm device not ready", pwm->name);
		return 0;
	}

	LOG_INF("Configuring PWM to generate signal on channel 1.");
	LOG_INF("TRGO event will be generated twice per given period.");
	ret = pwm_set(pwm, 1, PWM_USEC(PWM_PERIOD_US), PWM_USEC(PWM_PULSE_TIME_US),
		      PWM_POLARITY_NORMAL);
	if (ret < 0) {
		LOG_ERR("pwm_set() failed");
		return 0;
	}

	LOG_INF("TRGO event will be generated every 100us.");

	while (true) {
		k_sleep(K_MSEC(2000));
	}

	return 0;
}
