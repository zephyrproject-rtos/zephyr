/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test.h"

#define BUSY_WAIT_DELAY_US		(10 * 1000000)

#define LPS_STATE_ENTER_TO		(30 * 1000)
#define LPS1_STATE_ENTER_TO		(90 * 1000)
#define DEEP_SLEEP_STATE_ENTER_TO	(120 * 1000)

#define DEMO_DESCRIPTION	\
	"Demo Description\n"	\
	"Application creates Idleness, Due to which System Idle Thread is\n"\
	"scheduled and it enters into various Low Power States.\n"\

void gpio_setup(void);
void create_device_list(void);

/* Application main Thread */
void main(void)
{
	struct device *gpio_in;
	u32_t level = 0;

	printk("\n\n***Power Management Demo on %s****\n", CONFIG_ARCH);
	printk(DEMO_DESCRIPTION);

	gpio_setup();
	create_device_list();
	gpio_in = device_get_binding(PORT);

	for (int i = 1; i <= 4; i++) {
		printk("\n<-- App doing busy wait for 10 Sec -->\n");
		k_busy_wait(BUSY_WAIT_DELAY_US);

		/* Create Idleness to make Idle thread run */
		if ((i % 2) == 0) {
			printk("\n<-- App going to sleep for 90 Sec -->\n");
			k_sleep(LPS1_STATE_ENTER_TO);
		} else {
			printk("\n<-- App going to sleep for 30 Sec -->\n");
			k_sleep(LPS_STATE_ENTER_TO);
		}

	}

	printk("\nPress BUTTON1 to enter into Deep Dleep state..."
			"Press BUTTON2 to exit Deep Sleep state\n");
	while (1) {
		gpio_pin_read(gpio_in, BUTTON_1, &level);
		if (level == LOW) {
			k_sleep(DEEP_SLEEP_STATE_ENTER_TO);
		}
		k_busy_wait(1000);
	}
}
