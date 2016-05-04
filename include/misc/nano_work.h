
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
 * @brief An item which can be scheduled on a @ref nano_workqueue.
 */
struct nano_work {
	void *_reserved;		/* Used by nano_fifo implementation. */
	work_handler_t handler;
};

/**
 * @brief Submit a work item to a workqueue.
 */
static inline void nano_work_submit_to_queue(struct nano_workqueue *wq,
					     struct nano_work *work)
{
	nano_fifo_put(&wq->fifo, work);
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

#ifdef CONFIG_SYSTEM_WORKQUEUE

extern struct nano_workqueue sys_workqueue;

static inline void nano_work_submit(struct nano_work *work)
{
	nano_work_submit_to_queue(&sys_workqueue, work);
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* _misc_nano_work__h_ */
