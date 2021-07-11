/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2020 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <string.h>
#include <soc.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/printk.h>

const struct device *gpio_bn, *gpio_led;

/* in msec */
#define WAIT_TIME 500
/* change the value of SLEEP_TIME to check the residency */
#define SLEEP_TIME 4000
/* setting to K_TICKS_FOREVER will activate the deepsleep mode */

#define DEMO_DESCRIPTION	\
	"Demo Description\n"	\
	"Application is demonstrating the LPTIMER use as systick.\n"\
	"The system goes to sleep mode on k_sleep() and wakes-up\n" \
	"(led on) by the lptim timeout expiration\n" \
	"or when pressing the user push button (irq mode on EXTI)\n\n"

/* The devicetree node identifier for the "sw0" alias. */
#define SW0_NODE DT_ALIAS(sw0)
/* GPIO for the User push button */
#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define SW	DT_GPIO_LABEL(SW0_NODE, gpios)
#define PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#if DT_PHA_HAS_CELL(SW0_NODE, gpios, flags)
#define SW0_FLAGS DT_GPIO_FLAGS(SW0_NODE, gpios)
#else
#define SW0_FLAGS 0
#endif
#else
/* A build error here means your board isn't set up to input a button. */
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
/* GPIO for the Led */
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN0	DT_GPIO_PIN(LED0_NODE, gpios)
#define LED0_FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)

static struct gpio_callback gpio_cb;

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);
	ARG_UNUSED(cb);
	/* indicates a signal wakeUp from pin */
	gpio_pin_set(gpio_led, PIN0, 1);
}

/* Application main Thread */
void main(void)
{
	printk("\n\n*** Low power Management Demo on %s ***\n", CONFIG_BOARD);
	printk(DEMO_DESCRIPTION);

	gpio_bn = device_get_binding(SW);

	/* Configure Button 1 to wakeup from sleep mode */
	gpio_pin_configure(gpio_bn, PIN,
			SW0_FLAGS | GPIO_INPUT);
	gpio_pin_interrupt_configure(gpio_bn,
				   PIN,
				   GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&gpio_cb, button_pressed, BIT(PIN));
	gpio_add_callback(gpio_bn, &gpio_cb);

	/* Configure LEDs */
	gpio_led = device_get_binding(LED0);
	if (gpio_led == NULL) {
		return;
	}

	if (gpio_pin_configure(gpio_led, PIN0,
			GPIO_OUTPUT_ACTIVE | LED0_FLAGS) < 0) {
		return;
	}

	/*
	 * Start the demo.
	 */
	while (1) {
		/* Set pin to HIGH/LOW */
		gpio_pin_set(gpio_led, PIN0, 1);
		/* wait for while before going to low power mode */
		k_busy_wait(WAIT_TIME * 1000);
		printk(" sleeping...");

		gpio_pin_set(gpio_led, PIN0, 0);
		/* now going to low power mode */
		k_msleep(SLEEP_TIME);
		printk("...wakeUp\n");
	}
}
