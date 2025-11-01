/*
 * Copyright (c) 2025 Matthias Plöger <matze.ploeger@gmx.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pwm.h> /* Access pin control functions. */
#include <zephyr/kernel.h>

/* */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

static const struct device *pwm = DEVICE_DT_GET(DT_NODELABEL(pwm3));

int main(void)
{
	int ret;

	if (!device_is_ready(pwm)) {
		LOG_ERR("%s: pwm device not ready", pwm->name);
		return -ENODEV;
	}

	LOG_INF("Configure PWM module to generate PWM signal on channel 1. "
		"TRGO event will be generated twice per given period.");
	ret = pwm_set(pwm, 1, PWM_USEC(100), PWM_USEC(200), PWM_POLARITY_NORMAL);
	if (ret < 0) {
		LOG_ERR("pwm_set failed");
		return ret;
	}

	LOG_INF("TRGO event will be generated every 100us.");

	while (true) {
		k_sleep(K_MSEC(2000));
	}

	return 0;
}
