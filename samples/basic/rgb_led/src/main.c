/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM-based RGB LED control
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/pwm.h>

/*
 * Extract devicetree configuration.
 */

#define RED_NODE DT_ALIAS(red_pwm_led)
#define GREEN_NODE DT_ALIAS(green_pwm_led)
#define BLUE_NODE DT_ALIAS(blue_pwm_led)

#if DT_NODE_HAS_STATUS(RED_NODE, okay)
#define RED_LABEL	DT_PWMS_LABEL(RED_NODE)
#define RED_CHANNEL	DT_PWMS_CHANNEL(RED_NODE)
#define RED_FLAGS	DT_PWMS_FLAGS(RED_NODE)
#else
#error "Unsupported board: red-pwm-led devicetree alias is not defined"
#define RED_LABEL	""
#define RED_CHANNEL	0
#define RED_FLAGS	0
#endif

#if DT_NODE_HAS_STATUS(GREEN_NODE, okay)
#define GREEN_LABEL	DT_PWMS_LABEL(GREEN_NODE)
#define GREEN_CHANNEL	DT_PWMS_CHANNEL(GREEN_NODE)
#define GREEN_FLAGS	DT_PWMS_FLAGS(GREEN_NODE)
#else
#error "Unsupported board: green-pwm-led devicetree alias is not defined"
#define GREEN_LABEL	""
#define GREEN_CHANNEL	0
#define GREEN_FLAGS	0
#endif

#if DT_NODE_HAS_STATUS(BLUE_NODE, okay)
#define BLUE_LABEL	DT_PWMS_LABEL(BLUE_NODE)
#define BLUE_CHANNEL	DT_PWMS_CHANNEL(BLUE_NODE)
#define BLUE_FLAGS	DT_PWMS_FLAGS(BLUE_NODE)
#else
#error "Unsupported board: blue-pwm-led devicetree alias is not defined"
#define BLUE_LABEL	""
#define BLUE_CHANNEL	0
#define BLUE_FLAGS	0
#endif

/*
 * 50 is flicker fusion threshold. Modulated light will be perceived
 * as steady by our eyes when blinking rate is at least 50.
 */
#define PERIOD_USEC	(USEC_PER_SEC / 50U)
#define STEPSIZE_USEC	2000

static int pwm_set(struct device *pwm_dev, uint32_t pwm_pin,
		     uint32_t pulse_width, pwm_flags_t flags)
{
	return pwm_pin_set_usec(pwm_dev, pwm_pin, PERIOD_USEC,
				pulse_width, flags);
}

enum { RED, GREEN, BLUE };

void main(void)
{
	struct device *pwm_dev[3];
	uint32_t pulse_red, pulse_green, pulse_blue; /* pulse widths */
	int ret;

	printk("PWM-based RGB LED control\n");

	pwm_dev[RED] = device_get_binding(RED_LABEL);
	pwm_dev[GREEN] = device_get_binding(GREEN_LABEL);
	pwm_dev[BLUE] = device_get_binding(BLUE_LABEL);
	if (!pwm_dev[RED] || !pwm_dev[GREEN] || !pwm_dev[BLUE]) {
		printk("Error: cannot find one or more PWM devices\n");
		return;
	}

	while (1) {
		for (pulse_red = 0U; pulse_red <= PERIOD_USEC;
		     pulse_red += STEPSIZE_USEC) {
			ret = pwm_set(pwm_dev[RED], RED_CHANNEL,
				      pulse_red, RED_FLAGS);
			if (ret != 0) {
				printk("Error %d: "
				       "red write failed\n",
				       ret);
				return;
			}

			for (pulse_green = 0U; pulse_green <= PERIOD_USEC;
			     pulse_green += STEPSIZE_USEC) {
				ret = pwm_set(pwm_dev[GREEN], GREEN_CHANNEL,
					      pulse_green, GREEN_FLAGS);
				if (ret != 0) {
					printk("Error %d: "
					       "green write failed\n",
					       ret);
					return;
				}

				for (pulse_blue = 0U; pulse_blue <= PERIOD_USEC;
				     pulse_blue += STEPSIZE_USEC) {
					ret = pwm_set(pwm_dev[BLUE],
						      BLUE_CHANNEL,
						      pulse_blue,
						      BLUE_FLAGS);
					if (ret != 0) {
						printk("Error %d: "
						       "blue write failed\n",
						       ret);
						return;
					}
					k_sleep(K_SECONDS(1));
				}
			}
		}
	}
}
