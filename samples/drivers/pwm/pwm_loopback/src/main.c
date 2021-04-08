/*
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 * Copyright (c) 2021 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <devicetree.h>
#include <drivers/pwm.h>

#define PWM_LOOPBACK_OUT_IDX 0
#define PWM_LOOPBACK_IN_IDX  1

#define PWM_LOOPBACK_NODE DT_INST(0, test_pwm_loopback)

#define PWM_LOOPBACK_OUT_CTLR \
	DT_PWMS_CTLR_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_OUT_IDX)
#define PWM_LOOPBACK_OUT_CHANNEL \
	DT_PWMS_CHANNEL_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_OUT_IDX)
#define PWM_LOOPBACK_OUT_FLAGS \
	DT_PWMS_FLAGS_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_OUT_IDX)

#define PWM_LOOPBACK_IN_CTLR \
	DT_PWMS_CTLR_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_IN_IDX)
#define PWM_LOOPBACK_IN_CHANNEL \
	DT_PWMS_CHANNEL_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_IN_IDX)
#define PWM_LOOPBACK_IN_FLAGS \
	DT_PWMS_FLAGS_BY_IDX(PWM_LOOPBACK_NODE, PWM_LOOPBACK_IN_IDX)

#if defined(CONFIG_APP_CAPTURE_TYPE_PULSE)
#define APP_CAPTURE_TYPE PWM_CAPTURE_TYPE_PULSE
#elif defined(CONFIG_APP_CAPTURE_TYPE_PERIOD)
#define APP_CAPTURE_TYPE PWM_CAPTURE_TYPE_PERIOD
#elif defined(CONFIG_APP_CAPTURE_TYPE_BOTH)
#define APP_CAPTURE_TYPE PWM_CAPTURE_TYPE_BOTH
#endif

struct test_pwm {
	const struct device *dev;
	uint32_t pwm;
	pwm_flags_t flags;
};

struct test_vector {
	uint64_t pulse_us;
	uint64_t period_us;
};

int get_test_pwms(struct test_pwm *out, struct test_pwm *in)
{
	/* PWM generator device */
	out->dev = DEVICE_DT_GET(PWM_LOOPBACK_OUT_CTLR);
	out->pwm = PWM_LOOPBACK_OUT_CHANNEL;
	out->flags = PWM_LOOPBACK_OUT_FLAGS;

	if (!device_is_ready(out->dev)) {
		printk("pwm loopback output device is not ready\n");
		return -1;
	}

	/* PWM capture device */
	in->dev = DEVICE_DT_GET(PWM_LOOPBACK_IN_CTLR);
	in->pwm = PWM_LOOPBACK_IN_CHANNEL;
	in->flags = PWM_LOOPBACK_IN_FLAGS;

	if (!device_is_ready(in->dev)) {
		printk("pwm loopback input device is not ready\n");
		return -1;
	}

	return 0;
}

struct test_vector vectors[] =  {
	{.pulse_us = 2,    .period_us = 10},
	{.pulse_us = 7,    .period_us = 10},
	{.pulse_us = 10,   .period_us = 100},
	{.pulse_us = 80,   .period_us = 100},
	{.pulse_us = 300,  .period_us = 1000},
	{.pulse_us = 700,  .period_us = 1000},
	{.pulse_us = 4000, .period_us = 10000},
	{.pulse_us = 9000, .period_us = 10000},
};

void main(void)
{
	struct test_pwm in;
	struct test_pwm out;
	uint64_t out_period;
	uint64_t out_pulse;
	uint64_t capture_period = 0;
	uint64_t capture_pulse = 0;
	int index = -1;
	int err;

	if (get_test_pwms(&out, &in) != 0) {
		printk("Coudn't get pwm devices\n");
		return;
	}

	while (true) {
		index = (index + 1) % ARRAY_SIZE(vectors);
		out_pulse = vectors[index].pulse_us;
		out_period = vectors[index].period_us;
		printk("Testing PWM capture @ %llu/%llu usec\n", out_pulse, out_period);

		err = pwm_pin_set_usec(out.dev, out.pwm, out_period,
				       out_pulse, out.flags);
		if (err != 0) {
			printk("Couldn't set pwm output, trying next.\n");
			continue;
		}

		k_sleep(K_MSEC(3000));

		if (pwm_pin_capture_usec(in.dev, in.pwm,
					 APP_CAPTURE_TYPE | PWM_POLARITY_NORMAL,
					 &capture_period, &capture_pulse,
					 K_USEC(out_period * 10)) == 0) {
			printk("Captured: %llu/%llu usec\n\n",
			       capture_pulse, capture_period);
		} else {
			printk("Couldn't capture pwm\n\n");
		}
	}
}
