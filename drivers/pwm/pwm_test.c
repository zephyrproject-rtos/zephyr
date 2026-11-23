/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real PWM driver. It is used to instantiate struct
 * devices for the "vnd,pwm" devicetree compatible used in test code.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#define DT_DRV_COMPAT vnd_pwm

static int vnd_pwm_set_cycles(const struct device *dev, uint32_t channel,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	return -ENOTSUP;
}

#ifdef CONFIG_PWM_CAPTURE
static int vnd_pwm_configure_capture(const struct device *dev, uint32_t channel,
				     pwm_flags_t flags,
				     pwm_capture_callback_handler_t cb,
				     void *user_data)
{
	return -ENOTSUP;
}

static int vnd_pwm_enable_capture(const struct device *dev, uint32_t channel)
{
	return -ENOTSUP;
}

static int vnd_pwm_disable_capture(const struct device *dev, uint32_t channel)
{
	return -ENOTSUP;
}
#endif /* CONFIG_PWM_CAPTURE */

static int vnd_pwm_get_cycles_per_sec(const struct device *dev,
				      uint32_t channel, uint64_t *cycles)
{
	return -ENOTSUP;
}

static DEVICE_API(pwm, vnd_pwm_api) = {
	.set_cycles = vnd_pwm_set_cycles,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = vnd_pwm_configure_capture,
	.enable_capture = vnd_pwm_enable_capture,
	.disable_capture = vnd_pwm_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
	.get_cycles_per_sec = vnd_pwm_get_cycles_per_sec,
};

#define VND_PWM_INIT(n)							       \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,	       \
			      CONFIG_PWM_INIT_PRIORITY, &vnd_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(VND_PWM_INIT)
