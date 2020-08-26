/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
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

/*
 * Get button configuration from the devicetree sw0 alias.
 *
 * At least a GPIO device and pin number must be provided. The 'flags'
 * cell is optional.
 */

#define SW0_NODE	DT_ALIAS(sw0)

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define SW0_GPIO_LABEL	DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_GPIO_PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_GPIO_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))
#else
#error "Unsupported board: sw0 devicetree alias is not defined"
#define SW0_GPIO_LABEL	""
#define SW0_GPIO_PIN	0
#define SW0_GPIO_FLAGS	0
#endif

/* LED helpers, which use the led0 devicetree alias if it's available. */
static struct device *initialize_led(void);
static void match_led_to_button(struct device *button, struct device *led);

static struct gpio_callback button_cb_data;

void button_pressed(struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
}

void main(void)
{
	struct device *button;
	struct device *led;
	int ret;

	button = device_get_binding(SW0_GPIO_LABEL);
	if (button == NULL) {
		printk("Error: didn't find %s device\n", SW0_GPIO_LABEL);
		return;
	}

	ret = gpio_pin_configure(button, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
		return;
	}

	ret = gpio_pin_interrupt_configure(button,
					   SW0_GPIO_PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(button, &button_cb_data);
	printk("Set up button at %s pin %d\n", SW0_GPIO_LABEL, SW0_GPIO_PIN);

	led = initialize_led();

	printk("Press the button\n");
	while (1) {
		match_led_to_button(button, led);
		k_msleep(SLEEP_TIME_MS);
	}
}

/*
 * The led0 devicetree alias is optional. If present, we'll use it
 * to turn on the LED whenever the button is pressed.
 */

#define LED0_NODE	DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay) && DT_NODE_HAS_PROP(LED0_NODE, gpios)
#define LED0_GPIO_LABEL	DT_GPIO_LABEL(LED0_NODE, gpios)
#define LED0_GPIO_PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define LED0_GPIO_FLAGS	(GPIO_OUTPUT | DT_GPIO_FLAGS(LED0_NODE, gpios))
#endif

#ifdef LED0_GPIO_LABEL
static struct device *initialize_led(void)
{
	struct device *led;
	int ret;

	led = device_get_binding(LED0_GPIO_LABEL);
	if (led == NULL) {
		printk("Didn't find LED device %s\n", LED0_GPIO_LABEL);
		return NULL;
	}

	ret = gpio_pin_configure(led, LED0_GPIO_PIN, LED0_GPIO_FLAGS);
	if (ret != 0) {
		printk("Error %d: failed to configure LED device %s pin %d\n",
		       ret, LED0_GPIO_LABEL, LED0_GPIO_PIN);
		return NULL;
	}

	printk("Set up LED at %s pin %d\n", LED0_GPIO_LABEL, LED0_GPIO_PIN);

	return led;
}

static void match_led_to_button(struct device *button, struct device *led)
{
	bool val;

	val = gpio_pin_get(button, SW0_GPIO_PIN);
	gpio_pin_set(led, LED0_GPIO_PIN, val);
}

#else  /* !defined(LED0_GPIO_LABEL) */
static struct device *initialize_led(void)
{
	printk("No LED device was defined\n");
	return NULL;
}

static void match_led_to_button(struct device *button, struct device *led)
{
	return;
}
#endif	/* LED0_GPIO_LABEL */
