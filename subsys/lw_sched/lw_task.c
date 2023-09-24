/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2023 Intel Corporation
 */

#include <zephyr/kernel.h>
#include <zephyr/lw_sched/lw_sched.h>

static int order_compare(sys_dnode_t *node, void *data)
{
	struct lw_task *task;
	uint32_t order = (uint32_t)(uintptr_t)data;

	task = CONTAINER_OF(node, struct lw_task, node);

	return task->order < order ? 0 : 1;
}

void lw_task_insert(struct lw_scheduler *sched, struct lw_task *task)
{
	sys_dlist_insert_at(&sched->list, &task->node,
			    order_compare, (void *)(uintptr_t)task->order);
}

struct lw_task *lw_task_init(struct lw_task *task,
			     struct lw_task_ops *ops,
			     struct lw_task_args *args,
			     struct lw_scheduler *sched,
			     uint32_t order)
{
	k_spinlock_key_t  key;

	if ((task == NULL) || (ops == NULL) || (sched == NULL) ||
	    (ops->execute == NULL)) {
		return NULL;
	}

	key = k_spin_lock(&sched->lock);

	task->sched = sched;
	task->ops = ops;
	task->args = args;
	task->state = LW_TASK_PAUSED;
	task->order = order;
	task->new_order = order;
	task->delay = 0;

	/*
	 * If the scheduler is not currently processing any tasks, then
	 * insert the new task into the list. Otherwise, add it to the list
	 * of tasks that need re-ordering.
	 */

	if (sched->current == NULL) {
		lw_task_insert(sched, task);
	} else {
		sys_dlist_append(&sched->reorder, &task->reorder);
	}

	k_spin_unlock(&sched->lock, key);

	return task;
}

int lw_task_start(struct lw_task *task)
{
	struct lw_scheduler *sched;
	k_spinlock_key_t  key;
	int  status = -EINVAL;

	sched = task->sched;

	key = k_spin_lock(&sched->lock);

	if (task->state != LW_TASK_ABORT) {
		task->state = LW_TASK_EXECUTE;
		status = 0;
	}

	k_spin_unlock(&sched->lock, key);

	return status;
}

void lw_task_abort(struct lw_task *task)
{
	struct lw_scheduler *sched = task->sched;
	k_spinlock_key_t  key;

	key = k_spin_lock(&sched->lock);

	if (task->state == LW_TASK_ABORT) {

		/* Nothing to do. */

		k_spin_unlock(&sched->lock, key);
		return;
	}

	/* Mark the task for aborting; remove it from the lists */

	task->state = LW_TASK_ABORT;

	if (sys_dnode_is_linked(&task->reorder)) {
		sys_dlist_remove(&task->reorder);
	}

	if (sched->current != task) {
		sys_dlist_remove(&task->node);
	}

	k_spin_unlock(&sched->lock, key);
}

void lw_task_delay(struct lw_task *task, uint32_t num_intervals)
{
	k_spinlock_key_t  key;

	key = k_spin_lock(&task->sched->lock);

	if (task->state != LW_TASK_ABORT) {
		task->delay = num_intervals;
	}

	k_spin_unlock(&task->sched->lock, key);
}

void lw_task_pause(struct lw_task *task)
{
	k_spinlock_key_t  key;

	key = k_spin_lock(&task->sched->lock);

	if (task->state != LW_TASK_ABORT) {
		task->state = LW_TASK_PAUSED;
	}

	k_spin_unlock(&task->sched->lock, key);
}

struct lw_task *lw_task_current_get(struct lw_scheduler *sched)
{
	return sched->current;
}

void lw_task_reorder(struct lw_task *task, uint32_t new_order)
{
	k_spinlock_key_t  key;
	struct lw_scheduler *sched = task->sched;

	key = k_spin_lock(&sched->lock);

	if (task->state == LW_TASK_ABORT) {
		k_spin_unlock(&sched->lock, key);
		return;
	}

	if (sched->current == NULL) {

		/* It is safe to immediately insert the task. */

		task->order = new_order;
		lw_task_insert(sched, task);
	} else if (!sys_dnode_is_linked(&task->reorder)) {

		/* Scheduler is in progress. Delay the reordering. */

		task->new_order = new_order;
		sys_dlist_append(&task->sched->reorder, &task->reorder);
	}

	k_spin_unlock(&task->sched->lock, key);
}
