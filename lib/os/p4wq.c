/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/sys/p4wq.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/iterable_sections.h>
/* private kernel APIs */
#include <ksched.h>
#include <wait_q.h>

LOG_MODULE_REGISTER(p4wq, CONFIG_LOG_DEFAULT_LEVEL);

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

static void thread_set_requeued(struct k_thread *th)
{
	th->base.user_options |= K_CALLBACK_STATE;
}

static void thread_clear_requeued(struct k_thread *th)
{
	th->base.user_options &= ~K_CALLBACK_STATE;
}

static bool thread_was_requeued(struct k_thread *th)
{
	return !!(th->base.user_options & K_CALLBACK_STATE);
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
	} else {
		;
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
			thread_clear_requeued(_current);

			k_spin_unlock(&queue->lock, k);

			w->handler(w);

			k = k_spin_lock(&queue->lock);

			/* Remove from the active list only if it
			 * wasn't resubmitted already
			 */
			if (!thread_was_requeued(_current)) {
				sys_dlist_remove(&w->dlnode);
				w->thread = NULL;

				if (queue->done_handler) {
					k_spin_unlock(&queue->lock, k);
					queue->done_handler(w);
					k = k_spin_lock(&queue->lock);
				} else {
					k_sem_give(&w->done_sem);
				}
			}
		} else {
			z_pend_curr(&queue->lock, k, &queue->waitq, K_FOREVER);
			k = k_spin_lock(&queue->lock);
		}
	}
}

/* Must be called to regain ownership of the work item */
int k_p4wq_wait(struct k_p4wq_work *work, k_timeout_t timeout)
{
	if (work->sync) {
		return k_sem_take(&work->done_sem, timeout);
	}

	return k_sem_count_get(&work->done_sem) ? 0 : -EBUSY;
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
			K_HIGHEST_THREAD_PRIO, 0,
			queue->flags & K_P4WQ_DELAYED_START ? K_FOREVER : K_NO_WAIT);
}

static int static_init(void)
{

	STRUCT_SECTION_FOREACH(k_p4wq_initparam, pp) {
		for (int i = 0; i < pp->num; i++) {
			uintptr_t ssz = K_THREAD_STACK_LEN(pp->stack_size);
			struct k_p4wq *q = pp->flags & K_P4WQ_QUEUE_PER_THREAD ?
				pp->queue + i : pp->queue;

			if (!i || (pp->flags & K_P4WQ_QUEUE_PER_THREAD)) {
				k_p4wq_init(q);
				q->done_handler = pp->done_handler;
			}

			q->flags = pp->flags;

			/*
			 * If the user wants to specify CPU affinity, we have to
			 * delay starting threads until that has been done
			 */
			if (q->flags & K_P4WQ_USER_CPU_MASK) {
				q->flags |= K_P4WQ_DELAYED_START;
			}

			k_p4wq_add_thread(q, &pp->threads[i],
					  &pp->stacks[ssz * i],
					  pp->stack_size);

#ifdef CONFIG_SCHED_CPU_MASK
			if (pp->flags & K_P4WQ_USER_CPU_MASK) {
				int ret = k_thread_cpu_mask_clear(&pp->threads[i]);

				if (ret < 0) {
					LOG_ERR("Couldn't clear CPU mask: %d", ret);
				}
			}
#endif
		}
	}

	return 0;
}

void k_p4wq_enable_static_thread(struct k_p4wq *queue, struct k_thread *thread,
				 uint32_t cpu_mask)
{
#ifdef CONFIG_SCHED_CPU_MASK
	if (queue->flags & K_P4WQ_USER_CPU_MASK) {
		unsigned int i;

		while ((i = find_lsb_set(cpu_mask))) {
			int ret = k_thread_cpu_mask_enable(thread, i - 1);

			if (ret < 0) {
				LOG_ERR("Couldn't set CPU mask for %u: %d", i, ret);
			}
			cpu_mask &= ~BIT(i - 1);
		}
	}
#endif

	if (queue->flags & K_P4WQ_DELAYED_START) {
		k_thread_start(thread);
	}
}

/* We spawn a bunch of high priority threads, use the "SMP" initlevel
 * so they can initialize in parallel instead of serially on the main
 * CPU.
 */
#if defined(CONFIG_P4WQ_INIT_STAGE_EARLY)
SYS_INIT(static_init, POST_KERNEL, 1);
#else
SYS_INIT(static_init, APPLICATION, 99);
#endif

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
		thread_set_requeued(_current);
		item->thread = NULL;
	} else {
		k_sem_init(&item->done_sem, 0, 1);
	}
	__ASSERT_NO_MSG(item->thread == NULL);

	rb_insert(&queue->queue, &item->rbnode);
	item->queue = queue;

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
	uint32_t n_beaten_by = 0, active_target = arch_num_cpus();

	SYS_DLIST_FOR_EACH_CONTAINER(&queue->active, wi, dlnode) {
		/*
		 * item_lessthan(a, b) == true means a has lower priority than b
		 * !item_lessthan(a, b) counts all work items with higher or
		 * equal priority
		 */
		if (!item_lessthan(wi, item)) {
			n_beaten_by++;
		}
	}

	if (n_beaten_by >= active_target) {
		/* Too many already have higher priority, not preempting */
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

		if (queue->done_handler) {
			k_spin_unlock(&queue->lock, k);
			queue->done_handler(item);
			k = k_spin_lock(&queue->lock);
		} else {
			k_sem_give(&item->done_sem);
		}
	}

	k_spin_unlock(&queue->lock, k);
	return ret;
}
