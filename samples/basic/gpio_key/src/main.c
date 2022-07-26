/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <soc.h>
#include <zephyr/drivers/key.h>
#include <zephyr/drivers/led.h>
#include <inttypes.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(gpio_keys)
#error "No gpio_keys compatible node found in the device tree"
#endif

#if !DT_HAS_COMPAT_STATUS_OKAY(gpio_leds)
#error "No gpio_leds compatible node found in the device tree"
#endif

static const struct device *key = NULL;
static const struct device *led = NULL;

static void key_callback(const struct device *dev, uint16_t code, key_event_t event)
{
	printk("dev[%s] code[%d] event[%d] \r\n", dev->name, code, event);

	switch(event)
	{
	case KEY_EVENT_PRESSED:
		led_on(led, 0);
		break;

    case KEY_EVENT_RELEASE:
		led_off(led, 0);
		break;

	default:
		break;
	}
}

void main(void)
{
	key = DEVICE_DT_GET_ONE(gpio_keys);

	if (key == NULL) {
		printk("key device is not exist\n");
		return;
	} else if (!device_is_ready(key)) {
		printk("key device %s is not ready\n", key->name);
		return;
	}

	led = DEVICE_DT_GET_ONE(gpio_leds);

	if (led == NULL) {
		printk("led device is not exist\n");
		return;
	} else if (!device_is_ready(led)) {
		printk("led device %s is not ready\n", led->name);
		return;
	}

	key_setup(key, key_callback);

	while (1)
	{
		k_msleep(1000);
	}
}
