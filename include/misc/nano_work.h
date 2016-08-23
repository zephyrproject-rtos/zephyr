
/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 * @brief Workqueue structures and routines.
 */

#ifndef _misc_nano_work__h_
#define _misc_nano_work__h_

#include <nanokernel.h>
#include <atomic.h>
#include <misc/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nano_work;

typedef void (*work_handler_t)(struct nano_work *);

/**
 * A workqueue is a fiber that executes @ref nano_work items that are
 * queued to it.  This is useful for drivers which need to schedule
 * execution of code which might sleep from ISR context.  The actual
 * fiber identifier is not stored in the structure in order to save
 * space.
 */
struct nano_workqueue {
	struct nano_fifo fifo;
};

/**
 * @brief Work flags.
 */
enum {
	NANO_WORK_STATE_IDLE,		/* Work item idle state */

	NANO_WORK_NUM_FLAGS,		/* Number of flags - must be last */
};

/**
 * @brief An item which can be scheduled on a @ref nano_workqueue.
 */
struct nano_work {
	void *_reserved;		/* Used by nano_fifo implementation. */
	work_handler_t handler;
	ATOMIC_DEFINE(flags, NANO_WORK_NUM_FLAGS);
};

/**
 * @brief Initialize work item
 */
static inline void nano_work_init(struct nano_work *work,
				  work_handler_t handler)
{
	atomic_set_bit(work->flags, NANO_WORK_STATE_IDLE);
	work->handler = handler;
}

/**
 * @brief Submit a work item to a workqueue.
 */
static inline void nano_work_submit_to_queue(struct nano_workqueue *wq,
					     struct nano_work *work)
{
	if (!atomic_test_and_clear_bit(work->flags, NANO_WORK_STATE_IDLE)) {
		__ASSERT_NO_MSG(0);
	} else {
		nano_fifo_put(&wq->fifo, work);
	}
}

/**
 * @brief Start a new workqueue.  Call this from fiber context.
 */
extern void nano_fiber_workqueue_start(struct nano_workqueue *wq,
				       const struct fiber_config *config);

/**
 * @brief Start a new workqueue.  Call this from task context.
 */
extern void nano_task_workqueue_start(struct nano_workqueue *wq,
				      const struct fiber_config *config);

/**
 * @brief Start a new workqueue.  This routine can be called from either
 * fiber or task context.
 */
extern void nano_workqueue_start(struct nano_workqueue *wq,
				 const struct fiber_config *config);

#if defined(CONFIG_NANO_TIMEOUTS)

 /*
 * @brief An item which can be scheduled on a @ref nano_workqueue with a
 * delay.
 */
struct nano_delayed_work {
	struct nano_work work;
	struct _nano_timeout timeout;
	struct nano_workqueue *wq;
};

/**
 * @brief Initialize delayed work
 */
void nano_delayed_work_init(struct nano_delayed_work *work,
			    work_handler_t handler);

/**
 * @brief Submit a delayed work item to a workqueue.
 *
 * This procedure schedules a work item to be processed after a delay.
 * Once the delay has passed, the work item is submitted to the work queue:
 * at this point, it is no longer possible to cancel it. Once the work item's
 * handler is about to be executed, the work is considered complete and can be
 * resubmitted.
 *
 * Care must be taken if the handler blocks or yield as there is no implicit
 * mutual exclusion mechanism. Such usage is not recommended and if necessary,
 * it should be explicitly done between the submitter and the handler.
 *
 * @param wq Workqueue to schedule the work item
 * @param work Delayed work item
 * @param ticks Ticks to wait before scheduling the work item
 *
 * @return 0 in case of success or negative value in case of error.
 */
int nano_delayed_work_submit_to_queue(struct nano_workqueue *wq,
				      struct nano_delayed_work *work,
				      int ticks);

/**
 * @brief Cancel a delayed work item
 *
 * This procedure cancels a scheduled work item. If the work has been completed
 * or is idle, this will do nothing. The only case where this can fail is when
 * the work has been submitted to the work queue, but the handler has not run
 * yet.
 *
 * @param work Delayed work item to be canceled
 *
 * @return 0 in case of success or negative value in case of error.
 */
int nano_delayed_work_cancel(struct nano_delayed_work *work);

#endif /* CONFIG_NANO_TIMEOUTS */

#if defined(CONFIG_SYSTEM_WORKQUEUE)

extern struct nano_workqueue sys_workqueue;

/*
 * @brief Submit a work item to the system workqueue.
 *
 * @ref nano_work_submit_to_queue
 *
 * When using the system workqueue it is not recommended to block or yield
 * on the handler since its fiber is shared system wide it may cause
 * unexpected behavior.
 */
static inline void nano_work_submit(struct nano_work *work)
{
	nano_work_submit_to_queue(&sys_workqueue, work);
}

#if defined(CONFIG_NANO_TIMEOUTS)
/*
 * @brief Submit a delayed work item to the system workqueue.
 *
 * @ref nano_delayed_work_submit_to_queue
 *
 * When using the system workqueue it is not recommended to block or yield
 * on the handler since its fiber is shared system wide it may cause
 * unexpected behavior.
 */
static inline int nano_delayed_work_submit(struct nano_delayed_work *work,
					   int ticks)
{
	return nano_delayed_work_submit_to_queue(&sys_workqueue, work, ticks);
}

#endif /* CONFIG_NANO_TIMEOUTS */
#endif /* CONFIG_SYSTEM_WORKQUEUE */

#ifdef __cplusplus
}
#endif

#endif /* _misc_nano_work__h_ */
