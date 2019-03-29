/*
 * @file
 * @brief Timer header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFI_TIMER_H_
#define _WIFI_TIMER_H_

#include "os_adapter.h"
#include "sm.h"

#define wifimgr_timer_stop(timerid) wifimgr_timer_start(timerid, 0)

void wifimgr_timeout(void *sival_ptr);
int wifimgr_timer_start(timer_t timerid, unsigned int sec);
int wifimgr_interval_timer_start(timer_t timerid, unsigned int sec,
				 unsigned int interval_sec);
int wifimgr_timer_init(struct wifimgr_delayed_work *dwork, void *sighand,
		       timer_t *timerid);
int wifimgr_timer_release(timer_t timerid);

#endif
