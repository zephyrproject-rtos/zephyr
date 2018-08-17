/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <power.h>
#include <soc_power.h>
#include <misc/printk.h>
#include <string.h>
#include <board.h>
#include <device.h>
#include <gpio.h>

#define SECONDS_TO_SLEEP	1

#define BUSY_WAIT_DELAY_US		(10 * 1000000)

#define LPS_STATE_ENTER_TO		((CONFIG_PM_LPS_MIN_RES + 1) * 1000)
#define LPS1_STATE_ENTER_TO		((CONFIG_PM_LPS_1_MIN_RES + 1) * 1000)

#define DEMO_DESCRIPTION	\
	"Demo Description\n"	\
	"Application creates Idleness, Due to which System Idle Thread is\n"\
	"scheduled and it enters into various Low Power States.\n"\

void sys_pm_notify_lps_entry(enum power_states state)
{
	printk("Entering Low Power state (%d)\n", state);
}

void sys_pm_notify_lps_exit(enum power_states state)
{
	printk("Entering Low Power state (%d)\n", state);
}

/* Application main Thread */
void main(void)
{
	printk("\n\n***OS Power Management Demo on %s****\n", CONFIG_ARCH);
	printk(DEMO_DESCRIPTION);

	for (int i = 1; i <= 4; i++) {
		printk("\n<-- App doing busy wait for 10 Sec -->\n");
		k_busy_wait(BUSY_WAIT_DELAY_US);

		/* Create Idleness to make Idle thread run */
		if ((i % 2) == 0) {
			printk("\n<-- App going to sleep for %d msec -->\n",
							LPS1_STATE_ENTER_TO);
			k_sleep(LPS1_STATE_ENTER_TO);
		} else {
			printk("\n<-- App going to sleep for %d msec -->\n",
							LPS_STATE_ENTER_TO);
			k_sleep(LPS_STATE_ENTER_TO);
		}
	}

	printk("OS managed Power Management Test completed\n");
}
