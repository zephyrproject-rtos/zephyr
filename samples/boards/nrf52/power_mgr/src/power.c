/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sample.h"

/*
 * Application defined function for doing any target specific operations
 * for low power entry.
 */
void sys_pm_notify_power_state_entry(enum power_states state)
{
	switch (state) {
	case SYS_POWER_STATE_SLEEP_1:
		printk("%s --> Entering to SYS_POWER_STATE_SLEEP_1 state.\n",
		       uptime_str());

		/* Turn off LED1 to indicate decreased power, then idle */
		gpio_pin_write(gpio_port, LED_1, LED_OFF);
		k_cpu_idle();

		break;

	case SYS_POWER_STATE_SLEEP_2:
		printk("%s --> Entering to SYS_POWER_STATE_SLEEP_2 state.\n",
		       uptime_str());

		/* Turn off LED2 to indicate decreased power, then idle */
		gpio_pin_write(gpio_port, LED_2, LED_OFF);
		k_cpu_idle();

		break;

	case SYS_POWER_STATE_DEEP_SLEEP_1:
		printk("%s --> Entering to SYS_POWER_STATE_DEEP_SLEEP_1 state.\n",
		       uptime_str());

		/*
		 * This power mode is already implemented by the OS. It will be
		 * handled by SoC-specific code. Here we just turn off the LEDs
		 * to save power.
		 */
		gpio_pin_write(gpio_port, LED_1, LED_OFF);
		gpio_pin_write(gpio_port, LED_2, LED_OFF);

		/* Nothing else to do */
		break;

	default:
		printk("Unsupported power state %u", state);
		break;
	}
}

/*
 * Application defined function for doing any target specific operations
 * for low power exit.
 */
void sys_pm_notify_power_state_exit(enum power_states state)
{
	switch (state) {
	case SYS_POWER_STATE_SLEEP_1:
		printk("%s --> Exited from SYS_POWER_STATE_SLEEP_1 state.\n",
		       uptime_str());

		/* Turn LED1 back on */
		gpio_pin_write(gpio_port, LED_1, LED_ON);
		break;

	case SYS_POWER_STATE_SLEEP_2:
		printk("%s --> Exited from SYS_POWER_STATE_SLEEP_2 state.\n",
		       uptime_str());

		/* Turn LED2 back on */
		gpio_pin_write(gpio_port, LED_2, LED_ON);
		break;

	case SYS_POWER_STATE_DEEP_SLEEP_1:
		/* Deep sleep on Nordic is system off: you can't get here. */

		printk("%s --> Exited from SYS_POWER_STATE_DEEP_SLEEP_1 state.\n",
		       uptime_str());

		/* Turn on LEDs which were powered down before deep sleep. */
		gpio_pin_write(gpio_port, LED_1, LED_ON);
		gpio_pin_write(gpio_port, LED_2, LED_ON);
		break;

	default:
		printk("Unsupported power state %u", state);
		break;
	}
}
