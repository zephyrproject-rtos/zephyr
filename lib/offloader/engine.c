/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/zassert.h>
#include <zephyr/offloader/engine.h>

#include <wait_q.h>
#include <ksched.h>
#include <kspinlock.h>
#include <scheduler.h>
#include <kernel_internal.h>

ZASSERT_MODULE(DEFAULT);

struct offloader_req {
	offloader_work_fn_t fn;
	void *args;
};

struct pending_count {
	int prio;
	size_t count;
};

static int count_pending(struct k_thread *thread, void *data)
{
	struct pending_count *ctx = (struct pending_count *)data;

	if (!z_is_prio_higher_or_equal(k_thread_priority_get(thread), ctx->prio)) {
		return 1;
	}

	ctx->count++;

	return 0;
}

static ALWAYS_INLINE bool engine_worker(struct offloader_engine *engine)
{
	struct offloader_worker *self = CONTAINER_OF(_current, struct offloader_worker, thread);

	return self >= engine->workers && self < &engine->workers[engine->num_workers];
}

static ALWAYS_INLINE void promote_priority(struct offloader_engine *engine, int prio)
{
	struct pending_count pending = { .prio = prio, .count = 1 };
	struct offloader_worker *lowest = NULL;
	struct offloader_worker *worker;
	int lowest_prio = prio;
	size_t have = 0;

	(void)z_sched_waitq_walk(&engine->pending, count_pending, NULL, &pending);

	for (size_t i = 0; i < engine->num_workers; i++) {
		worker = &engine->workers[i];
		if (worker->client == NULL) {
			continue;
		}

		if (z_is_prio_higher_or_equal(k_thread_priority_get(&worker->thread), prio)) {
			have++;
		} else if (z_is_prio_higher(lowest_prio, k_thread_priority_get(&worker->thread))) {
			lowest_prio = k_thread_priority_get(&worker->thread);
			lowest = worker;
		}
	}

	if (lowest != NULL && have < pending.count) {
		(void)z_thread_prio_set(&lowest->thread, prio);
	}
}

void offloader_dispatch(struct offloader_engine *engine, offloader_work_fn_t fn, void *args)
{
	k_spinlock_key_t key;
	struct offloader_req req;

	ZASSERT(engine != NULL && fn != NULL);
	ZASSERT(!k_is_in_isr(), "Cannot dispatch from ISR context");

	if (engine_worker(engine)) {
		fn(args);
		return;
	}

	req = (struct offloader_req) {
		.fn = fn,
		.args = args
	};
	_current->base.swap_data = &req;

	key = k_spin_lock(&engine->lock);

	if (!z_sched_wake(&engine->idle, 0, NULL)) {
		promote_priority(engine, k_thread_priority_get(_current));
	}

	z_pend_curr(&engine->lock, key, &engine->pending, K_FOREVER);
}

void offloader_engine_thread(void *p1, void *p2, void *p3)
{
	struct offloader_engine *engine = (struct offloader_engine *)p1;
	struct offloader_worker *self = (struct offloader_worker *)p2;
	struct offloader_req *req;
	k_spinlock_key_t key;

	ARGS_UNUSED(p3);

	while (true) {
		key = k_spin_lock(&engine->lock);

		LOCK_SCHED_SPINLOCK {
			if (self->client != NULL) {
				z_thread_return_value_set_with_data(self->client, 0, NULL);
				z_sched_wake_thread_locked(self->client);
			}

			self->client = z_unpend_first_thread_locked(&engine->pending);
			if (self->client != NULL) {
				z_sched_add_to_waitq_locked(self->client, &engine->running);
			}
		}

		if (self->client == NULL) {
			z_thread_prio_set(_current, OFFLOADER_IDLE_PRIO);
			z_pend_curr(&engine->lock, key, &engine->idle, K_FOREVER);
			continue;
		}

		z_thread_prio_set(_current, k_thread_priority_get(self->client));
		req = (struct offloader_req *)self->client->base.swap_data;
		z_reschedule(&engine->lock, key);

		req->fn(req->args);
	}
}
