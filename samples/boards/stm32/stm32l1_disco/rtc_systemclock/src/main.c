/*
 * Copyright (c) 2020 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <soc.h>
#include <device.h>

#define TIMER DT_LABEL(DT_INST(0, st_stm32_rtc))

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

const struct device *rtc = NULL;

void check_rtc_dev(void)
{

	printk("check_rtc_dev init on %u\n", z_timer_cycle_get_32());

	rtc = device_get_binding(TIMER);
		if (rtc == NULL) {
			printk("RTC dev not found\n");
		}
		else{
			printk("RTC dev found\n");
		}

	printk("check_rtc_dev terminated on %u\n", z_timer_cycle_get_32());

	k_sleep(K_FOREVER);
}


void check_busy_wait(void)
{
	uint32_t busy_wait_msec = 0;
	printk("check_busy_wait init on %u\n", z_timer_cycle_get_32());

	while(1){
		busy_wait_msec = 1*MSEC_PER_SEC;
		printk("check_busy_wait busy_wait for %u msec on %u\n", busy_wait_msec, z_timer_cycle_get_32());
		k_busy_wait(busy_wait_msec*USEC_PER_MSEC);

		printk("check_busy_wait ready on %u\n", z_timer_cycle_get_32());

		busy_wait_msec = 156;
		printk("check_busy_wait busy_wait for %u msec on %u\n", busy_wait_msec, z_timer_cycle_get_32());
		k_busy_wait(busy_wait_msec*USEC_PER_MSEC);

		printk("check_busy_wait ready on %u\n", z_timer_cycle_get_32());
	}

}

void check_sleep(void)
{
	uint32_t sleep_msec = 0;
	printk("check_sleep init on %u\n", z_timer_cycle_get_32());

	while(1){
		sleep_msec = 5500;
		printk("check_sleep sleep for %u msec on %u\n", sleep_msec, z_timer_cycle_get_32());
		k_sleep(K_MSEC(sleep_msec));

		printk("check_sleep awake on %u\n", z_timer_cycle_get_32());

		sleep_msec = 156;
		printk("check_sleep sleep for %u msec on %u\n", sleep_msec, z_timer_cycle_get_32());
		k_sleep(K_MSEC(sleep_msec));

		printk("check_sleep awake on %u\n", z_timer_cycle_get_32());
	}
}

K_THREAD_DEFINE(check_rtc_dev_id, STACKSIZE, check_rtc_dev, NULL, NULL, NULL,
		PRIORITY, 0, 0);

//K_THREAD_DEFINE(check_busy_wait_id, STACKSIZE, check_busy_wait, NULL, NULL, NULL,
//		PRIORITY, 0, 0);

K_THREAD_DEFINE(check_sleep_id, STACKSIZE, check_sleep, NULL, NULL, NULL,
		PRIORITY, 0, 0);


