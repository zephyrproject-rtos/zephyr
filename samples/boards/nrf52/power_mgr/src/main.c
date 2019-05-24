/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sample.h"

#define BUSY_WAIT_DELAY_US		(10 * 1000000)

#define LPS1_STATE_ENTER_TO		10
#define LPS2_STATE_ENTER_TO		30
#define DEEP_SLEEP_STATE_ENTER_TO	90

#define DEMO_DESCRIPTION	\
	"Demo Description\n"	\
	"Application creates idleness, due to which System Idle Thread is\n"\
	"scheduled and it enters into various Low Power States.\n"\

struct device *gpio_port;

/* Application main Thread */
void main(void)
{
	u32_t level = 0U;

	printk("\n\n*** Power Management Demo on %s ***\n", CONFIG_BOARD);
	printk(DEMO_DESCRIPTION);

	gpio_port = device_get_binding(PORT);

	/* Configure Button 1 as deep sleep trigger event */
	gpio_pin_configure(gpio_port, BUTTON_1, GPIO_DIR_IN
						| GPIO_PUD_PULL_UP);

	/* Configure Button 2 as wake source from deep sleep */
	gpio_pin_configure(gpio_port, BUTTON_2, GPIO_DIR_IN
						| GPIO_PUD_PULL_UP
						| GPIO_INT | GPIO_INT_LEVEL
						| GPIO_CFG_SENSE_LOW);

	gpio_pin_enable_callback(gpio_port, BUTTON_2);

	/* Configure LEDs */
	gpio_pin_configure(gpio_port, LED_1, GPIO_DIR_OUT);
	gpio_pin_write(gpio_port, LED_1, LED_ON);

	gpio_pin_configure(gpio_port, LED_2, GPIO_DIR_OUT);
	gpio_pin_write(gpio_port, LED_2, LED_ON);

	/*
	 * Start the demo.
	 */
	for (int i = 1; i <= 8; i++) {
		unsigned int sleep_seconds;

		switch (i) {
		case 3:
			printk("\n<-- Disabling %s state --->\n",
					STRINGIFY(SYS_POWER_STATE_SLEEP_3));
			sys_pm_ctrl_disable_state(SYS_POWER_STATE_SLEEP_3);
			break;

		case 5:
			printk("\n<-- Enabling %s state --->\n",
				       STRINGIFY(SYS_POWER_STATE_SLEEP_3));
			sys_pm_ctrl_enable_state(SYS_POWER_STATE_SLEEP_3);

			printk("<-- Disabling %s state --->\n",
					STRINGIFY(SYS_POWER_STATE_SLEEP_2));
			sys_pm_ctrl_disable_state(SYS_POWER_STATE_SLEEP_2);
			break;

		case 7:
			printk("\n<-- Enabling %s state --->\n",
				       STRINGIFY(SYS_POWER_STATE_SLEEP_2));
			sys_pm_ctrl_enable_state(SYS_POWER_STATE_SLEEP_2);

			printk("<-- Forcing %s state --->\n",
				       STRINGIFY(SYS_POWER_STATE_SLEEP_3));
			sys_pm_force_power_state(SYS_POWER_STATE_SLEEP_3);
			break;

		default:
			/* Do nothing. */
			break;
		}

		printk("\n<-- App doing busy wait for 10 Sec -->\n");
		k_busy_wait(BUSY_WAIT_DELAY_US);

		sleep_seconds = (i % 2 != 0) ? LPS1_STATE_ENTER_TO :
					       LPS2_STATE_ENTER_TO;

		printk("\n<-- App going to sleep for %u Sec -->\n",
							sleep_seconds);
		k_sleep(K_SECONDS(sleep_seconds));
	}

	/* Restore automatic power management. */
	printk("\n<-- Forcing %s state --->\n",
		       STRINGIFY(SYS_POWER_STATE_AUTO));
	sys_pm_force_power_state(SYS_POWER_STATE_AUTO);

	printk("\nPress BUTTON1 to enter into Deep Sleep state. "
			"Press BUTTON2 to exit Deep Sleep state\n");
	while (1) {
		gpio_pin_read(gpio_port, BUTTON_1, &level);
		if (level == LOW) {
			k_sleep(K_SECONDS(DEEP_SLEEP_STATE_ENTER_TO));
		}
		k_busy_wait(1000);
	}
}
