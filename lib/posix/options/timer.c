/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "posix_clock.h"

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/signal.h>
#include <zephyr/posix/time.h>

#define ACTIVE 1
#define NOT_ACTIVE 0

LOG_MODULE_REGISTER(posix_timer);

static void zephyr_timer_wrapper(struct k_timer *ztimer);

struct timer_obj {
	struct k_timer ztimer;
	struct sigevent evp;
	struct k_sem sem_cond;
	pthread_t thread;
	struct timespec interval;	/* Reload value */
	uint32_t reload;			/* Reload value in ms */
	uint32_t status;
};

K_MEM_SLAB_DEFINE(posix_timer_slab, sizeof(struct timer_obj), CONFIG_POSIX_TIMER_MAX,
		  __alignof__(struct timer_obj));

static void zephyr_timer_wrapper(struct k_timer *ztimer)
{
	struct timer_obj *timer;

	timer = (struct timer_obj *)ztimer;

	if (timer->reload == 0U) {
		timer->status = NOT_ACTIVE;
		LOG_DBG("timer %p not active", timer);
	}

	if (timer->evp.sigev_notify == SIGEV_NONE) {
		LOG_DBG("SIGEV_NONE");
		return;
	}

	if (timer->evp.sigev_notify_function == NULL) {
		LOG_DBG("NULL sigev_notify_function");
		return;
	}

	LOG_DBG("calling sigev_notify_function %p", timer->evp.sigev_notify_function);
	(timer->evp.sigev_notify_function)(timer->evp.sigev_value);
}

static void *zephyr_thread_wrapper(void *arg)
{
	int ret;
	struct timer_obj *timer = (struct timer_obj *)arg;

	ret = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	__ASSERT(ret == 0, "pthread_setcanceltype() failed: %d", ret);

	if (timer->evp.sigev_notify_attributes == NULL) {
		ret = pthread_detach(pthread_self());
		__ASSERT(ret == 0, "pthread_detach() failed: %d", ret);
	}

	while (1) {
		if (timer->reload == 0U) {
			timer->status = NOT_ACTIVE;
			LOG_DBG("timer %p not active", timer);
		}

		ret = k_sem_take(&timer->sem_cond, K_FOREVER);
		__ASSERT(ret == 0, "k_sem_take() failed: %d", ret);

		if (timer->evp.sigev_notify_function == NULL) {
			LOG_DBG("NULL sigev_notify_function");
			continue;
		}

		LOG_DBG("calling sigev_notify_function %p", timer->evp.sigev_notify_function);
		(timer->evp.sigev_notify_function)(timer->evp.sigev_value);
	}

	return NULL;
}

static void zephyr_timer_interrupt(struct k_timer *ztimer)
{
	struct timer_obj *timer;

	timer = (struct timer_obj *)ztimer;
	k_sem_give(&timer->sem_cond);
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
	int ret = 0;
	int detachstate;
	struct timer_obj *timer;
	const k_timeout_t alloc_timeout = K_MSEC(CONFIG_TIMER_CREATE_WAIT);

	if (evp == NULL || timerid == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (k_mem_slab_alloc(&posix_timer_slab, (void **)&timer, alloc_timeout) != 0) {
		LOG_DBG("k_mem_slab_alloc() failed: %d", ret);
		errno = ENOMEM;
		return -1;
	}

	*timer = (struct timer_obj){0};
	timer->evp = *evp;
	evp = &timer->evp;

	switch (evp->sigev_notify) {
	case SIGEV_NONE:
		k_timer_init(&timer->ztimer, NULL, NULL);
		break;
	case SIGEV_SIGNAL:
		k_timer_init(&timer->ztimer, zephyr_timer_wrapper, NULL);
		break;
	case SIGEV_THREAD:
		if (evp->sigev_notify_attributes != NULL) {
			ret = pthread_attr_getdetachstate(evp->sigev_notify_attributes,
							  &detachstate);
			if (ret != 0) {
				LOG_DBG("pthread_attr_getdetachstate() failed: %d", ret);
				errno = ret;
				ret = -1;
				goto free_timer;
			}

			if (detachstate != PTHREAD_CREATE_DETACHED) {
				ret = pthread_attr_setdetachstate(evp->sigev_notify_attributes,
								  PTHREAD_CREATE_DETACHED);
				if (ret != 0) {
					LOG_DBG("pthread_attr_setdetachstate() failed: %d", ret);
					errno = ret;
					ret = -1;
					goto free_timer;
				}
			}
		}

		ret = k_sem_init(&timer->sem_cond, 0, 1);
		if (ret != 0) {
			LOG_DBG("k_sem_init() failed: %d", ret);
			errno = -ret;
			ret = -1;
			goto free_timer;
		}

		ret = pthread_create(&timer->thread, evp->sigev_notify_attributes,
							zephyr_thread_wrapper, timer);
		if (ret != 0) {
			LOG_DBG("pthread_create() failed: %d", ret);
			errno = ret;
			ret = -1;
			goto free_timer;
		}

		k_timer_init(&timer->ztimer, zephyr_timer_interrupt, NULL);
		break;
	default:
		ret = -1;
		errno = EINVAL;
		goto free_timer;
	}

	*timerid = (timer_t)timer;
	goto out;

free_timer:
	k_mem_slab_free(&posix_timer_slab, (void *)timer);

out:
	return ret;
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

	if ((timer == NULL) || !timespec_is_valid(&value->it_interval) ||
	    !timespec_is_valid(&value->it_value)) {
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
	timer->reload = ts_to_ms(&value->it_interval);
	timer->interval.tv_sec = value->it_interval.tv_sec;
	timer->interval.tv_nsec = value->it_interval.tv_nsec;

	/* Calculate timer duration */
	duration = ts_to_ms(&(value->it_value));
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

	if (overruns > CONFIG_POSIX_DELAYTIMER_MAX) {
		overruns = CONFIG_POSIX_DELAYTIMER_MAX;
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

	if (timer->evp.sigev_notify == SIGEV_THREAD) {
		(void)pthread_cancel(timer->thread);
	}

	k_mem_slab_free(&posix_timer_slab, (void *)timer);

	return 0;
}
