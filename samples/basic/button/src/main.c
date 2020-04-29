/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <inttypes.h>

#define SLEEP_TIME_MS	1

#if DT_PHA_HAS_CELL(DT_ALIAS(sw0), gpios, flags)
#define SW0_FLAGS DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios)
#else
#define SW0_FLAGS 0
#endif

#if DT_PHA_HAS_CELL(DT_ALIAS(led0), gpios, flags)
#define LED0_FLAGS DT_GPIO_FLAGS(DT_ALIAS(led0), gpios)
#else
#define LED0_FLAGS 0
#endif

void button_pressed(struct device *dev, struct gpio_callback *cb,
		    u32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
}

static struct gpio_callback button_cb_data;

void main(void)
{
	struct device *dev_button;
	int ret;

	dev_button = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(sw0), gpios));
	if (dev_button == NULL) {
		printk("Error: didn't find %s device\n",
			DT_GPIO_LABEL(DT_ALIAS(sw0), gpios));
		return;
	}

	ret = gpio_pin_configure(dev_button, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				 SW0_FLAGS | GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d '%s'\n",
			ret, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
			DT_LABEL(DT_ALIAS(sw0)));
		return;
	}

	ret = gpio_pin_interrupt_configure(dev_button,
					   DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
					   GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on pin %d '%s'\n",
			ret, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
			DT_LABEL(DT_ALIAS(sw0)));
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed,
			   BIT(DT_GPIO_PIN(DT_ALIAS(sw0), gpios)));
	gpio_add_callback(dev_button, &button_cb_data);

#if DT_NODE_HAS_PROP(DT_ALIAS(led0), gpios)
	struct device *dev_led;

	dev_led = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
	if (dev_led == NULL) {
		printk("Error: didn't find %s device\n",
			DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
		return;
	}

	ret = gpio_pin_configure(dev_led, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
				 LED0_FLAGS | GPIO_OUTPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d '%s'\n",
			ret, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
			DT_LABEL(DT_ALIAS(led0)));
		return;
	}
#endif
	printk("Press %s on the board\n", DT_LABEL(DT_ALIAS(sw0)));

	while (1) {
#if DT_NODE_HAS_PROP(DT_ALIAS(led0), gpios)
		bool val;

		val = gpio_pin_get(dev_button, DT_GPIO_PIN(DT_ALIAS(sw0), gpios));
		gpio_pin_set(dev_led, DT_GPIO_PIN(DT_ALIAS(led0), gpios), val);
		k_msleep(SLEEP_TIME_MS);
#endif
	}
}
