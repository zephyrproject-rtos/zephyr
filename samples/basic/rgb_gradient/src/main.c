/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

#include "RGB_states.h"

static const struct pwm_dt_spec red_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));
static const struct pwm_dt_spec green_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led));
static const struct pwm_dt_spec blue_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led));

uint32_t red_period;
uint32_t green_period;
uint32_t blue_period;

int32_t red_color_unit_pulse_step;
int32_t green_color_unit_pulse_step;
int32_t blue_color_unit_pulse_step;

const struct rgb_color *colors[10] = {
	&red, &orange, &yellow,
	&lime, &green, &aquamarine,
	&cyan, &blue, &purple, &pink
};

const int num_colors = sizeof(colors);

#define PWM_SLEEP_TIME K_MSEC(40)

static void rgb_switch_state(struct rgb_color *current_color, const struct rgb_color *new_color)
{
	int32_t red_pulse_step = (new_color->red - current_color->red) *
					red_color_unit_pulse_step / 10;
	int32_t green_pulse_step = (new_color->green - current_color->green) *
					green_color_unit_pulse_step / 10;
	int32_t blue_pulse_step = (new_color->blue - current_color->blue) *
					blue_color_unit_pulse_step / 10;

	for (int i = 0; i < 10; i++) {
		pwm_set_pulse_dt(&red_pwm_led,
		(red_period * current_color->red / 255) + red_pulse_step * i);
		pwm_set_pulse_dt(&green_pwm_led,
		(green_period * current_color->green / 255) + green_pulse_step * i);
		pwm_set_pulse_dt(&blue_pwm_led,
		(blue_period * current_color->blue / 255) + blue_pulse_step * i);
		k_sleep(PWM_SLEEP_TIME);
	}

	current_color->red = new_color->red;
	current_color->green = new_color->green;
	current_color->blue = new_color->blue;

	k_sleep(K_MSEC(100));
}

static void cycle_colors(struct rgb_color *current_color)
{
	for (int i = 0; i < 10; i++) {
		rgb_switch_state(current_color, colors[i]);
	}
}

int main(void)
{
	struct rgb_color current_color = {0};

	if (!device_is_ready(red_pwm_led.dev) ||
	    !device_is_ready(green_pwm_led.dev) ||
	    !device_is_ready(blue_pwm_led.dev)) {
		printf("RGB LED device not ready\n");
		return -ENODEV;
	}

	red_period = red_pwm_led.period;
	green_period = green_pwm_led.period;
	blue_period = blue_pwm_led.period;

	red_color_unit_pulse_step = red_period / 255;
	green_color_unit_pulse_step = green_period / 255;
	blue_color_unit_pulse_step = blue_period / 255;

	printf("Disco mode activated.\n");

	while (1) {
		cycle_colors(&current_color);
	}

	return 0;
}
