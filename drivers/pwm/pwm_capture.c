/*
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <drivers/pwm.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(pwm_capture, CONFIG_PWM_LOG_LEVEL);

struct z_pwm_pin_capture_cb_data {
	uint32_t period;
	uint32_t pulse;
	struct k_sem sem;
	int status;
};

static void z_pwm_pin_capture_cycles_callback(const struct device *dev,
					      uint32_t pwm,
					      uint32_t period_cycles,
					      uint32_t pulse_cycles,
					      int status,
					      void *user_data)
{
	struct z_pwm_pin_capture_cb_data *data = user_data;

	data->period = period_cycles;
	data->pulse = pulse_cycles;
	data->status = status;

	k_sem_give(&data->sem);
}

int z_impl_pwm_pin_capture_cycles(const struct device *dev, uint32_t pwm,
				  pwm_flags_t flags, uint32_t *period,
				  uint32_t *pulse, k_timeout_t timeout)
{
	struct z_pwm_pin_capture_cb_data data;
	int err;

	if ((flags & PWM_CAPTURE_MODE_MASK) == PWM_CAPTURE_MODE_CONTINUOUS) {
		LOG_ERR("continuous capture mode only supported via callback");
		return -ENOTSUP;
	}

	flags |= PWM_CAPTURE_MODE_SINGLE;
	k_sem_init(&data.sem, 0, 1);

	err = pwm_pin_configure_capture(dev, pwm, flags,
					z_pwm_pin_capture_cycles_callback,
					&data);
	if (err) {
		LOG_ERR("failed to configure pwm capture");
		return err;
	}

	err = pwm_pin_enable_capture(dev, pwm);
	if (err) {
		LOG_ERR("failed to enable pwm capture");
		return err;
	}

	err = k_sem_take(&data.sem, timeout);
	if (err == -EAGAIN) {
		(void)pwm_pin_disable_capture(dev, pwm);
		(void)pwm_pin_configure_capture(dev, pwm, flags, NULL, NULL);
		LOG_WRN("pwm capture timed out");
		return err;
	}

	if (data.status == 0) {
		if (period != NULL) {
			*period = data.period;
		}

		if (pulse != NULL) {
			*pulse = data.pulse;
		}
	}

	return data.status;
}
