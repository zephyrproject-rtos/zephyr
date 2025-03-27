/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <CANopen.h>
#include <canopennode.h>

struct canopen_leds_state {
	canopen_led_callback_t green_cb;
	void *green_arg;
	canopen_led_callback_t red_cb;
	void *red_arg;
	bool green : 1;
	bool red : 1;
};

static struct canopen_leds_state canopen_leds;

void canopen_leds_update(struct canopen_context *ctx)
{
	bool green = false;
	bool red = false;

	green = CO_LED_GREEN(ctx->co->LEDs, CO_LED_CANopen);
	red = CO_LED_RED(ctx->co->LEDs, CO_LED_CANopen);

#ifdef CONFIG_CANOPENNODE_LEDS_BICOLOR
	if (red && canopen_leds.red_cb) {
		green = false;
	}
#endif

	if (canopen_leds.green_cb) {
		if (green != canopen_leds.green) {
			canopen_leds.green_cb(green, canopen_leds.green_arg);
			canopen_leds.green = green;
		}
	}

	if (canopen_leds.red_cb) {
		if (red != canopen_leds.red) {
			canopen_leds.red_cb(red, canopen_leds.red_arg);
			canopen_leds.red = red;
		}
	}
}

void canopen_leds_init(struct canopen_context *ctx,
		       canopen_led_callback_t green_cb, void *green_arg,
		       canopen_led_callback_t red_cb, void *red_arg)
{
	/* Call existing callbacks to turn off LEDs */
	if (canopen_leds.green_cb) {
		canopen_leds.green_cb(false, canopen_leds.green_arg);
	}
	if (canopen_leds.red_cb) {
		canopen_leds.red_cb(false, canopen_leds.red_arg);
	}

	canopen_leds.green_cb = green_cb;
	canopen_leds.green_arg = green_arg;
	canopen_leds.green = false;
	canopen_leds.red_cb = red_cb;
	canopen_leds.red_arg = red_arg;
	canopen_leds.red = false;

	/* Call new callbacks to turn off LEDs */
	if (canopen_leds.green_cb) {
		canopen_leds.green_cb(false, canopen_leds.green_arg);
	}
	if (canopen_leds.red_cb) {
		canopen_leds.red_cb(false, canopen_leds.red_arg);
	}
}
