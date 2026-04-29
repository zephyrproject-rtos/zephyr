/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/workq.h>
#include <ksched.h>
#include <kthread.h>
#include <wait_q.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(workq, LOG_LEVEL_DBG);

/* private struct, only used locally */
struct active_work {
	uintptr_t work;
	sys_snode_t node;
};

/*FIXME: fake config_*/
#define CONFIG_WORKQ_DEFAULT_THREAD_PRIORITY 0
static struct workq_thread_config default_cfg = {
	.name = NULL,
	.prio = CONFIG_WORKQ_DEFAULT_THREAD_PRIORITY,
};

/* should only be called with lock held */
static inline bool workq_closed(struct workq *wq)
{
	return (wq->flags & WORKQ_FLAG_OPEN) == 0;
}

/* should only be called with lock held */
static inline bool workq_idle(struct workq *wq)
{
	return sys_slist_is_empty(&wq->pending) && sys_slist_is_empty(&wq->delayed) &&
		sys_slist_is_empty(&wq->active);
}

/* should only be called with lock held */
static inline bool pending(struct workq *wq, struct work_item *item)
{
	struct work_item *wi;

	SYS_SLIST_FOR_EACH_CONTAINER(&wq->pending, wi, node) {
		if (wi == item) {
			return true;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&wq->delayed, wi, node) {
		if (wi == item) {
			return true;
		}
	}

	return false;
}

/* should only be called with lock held */
static inline bool active(struct workq *wq, struct work_item *work)
{
	struct active_work *active;

	SYS_SLIST_FOR_EACH_CONTAINER(&wq->active, active, node) {
		if (active->work == (uintptr_t)work) {
			return true;
		}
	}

	return false;
}

/* should only be called with lock held */
static inline struct work_item *get_next_work(struct workq *wq)
{
	sys_snode_t *node = sys_slist_get(&wq->pending);

	if (node == NULL) {
		return NULL;
	}

	return CONTAINER_OF(node, struct work_item, node);
}

/* should only be called with lock held */
static inline void awake(struct workq *wq)
{
	z_sched_wake(&wq->idle, 0, NULL);
}

static void schedule_cb(struct _timeout *t);
/* should only be called with lock held */
static void schedule_next_timeout(struct workq *wq)
{
	struct work_item *item;

	if (sys_slist_is_empty(&wq->delayed)) {
		return;
	}

	item = CONTAINER_OF(sys_slist_peek_head(&wq->delayed), struct work_item, node);
	z_add_timeout(&wq->timeout, schedule_cb, sys_timepoint_timeout(item->exec_time));
}

/* should only be called with lock held */
static inline int cancel(struct workq *wq, struct work_item *item)
{
	if (active(wq, item)) {
		/* cannot cancel running item */
		return -EBUSY;
	}

	if (sys_slist_peek_head(&wq->delayed) == &item->node) {
		z_abort_timeout(&wq->timeout);
		sys_slist_find_and_remove(&wq->delayed, &item->node);
		schedule_next_timeout(wq);
	} else {
		sys_slist_find_and_remove(&wq->delayed, &item->node);
		sys_slist_find_and_remove(&wq->pending, &item->node);
	}

	return 0;
}

/* should only be called with lock held */
static inline void delayed_submit(struct workq *wq, struct work_item *item, k_timepoint_t exec_time)
{
	struct work_item *next;
	sys_snode_t *node, *prev = NULL;

	item->exec_time = exec_time;
	SYS_SLIST_FOR_EACH_NODE(&wq->delayed, node) {
		next = CONTAINER_OF(node, struct work_item, node);
		if (sys_timepoint_cmp(item->exec_time, next->exec_time) < 0) {
			sys_slist_insert(&wq->delayed, prev, &item->node);
			break;
		}
		prev = node;
	}

	if (node == NULL) {
		sys_slist_append(&wq->delayed, &item->node);
	}

	if (sys_slist_peek_head(&wq->delayed) == &item->node) {
		z_abort_timeout(&wq->timeout);
		schedule_next_timeout(wq);
	}
}


static inline bool thread_running(struct workq_thread *wqt)
{
	bool running;

	K_SPINLOCK(&wqt->lock) {
		running = (wqt->flags & WORKQ_THREAD_FLAG_RUNNING) != 0;
	}

	return running;
}

static void schedule_cb(struct _timeout *t)
{
	struct workq *wq = CONTAINER_OF(t, struct workq, timeout);
	struct work_item *item;

	K_SPINLOCK(&wq->lock) {
		while (!sys_slist_is_empty(&wq->delayed)) {
			item = CONTAINER_OF(sys_slist_peek_head(&wq->delayed), struct work_item, node);
			if (!sys_timepoint_expired(item->exec_time)) {
				break;
			}
			sys_slist_get(&wq->delayed);
			sys_slist_append(&wq->pending, &item->node);
		}
		schedule_next_timeout(wq);
		awake(wq);
	}
}

static inline int sleep(struct workq *wq, k_spinlock_key_t *key, k_timeout_t timeout)
{
	int rc = 0;

	rc = z_pend_curr(&wq->lock, *key, &wq->idle, timeout);
	*key = k_spin_lock(&wq->lock);

	return rc;
}

int workq_run(struct workq *wq, k_timeout_t timeout)
{
	int rc;
	work_fn_t fn;
	struct work_item *work;
	struct active_work active;
	k_spinlock_key_t key = k_spin_lock(&wq->lock);

	while ((work = get_next_work(wq)) == NULL) {
		rc = sleep(wq, &key, timeout);
		if (rc != 0) {
			k_spin_unlock(&wq->lock, key);
			return rc;
		} else if (workq_idle(wq)) {
			z_sched_wake_all(&wq->drain, 0, NULL);
		}
	}

	fn = work->fn;
	active.work = (uintptr_t)work;
	sys_slist_append(&wq->active, &active.node);
	k_spin_unlock(&wq->lock, key);

	__ASSERT(fn != NULL, "Work item has not been initialized properly");
	fn(work); /* "work" should be freeable during this function call */

	K_SPINLOCK(&wq->lock) {
		sys_slist_find_and_remove(&wq->active, &active.node);
		if (workq_idle(wq)) {
			z_sched_wake_all(&wq->drain, 0, NULL);
		}
	}

	return 0;
}

void workq_init(struct workq *wq)
{
	wq->lock = (struct k_spinlock){};
	sys_slist_init(&wq->active);
	sys_slist_init(&wq->pending);
	sys_slist_init(&wq->delayed);
	z_init_timeout(&wq->timeout);
	z_waitq_init(&wq->idle);
	z_waitq_init(&wq->drain);
	workq_open(wq);
}

void workq_open(struct workq *wq)
{
	K_SPINLOCK(&wq->lock) {
		wq->flags |= WORKQ_FLAG_OPEN;
		awake(wq);
	}
}

void workq_close(struct workq *wq)
{
	K_SPINLOCK(&wq->lock) {
		wq->flags &= ~WORKQ_FLAG_OPEN;
	}
}

void workq_freeze(struct workq *wq)
{
	K_SPINLOCK(&wq->lock) {
		wq->flags &= ~WORKQ_FLAG_OPEN;
		wq->flags |= WORKQ_FLAG_FROZEN;
		z_abort_timeout(&wq->timeout);
	}
}

void workq_thaw(struct workq *wq)
{
	K_SPINLOCK(&wq->lock) {
		if ((wq->flags & WORKQ_FLAG_FROZEN) == 0) {
			K_SPINLOCK_BREAK;
		}
		wq->flags &= ~WORKQ_FLAG_FROZEN;
		schedule_next_timeout(wq);
	}
}

void work_init(struct work_item *item, work_fn_t fn)
{
	item->fn = fn;
}

int workq_submit(struct workq *wq, struct work_item *item)
{
	int rc = 0;

	K_SPINLOCK(&wq->lock) {
		if (workq_closed(wq)) {
			rc = -EAGAIN;
			K_SPINLOCK_BREAK;
		}
		if (pending(wq, item)) {
			rc = -EALREADY;
			K_SPINLOCK_BREAK;
		}
		sys_slist_append(&wq->pending, &item->node);
		awake(wq);
	}

	return rc;
}

int workq_delayed_submit(struct workq *wq, struct work_item *item, k_timeout_t delay)
{
	int rc = 0;
	k_timepoint_t exec_time = sys_timepoint_calc(delay);

	K_SPINLOCK(&wq->lock) {
		if (workq_closed(wq)) {
			rc = -EBUSY;
			K_SPINLOCK_BREAK;
		}
		if (pending(wq, item)) {
			rc = -EALREADY;
			K_SPINLOCK_BREAK;
		}
		delayed_submit(wq, item, exec_time);
	}

	return rc;
}

int workq_reschedule(struct workq *wq, struct work_item *item, k_timeout_t delay)
{
	int rc = 0;
	k_timepoint_t exec_time = sys_timepoint_calc(delay);

	K_SPINLOCK(&wq->lock) {
		if (pending(wq, item)) {
			rc = cancel(wq, item);
			if (rc != 0) {
				K_SPINLOCK_BREAK;
			}
		}
		delayed_submit(wq, item, exec_time);
	}

	return rc;
}

int workq_cancel(struct workq *wq, struct work_item *item)
{
	int rc;

	K_SPINLOCK(&wq->lock) {
		if (active(wq, item)) {
			rc = -EBUSY;
			K_SPINLOCK_BREAK;
		} else if (!pending(wq, item)) {
			rc = -ENOENT;
			K_SPINLOCK_BREAK;
		}
		rc = cancel(wq, item);
	}

	return rc;
}

int workq_drain(struct workq *wq, k_timeout_t timeout)
{
	k_spinlock_key_t key = k_spin_lock(&wq->lock);

	if (workq_idle(wq)) {
		k_spin_unlock(&wq->lock, key);
		return 0;
	}

	return z_pend_curr(&wq->lock, key, &wq->drain, timeout);
}

void workq_thread_fn(void *arg1, void *arg2, void *arg3)
{
	int rc;
	struct workq_thread *wqt = (struct workq_thread *)arg1;

	LOG_DBG("[%p] Workq thread entered", &wqt->thread);
	while (thread_running(wqt)) {
		rc = workq_run(wqt->wq, K_FOREVER);
		if (unlikely(rc != 0)) {
			break;
		}
	}

	K_SPINLOCK(&wqt->lock) {
		wqt->flags &= ~WORKQ_THREAD_FLAG_RUNNING;
	}
	LOG_DBG("[%p] Workq thread exited", &wqt->thread);
}

void workq_thread_init(struct workq_thread *wt, struct workq *wq, k_thread_stack_t *stack,
		size_t stack_size, struct workq_thread_config *cfg)
{
	wt->wq = wq;
	wt->flags = WORKQ_THREAD_FLAG_INITIALIZED;
	wt->lock = (struct k_spinlock){};

	wt->stack = stack;
	wt->stack_size = stack_size;
	wt->cfg = cfg ? cfg : &default_cfg;
}

int workq_thread_start(struct workq_thread *wqt)
{
	int rc = 0;
	k_tid_t tid;

	K_SPINLOCK(&wqt->lock) {
		if ((wqt->flags & WORKQ_THREAD_FLAG_INITIALIZED) == 0) {
			LOG_ERR("Workq thread not initialized");
			rc = -ENODEV;
			K_SPINLOCK_BREAK;
		} else if ((wqt->flags & WORKQ_THREAD_FLAG_RUNNING) != 0) {
			LOG_ERR("Workq thread already running");
			rc = -EALREADY;
			K_SPINLOCK_BREAK;
		}
		tid = k_thread_create(&wqt->thread, wqt->stack, wqt->stack_size,
				workq_thread_fn, wqt, NULL, NULL,
				wqt->cfg->prio, 0, K_NO_WAIT);
		if (tid == NULL) {
			LOG_ERR("Failed to create workq thread");
			rc = -EINVAL;
			K_SPINLOCK_BREAK;
		}
		if (wqt->cfg->name) {
			if (k_thread_name_set(&wqt->thread, wqt->cfg->name)) {
				LOG_ERR("Failed to set workq thread name:%d", rc);
			}
		}
		wqt->flags |= WORKQ_THREAD_FLAG_RUNNING;
		LOG_DBG("[%p] Workq thread started", &wqt->thread);
	}
	return rc;
}

int workq_thread_stop(struct workq_thread *wqt, k_timeout_t timeout)
{
	k_spinlock_key_t key = k_spin_lock(&wqt->lock);
	k_spinlock_key_t key_q = k_spin_lock(&wqt->wq->lock);

	wqt->flags &= ~WORKQ_THREAD_FLAG_RUNNING;
	if (z_is_thread_pending(&wqt->thread)) {
		arch_thread_return_value_set(&wqt->thread, -ESHUTDOWN);
		z_unpend_thread(&wqt->thread);
		z_ready_thread(&wqt->thread);
	}
	k_spin_unlock(&wqt->wq->lock, key_q);
	k_spin_unlock(&wqt->lock, key);

	return k_thread_join(&wqt->thread, timeout);
}
