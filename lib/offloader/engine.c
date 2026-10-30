/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/zassert.h>
#include <zephyr/offloader/engine.h>

#include <ksched.h>
#include <kspinlock.h>
#include <wait_q.h>

ZASSERT_MODULE(DEFAULT);

#define OFFLOADER_IDLE_PRIO K_PRIO_COOP(0)

struct active_worker {
	sys_snode_t node;
	k_tid_t tid;
	int prio;
};

struct offloader_req {
	sys_snode_t node;
	_wait_q_t waitq;
	offloader_work_fn_t fn;
	void *args;
	int prio;
};

static ALWAYS_INLINE void req_queue(struct offloader_engine *engine, struct offloader_req *req)
{
	struct offloader_req *queued;
	sys_snode_t *prev = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&engine->queue, queued, node) {
		if (z_is_prio_higher(req->prio, queued->prio)) {
			break;
		}
		prev = &queued->node;
	}

	sys_slist_insert(&engine->queue, prev, &req->node);
}

static ALWAYS_INLINE struct offloader_req *req_dequeue(struct offloader_engine *engine)
{
	sys_snode_t *node = sys_slist_get(&engine->queue);

	return node == NULL ? NULL : CONTAINER_OF(node, struct offloader_req, node);
}

static ALWAYS_INLINE void wait_for_completion(struct offloader_engine *engine,
				struct offloader_req *req, k_spinlock_key_t key)
{
	z_pend_curr(&engine->lock, key, &req->waitq, K_FOREVER);
}

static ALWAYS_INLINE void wakeup_caller(struct offloader_engine *engine, struct offloader_req *req,
			  struct active_worker *self)
{
	k_spinlock_key_t key = k_spin_lock(&engine->lock);

	sys_slist_find_and_remove(&engine->active, &self->node);

	if (z_sched_wake(&req->waitq, 0, NULL)) {
		z_reschedule(&engine->lock, key);
	} else {
		k_spin_unlock(&engine->lock, key);
	}
}

static ALWAYS_INLINE bool wakeup_worker(struct offloader_engine *engine)
{
	struct k_thread *worker = NULL;

	LOCK_SCHED_SPINLOCK {
		worker = z_unpend_first_thread_locked(&engine->waitq);
		if (worker != NULL) {
			z_sched_ready_locked(worker);
		}
	}

	return worker != NULL;
}

static ALWAYS_INLINE bool engine_worker(struct offloader_engine *engine)
{
	return _current >= engine->workers && _current < &engine->workers[engine->num_workers];
}

static ALWAYS_INLINE void req_init(struct offloader_req *req, offloader_work_fn_t fn, void *args)
{
	req->fn = fn;
	req->args = args;
	req->prio = _current->base.prio;
	z_waitq_init(&req->waitq);
}

static ALWAYS_INLINE void promote_priority(struct offloader_engine *engine, int prio)
{
	struct active_worker *lowest = NULL;
	struct offloader_req *queued;
	struct active_worker *worker;
	int lowest_prio = prio;
	size_t need = 0;
	size_t have = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&engine->queue, queued, node) {
		if (z_is_prio_higher_or_equal(queued->prio, prio)) {
			need++;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&engine->active, worker, node) {
		if (z_is_prio_higher_or_equal(worker->prio, prio)) {
			have++;
		} else if (z_is_prio_higher(lowest_prio, worker->prio)) {
			lowest_prio = worker->prio;
			lowest = worker;
		}
	}

	if (lowest != NULL && have < need) {
		z_thread_prio_set(lowest->tid, prio);
		lowest->prio = prio;
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

	req_init(&req, fn, args);
	key = k_spin_lock(&engine->lock);

	req_queue(engine, &req);
	if (!wakeup_worker(engine)) {
		promote_priority(engine, req.prio);
	}

	wait_for_completion(engine, &req, key);
}

void offloader_engine_thread(void *p1, void *p2, void *p3)
{
	struct offloader_engine *engine = (struct offloader_engine *)p1;
	struct offloader_req *req;
	struct active_worker self = { .tid = _current };
	k_spinlock_key_t key;

	ARGS_UNUSED(p2, p3);

	while (true) {
		key = k_spin_lock(&engine->lock);
		req = req_dequeue(engine);
		if (req == NULL) {
			z_pend_curr(&engine->lock, key, &engine->waitq, K_FOREVER);
			continue;
		}
		sys_slist_append(&engine->active, &self.node);
		self.prio = req->prio;
		z_thread_prio_set(_current, req->prio);
		z_reschedule(&engine->lock, key);

		req->fn(req->args);
		wakeup_caller(engine, req, &self);
		z_thread_prio_set(_current, OFFLOADER_IDLE_PRIO);
	}
}
