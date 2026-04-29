/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#define SLEEP_TIME_MS	100

/* ============================================================
 *                           TEST_TIMER
 * ============================================================ */

static void expiry_handler(struct k_timer *timer);
static void stop_handler(struct k_timer *timer);

static K_TIMER_DEFINE(my_timer, expiry_handler, stop_handler);

static void expiry_handler(struct k_timer *timer)
{
	printk("timer: expiry_handler\n");
}

static void stop_handler(struct k_timer *timer)
{
	printk("timer: stop_handler\n");
}

static int test_timer(void)
{
	printk("timer: start_timer\n");

	k_timer_start(&my_timer, K_MSEC(25), K_MSEC(5));
	k_msleep(SLEEP_TIME_MS);
	k_timer_stop(&my_timer);

	return 0;
}

int main(void)
{
	printk("SYSTIM TEST [%d](%d)\n", k_uptime_get_32(), k_cycle_get_32());

	test_timer();

	printk("DONE [%d](%d)\n", k_uptime_get_32(), k_cycle_get_32());

	return 0;
}
