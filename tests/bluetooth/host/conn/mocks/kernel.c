/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include "kernel.h"

DEFINE_FAKE_VALUE_FUNC(bool, k_is_in_isr);
DEFINE_FAKE_VALUE_FUNC(int, k_poll_signal_raise, struct k_poll_signal *, int);
DEFINE_FAKE_VALUE_FUNC(int, k_sem_take, struct k_sem *, k_timeout_t);
DEFINE_FAKE_VALUE_FUNC(unsigned int, k_sem_count_get, struct k_sem *);
DEFINE_FAKE_VOID_FUNC(k_sem_give, struct k_sem *);
DEFINE_FAKE_VALUE_FUNC(k_tid_t, k_sched_current_thread_query);
DEFINE_FAKE_VOID_FUNC(k_work_init, struct k_work *, k_work_handler_t);
DEFINE_FAKE_VOID_FUNC(k_work_init_delayable, struct k_work_delayable *, k_work_handler_t);
DEFINE_FAKE_VALUE_FUNC(int, k_work_cancel_delayable, struct k_work_delayable *);
DEFINE_FAKE_VALUE_FUNC(bool, k_work_flush, struct k_work *, struct k_work_sync *);
DEFINE_FAKE_VALUE_FUNC(int, k_work_submit, struct k_work *);
DEFINE_FAKE_VALUE_FUNC(int, k_work_submit_to_queue, struct k_work_q *, struct k_work *);
DEFINE_FAKE_VALUE_FUNC(int, k_work_reschedule, struct k_work_delayable *, k_timeout_t);
DEFINE_FAKE_VALUE_FUNC(int, k_work_schedule, struct k_work_delayable *, k_timeout_t);
DEFINE_FAKE_VALUE_FUNC(int, k_work_busy_get, const struct k_work *);
DEFINE_FAKE_VOID_FUNC(k_queue_init, struct k_queue *);
DEFINE_FAKE_VOID_FUNC(k_queue_append, struct k_queue *, void *);
DEFINE_FAKE_VALUE_FUNC(int, k_queue_is_empty, struct k_queue *);
DEFINE_FAKE_VALUE_FUNC(void *, k_queue_get, struct k_queue *, k_timeout_t);
DEFINE_FAKE_VOID_FUNC(k_queue_prepend, struct k_queue *, void *);
DEFINE_FAKE_VALUE_FUNC(void *, k_heap_alloc, struct k_heap *, size_t, k_timeout_t);
DEFINE_FAKE_VOID_FUNC(k_heap_free, struct k_heap *, void *);
DEFINE_FAKE_VOID_FUNC(k_sched_lock);
DEFINE_FAKE_VOID_FUNC(k_sched_unlock);
DEFINE_FAKE_VALUE_FUNC(void *, k_heap_aligned_alloc, struct k_heap *,
			size_t, size_t, k_timeout_t);

struct k_work_q k_sys_work_q;
