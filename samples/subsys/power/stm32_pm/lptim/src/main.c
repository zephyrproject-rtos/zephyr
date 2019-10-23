/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2019 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sample.h"

struct device *gpio_port, *gpio_port0;

/* 1000 msec = 1 sec */
#define WAIT_TIME 500
#define SLEEP_TIME 2001
/* measurement gives TIME VALUE +- 0.1ms */

#define DEMO_DESCRIPTION	\
	"Demo Description\n"	\
	"Application is demonstrating the LPTIMER use as systick.\n"	\
	"The led ON duration is increased by 500ms\n"	\
	"and the total elapsed time is displayed\n"	\
	"when pressing the user push button\n\n"

static struct gpio_callback gpio_cb;
static u32_t timeout = WAIT_TIME;
static s64_t last_time;

void button_pressed(struct device *gpiob, struct gpio_callback *cb,
		    u32_t pins)
{
		LL_PWR_ClearFlag_WU();
		timeout = timeout + WAIT_TIME;

		printk(" time = %d s with delta = %d s   \r",
		 (u32_t)last_time/1000, k_uptime_delta_32(&last_time)/1000);
		last_time = k_uptime_get_32();
}

/* Application main Thread */
void main(void)
{

	printk("\n\n*** LPTIM Management Demo on %s ***\n", CONFIG_BOARD);
	printk(DEMO_DESCRIPTION);

	gpio_port = device_get_binding(PORT);

	/* Configure Button 1 to increase timmeout value (no Pull up on PA0) */
	gpio_pin_configure(gpio_port, SW,
				GPIO_DIR_IN | GPIO_INT |
				GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW);

	gpio_init_callback(&gpio_cb, button_pressed, BIT(SW));

	gpio_add_callback(gpio_port, &gpio_cb);
	gpio_pin_enable_callback(gpio_port, SW);

	/* Configure LEDs */
	gpio_port0 = device_get_binding(PORT0);
	gpio_pin_configure(gpio_port0, LED0, GPIO_DIR_OUT);
	gpio_pin_write(gpio_port0, LED0, LED_OFF);

	/*
	 * Start the demo.
	 */
	last_time = k_uptime_get_32();

	while (1) {
		/* Set pin to HIGH/LOW */
		gpio_pin_write(gpio_port0, LED0, LED_ON);

		k_sleep(timeout);

		gpio_pin_write(gpio_port0, LED0, LED_OFF);

		k_sleep(SLEEP_TIME);
	}
}
