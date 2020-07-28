/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log.h>
#include <sys/p4wq.h>
#include <wait_q.h>
#include <ksched.h>
#include <init.h>

LOG_MODULE_REGISTER(p4wq);

struct device;

static void set_prio(struct k_thread *th, struct k_p4wq_work *item)
{
	__ASSERT_NO_MSG(!IS_ENABLED(CONFIG_SMP) || !z_is_thread_queued(th));
	th->base.prio = item->priority;
	th->base.prio_deadline = item->deadline;
}

static bool rb_lessthan(struct rbnode *a, struct rbnode *b)
{
	struct k_p4wq_work *aw = CONTAINER_OF(a, struct k_p4wq_work, rbnode);
	struct k_p4wq_work *bw = CONTAINER_OF(b, struct k_p4wq_work, rbnode);

	if (aw->priority != bw->priority) {
		return aw->priority > bw->priority;
	}

	if (aw->deadline != bw->deadline) {
		return aw->deadline - bw->deadline > 0;
	}

	return (uintptr_t)a < (uintptr_t)b;
}

/* Slightly different semantics: rb_lessthan must be perfectly
 * symmetric (to produce a single tree structure) and will use the
 * pointer value to break ties where priorities are equal, here we
 * tolerate equality as meaning "not lessthan"
 */
static inline bool item_lessthan(struct k_p4wq_work *a, struct k_p4wq_work *b)
{
	if (a->priority > b->priority) {
		return true;
	} else if ((a->priority == b->priority) &&
		   (a->deadline != b->deadline)) {
		return a->deadline - b->deadline > 0;
	}
	return false;
}

static FUNC_NORETURN void p4wq_loop(void *p0, void *p1, void *p2)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	struct k_p4wq *queue = p0;
	k_spinlock_key_t k = k_spin_lock(&queue->lock);

	while (true) {
		struct rbnode *r = rb_get_max(&queue->queue);

		if (r) {
			struct k_p4wq_work *w
				= CONTAINER_OF(r, struct k_p4wq_work, rbnode);

			rb_remove(&queue->queue, r);
			w->thread = _current;
			sys_dlist_append(&queue->active, &w->dlnode);
			set_prio(_current, w);

			k_spin_unlock(&queue->lock, k);
			w->handler(w);
			k = k_spin_lock(&queue->lock);

			/* Remove from the active list only if it
			 * wasn't resubmitted already
			 */
			if (w->thread == _current) {
				sys_dlist_remove(&w->dlnode);
				w->thread = NULL;
			}
		} else {
			z_pend_curr(&queue->lock, k, &queue->waitq, K_FOREVER);
			k = k_spin_lock(&queue->lock);
		}
	}
}

void k_p4wq_init(struct k_p4wq *queue)
{
	memset(queue, 0, sizeof(*queue));
	z_waitq_init(&queue->waitq);
	queue->queue.lessthan_fn = rb_lessthan;
	sys_dlist_init(&queue->active);
}

void k_p4wq_add_thread(struct k_p4wq *queue, struct k_thread *thread,
			k_thread_stack_t *stack,
			size_t stack_size)
{
	k_thread_create(thread, stack, stack_size,
			p4wq_loop, queue, NULL, NULL,
			K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);
}

static int static_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	Z_STRUCT_SECTION_FOREACH(k_p4wq_initparam, pp) {
		k_p4wq_init(pp->queue);
		for (int i = 0; i < pp->num; i++) {
			uintptr_t ssz = K_THREAD_STACK_LEN(pp->stack_size);

			k_p4wq_add_thread(pp->queue, &pp->threads[i],
					  &pp->stacks[ssz * i],
					  pp->stack_size);
		}
	}
	return 0;
}

/* We spawn a bunch of high priority threads, use the "SMP" initlevel
 * so they can initialize in parallel instead of serially on the main
 * CPU.
 */
SYS_INIT(static_init, SMP, 99);

void k_p4wq_submit(struct k_p4wq *queue, struct k_p4wq_work *item)
{
	k_spinlock_key_t k = k_spin_lock(&queue->lock);

	/* Input is a delta time from now (to match
	 * k_thread_deadline_set()), but we store and use the absolute
	 * cycle count.
	 */
	item->deadline += k_cycle_get_32();

	/* Resubmission from within handler?  Remove from active list */
	if (item->thread == _current) {
		sys_dlist_remove(&item->dlnode);
		item->thread = NULL;
	}
	__ASSERT_NO_MSG(item->thread == NULL);

	rb_insert(&queue->queue, &item->rbnode);

	/* If there were other items already ahead of it in the queue,
	 * then we don't need to revisit active thread state and can
	 * return.
	 */
	if (rb_get_max(&queue->queue) != &item->rbnode) {
		goto out;
	}

	/* Check the list of active (running or preempted) items, if
	 * there are at least an "active target" of those that are
	 * higher priority than the new item, then no one needs to be
	 * preempted and we can return.
	 */
	struct k_p4wq_work *wi;
	uint32_t n_beaten_by = 0, active_target = CONFIG_MP_NUM_CPUS;

	SYS_DLIST_FOR_EACH_CONTAINER(&queue->active, wi, dlnode) {
		if (!item_lessthan(wi, item)) {
			n_beaten_by++;
		}
	}

	if (n_beaten_by >= active_target) {
		goto out;
	}

	/* Grab a thread, set its priority and queue it.  If there are
	 * no threads available to unpend, this is a soft runtime
	 * error: we are breaking our promise about run order.
	 * Complain.
	 */
	struct k_thread *th = z_unpend_first_thread(&queue->waitq);

	if (th == NULL) {
		LOG_WRN("Out of worker threads, priority guarantee violated");
		goto out;
	}

	set_prio(th, item);
	z_ready_thread(th);
	z_reschedule(&queue->lock, k);
	return;

out:
	k_spin_unlock(&queue->lock, k);
}

bool k_p4wq_cancel(struct k_p4wq *queue, struct k_p4wq_work *item)
{
	k_spinlock_key_t k = k_spin_lock(&queue->lock);
	bool ret = rb_contains(&queue->queue, &item->rbnode);

	if (ret) {
		rb_remove(&queue->queue, &item->rbnode);
	}

	k_spin_unlock(&queue->lock, k);
	return ret;
}
