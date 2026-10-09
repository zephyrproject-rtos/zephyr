/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/zassert.h>
#include <zephyr/call_offloader/call_offloader.h>

#include <wait_q.h>
#include <ksched.h>
#include <kspinlock.h>
#include <scheduler.h>
#include <kernel_internal.h>

ZASSERT_MODULE(DEFAULT);

#define CALL_OFFLOADER_PARK_PRIO K_PRIO_COOP(CONFIG_NUM_METAIRQ_PRIORITIES)

#define CALL_OFFLOADER_WORKER_DIED 0
#define CALL_OFFLOADER_WORK_DONE 1

static void engine_thread(void *p1, void *p2, void *p3);

static void create_worker(struct call_offloader_engine *engine,
			  struct call_offloader_worker *worker)
{
	worker->client = NULL;
	k_thread_create(&worker->thread, worker->stack, engine->stack_size, engine_thread,
			engine, worker, NULL, CALL_OFFLOADER_PARK_PRIO, 0, K_FOREVER);
	k_thread_name_set(&worker->thread, engine->name);
}

static void park_worker(struct call_offloader_engine *engine,
			struct call_offloader_worker *worker)
{
	LOCK_SCHED_SPINLOCK {
		z_mark_thread_as_not_sleeping(&worker->thread);
		z_sched_add_to_waitq_locked(&worker->thread, &engine->idle);
	}
}

static void call_offloader_init(void)
{
	STRUCT_SECTION_FOREACH(call_offloader_engine, engine) {
		for (size_t i = 0; i < engine->num_workers; i++) {
			create_worker(engine, &engine->workers[i]);
			park_worker(engine, &engine->workers[i]);
		}
	}
}

K_KERNEL_INIT_POST(call_offloader_init);

struct call_offloader_req {
	call_offloader_work_fn_t fn;
	void *args;
};

struct pending_count {
	int prio;
	size_t count;
};

static int count_pending_walk_op(struct k_thread *thread, void *data)
{
	struct pending_count *ctx = (struct pending_count *)data;

	if (z_is_prio_higher_or_equal(k_thread_priority_get(thread), ctx->prio)) {
		ctx->count++;
	}

	return 0;
}

static ALWAYS_INLINE bool is_engine_worker(struct call_offloader_engine *engine,
					   struct k_thread *thread)
{
	for (size_t i = 0; i < engine->num_workers; i++) {
		if (thread == &engine->workers[i].thread) {
			return true;
		}
	}

	return false;
}

static ALWAYS_INLINE void boost_lowest_priority_worker(struct call_offloader_engine *engine,
						       int prio)
{
	struct pending_count pending = { .prio = prio, .count = 1 };
	struct call_offloader_worker *lowest = NULL;
	struct call_offloader_worker *worker;
	int lowest_prio = prio;
	int worker_prio;
	size_t have = 0;

	z_sched_waitq_walk(&engine->pending, count_pending_walk_op, NULL, &pending);
	for (size_t i = 0; i < engine->num_workers; i++) {
		worker = &engine->workers[i];
		if (worker->client == NULL && !z_is_thread_ready(&worker->thread)) {
			continue;
		}

		worker_prio = k_thread_priority_get(&worker->thread);

		if (z_is_prio_higher_or_equal(worker_prio, prio)) {
			have++;
		} else if (z_is_prio_higher(lowest_prio, worker_prio)) {
			lowest_prio = worker_prio;
			lowest = worker;
		}
	}

	if (lowest != NULL && have < pending.count) {
		z_thread_prio_set(&lowest->thread, prio);
	}
}

static ALWAYS_INLINE bool wake_idle_worker(struct call_offloader_engine *engine, int prio)
{
	struct k_thread *worker;
	bool woke = false;

	LOCK_SCHED_SPINLOCK {
		worker = z_unpend_first_thread_locked(&engine->idle);
		if (worker != NULL) {
			worker->base.prio = prio;
			z_sched_ready_locked(worker);
			woke = true;
		}
	}

	return woke;
}

static void respawn_dead_worker(struct call_offloader_engine *engine)
{
	struct k_thread *head = NULL;
	struct call_offloader_worker *dead = NULL;

	K_SPINLOCK(&engine->lock) {
		for (size_t i = 0; i < engine->num_workers; i++) {
			if (engine->workers[i].client == _current) {
				dead = &engine->workers[i];
				dead->client = NULL;
				int dead_prio = k_thread_priority_get(&dead->thread);

				if (z_is_prio_higher(dead_prio, k_thread_priority_get(_current))) {
					z_thread_prio_set(_current, dead_prio);
				}
				break;
			}
		}
	}

	if (dead != NULL) {
		k_thread_join(&dead->thread, K_FOREVER);
		create_worker(engine, dead);

		K_SPINLOCK(&engine->lock) {
			park_worker(engine, dead);
			head = z_waitq_head(&engine->pending);
			if (head != NULL) {
				wake_idle_worker(engine, k_thread_priority_get(head));
			}
		}
	}
}

void call_offloader_dispatch(struct call_offloader_engine *engine, call_offloader_work_fn_t fn,
			     void *args)
{
	ZASSERT(engine != NULL && fn != NULL);
	ZASSERT(!k_is_in_isr(), "Cannot dispatch from ISR context");
	ZASSERT(!k_is_pre_kernel(), "Cannot dispatch before the kernel has started");

	if (is_engine_worker(engine, _current)) {
		fn(args);
		return;
	}

	k_spinlock_key_t key;
	int prio = k_thread_priority_get(_current);
	struct call_offloader_req req = {
		.fn = fn,
		.args = args
	};
	_current->base.swap_data = &req;
	key = k_spin_lock(&engine->lock);

	if (!wake_idle_worker(engine, prio)) {
		boost_lowest_priority_worker(engine, prio);
	}

	if (z_pend_curr(&engine->lock, key, &engine->pending, K_FOREVER) ==
	    CALL_OFFLOADER_WORKER_DIED) {
		respawn_dead_worker(engine);
		k_thread_abort(_current);
		CODE_UNREACHABLE;
	}
}

static ALWAYS_INLINE void adopt_client_context(struct k_thread *worker, struct k_thread *client)
{
	int prio = k_thread_priority_get(client);

	if (worker->base.prio != prio) {
		z_thread_prio_set(worker, prio);
	}
	worker->resource_pool = client->resource_pool;
#ifdef CONFIG_THREAD_CUSTOM_DATA
	worker->custom_data = client->custom_data;
#endif
#ifdef CONFIG_SCHED_DEADLINE
	worker->base.prio_deadline = client->base.prio_deadline;
#endif
}

static void engine_thread(void *p1, void *p2, void *p3)
{
	struct call_offloader_engine *engine = (struct call_offloader_engine *)p1;
	struct call_offloader_worker *self = (struct call_offloader_worker *)p2;
	struct call_offloader_req *req;
	k_spinlock_key_t key;

	ARG_UNUSED(p3);
	while (true) {
		key = k_spin_lock(&engine->lock);

		LOCK_SCHED_SPINLOCK {
			self->client = z_unpend_first_thread_locked(&engine->pending);
			if (self->client != NULL) {
				z_sched_add_to_waitq_locked(self->client, &self->thread.join_queue);
			}
		}

		if (self->client == NULL) {
			z_pend_curr(&engine->lock, key, &engine->idle, K_FOREVER);
			continue;
		}

		adopt_client_context(_current, self->client);
		req = (struct call_offloader_req *)self->client->base.swap_data;
		z_reschedule(&engine->lock, key);

		req->fn(req->args);

		K_SPINLOCK(&engine->lock) {
			LOCK_SCHED_SPINLOCK {
#ifdef CONFIG_THREAD_CUSTOM_DATA
				self->client->custom_data = _current->custom_data;
#endif
				z_thread_return_value_set_with_data(self->client,
					CALL_OFFLOADER_WORK_DONE, NULL);
				z_sched_wake_thread_locked(self->client);
				self->client = NULL;
			}
		}
	}
}
