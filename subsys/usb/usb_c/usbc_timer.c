/*
 * Copyright (c) 2022  The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usbc_timer.h"

/** Timer flag to track if timer was started */
#define TIMER_STARTED 0
/** Timer flag to track if timer has expired */
#define TIMER_EXPIRED 1

/**
 * @brief The timer function that's executed when the timer expires
 */
static void usbc_timer_handler(struct k_timer *timer)
{
	struct usbc_timer_t *usbc_timer = k_timer_user_data_get(timer);

	atomic_set_bit(&usbc_timer->flags, TIMER_EXPIRED);
}

void usbc_timer_init(struct usbc_timer_t *usbc_timer, uint32_t timeout_ms)
{
	k_timer_init(&usbc_timer->timer, usbc_timer_handler, NULL);
	k_timer_user_data_set(&usbc_timer->timer, usbc_timer);
	usbc_timer->timeout_ms = timeout_ms;
}

void usbc_timer_start(struct usbc_timer_t *usbc_timer)
{
	atomic_clear_bit(&usbc_timer->flags, TIMER_EXPIRED);
	atomic_set_bit(&usbc_timer->flags, TIMER_STARTED);
	k_timer_start(&usbc_timer->timer, K_MSEC(usbc_timer->timeout_ms), K_NO_WAIT);
}

bool usbc_timer_expired(struct usbc_timer_t *usbc_timer)
{
	bool started = atomic_test_bit(&usbc_timer->flags, TIMER_STARTED);
	bool expired = atomic_test_bit(&usbc_timer->flags, TIMER_EXPIRED);

	if (started & expired) {
		atomic_clear_bit(&usbc_timer->flags, TIMER_STARTED);
		return true;
	}

	return false;
}

bool usbc_timer_running(struct usbc_timer_t *usbc_timer)
{
	return atomic_test_bit(&usbc_timer->flags, TIMER_STARTED);
}

void usbc_timer_stop(struct usbc_timer_t *usbc_timer)
{
	atomic_clear_bit(&usbc_timer->flags, TIMER_STARTED);
	k_timer_stop(&usbc_timer->timer);
}
