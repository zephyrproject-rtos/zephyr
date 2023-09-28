/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <errno.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/posix/signal.h>
#include <zephyr/posix/time.h>

#define ACTIVE 1
#define NOT_ACTIVE 0

static void zephyr_timer_wrapper(struct k_timer *ztimer);

struct timer_obj {
	struct k_timer ztimer;
	void (*sigev_notify_function)(union sigval val);
	union sigval val;
	struct timespec interval;	/* Reload value */
	uint32_t reload;			/* Reload value in ms */
	uint32_t status;
};

K_MEM_SLAB_DEFINE(posix_timer_slab, sizeof(struct timer_obj),
		  CONFIG_MAX_TIMER_COUNT, 4);

static void zephyr_timer_wrapper(struct k_timer *ztimer)
{
	struct timer_obj *timer;

	timer = (struct timer_obj *)ztimer;

	if (timer->reload == 0U) {
		timer->status = NOT_ACTIVE;
	}

	(timer->sigev_notify_function)(timer->val);
}

/**
 * @brief Create a per-process timer.
 *
 * This API does not accept SIGEV_THREAD as valid signal event notification
 * type.
 *
 * See IEEE 1003.1
 */
int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid)
{
	struct timer_obj *timer;
	const k_timeout_t alloc_timeout = K_MSEC(CONFIG_TIMER_CREATE_WAIT);

	if (clockid != CLOCK_MONOTONIC || evp == NULL ||
	    (evp->sigev_notify != SIGEV_NONE &&
	     evp->sigev_notify != SIGEV_SIGNAL)) {
		errno = EINVAL;
		return -1;
	}

	if (k_mem_slab_alloc(&posix_timer_slab, (void **)&timer, alloc_timeout) == 0) {
		(void)memset(timer, 0, sizeof(struct timer_obj));
	} else {
		errno = ENOMEM;
		return -1;
	}

	timer->sigev_notify_function = evp->sigev_notify_function;
	timer->val = evp->sigev_value;
	timer->interval.tv_sec = 0;
	timer->interval.tv_nsec = 0;
	timer->reload = 0U;
	timer->status = NOT_ACTIVE;

	if (evp->sigev_notify == SIGEV_NONE) {
		k_timer_init(&timer->ztimer, NULL, NULL);
	} else {
		k_timer_init(&timer->ztimer, zephyr_timer_wrapper, NULL);
	}

	*timerid = (timer_t)timer;

	return 0;
}

/**
 * @brief Get amount of time left for expiration on a per-process timer.
 *
 * See IEEE 1003.1
 */
int timer_gettime(timer_t timerid, struct itimerspec *its)
{
	struct timer_obj *timer = (struct timer_obj *)timerid;
	int32_t remaining, leftover;
	int64_t   nsecs, secs;

	if (timer == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (timer->status == ACTIVE) {
		remaining = k_timer_remaining_get(&timer->ztimer);
		secs =  remaining / MSEC_PER_SEC;
		leftover = remaining - (secs * MSEC_PER_SEC);
		nsecs = (int64_t)leftover * NSEC_PER_MSEC;
		its->it_value.tv_sec = (int32_t) secs;
		its->it_value.tv_nsec = (int32_t) nsecs;
	} else {
		/* Timer is disarmed */
		its->it_value.tv_sec = 0;
		its->it_value.tv_nsec = 0;
	}

	/* The interval last set by timer_settime() */
	its->it_interval = timer->interval;
	return 0;
}

/**
 * @brief Sets expiration time of per-process timer.
 *
 * See IEEE 1003.1
 */
int timer_settime(timer_t timerid, int flags, const struct itimerspec *value,
		  struct itimerspec *ovalue)
{
	struct timer_obj *timer = (struct timer_obj *) timerid;
	uint32_t duration, current;

	if (timer == NULL ||
	    value->it_interval.tv_nsec < 0 ||
	    value->it_interval.tv_nsec >= NSEC_PER_SEC ||
	    value->it_value.tv_nsec < 0 ||
	    value->it_value.tv_nsec >= NSEC_PER_SEC) {
		errno = EINVAL;
		return -1;
	}

	/*  Save time to expire and old reload value. */
	if (ovalue != NULL) {
		timer_gettime(timerid, ovalue);
	}

	/* Stop the timer if the value is 0 */
	if ((value->it_value.tv_sec == 0) && (value->it_value.tv_nsec == 0)) {
		if (timer->status == ACTIVE) {
			k_timer_stop(&timer->ztimer);
		}

		timer->status = NOT_ACTIVE;
		return 0;
	}

	/* Calculate timer period */
	timer->reload = _ts_to_ms(&value->it_interval);
	timer->interval.tv_sec = value->it_interval.tv_sec;
	timer->interval.tv_nsec = value->it_interval.tv_nsec;

	/* Calculate timer duration */
	duration = _ts_to_ms(&(value->it_value));
	if ((flags & TIMER_ABSTIME) != 0) {
		current = k_timer_remaining_get(&timer->ztimer);

		if (current >= duration) {
			duration = 0U;
		} else {
			duration -= current;
		}
	}

	if (timer->status == ACTIVE) {
		k_timer_stop(&timer->ztimer);
	}

	timer->status = ACTIVE;
	k_timer_start(&timer->ztimer, K_MSEC(duration), K_MSEC(timer->reload));
	return 0;
}

/**
 * @brief Returns the timer expiration overrun count.
 *
 * See IEEE 1003.1
 */
int timer_getoverrun(timer_t timerid)
{
	struct timer_obj *timer = (struct timer_obj *) timerid;

	if (timer == NULL) {
		errno = EINVAL;
		return -1;
	}

	int overruns = k_timer_status_get(&timer->ztimer) - 1;

	if (overruns > CONFIG_TIMER_DELAYTIMER_MAX) {
		overruns = CONFIG_TIMER_DELAYTIMER_MAX;
	}

	return overruns;
}

/**
 * @brief Delete a per-process timer.
 *
 * See IEEE 1003.1
 */
int timer_delete(timer_t timerid)
{
	struct timer_obj *timer = (struct timer_obj *) timerid;

	if (timer == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (timer->status == ACTIVE) {
		timer->status = NOT_ACTIVE;
		k_timer_stop(&timer->ztimer);
	}

	k_mem_slab_free(&posix_timer_slab, (void *)timer);

	return 0;
}
