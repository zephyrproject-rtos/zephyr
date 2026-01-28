/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <zephyr/drivers/led.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

static const struct led_dt_spec led0 = LED_DT_SPEC_GET_OR(DT_ALIAS(led0), {0});

static void button_input_cb(struct input_event *evt, void *user_data)
{
	if (evt->sync == 0) {
		return;
	}

	printk("Button %d %s at %" PRIu32 "\n",
	       evt->code,
	       evt->value ? "pressed" : "released",
	       k_cycle_get_32());

	if (led0.dev != NULL) {
		led_set_brightness_dt(&led0, evt->value ? 100 : 0);
	}
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);

int main(void)
{
	printk("Press the button\n");

	k_sleep(K_FOREVER);

	return 0;
}
