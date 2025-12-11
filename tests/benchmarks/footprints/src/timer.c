/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>

#include "footprint.h"

#define DURATION	100
#define PERIOD		50
#define EXPIRE_TIMES	4

struct timer_data {
	uint32_t expire_cnt;
	uint32_t stop_cnt;
	int64_t timestamp;
};

static struct k_timer timer0;
static FP_BMEM struct timer_data tdata;

static void init_timer_data(void)
{
	tdata.expire_cnt = 0;
	tdata.stop_cnt = 0;
}

static void timer_init(struct k_timer *timer, k_timer_expiry_t expiry_fn,
		       k_timer_stop_t stop_fn)
{
	k_object_access_grant(timer, k_current_get());
	k_timer_init(timer, expiry_fn, stop_fn);
}

static void timer_stop(struct k_timer *timer)
{
	tdata.stop_cnt++;
}

static void timer_expire(struct k_timer *timer)
{
	tdata.expire_cnt++;
}

static void busy_wait_ms(int32_t ms)
{
	k_busy_wait(ms*1000);
}

static void thread_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	init_timer_data();
	k_timer_start(&timer0, K_MSEC(DURATION), K_MSEC(PERIOD));
	tdata.timestamp = k_uptime_get();
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);
	k_timer_stop(&timer0);

	init_timer_data();
	k_timer_start(&timer0, K_MSEC(DURATION), K_MSEC(PERIOD));

	/* Call the k_timer_start() again to make sure that
	 * the initial timeout request gets cancelled and new
	 * one will get added.
	 */
	busy_wait_ms(DURATION / 2);
	k_timer_start(&timer0, K_MSEC(DURATION), K_MSEC(PERIOD));
	tdata.timestamp = k_uptime_get();
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);

	k_timer_stop(&timer0);
}

void run_timer(void)
{
	k_tid_t tid;

	timer_init(&timer0, timer_expire, timer_stop);

	tid = k_thread_create(&my_thread, my_stack_area, STACK_SIZE,
			      thread_fn, NULL, NULL, NULL,
			      0, 0, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);

#ifdef CONFIG_USERSPACE
	tid = k_thread_create(&my_thread, my_stack_area, STACK_SIZE,
			      thread_fn, NULL, NULL, NULL,
			      0, K_USER, K_NO_WAIT);

	k_mem_domain_add_thread(&footprint_mem_domain, tid);

	k_thread_access_grant(tid, &timer0);
	k_thread_start(tid);

	k_thread_join(tid, K_FOREVER);

#endif
}
