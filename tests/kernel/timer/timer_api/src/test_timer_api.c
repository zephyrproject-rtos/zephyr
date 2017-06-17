/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_timer.h"
#include <ztest.h>

#define DURATION 100
#define PERIOD 50
#define EXPIRE_TIMES 4
static void duration_expire(struct k_timer *timer);
static void duration_stop(struct k_timer *timer);

/** TESTPOINT: init timer via K_TIMER_DEFINE */
K_TIMER_DEFINE(ktimer, duration_expire, duration_stop);
static struct k_timer timer;
static struct timer_data tdata;

#define TIMER_ASSERT(exp, tmr)			 \
	do {					 \
		if (!(exp)) {			 \
			k_timer_stop(tmr);	 \
			zassert_true(exp, NULL); \
		}				 \
	} while (0)

static void init_timer_data(void)
{
	tdata.expire_cnt = 0;
	tdata.stop_cnt = 0;
}

/* entry routines */
static void duration_expire(struct k_timer *timer)
{
	/** TESTPOINT: expire function */
	tdata.expire_cnt++;
	if (tdata.expire_cnt == 1) {
		TIMER_ASSERT(k_uptime_delta(&tdata.timestamp) >= DURATION,
			     timer);
	} else {
		TIMER_ASSERT(k_uptime_delta(&tdata.timestamp) >= PERIOD, timer);
	}

	tdata.timestamp = k_uptime_get();
	if (tdata.expire_cnt >= EXPIRE_TIMES) {
		k_timer_stop(timer);
	}
}

static void duration_stop(struct k_timer *timer)
{
	tdata.stop_cnt++;
}

static void period0_expire(struct k_timer *timer)
{
	tdata.expire_cnt++;
}

static void status_expire(struct k_timer *timer)
{
	/** TESTPOINT: status get upon timer expired */
	TIMER_ASSERT(k_timer_status_get(timer) == 1, timer);
	/** TESTPOINT: remaining get upon timer expired */
	TIMER_ASSERT(k_timer_remaining_get(timer) >= PERIOD, timer);

	if (tdata.expire_cnt >= EXPIRE_TIMES) {
		k_timer_stop(timer);
	}
}

static void busy_wait_ms(s32_t ms)
{
#ifdef CONFIG_TICKLESS_KERNEL
	k_enable_sys_clock_always_on();
#endif
	s32_t deadline = k_uptime_get() + ms;

	volatile s32_t now = k_uptime_get();

	while (now < deadline) {
		now = k_uptime_get();
	}
#ifdef CONFIG_TICKLESS_KERNEL
	k_disable_sys_clock_always_on();
#endif
}

static void status_stop(struct k_timer *timer)
{
	/** TESTPOINT: remaining get upon timer stopped */
	TIMER_ASSERT(k_timer_remaining_get(timer) == 0, timer);
}

/* test cases */
void test_timer_duration_period(void)
{
	init_timer_data();
	/** TESTPOINT: init timer via k_timer_init */
	k_timer_init(&timer, duration_expire, duration_stop);
	k_timer_start(&timer, DURATION, PERIOD);
	tdata.timestamp = k_uptime_get();
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);
	/** TESTPOINT: check expire and stop times */
	TIMER_ASSERT(tdata.expire_cnt == EXPIRE_TIMES, &timer);
	TIMER_ASSERT(tdata.stop_cnt == 1, &timer);

	/* cleanup environemtn */
	k_timer_stop(&timer);
}

void test_timer_period_0(void)
{
	init_timer_data();
	/** TESTPOINT: set period 0 */
	k_timer_init(&timer, period0_expire, NULL);
	k_timer_start(&timer, DURATION, 0);
	tdata.timestamp = k_uptime_get();
	busy_wait_ms(DURATION + 1);

	/** TESTPOINT: ensure it is one-short timer */
	TIMER_ASSERT(tdata.expire_cnt == 1, &timer);
	TIMER_ASSERT(tdata.stop_cnt == 0, &timer);

	/* cleanup environemtn */
	k_timer_stop(&timer);
}

void test_timer_expirefn_null(void)
{
	init_timer_data();
	/** TESTPOINT: expire function NULL */
	k_timer_init(&timer, NULL, duration_stop);
	k_timer_start(&timer, DURATION, PERIOD);
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);

	k_timer_stop(&timer);
	/** TESTPOINT: expire handler is not invoked */
	TIMER_ASSERT(tdata.expire_cnt == 0, &timer);
	/** TESTPOINT: stop handler is invoked */
	TIMER_ASSERT(tdata.stop_cnt == 1, &timer);

	/* cleanup environment */
	k_timer_stop(&timer);
}

void test_timer_status_get(void)
{
	init_timer_data();
	k_timer_init(&timer, status_expire, status_stop);
	k_timer_start(&timer, DURATION, PERIOD);
	/** TESTPOINT: status get upon timer starts */
	TIMER_ASSERT(k_timer_status_get(&timer) == 0, &timer);
	/** TESTPOINT: remaining get upon timer starts */
	TIMER_ASSERT(k_timer_remaining_get(&timer) >= DURATION / 2, &timer);

	/* cleanup environment */
	k_timer_stop(&timer);
}

void test_timer_status_get_anytime(void)
{
	init_timer_data();
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, DURATION, PERIOD);
	busy_wait_ms(DURATION + PERIOD * (EXPIRE_TIMES - 1) + PERIOD / 2);

	/** TESTPOINT: status get at any time */
	TIMER_ASSERT(k_timer_status_get(&timer) == EXPIRE_TIMES, &timer);

	/* cleanup environment */
	k_timer_stop(&timer);
}

void test_timer_status_sync(void)
{
	init_timer_data();
	k_timer_init(&timer, duration_expire, duration_stop);
	k_timer_start(&timer, DURATION, PERIOD);

	for (int i = 0; i < EXPIRE_TIMES; i++) {
		/** TESTPOINT: check timer not expire */
		TIMER_ASSERT(tdata.expire_cnt == i, &timer);
		/** TESTPOINT： expired times returned by status sync */
		TIMER_ASSERT(k_timer_status_sync(&timer) == 1, &timer);
		/** TESTPOINT: check timer not expire */
		TIMER_ASSERT(tdata.expire_cnt == (i + 1), &timer);
	}

	/* cleanup environment */
	k_timer_stop(&timer);
}

void test_timer_k_define(void)
{
	init_timer_data();
	/** TESTPOINT: init timer via k_timer_init */
	k_timer_start(&ktimer, DURATION, PERIOD);
	tdata.timestamp = k_uptime_get();
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);

	/** TESTPOINT: check expire and stop times */
	TIMER_ASSERT(tdata.expire_cnt == EXPIRE_TIMES, &ktimer);
	TIMER_ASSERT(tdata.stop_cnt == 1, &ktimer);

	/* cleanup environment */
	k_timer_stop(&ktimer);
}

/* k_timer_user_data_set/get test */

static void user_data_timer_handler(struct k_timer *timer);

static struct k_timer user_data_timer[5] = {
	K_TIMER_INITIALIZER(user_data_timer[0], user_data_timer_handler, NULL),
	K_TIMER_INITIALIZER(user_data_timer[1], user_data_timer_handler, NULL),
	K_TIMER_INITIALIZER(user_data_timer[2], user_data_timer_handler, NULL),
	K_TIMER_INITIALIZER(user_data_timer[3], user_data_timer_handler, NULL),
	K_TIMER_INITIALIZER(user_data_timer[4], user_data_timer_handler, NULL),
};

static const intptr_t user_data[5] = { 0x1337, 0xbabe, 0xd00d, 0xdeaf, 0xfade };

static int user_data_correct[5] = { 0, 0, 0, 0, 0 };

static void user_data_timer_handler(struct k_timer *timer)
{
	int timer_num = timer == &user_data_timer[0] ? 0 :
			timer == &user_data_timer[1] ? 1 :
			timer == &user_data_timer[2] ? 2 :
			timer == &user_data_timer[3] ? 3 :
			timer == &user_data_timer[4] ? 4 : -1;

	if (timer_num == -1) {
		return;
	}

	intptr_t data_retrieved = (intptr_t)k_timer_user_data_get(timer);
	user_data_correct[timer_num] = user_data[timer_num] == data_retrieved;
}

void test_timer_user_data(void)
{
	int ii;

	for (ii = 0; ii < 5; ii++) {
		intptr_t check;

		k_timer_user_data_set(&user_data_timer[ii],
				      (void *)user_data[ii]);
		check = (intptr_t)k_timer_user_data_get(&user_data_timer[ii]);

		zassert_true(check == user_data[ii], NULL);
	}

	for (ii = 0; ii < 5; ii++) {
		k_timer_start(&user_data_timer[ii], 50 + ii * 50, 0);
	}

	k_sleep(50 * ii + 50);

	for (ii = 0; ii < 5; ii++) {
		k_timer_stop(&user_data_timer[ii]);
	}

	for (ii = 0; ii < 5; ii++) {
		zassert_true(user_data_correct[ii], NULL);
	}
}
