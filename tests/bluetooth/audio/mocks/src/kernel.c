/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/slist.h>

#include "mock_kernel.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(z_timeout_remaining)                                                                  \
	FAKE(k_work_cancel_delayable_sync)                                                         \

/* List of k_work items to be worked. */
static sys_slist_t work_pending;

DEFINE_FAKE_VALUE_FUNC(k_ticks_t, z_timeout_remaining, const struct _timeout *);
DEFINE_FAKE_VALUE_FUNC(bool, k_work_cancel_delayable_sync, struct k_work_delayable *,
		       struct k_work_sync *);
DEFINE_FAKE_VALUE_FUNC(int, k_sem_take, struct k_sem *, k_timeout_t);
DEFINE_FAKE_VOID_FUNC(k_sem_give, struct k_sem *);

void k_work_init_delayable(struct k_work_delayable *dwork, k_work_handler_t handler)
{
	dwork->work.handler = handler;
}

int k_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	bool on_list = false;
	struct k_work *work;

	dwork->timeout.dticks = delay.ticks;

	/* Determine whether the work item is queued already. */
	SYS_SLIST_FOR_EACH_CONTAINER(&work_pending, work, node) {
		on_list = work == &dwork->work;
		if (on_list) {
			break;
		}
	}

	if (dwork->timeout.dticks == 0) {
		dwork->work.handler(&dwork->work);
		if (on_list) {
			(void)sys_slist_remove(&work_pending, NULL, &dwork->work.node);
		}
	} else if (!on_list) {
		sys_slist_append(&work_pending, &dwork->work.node);
	}

	return 0;
}

int k_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	struct k_work *work;

	/* Determine whether the work item is queued already. */
	SYS_SLIST_FOR_EACH_CONTAINER(&work_pending, work, node) {
		if (work == &dwork->work) {
			return 0;
		}
	}

	dwork->timeout.dticks = delay.ticks;
	if (dwork->timeout.dticks == 0) {
		dwork->work.handler(&dwork->work);
	} else {
		sys_slist_append(&work_pending, &dwork->work.node);
	}

	return 0;
}

int k_work_cancel_delayable(struct k_work_delayable *dwork)
{
	(void)sys_slist_find_and_remove(&work_pending, &dwork->work.node);

	return 0;
}

int k_work_cancel(struct k_work *work)
{
	(void)sys_slist_find_and_remove(&work_pending, &work->node);

	return 0;
}

void k_work_init(struct k_work *work, k_work_handler_t handler)
{
	work->handler = handler;
}

int k_work_submit(struct k_work *work)
{
	work->handler(work);

	return 0;
}

int k_work_busy_get(const struct k_work *work)
{
	return 0;
}

int32_t k_sleep(k_timeout_t timeout)
{
	struct k_work *work;

	SYS_SLIST_FOR_EACH_CONTAINER(&work_pending, work, node) {
		if (work->flags & K_WORK_DELAYED) {
			struct k_work_delayable *dwork = k_work_delayable_from_work(work);

			if (dwork->timeout.dticks > timeout.ticks) {
				dwork->timeout.dticks -= timeout.ticks;
				continue;
			}
		}

		(void)sys_slist_remove(&work_pending, NULL, &work->node);
		work->handler(work);
	}

	return 0;
}

void mock_kernel_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);

	sys_slist_init(&work_pending);
}

void mock_kernel_cleanup(void)
{
	struct k_work *work, *tmp;

	/* Run all pending works */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&work_pending, work, tmp, node) {
		(void)sys_slist_remove(&work_pending, NULL, &work->node);
		work->handler(work);
	}
}
