/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static struct k_work *observed_work;
static uintptr_t observed_sync;

bool __real_k_work_flush(struct k_work *work, struct k_work_sync *sync);

bool __wrap_k_work_flush(struct k_work *work, struct k_work_sync *sync)
{
	if (work == observed_work) {
		const struct k_thread *thread = k_current_get();
		uintptr_t start = thread->stack_info.start;

		observed_sync = (uintptr_t)sync;
		zassert_false(observed_sync >= start &&
				      observed_sync - start < thread->stack_info.size,
			      "queue flush synchronization storage is on the caller's stack");
	}

	return __real_k_work_flush(work, sync);
}

ZTEST(queue_completion, test_queue_free_uses_independent_sync_storage)
{
	struct virtq first;
	struct virtq second;
	uintptr_t first_sync;

	zassert_ok(virtq_create(&first, 2));
	zassert_ok(virtq_create(&second, 2));

	observed_work = &first.wake_work;
	observed_sync = 0;
	virtq_free(&first);
	first_sync = observed_sync;
	zassert_not_equal(first_sync, 0, "queue teardown did not flush its work");

	observed_work = &second.wake_work;
	observed_sync = 0;
	virtq_free(&second);
	observed_work = NULL;
	zassert_not_equal(observed_sync, 0, "queue teardown did not flush its work");
	zassert_not_equal(first_sync, observed_sync, "queues share flush synchronization storage");
}
