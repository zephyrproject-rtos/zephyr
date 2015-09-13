/* microkernel/task.h */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TASK_H
#define TASK_H
/**
 * @brief Microkernel Tasks
 * @defgroup microkernel_task Microkernel Tasks
 * @ingroup microkernel_services
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <sections.h>

/*
 * The following task groups are reserved for system use.
 * sysgen automatically generates corresponding TASKGROUPs with reserved
 * GROUPIDs
 * sysgen must be updated if any changes are made to the reserved groups.
 */

#define EXE_GROUP 1 /* TASKGROUP EXE */
#define USR_GROUP 2 /* TASKGROUP SYS */
#define FPU_GROUP 4 /* TASKGROUP FPU */

/**
 * @cond internal
 */

/**
 * @brief Initialize a struct k_task given parameters.
 *
 * @param ident Numeric identifier of this task object.
 * @param priority Priority of task.
 * @param state State of task.
 * @param groups Groups this task belong to.
 * @param fn_start Entry function.
 * @param workspace Pointer to workspace (aka, stack).
 * @param worksize Size of workspace.
 * @param fn_abort Abort function.
 */
#define __K_TASK_INITIALIZER(ident, priority, state, groups, \
			     fn_start, workspace, worksize, fn_abort) \
	{ \
	  NULL, NULL, priority, ident, state, ((groups) ^ SYS), \
	  fn_start, workspace, worksize, fn_abort, NULL, \
	}
extern struct k_task _k_task_list[];

extern void _task_ioctl(ktask_t, int);
extern void _task_group_ioctl(ktask_group_t, int);

/**
 * @endcond
 */

/**
 * @brief Yield the CPU to another task
 *
 * This routine yields the processor to the next equal priority task that is
 * runnable. Using task_yield(), it is possible to achieve the effect of round
 * robin scheduling. If no task with the same priority is runnable then no task
 * switch occurs and the calling task resumes execution.
 *
 * @return N/A
 */
extern void task_yield(void);

/**
 * @brief Set the priority of a task
 *
 * This routine changes the priority of the specified task.
 *
 * The call has immediate effect. If the calling task is no longer the highest
 * priority runnable task, a task switch occurs.
 *
 * The priority should be specified in the range 0 to 62. 0 is the highest
 * priority.
 * @param task Task whose priority is to be set
 * @param prio New priority
 * @return N/A
 */
extern void task_priority_set(ktask_t task, kpriority_t prio);

/**
 * @brief Set the entry point of a task
 *
 * This routine sets the entry point of a task to a given routine. It is only
 * needed if the entry point is different from that specified in the project
 * file. It must be called before task_start() to have any effect, so it
 * cannot work with members of the EXE group or of any group that automatically
 * starts when the application is loaded.
 *
 * The routine is executed when the task is started
 *
 * @param task Task to operate on.
 * @pram func Entry point
 * @return N/A
 */
extern void task_entry_set(ktask_t task, void (*func)(void));

/**
 * @brief Install an abort handler
 *
 * This routine installs an abort handler for the calling task.
 *
 * The abort handler is run when the calling task is aborted by a _TaskAbort()
 * or task_group_abort() call.
 *
 * Each call to task_abort_handler_set() replaces the previously installed
 * handler.
 *
 * To remove an abort handler, set the parameter to NULL as below:
 *      task_abort_handler_set (NULL)
 * @param func Abort handler
 * @return N/A
 */
extern void task_abort_handler_set(void (*func)(void));

/**
 * @brief Issue a custom call from within the microkernel server fiber
 *
 * This routine issues a request to execute a function from within the context
 * of the microkernel server fiber.
 *
 * @param func function to call from within the microkernel server fiber
 * @param argp argument to pass to custom function
 *
 * @return return value from custom <func> call
 */
extern int task_offload_to_fiber(int (*)(), void *);

/*
 * Operations supported by _task_ioctl() and _task_group_ioctl()
 */

#define TASK_START 0
#define TASK_ABORT 1
#define TASK_SUSPEND 2
#define TASK_RESUME 3
#define TASK_BLOCK 4
#define TASK_UNBLOCK 5

#define TASK_GROUP_START 0
#define TASK_GROUP_ABORT 1
#define TASK_GROUP_SUSPEND 2
#define TASK_GROUP_RESUME 3
#define TASK_GROUP_BLOCK 4
#define TASK_GROUP_UNBLOCK 5

/**
 * @brief Get task identifier
 *
 * @return identifier for current task
 */
extern ktask_t task_id_get();

/**
 * @brief Get task priority
 *
 * @return priority of current task
 */
extern kpriority_t task_priority_get();

/**
 * @brief Start a task
 * @param t Task to start
 * @return N/A
 */
#define task_start(t) _task_ioctl(t, TASK_START)

/**
 * @brief Abort a task
 * @param t Task to abort
 * @return N/A
 */
#define task_abort(t) _task_ioctl(t, TASK_ABORT)

/**
 * @brief Suspend a task
 * @param t Task to suspend
 * @return N/A
 */
#define task_suspend(t) _task_ioctl(t, TASK_SUSPEND)

/**
 * @brief Resume a task
 * @param t Task to resume
 * @return N/A
 */
#define task_resume(t) _task_ioctl(t, TASK_RESUME)

/**
 * @brief Get task groups for task
 *
 * @return task groups associated with current task
 */
extern uint32_t task_group_mask_get();

/**
 * @brief Add task to task group(s)
 *
 * @param groups Task Groups
 * @return N/A
 */
extern void task_group_join(uint32_t groups);

/**
 * @brief Remove task from task group(s)
 * @param groups Task Groups
 * @return N/A
 */
extern void task_group_leave(uint32_t groups);

/**
 * @brief Start a task group
 * @param g Task group to start
 * @return N/A
 */
#define task_group_start(g) _task_group_ioctl(g, TASK_GROUP_START)

/**
 * @brief Abort a task group
 * @param g Task group to abort
 * @return N/A
 */
#define task_group_abort(g) _task_group_ioctl(g, TASK_GROUP_ABORT)

/**
 * @brief Suspend a task group
 * @param g Task group to suspend
 * @return N/A
 */
#define task_group_suspend(g) _task_group_ioctl(g, TASK_GROUP_SUSPEND)

/**
 * @brief Resume a task group
 * @param g Task group to resume
 * @return N/A
 */
#define task_group_resume(g) _task_group_ioctl(g, TASK_GROUP_RESUME)

/**
 * @brief Get task identifier
 *
 * @return identifier for current task
 */
#define isr_task_id_get() task_id_get()

/**
 * @brief Get task priority
 *
 * @return priority of current task
 */
#define isr_task_priority_get() task_priority_get()

/**
 * @brief Get task groups for task
 *
 * @return task groups associated with current task
 */
#define isr_task_group_mask_get() task_group_mask_get()


/**
 * @brief Define a private microkernel task.
 *
 * This declares and initializes a private task. The new task
 * can be passed to the microkernel task functions.
 *
 * @param name Name of the task.
 * @param priority Priority of task.
 * @param entry Entry function.
 * @param stack_size size of stack (in bytes)
 * @param groups Groups this task belong to.
 */
#define DEFINE_TASK(name, priority, entry, stack_size, groups) \
	extern void entry(void); \
	char __noinit __stack __stack_##name[stack_size]; \
	struct k_task _k_task_obj_##name \
		__section(_k_task_list, private, task) = \
		__K_TASK_INITIALIZER( \
			(ktask_t)&_k_task_obj_##name, \
			priority, 0x00000001, groups, \
			entry, &__stack_##name[0], stack_size, NULL); \
	const ktask_t name = (ktask_t)&_k_task_obj_##name;

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* TASK_H */
