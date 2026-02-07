/*
 * Copyright (c) 2020 Lexmark International, Inc.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_user.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>

void __weak sys_trace_thread_abort_enter(struct k_thread *thread) {}
void __weak sys_trace_thread_abort_exit(struct k_thread *thread) {}
void __weak sys_trace_thread_resume_exit(struct k_thread *thread) {}
void __weak sys_trace_thread_sched_abort(struct k_thread *thread) {}
void __weak sys_trace_thread_sched_resume(struct k_thread *thread) {}
void __weak sys_trace_thread_sched_suspend(struct k_thread *thread) {}
void __weak sys_trace_thread_foreach_enter(void) {}
void __weak sys_trace_thread_foreach_exit(void) {}
void __weak sys_trace_thread_foreach_unlocked_enter(void) {}
void __weak sys_trace_thread_foreach_unlocked_exit(void) {}
void __weak sys_trace_thread_user_mode_enter(void) {}
void __weak sys_trace_thread_heap_assign(struct k_thread *thread, struct k_heap *heap) {}
void __weak sys_trace_thread_join_blocking(struct k_thread *thread, k_timeout_t timeout) {}
void __weak sys_trace_thread_join_enter(struct k_thread *thread, k_timeout_t timeout) {}
void __weak sys_trace_thread_join_exit(struct k_thread *thread, k_timeout_t timeout, int ret) {}
void __weak sys_trace_thread_sleep_enter(k_timeout_t timeout) {}
void __weak sys_trace_thread_sleep_exit(k_timeout_t timeout, int ret) {}
void __weak sys_trace_thread_msleep_enter(int32_t ms) {}
void __weak sys_trace_thread_msleep_exit(int32_t ms, int ret) {}
void __weak sys_trace_thread_usleep_enter(int32_t us) {}
void __weak sys_trace_thread_usleep_exit(int32_t us, int ret) {}
void __weak sys_trace_thread_busy_wait_enter(uint32_t usec_to_wait) {}
void __weak sys_trace_thread_busy_wait_exit(uint32_t usec_to_wait) {}
void __weak sys_trace_thread_yield(void) {}
void __weak sys_trace_thread_wakeup(struct k_thread *thread) {}
void __weak sys_trace_thread_start(struct k_thread *thread) {}
void __weak sys_trace_thread_priority_set(struct k_thread *thread) {}
void __weak sys_trace_thread_sched_lock(void) {}
void __weak sys_trace_thread_sched_unlock(void) {}
void __weak sys_trace_thread_name_set(struct k_thread *thread, int ret) {}
void __weak sys_trace_thread_ready(struct k_thread *thread) {}

/* Work tracing weak stubs */
void __weak sys_trace_k_work_init(struct k_work *work) {}
void __weak sys_trace_k_work_submit_to_queue_enter(struct k_work_q *queue,
							    struct k_work *work)
{}
void __weak sys_trace_k_work_submit_to_queue_exit(struct k_work_q *queue,
							   struct k_work *work,
							   int ret)
{}
void __weak sys_trace_k_work_submit_enter(struct k_work *work) {}
void __weak sys_trace_k_work_submit_exit(struct k_work *work, int ret) {}
void __weak sys_trace_k_work_flush_enter(struct k_work *work) {}
void __weak sys_trace_k_work_flush_blocking(struct k_work *work, k_timeout_t timeout) {}
void __weak sys_trace_k_work_flush_exit(struct k_work *work, bool ret) {}
void __weak sys_trace_k_work_cancel_enter(struct k_work *work) {}
void __weak sys_trace_k_work_cancel_exit(struct k_work *work, int ret) {}
void __weak sys_trace_k_work_cancel_sync_enter(struct k_work *work,
							struct k_work_sync *sync)
{}
void __weak sys_trace_k_work_cancel_sync_blocking(struct k_work *work,
							   struct k_work_sync *sync)
{}
void __weak sys_trace_k_work_cancel_sync_exit(struct k_work *work,
						       struct k_work_sync *sync,
						       bool ret)
{}

/* Work queue tracing weak stubs */
void __weak sys_trace_k_work_queue_init(struct k_work_q *queue) {}
void __weak sys_trace_k_work_queue_start_enter(struct k_work_q *queue) {}
void __weak sys_trace_k_work_queue_start_exit(struct k_work_q *queue) {}
void __weak sys_trace_k_work_queue_stop_enter(struct k_work_q *queue, k_timeout_t timeout) {}
void __weak sys_trace_k_work_queue_stop_blocking(struct k_work_q *queue,
							  k_timeout_t timeout)
{}
void __weak sys_trace_k_work_queue_stop_exit(struct k_work_q *queue,
						      k_timeout_t timeout,
						      int ret)
{}
void __weak sys_trace_k_work_queue_drain_enter(struct k_work_q *queue) {}
void __weak sys_trace_k_work_queue_drain_exit(struct k_work_q *queue, int ret) {}
void __weak sys_trace_k_work_queue_unplug_enter(struct k_work_q *queue) {}
void __weak sys_trace_k_work_queue_unplug_exit(struct k_work_q *queue, int ret) {}

/* Delayable work tracing weak stubs */
void __weak sys_trace_k_work_delayable_init(struct k_work_delayable *dwork) {}
void __weak sys_trace_k_work_schedule_for_queue_enter(struct k_work_q *queue,
							       struct k_work_delayable *dwork,
							       k_timeout_t delay)
{}
void __weak sys_trace_k_work_schedule_for_queue_exit(struct k_work_q *queue,
							      struct k_work_delayable *dwork,
							      k_timeout_t delay,
							      int ret)
{}
void __weak sys_trace_k_work_schedule_enter(struct k_work_delayable *dwork,
						     k_timeout_t delay)
{}
void __weak sys_trace_k_work_schedule_exit(struct k_work_delayable *dwork,
						    k_timeout_t delay,
						    int ret)
{}
void __weak sys_trace_k_work_reschedule_for_queue_enter(struct k_work_q *queue,
								struct k_work_delayable *dwork,
								k_timeout_t delay)
{}
void __weak sys_trace_k_work_reschedule_for_queue_exit(struct k_work_q *queue,
							       struct k_work_delayable *dwork,
							       k_timeout_t delay,
							       int ret)
{}
void __weak sys_trace_k_work_reschedule_enter(struct k_work_delayable *dwork,
						       k_timeout_t delay)
{}
void __weak sys_trace_k_work_reschedule_exit(struct k_work_delayable *dwork,
						      k_timeout_t delay,
						      int ret)
{}
void __weak sys_trace_k_work_flush_delayable_enter(struct k_work_delayable *dwork,
							    struct k_work_sync *sync)
{}
void __weak sys_trace_k_work_flush_delayable_exit(struct k_work_delayable *dwork,
							   struct k_work_sync *sync,
							   bool ret)
{}
void __weak sys_trace_k_work_cancel_delayable_enter(
					struct k_work_delayable *dwork)
{}
void __weak sys_trace_k_work_cancel_delayable_exit(
					struct k_work_delayable *dwork, int ret)
{}
void __weak sys_trace_k_work_cancel_delayable_sync_enter(
					struct k_work_delayable *dwork,
					struct k_work_sync *sync)
{}
void __weak sys_trace_k_work_cancel_delayable_sync_exit(
					struct k_work_delayable *dwork,
					struct k_work_sync *sync,
					bool ret)
{}

/* Work poll tracing weak stubs */
void __weak sys_trace_k_work_poll_init_enter(struct k_work_poll *work) {}
void __weak sys_trace_k_work_poll_init_exit(struct k_work_poll *work) {}
void __weak sys_trace_k_work_poll_submit_to_queue_enter(
						struct k_work_q *work_q,
						struct k_work_poll *work,
						k_timeout_t timeout)
{}
void __weak sys_trace_k_work_poll_submit_to_queue_blocking(
							struct k_work_q *work_q,
							struct k_work_poll *work,
							k_timeout_t timeout)
{}
void __weak sys_trace_k_work_poll_submit_to_queue_exit(
							struct k_work_q *work_q,
							struct k_work_poll *work,
							k_timeout_t timeout,
							int ret)
{}
void __weak sys_trace_k_work_poll_submit_enter(struct k_work_poll *work,
							k_timeout_t timeout)
{}
void __weak sys_trace_k_work_poll_submit_exit(struct k_work_poll *work,
						       k_timeout_t timeout,
						       int ret)
{}
void __weak sys_trace_k_work_poll_cancel_enter(struct k_work_poll *work) {}
void __weak sys_trace_k_work_poll_cancel_exit(struct k_work_poll *work, int ret) {}

/* Poll API tracing weak stubs */
void __weak sys_trace_k_poll_api_event_init(struct k_poll_event *event) {}
void __weak sys_trace_k_poll_api_poll_enter(struct k_poll_event *events) {}
void __weak sys_trace_k_poll_api_poll_exit(struct k_poll_event *events, int ret) {}
void __weak sys_trace_k_poll_api_signal_init(struct k_poll_signal *sig) {}
void __weak sys_trace_k_poll_api_signal_reset(struct k_poll_signal *sig) {}
void __weak sys_trace_k_poll_api_signal_check(struct k_poll_signal *sig) {}
void __weak sys_trace_k_poll_api_signal_raise(struct k_poll_signal *sig, int ret) {}

/* Semaphore tracing weak stubs */
void __weak sys_trace_k_sem_init(struct k_sem *sem, int ret) {}
void __weak sys_trace_k_sem_give_enter(struct k_sem *sem) {}
void __weak sys_trace_k_sem_give_exit(struct k_sem *sem) {}
void __weak sys_trace_k_sem_take_enter(struct k_sem *sem, k_timeout_t timeout) {}
void __weak sys_trace_k_sem_take_blocking(struct k_sem *sem, k_timeout_t timeout) {}
void __weak sys_trace_k_sem_take_exit(struct k_sem *sem, k_timeout_t timeout, int ret) {}
void __weak sys_trace_k_sem_reset(struct k_sem *sem) {}

/* Mutex tracing weak stubs */
void __weak sys_trace_k_mutex_init(struct k_mutex *mutex, int ret) {}
void __weak sys_trace_k_mutex_lock_enter(struct k_mutex *mutex, k_timeout_t timeout) {}
void __weak sys_trace_k_mutex_lock_blocking(struct k_mutex *mutex, k_timeout_t timeout) {}
void __weak sys_trace_k_mutex_lock_exit(struct k_mutex *mutex, k_timeout_t timeout, int ret) {}
void __weak sys_trace_k_mutex_unlock_enter(struct k_mutex *mutex) {}
void __weak sys_trace_k_mutex_unlock_exit(struct k_mutex *mutex, int ret) {}

/* Condition variable tracing weak stubs */
void __weak sys_trace_k_condvar_init(struct k_condvar *condvar, int ret) {}
void __weak sys_trace_k_condvar_signal_enter(struct k_condvar *condvar) {}
void __weak sys_trace_k_condvar_signal_blocking(struct k_condvar *condvar,
							 k_timeout_t timeout)
{}
void __weak sys_trace_k_condvar_signal_exit(struct k_condvar *condvar,
						    int ret)
{}
void __weak sys_trace_k_condvar_broadcast_enter(struct k_condvar *condvar) {}
void __weak sys_trace_k_condvar_broadcast_exit(struct k_condvar *condvar,
						       int ret)
{}
void __weak sys_trace_k_condvar_wait_enter(struct k_condvar *condvar,
						   k_timeout_t timeout)
{}
void __weak sys_trace_k_condvar_wait_exit(struct k_condvar *condvar,
						  k_timeout_t timeout, int ret)
{}

/* Queue tracing weak stubs */
void __weak sys_trace_k_queue_init(struct k_queue *queue) {}
void __weak sys_trace_k_queue_cancel_wait(struct k_queue *queue) {}
void __weak sys_trace_k_queue_queue_insert_enter(struct k_queue *queue, bool alloc) {}
void __weak sys_trace_k_queue_queue_insert_blocking(struct k_queue *queue,
							     bool alloc,
							     k_timeout_t timeout)
{}
void __weak sys_trace_k_queue_queue_insert_exit(struct k_queue *queue,
							bool alloc, int ret)
{}
void __weak sys_trace_k_queue_append_enter(struct k_queue *queue) {}
void __weak sys_trace_k_queue_append_exit(struct k_queue *queue) {}
void __weak sys_trace_k_queue_alloc_append_enter(struct k_queue *queue) {}
void __weak sys_trace_k_queue_alloc_append_exit(struct k_queue *queue, int ret) {}
void __weak sys_trace_k_queue_prepend_enter(struct k_queue *queue) {}
void __weak sys_trace_k_queue_prepend_exit(struct k_queue *queue) {}
void __weak sys_trace_k_queue_alloc_prepend_enter(struct k_queue *queue) {}
void __weak sys_trace_k_queue_alloc_prepend_exit(struct k_queue *queue, int ret) {}
void __weak sys_trace_k_queue_insert_enter(struct k_queue *queue) {}
void __weak sys_trace_k_queue_insert_blocking(struct k_queue *queue, k_timeout_t timeout) {}
void __weak sys_trace_k_queue_insert_exit(struct k_queue *queue) {}
void __weak sys_trace_k_queue_append_list_enter(struct k_queue *queue) {}
void __weak sys_trace_k_queue_append_list_exit(struct k_queue *queue, int ret) {}
void __weak sys_trace_k_queue_merge_slist_enter(struct k_queue *queue) {}
void __weak sys_trace_k_queue_merge_slist_exit(struct k_queue *queue, int ret) {}
void __weak sys_trace_k_queue_get_enter(struct k_queue *queue, k_timeout_t timeout) {}
void __weak sys_trace_k_queue_get_blocking(struct k_queue *queue, k_timeout_t timeout) {}
void __weak sys_trace_k_queue_get_exit(struct k_queue *queue, k_timeout_t timeout, void *ret) {}
void __weak sys_trace_k_queue_remove_enter(struct k_queue *queue) {}
void __weak sys_trace_k_queue_remove_exit(struct k_queue *queue, bool ret) {}
void __weak sys_trace_k_queue_unique_append_enter(struct k_queue *queue) {}
void __weak sys_trace_k_queue_unique_append_exit(struct k_queue *queue, bool ret) {}
void __weak sys_trace_k_queue_peek_head(struct k_queue *queue, void *ret) {}
void __weak sys_trace_k_queue_peek_tail(struct k_queue *queue, void *ret) {}

/* FIFO tracing weak stubs */
void __weak sys_trace_k_fifo_init_enter(struct k_fifo *fifo) {}
void __weak sys_trace_k_fifo_init_exit(struct k_fifo *fifo) {}
void __weak sys_trace_k_fifo_cancel_wait_enter(struct k_fifo *fifo) {}
void __weak sys_trace_k_fifo_cancel_wait_exit(struct k_fifo *fifo) {}
void __weak sys_trace_k_fifo_put_enter(struct k_fifo *fifo, void *data) {}
void __weak sys_trace_k_fifo_put_exit(struct k_fifo *fifo, void *data) {}
void __weak sys_trace_k_fifo_alloc_put_enter(struct k_fifo *fifo, void *data) {}
void __weak sys_trace_k_fifo_alloc_put_exit(struct k_fifo *fifo, void *data, int ret) {}
void __weak sys_trace_k_fifo_put_list_enter(struct k_fifo *fifo, void *head, void *tail) {}
void __weak sys_trace_k_fifo_put_list_exit(struct k_fifo *fifo, void *head, void *tail) {}
void __weak sys_trace_k_fifo_put_slist_enter(struct k_fifo *fifo, sys_slist_t *list) {}
void __weak sys_trace_k_fifo_put_slist_exit(struct k_fifo *fifo, sys_slist_t *list) {}
void __weak sys_trace_k_fifo_get_enter(struct k_fifo *fifo, k_timeout_t timeout) {}
void __weak sys_trace_k_fifo_get_exit(struct k_fifo *fifo, k_timeout_t timeout, void *ret) {}
void __weak sys_trace_k_fifo_peek_head_enter(struct k_fifo *fifo) {}
void __weak sys_trace_k_fifo_peek_head_exit(struct k_fifo *fifo, void *ret) {}
void __weak sys_trace_k_fifo_peek_tail_enter(struct k_fifo *fifo) {}
void __weak sys_trace_k_fifo_peek_tail_exit(struct k_fifo *fifo, void *ret) {}

/* Stack tracing weak stubs */
void __weak sys_trace_k_stack_init(struct k_stack *stack) {}
void __weak sys_trace_k_stack_alloc_init_enter(struct k_stack *stack) {}
void __weak sys_trace_k_stack_alloc_init_exit(struct k_stack *stack, int ret) {}
void __weak sys_trace_k_stack_cleanup_enter(struct k_stack *stack) {}
void __weak sys_trace_k_stack_cleanup_exit(struct k_stack *stack, int ret) {}
void __weak sys_trace_k_stack_push_enter(struct k_stack *stack) {}
void __weak sys_trace_k_stack_push_exit(struct k_stack *stack, int ret) {}
void __weak sys_trace_k_stack_pop_enter(struct k_stack *stack, k_timeout_t timeout) {}
void __weak sys_trace_k_stack_pop_blocking(struct k_stack *stack, k_timeout_t timeout) {}
void __weak sys_trace_k_stack_pop_exit(struct k_stack *stack, k_timeout_t timeout, int ret) {}

/* Message queue tracing weak stubs */
void __weak sys_trace_k_msgq_init(struct k_msgq *msgq) {}
void __weak sys_trace_k_msgq_alloc_init_enter(struct k_msgq *msgq) {}
void __weak sys_trace_k_msgq_alloc_init_exit(struct k_msgq *msgq, int ret) {}
void __weak sys_trace_k_msgq_cleanup_enter(struct k_msgq *msgq) {}
void __weak sys_trace_k_msgq_cleanup_exit(struct k_msgq *msgq, int ret) {}
void __weak sys_trace_k_msgq_put_enter(struct k_msgq *msgq, k_timeout_t timeout) {}
void __weak sys_trace_k_msgq_put_blocking(struct k_msgq *msgq, k_timeout_t timeout) {}
void __weak sys_trace_k_msgq_put_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret) {}
void __weak sys_trace_k_msgq_put_front_enter(struct k_msgq *msgq, k_timeout_t timeout) {}
void __weak sys_trace_k_msgq_put_front_blocking(struct k_msgq *msgq, k_timeout_t timeout) {}
void __weak sys_trace_k_msgq_put_front_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret) {}
void __weak sys_trace_k_msgq_get_enter(struct k_msgq *msgq, k_timeout_t timeout) {}
void __weak sys_trace_k_msgq_get_blocking(struct k_msgq *msgq, k_timeout_t timeout) {}
void __weak sys_trace_k_msgq_get_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret) {}
void __weak sys_trace_k_msgq_peek(struct k_msgq *msgq, int ret) {}
void __weak sys_trace_k_msgq_purge(struct k_msgq *msgq) {}

/* Mailbox tracing weak stubs */
void __weak sys_trace_k_mbox_init(struct k_mbox *mbox) {}
void __weak sys_trace_k_mbox_message_put_enter(struct k_mbox *mbox, k_timeout_t timeout) {}
void __weak sys_trace_k_mbox_message_put_blocking(struct k_mbox *mbox, k_timeout_t timeout) {}
void __weak sys_trace_k_mbox_message_put_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret) {}
void __weak sys_trace_k_mbox_put_enter(struct k_mbox *mbox, k_timeout_t timeout) {}
void __weak sys_trace_k_mbox_put_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret) {}
void __weak sys_trace_k_mbox_async_put_enter(struct k_mbox *mbox, struct k_sem *sem) {}
void __weak sys_trace_k_mbox_async_put_exit(struct k_mbox *mbox, struct k_sem *sem) {}
void __weak sys_trace_k_mbox_get_enter(struct k_mbox *mbox, k_timeout_t timeout) {}
void __weak sys_trace_k_mbox_get_blocking(struct k_mbox *mbox, k_timeout_t timeout) {}
void __weak sys_trace_k_mbox_get_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret) {}
void __weak sys_trace_k_mbox_data_get(struct k_mbox_msg *rx_msg) {}

/* Pipe tracing weak stubs */
void __weak sys_trace_k_pipe_init(struct k_pipe *pipe, unsigned char *buffer,
					   size_t size)
{}
void __weak sys_trace_k_pipe_reset_enter(struct k_pipe *pipe) {}
void __weak sys_trace_k_pipe_reset_exit(struct k_pipe *pipe) {}
void __weak sys_trace_k_pipe_close_enter(struct k_pipe *pipe) {}
void __weak sys_trace_k_pipe_close_exit(struct k_pipe *pipe) {}
void __weak sys_trace_k_pipe_write_enter(struct k_pipe *pipe, void *data,
						 size_t len,
						 k_timeout_t timeout)
{}
void __weak sys_trace_k_pipe_write_blocking(struct k_pipe *pipe,
						     k_timeout_t timeout)
{}
void __weak sys_trace_k_pipe_write_exit(struct k_pipe *pipe, int ret) {}
void __weak sys_trace_k_pipe_read_enter(struct k_pipe *pipe, void *data,
						size_t len,
						k_timeout_t timeout)
{}
void __weak sys_trace_k_pipe_read_blocking(struct k_pipe *pipe, k_timeout_t timeout) {}
void __weak sys_trace_k_pipe_read_exit(struct k_pipe *pipe, int ret) {}

/* Heap tracing weak stubs */
void __weak sys_trace_k_heap_init(struct k_heap *heap) {}
void __weak sys_trace_k_heap_aligned_alloc_enter(struct k_heap *heap,
							 k_timeout_t timeout)
{}
void __weak sys_trace_k_heap_alloc_helper_blocking(struct k_heap *heap,
							    k_timeout_t timeout)
{}
void __weak sys_trace_k_heap_aligned_alloc_exit(struct k_heap *heap,
							k_timeout_t timeout,
							void *ret)
{}
void __weak sys_trace_k_heap_alloc_enter(struct k_heap *heap, k_timeout_t timeout) {}
void __weak sys_trace_k_heap_alloc_exit(struct k_heap *heap, k_timeout_t timeout, void *ret) {}
void __weak sys_trace_k_heap_calloc_enter(struct k_heap *heap,
						   k_timeout_t timeout)
{}
void __weak sys_trace_k_heap_calloc_exit(struct k_heap *heap,
						  k_timeout_t timeout,
						  void *ret)
{}
void __weak sys_trace_k_heap_free(struct k_heap *heap) {}
void __weak sys_trace_k_heap_realloc_enter(struct k_heap *h, void *ptr,
						   size_t bytes,
						   k_timeout_t timeout)
{}
void __weak sys_trace_k_heap_realloc_exit(struct k_heap *h, void *ptr,
						  size_t bytes,
						  k_timeout_t timeout,
						  void *ret)
{}
void __weak sys_trace_k_heap_sys_k_aligned_alloc_enter(
							struct k_heap *heap)
{}
void __weak sys_trace_k_heap_sys_k_aligned_alloc_exit(
							struct k_heap *heap,
							void *ret)
{}
void __weak sys_trace_k_heap_sys_k_malloc_enter(struct k_heap *heap) {}
void __weak sys_trace_k_heap_sys_k_malloc_exit(struct k_heap *heap,
						       void *ret)
{}
void __weak sys_trace_k_heap_sys_k_free_enter(struct k_heap *heap,
						       struct k_heap **heap_ref)
{}
void __weak sys_trace_k_heap_sys_k_free_exit(struct k_heap *heap,
						      struct k_heap **heap_ref)
{}
void __weak sys_trace_k_heap_sys_k_calloc_enter(struct k_heap *heap) {}
void __weak sys_trace_k_heap_sys_k_calloc_exit(struct k_heap *heap,
						       void *ret)
{}
void __weak sys_trace_k_heap_sys_k_realloc_enter(struct k_heap *heap,
							 void *ptr)
{}
void __weak sys_trace_k_heap_sys_k_realloc_exit(struct k_heap *heap,
							void *ptr, void *ret)
{}

/* Memory slab tracing weak stubs */
void __weak sys_trace_k_mem_slab_init(struct k_mem_slab *slab, int rc) {}
void __weak sys_trace_k_mem_slab_alloc_enter(struct k_mem_slab *slab,
						     k_timeout_t timeout)
{}
void __weak sys_trace_k_mem_slab_alloc_blocking(struct k_mem_slab *slab,
							k_timeout_t timeout)
{}
void __weak sys_trace_k_mem_slab_alloc_exit(struct k_mem_slab *slab,
						    k_timeout_t timeout,
						    int ret)
{}
void __weak sys_trace_k_mem_slab_free_enter(struct k_mem_slab *slab) {}
void __weak sys_trace_k_mem_slab_free_exit(struct k_mem_slab *slab) {}

/* Event tracing weak stubs */
void __weak sys_trace_k_event_init(struct k_event *event) {}
void __weak sys_trace_k_event_post_enter(struct k_event *event,
						 uint32_t events,
						 uint32_t events_mask)
{}
void __weak sys_trace_k_event_post_exit(struct k_event *event,
						uint32_t events,
						uint32_t events_mask)
{}
void __weak sys_trace_k_event_wait_enter(struct k_event *event,
						 uint32_t events,
						 uint32_t options,
						 k_timeout_t timeout)
{}
void __weak sys_trace_k_event_wait_blocking(struct k_event *event,
						    uint32_t events,
						    uint32_t options,
						    k_timeout_t timeout)
{}
void __weak sys_trace_k_event_wait_exit(struct k_event *event, uint32_t events, uint32_t ret) {}

/* PM tracing weak stubs */
void __weak sys_trace_pm_system_suspend_enter(int32_t ticks) {}
void __weak sys_trace_pm_system_suspend_exit(int32_t ticks, uint32_t state) {}
void __weak sys_trace_pm_device_runtime_get_enter(const struct device *dev) {}
void __weak sys_trace_pm_device_runtime_get_exit(const struct device *dev, int ret) {}
void __weak sys_trace_pm_device_runtime_put_enter(
							const struct device *dev)
{}
void __weak sys_trace_pm_device_runtime_put_exit(const struct device *dev,
							 int ret)
{}
void __weak sys_trace_pm_device_runtime_put_async_enter(
							const struct device *dev,
							k_timeout_t delay)
{}
void __weak sys_trace_pm_device_runtime_put_async_exit(
							const struct device *dev,
							k_timeout_t delay,
							int ret)
{}
void __weak sys_trace_pm_device_runtime_enable_enter(const struct device *dev) {}
void __weak sys_trace_pm_device_runtime_enable_exit(const struct device *dev, int ret) {}
void __weak sys_trace_pm_device_runtime_disable_enter(const struct device *dev) {}
void __weak sys_trace_pm_device_runtime_disable_exit(const struct device *dev, int ret) {}

/* Socket tracing weak stubs */
void __weak sys_trace_socket_init(int sock, int family, int type,
					   int proto)
{}
void __weak sys_trace_socket_close_enter(int sock) {}
void __weak sys_trace_socket_close_exit(int sock, int ret) {}
void __weak sys_trace_socket_shutdown_enter(int sock, int how) {}
void __weak sys_trace_socket_shutdown_exit(int sock, int ret) {}
void __weak sys_trace_socket_bind_enter(int sock,
						const struct sockaddr *addr,
						socklen_t addrlen)
{}
void __weak sys_trace_socket_bind_exit(int sock, int ret) {}
void __weak sys_trace_socket_connect_enter(int sock,
						   const struct sockaddr *addr,
						   socklen_t addrlen)
{}
void __weak sys_trace_socket_connect_exit(int sock, int ret) {}
void __weak sys_trace_socket_listen_enter(int sock, int backlog) {}
void __weak sys_trace_socket_listen_exit(int sock, int ret) {}
void __weak sys_trace_socket_accept_enter(int sock) {}
void __weak sys_trace_socket_accept_exit(int sock,
						 const struct sockaddr *addr,
						 const socklen_t *addrlen,
						 int ret)
{}
void __weak sys_trace_socket_sendto_enter(int sock, size_t len, int flags,
						  const struct sockaddr *dest_addr,
						  socklen_t addrlen)
{}
void __weak sys_trace_socket_sendto_exit(int sock, int ret) {}
void __weak sys_trace_socket_sendmsg_enter(int sock,
						   const struct msghdr *msg,
						   int flags)
{}
void __weak sys_trace_socket_sendmsg_exit(int sock, int ret) {}
void __weak sys_trace_socket_recvfrom_enter(int sock, size_t max_len,
						    int flags,
						    struct sockaddr *addr,
						    socklen_t *addrlen)
{}
void __weak sys_trace_socket_recvfrom_exit(int sock,
						   const struct sockaddr *src_addr,
						   const socklen_t *addrlen,
						   int ret)
{}
void __weak sys_trace_socket_recvmsg_enter(int sock,
						    const struct msghdr *msg,
						    int flags)
{}
void __weak sys_trace_socket_recvmsg_exit(int sock,
						  const struct msghdr *msg,
						  int ret)
{}
void __weak sys_trace_socket_fcntl_enter(int sock, int cmd, int flags) {}
void __weak sys_trace_socket_fcntl_exit(int sock, int ret) {}
void __weak sys_trace_socket_ioctl_enter(int sock, int req) {}
void __weak sys_trace_socket_ioctl_exit(int sock, int ret) {}
void __weak sys_trace_socket_poll_enter(struct zsock_pollfd *fds, int nfds,
						int timeout)
{}
void __weak sys_trace_socket_poll_exit(struct zsock_pollfd *fds, int nfds,
					       int ret)
{}
void __weak sys_trace_socket_getsockopt_enter(int sock, int level,
						       int optname)
{}
void __weak sys_trace_socket_getsockopt_exit(int sock, int level,
						      int optname, void *optval,
						      const socklen_t *optlen,
						      int ret)
{}
void __weak sys_trace_socket_setsockopt_enter(int sock, int level,
						       int optname,
						       const void *optval,
						       socklen_t optlen)
{}
void __weak sys_trace_socket_setsockopt_exit(int sock, int ret) {}
void __weak sys_trace_socket_getpeername_enter(int sock) {}
void __weak sys_trace_socket_getpeername_exit(int sock,
						       struct sockaddr *addr,
						       const socklen_t *addrlen,
						       int ret)
{}
void __weak sys_trace_socket_getsockname_enter(int sock) {}
void __weak sys_trace_socket_getsockname_exit(int sock,
						       struct sockaddr *addr,
						       const socklen_t *addrlen,
						       int ret)
{}
void __weak sys_trace_socket_socketpair_enter(int family, int type, int proto, int *sv) {}
void __weak sys_trace_socket_socketpair_exit(int sockA, int sockB, int ret) {}

/* Network tracing weak stubs */
void __weak sys_trace_net_recv_data_enter(struct net_if *iface, struct net_pkt *pkt) {}
void __weak sys_trace_net_recv_data_exit(struct net_if *iface, struct net_pkt *pkt, int ret) {}
void __weak sys_trace_net_send_data_enter(struct net_pkt *pkt) {}
void __weak sys_trace_net_send_data_exit(struct net_pkt *pkt, int ret) {}
void __weak sys_trace_net_rx_time(struct net_pkt *pkt, uint32_t end_time) {}
void __weak sys_trace_net_tx_time(struct net_pkt *pkt, uint32_t end_time) {}

void __weak sys_trace_thread_create(struct k_thread *thread) {}

void __weak sys_trace_thread_abort(struct k_thread *thread) {}

void __weak sys_trace_thread_suspend(struct k_thread *thread) {}

void __weak sys_trace_thread_resume(struct k_thread *thread) {}

void __weak sys_trace_thread_name_set(struct k_thread *thread) {}

void __weak sys_trace_thread_switched_in(void) {}

void __weak sys_trace_thread_switched_out(void) {}

void __weak sys_trace_thread_info(struct k_thread *thread) {}

void __weak sys_trace_thread_sched_priority_set(struct k_thread *thread, int prio) {}

void __weak sys_trace_thread_sched_ready(struct k_thread *thread) {}

void __weak sys_trace_thread_pend(struct k_thread *thread) {}

void __weak sys_trace_isr_enter(void) {}

void __weak sys_trace_isr_exit(void) {}

void __weak sys_trace_idle(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_on_enter_idle();
	}
}

void __weak sys_trace_idle_exit(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_on_exit_idle();
	}
}

void __weak sys_trace_sys_init_enter(const struct init_entry *entry, int level) {}

void __weak sys_trace_sys_init_exit(const struct init_entry *entry, int level, int result) {}

void __weak sys_trace_gpio_pin_interrupt_configure_enter(
						const struct device *port,
						gpio_pin_t pin,
						gpio_flags_t flags)
{}

void __weak sys_trace_gpio_pin_interrupt_configure_exit(
						const struct device *port,
						gpio_pin_t pin,
						int ret)
{}

void __weak sys_trace_gpio_pin_configure_enter(const struct device *port,
							gpio_pin_t pin,
							gpio_flags_t flags)
{}

void __weak sys_trace_gpio_pin_configure_exit(const struct device *port,
						       gpio_pin_t pin,
						       int ret)
{}

void __weak sys_trace_gpio_port_get_direction_enter(
					const struct device *port,
					gpio_port_pins_t map,
					gpio_port_pins_t inputs,
					gpio_port_pins_t outputs)
{}

void __weak sys_trace_gpio_port_get_direction_exit(
						const struct device *port, int ret)
{}

void __weak sys_trace_gpio_pin_get_config_enter(
						const struct device *port,
						gpio_pin_t pin, int ret)
{}

void __weak sys_trace_gpio_pin_get_config_exit(const struct device *port,
						       gpio_pin_t pin,
						       int ret)
{}

void __weak sys_trace_gpio_port_get_raw_enter(
					const struct device *port,
					gpio_port_value_t *value)
{}

void __weak sys_trace_gpio_port_get_raw_exit(const struct device *port,
						      int ret)
{}

void __weak sys_trace_gpio_port_set_masked_raw_enter(
						const struct device *port,
						gpio_port_pins_t mask,
						gpio_port_value_t value)
{}

void __weak sys_trace_gpio_port_set_masked_raw_exit(
						const struct device *port,
						int ret)
{}

void __weak sys_trace_gpio_port_set_bits_raw_enter(
						const struct device *port,
						gpio_port_pins_t pins)
{}

void __weak sys_trace_gpio_port_set_bits_raw_exit(
						const struct device *port,
						int ret)
{}

void __weak sys_trace_gpio_port_clear_bits_raw_enter(
						const struct device *port,
						gpio_port_pins_t pins)
{}

void __weak sys_trace_gpio_port_clear_bits_raw_exit(
						const struct device *port,
						int ret)
{}

void __weak sys_trace_gpio_port_toggle_bits_enter(
						const struct device *port,
						gpio_port_pins_t pins)
{}

void __weak sys_trace_gpio_port_toggle_bits_exit(
						const struct device *port,
						int ret)
{}

void __weak sys_trace_gpio_init_callback_enter(
						struct gpio_callback *callback,
						gpio_callback_handler_t handler,
						gpio_port_pins_t pin_mask)
{}

void __weak sys_trace_gpio_init_callback_exit(
						struct gpio_callback *callback)
{}

void __weak sys_trace_gpio_add_callback_enter(
						const struct device *port,
						struct gpio_callback *callback)
{}

void __weak sys_trace_gpio_add_callback_exit(const struct device *port,
						      int ret)
{}

void __weak sys_trace_gpio_remove_callback_enter(
						const struct device *port,
						struct gpio_callback *callback)
{}

void __weak sys_trace_gpio_remove_callback_exit(
						const struct device *port,
						int ret)
{}

void __weak sys_trace_gpio_get_pending_int_enter(
						const struct device *dev)
{}

void __weak sys_trace_gpio_get_pending_int_exit(
						const struct device *port,
						int ret)
{}

void __weak sys_trace_gpio_fire_callbacks_enter(
						sys_slist_t *list,
						const struct device *port,
						gpio_pin_t pins)
{}

void __weak sys_trace_gpio_fire_callback(const struct device *port,
						 struct gpio_callback *callback)
{}

void __weak sys_trace_rtio_submit_enter(const struct rtio *r, uint32_t wait_count)
{
	printk("rtio_submit_enter_user: %p, wait_count: %d\n", r, wait_count);
}

void __weak sys_trace_rtio_submit_exit(const struct rtio *r)
{
	printk("rtio_submit_exit: rtio: %p\n", r);
}

void __weak sys_trace_rtio_sqe_acquire_enter(const struct rtio *r)
{
	printk("sqe_acquire_enter: rtio: %p\n", r);
}

void __weak sys_trace_rtio_sqe_acquire_exit(const struct rtio *r, const struct rtio_sqe *sqe)
{
	printk("sqe_acquire_exit: rtio: %p\t sqe: %p\n", r, sqe);
}

void __weak sys_trace_rtio_sqe_cancel(const struct rtio_sqe *sqe)
{
	printk("sqe_cancel_user: sqe: %p", sqe);
}

void __weak sys_trace_rtio_cqe_submit_enter(const struct rtio *r, int result, uint32_t flags)
{
	printk("cqe_submit_enter_user: rtio: %p\t result: %d\t flags: %d\n", r, result, flags);
}

void __weak sys_trace_rtio_cqe_submit_exit(const struct rtio *r)
{
	printk("cqe_submit_exit: rtio: %p\n", r);
}

void __weak sys_trace_rtio_cqe_acquire_enter(const struct rtio *r)
{
	printk("cqe_acquire_enter_user: rtio: %p\n", r);
}

void __weak sys_trace_rtio_cqe_acquire_exit(const struct rtio *r, const struct rtio_cqe *cqe)
{
	printk("cqe_acquire_exit_user: rtio: %p\t cqe: %p\n", r, cqe);
}

void __weak sys_trace_rtio_cqe_release(const struct rtio *r, const struct rtio_cqe *cqe)
{
	printk("cqe_release: rtio: %p\t cqe: %p\n", r, cqe);
}

void __weak sys_trace_rtio_cqe_consume_enter(const struct rtio *r)
{
	printk("cqe_consume_enter: rtio: %p\n", r);
}

void __weak sys_trace_rtio_cqe_consume_exit(const struct rtio *r, const struct rtio_cqe *cqe)
{
	printk("cqe_consume_exit: rtio: %p\t cqe: %p\n", r, cqe);
}

void __weak sys_trace_rtio_txn_next_enter(
					const struct rtio *r,
					const struct rtio_iodev_sqe *iodev_sqe)
{
	printk("txn_next_enter: rtio: %p\t iodev_sqe: %p\n", r, iodev_sqe);
}

void __weak sys_trace_rtio_txn_next_exit(
					const struct rtio *r,
					const struct rtio_iodev_sqe *iodev_sqe)
{
	printk("txn_next_exit: rtio: %p\t iodev_sqe: %p\n", r, iodev_sqe);
}

void __weak sys_trace_rtio_chain_next_enter(
					const struct rtio *r,
					const struct rtio_iodev_sqe *iodev_sqe)
{
	printk("chain_next_enter: rtio: %p\t iodev_sqe: %p\n", r,
	       iodev_sqe);
}

void __weak sys_trace_rtio_chain_next_exit(
					const struct rtio *r,
					const struct rtio_iodev_sqe *iodev_sqe)
{
	printk("chain_next_exit: rtio: %p\t iodev_sqe: %p\n", r,
	       iodev_sqe);
}

void __weak sys_trace_timer_init(struct k_timer *timer) {}

void __weak sys_trace_timer_start(struct k_timer *timer,
					   k_timeout_t duration,
					   k_timeout_t period)
{}

void __weak sys_trace_timer_stop(struct k_timer *timer) {}

void __weak sys_trace_timer_status_sync_enter(struct k_timer *timer) {}

void __weak sys_trace_timer_status_sync_blocking(struct k_timer *timer, k_timeout_t timeout) {}

void __weak sys_trace_timer_status_sync_exit(struct k_timer *timer, uint32_t result) {}

void __weak sys_trace_timer_expiry_enter(struct k_timer *timer) {}

void __weak sys_trace_timer_expiry_exit(struct k_timer *timer) {}

void __weak sys_trace_timer_stop_fn_expiry_enter(struct k_timer *timer) {}

void __weak sys_trace_timer_stop_fn_expiry_exit(struct k_timer *timer) {}
