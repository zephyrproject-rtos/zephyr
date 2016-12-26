/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#define TIMER_ASSERT(exp, tmr)				\
	do {						\
		if (!(exp)) {				\
			k_timer_stop(tmr);		\
			assert_true(exp, NULL);		\
		}					\
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

static void busy_wait_ms(int32_t ms)
{
	int32_t deadline = k_uptime_get() + ms;

	volatile int32_t now = k_uptime_get();

	while (now < deadline) {
		now = k_uptime_get();
	}
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
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD/2);
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
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD/2);

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
	TIMER_ASSERT(k_timer_remaining_get(&timer) >= DURATION/2, &timer);

	/* cleanup environment */
	k_timer_stop(&timer);
}

void test_timer_status_get_anytime(void)
{
	init_timer_data();
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, DURATION, PERIOD);
	busy_wait_ms(DURATION + PERIOD * (EXPIRE_TIMES - 1) + PERIOD/2);

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
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD/2);

	/** TESTPOINT: check expire and stop times */
	TIMER_ASSERT(tdata.expire_cnt == EXPIRE_TIMES, &ktimer);
	TIMER_ASSERT(tdata.stop_cnt == 1, &ktimer);

	/* cleanup environment */
	k_timer_stop(&ktimer);
}
