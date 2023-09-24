/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2023 Intel Corporation
 */

#include <zephyr/kernel.h>
#include <zephyr/lw_sched/lw_sched.h>

extern void lw_task_insert(struct lw_scheduler *sched, struct lw_task *task);

static void timer_expiry(struct k_timer *timer)
{
	struct lw_scheduler *sched;

	/* Get the scheduler based on the timer */

	sched = CONTAINER_OF(timer, struct lw_scheduler, timer);

	k_sem_give(&sched->sem);   /* Wake the scheduler's thread */
}

static void lw_scheduler_reorder(struct lw_scheduler *sched)
{
	sys_dnode_t *reorder;
	struct lw_task *task;

	/*
	 * Loop through each node in the reorder list. Remove the associated
	 * task from the list of tasks and insert it at the correct priority.
	 */

	while ((reorder = sys_dlist_get(&sched->reorder)) != NULL) {
		task = CONTAINER_OF(reorder, struct lw_task, reorder);
		if (task->state != LW_TASK_ABORT) {
			if (sys_dnode_is_linked(&task->node)) {
				sys_dlist_remove(&task->node);
			}
			task->order = task->new_order;
			lw_task_insert(sched, task);
		}
	}
}

static void lw_scheduler_entry(void *p1, void *p2, void *p3)
{
	struct lw_scheduler *sched = p1;
	struct lw_task *task;
	sys_dnode_t *node;
	uint32_t  state;
	k_spinlock_key_t  key;
	sys_dnode_t  dummy;

	while (1) {

		/* Wait until the timer gives the semaphore */

		k_sem_take(&sched->sem, K_FOREVER);

		key = k_spin_lock(&sched->lock);
		node = sys_dlist_peek_head(&sched->list);
		while (node != NULL) {
			task = CONTAINER_OF(node, struct lw_task, node);
			sched->current = task;

			switch (task->state) {
			case LW_TASK_PAUSED:
				/* Task is paused. Nothing to do. */

				break;

			case LW_TASK_EXECUTE:
				if (task->delay > 0) {
					task->delay--;
					break;
				}

				k_spin_unlock(&sched->lock, key);

				state = task->ops->execute(task->args == NULL ?
							   NULL :
							   task->args->execute);

				key = k_spin_lock(&sched->lock);

				if (task->state != LW_TASK_ABORT) {
					task->state = state;
				}

				if (task->state != LW_TASK_ABORT) {
					break;
				}

				__fallthrough;

			case LW_TASK_ABORT:

				/*
				 * Remove the task from the list of tasks
				 * Use a dummy node to preserve information.
				 */

				dummy = task->node;
				sys_dlist_remove(&task->node);
				node = &dummy;
				break;

			default:
				break;
			}

			node = sys_dlist_peek_next(&sched->list, node);
		}

		sched->current = NULL;
		lw_scheduler_reorder(sched);
		k_spin_unlock(&sched->lock, key);
	}
}

struct lw_scheduler *lw_scheduler_init(struct lw_scheduler *sched,
				       k_thread_stack_t *stack,
				       size_t stack_size,
				       int priority, uint32_t options,
				       k_timeout_t  period)
{
	k_thread_create(&sched->thread, stack, stack_size,
			lw_scheduler_entry, sched, NULL, NULL,
			priority, options, K_FOREVER);

	k_timer_init(&sched->timer, timer_expiry, NULL);

	k_sem_init(&sched->sem, 0, 1);

	sched->lock = (struct k_spinlock){};
	sched->current = NULL;

	sys_dlist_init(&sched->list);
	sys_dlist_init(&sched->reorder);

	sched->period = period;

	return sched;
}

void lw_scheduler_start(struct lw_scheduler *sched, k_timeout_t delay)
{
	k_thread_start(&sched->thread);
	k_timer_start(&sched->timer, delay, sched->period);
}

void lw_scheduler_abort(struct lw_scheduler *sched)
{
	k_spinlock_key_t  key;
	sys_dnode_t *node;
	sys_dnode_t *next;
	struct lw_task *task;

	k_timer_stop(&sched->timer);

	k_thread_abort(&sched->thread);

	key = k_spin_lock(&sched->lock);
	SYS_DLIST_FOR_EACH_NODE_SAFE(&sched->list, node, next) {
		task = CONTAINER_OF(node, struct lw_task, node);

		sys_dlist_remove(node);
		if (task->ops->abort != NULL) {
			task->ops->abort(task->args == NULL ?
					 NULL : task->args->abort);
		}
	}
	k_spin_unlock(&sched->lock, key);
}

struct lw_task *lw_scheduler_current_get(struct lw_scheduler *sched)
{
	return sched->current;
}
