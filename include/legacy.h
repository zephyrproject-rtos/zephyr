/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
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
 * @brief Public legacy kernel APIs.
 */

#ifndef _legacy__h_
#define _legacy__h_

#include <stdint.h>
#include <errno.h>
#include <limits.h>

/* nanokernel/microkernel execution context types */
#define NANO_CTX_ISR (K_ISR)
#define NANO_CTX_FIBER (K_COOP_THREAD)
#define NANO_CTX_TASK (K_PREEMPT_THREAD)

/* timeout special values */
#define TICKS_UNLIMITED (K_FOREVER)
#define TICKS_NONE (K_NO_WAIT)

/* microkernel object return codes */
#define RC_OK 0
#define RC_FAIL 1
#define RC_TIME 2
#define RC_ALIGNMENT 3
#define RC_INCOMPLETE 4

#define ANYTASK K_ANY

/* end-of-list, mostly used for semaphore groups */
#define ENDLIST K_END

/* pre-defined task groups */
#define K_TASK_GROUP_EXE 0x1
#define K_TASK_GROUP_SYS 0x2
#define K_TASK_GROUP_FPU 0x4
/* the following is for x86 architecture only */
#define K_TASK_GROUP_SSE 0x8

/* pipe amount of content to receive (0+, 1+, all) */
typedef enum {
	_0_TO_N = 0x0,
	_1_TO_N = 0x1,
	_ALL_N  = 0x2,
} K_PIPE_OPTION;

#define kpriority_t uint32_t

static inline int32_t _ticks_to_ms(int32_t ticks)
{
	return (ticks == TICKS_UNLIMITED) ? K_FOREVER : __ticks_to_ms(ticks);
}

static inline int _error_to_rc(int err)
{
	return err == 0 ? RC_OK : err == -EAGAIN ? RC_TIME : RC_FAIL;
}

static inline int _error_to_rc_no_timeout(int err)
{
	return err == 0 ? RC_OK : RC_FAIL;
}

/* tasks/fibers/scheduler */

#define ktask_t k_tid_t
#define nano_thread_id_t k_tid_t
typedef void (*nano_fiber_entry_t)(int i1, int i2);
typedef int nano_context_type_t;

#define _MDEF_THREAD_DEFINE(name, stack_size,                              \
			    entry, p1, p2, p3,                             \
			    abort, prio, groups)                           \
	char __noinit __stack _k_thread_obj_##name[stack_size];            \
	struct _static_thread_data _k_thread_data_##name __aligned(4)      \
		__in_section(_static_thread_data, static, name) =          \
		_THREAD_INITIALIZER(_k_thread_obj_##name, stack_size,      \
				    entry, p1, p2, p3, prio, 0, K_FOREVER, \
				    abort, groups)

/**
 * @brief Define a private microkernel task.
 *
 * <b> Legacy API </b>
 *
 * This declares and initializes a private task. The new task
 * can be passed to the microkernel task functions.
 *
 * @param name Name of the task.
 * @param priority Priority of task.
 * @param entry Entry function.
 * @param stack_size Size of stack (in bytes)
 * @param groups Groups this task belong to.
 */
#define DEFINE_TASK(name, priority, entry, stack_size, groups)        \
	extern void entry(void);                                      \
	char __noinit __stack _k_thread_obj_##name[stack_size];       \
	struct _static_thread_data _k_thread_data_##name __aligned(4) \
		__in_section(_static_thread_data, static, name) =     \
		_THREAD_INITIALIZER(_k_thread_obj_##name, stack_size, \
				    entry, NULL, NULL, NULL,          \
				    priority, 0, K_FOREVER,           \
				    NULL, (uint32_t)(groups));        \
	k_tid_t const name = (k_tid_t)_k_thread_obj_##name

/**
 * @brief Return the ID of the currently executing thread.
 *
 * <b> Legacy API </b>
 *
 * This routine returns a pointer to the thread control block of the currently
 * executing thread. It is cast to a nano_thread_id_t for public use.
 *
 * @return The ID of the currently executing thread.
 */
#define sys_thread_self_get k_current_get

/**
 * @brief Cause the currently executing thread to busy wait.
 *
 * <b> Legacy API </b>
 *
 * This routine causes the current task or fiber to execute a "do nothing"
 * loop for a specified period of time.
 *
 * @warning This routine utilizes the system clock, so it must not be invoked
 * until the system clock is fully operational or while interrupts
 * are locked.
 *
 * @param usec_to_wait Number of microseconds to busy wait.
 *
 * @return N/A
 */
#define sys_thread_busy_wait k_busy_wait

/**
 * @brief Return the type of the current execution context.
 *
 * <b> Legacy API </b>
 *
 * This routine returns the type of execution context currently executing.
 *
 * @return The type of the current execution context.
 * @retval NANO_CTX_ISR (0): executing an interrupt service routine.
 * @retval NANO_CTX_FIBER (1): current thread is a fiber.
 * @retval NANO_CTX_TASK (2): current thread is a task.
 */
extern int sys_execution_context_type_get(void);

/**
 * @brief Initialize and start a fiber.
 *
 * <b> Legacy API </b>
 *
 * This routine initializes and starts a fiber. It can be called from
 * either a fiber or a task. When this routine is called from a
 * task, the newly created fiber will start executing immediately.
 *
 * @internal
 * Given that this routine is _not_ ISR-callable, the following code is used
 * to differentiate between a task and fiber:
 *
 * if ((_nanokernel.current->flags & TASK) == TASK)
 *
 * Given that the _fiber_start() primitive is not considered real-time
 * performance critical, a runtime check to differentiate between a calling
 * task or fiber is performed to conserve footprint.
 * @endinternal
 *
 * @param stack Pointer to the stack space.
 * @param stack_size Stack size in bytes.
 * @param entry Fiber entry.
 * @param arg1 1st entry point parameter.
 * @param arg2 2nd entry point parameter.
 * @param prio The fiber's priority.
 * @param options Not used currently.
 *
 * @return nanokernel thread identifier
 */
static inline nano_thread_id_t fiber_start(char *stack, unsigned stack_size,
						nano_fiber_entry_t entry,
						int arg1, int arg2,
						unsigned prio,
						unsigned options)
{
	return k_thread_spawn(stack, stack_size, (k_thread_entry_t)entry,
				(void *)(intptr_t)arg1, (void *)(intptr_t)arg2,
				NULL, K_PRIO_COOP(prio), options, 0);
}

/**
 * @brief Initialize and start a fiber from a fiber.
 *
 * <b> Legacy API </b>
 *
 * Like fiber_start(), but may only be called from a fiber.
 *
 * @sa fiber_start
 */
#define fiber_fiber_start fiber_start

/**
 * @brief Initialize and start a fiber from a task.
 *
 * <b> Legacy API </b>
 *
 * Like fiber_start(), but may only be called from a task.
 *
 * @sa fiber_start
 */
#define task_fiber_start fiber_start

/**
 * @brief Fiber configuration structure.
 *
 * <b> Legacy API </b>
 *
 * Parameters such as stack size and fiber priority are often
 * user configurable. This structure makes it simple to specify such a
 * configuration.
 */
struct fiber_config {
	char *stack;
	unsigned stack_size;
	unsigned prio;
};

/**
 * @brief Start a fiber based on a @ref fiber_config.
 *
 * <b> Legacy API </b>
 *
 * This routine can be called from either a fiber or a task.
 *
 * @param config Pointer to fiber configuration structure
 * @param entry Fiber entry.
 * @param arg1 1st entry point parameter.
 * @param arg2 2nd entry point parameter.
 * @param options Not used currently.
 *
 * @return thread ID
 */
#define fiber_start_config(config, entry, arg1, arg2, options) \
	fiber_start(config->stack, config->stack_size, \
		    entry, arg1, arg2, \
		    config->prio, options)

/**
 * @brief Start a fiber based on a @ref fiber_config, from fiber context.
 *
 * <b> Legacy API </b>
 *
 * Like fiber_start_config(), but may only be called from a fiber.
 *
 * @sa fiber_start_config()
 */
#define fiber_fiber_start_config fiber_start_config

/**
 * @brief Start a fiber based on a @ref fiber_config, from task context.
 *
 * <b> Legacy API </b>
 *
 * Like fiber_start_config(), but may only be called from a task.
 *
 * @sa fiber_start_config()
 */
#define task_fiber_start_config fiber_start_config

/**
 * @brief Start a fiber while delaying its execution.
 *
 * <b> Legacy API </b>
 *
 * @param stack Pointer to the stack space.
 * @param stack_size_in_bytes Stack size in bytes.
 * @param entry_point The fiber's entry point.
 * @param param1 1st entry point parameter.
 * @param param2 2nd entry point parameter.
 * @param priority The fiber's priority.
 * @param options Not used currently.
 * @param timeout_in_ticks Timeout duration in ticks.
 *
 * @return A handle potentially used to cancel the delayed start.
 */
static inline nano_thread_id_t
fiber_delayed_start(char *stack, unsigned int stack_size_in_bytes,
			nano_fiber_entry_t entry_point, int param1,
			int param2, unsigned int priority,
			unsigned int options, int32_t timeout_in_ticks)
{
	return k_thread_spawn(stack, stack_size_in_bytes,
			      (k_thread_entry_t)entry_point,
			      (void *)(intptr_t)param1,
			      (void *)(intptr_t)param2, NULL,
			      K_PRIO_COOP(priority), options,
			      _ticks_to_ms(timeout_in_ticks));
}

/**
 * @brief Start a fiber while delaying its execution.
 *
 * <b> Legacy API </b>
 *
 * Like fiber_delayed_start(), but may only be called from a fiber.
 *
 * @sa fiber_delayed_start
 */
#define fiber_fiber_delayed_start fiber_delayed_start

/**
 * @brief Start a fiber while delaying its execution.
 *
 * <b> Legacy API </b>
 *
 * Like fiber_delayed_start(), but may only be called from a task.
 *
 * @sa fiber_delayed_start
 */
#define task_fiber_delayed_start fiber_delayed_start

/**
 * @brief Cancel a delayed fiber start.
 *
 * <b> Legacy API </b>
 *
 * @param handle The handle returned when starting the delayed fiber.
 *
 * @return N/A
 */
#define fiber_delayed_start_cancel k_thread_cancel

/**
 * @brief Cancel a delayed fiber start from a fiber
 *
 * <b> Legacy API </b>
 *
 * Like fiber_delayed_start_cancel(), but may only be called from a fiber.
 *
 * @sa fiber_delayed_start_cancel
 */
#define fiber_fiber_delayed_start_cancel fiber_delayed_start_cancel

/**
 * @brief Cancel a delayed fiber start from a task
 *
 * <b> Legacy API </b>
 *
 * Like fiber_delayed_start_cancel(), but may only be called from a fiber.
 *
 * @sa fiber_delayed_start_cancel
 */
#define task_fiber_delayed_start_cancel fiber_delayed_start_cancel

/**
 * @brief Yield the current fiber.
 *
 * <b> Legacy API </b>
 *
 * Calling this routine results in the current fiber yielding to
 * another fiber of the same or higher priority. If there are no
 * other runnable fibers of the same or higher priority, the
 * routine will return immediately.
 *
 * This routine can only be called from a fiber.
 *
 * @return N/A
 */
#define fiber_yield k_yield

/**
 * @brief Abort the currently executing fiber.
 *
 * <b> Legacy API </b>
 *
 * This routine aborts the currently executing fiber. An abort can occur
 * because of one of three reasons:
 * - The fiber has explicitly aborted itself by calling this routine.
 * - The fiber has implicitly aborted itself by returning from its entry point.
 * - The fiber has encountered a fatal exception.
 *
 * This routine can only be called from a fiber.
 *
 * @return N/A
 */
#define fiber_abort() k_thread_abort(k_current_get())

extern void _legacy_sleep(int32_t ticks);

/**
 * @brief Put the current fiber to sleep.
 *
 * <b> Legacy API </b>
 *
 * This routine puts the currently running fiber to sleep
 * for the number of system ticks passed in the
 * @a timeout_in_ticks parameter.
 *
 * @param timeout_in_ticks Number of system ticks the fiber sleeps.
 *
 * @return N/A
 */
#define fiber_sleep _legacy_sleep

/**
 * @brief Put the task to sleep.
 *
 * <b> Legacy API </b>
 *
 * This routine puts the currently running task to sleep for the number
 * of system ticks passed in the @a timeout_in_ticks parameter.
 *
 * @param timeout_in_ticks Number of system ticks the task sleeps.
 *
 * @warning A value of TICKS_UNLIMITED is considered invalid and may result in
 * unexpected behavior.
 *
 * @return N/A
 *
 * @sa TICKS_UNLIMITED
 */
#define task_sleep _legacy_sleep

/**
 * @brief Wake the specified fiber from sleep
 *
 * <b> Legacy API </b>
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid
 * unnecessary overhead.
 *
 * @param fiber Identifies fiber to wake
 *
 * @return N/A
 */
#define fiber_wakeup k_wakeup

/**
 * @brief Wake the specified fiber from sleep
 *
 * <b> Legacy API </b>
 *
 * Like fiber_wakeup(), but may only be called from an ISR.
 *
 * @sa fiber_wakeup
 */
#define isr_fiber_wakeup k_wakeup

/**
 * @brief Wake the specified fiber from sleep
 *
 * <b> Legacy API </b>
 *
 * Like fiber_wakeup, but may only be called from a fiber.
 *
 * @sa fiber_wakeup
 */
#define fiber_fiber_wakeup k_wakeup

/**
 * @brief Wake the specified fiber from sleep
 *
 * <b> Legacy API </b>
 *
 * Like fiber_wakeup, but may only be called from a task.
 *
 * @sa fiber_wakeup
 */
#define task_fiber_wakeup k_wakeup

/**
 * @brief Yield the CPU to another task.
 *
 * <b> Legacy API </b>
 *
 * This routine yields the processor to the next-equal priority runnable
 * task. With task_yield(), the effect of round-robin scheduling is
 * possible. When no task of equal priority is runnable, no task switch
 * occurs, and the calling task resumes execution.
 *
 * @return N/A
 */
#define task_yield k_yield

/**
 * @brief Set the priority of a task.
 *
 * <b> Legacy API </b>
 *
 * This routine changes the priority of the specified task.
 *
 * The call has immediate effect. When the calling task no longer is the
 * highest-priority runnable task, a task switch occurs.
 *
 * Priority can be assigned in the range 0 to 62, where 0 is the
 * highest priority.
 *
 * @param task Task whose priority is to be set.
 * @param prio New priority.
 *
 * @return N/A
 */
#define task_priority_set(task, prio) k_thread_priority_set(task, (int)prio)

/**
 * @brief Set the entry point of a task.
 *
 * <b> Legacy API </b>
 *
 * This routine sets the entry point of a task to a given routine. It is
 * needed only when an entry point differs from what is set in the project
 * file. To have any effect, it must be called before task_start(), and it
 * cannot work with members of the EXE group or with any group that starts
 * automatically on application loading.
 *
 * The routine is executed when the task is started.
 *
 * @param task Task to operate on.
 * @param entry Entry point.
 *
 * @return N/A
 */
#define task_entry_set(task, entry) \
	k_thread_entry_set(task, (k_thread_entry_t)entry)

/**
 * @brief Install an abort handler.
 *
 * <b> Legacy API </b>
 *
 * This routine installs an abort handler for the calling task.
 *
 * The abort handler runs when the calling task is aborted by a _TaskAbort()
 * or task_group_abort() call.
 *
 * Each call to task_abort_handler_set() replaces the previously-installed
 * handler.
 *
 * To remove an abort handler, set the parameter to NULL as below:
 *      task_abort_handler_set (NULL)
 *
 * @param handler Abort handler.
 *
 * @return N/A
 */
extern void task_abort_handler_set(void (*handler)(void));

/**
 * @brief Process an "offload" request
 *
 * <b> Legacy API </b>
 *
 * The routine places the @a func into the work queue. This allows
 * the task to execute a routine uninterrupted by other tasks.
 *
 * Note: this routine can be invoked only from a task.
 * For the routine to work, the scheduler must be unlocked.
 *
 * @param func function to call
 * @param argp function arguments
 *
 * @return result of @a func call
 */
extern int task_offload_to_fiber(int (*func)(), void *argp);

/**
 * @brief Gets task identifier
 *
 * <b> Legacy API </b>
 *
 * @return identifier for current task
 */
#define task_id_get k_current_get

/**
 * @brief Gets task priority
 *
 * <b> Legacy API </b>
 *
 * @return priority of current task
 */
#define task_priority_get() \
	(kpriority_t)(k_thread_priority_get(k_current_get()))

/**
 * @brief Abort a task
 *
 * <b> Legacy API </b>
 *
 * @param task Task to abort
 *
 * @return N/A
 */
#define task_abort k_thread_abort

/**
 * @brief Suspend a task
 *
 * <b> Legacy API </b>
 *
 * @param task Task to suspend
 *
 * @return N/A
 */
#define task_suspend k_thread_suspend

/**
 * @brief Resume a task
 *
 * <b> Legacy API </b>
 *
 * @param task Task to resume
 *
 * @return N/A
 */
#define task_resume k_thread_resume

/**
 * @brief Start a task
 *
 * <b> Legacy API </b>
 *
 * @param task Task to start
 *
 * @return N/A
 */
extern void task_start(ktask_t task);

/**
 * @brief Set time-slicing period and scope
 *
 * <b> Legacy API </b>
 *
 * This routine controls how task time slicing is performed by the task
 * scheduler; it specifes the maximum time slice length (in ticks) and
 * the highest priority task level for which time slicing is performed.
 *
 * To enable time slicing, a non-zero time slice length must be specified.
 * The task scheduler then ensures that no executing task runs for more than
 * the specified number of ticks before giving other tasks of that priority
 * a chance to execute. (However, any task whose priority is higher than the
 * specified task priority level is exempted, and may execute as long as
 * desired without being pre-empted due to time slicing.)
 *
 * Time slicing limits only the maximum amount of time a task may continuously
 * execute. Once the scheduler selects a task for execution, there is no minimum
 * guaranteed time the task will execute before tasks of greater or equal
 * priority are scheduled.
 *
 * When the currently-executing task is the only one of that priority eligible
 * for execution, this routine has no effect; the task is immediately
 * rescheduled after the slice period expires.
 *
 * To disable timeslicing, call the API with both parameters set to zero.
 *
 * @param ticks Maximum time slice length in ticks
 * @param priority Highest priority task level for which time slicing is
 *        performed
 *
 * @return N/A
 */
static inline void sys_scheduler_time_slice_set(int32_t ticks,
						kpriority_t priority)
{
	k_sched_time_slice_set(_ticks_to_ms(ticks), (int)priority);
}

extern void _k_thread_group_op(uint32_t groups, void (*func)(struct tcs *));

/**
 * @brief Get task groups for task
 *
 * <b> Legacy API </b>
 *
 * @return task groups associated with current task
 */
static inline uint32_t task_group_mask_get(void)
{
	extern uint32_t _k_thread_group_mask_get(struct tcs *thread);

	return _k_thread_group_mask_get(k_current_get());
}

/**
 * @brief Get task groups for task
 *
 * <b> Legacy API </b>
 *
 * @return task groups associated with current task
 */
#define isr_task_group_mask_get  task_group_mask_get

/**
 * @brief Add task to task group(s)
 *
 * <b> Legacy API </b>
 *
 * @param groups Task Groups
 *
 * @return N/A
 */
static inline void task_group_join(uint32_t groups)
{
	extern void _k_thread_group_join(uint32_t groups, struct tcs *thread);

	_k_thread_group_join(groups, k_current_get());
}

/**
 * @brief Remove task from task group(s)
 *
 * <b> Legacy API </b>
 *
 * @param groups Task Groups
 *
 * @return N/A
 */
static inline void task_group_leave(uint32_t groups)
{
	extern void _k_thread_group_leave(uint32_t groups, struct tcs *thread);

	_k_thread_group_leave(groups, k_current_get());
}

/**
 * @brief Start one or more task groups
 *
 * <b> Legacy API </b>
 *
 * @param groups Task groups to start
 *
 * @return N/A
 */
static inline void task_group_start(uint32_t groups)
{
	extern void _k_thread_single_start(struct tcs *thread);

	return _k_thread_group_op(groups, _k_thread_single_start);
}

/**
 * @brief Suspend one or more task groups
 *
 * <b> Legacy API </b>
 *
 * @param groups Task groups to suspend
 *
 * @return N/A
 */
static inline void task_group_suspend(uint32_t groups)
{
	extern void _k_thread_single_suspend(struct tcs *thread);

	return _k_thread_group_op(groups, _k_thread_single_suspend);
}

/**
 * @brief Resume one or more task groups
 *
 * <b> Legacy API </b>
 *
 * @param groups Task groups to resume
 *
 * @return N/A
 */
static inline void task_group_resume(uint32_t groups)
{
	extern void _k_thread_single_resume(struct tcs *thread);

	return _k_thread_group_op(groups, _k_thread_single_resume);
}

/**
 * @brief Abort one or more task groups
 *
 * <b> Legacy API </b>
 *
 * @param groups Task groups to abort
 *
 * @return N/A
 */
static inline void task_group_abort(uint32_t groups)
{
	extern void _k_thread_single_abort(struct tcs *thread);

	return _k_thread_group_op(groups, _k_thread_single_abort);
}

/**
 * @brief Get task identifier
 *
 * <b> Legacy API </b>
 *
 * @return identifier for current task
 */
#define isr_task_id_get task_id_get

/**
 * @brief Get task priority
 *
 * <b> Legacy API </b>
 *
 * @return priority of current task
 */
#define isr_task_priority_get task_priority_get

/* mutexes */

#define kmutex_t struct k_mutex *

/**
 * @brief Lock mutex.
 *
 * <b> Legacy API </b>
 *
 * This routine locks mutex @a mutex. When the mutex is locked by another task,
 * the routine will either wait until it becomes available, or until a specified
 * time limit is reached.
 *
 * A task is permitted to lock a mutex it has already locked; in such a case,
 * this routine immediately succeeds.
 *
 * @param mutex Mutex name.
 * @param timeout Determine the action to take when the mutex is already locked.
 *   For TICKS_NONE, return immediately.
 *   For TICKS_UNLIMITED, wait as long as necessary.
 *   Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @retval RC_OK Successfully locked mutex.
 * @retval RC_TIME Timed out while waiting for mutex.
 * @retval RC_FAIL Failed to immediately lock mutex when
 * @a timeout = TICKS_NONE.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline int task_mutex_lock(kmutex_t mutex, int32_t timeout)
{
	return _error_to_rc(k_mutex_lock(mutex, _ticks_to_ms(timeout)));
}

/**
 * @brief Unlock mutex.
 *
 * <b> Legacy API </b>
 *
 * This routine unlocks mutex @a mutex. The mutex must already be locked by the
 * requesting task.
 *
 * The mutex cannot be claimed by another task until it has been unlocked by
 * the requesting task as many times as it was locked by that task.
 *
 * @param mutex Mutex name.
 *
 * @return N/A
 */
#define task_mutex_unlock k_mutex_unlock

/**
 * @brief Define a private mutex.
 *
 * <b> Legacy API </b>
 *
 * @param name Mutex name.
 */
#define DEFINE_MUTEX(name) \
	K_MUTEX_DEFINE(_k_mutex_obj_##name); \
	struct k_mutex * const name = &_k_mutex_obj_##name

/* semaphores */

#define nano_sem k_sem
#define ksem_t struct k_sem *

/**
 * @brief Initialize a nanokernel semaphore object.
 *
 * <b> Legacy API </b>
 *
 * This function initializes a nanokernel semaphore object structure. After
 * initialization, the semaphore count is 0.
 *
 * It can be called from either a fiber or task.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
static inline void nano_sem_init(struct nano_sem *sem)
{
	k_sem_init(sem, 0, UINT_MAX);
}

/**
 * @brief Give a nanokernel semaphore.
 *
 * <b> Legacy API </b>
 *
 * This routine performs a "give" operation on a nanokernel sempahore object.
 *
 * It is also a convenience wrapper for the execution of context-specific
 * APIs and helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */

#define nano_sem_give k_sem_give

/**
 * @brief Give a nanokernel semaphore (no context switch).
 *
 * <b> Legacy API </b>
 *
 * Like nano_sem_give(), but may only be called from an ISR. A fiber
 * pending on the semaphore object will be made ready, but will NOT be
 * scheduled to execute.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @sa nano_sem_give
 */

#define nano_isr_sem_give k_sem_give

/**
 * @brief Give a nanokernel semaphore (no context switch).
 *
 * <b> Legacy API </b>
 *
 * Like nano_sem_give(), but may only be called from a fiber.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @sa nano_sem_give
 */

#define nano_fiber_sem_give k_sem_give

/**
 * @brief Give a nanokernel semaphore.
 *
 * <b> Legacy API </b>
 *
 * Like nano_sem_give(), but may only be called from a task. A fiber pending
 * on the semaphore object will be made ready, and will preempt the running
 * task immediately.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @sa nano_sem_give
 */

#define nano_task_sem_give k_sem_give

/**
 * @brief Take a nanokernel semaphore, poll/pend if not available.
 *
 * <b> Legacy API </b>
 *
 * This routine performs a "give" operation on a nanokernel sempahore object.
 *
 * It is also a convenience wrapper for the execution of context-specific
 * APIs and is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param sem Pointer to a nano_sem structure.
 * @param timeout_in_ticks Determines the action to take when the semaphore is
 *        unavailable.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *        Otherwise, wait up to the specified number of ticks before timing
 *        out.
 *
 * @warning If it is to be called from the context of an ISR, then @a
 * timeout_in_ticks must be set to TICKS_NONE.
 *
 * @retval 1 When semaphore is available
 * @retval 0 Otherwise
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline int nano_sem_take(struct nano_sem *sem, int32_t timeout_in_ticks)
{
	return k_sem_take((struct k_sem *)sem, _ticks_to_ms(timeout_in_ticks))
		== 0 ? 1 : 0;
}

/**
 * @brief Take a nanokernel semaphore, fail if unavailable.
 *
 * <b> Legacy API </b>
 *
 * Like nano_sem_take(), but must only be called from an ISR with a timeout
 * of TICKS_NONE.
 *
 * @sa nano_sem_take
 */
#define nano_isr_sem_take  nano_sem_take

/**
 * @brief Take a nanokernel semaphore, wait or fail if unavailable.
 *
 * <b> Legacy API </b>
 *
 * Like nano_sem_take(), but may only be called from a fiber.
 *
 * @sa nano_sem_take
 */
#define nano_fiber_sem_take  nano_sem_take

/**
 * @brief Take a nanokernel semaphore, fail if unavailable.
 *
 * <b> Legacy API </b>
 *
 * Like nano_sem_take(), but may only be called from a task.
 *
 * @sa nano_sem_take
 */
#define nano_task_sem_take  nano_sem_take

/**
 * @brief Give semaphore from an ISR.
 *
 * <b> Legacy API </b>
 *
 * This routine gives semaphore @a sem from an ISR, rather than a task.
 *
 * @param sem Semaphore name.
 *
 * @return N/A
 */
#define isr_sem_give k_sem_give

/**
 * @brief Give semaphore from a fiber.
 *
 * <b> Legacy API </b>
 *
 * This routine gives semaphore @a sem from a fiber, rather than a task.
 *
 * @param sem Semaphore name.
 *
 * @return N/A
 */
#define fiber_sem_give k_sem_give

/**
 * @brief Give semaphore.
 *
 * <b> Legacy API </b>
 *
 * This routine gives semaphore @a sem.
 *
 * @param sem Semaphore name.
 *
 * @return N/A
 */
#define task_sem_give k_sem_give

/**
 *
 * @brief Take a semaphore or fail.
 *
 * <b> Legacy API </b>
 *
 * This routine takes the semaphore @a sem. If the semaphore's count is
 * zero the routine immediately returns a failure indication.
 *
 * @param sem Semaphore name.
 * @param timeout Determines the action to take when the semaphore is
 *        unavailable.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *        Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @retval RC_OK Successfully took semaphore
 * @retval RC_TIME Timed out while waiting for semaphore
 * @retval RC_FAIL Failed to immediately take semaphore when
 *         @a timeout = TICKS_NONE
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline int task_sem_take(ksem_t sem, int32_t timeout)
{
	return _error_to_rc(k_sem_take(sem, _ticks_to_ms(timeout)));
}

/**
 * @brief Reset the semaphore's count.
 *
 * <b> Legacy API </b>
 *
 * This routine resets the count of the semaphore @a sem to zero.
 *
 * @param sem Semaphore name.
 *
 * @return N/A
 */
#define task_sem_reset k_sem_reset

/**
 * @brief Read a semaphore's count.
 *
 * <b> Legacy API </b>
 *
 * This routine reads the current count of the semaphore @a sem.
 *
 * @param sem Semaphore name.
 *
 * @return Semaphore count.
 */
#define task_sem_count_get k_sem_count_get

/**
 * @brief Read a nanokernel semaphore's count.
 *
 * <b> Legacy API </b>
 *
 * This routine reads the current count of the semaphore @a sem.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return Semaphore count.
 */
#define nano_sem_count_get k_sem_count_get

#ifdef CONFIG_SEMAPHORE_GROUPS
typedef ksem_t *ksemg_t;

/**
 * @brief Wait for a semaphore from the semaphore group.
 *
 * <b> Legacy API </b>
 *
 * This routine waits for the @a timeout ticks to take a semaphore from the
 * semaphore group @a group.
 *
 * @param group Array of semaphore names - terminated by ENDLIST.
 * @param timeout Determines the action to take when the semaphore is
 *        unavailable.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *        Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @retval Name of the semaphore that was taken if successful.
 * @retval ENDLIST Otherwise.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline ksem_t task_sem_group_take(ksemg_t group, int32_t timeout)
{
	struct k_sem *sem;

	(void)k_sem_group_take(group, &sem, _ticks_to_ms(timeout));

	return sem;
}

/**
 * @brief Give a group of semaphores.
 *
 * <b> Legacy API </b>
 *
 * This routine gives each semaphore in a semaphore group @a semagroup.
 * This method is faster than giving the semaphores individually, and
 * ensures that all the semaphores are given before any waiting tasks run.
 *
 * @param semagroup Array of semaphore names - terminated by ENDLIST.
 *
 * @return N/A
 */
#define task_sem_group_give k_sem_group_give

/**
 * @brief Reset a group of semaphores.
 *
 * <b> Legacy API </b>
 *
 * This routine resets the count for each semaphore in the sempahore group
 * @a semagroup to zero. This method is faster than resetting the semaphores
 * individually.
 *
 * @param semagroup Array of semaphore names - terminated by ENDLIST.
 *
 * @return N/A
 */
#define task_sem_group_reset k_sem_group_reset
#endif

/**
 * @brief Define a private microkernel semaphore
 *
 * <b> Legacy API </b>
 *
 * @param name Semaphore name.
 */
#define DEFINE_SEMAPHORE(name) \
	K_SEM_DEFINE(_k_sem_obj_##name, 0, UINT_MAX); \
	struct k_sem * const name = &_k_sem_obj_##name

/* workqueues */

#define nano_work k_work
#define work_handler_t k_work_handler_t

/**
 * A workqueue is a fiber that executes @ref nano_work items that are
 * queued to it.  This is useful for drivers which need to schedule
 * execution of code which might sleep from ISR context.  The actual
 * fiber identifier is not stored in the structure in order to save
 * space.
 */
#define nano_workqueue k_work_q

/**
 * @brief An item which can be scheduled on a @ref nano_workqueue with a delay
 *
 * <b> Legacy API </b>
 */
#define nano_delayed_work k_delayed_work

/**
 * @brief Initialize work item
 *
 * <b> Legacy API </b>
 *
 * @param work  Work item to initialize
 * @param handler Handler to process work item
 *
 * @return N/A
 */
#define nano_work_init k_work_init

/**
 * @brief Submit a work item to a workqueue.
 *
 * <b> Legacy API </b>
 *
 * This procedure schedules a work item to be processed.
 * In the case where the work item has already been submitted and is pending
 * execution, calling this function will result in a no-op. In this case, the
 * work item must not be modified externally (e.g. by the caller of this
 * function), since that could cause the work item to be processed in a
 * corrupted state.
 *
 * @param wq Work queue
 * @param work Work item
 *
 * @return N/A
 */
#define nano_work_submit_to_queue k_work_submit_to_queue

/**
 * @brief Start a new workqueue.
 *
 * <b> Legacy API </b>
 *
 * This routine can be called from either fiber or task context.
 *
 * @param wq Work queue
 * @param config Fiber configuration structure
 *
 * @return N/A
 */
#define nano_workqueue_start k_work_q_start

/**
 * @brief Start a new workqueue.
 *
 * <b> Legacy API </b>
 *
 * Call this from task context.
 *
 * @sa nano_workqueue_start
 */
#define nano_task_workqueue_start nano_workqueue_start

/**
 * @brief Start a new workqueue.
 *
 * <b> Legacy API </b>
 *
 * Call this from fiber context.
 *
 * @sa nano_workqueue_start
 */
#define nano_fiber_workqueue_start nano_workqueue_start

#if CONFIG_SYS_CLOCK_EXISTS
/**
 * @brief Initialize delayed work
 *
 * <b> Legacy API </b>
 *
 * @param work Work item
 * @param handler Handler to process work item
 *
 * @return N/A
 */
#define nano_delayed_work_init k_delayed_work_init

/**
 * @brief Submit a delayed work item to a workqueue.
 *
 * <b> Legacy API </b>
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
static inline int nano_delayed_work_submit_to_queue(struct nano_workqueue *wq,
				      struct nano_delayed_work *work,
				      int ticks)
{
	return k_delayed_work_submit_to_queue(wq, work, _ticks_to_ms(ticks));
}

/**
 * @brief Cancel a delayed work item
 *
 * <b> Legacy API </b>
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
#define nano_delayed_work_cancel k_delayed_work_cancel
#endif

/**
 * @brief Submit a work item to the system workqueue.
 *
 * <b> Legacy API </b>
 *
 * @ref nano_work_submit_to_queue
 *
 * When using the system workqueue it is not recommended to block or yield
 * on the handler since its fiber is shared system wide it may cause
 * unexpected behavior.
 */
#define nano_work_submit k_work_submit

/**
 * @brief Submit a delayed work item to the system workqueue.
 *
 * <b> Legacy API </b>
 *
 * @ref nano_delayed_work_submit_to_queue
 *
 * When using the system workqueue it is not recommended to block or yield
 * on the handler since its fiber is shared system wide it may cause
 * unexpected behavior.
 */
#define nano_delayed_work_submit(work, ticks) \
	nano_delayed_work_submit_to_queue(&k_sys_work_q, work, ticks)

/* events */

#define kevent_t const struct k_alert *
typedef int (*kevent_handler_t)(int event);

/**
 * @brief Signal an event from an ISR.
 *
 * <b> Legacy API </b>
 *
 * This routine does @em not validate the specified event number.
 *
 * @param event Event to signal.
 *
 * @return N/A
 */
#define isr_event_send task_event_send

/**
 * @brief Signal an event from a fiber.
 *
 * <b> Legacy API </b>
 *
 * This routine does @em not validate the specified event number.
 *
 * @param event Event to signal.
 *
 * @return N/A
 */
#define fiber_event_send task_event_send

/**
 * @brief Set event handler request.
 *
 * <b> Legacy API </b>
 *
 * This routine specifies the event handler that runs in the context of the
 * microkernel server fiber when the associated event is signaled. Specifying
 * a non-NULL handler installs a new handler, while specifying a NULL event
 * handler removes the existing event handler.
 *
 * A new event handler cannot be installed if one already exists for that event.
 * The old handler must be removed first. However, the NULL event handler can be
 * replaced with itself.
 *
 * @param legacy_event Event upon which to register.
 * @param handler Function pointer to handler.
 *
 * @retval RC_FAIL If an event handler exists or the event number is invalid.
 * @retval RC_OK Otherwise.
 */
static inline int task_event_handler_set(kevent_t legacy_event,
					 kevent_handler_t handler)
{
	struct k_alert *alert = (struct k_alert *)legacy_event;

	if ((alert->handler != NULL) && (handler != NULL)) {
		/* can't overwrite an existing event handler */
		return RC_FAIL;
	}

	alert->handler = (k_alert_handler_t)handler;
	return RC_OK;
}

/**
 * @brief Signal an event request.
 *
 * <b> Legacy API </b>
 *
 * This routine signals the specified event from a task. If an event handler
 * is installed for that event, it will run. If no event handler is installed,
 * any task waiting on the event is released.
 *
 * @param legacy_event Event to signal.
 *
 * @retval RC_FAIL If the event number is invalid.
 * @retval RC_OK Otherwise.
 */
static inline int task_event_send(kevent_t legacy_event)
{
	k_alert_send((struct k_alert *)legacy_event);
	return RC_OK;
}

/**
 * @brief Test for an event request with timeout.
 *
 * <b> Legacy API </b>
 *
 * This routine tests an event to see if it has been signaled.
 *
 * @param legacy_event Event to test.
 * @param timeout Determines the action to take when the event has not yet
 *        been signaled.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *        Otherwise, wait up to the specified number of ticks before
 *        timing out.
 *
 * @retval RC_OK Successfully received signaled event
 * @retval RC_TIME Timed out while waiting for signaled event
 * @retval RC_FAIL Failed to immediately receive signaled event when
 *                 timeout = TICKS_NONE
 */
static inline int task_event_recv(kevent_t legacy_event, int32_t timeout)
{
	return _error_to_rc(k_alert_recv((struct k_alert *)legacy_event,
					 _ticks_to_ms(timeout)));
}

/**
 * @brief Define a private microkernel event
 *
 * <b> Legacy API </b>
 *
 * This declares and initializes a private event. The new event
 * can be passed to the microkernel event functions.
 *
 * @param name Name of the event
 * @param event_handler Function to handle the event (can be NULL)
 */
#define DEFINE_EVENT(name, event_handler) \
	K_ALERT_DEFINE(_k_event_obj_##name, event_handler, 1); \
	struct k_alert * const name = &(_k_event_obj_##name)

/* memory maps */

#define kmemory_map_t struct k_mem_slab *

/**
 * @brief Allocate memory map block.
 *
 * <b> Legacy API </b>
 *
 * This routine allocates a block from memory map @a map, and saves the
 * block's address in the area indicated by @a mptr. When no block is available,
 * the routine waits until either one can be allocated, or until the specified
 * time limit is reached.
 *
 * @param map Memory map name.
 * @param mptr Pointer to memory block address area.
 * @param timeout Determines the action to take when the memory map is
 *   exhausted.
 *   For TICKS_NONE, return immediately.
 *   For TICKS_UNLIMITED, wait as long as necessary.
 *   Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @retval RC_OK Successfully allocated memory block.
 * @retval RC_TIME Timed out while waiting for memory block.
 * @retval RC_FAIL Failed to immediately allocate memory block when
 * @a timeout = TICKS_NONE.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline int task_mem_map_alloc(kmemory_map_t map, void **mptr,
					int32_t timeout)
{
	return _error_to_rc(k_mem_slab_alloc(map, mptr,
			    _ticks_to_ms(timeout)));
}

/**
 * @brief Return memory slab block.
 *
 * <b> Legacy API </b>
 *
 * This routine returns a block to the specified memory slab.
 *
 * @param m Memory slab name.
 * @param p Memory block address.
 *
 * @return N/A
 */
#define task_mem_map_free k_mem_slab_free


/**
 * @brief Read the number of used blocks in a memory map.
 *
 * <b> Legacy API </b>
 *
 * This routine returns the number of blocks in use for the memory map.
 *
 * @param map Memory map name.
 *
 * @return Number of used blocks.
 */
static inline int task_mem_map_used_get(kmemory_map_t map)
{
	return (int)k_mem_slab_num_used_get(map);
}

/**
 * @brief Define a private microkernel memory map.
 *
 * <b> Legacy API </b>
 *
 * @param name Memory map name.
 * @param map_num_blocks Number of blocks.
 * @param map_block_size Size of each block, in bytes.
 */
#define DEFINE_MEM_MAP(name, map_num_blocks, map_block_size) \
	K_MEM_SLAB_DEFINE(_k_mem_map_obj_##name, map_block_size, \
			  map_num_blocks, 4); \
	struct k_mem_slab *const name = &_k_mem_map_obj_##name


/* memory pools */

#define k_block k_mem_block
#define kmemory_pool_t struct k_mem_pool *
#define pool_struct k_mem_pool

/**
 * @brief Allocate memory pool block.
 *
 * <b> Legacy API </b>
 *
 * This routine allocates a block of at least @a reqsize bytes from memory pool
 * @a pool_id, and saves its information in block descriptor @a blockptr. When
 * no such block is available, the routine waits either until one can be
 * allocated, or until the specified time limit is reached.
 *
 * @param blockptr Pointer to block descriptor.
 * @param pool_id Memory pool name.
 * @param reqsize Requested block size, in bytes.
 * @param timeout Determines the action to take when the memory pool is
 *   exhausted.
 *   For TICKS_NONE, return immediately.
 *   For TICKS_UNLIMITED, wait as long as necessary.
 *   Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @retval RC_OK Successfully allocated memory block
 * @retval RC_TIME Timed out while waiting for memory block
 * @retval RC_FAIL Failed to immediately allocate memory block when
 * @a timeout = TICKS_NONE
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline int task_mem_pool_alloc(struct k_block *blockptr,
				      kmemory_pool_t pool_id,
				      int reqsize, int32_t timeout)
{
	return _error_to_rc(k_mem_pool_alloc(pool_id, blockptr, reqsize,
						_ticks_to_ms(timeout)));
}

/**
 * @brief Return memory pool block.
 *
 * <b> Legacy API </b>
 *
 * This routine returns a block to the memory pool from which it was allocated.
 *
 * @param block Pointer to block descriptor.
 *
 * @return N/A
 */
#define task_mem_pool_free k_mem_pool_free

/**
 * @brief Defragment memory pool.
 *
 * <b> Legacy API </b>
 *
 * This routine concatenates unused blocks that can be merged in memory pool
 * @a p.
 *
 * Doing a full defragmentation of a memory pool before allocating a set
 * of blocks may be more efficient than having the pool do an implicit
 * partial defragmentation each time a block is allocated.
 *
 * @param pool Memory pool name.
 *
 * @return N/A
 */
#define task_mem_pool_defragment k_mem_pool_defrag

/**
 * @brief Allocate memory
 *
 * <b> Legacy API </b>
 *
 * This routine  provides traditional malloc semantics and is a wrapper on top
 * of microkernel pool alloc API. It returns an aligned memory address which
 * points to the start of a memory block of at least \p size bytes.
 * This memory comes from heap memory pool, consequently the app should
 * specify its intention to use a heap pool via the HEAP_SIZE keyword in
 * MDEF file, if it uses this API.
 * When not enough free memory is available in the heap pool, it returns NULL
 *
 * @param size Size of memory requested by the caller.
 *
 * @retval address of the block if successful otherwise returns NULL
 */
#define task_malloc k_malloc

/**
 * @brief Free memory allocated through task_malloc
 *
 * <b> Legacy API </b>
 *
 * This routine provides traditional free semantics and is intended to free
 * memory allocated using task_malloc API.
 *
 * @param ptr pointer to be freed
 *
 * @return NA
 */
#define task_free k_free

/* message queues */

#define kfifo_t struct k_msgq *

/**
 * @brief FIFO enqueue request.
 *
 * <b> Legacy API </b>
 *
 * This routine adds an item to the FIFO queue. When the FIFO is full,
 * the routine will wait either for space to become available, or until the
 * specified time limit is reached.
 *
 * @param queue FIFO queue.
 * @param data Pointer to data to add to queue.
 * @param timeout Determines the action to take when the FIFO is full.
 * For TICKS_NONE, return immediately.
 * For TICKS_UNLIMITED, wait as long as necessary.
 * Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @retval RC_OK Successfully added item to FIFO.
 * @retval RC_TIME Timed out while waiting to add item to FIFO.
 * @retval RC_FAIL Failed to immediately add item to FIFO when
 * @a timeout = TICKS_NONE.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline int task_fifo_put(kfifo_t queue, void *data, int32_t timeout)
{
	return _error_to_rc(k_msgq_put(queue, data, _ticks_to_ms(timeout)));
}

/**
 * @brief FIFO dequeue request.
 *
 * <b> Legacy API </b>
 *
 * This routine fetches the oldest item from the FIFO queue. When the FIFO is
 * found empty, the routine will wait either until an item is added to the FIFO
 * queue or until the specified time limit is reached.
 *
 * @param queue FIFO queue.
 * @param data Pointer to storage location of the FIFO entry.
 * @param timeout Affects the action to take when the FIFO is empty.
 * For TICKS_NONE, return immediately.
 * For TICKS_UNLIMITED, wait as long as necessary.
 * Otherwise wait up to the specified number of ticks before timing out.
 *
 * @retval RC_OK Successfully fetched item from FIFO.
 * @retval RC_TIME Timed out while waiting to fetch item from FIFO.
 * @retval RC_FAIL Failed to immediately fetch item from FIFO when
 * @a timeout = TICKS_NONE.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline int task_fifo_get(kfifo_t queue, void *data, int32_t timeout)
{
	return _error_to_rc(k_msgq_get(queue, data, _ticks_to_ms(timeout)));
}

/**
 * @brief Purge the FIFO of all its entries.
 *
 * <b> Legacy API </b>
 *
 * @param queue FIFO queue.
 *
 * @return RC_OK on purge.
 */
static inline int task_fifo_purge(kfifo_t queue)
{
	k_msgq_purge(queue);
	return RC_OK;
}

/**
 * @brief Query the number of FIFO entries.
 *
 * <b> Legacy API </b>
 *
 * @param queue FIFO queue.
 *
 * @return # of FIFO entries on query.
 */
static inline int task_fifo_size_get(kfifo_t queue)
{
	return queue->used_msgs;
}

/**
 * @brief Define a private microkernel FIFO.
 *
 * <b> Legacy API </b>
 *
 * This declares and initializes a private FIFO. The new FIFO
 * can be passed to the microkernel FIFO functions.
 *
 * @param name Name of the FIFO.
 * @param q_depth Depth of the FIFO.
 * @param q_width Width of the FIFO.
 */
#define DEFINE_FIFO(name, q_depth, q_width) \
	K_MSGQ_DEFINE(_k_fifo_obj_##name, q_width, q_depth, 4); \
	struct k_msgq * const name = &_k_fifo_obj_##name

/* mailboxes */

#define kmbox_t struct k_mbox *

struct k_msg {
	/** Mailbox ID */
	kmbox_t mailbox;
	/** size of message (bytes) */
	uint32_t size;
	/** information field, free for user   */
	uint32_t info;
	/** pointer to message data at sender side */
	void *tx_data;
	/** pointer to message data at receiver    */
	void *rx_data;
	/** for async message posting   */
	struct k_block tx_block;
	/** sending task */
	ktask_t tx_task;
	/** receiving task */
	ktask_t rx_task;
	/** internal use only */
	union {
		/** for 2-steps data transfer operation */
		struct k_args *transfer;
		/** semaphore to signal when asynchr. call */
		ksem_t sema;
	} extra;
};

/**
 * @brief Send a message to a mailbox.
 *
 * <b> Legacy API </b>
 *
 * This routine sends a message to a mailbox and looks for a matching receiver.
 *
 * @param mbox Mailbox.
 * @param prio Priority of data transfer.
 * @param msg Pointer to message to send.
 * @param timeout Determines the action to take when there is no waiting
 * receiver.
 * For TICKS_NONE, return immediately.
 * For TICKS_UNLIMITED, wait as long as necessary.
 * Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @return RC_OK Successfully delivered message.
 * @return RC_TIME Timed out while waiting to deliver message.
 * @return RC_FAIL Failed to immediately deliver message when
 * @a timeout = TICKS_NONE.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
int task_mbox_put(kmbox_t mbox, kpriority_t prio, struct k_msg *msg,
		  int32_t timeout);

/**
 * @brief Send a message asynchronously to a mailbox.
 *
 * <b> Legacy API </b>
 *
 * This routine sends a message to a mailbox and does not wait for a matching
 * receiver. No exchange header is returned to the sender. When the data
 * has been transferred to the receiver, the semaphore signaling is performed.
 *
 * @param mbox Mailbox to which to send message.
 * @param prio Priority of data transfer.
 * @param msg Pointer to message to send.
 * @param sema Semaphore to signal when transfer is complete.
 *
 * @return N/A
 */
void task_mbox_block_put(kmbox_t mbox, kpriority_t prio, struct k_msg *msg,
			 ksem_t sema);

/**
 * @brief Get @b struct @b k_msg message header structure information from
 *
 * <b> Legacy API </b>
 * a mailbox and wait with timeout.
 *
 * @param mbox Mailbox.
 * @param msg Pointer to message.
 * @param timeout Determines the action to take when there is no waiting
 * receiver.
 * For TICKS_NONE, return immediately.
 * For TICKS_UNLIMITED, wait as long as necessary.
 * Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @return RC_OK Successfully received message.
 * @return RC_TIME Timed out while waiting to receive message.
 * @return RC_FAIL Failed to immediately receive message when
 * @a timeout = TICKS_NONE.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
int task_mbox_get(kmbox_t mbox, struct k_msg *msg, int32_t timeout);

/**
 * @brief Get message data.
 *
 * <b> Legacy API </b>
 *
 * Call this routine for one of two reasons:
 * 1. To transfer data when the call to @b task_mbox_get() yields an existing
 *    field in the @b struct @b k_msg header structure.
 * 2. To wake up and release a transmitting task currently blocked from calling
 *    @b task_mbox_put().
 *
 * @param msg Message from which to get data.
 *
 * @return N/A
 */
void task_mbox_data_get(struct k_msg *msg);

/**
 * @brief Retrieve message data into a block, with time-limited waiting.
 *
 * <b> Legacy API </b>
 *
 * @param msg Message from which to get data.
 * @param block Block.
 * @param pool_id Memory pool name.
 * @param timeout Determines the action to take when no waiting sender exists.
 * For TICKS_NONE, return immediately.
 * For TICKS_UNLIMITED, wait as long as necessary.
 * Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @retval RC_OK Successful retrieval of message data.
 * @retval RC_TIME Timed out while waiting to receive message data.
 * @retval RC_FAIL Failed to immediately receive message data when
 * @a timeout = TICKS_NONE.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
int task_mbox_data_block_get(struct k_msg *msg, struct k_block *block,
			     kmemory_pool_t pool_id, int32_t timeout);

/**
 * @brief Define a private microkernel mailbox.
 *
 * <b> Legacy API </b>
 *
 * This routine declares and initializes a private mailbox. The new mailbox
 * can be passed to the microkernel mailbox functions.
 *
 * @param name Name of the mailbox
 */
#define DEFINE_MAILBOX(name) \
	K_MBOX_DEFINE(_k_mbox_obj_##name); \
	struct k_mbox * const name = &_k_mbox_obj_##name

/* pipes */

#define kpipe_t struct k_pipe *

/**
 * @brief Pipe write request.
 *
 * <b> Legacy API </b>
 *
 * Attempt to write data from a memory-buffer area to the
 * specified pipe with a timeout option.
 *
 * @param id Pipe ID.
 * @param buffer Buffer.
 * @param bytes_to_write Number of bytes to write.
 * @param bytes_written Pointer to number of bytes written.
 * @param options Pipe options.
 * @param timeout Determines the action to take when the pipe is already full.
 *  For TICKS_NONE, return immediately.
 *  For TICKS_UNLIMITED, wait as long as necessary.
 *  Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @retval RC_OK Successfully wrote data to pipe.
 * @retval RC_ALIGNMENT Data is improperly aligned.
 * @retval RC_INCOMPLETE Only some of the data was written to the pipe when
 * @a options = _ALL_N.
 * @retval RC_TIME Timed out while waiting to write to pipe.
 * @retval RC_FAIL Failed to immediately write to pipe when
 * @a timeout = TICKS_NONE
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline int task_pipe_put(kpipe_t id, void *buffer, int bytes_to_write,
				int *bytes_written, K_PIPE_OPTION options,
				int32_t timeout)
{
	size_t  min_xfer = (size_t)options;

	__ASSERT((options == _0_TO_N) ||
		 (options == _1_TO_N) ||
		 (options == _ALL_N), "Invalid pipe option");

	*bytes_written = 0;

	if (bytes_to_write == 0) {
		return RC_FAIL;
	}

	if ((options == _0_TO_N) && (timeout != K_NO_WAIT)) {
		return RC_FAIL;
	}

	if (options == _ALL_N) {
		min_xfer = bytes_to_write;
	}

	return _error_to_rc(k_pipe_put(id, buffer, bytes_to_write,
					(size_t *)bytes_written, min_xfer,
					_ticks_to_ms(timeout)));
}

/**
 * @brief Pipe read request.
 *
 * <b> Legacy API </b>
 *
 * Attempt to read data into a memory buffer area from the
 * specified pipe with a timeout option.
 *
 * @param id Pipe ID.
 * @param buffer Buffer.
 * @param bytes_to_read Number of bytes to read.
 * @param bytes_read Pointer to number of bytes read.
 * @param options Pipe options.
 * @param timeout Determines the action to take when the pipe is already full.
 *  For TICKS_NONE, return immediately.
 *  For TICKS_UNLIMITED, wait as long as necessary.
 *  Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @retval RC_OK Successfully read data from pipe.
 * @retval RC_ALIGNMENT Data is improperly aligned.
 * @retval RC_INCOMPLETE Only some of the data was read from the pipe when
 * @a options = _ALL_N.
 * @retval RC_TIME Timed out waiting to read from pipe.
 * @retval RC_FAIL Failed to immediately read from pipe when
 * @a timeout = TICKS_NONE.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline int task_pipe_get(kpipe_t id, void *buffer, int bytes_to_read,
				int *bytes_read, K_PIPE_OPTION options,
				int32_t timeout)
{
	size_t  min_xfer = (size_t)options;

	__ASSERT((options == _0_TO_N) ||
		 (options == _1_TO_N) ||
		 (options == _ALL_N), "Invalid pipe option");

	*bytes_read = 0;

	if (bytes_to_read == 0) {
		return RC_FAIL;
	}

	if ((options == _0_TO_N) && (timeout != K_NO_WAIT)) {
		return RC_FAIL;
	}

	if (options == _ALL_N) {
		min_xfer = bytes_to_read;
	}

	return _error_to_rc(k_pipe_get(id, buffer, bytes_to_read,
					(size_t *)bytes_read, min_xfer,
					_ticks_to_ms(timeout)));
}

#if CONFIG_NUM_PIPE_ASYNC_MSGS > 0
/**
 * @brief Send a block of data asynchronously to a pipe
 *
 * <b> Legacy API </b>
 *
 * This routine asynchronously sends a message from the pipe specified by
 * @a id. Once all @a size bytes have been accepted by the pipe, it will
 * free the memory block @a block and give the semaphore @a sem (if specified).
 *
 * @param id Pipe ID.
 * @param block Memory block containing data to send
 * @param size Number of data bytes in memory block to send
 * @param sem Semaphore to signal upon completion
 *
 * @retval RC_OK Successfully sent data to the pipe.
 * @retval RC_FAIL Block size is zero
 */
static inline int task_pipe_block_put(kpipe_t id, struct k_block block,
				      int size, ksem_t sem)
{
	if (size == 0) {
		return RC_FAIL;
	}

	k_pipe_block_put(id, &block, size, sem);

	return RC_OK;
}
#endif /* CONFIG_NUM_PIPE_ASYNC_MSGS > 0 */

/**
 * @brief Define a private microkernel pipe.
 *
 * <b> Legacy API </b>
 *
 * @param name Name of the pipe.
 * @param pipe_buffer_size Size of the pipe buffer (in bytes)
 */
#define DEFINE_PIPE(name, pipe_buffer_size) \
	K_PIPE_DEFINE(_k_pipe_obj_##name, pipe_buffer_size, 4); \
	struct k_pipe * const name = &_k_pipe_obj_##name

#define nano_fifo k_fifo

/**
 * @brief Initialize a nanokernel FIFO (fifo) object.
 *
 * <b> Legacy API </b>
 *
 * This function initializes a nanokernel FIFO (fifo) object
 * structure.
 *
 * It can be called from either a fiber or task.
 *
 * @param fifo FIFO to initialize.
 *
 * @return N/A
 */
#define nano_fifo_init k_fifo_init

/* nanokernel fifos */

/**
 * @brief Add an element to the end of a FIFO.
 *
 * <b> Legacy API </b>
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * FIFO data items must be aligned on a 4-byte boundary, as the kernel reserves
 * the first 32 bits of each item for use as a pointer to the next data item in
 * the FIFO's link list.  Each data item added to the FIFO must include and
 * reserve these first 32 bits.
 *
 * @param fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 */
#define nano_fifo_put k_fifo_put

/**
 * @brief Add an element to the end of a FIFO from an ISR context.
 *
 * <b> Legacy API </b>
 *
 * Like nano_fifo_put(), but may only be called from an ISR.
 *
 * @sa nano_fifo_put
 */
#define nano_isr_fifo_put k_fifo_put

/**
 * @brief Add an element to the end of a FIFO from a fiber.
 *
 * <b> Legacy API </b>
 *
 * Like nano_fifo_put(), but may only be called from a fiber.
 *
 * @sa nano_fifo_put
 */
#define nano_fiber_fifo_put k_fifo_put

/**
 * @brief Add an element to the end of a FIFO.
 *
 * <b> Legacy API </b>
 *
 * Like nano_fifo_put(), but may only be called from a task.
 *
 * @sa nano_fifo_put
 */
#define nano_task_fifo_put k_fifo_put

/**
 * @brief Atomically add a list of elements to the end of a FIFO.
 *
 * <b> Legacy API </b>
 *
 * This routine adds a list of elements in one shot to the end of a FIFO
 * object. If fibers are pending on the FIFO object, they become ready to run.
 * If this API is called from a task, the highest priority one will preempt the
 * running task once the put operation is complete.
 *
 * If enough fibers are waiting on the FIFO, the address of each element given
 * to fibers is returned to the waiting fiber. The remaining elements are
 * linked to the end of the list.
 *
 * The list must be a singly-linked list, where each element only has a pointer
 * to the next one. The list must be NULL-terminated.
 *
 * Unlike the fiber/ISR versions of this API which is not much different
 * conceptually than calling nano_fifo_put once for each element to queue, the
 * behaviour is indeed different for tasks. There is no context switch being
 * done for each element queued, so the task can enqueue all elements without
 * being interrupted by a fiber being woken up.
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param fifo FIFO on which to interact.
 * @param head head of singly-linked list
 * @param tail tail of singly-linked list
 *
 * @return N/A
 *
 * @sa nano_fifo_put_slist, nano_isr_fifo_put_list, nano_fiber_fifo_put_list,
 *     nano_task_fifo_put_list
 */
#define nano_fifo_put_list k_fifo_put_list

/**
 * @brief Atomically add a list of elements to the end of a FIFO from an ISR.
 *
 * <b> Legacy API </b>
 *
 * Like nano_fifo_put_list(), but may only be called from an ISR.
 *
 * @sa nano_fifo_put_list
 */
#define nano_isr_fifo_put_list k_fifo_put_list

/**
 *
 * @brief Atomically add a list of elements to the end of a FIFO from a fiber.
 *
 * <b> Legacy API </b>
 *
 * Like nano_fifo_put_list(), but may only be called from a fiber.
 *
 * @sa nano_fifo_put_list
 */
#define nano_fiber_fifo_put_list k_fifo_put_list

/**
 * @brief Atomically add a list of elements to the end of a FIFO from a fiber.
 *
 * <b> Legacy API </b>
 *
 * Like nano_fifo_put_list(), but may only be called from a task.
 *
 * @sa nano_fifo_put_list
 */
#define nano_task_fifo_put_list k_fifo_put_list

/**
 * @brief Atomically add a list of elements to the end of a FIFO.
 *
 * <b> Legacy API </b>
 *
 * See nano_fifo_put_list for the description of the behaviour.
 *
 * It takes a pointer to a sys_slist_t object instead of the head and tail of a
 * custom singly-linked list. The sys_slist_t object is invalid afterwards and
 * must be re-initialized via sys_slist_init().
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param fifo FIFO on which to interact.
 * @param list pointer to singly-linked list
 *
 * @return N/A
 *
 * @sa nano_fifo_put_list, nano_isr_fifo_put_slist, nano_fiber_fifo_put_slist,
 *     nano_task_fifo_put_slist
 */
#define nano_fifo_put_slist k_fifo_put_slist

/**
 * @brief Atomically add a list of elements to the end of a FIFO from an ISR.
 *
 * <b> Legacy API </b>
 *
 * Like nano_fifo_put_slist(), but may only be called from an ISR.
 *
 * @sa nano_fifo_put_slist
 */
#define nano_isr_fifo_put_slist k_fifo_put_slist

/**
 * @brief Atomically add a list of elements to the end of a FIFO from a fiber.
 *
 * <b> Legacy API </b>
 *
 * Like nano_fifo_put_slist(), but may only be called from a fiber.
 *
 * @sa nano_fifo_put_slist
 */
#define nano_fiber_fifo_put_slist k_fifo_put_slist

/**
 * @brief Atomically add a list of elements to the end of a FIFO from a fiber.
 *
 * Like nano_fifo_put_slist(), but may only be called from a fiber.
 *
 * @sa nano_fifo_put_slist
 */
#define nano_task_fifo_put_slist k_fifo_put_slist

#ifdef KERNEL /* ztest layer redefines to a different function */
/**
 * @brief Get an element from the head a FIFO.
 *
 * <b> Legacy API </b>
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * If no element is available, the function returns NULL. The first word in
 * the element contains invalid data because its memory location was used to
 * store a pointer to the next element in the linked list.
 *
 * @param fifo FIFO on which to interact.
 * @param timeout_in_ticks Affects the action taken should the FIFO be empty.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necessary. Otherwise, wait up to the specified number of ticks
 * before timing out.
 *
 * @warning If it is to be called from the context of an ISR, then @a
 * timeout_in_ticks must be set to TICKS_NONE.
 *
 * @return Pointer to head element in the list when available.
 *         NULL Otherwise.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline void *nano_fifo_get(struct nano_fifo *fifo,
				  int32_t timeout_in_ticks)
{
	return k_fifo_get((struct k_fifo *)fifo,
			  _ticks_to_ms(timeout_in_ticks));
}
#else
void *nano_fifo_get(struct nano_fifo *fifo, int32_t timeout_in_ticks);
#endif /* KERNEL */

/**
 * @brief Get an element from the head of a FIFO from an ISR context.
 *
 * <b> Legacy API </b>
 *
 * Like nano_fifo_get(), but may only be called from an ISR with a timeout
 * of TICKS_NONE.
 *
 * @sa nano_fifo_get
 */
#define nano_isr_fifo_get nano_fifo_get

/**
 * @brief Get an element from the head of a FIFO from a fiber.
 *
 * <b> Legacy API </b>
 *
 * Like nano_fifo_get(), but may only be called from a fiber.
 *
 * @sa nano_fifo_get
 */
#define nano_fiber_fifo_get nano_fifo_get

/**
 * @brief Get an element from a FIFO's head that comes from a task, poll if
 * empty.
 *
 * <b> Legacy API </b>
 *
 * Like nano_fifo_get(), but may only be called from a task.
 *
 * @sa nano_fifo_get
 */
#define nano_task_fifo_get nano_fifo_get

/* nanokernel lifos */

#define nano_lifo k_lifo

/**
 * @brief Initialize a nanokernel linked list LIFO (lifo) object.
 *
 * <b> Legacy API </b>
 *
 * This function initializes a nanokernel system-level linked list LIFO
 * (lifo) object structure.
 *
 * It is called from either a fiber or task.
 *
 * @param lifo LIFO to initialize.
 *
 * @return N/A
 */
#define nano_lifo_init k_lifo_init

/**
 * @brief Prepend an element to a LIFO.
 *
 * <b> Legacy API </b>
 *
 * This routine adds an element to the LIFOs' object head
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param lifo LIFO on which to put.
 * @param data Data to insert.
 *
 * @return N/A
 */
#define nano_lifo_put k_lifo_put

/**
 * @brief Prepend an element to a LIFO without a context switch.
 *
 * <b> Legacy API </b>
 *
 * Like nano_lifo_put(), but may only be called from an ISR. A fiber
 * pending on the LIFO object will be made ready, but will NOT be scheduled
 * to execute.
 *
 * @sa nano_lifo_put
 */
#define nano_isr_lifo_put k_lifo_put

/**
 * @brief Prepend an element to a LIFO without a context switch.
 *
 * <b> Legacy API </b>
 *
 * Like nano_lifo_put(), but may only be called from a fiber. A fiber
 * pending on the LIFO object will be made ready, but will NOT be scheduled
 * to execute.
 *
 * @sa nano_lifo_put
 */
#define nano_fiber_lifo_put k_lifo_put

/**
 * @brief Add an element to the LIFO's linked list head.
 *
 * <b> Legacy API </b>
 *
 * Like nano_lifo_put(), but may only be called from a task. A fiber
 * pending on the LIFO object will be made ready, and will preempty the
 * running task immediately.
 *
 * @sa nano_lifo_put
 */
#define nano_task_lifo_put k_lifo_put

/**
 * @brief Get the first element from a LIFO.
 *
 * <b> Legacy API </b>
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param lifo LIFO on which to receive.
 * @param timeout_in_ticks Affects the action taken should the LIFO be empty.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necesssary. Otherwise wait up to the specified number of ticks
 * before timing out.
 *
 * @warning If it is to be called from the context of an ISR, then @a
 * timeout_in_ticks must be set to TICKS_NONE.
 *
 * @return Pointer to head element in the list when available.
 *         NULL Otherwise.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
static inline void *nano_lifo_get(struct nano_lifo *lifo,
				  int32_t timeout_in_ticks)
{
	return k_lifo_get((struct k_lifo *)lifo,
			  _ticks_to_ms(timeout_in_ticks));
}

/**
 * @brief Remove the first element from a LIFO linked list.
 *
 * <b> Legacy API </b>
 *
 * Like nano_lifo_get(), but may only be called from an ISR with a timeout
 * of TICKS_NONE.
 *
 * @sa nano_lifo_get
 */
#define nano_isr_lifo_get nano_lifo_get

/**
 * @brief Prepend an element to a LIFO without a context switch.
 *
 * <b> Legacy API </b>
 *
 * Like nano_lifo_get(), but may only be called from a fiber.
 *
 * @sa nano_lifo_get
 */
#define nano_fiber_lifo_get nano_lifo_get

/**
 * @brief Remove the first element from a LIFO linked list.
 *
 * <b> Legacy API </b>
 *
 * Like nano_lifo_get(), but may only be called from a task.
 *
 * @sa nano_lifo_get
 */
#define nano_task_lifo_get nano_lifo_get

/* nanokernel stacks */

#define nano_stack k_stack

/**
 * @brief Initialize a nanokernel stack object.
 *
 * <b> Legacy API </b>
 *
 * This function initializes a nanokernel stack object structure.
 *
 * It is called from either a fiber or a task.
 *
 * @return N/A
 */
static inline void nano_stack_init(struct nano_stack *stack, uint32_t *data)
{
	k_stack_init(stack, data, UINT_MAX);
}

/**
 * @brief Push data onto a stack.
 *
 * <b> Legacy API </b>
 *
 * This routine pushes a data item onto a stack object. It is a convenience
 * wrapper for the execution of context-specific APIs and is helpful when
 * the exact execution context is not known. However, it should be avoided
 * when the context is known up-front to avoid unnecessary overhead.
 *
 * @param stack Stack on which to interact.
 * @param data Data to push on stack.
 *
 * @return N/A
 */
#define nano_stack_push k_stack_push

/**
 * @brief Push data onto a stack (no context switch).
 *
 * <b> Legacy API </b>
 *
 * Like nano_stack_push(), but may only be called from an ISR. A fiber that
 * pends on the stack object becomes ready but will NOT be scheduled to execute.
 *
 * @sa nano_stack_push
 */
#define nano_isr_stack_push k_stack_push

/**
 * @brief Push data onto a stack (no context switch).
 *
 * <b> Legacy API </b>
 *
 * Like nano_stack_push(), but may only be called from a fiber. A fiber that
 * pends on the stack object becomes ready but will NOT be scheduled to execute.
 *
 * @sa nano_stack_push
 */
#define nano_fiber_stack_push k_stack_push

/**
 * @brief Push data onto a nanokernel stack.
 *
 * <b> Legacy API </b>
 *
 * Like nano_stack_push(), but may only be called from a task. A fiber that
 * pends on the stack object becomes ready and preempts the running task
 * immediately.
 *
 * @sa nano_stack_push
 */
#define nano_task_stack_push k_stack_push

/**
 * @brief Pop data off a stack.
 *
 * <b> Legacy API </b>
 *
 * This routine pops the first data word from a nanokernel stack object.
 * It is a convenience wrapper for the execution of context-specific APIs
 * and is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * When the stack is not empty, a data word is popped and copied to the
 * provided address @a data and a non-zero value is returned. When the routine
 * finds an empty stack, zero is returned.
 *
 * @param stack Stack on which to interact.
 * @param data Container for data to pop
 * @param timeout_in_ticks Determines the action to take when the FIFO
 *        is empty.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *        Otherwise, wait up to the specified number of ticks before timing
 *        out.
 *
 * @retval 1 When data is popped from the stack.
 * @retval 0 Otherwise.
 */
static inline int nano_stack_pop(struct nano_stack *stack, uint32_t *data,
				 int32_t timeout_in_ticks)
{
	return k_stack_pop((struct k_stack *)stack, data,
			   _ticks_to_ms(timeout_in_ticks)) == 0 ? 1 : 0;
}

/**
 * @brief Pop data from a nanokernel stack.
 *
 * <b> Legacy API </b>
 *
 * Like nano_stack_pop(), but may only be called from an ISR.
 *
 * @sa nano_stack_pop
 */
#define nano_isr_stack_pop nano_stack_pop

/**
 * @brief Pop data from a nanokernel stack.
 *
 * <b> Legacy API </b>
 *
 * Like nano_stack_pop(), but may only be called from a fiber.
 *
 * @sa nano_stack_pop
 */
#define nano_fiber_stack_pop nano_stack_pop

/**
 * @brief Pop data from a nanokernel stack.
 *
 * <b> Legacy API </b>
 *
 * Like nano_stack_pop(), but may only be called from a task.
 *
 * @sa nano_stack_pop
 */
#define nano_task_stack_pop nano_stack_pop

/* kernel clocks */

extern int32_t _ms_to_ticks(int32_t ms);

/**
 * @brief Return the current system tick count.
 *
 * <b> Legacy API </b>
 *
 * @return The current system tick count.
 */
extern int64_t sys_tick_get(void);

/**
 * @brief Return the lower part of the current system tick count.
 *
 * <b> Legacy API </b>
 *
 * @return The current system tick count.
 */
extern uint32_t sys_tick_get_32(void);

/**
 * @brief Return number of ticks elapsed since a reference time.
 *
 * <b> Legacy API </b>
 *
 * @param reftime Reference time.
 *
 * @return The tick count since reference time; undefined for first invocation.
 */
extern int64_t sys_tick_delta(int64_t *reftime);

/**
 *
 * @brief Return 32-bit number of ticks since a reference time.
 *
 * <b> Legacy API </b>
 *
 * @param reftime Reference time.
 *
 * @return A 32-bit tick count since reference time. Undefined for first
 *         invocation.
 */
extern uint32_t sys_tick_delta_32(int64_t *reftime);

/**
 * @brief Return a time stamp in high-resolution format.
 *
 * <b> Legacy API </b>
 *
 * This routine reads the counter register on the processor's high precision
 * timer device. This counter register increments at a relatively high rate
 * (e.g. 20 MHz), and is thus considered a high-resolution timer. This is
 * in contrast to sys_tick_get_32() which returns the value of the system
 * ticks variable.
 *
 * @return The current high-precision clock value.
 */
#define sys_cycle_get_32 k_cycle_get_32

/* microkernel timers */

#if (CONFIG_NUM_DYNAMIC_TIMERS > 0)

#define CONFIG_NUM_TIMER_PACKETS CONFIG_NUM_DYNAMIC_TIMERS

#define ktimer_t struct k_timer *

/**
 * @brief Allocate a timer and return its object identifier.
 *
 * <b> Legacy API </b>
 *
 * @return timer identifier
 */
extern ktimer_t task_timer_alloc(void);

/**
 * @brief Deallocate a timer
 *
 * <b> Legacy API </b>
 *
 * This routine frees the resources associated with the timer.  If a timer was
 * started, it has to be stopped using task_timer_stop() before it can be freed.
 *
 * @param timer   Timer to deallocate.
 *
 * @return N/A
 */
extern void task_timer_free(ktimer_t timer);

/**
 * @brief Start or restart the specified low-resolution timer
 *
 * <b> Legacy API </b>
 *
 * This routine starts or restarts the specified low-resolution timer.
 *
 * Signals the semaphore after a specified number of ticks set by
 * @a duration expires. The timer repeats the expiration/signal cycle
 * each time @a period ticks elapses.
 *
 * Setting @a period to 0 stops the timer at the end of the initial delay.

 * If either @a duration or @a period is passed an invalid value
 * (@a duration <= 0, * @a period < 0), this kernel API acts like a
 * task_timer_stop(): if the allocated timer was still running (from a
 * previous call), it will be cancelled; if not, nothing will happen.
 *
 * @param timer      Timer to start.
 * @param duration   Initial delay in ticks.
 * @param period     Repetition interval in ticks.
 * @param sema       Semaphore to signal.
 *
 * @return N/A
 */
extern void task_timer_start(ktimer_t timer, int32_t duration,
			     int32_t period, ksem_t sema);

/**
 * @brief Restart a timer
 *
 * <b> Legacy API </b>
 *
 * This routine restarts the timer specified by @a timer. The timer must
 * have previously been started by a call to task_timer_start().
 *
 * @param timer      Timer to restart.
 * @param duration   Initial delay.
 * @param period     Repetition interval.
 *
 * @return N/A
 */
static inline void task_timer_restart(ktimer_t timer, int32_t duration,
				      int32_t period)
{
	k_timer_start(timer, _ticks_to_ms(duration), _ticks_to_ms(period));
}

/**
 * @brief Stop a timer
 *
 * <b> Legacy API </b>
 *
 * This routine stops the specified timer. If the timer period has already
 * elapsed, the call has no effect.
 *
 * @param timer   Timer to stop.
 *
 * @return N/A
 */

#define task_timer_stop k_timer_stop

#endif /* CONFIG_NUM_DYNAMIC_TIMERS > 0 */

/* nanokernel timers */

#define nano_timer k_timer

/**
 * @brief Initialize a nanokernel timer object.
 *
 * <b> Legacy API </b>
 *
 * This function initializes a nanokernel timer object structure.
 *
 * It can be called from either a fiber or task.
 *
 * The @a data passed to this function is a pointer to a data structure defined
 * by the user. It contains data that the user wishes to store when initializing
 * the timer and recover when the timer expires. However, the first field of
 * this data structure must be a pointer reserved for the API's use that can be
 * overwritten by the API and, as such, should not contain user data.
 *
 * @param timer Timer.
 * @param data User Data.
 *
 * @return N/A
 */
static inline void nano_timer_init(struct k_timer *timer, void *data)
{
	k_timer_init(timer, NULL, NULL);
	timer->_legacy_data = data;
}

/**
 * @brief Start a nanokernel timer.
 *
 * <b> Legacy API </b>
 *
 * This routine starts a previously initialized nanokernel timer object. The
 * timer will expire in @a ticks system clock ticks. It is also a convenience
 * wrapper for the execution of context-specific APIs and is helpful when the
 * the exact execution context is not known. However, it should be avoided when
 * the context is known up-front to avoid unnecessary overhead.
 *
 * @param timer Timer.
 * @param ticks Number of ticks.
 *
 * @return N/A
 */
static inline void nano_timer_start(struct nano_timer *timer, int ticks)
{
	k_timer_start(timer, _ticks_to_ms(ticks), 0);
}

/**
 * @brief Start a nanokernel timer from an ISR.
 *
 * <b> Legacy API </b>
 *
 * Like nano_timer_start(), but may only be called from an ISR with a
 * timeout of TICKS_NONE.
 *
 * @sa nano_timer_start
 */
#define nano_isr_timer_start nano_timer_start

/**
 * @brief Start a nanokernel timer from a fiber.
 *
 * <b> Legacy API </b>
 *
 * Like nano_timer_start(), but may only be called from a fiber.
 *
 * @sa nano_timer_start
 */
#define nano_fiber_timer_start nano_timer_start

/**
 * @brief Start a nanokernel timer from a task.
 *
 * <b> Legacy API </b>
 *
 * Like nano_timer_start(), but may only be called from a task.
 *
 * @sa nano_timer_start
 */
#define nano_task_timer_start nano_timer_start

/**
 * @brief Wait for a nanokernel timer to expire.
 *
 * <b> Legacy API </b>
 *
 * This routine checks if a previously started nanokernel timer object has
 * expired. It is also a convenience wrapper for the execution of context-
 * specific APIs. It is helpful when the exact execution context is not known.
 * However, it should be avoided when the context is known up-front to avoid
 * unnecessary overhead.
 *
 * @param timer Timer.
 * @param timeout_in_ticks Determines the action to take when the timer has
 *        not expired.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *
 * @retval Pointer to timer initialization data.
 * @retval NULL If timer not expired.
 *
 * @warning If called from an ISR, then @a timeout_in_ticks must be TICKS_NONE.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern void *nano_timer_test(struct nano_timer *timer,
			     int32_t timeout_in_ticks);

/**
 * @brief Make the current ISR check for a timer expiry.
 *
 * <b> Legacy API </b>
 *
 * Like nano_timer_test(), but may only be called from an ISR with a timeout
 * of TICKS_NONE.
 *
 * @sa nano_timer_test
 */
#define nano_isr_timer_test nano_timer_test

/**
 * @brief Make the current fiber check for a timer expiry.
 *
 * <b> Legacy API </b>
 *
 * Like nano_timer_test(), but may only be called from a fiber.
 *
 * @sa nano_timer_test
 */
#define nano_fiber_timer_test nano_timer_test

/**
 * @brief Make the current task check for a timer expiry.
 *
 * <b> Legacy API </b>
 *
 * Like nano_timer_test(), but may only be called from a task.
 *
 * @sa nano_timer_test
 */
#define nano_task_timer_test nano_timer_test

/**
 * @brief Stop a timer.
 *
 * <b> Legacy API </b>
 *
 * This routine stops the specified timer. If the timer period has already
 * elapsed, the call has no effect.
 *
 * @param timer Timer to stop.
 *
 * @return N/A
 */
#define task_timer_stop k_timer_stop

/**
 * @brief Stop a nanokernel timer
 *
 * <b> Legacy API </b>
 *
 * This routine stops a previously started nanokernel timer object. It is also
 * a convenience wrapper for the execution of context-specific APIs. It is
 * helpful when the exact execution context is not known. However, it should be
 * avoided when the context is known up-front to avoid unnecessary overhead.
 *
 * @param timer Timer to stop.
 *
 * @return N/A
 */

#define nano_timer_stop k_timer_stop

/**
 * @brief Stop a nanokernel timer from an ISR.
 *
 * <b> Legacy API </b>
 *
 * Like nano_timer_stop(), but may only be called from an ISR.
 *
 * @sa nano_timer_stop
 */

#define nano_isr_timer_stop k_timer_stop

/**
 * @brief Stop a nanokernel timer.
 *
 * <b> Legacy API </b>
 *
 * Like nano_timer_stop(), but may only be called from a fiber.
 *
 * @sa nano_timer_stop
 */

#define nano_fiber_timer_stop k_timer_stop

/**
 * @brief Stop a nanokernel timer from a task.
 *
 * <b> Legacy API </b>
 *
 * Like nano_timer_stop(), but may only be called from a task.
 *
 * @sa nano_timer_stop
 */

#define nano_task_timer_stop k_timer_stop

/**
 * @brief Get nanokernel timer remaining ticks.
 *
 * <b> Legacy API </b>
 *
 * This function returns the remaining ticks of the previously
 * started nanokernel timer object.
 *
 * @param timer Timer to query
 *
 * @return remaining ticks or 0 if the timer has expired
 */
static inline int32_t nano_timer_ticks_remain(struct nano_timer *timer)
{
	return _ms_to_ticks(k_timer_remaining_get(timer));
}

/* floating point services */

#define USE_FP  K_FP_REGS
#define USE_SSE K_SSE_REGS

/**
 * @brief Enable floating point hardware resources sharing
 *
 * <b> Legacy API </b>
 *
 * This routine dynamically enables the capability of a thread to share floating
 * point hardware resources. The same "floating point" options accepted by
 * fiber_fiber_start() are accepted by this API (i.e. USE_FP and USE_SSE).
 *
 * @param thread_id ID of thread that may share the floating point hardware
 * @param options USE_FP or USE_SSE
 *
 * @return N/A
 */

#define fiber_float_enable k_float_enable

/**
 * @brief Enable floating point hardware resources sharing
 *
 * <b> Legacy API </b>
 *
 * This routine dynamically enables the capability of a thread to share
 * floating point hardware resources. The same "floating point" options
 * accepted by fiber_fiber_start() are accepted by this API
 * (i.e. USE_FP and USE_SSE).
 *
 * @param thread_id ID of thread that may share the floating point hardware
 * @param options USE_FP or USE_SSE
 *
 * @return N/A
 */

#define task_float_enable k_float_enable

/**
 * @brief Disable floating point hardware resources sharing
 *
 * <b> Legacy API </b>
 *
 * This routine dynamically disables the capability of a thread to share
 * floating point hardware resources. The same "floating point" options
 * accepted by fiber_fiber_start() are accepted by this API
 * (i.e. USE_FP and USE_SSE).
 *
 * @param thread_id ID of thread that may not share the floating point hardware
 *
 * @return N/A
 */

#define fiber_float_disable k_float_disable

/**
 * @brief Enable floating point hardware resources sharing
 *
 * <b> Legacy API </b>
 *
 * This routine dynamically disables the capability of a thread to share
 * floating point hardware resources. The same "floating point" options
 * accepted by fiber_fiber_start() are accepted by this API
 * (i.e. USE_FP and USE_SSE).
 *
 * @param thread_id ID of thread that may not share the floating point hardware
 *
 * @return N/A
 */

#define task_float_disable k_float_disable

#endif /* _legacy__h_ */
