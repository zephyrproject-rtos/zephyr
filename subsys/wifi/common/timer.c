/*
 * @file
 * @brief Timer related functions
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "os_adapter.h"
#include "sm.h"

void wifimgr_timeout(void *sival_ptr)
{
	struct wifimgr_delayed_work *dwork =
	    (struct wifimgr_delayed_work *)sival_ptr;

	wifimgr_queue_work(&dwork->wq, &dwork->work);
}

int wifimgr_timer_start(timer_t timerid, unsigned int sec)
{
	struct itimerspec todelay;
	int ret;

	/* Start, restart, or stop the timer */
	todelay.it_value.tv_sec = sec;
	todelay.it_value.tv_nsec = 0;
	todelay.it_interval.tv_sec = 0;
	todelay.it_interval.tv_nsec = 0;

	ret = timer_settime(timerid, 0, &todelay, NULL);
	if (ret == -1)
		ret = -errno;

	return ret;
}

int wifimgr_interval_timer_start(timer_t timerid, unsigned int sec,
				 unsigned int interval_sec)
{
	struct itimerspec todelay;
	int ret;

	/* Start, restart, or stop the timer */
	todelay.it_value.tv_sec = sec;
	todelay.it_value.tv_nsec = 0;
	todelay.it_interval.tv_sec = interval_sec;
	todelay.it_interval.tv_nsec = 0;

	ret = timer_settime(timerid, 0, &todelay, NULL);
	if (ret == -1)
		ret = -errno;

	return ret;
}

int wifimgr_timer_init(struct wifimgr_delayed_work *dwork, void *sighand,
		       timer_t *timerid)
{
	struct sigevent toevent;
	int ret;

	/* Create a POSIX timer to handle timeouts */
	toevent.sigev_value.sival_ptr = dwork;
	toevent.sigev_notify = SIGEV_SIGNAL;
	toevent.sigev_notify_function = sighand;
	toevent.sigev_notify_attributes = NULL;

	ret = timer_create(CLOCK_MONOTONIC, &toevent, timerid);
	if (ret == -1)
		ret = -errno;

	return ret;
}

int wifimgr_timer_release(timer_t timerid)
{
	int ret;

	/* Delete the POSIX timer */
	ret = timer_delete(timerid);
	if (ret == -1)
		ret = -errno;

	return ret;
}
