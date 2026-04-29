/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#define SLEEP_TIME_MS	1

int main(void)
{
	printk("SYSTIM TEST [%d](%d)", k_uptime_get_32(), k_cycle_get_32());

	int i = 0;
	int timeout;

	timeout = 1;
	for (i=0; i<10; i++)
	{
		k_msleep(timeout);
		printk("1ms\n");
	}

	timeout = 10;
	for (i=0; i<10; i++)
	{
		k_msleep(timeout);
		printk("10ms\n");
	}

	timeout = 20;
	for (i=0; i<10; i++)
	{
		k_msleep(timeout);
		printk("20ms\n");
	}

	timeout = 50;
	for (i=0; i<10; i++)
	{
		k_msleep(timeout);
		printk("50ms\n");
	}

	timeout = 100;
	for (i=0; i<10; i++)
	{
		k_msleep(timeout);
		printk("100ms\n");
	}

	timeout = 200;
	for (i=0; i<10; i++)
	{
		k_msleep(timeout);
		printk("200ms\n");
	}

	timeout = 500;
	for (i=0; i<10; i++)
	{
		k_msleep(timeout);
		printk("500ms\n");
	}

	timeout = 1000;
	for (i=0; i<10; i++)
	{
		k_msleep(timeout);
		printk("1000ms\n");
	}

	printk("DONE [%d](%d)\n", k_uptime_get_32(), k_cycle_get_32());

	return 0;
}
