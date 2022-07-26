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
#include <zephyr/drivers/gpio.h>
#include <inttypes.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(gpio_keys)
#error "No gpio_keys compatible node found in the device tree"
#endif

static const struct device *key = NULL;

/*
 * The led0 devicetree alias is optional. If present, we'll use it
 * to turn on the LED whenever the button is pressed.
 */
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

static void key_callback(const struct device *dev, uint16_t code, key_event_t event)
{
	printk("dev[%s] code[%d] event[%d] \r\n", dev->name, code, event);

	switch(event)
	{
	case KEY_EVENT_PRESSED:
		if (led.port)
			gpio_pin_set_dt(&led, 1);
		break;

    case KEY_EVENT_RELEASE:
		if (led.port)
			gpio_pin_set_dt(&led, 0);
		break;

	default:
		break;
	}
}

void main(void)
{
	int ret = 0;

	key = DEVICE_DT_GET_ONE(gpio_keys);

	if (key == NULL) {
		printk("key device is not exist\n");
		return;
	} else if (!device_is_ready(key)) {
		printk("key device %s is not ready\n", key->name);
		return;
	}

	if (led.port && !device_is_ready(led.port)) {
		printk("Error %d: LED device %s is not ready; ignoring it\n",
		       ret, led.port->name);
		led.port = NULL;
	}
	if (led.port) {
		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n",
			       ret, led.port->name, led.pin);
			led.port = NULL;
		} else {
			printk("Set up LED at %s pin %d\n", led.port->name, led.pin);
		}
	}

	key_setup(key, key_callback);

	while (1)
	{
		k_msleep(1000);
	}
}
