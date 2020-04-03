/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/types.h>

struct timer_data {
	int expire_cnt;
	int stop_cnt;
	s64_t timestamp;
};

#define DURATION 100
#define PERIOD 50
#define EXPIRE_TIMES 4
#define WITHIN_ERROR(var, target, epsilon)       \
		(((var) >= (target)) && ((var) <= (target) + (epsilon)))

static void duration_expire(struct k_timer *timer);
static void duration_stop(struct k_timer *timer);

/** TESTPOINT: init timer via K_TIMER_DEFINE */
K_TIMER_DEFINE(ktimer, duration_expire, duration_stop);

static struct k_timer duration_timer;
static struct k_timer period0_timer;
static struct k_timer expire_timer;
static struct k_timer sync_timer;
static struct k_timer periodicity_timer;
static struct k_timer status_timer;
static struct k_timer status_anytime_timer;
static struct k_timer status_sync_timer;
static struct k_timer remain_timer;

static ZTEST_BMEM struct timer_data tdata;

extern void test_time_conversions(void);

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
	s64_t interval = k_uptime_delta(&tdata.timestamp);

	tdata.expire_cnt++;
	if (tdata.expire_cnt == 1) {
		TIMER_ASSERT(interval >= DURATION, timer);
	} else {
		TIMER_ASSERT(interval >= PERIOD, timer);
	}

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
	k_busy_wait(ms*1000);
}

static void status_stop(struct k_timer *timer)
{
	/** TESTPOINT: remaining get upon timer stopped */
	TIMER_ASSERT(k_timer_remaining_get(timer) == 0, timer);
}

/**
 * @brief Tests for the Timer kernel object
 * @defgroup kernel_timer_tests Timer
 * @ingroup all_tests
 * @{
 * @}
 */

/**
 * @brief Test duration and period of Timer
 *
 * Validates initial duration and period of timer.
 *
 * It initializes the timer with k_timer_init(), then starts the timer
 * using k_timer_start() with specific initial duration and period.
 * Stops the timer using k_timer_stop() and checks for proper completion
 * of duration and period.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_init(), k_timer_start(), k_timer_stop(), k_uptime_get(),
 * k_busy_wait()
 */
void test_timer_duration_period(void)
{
	init_timer_data();
	/** TESTPOINT: init timer via k_timer_init */
	k_timer_start(&duration_timer, K_MSEC(DURATION), K_MSEC(PERIOD));
	tdata.timestamp = k_uptime_get();
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);
	/** TESTPOINT: check expire and stop times */
	TIMER_ASSERT(tdata.expire_cnt == EXPIRE_TIMES, &duration_timer);
	TIMER_ASSERT(tdata.stop_cnt == 1, &duration_timer);

	/* cleanup environemtn */
	k_timer_stop(&duration_timer);
}

/**
 * @brief Test Timer with zero period value
 *
 * Validates initial timer duration, keeping timer period to zero.
 * Basically, acting as one-short timer.
 * It initializes the timer with k_timer_init(), then starts the timer
 * using k_timer_start() with specific initial duration and period as
 * zero. Stops the timer using k_timer_stop() and checks for proper
 * completion.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_init(), k_timer_start(), k_timer_stop(), k_uptime_get(),
 * k_busy_wait()
 */
void test_timer_period_0(void)
{
	init_timer_data();
	/** TESTPOINT: set period 0 */
	k_timer_start(&period0_timer, K_MSEC(DURATION), K_NO_WAIT);
	tdata.timestamp = k_uptime_get();
	busy_wait_ms(DURATION + 1);

	/** TESTPOINT: ensure it is one-short timer */
	TIMER_ASSERT(tdata.expire_cnt == 1, &period0_timer);
	TIMER_ASSERT(tdata.stop_cnt == 0, &period0_timer);

	/* cleanup environemtn */
	k_timer_stop(&period0_timer);
}

/**
 * @brief Test Timer without any timer expiry callback function
 *
 * Validates timer without any expiry_fn(set to NULL). expiry_fn() is a
 * function that is invoked each time the timer expires.
 *
 * It initializes the timer with k_timer_init(), then starts the timer
 * using k_timer_start(). Stops the timer using k_timer_stop() and
 * checks for expire_cnt to zero, as expiry_fn was not defined at all.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_init(), k_timer_start(), k_timer_stop(), k_uptime_get(),
 * k_busy_wait()
 */
void test_timer_expirefn_null(void)
{
	init_timer_data();
	/** TESTPOINT: expire function NULL */
	k_timer_start(&expire_timer, K_MSEC(DURATION), K_MSEC(PERIOD));
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);

	k_timer_stop(&expire_timer);
	/** TESTPOINT: expire handler is not invoked */
	TIMER_ASSERT(tdata.expire_cnt == 0, &expire_timer);
	/** TESTPOINT: stop handler is invoked */
	TIMER_ASSERT(tdata.stop_cnt == 1, &expire_timer);

	/* cleanup environment */
	k_timer_stop(&expire_timer);
}

/* Wait for the next expiration of an OS timer tick, to synchronize
 * test start
 */
static void tick_sync(void)
{
	k_timer_start(&sync_timer, K_NO_WAIT, K_MSEC(1));
	k_timer_status_sync(&sync_timer);
	k_timer_stop(&sync_timer);
}

/**
 * @brief Test to check timer periodicity
 *
 * Timer test to check for the predictability with which the timer
 * expires depending on the period configured.
 *
 * It initializes the timer with k_timer_init(), then starts the timer
 * using k_timer_start() with specific period. It resets the timer’s
 * status to zero with k_timer_status_sync and identifies the delta
 * between each timer expiry to check for the timer expiration period
 * correctness. Finally, stops the timer using k_timer_stop().
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_init(), k_timer_start(), k_timer_status_sync(),
 * k_timer_stop(), k_uptime_get(), k_uptime_delta()
 */
void test_timer_periodicity(void)
{
	s64_t delta;

	/* Start at a tick boundary, otherwise a tick expiring between
	 * the unlocked (and unlockable) start/uptime/sync steps below
	 * will throw off the math.
	 */
	tick_sync();

	init_timer_data();
	/** TESTPOINT: set duration 0 */
	k_timer_start(&periodicity_timer, K_NO_WAIT, K_MSEC(PERIOD));

	/* clear the expiration that would have happened due to
	 * whatever duration that was set. Since timer is likely
	 * to fire before call to k_timer_status_sync(), we have
	 * to synchronize twice to ensure that the timestamp will
	 * be fetched as soon as possible after timer firing.
	 */
	k_timer_status_sync(&periodicity_timer);
	k_timer_status_sync(&periodicity_timer);
	tdata.timestamp = k_uptime_get();

	for (int i = 0; i < EXPIRE_TIMES; i++) {
		/** TESTPOINT: expired times returned by status sync */
		TIMER_ASSERT(k_timer_status_sync(&periodicity_timer) == 1,
			     &periodicity_timer);

		delta = k_uptime_delta(&tdata.timestamp);

		/** TESTPOINT: check if timer fired within 1ms of the
		 *  expected period (firing time).
		 *
		 * Please note, that expected firing time is not the
		 * one requested, as the kernel uses the ticks to manage
		 * time. The actual perioid will be equal to [tick time]
		 * multiplied by k_ms_to_ticks_ceil32(PERIOD).
		 */
		TIMER_ASSERT(WITHIN_ERROR(delta,
				k_ticks_to_ms_floor64(k_ms_to_ticks_ceil32(PERIOD)), 1),
				&periodicity_timer);
	}

	/* cleanup environment */
	k_timer_stop(&periodicity_timer);
}

/**
 * @brief Test Timer status and time remaining before next expiry
 *
 * Timer test to validate timer status and next trigger expiry time
 *
 * It initializes the timer with k_timer_init(), then starts the timer
 * using k_timer_start() and checks for timer current status with
 * k_timer_status_get() and remaining time before next expiry using
 * k_timer_remaining_get(). Stops the timer using k_timer_stop().
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_init(), k_timer_start(), k_timer_status_get(),
 * k_timer_remaining_get(), k_timer_stop()
 */
void test_timer_status_get(void)
{
	init_timer_data();
	k_timer_start(&status_timer, K_MSEC(DURATION), K_MSEC(PERIOD));
	/** TESTPOINT: status get upon timer starts */
	TIMER_ASSERT(k_timer_status_get(&status_timer) == 0, &status_timer);
	/** TESTPOINT: remaining get upon timer starts */
	TIMER_ASSERT(k_timer_remaining_get(&status_timer) >= DURATION / 2,
		     &status_timer);

	/* cleanup environment */
	k_timer_stop(&status_timer);
}

/**
 * @brief Test Timer status randomly after certain duration
 *
 * Validate timer status function using k_timer_status_get().
 *
 * It initializes the timer with k_timer_init(), then starts the timer
 * using k_timer_start() with specific initial duration and period.
 * Checks for timer status randomly after certain duration.
 * Stops the timer using k_timer_stop().
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_init(), k_timer_start(), k_timer_status_get(),
 * k_timer_stop(), k_busy_wait()
 */
void test_timer_status_get_anytime(void)
{
	init_timer_data();
	k_timer_start(&status_anytime_timer, K_MSEC(DURATION),
		      K_MSEC(PERIOD));
	busy_wait_ms(DURATION + PERIOD * (EXPIRE_TIMES - 1) + PERIOD / 2);

	/** TESTPOINT: status get at any time */
	TIMER_ASSERT(k_timer_status_get(&status_anytime_timer) == EXPIRE_TIMES,
		     &status_anytime_timer);

	/* cleanup environment */
	k_timer_stop(&status_anytime_timer);
}

/**
 * @brief Test Timer thread synchronization
 *
 * Validate thread synchronization by blocking the calling thread until
 * the timer expires.
 *
 * It initializes the timer with k_timer_init(), then starts the timer
 * using k_timer_start() and checks timer status with
 * k_timer_status_sync() for thread synchronization with expiry count.
 * Stops the timer using k_timer_stop.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_init(), k_timer_start(), k_timer_status_sync(),
 * k_timer_stop()
 */
void test_timer_status_sync(void)
{
	init_timer_data();
	k_timer_start(&status_sync_timer, K_MSEC(DURATION), K_MSEC(PERIOD));

	for (int i = 0; i < EXPIRE_TIMES; i++) {
		/** TESTPOINT: check timer not expire */
		TIMER_ASSERT(tdata.expire_cnt == i, &status_sync_timer);
		/** TESTPOINT： expired times returned by status sync */
		TIMER_ASSERT(k_timer_status_sync(&status_sync_timer) == 1,
			     &status_sync_timer);
		/** TESTPOINT: check timer not expire */
		TIMER_ASSERT(tdata.expire_cnt == (i + 1), &status_sync_timer);
	}

	/* cleanup environment */
	k_timer_stop(&status_sync_timer);
}

/**
 * @brief Test statically defined Timer init
 *
 * Validate statically defined timer init using K_TIMER_DEFINE
 *
 * It creates prototype of K_TIMER_DEFINE to statically define timer
 * init and starts the timer with k_timer_start() with specific initial
 * duration and period. Stops the timer using k_timer_stop() and checks
 * for proper completion of duration and period.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_start(), K_TIMER_DEFINE(), k_timer_stop()
 * k_uptime_get(), k_busy_wait()
 */
void test_timer_k_define(void)
{
	init_timer_data();
	/** TESTPOINT: init timer via k_timer_init */
	k_timer_start(&ktimer, K_MSEC(DURATION), K_MSEC(PERIOD));
	tdata.timestamp = k_uptime_get();
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);

	/** TESTPOINT: check expire and stop times */
	TIMER_ASSERT(tdata.expire_cnt == EXPIRE_TIMES, &ktimer);
	TIMER_ASSERT(tdata.stop_cnt == 1, &ktimer);

	/* cleanup environment */
	k_timer_stop(&ktimer);

	init_timer_data();
	/** TESTPOINT: init timer via k_timer_init */
	k_timer_start(&ktimer, K_MSEC(DURATION), K_MSEC(PERIOD));

	/* Call the k_timer_start() again to make sure that
	 * the initial timeout request gets cancelled and new
	 * one will get added.
	 */
	busy_wait_ms(DURATION / 2);
	k_timer_start(&ktimer, K_MSEC(DURATION), K_MSEC(PERIOD));
	tdata.timestamp = k_uptime_get();
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);

	/** TESTPOINT: check expire and stop times */
	TIMER_ASSERT(tdata.expire_cnt == EXPIRE_TIMES, &ktimer);
	TIMER_ASSERT(tdata.stop_cnt == 1, &ktimer);

	/* cleanup environment */
	k_timer_stop(&ktimer);
}

static void user_data_timer_handler(struct k_timer *timer);

K_TIMER_DEFINE(timer0, user_data_timer_handler, NULL);
K_TIMER_DEFINE(timer1, user_data_timer_handler, NULL);
K_TIMER_DEFINE(timer2, user_data_timer_handler, NULL);
K_TIMER_DEFINE(timer3, user_data_timer_handler, NULL);
K_TIMER_DEFINE(timer4, user_data_timer_handler, NULL);

static ZTEST_DMEM struct k_timer *user_data_timer[5] = {
	&timer0, &timer1, &timer2, &timer3, &timer4
};

static const intptr_t user_data[5] = { 0x1337, 0xbabe, 0xd00d, 0xdeaf, 0xfade };

static ZTEST_BMEM int user_data_correct[5];

static void user_data_timer_handler(struct k_timer *timer)
{
	int timer_num = timer == user_data_timer[0] ? 0 :
			timer == user_data_timer[1] ? 1 :
			timer == user_data_timer[2] ? 2 :
			timer == user_data_timer[3] ? 3 :
			timer == user_data_timer[4] ? 4 : -1;

	if (timer_num == -1) {
		return;
	}

	intptr_t data_retrieved = (intptr_t)k_timer_user_data_get(timer);
	user_data_correct[timer_num] = user_data[timer_num] == data_retrieved;
}

/**
 * @brief Test user-specific data associated with timer
 *
 * Validate user-specific data associated with timer
 *
 * It creates prototype of K_TIMER_DEFINE and starts the timer using
 * k_timer_start() with specific initial duration, alongwith associated
 * user data using k_timer_user_data_set and k_timer_user_data_get().
 * Stops the timer using k_timer_stop() and checks for correct data
 * retrieval after timer completion.
 *
 * @ingroup kernel_timer_tests
 *
 * @see K_TIMER_DEFINE(), k_timer_user_data_set(), k_timer_start(),
 * k_timer_user_data_get(), k_timer_stop()
 */
void test_timer_user_data(void)
{
	int ii;

	for (ii = 0; ii < 5; ii++) {
		intptr_t check;

		k_timer_user_data_set(user_data_timer[ii],
				      (void *)user_data[ii]);
		check = (intptr_t)k_timer_user_data_get(user_data_timer[ii]);

		zassert_true(check == user_data[ii], NULL);
	}

	for (ii = 0; ii < 5; ii++) {
		k_timer_start(user_data_timer[ii], K_MSEC(50 + ii * 50),
			      K_NO_WAIT);
	}

	k_msleep(50 * ii + 50);

	for (ii = 0; ii < 5; ii++) {
		k_timer_stop(user_data_timer[ii]);
	}

	for (ii = 0; ii < 5; ii++) {
		zassert_true(user_data_correct[ii], NULL);
	}
}

/**
 * @brief Test accuracy of k_timer_remaining_get()
 *
 * Validate countdown of time to expiration
 *
 * Starts a timer, busy-waits for half the DURATION, then checks the
 * remaining time to expiration and stops the timer. The remaining time
 * should reflect the passage of at least the busy-wait interval.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_init(), k_timer_start(), k_timer_stop(),
 * k_timer_remaining_get()
 */

void test_timer_remaining(void)
{
	u32_t dur_ticks = k_ms_to_ticks_ceil32(DURATION);
	u32_t rem_ms, rem_ticks, exp_ticks;
	u64_t now;

	k_usleep(1); /* align to tick */

	init_timer_data();
	k_timer_start(&remain_timer, K_MSEC(DURATION), K_NO_WAIT);
	busy_wait_ms(DURATION / 2);
	now = k_uptime_ticks();
	rem_ms = k_timer_remaining_get(&remain_timer);
	rem_ticks = k_timer_remaining_ticks(&remain_timer);
	exp_ticks = k_timer_expires_ticks(&remain_timer);
	k_timer_stop(&remain_timer);

	/*
	 * While the busy_wait_ms() works with the maximum possible resolution,
	 * the k_timer api is limited by the system tick abstraction. As result
	 * the value obtained through k_timer_remaining_get() could be larger
	 * than actual remaining time with maximum error equal to one tick.
	 */
	zassert_true(rem_ms <= (DURATION / 2) + k_ticks_to_ms_floor64(1),
		     NULL);

	zassert_true(rem_ticks <= dur_ticks / 2, NULL);

	/* Note +1 tick precision: even though we're calcluating in
	 * ticks, we're waiting in k_busy_wait(), not for a timer
	 * interrupt, so it's possible for that to take 1 tick longer
	 * than expected on systems where the requested microsecond
	 * delay cannot be exactly represented as an integer number of
	 * ticks.
	 */
	zassert_true(((s64_t)exp_ticks - (s64_t)now) <= (dur_ticks / 2) + 1,
		     NULL);
}

void test_timeout_abs(void)
{
#ifdef CONFIG_TIMEOUT_64BIT
	const int expiration = 10000000; /* 10M ticks */
	k_timeout_t t = K_TIMEOUT_ABS_TICKS(10000000), t2;

	/* Check the other generator macros to make sure they produce
	 * the same (whiteboxed) converted values
	 */
	t2 = K_TIMEOUT_ABS_MS(k_ticks_to_ms_ceil64(expiration));
	zassert_true(t2.ticks == t.ticks, NULL);

	t2 = K_TIMEOUT_ABS_US(k_ticks_to_us_ceil64(expiration));
	zassert_true(t2.ticks == t.ticks, NULL);

	t2 = K_TIMEOUT_ABS_NS(k_ticks_to_ns_ceil64(expiration));
	zassert_true(t2.ticks == t.ticks, NULL);

	t2 = K_TIMEOUT_ABS_CYC(k_ticks_to_cyc_ceil64(expiration));
	zassert_true(t2.ticks == t.ticks, NULL);

	/* Now set the timeout and make sure the expiration time is
	 * correct vs. current time.  Tick units and tick alignment
	 * makes this math exact: remember to add one to match the
	 * convention (i.e. a timer of "1 tick" will expire at "now
	 * plus 2 ticks", because "now plus one" will always be
	 * somewhat less than a tick).
	 */
	k_usleep(1); /* align to tick */
	k_timer_start(&remain_timer, t, K_FOREVER);
	zassert_true(k_timer_remaining_ticks(&remain_timer)
		     + k_uptime_ticks() + 1 == expiration, NULL);
	k_timer_stop(&remain_timer);
#endif
}

static void timer_init(struct k_timer *timer, k_timer_expiry_t expiry_fn,
		       k_timer_stop_t stop_fn)
{
	k_object_access_grant(timer, k_current_get());
	k_timer_init(timer, expiry_fn, stop_fn);
}

void test_main(void)
{
	timer_init(&duration_timer, duration_expire, duration_stop);
	timer_init(&period0_timer, period0_expire, NULL);
	timer_init(&expire_timer, NULL, duration_stop);
	timer_init(&sync_timer, NULL, NULL);
	timer_init(&periodicity_timer, NULL, NULL);
	timer_init(&status_timer, status_expire, status_stop);
	timer_init(&status_anytime_timer, NULL, NULL);
	timer_init(&status_sync_timer, duration_expire, duration_stop);
	timer_init(&remain_timer, NULL, NULL);

	k_thread_access_grant(k_current_get(), &ktimer, &timer0, &timer1,
			      &timer2, &timer3, &timer4);

	ztest_test_suite(timer_api,
			 ztest_unit_test(test_time_conversions),
			 ztest_user_unit_test(test_timer_duration_period),
			 ztest_user_unit_test(test_timer_period_0),
			 ztest_user_unit_test(test_timer_expirefn_null),
			 ztest_user_unit_test(test_timer_periodicity),
			 ztest_user_unit_test(test_timer_status_get),
			 ztest_user_unit_test(test_timer_status_get_anytime),
			 ztest_user_unit_test(test_timer_status_sync),
			 ztest_user_unit_test(test_timer_k_define),
			 ztest_user_unit_test(test_timer_user_data),
			 ztest_user_unit_test(test_timer_remaining),
			 ztest_user_unit_test(test_timeout_abs));
	ztest_run_test_suite(timer_api);
}
