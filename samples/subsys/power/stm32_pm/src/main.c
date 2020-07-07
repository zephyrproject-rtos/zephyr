/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2019 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sample.h"

struct device *gpio_bn, *gpio_led, *gpio_led2;

/* in msec */
#define WAIT_TIME 500
#define SLEEP_TIME 4001
/* setting to K_TICKS_FOREVER willl activate the deepsleep mode */

#define DEMO_DESCRIPTION	\
	"Demo Description\n"	\
	"Application is demonstrating the LPTIMER used as systick.\n"\
	"The system goes to sleep mode on k_sleep() and wakes-up\n" \
	"(red led on) by the lptim timeout expiration\n" \
	"or when pressing the user push button (WUP2 is PC13)\n\n"

static struct gpio_callback gpio_cb;

void button_pressed(struct device *gpiob, struct gpio_callback *cb,
		    uint32_t pins)
{
		/* Set pin to HIGH/LOW */
		gpio_pin_set(gpio_led, PIN0, 1);

		k_busy_wait(10000);
}

/* Application main Thread */
void main(void)
{
	printk("\n\n*** LPTIM Management Demo on %s ***\n", CONFIG_BOARD);
	printk(DEMO_DESCRIPTION);

	gpio_bn = device_get_binding(SW);

	if (IS_ENABLED(CONFIG_SYS_POWER_MANAGEMENT)) {
	/* Configure Button 1 to wakeup from sleep mode */
		gpio_pin_configure(gpio_bn, PIN,
				SW0_FLAGS | GPIO_INPUT);
		gpio_pin_interrupt_configure(gpio_bn,
					   PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);
		gpio_init_callback(&gpio_cb, button_pressed, BIT(PIN));
		gpio_add_callback(gpio_bn, &gpio_cb);

	}
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

		gpio_pin_set(gpio_led, PIN0, 0);

		k_msleep(SLEEP_TIME);
	}
}
