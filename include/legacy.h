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

#define DEFINE_TASK(name, prio, entry, stack_size, groups) \
	extern void entry(void); \
	char __noinit __stack _k_thread_obj_##name[stack_size]; \
	struct _static_thread_data _k_thread_data_##name __aligned(4) \
		__in_section(_k_task_list, private, task) = \
		_MDEF_THREAD_INITIALIZER(_k_thread_obj_##name, stack_size, \
				     entry, NULL, NULL, NULL, \
				     NULL, prio, (uint32_t)(groups)); \
	k_tid_t const name = (k_tid_t)_k_thread_obj_##name

#define sys_thread_self_get k_current_get
#define sys_thread_busy_wait k_busy_wait

extern int sys_execution_context_type_get(void);

static inline nano_thread_id_t fiber_start(char *stack, unsigned stack_size,
						nano_fiber_entry_t entry,
						int arg1, int arg2,
						unsigned prio,
						unsigned options)
{
	return k_thread_spawn(stack, stack_size, (k_thread_entry_t)entry,
				(void *)arg1, (void *)arg2, NULL,
				K_PRIO_COOP(prio), options, 0);
}
#define fiber_fiber_start fiber_start
#define task_fiber_start fiber_start

struct fiber_config {
	char *stack;
	unsigned stack_size;
	unsigned prio;
};

#define fiber_start_config(config, entry, arg1, arg2, options) \
	fiber_start(config->stack, config->stack_size, \
		    entry, arg1, arg2, \
		    config->prio, options)
#define fiber_fiber_start_config fiber_start_config
#define task_fiber_start_config fiber_start_config

static inline nano_thread_id_t
fiber_delayed_start(char *stack, unsigned int stack_size_in_bytes,
			nano_fiber_entry_t entry_point, int param1,
			int param2, unsigned int priority,
			unsigned int options, int32_t timeout_in_ticks)
{
	return k_thread_spawn(stack, stack_size_in_bytes,
			      (k_thread_entry_t)entry_point,
			      (void *)param1, (void *)param2, NULL,
			      K_PRIO_COOP(priority), options,
			      _ticks_to_ms(timeout_in_ticks));
}

#define fiber_fiber_delayed_start fiber_delayed_start
#define task_fiber_delayed_start fiber_delayed_start

#define fiber_delayed_start_cancel(fiber) k_thread_cancel((k_tid_t)fiber)
#define fiber_fiber_delayed_start_cancel fiber_delayed_start_cancel
#define task_fiber_delayed_start_cancel fiber_delayed_start_cancel

#define fiber_yield k_yield
#define fiber_abort() k_thread_abort(k_current_get())

extern void _legacy_sleep(int32_t ticks);
#define fiber_sleep _legacy_sleep
#define task_sleep _legacy_sleep

#define fiber_wakeup k_wakeup
#define isr_fiber_wakeup k_wakeup
#define fiber_fiber_wakeup k_wakeup
#define task_fiber_wakeup k_wakeup

#define task_yield k_yield
#define task_priority_set(task, prio) k_thread_priority_set(task, (int)prio)
#define task_entry_set(task, entry) \
	k_thread_entry_set(task, (k_thread_entry_t)entry)
extern void task_abort_handler_set(void (*handler)(void));

/**
 * @brief Process an "offload" request
 *
 * The routine places the @a function into the work queue. This allows
 * the task to execute a routine uninterrupted by other tasks.
 *
 * Note: this routine can be invoked only from a task.
 * In order the routine to work, scheduler must be unlocked.
 *
 * @param func function to call
 * @param argp function arguments
 *
 * @return result of @a function call
 */
extern int task_offload_to_fiber(int (*func)(), void *argp);

#define task_id_get k_current_get
#define task_priority_get() \
	(kpriority_t)(k_thread_priority_get(k_current_get()))
#define task_abort k_thread_abort
#define task_suspend k_thread_suspend
#define task_resume k_thread_resume

extern void task_start(ktask_t task);

static inline void sys_scheduler_time_slice_set(int32_t ticks,
						kpriority_t prio)
{
	k_sched_time_slice_set(_ticks_to_ms(ticks), (int)prio);
}

extern void _k_thread_group_op(uint32_t groups, void (*func)(struct tcs *));

static inline uint32_t task_group_mask_get(void)
{
	extern uint32_t _k_thread_group_mask_get(struct tcs *thread);

	return _k_thread_group_mask_get(k_current_get());
}
#define isr_task_group_mask_get  task_group_mask_get

static inline void task_group_join(uint32_t groups)
{
	extern void _k_thread_group_join(uint32_t groups, struct tcs *thread);

	_k_thread_group_join(groups, k_current_get());
}

static inline void task_group_leave(uint32_t groups)
{
	extern void _k_thread_group_leave(uint32_t groups, struct tcs *thread);

	_k_thread_group_leave(groups, k_current_get());
}

static inline void task_group_start(uint32_t groups)
{
	extern void _k_thread_single_start(struct tcs *thread);

	return _k_thread_group_op(groups, _k_thread_single_start);
}

static inline void task_group_suspend(uint32_t groups)
{
	extern void _k_thread_single_suspend(struct tcs *thread);

	return _k_thread_group_op(groups, _k_thread_single_suspend);
}

static inline void task_group_resume(uint32_t groups)
{
	extern void _k_thread_single_resume(struct tcs *thread);

	return _k_thread_group_op(groups, _k_thread_single_resume);
}

static inline void task_group_abort(uint32_t groups)
{
	extern void _k_thread_single_abort(struct tcs *thread);

	return _k_thread_group_op(groups, _k_thread_single_abort);
}

#define isr_task_id_get() task_id_get()
#define isr_task_priority_get() task_priority_get()

/* mutexes */

#define kmutex_t struct k_mutex *

static inline int task_mutex_lock(kmutex_t id, int32_t timeout)
{
	return _error_to_rc(k_mutex_lock(id, _ticks_to_ms(timeout)));
}

#define task_mutex_unlock k_mutex_unlock

#define DEFINE_MUTEX(name) \
	K_MUTEX_DEFINE(_k_mutex_obj_##name); \
	struct k_mutex * const name = &_k_mutex_obj_##name

/* semaphores */

#define nano_sem k_sem
#define ksem_t struct k_sem *

static inline void nano_sem_init(struct nano_sem *sem)
{
	k_sem_init(sem, 0, UINT_MAX);
}

#define nano_sem_give(id)       k_sem_give((struct k_sem *)id)
#define nano_isr_sem_give(id)   k_sem_give((struct k_sem *)id)
#define nano_fiber_sem_give(id) k_sem_give((struct k_sem *)id)
#define nano_task_sem_give(id)  k_sem_give((struct k_sem *)id)

static inline int nano_sem_take(struct nano_sem *sem, int32_t timeout)
{
	return k_sem_take((struct k_sem *)sem, _ticks_to_ms(timeout))
		== 0 ? 1 : 0;
}
#define nano_isr_sem_take   nano_sem_take
#define nano_fiber_sem_take nano_sem_take
#define nano_task_sem_take  nano_sem_take

#define isr_sem_give   k_sem_give
#define fiber_sem_give k_sem_give
#define task_sem_give  k_sem_give

static inline int task_sem_take(ksem_t sem, int32_t timeout)
{
	return _error_to_rc(k_sem_take(sem, _ticks_to_ms(timeout)));
}

#define task_sem_reset k_sem_reset
#define task_sem_count_get k_sem_count_get

#define nano_sem_count_get k_sem_count_get

#ifdef CONFIG_SEMAPHORE_GROUPS
typedef ksem_t *ksemg_t;

static inline ksem_t task_sem_group_take(ksemg_t group, int32_t timeout)
{
	struct k_sem *sem;

	(void)k_sem_group_take(group, &sem, _ticks_to_ms(timeout));

	return sem;
}

#define task_sem_group_give k_sem_group_give
#define task_sem_group_reset k_sem_group_reset
#endif

#define DEFINE_SEMAPHORE(name) \
	K_SEM_DEFINE(_k_sem_obj_##name, 0, UINT_MAX); \
	struct k_sem * const name = &_k_sem_obj_##name

/* workqueues */

#define nano_work k_work
#define work_handler_t k_work_handler_t
#define nano_workqueue k_work_q
#define nano_delayed_work k_delayed_work

#define nano_work_init k_work_init
#define nano_work_submit_to_queue k_work_submit_to_queue
#define nano_workqueue_start k_work_q_start
#define nano_task_workqueue_start nano_fiber_workqueue_start
#define nano_fiber_workqueue_start nano_fiber_workqueue_start

#if CONFIG_SYS_CLOCK_EXISTS
#define nano_delayed_work_init k_delayed_work_init

static inline int nano_delayed_work_submit_to_queue(struct nano_workqueue *wq,
				      struct nano_delayed_work *work,
				      int ticks)
{
	return k_delayed_work_submit_to_queue(wq, work, _ticks_to_ms(ticks));
}

#define nano_delayed_work_cancel k_delayed_work_cancel
#endif

#define nano_work_submit k_work_submit

#define nano_delayed_work_submit(work, ticks) \
	nano_delayed_work_submit_to_queue(&k_sys_work_q, work, ticks)

/* events */

#define kevent_t const struct k_event *
typedef int (*kevent_handler_t)(int event);

#define isr_event_send task_event_send
#define fiber_event_send task_event_send

static inline int task_event_handler_set(kevent_t legacy_event,
					 kevent_handler_t handler)
{
	struct k_event *event = (struct k_event *)legacy_event;

	if ((event->handler != NULL) && (handler != NULL)) {
		/* can't overwrite an existing event handler */
		return RC_FAIL;
	}

	event->handler = (k_event_handler_t)handler;
	return RC_OK;
}

static inline int task_event_send(kevent_t legacy_event)
{
	k_event_send((struct k_event *)legacy_event);
	return RC_OK;
}

static inline int task_event_recv(kevent_t legacy_event, int32_t timeout)
{
	return _error_to_rc(k_event_recv((struct k_event *)legacy_event,
					 _ticks_to_ms(timeout)));
}

#define DEFINE_EVENT(name, event_handler) \
	K_EVENT_DEFINE(_k_event_obj_##name, event_handler); \
	struct k_event * const name = &(_k_event_obj_##name)

/* memory maps */

#define kmemory_map_t struct k_mem_map *

static inline int task_mem_map_alloc(kmemory_map_t map, void **mptr,
					int32_t timeout)
{
	return _error_to_rc(k_mem_map_alloc(map, mptr, _ticks_to_ms(timeout)));
}

#define task_mem_map_free k_mem_map_free

static inline int task_mem_map_used_get(kmemory_map_t map)
{
	return (int)k_mem_map_num_used_get(map);
}

#define DEFINE_MEM_MAP(name, map_num_blocks, map_block_size) \
	K_MEM_MAP_DEFINE(_k_mem_map_obj_##name, map_block_size, \
			 map_num_blocks, 4); \
	struct k_mem_map *const name = &_k_mem_map_obj_##name


/* memory pools */

#define k_block k_mem_block
#define kmemory_pool_t struct k_mem_pool *
#define pool_struct k_mem_pool

static inline int task_mem_pool_alloc(struct k_block *blockptr,
				      kmemory_pool_t pool_id,
				      int reqsize, int32_t timeout)
{
	return _error_to_rc(k_mem_pool_alloc(pool_id, blockptr, reqsize,
						_ticks_to_ms(timeout)));
}

#define task_mem_pool_free k_mem_pool_free
#define task_mem_pool_defragment k_mem_pool_defrag
#define task_malloc k_malloc
#define task_free k_free

/* message queues */

#define kfifo_t struct k_msgq *

static inline int task_fifo_put(kfifo_t queue, void *data, int32_t timeout)
{
	return _error_to_rc(k_msgq_put(queue, data, _ticks_to_ms(timeout)));
}

static inline int task_fifo_get(kfifo_t queue, void *data, int32_t timeout)
{
	return _error_to_rc(k_msgq_get(queue, data, _ticks_to_ms(timeout)));
}

static inline int task_fifo_purge(kfifo_t queue)
{
	k_msgq_purge(queue);
	return RC_OK;
}

static inline int task_fifo_size_get(kfifo_t queue)
{
	return queue->used_msgs;
}

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

int task_mbox_put(kmbox_t mbox, kpriority_t prio, struct k_msg *msg,
		  int32_t timeout);
void task_mbox_block_put(kmbox_t mbox, kpriority_t prio, struct k_msg *msg,
			 ksem_t sema);

int task_mbox_get(kmbox_t mbox, struct k_msg *msg, int32_t timeout);
void task_mbox_data_get(struct k_msg *msg);
int task_mbox_data_block_get(struct k_msg *msg, struct k_block *block,
			     kmemory_pool_t pool_id, int32_t timeout);

#define DEFINE_MAILBOX(name) \
	K_MBOX_DEFINE(_k_mbox_obj_##name); \
	struct k_mbox * const name = &_k_mbox_obj_##name

/* pipes */

#define kpipe_t struct k_pipe *

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

static inline int task_pipe_block_put(kpipe_t id, struct k_block block,
				      int size, ksem_t sema)
{
	if (size == 0) {
		return RC_FAIL;
	}

	k_pipe_block_put(id, &block, size, sema);

	return RC_OK;
}

#define DEFINE_PIPE(name, pipe_buffer_size) \
	K_PIPE_DEFINE(_k_pipe_obj_##name, pipe_buffer_size, 4); \
	struct k_pipe * const name = &_k_pipe_obj_##name

#define nano_fifo k_fifo
#define nano_fifo_init k_fifo_init

/* nanokernel fifos */

#define nano_fifo_put k_fifo_put
#define nano_isr_fifo_put k_fifo_put
#define nano_fiber_fifo_put k_fifo_put
#define nano_task_fifo_put k_fifo_put

#define nano_fifo_put_list k_fifo_put_list
#define nano_isr_fifo_put_list k_fifo_put_list
#define nano_fiber_fifo_put_list k_fifo_put_list
#define nano_task_fifo_put_list k_fifo_put_list

#define nano_fifo_put_slist k_fifo_put_slist
#define nano_isr_fifo_put_slist k_fifo_put_slist
#define nano_fiber_fifo_put_slist k_fifo_put_slist
#define nano_task_fifo_put_slist k_fifo_put_slist

static inline void *nano_fifo_get(struct nano_fifo *fifo,
				  int32_t timeout_in_ticks)
{
	return k_fifo_get((struct k_fifo *)fifo,
			  _ticks_to_ms(timeout_in_ticks));
}

#define nano_isr_fifo_get nano_fifo_get
#define nano_fiber_fifo_get nano_fifo_get
#define nano_task_fifo_get nano_fifo_get

/* nanokernel lifos */

#define nano_lifo k_lifo

#define nano_lifo_init k_lifo_init

#define nano_lifo_put k_lifo_put
#define nano_isr_lifo_put k_lifo_put
#define nano_fiber_lifo_put k_lifo_put
#define nano_task_lifo_put k_lifo_put

static inline void *nano_lifo_get(struct nano_lifo *lifo,
				  int32_t timeout_in_ticks)
{
	return k_lifo_get((struct k_lifo *)lifo,
			  _ticks_to_ms(timeout_in_ticks));
}


#define nano_isr_lifo_get nano_lifo_get
#define nano_fiber_lifo_get nano_lifo_get
#define nano_task_lifo_get nano_lifo_get

/* nanokernel stacks */

#define nano_stack k_stack

static inline void nano_stack_init(struct nano_stack *stack, uint32_t *data)
{
	k_stack_init(stack, data, UINT_MAX);
}

#define nano_stack_push k_stack_push
#define nano_isr_stack_push k_stack_push
#define nano_fiber_stack_push k_stack_push
#define nano_task_stack_push k_stack_push

static inline int nano_stack_pop(struct nano_stack *stack, uint32_t *data,
				 int32_t timeout_in_ticks)
{
	return k_stack_pop((struct k_stack *)stack, data,
			   _ticks_to_ms(timeout_in_ticks)) == 0 ? 1 : 0;
}

#define nano_isr_stack_pop nano_stack_pop
#define nano_fiber_stack_pop nano_stack_pop
#define nano_task_stack_pop nano_stack_pop

/* kernel clocks */

extern int32_t _ms_to_ticks(int32_t ms);

extern int64_t sys_tick_get(void);
extern uint32_t sys_tick_get_32(void);
extern int64_t sys_tick_delta(int64_t *reftime);
extern uint32_t sys_tick_delta_32(int64_t *reftime);

#define sys_cycle_get_32 k_cycle_get_32

/* microkernel timers */

#if (CONFIG_NUM_DYNAMIC_TIMERS > 0)

#define CONFIG_NUM_TIMER_PACKETS CONFIG_NUM_DYNAMIC_TIMERS

#define ktimer_t struct k_timer *

extern ktimer_t task_timer_alloc(void);
extern void task_timer_free(ktimer_t timer);

extern void task_timer_start(ktimer_t timer, int32_t duration,
			     int32_t period, ksem_t sema);

static inline void task_timer_restart(ktimer_t timer, int32_t duration,
				      int32_t period)
{
	k_timer_start(timer, _ticks_to_ms(duration), _ticks_to_ms(period));
}

static inline void task_timer_stop(ktimer_t timer)
{
	k_timer_stop(timer);
}

extern bool k_timer_pool_is_empty(void);

#endif /* CONFIG_NUM_DYNAMIC_TIMERS > 0 */

/* nanokernel timers */

#define nano_timer k_timer

static inline void nano_timer_init(struct k_timer *timer, void *data)
{
	k_timer_init(timer, NULL, NULL);
	timer->_legacy_data = data;
}

static inline void nano_timer_start(struct nano_timer *timer, int ticks)
{
	k_timer_start(timer, _ticks_to_ms(ticks), 0);
}

#define nano_isr_timer_start nano_timer_start
#define nano_fiber_timer_start nano_timer_start
#define nano_task_timer_start nano_timer_start

extern void *nano_timer_test(struct nano_timer *timer,
			     int32_t timeout_in_ticks);

#define nano_isr_timer_test nano_timer_test
#define nano_fiber_timer_test nano_timer_test
#define nano_task_timer_test nano_timer_test

#define task_timer_stop k_timer_stop

#define nano_isr_timer_stop k_timer_stop
#define nano_fiber_timer_stop k_timer_stop
#define nano_task_timer_stop k_timer_stop

static inline int32_t nano_timer_ticks_remain(struct nano_timer *timer)
{
	return _ms_to_ticks(k_timer_remaining_get(timer));
}

/* floating point services */

#define fiber_float_enable  k_float_enable
#define task_float_enable   k_float_enable
#define fiber_float_disable k_float_disable
#define task_float_disable  k_float_disable

#endif /* _legacy__h_ */
