/*
 * Copyright (c) 2025 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pwm.h>
#include <zephyr/dt-bindings/pwm/pwm.h>
#include <zephyr/kernel.h>

#define N_PERIODS 5

#define PWM_CHANNEL   0
#define PWM_PERIOD_NS (1 * NSEC_PER_MSEC)
#define PWM_FLAGS     PWM_POLARITY_NORMAL

static struct pwm_event_callback callback;

static bool pwm_idle;
static uint8_t counter;

static void pwm_callback_handler(const struct device *dev, struct pwm_event_callback *cb,
				 uint32_t channel, pwm_events_t events)
{
	ARG_UNUSED(channel);
	ARG_UNUSED(events);
	int ret;

	if (++counter == N_PERIODS) {
		pwm_idle = !pwm_idle;
		counter = 0;

		ret = pwm_set(dev, PWM_CHANNEL, PWM_PERIOD_NS, pwm_idle ? 0 : PWM_PERIOD_NS / 2,
			      PWM_FLAGS);
		if (ret < 0) {
			printk("failed to set pwm (%d)\n", ret);
			(void)pwm_remove_event_callback(dev, cb);
			return;
		}
	}
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(pwm_0));
	int ret;

	if (!device_is_ready(dev)) {
		printk("device is not ready\n");
		return 0;
	}

	ret = pwm_set(dev, PWM_CHANNEL, PWM_PERIOD_NS, PWM_PERIOD_NS / 2, PWM_FLAGS);
	if (ret < 0) {
		printk("failed to set idle (%d)\n", ret);
		return ret;
	}

	pwm_init_event_callback(&callback, pwm_callback_handler, PWM_CHANNEL,
				PWM_EVENT_TYPE_PERIOD);

	ret = pwm_add_event_callback(dev, &callback);
	if (ret < 0) {
		printk("failed to add event callback (%d)\n", ret);
		return ret;
	}

	return 0;
}
