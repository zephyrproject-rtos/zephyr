/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

static u32_t start, middle, end;
static struct k_timer timer;
static struct k_sem sem;

static void k_usleep_demo(void)
{
	u32_t i;

	printk("          | k_busy_wait() | k_usleep() | k_busy_wait() + k_usleep() |\n");

	for (i = 10; i < 1000; i += 10) {
		start = k_cycle_get_32();
		k_busy_wait(i);
		middle = k_cycle_get_32();
		k_usleep(1000 - i);
		end = k_cycle_get_32();
		
		printk("----------+----------------+-----------+----------------------------+\n");
		printk("REQUESTED | %10u us | %7u us |              %10u us |\n", i,  1000 - i, 1000);
		printk("MEASURED  | %10llu us | %7llu us |              %10llu us |\n",
			(u64_t)(middle - start) * USEC_PER_SEC / sys_clock_hw_cycles_per_sec(),
			(u64_t)(end - middle) * USEC_PER_SEC / sys_clock_hw_cycles_per_sec(),
			(u64_t)(end - start) * USEC_PER_SEC / sys_clock_hw_cycles_per_sec());

	}
	
	printk("----------+----------------+-----------+----------------------------+\n");
}

static void k_timer_demo_expire(struct k_timer *timer)
{
	end = k_cycle_get_32();
	k_sem_give(&sem);
}

static void k_timer_demo(void)
{
	u32_t i;

	k_sem_init(&sem, 0, 1);
	k_timer_init(&timer, k_timer_demo_expire, NULL);

	printk("          | k_busy_wait() | k_timer() | k_busy_wait() + k_timer() |\n");

	for (i = 1; i < 100; i += 1) {
		start = k_cycle_get_32();
		k_busy_wait(USEC_PER_MSEC * i);
		middle = k_cycle_get_32();
		k_timer_start(&timer, (100 - i), 0);
		k_sem_take(&sem, K_FOREVER);

		printk("----------+----------------+----------+---------------------------+\n");
		printk("REQUESTED | %10u us | %6u us |             %10u us |\n", USEC_PER_MSEC * i,  USEC_PER_MSEC * (100 - i), USEC_PER_MSEC * 100);
		printk("MEASURED  | %10llu us | %6llu us |             %10llu us |\n",
			(u64_t)(middle - start) * USEC_PER_SEC / sys_clock_hw_cycles_per_sec(),
			(u64_t)(end - middle) * USEC_PER_SEC / sys_clock_hw_cycles_per_sec(),
			(u64_t)(end - start) * USEC_PER_SEC / sys_clock_hw_cycles_per_sec());

	}
	
	printk("----------+----------------+----------+---------------------------+\n");
}

void main(void)
{
	printk ("\n==================== DEMO: k_timer API ====================\n\n");
	k_timer_demo();

	printk ("\n==================== DEMO: k_usleep API ====================\n\n");
	k_usleep_demo();
}
