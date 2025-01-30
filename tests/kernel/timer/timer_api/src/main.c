/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/ztest.h>
#include <zephyr/types.h>

struct timer_data {
	int expire_cnt;
	int stop_cnt;
	int64_t timestamp;
};

#define DURATION 100
#define PERIOD 50
#define EXPIRE_TIMES 4
#define WITHIN_ERROR(var, target, epsilon) (llabs((int64_t) ((target) - (var))) <= (epsilon))

/* ms can be converted precisely to ticks only when a ms is exactly
 * represented by an integral number of ticks.  If the conversion is
 * not precise, then the reverse conversion of a difference in ms can
 * end up being off by a tick depending on the relative error between
 * the first and second ms conversion, and we need to adjust the
 * tolerance interval.
 */
#define INEXACT_MS_CONVERT ((CONFIG_SYS_CLOCK_TICKS_PER_SEC % MSEC_PER_SEC) != 0)

#if CONFIG_NRF_RTC_TIMER
/* On Nordic SOCs one or both of the tick and busy-wait clocks may
 * derive from sources that have slews that sum to +/- 13%.
 */
#define BUSY_TICK_SLEW_PPM 130000U
#else
/* On other platforms assume the clocks are perfectly aligned. */
#define BUSY_TICK_SLEW_PPM 0U
#endif
#define PPM_DIVISOR 1000000U

/* If the tick clock is faster or slower than the busywait clock the
 * remaining time for a partially elapsed timer in ticks will be
 * larger or smaller than expected by a value that depends on the slew
 * between the two clocks.  Produce a maximum error for a given
 * duration in microseconds.
 */
#define BUSY_SLEW_THRESHOLD_TICKS(_us)					\
	k_us_to_ticks_ceil32((_us) * (uint64_t)BUSY_TICK_SLEW_PPM	\
			     / PPM_DIVISOR)

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

#define TIMER_ASSERT(exp, tmr)			 \
	do {					 \
		if (!(exp)) {			 \
			k_timer_stop(tmr);	 \
			zassert_true(exp); \
		}				 \
	} while (0)

static void init_timer_data(void)
{
	tdata.expire_cnt = 0;
	tdata.stop_cnt = 0;

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_usleep(1); /* align to tick */
	}

	tdata.timestamp = k_uptime_get();
}

static bool interval_check(int64_t interval, int64_t desired)
{
	int64_t slop = INEXACT_MS_CONVERT ? 1 : 0;

	/* Tickless kernels will advance time inside of an ISR, so it
	 * is always possible (especially with high tick rates and
	 * slow CPUs) for us to arrive at the uptime check above too
	 * late to see a full period elapse before the next period.
	 * We can alias at both sides of the interval, so two
	 * one-ticks deltas (NOT one two-tick delta!)
	 */
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		slop += 2 * k_ticks_to_ms_ceil32(1);
	}

	if (!WITHIN_ERROR(interval, desired, slop)) {
		return false;
	}

	return true;
}

/* entry routines */
static void duration_expire(struct k_timer *timer)
{
	/** TESTPOINT: expire function */
	int64_t interval = k_uptime_delta(&tdata.timestamp);

	tdata.expire_cnt++;
	if (tdata.expire_cnt == 1) {
		TIMER_ASSERT(interval_check(interval, DURATION), timer);
	} else {
		TIMER_ASSERT(interval_check(interval, PERIOD), timer);
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

static void busy_wait_ms(int32_t ms)
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
ZTEST_USER(timer_api, test_timer_duration_period)
{
	init_timer_data();
	/** TESTPOINT: init timer via k_timer_init */
	k_timer_start(&duration_timer, K_MSEC(DURATION), K_MSEC(PERIOD));
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);
	/** TESTPOINT: check expire and stop times */
	TIMER_ASSERT(tdata.expire_cnt == EXPIRE_TIMES, &duration_timer);
	TIMER_ASSERT(tdata.stop_cnt == 1, &duration_timer);

	k_timer_start(&duration_timer, K_FOREVER, K_MSEC(PERIOD));
	TIMER_ASSERT(tdata.stop_cnt == 1, &duration_timer);
	/* cleanup environment */
	k_timer_stop(&duration_timer);
}

/**
 *
 * @brief Test restart the timer
 *
 * @details Validates initial duration and period of timer. Start the timer with
 * specific duration and period. Then starts the timer again, and check
 * the status of timer.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_init(), k_timer_start(), k_timer_stop, k_uptime_get(),
 * k_busy_wait()
 *
 */
ZTEST_USER(timer_api, test_timer_restart)
{
	init_timer_data();
	k_timer_start(&status_anytime_timer, K_MSEC(DURATION),
		      K_MSEC(PERIOD));
	busy_wait_ms(DURATION + PERIOD * (EXPIRE_TIMES - 1) + PERIOD / 2);

	/** TESTPOINT: restart the timer */
	k_timer_start(&status_anytime_timer, K_MSEC(DURATION),
		      K_MSEC(PERIOD));
	/* Restart timer, timer's status is reset to zero */
	TIMER_ASSERT(k_timer_status_get(&status_anytime_timer) == 0,
		     &status_anytime_timer);

	/* cleanup environment */
	k_timer_stop(&status_anytime_timer);
}


/**
 * @brief Test Timer with zero period value
 *
 * Validates initial timer duration, keeping timer period to zero.
 * Basically, acting as one-shot timer.
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
ZTEST_USER(timer_api, test_timer_period_0)
{
	init_timer_data();
	/** TESTPOINT: set period 0 */
	k_timer_start(&period0_timer,
		      K_TICKS(k_ms_to_ticks_floor32(DURATION)
			      - BUSY_SLEW_THRESHOLD_TICKS(DURATION
							  * USEC_PER_MSEC)),
		      K_NO_WAIT);
	/* Need to wait at least 2 durations to ensure one-shot behavior. */
	busy_wait_ms(2 * DURATION + 1);

	/** TESTPOINT: ensure it is one-shot timer */
	TIMER_ASSERT((tdata.expire_cnt == 1)
		     || (INEXACT_MS_CONVERT
			 && (tdata.expire_cnt == 0)), &period0_timer);
	TIMER_ASSERT(tdata.stop_cnt == 0, &period0_timer);

	/* cleanup environment */
	k_timer_stop(&period0_timer);
}

/**
 * @brief Test Timer with K_FOREVER period value
 *
 * Validates initial timer duration, keeping timer period to K_FOREVER.
 * Basically, acting as one-shot timer.
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
ZTEST_USER(timer_api, test_timer_period_k_forever)
{
	init_timer_data();
	/** TESTPOINT: set period 0 */
	k_timer_start(
		&period0_timer,
		K_TICKS(k_ms_to_ticks_floor32(DURATION) -
			BUSY_SLEW_THRESHOLD_TICKS(DURATION * USEC_PER_MSEC)),
		K_FOREVER);
	tdata.timestamp = k_uptime_get();

	/* Need to wait at least 2 durations to ensure one-shot behavior. */
	busy_wait_ms(2 * DURATION + 1);

	/** TESTPOINT: ensure it is one-shot timer */
	TIMER_ASSERT((tdata.expire_cnt == 1) ||
			     (INEXACT_MS_CONVERT && (tdata.expire_cnt == 0)),
		     &period0_timer);
	TIMER_ASSERT(tdata.stop_cnt == 0, &period0_timer);

	/* cleanup environment */
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
ZTEST_USER(timer_api, test_timer_expirefn_null)
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
ZTEST_USER(timer_api, test_timer_periodicity)
{
	uint64_t period_ms = k_ticks_to_ms_floor64(k_ms_to_ticks_ceil32(PERIOD));
	int64_t delta;

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
		 * time. The actual period will be equal to [tick time]
		 * multiplied by k_ms_to_ticks_ceil32(PERIOD).
		 *
		 * In the case of inexact conversion the delta will
		 * occasionally be one less than the expected number.
		 */
		TIMER_ASSERT(WITHIN_ERROR(delta, period_ms, 1)
			     || (INEXACT_MS_CONVERT
				 && (delta == period_ms - 1)),
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
ZTEST_USER(timer_api, test_timer_status_get)
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
ZTEST_USER(timer_api, test_timer_status_get_anytime)
{
	init_timer_data();
	k_timer_start(&status_anytime_timer, K_MSEC(DURATION),
		      K_MSEC(PERIOD));
	busy_wait_ms(DURATION + PERIOD * (EXPIRE_TIMES - 1) + PERIOD / 2);

	/** TESTPOINT: status get at any time */
	TIMER_ASSERT(k_timer_status_get(&status_anytime_timer) == EXPIRE_TIMES,
		     &status_anytime_timer);
	busy_wait_ms(PERIOD);
	TIMER_ASSERT(k_timer_status_get(&status_anytime_timer) == 1,
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
ZTEST_USER(timer_api, test_timer_status_sync)
{
	init_timer_data();
	k_timer_start(&status_sync_timer, K_MSEC(DURATION), K_MSEC(PERIOD));

	for (int i = 0; i < EXPIRE_TIMES; i++) {
		/** TESTPOINT: check timer not expire */
		TIMER_ASSERT(tdata.expire_cnt == i, &status_sync_timer);
		/** TESTPOINT: expired times returned by status sync */
		TIMER_ASSERT(k_timer_status_sync(&status_sync_timer) == 1,
			     &status_sync_timer);
		/** TESTPOINT: check timer not expire */
		TIMER_ASSERT(tdata.expire_cnt == (i + 1), &status_sync_timer);
	}

	init_timer_data();
	k_timer_start(&status_sync_timer, K_MSEC(DURATION), K_MSEC(PERIOD));
	busy_wait_ms(PERIOD*2);
	zassert_true(k_timer_status_sync(&status_sync_timer));

	/* cleanup environment */
	k_timer_stop(&status_sync_timer);
	zassert_false(k_timer_status_sync(&status_sync_timer));
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
ZTEST_USER(timer_api, test_timer_k_define)
{
	init_timer_data();
	/** TESTPOINT: init timer via k_timer_init */
	k_timer_start(&ktimer, K_MSEC(DURATION), K_MSEC(PERIOD));
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
 * k_timer_start() with specific initial duration, along with associated
 * user data using k_timer_user_data_set and k_timer_user_data_get().
 * Stops the timer using k_timer_stop() and checks for correct data
 * retrieval after timer completion.
 *
 * @ingroup kernel_timer_tests
 *
 * @see K_TIMER_DEFINE(), k_timer_user_data_set(), k_timer_start(),
 * k_timer_user_data_get(), k_timer_stop()
 */
ZTEST_USER(timer_api, test_timer_user_data)
{
	int ii;

	for (ii = 0; ii < 5; ii++) {
		intptr_t check;

		k_timer_user_data_set(user_data_timer[ii],
				      (void *)user_data[ii]);
		check = (intptr_t)k_timer_user_data_get(user_data_timer[ii]);

		zassert_true(check == user_data[ii]);
	}

	for (ii = 0; ii < 5; ii++) {
		k_timer_start(user_data_timer[ii], K_MSEC(50 + ii * 50),
			      K_NO_WAIT);
	}

	uint32_t wait_ms = 50 * ii + 50;

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_msleep(wait_ms);
	} else {
		uint32_t wait_us = 1000 * wait_ms;

		k_busy_wait(wait_us + (wait_us * BUSY_TICK_SLEW_PPM) / PPM_DIVISOR);
	}

	for (ii = 0; ii < 5; ii++) {
		k_timer_stop(user_data_timer[ii]);
	}

	for (ii = 0; ii < 5; ii++) {
		zassert_true(user_data_correct[ii]);
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

ZTEST_USER(timer_api, test_timer_remaining)
{
	uint32_t dur_ticks = k_ms_to_ticks_ceil32(DURATION);
	uint32_t target_rem_ticks = k_ms_to_ticks_ceil32(DURATION / 2);
	uint32_t rem_ms, rem_ticks, exp_ticks;
	uint32_t latency_ticks;
	int32_t delta_ticks;
	uint32_t slew_ticks;
	uint64_t now;

	/* Test is running in a user space thread so there is an additional latency
	 * involved in executing k_busy_wait and k_timer_remaining_ticks. Due
	 * to that latency, returned ticks won't be exact as expected even if
	 * k_busy_wait is running using the same clock source as the system clock.
	 * If system clock frequency is low (e.g. 100Hz) 1 tick will be enough but
	 * for cases where clock frequency is much higher we need to accept higher
	 * deviation (in ticks). Arbitrary value of 100 us processing overhead is used.
	 */
	latency_ticks = k_us_to_ticks_ceil32(100);

	init_timer_data();
	k_timer_start(&remain_timer, K_MSEC(DURATION), K_NO_WAIT);
	busy_wait_ms(DURATION / 2);
	rem_ticks = k_timer_remaining_ticks(&remain_timer);
	now = k_uptime_ticks();
	rem_ms = k_timer_remaining_get(&remain_timer);
	exp_ticks = k_timer_expires_ticks(&remain_timer);
	k_timer_stop(&remain_timer);
	TIMER_ASSERT(tdata.expire_cnt == 0, &remain_timer);
	TIMER_ASSERT(tdata.stop_cnt == 1, &remain_timer);

	/*
	 * While the busy_wait_ms() works with the maximum possible resolution,
	 * the k_timer api is limited by the system tick abstraction. As result
	 * the value obtained through k_timer_remaining_get() could be larger
	 * than actual remaining time with maximum error equal to one tick.
	 */
	zassert_true(rem_ms <= (DURATION / 2) + k_ticks_to_ms_floor64(1),
		     NULL);

	/* Half the value of DURATION in ticks may not be the value of
	 * half DURATION in ticks, when DURATION/2 is not an integer
	 * multiple of ticks, so target_rem_ticks is used rather than
	 * dur_ticks/2.  Also set a threshold based on expected clock
	 * skew.
	 */
	delta_ticks = (int32_t)(rem_ticks - target_rem_ticks);
	slew_ticks = BUSY_SLEW_THRESHOLD_TICKS(DURATION * USEC_PER_MSEC / 2U);
	zassert_true(abs(delta_ticks) <= MAX(slew_ticks, latency_ticks),
		     "tick/busy slew %d larger than test threshold %u",
		     delta_ticks, slew_ticks);

	/* Note +1 tick precision: even though we're calculating in
	 * ticks, we're waiting in k_busy_wait(), not for a timer
	 * interrupt, so it's possible for that to take 1 tick longer
	 * than expected on systems where the requested microsecond
	 * delay cannot be exactly represented as an integer number of
	 * ticks.
	 * As above, use higher tolerance on platforms where the clock used
	 * by the kernel timer and the one used for busy-waiting may be skewed.
	 */
	zassert_true(((int64_t)exp_ticks - (int64_t)now)
		     <= (dur_ticks / 2) + 1 + slew_ticks, NULL);
}

ZTEST_USER(timer_api, test_timeout_abs)
{
#ifdef CONFIG_TIMEOUT_64BIT
	const uint64_t exp_ms = 10000000;
	uint64_t rem_ticks;
	uint64_t exp_ticks = k_ms_to_ticks_ceil64(exp_ms);
	k_timeout_t t = K_TIMEOUT_ABS_TICKS(exp_ticks), t2;
	uint64_t t0, t1;

	/* Check the other generator macros to make sure they produce
	 * the same (whiteboxed) converted values
	 */
	t2 = K_TIMEOUT_ABS_MS(exp_ms);
	zassert_true(t2.ticks == t.ticks);

	t2 = K_TIMEOUT_ABS_US(1000 * exp_ms);
	zassert_true(t2.ticks == t.ticks);

	t2 = K_TIMEOUT_ABS_NS(1000 * 1000 * exp_ms);
	zassert_true(t2.ticks == t.ticks);

	t2 = K_TIMEOUT_ABS_CYC(k_ms_to_cyc_ceil64(exp_ms));
	zassert_true(t2.ticks == t.ticks);

	/* Now set the timeout and make sure the expiration time is
	 * correct vs. current time.  Tick units and tick alignment
	 * makes this math exact, no slop is needed.  Note that time
	 * is advancing always, so we add a retry condition to be sure
	 * that a tick advance did not happen between our reads of
	 * "now" and "expires".
	 */
	init_timer_data();
	k_timer_start(&remain_timer, t, K_FOREVER);

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_usleep(1);
	}

	do {
		t0 = k_uptime_ticks();
		rem_ticks = k_timer_remaining_ticks(&remain_timer);
		t1 = k_uptime_ticks();
	} while (t0 != t1);

	zassert_true(t0 + rem_ticks == exp_ticks,
		     "Wrong remaining: now %lld rem %lld expires %lld (%d)",
		     (uint64_t)t0, (uint64_t)rem_ticks, (uint64_t)exp_ticks,
		     t0+rem_ticks-exp_ticks);

	k_timer_stop(&remain_timer);
#endif
}

ZTEST_USER(timer_api, test_sleep_abs)
{
	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		/* k_sleep is not supported when multithreading is off. */
		return;
	}

	const int sleep_ticks = 50;
	int64_t start, end;

	k_usleep(1); /* tick align */

	start = k_uptime_ticks();
	k_sleep(K_TIMEOUT_ABS_TICKS(start + sleep_ticks));
	end = k_uptime_ticks();

	/* Systems with very high tick rates and/or slow idle resume
	 * (I've seen this on intel_adsp) can occasionally take more
	 * than a tick to return from k_sleep().  Set a 100us real
	 *  time slop or more depending on the time to resume
	 */
	k_ticks_t late = end - (start + sleep_ticks);
	int slop = MAX(2, k_us_to_ticks_ceil32(250));

	zassert_true(late >= 0 && late <= slop,
		     "expected wakeup at %lld, got %lld (late %lld)",
		     start + sleep_ticks, end, late);

	/* Let's test that an absolute delay awakes at the correct time
	 * even if the system did not get some ticks announcements
	 */
	int tickless_wait = 5;

	start = end;
	k_busy_wait(k_ticks_to_us_ceil32(tickless_wait));
	/* We expect to not have got <tickless_wait> tick announcements,
	 * as there is currently nothing scheduled
	 */
	k_sleep(K_TIMEOUT_ABS_TICKS(start + sleep_ticks));
	end = k_uptime_ticks();
	late = end - (start + sleep_ticks);

	zassert_true(late >= 0 && late <= slop,
		     "expected wakeup at %lld, got %lld (late %lld)",
		     start + sleep_ticks, end, late);

}

static void timer_init(struct k_timer *timer, k_timer_expiry_t expiry_fn,
		       k_timer_stop_t stop_fn)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_object_access_grant(timer, k_current_get());
	}

	k_timer_init(timer, expiry_fn, stop_fn);
}

void *setup_timer_api(void)
{
	timer_init(&duration_timer, duration_expire, duration_stop);
	timer_init(&period0_timer, period0_expire, NULL);
	timer_init(&expire_timer, NULL, duration_stop);
	timer_init(&sync_timer, NULL, NULL);
	timer_init(&periodicity_timer, NULL, NULL);
	timer_init(&status_timer, status_expire, status_stop);
	timer_init(&status_anytime_timer, NULL, NULL);
	timer_init(&status_sync_timer, duration_expire, duration_stop);
	timer_init(&remain_timer, duration_expire, duration_stop);

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_thread_access_grant(k_current_get(), &ktimer, &timer0, &timer1,
			      &timer2, &timer3, &timer4);
	}

	return NULL;
}

ZTEST_SUITE(timer_api, NULL, setup_timer_api, NULL, NULL, NULL);
