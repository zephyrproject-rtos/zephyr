/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/osal/osal.h>
#include <zephyr/logging/log.h>

#include "osal_priv.h"

LOG_MODULE_DECLARE(osal, CONFIG_OSAL_LOG_LEVEL);

/*
 * Software timers map onto k_timer. The callback takes a single void*,
 * so the control block carries the callback, its argument and whether the
 * timer is periodic (so start() can program the repeat interval).
 */
struct osal_timer_cb {
	struct k_timer timer;
	osal_timer_cb_t cb;
	void *arg;
	bool periodic;
};

static void timer_expiry(struct k_timer *timer)
{
	struct osal_timer_cb *t = CONTAINER_OF(timer, struct osal_timer_cb, timer);

	if (t->cb != NULL) {
		t->cb(t->arg);
	}
}

osal_timer_t osal_timer_create(osal_timer_cb_t cb, void *arg, bool periodic)
{
	struct osal_timer_cb *t = osal_cb_alloc(sizeof(struct osal_timer_cb));

	if (t == NULL) {
		LOG_ERR("timer alloc failed");
		return NULL;
	}

	t->cb = cb;
	t->arg = arg;
	t->periodic = periodic;

	k_timer_init(&t->timer, timer_expiry, NULL);

	return (osal_timer_t)t;
}

int osal_timer_start(osal_timer_t timer, uint32_t period_ms)
{
	struct osal_timer_cb *t = timer;
	k_timeout_t period;

	if (t == NULL) {
		return OSAL_INVAL;
	}

	period = t->periodic ? K_MSEC(period_ms) : K_NO_WAIT;
	k_timer_start(&t->timer, K_MSEC(period_ms), period);

	return OSAL_OK;
}

int osal_timer_stop(osal_timer_t timer)
{
	struct osal_timer_cb *t = timer;

	if (t == NULL) {
		return OSAL_INVAL;
	}

	k_timer_stop(&t->timer);

	return OSAL_OK;
}

void osal_timer_delete(osal_timer_t timer)
{
	struct osal_timer_cb *t = timer;

	if (t == NULL) {
		return;
	}

	k_timer_stop(&t->timer);
	osal_cb_free(t);
}
