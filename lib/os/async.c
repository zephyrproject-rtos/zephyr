/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/async.h>
#include <zephyr/kernel.h>

void async_semaphore_cb(void *context, int result, void *sem_data)
{
	ARG_UNUSED(context);

	struct async_sem *a = (struct async_sem *)sem_data;

	a->result = result;

	k_sem_give(a->sem);
}

#ifdef CONFIG_POLL
void async_signal_cb(void *context, int result, void *sig_data)
{
	ARG_UNUSED(context);

	struct k_poll_signal *sig = (struct k_poll_signal *)sig_data;

	k_poll_signal_raise(sig, result);
}
#endif /* CONFIG_POLL */

void async_mutex_cb(void *context, int result, void *mutex_data)
{
	ARG_UNUSED(context);

	struct async_mutex *a = (struct async_mutex *)mutex_data;

	a->result = result;

	k_mutex_unlock(a->mutex);
}

void async_work_cb(void *context, int result, void *work_data)
{
	ARG_UNUSED(context);

	struct async_work *a = (struct async_work *)work_data;

	a->result = result;

	if (a->workq != NULL) {
		k_work_submit_to_queue(a->workq, a->work);
	} else {
		k_work_submit(a->work);
	}
}
