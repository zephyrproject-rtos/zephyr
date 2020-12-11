/*
 * Copyright (c) 2020 Stephane Dorre <stephane.dorre@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <zephyr.h>
#include <drivers/gpio.h>

static void button_pressed(const struct device *dev,
			   struct gpio_callback *cb, uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
}

static struct gpio_callback button_cb;

int main(void)
{
	int ret = 0;
	const struct device *dev_gpio = NULL;

	/* Only one GPIO peripheral in nRF52832 */
	/* So let's take the same for led AND button */
	dev_gpio = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
	if (!dev_gpio) {
		return (-EOPNOTSUPP);
	}

	ret = gpio_pin_configure(dev_gpio, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
			   GPIO_OUTPUT |
			   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios));
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d '%s'\n",
			ret, DT_GPIO_PIN(DT_ALIAS(led0), gpios), DT_LABEL(DT_ALIAS(led0)));
		return ret;
	}

	ret = gpio_pin_configure(dev_gpio, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				 GPIO_INPUT | GPIO_INT_EDGE_TO_ACTIVE |
				 DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios));
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d '%s'\n",
			ret, DT_GPIO_PIN(DT_ALIAS(sw0), gpios), DT_LABEL(DT_ALIAS(sw0)));
		return ret;
	}

	gpio_init_callback(&button_cb, button_pressed,
			   BIT(DT_GPIO_PIN(DT_ALIAS(sw0), gpios)));

	gpio_add_callback(dev_gpio, &button_cb);

	/* Pinetime trick : Enable button */
	ret = gpio_pin_configure(dev_gpio, DT_GPIO_PIN(DT_ALIAS(sw1), gpios),
				 GPIO_OUTPUT_ACTIVE |
				 DT_GPIO_FLAGS(DT_ALIAS(sw1), gpios));
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d '%s'\n",
			ret, DT_GPIO_PIN(DT_ALIAS(sw1), gpios), DT_LABEL(DT_ALIAS(sw1)));
		return ret;
	}

	printk("Init Done.");

	while (1) {
		/* button is pressed ==> turn on status LED */
		uint32_t val = 0u;
		uint8_t new_val = 0u;

		new_val = gpio_pin_get(dev_gpio, DT_GPIO_PIN(DT_ALIAS(sw0), gpios));
		if (new_val != val) {
			printk("New Button state %d.\n", new_val);
			gpio_pin_toggle(dev_gpio, DT_GPIO_PIN(DT_ALIAS(led0), gpios));
			val = new_val;
		}

		/* dont burn the CPU */
		k_sleep(K_MSEC(10));
	}

	return 0;
}
