/*
 * Copyright (c) 2022, tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/input.h>
#include <zephyr/dt-bindings/input/input.h>
#include <inttypes.h>

static const struct device *kbd;
static struct k_timer esc_timer;

static const char * const key_value_string[] = {
	"KEY_RELEASE",
	"KEY_PRESSED",
	"KEY_LONG_PRESSED",
	"KEY_HOLD_PRESSED",
	"KEY_LONG_RELEASE"
};

static void esc_timer_callback(struct k_timer *timer)
{
	struct input_event event;

	printk("%s arrived.\n", __func__);

	event.type = EV_KEY;
	event.code = KEY_CODE_BACKSPACE;

	event.value = KEY_PRESSED;
	input_event_write(kbd, &event);

	event.value = KEY_RELEASE;
	input_event_write(kbd, &event);
}

void main(void)
{
	union input_attr_data data;
	struct input_event event;

	kbd = DEVICE_DT_GET_ONE(keyboard_gpio);

	if (!device_is_ready(kbd)) {
		printk("kbd device %s is not ready\n", kbd->name);
		return;
	}

	k_timer_init(&esc_timer, esc_timer_callback, NULL);

	input_setup(kbd);
	input_attr_get(kbd, INPUT_ATTR_EVENT_READ_TIMEOUT, &data);
	printk("get read timeout %lld\n", data.timeout.ticks);

	while (1) {
		k_timeout_t timeouts[] = {K_NO_WAIT, K_MSEC(1000), K_FOREVER};
		static int step;

		data.timeout = timeouts[step];
		step = ((step + 1) % ARRAY_SIZE(timeouts));

		input_attr_set(kbd, INPUT_ATTR_EVENT_READ_TIMEOUT, &data);
		printk("set read timeout %lld\n", data.timeout.ticks);

		input_attr_get(kbd, INPUT_ATTR_EVENT_READ_TIMEOUT, &data);
		printk("get read timeout %lld\n", data.timeout.ticks);

		while (1) {
			if (input_event_read(kbd, &event) == 0) {
				if (event.type != EV_KEY) {
					continue;
				}

				printk("event [EV_KEY 0x%02x %s]\n", \
					event.code, key_value_string[event.value]);

				if (event.code != KEY_CODE_BACKSPACE) {
					k_timer_start(&esc_timer, K_SECONDS(10), K_SECONDS(0));
				} else if (event.value == KEY_RELEASE) {
					break;
				}
			}
		}
	}
}
