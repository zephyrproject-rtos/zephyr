/*
 * Copyright (c) 2021 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_TRACE_TEST_H
#define ZEPHYR_TRACE_TEST_H
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#define sys_port_trace_k_thread_foreach_enter() sys_trace_k_thread_foreach_enter(user_cb, user_data)
#define sys_port_trace_k_thread_foreach_exit() sys_trace_k_thread_foreach_exit(user_cb, user_data)
#define sys_port_trace_k_thread_foreach_unlocked_enter()                                           \
	sys_trace_k_thread_foreach_unlocked_enter(user_cb, user_data)
#define sys_port_trace_k_thread_foreach_unlocked_exit()                                            \
	sys_trace_k_thread_foreach_unlocked_exit(user_cb, user_data)
#define sys_port_trace_k_thread_create(new_thread)                                                 \
	sys_trace_k_thread_create(new_thread, stack_size, prio)
#define sys_port_trace_k_thread_user_mode_enter()                                                  \
	sys_trace_k_thread_user_mode_enter(entry, p1, p2, p3)
#define sys_port_trace_k_thread_heap_assign(thread, heap)                                          \
	sys_trace_k_thread_heap_assign(thread, heap)
#define sys_port_trace_k_thread_join_enter(thread, timeout)                                        \
	sys_trace_k_thread_join_blocking(thread, timeout)
#define sys_port_trace_k_thread_join_blocking(thread, timeout)                                     \
	sys_trace_k_thread_join_blocking(thread, timeout)
#define sys_port_trace_k_thread_join_exit(thread, timeout, ret)                                    \
	sys_trace_k_thread_join_exit(thread, timeout, ret)
#define sys_port_trace_k_thread_sleep_enter(timeout) sys_trace_k_thread_sleep_enter(timeout)
#define sys_port_trace_k_thread_sleep_exit(timeout, ret) sys_trace_k_thread_sleep_exit(timeout, ret)
#define sys_port_trace_k_thread_msleep_enter(ms) sys_trace_k_thread_msleep_enter(ms)
#define sys_port_trace_k_thread_msleep_exit(ms, ret) sys_trace_k_thread_msleep_exit(ms, ret)
#define sys_port_trace_k_thread_usleep_enter(us) sys_trace_k_thread_usleep_enter(us)
#define sys_port_trace_k_thread_usleep_exit(us, ret) sys_trace_k_thread_usleep_exit(us, ret)
#define sys_port_trace_k_thread_busy_wait_enter(usec_to_wait)
#define sys_port_trace_k_thread_busy_wait_exit(usec_to_wait)
#define sys_port_trace_k_thread_yield() sys_trace_k_thread_yield()
#define sys_port_trace_k_thread_wakeup(thread) sys_trace_k_thread_wakeup(thread)
#define sys_port_trace_k_thread_start(thread) sys_trace_k_thread_start(thread)
#define sys_port_trace_k_thread_abort(thread) sys_trace_k_thread_abort(thread)
#define sys_port_trace_k_thread_priority_set(thread) sys_trace_k_thread_priority_set(thread)
#define sys_port_trace_k_thread_suspend_enter(thread) sys_trace_k_thread_suspend(thread)
#define sys_port_trace_k_thread_suspend_exit(thread)
#define sys_port_trace_k_thread_resume_enter(thread) sys_trace_k_thread_resume(thread)

#define sys_port_trace_k_thread_sched_lock(...) sys_trace_k_thread_sched_lock()

#define sys_port_trace_k_thread_sched_unlock(...) sys_trace_k_thread_sched_unlock()

#define sys_port_trace_k_thread_name_set(thread, ret) sys_trace_k_thread_name_set(thread, ret)

#define sys_port_trace_k_thread_switched_out() sys_trace_k_thread_switched_out()

#define sys_port_trace_k_thread_switched_in() sys_trace_k_thread_switched_in()

#define sys_port_trace_k_thread_info(thread) sys_trace_k_thread_info(thread)

#define sys_port_trace_k_thread_sched_wakeup(thread) sys_trace_k_thread_sched_wakeup(thread)
#define sys_port_trace_k_thread_sched_abort(thread) sys_trace_k_thread_sched_abort(thread)
#define sys_port_trace_k_thread_sched_priority_set(thread, prio)                                   \
	sys_trace_k_thread_sched_set_priority(thread, prio)
#define sys_port_trace_k_thread_sched_ready(thread) sys_trace_k_thread_sched_ready(thread)
#define sys_port_trace_k_thread_sched_pend(thread) sys_trace_k_thread_sched_pend(thread)
#define sys_port_trace_k_thread_sched_resume(thread) sys_trace_k_thread_sched_resume(thread)
#define sys_port_trace_k_thread_sched_suspend(thread) sys_trace_k_thread_sched_suspend(thread)

#define sys_port_trace_k_work_init(work)
#define sys_port_trace_k_work_submit_to_queue_enter(queue, work)
#define sys_port_trace_k_work_submit_to_queue_exit(queue, work, ret)
#define sys_port_trace_k_work_submit_enter(work)
#define sys_port_trace_k_work_submit_exit(work, ret)
#define sys_port_trace_k_work_flush_enter(work)
#define sys_port_trace_k_work_flush_blocking(work, timeout)
#define sys_port_trace_k_work_flush_exit(work, ret)
#define sys_port_trace_k_work_cancel_enter(work)
#define sys_port_trace_k_work_cancel_exit(work, ret)
#define sys_port_trace_k_work_cancel_sync_enter(work, sync)
#define sys_port_trace_k_work_cancel_sync_blocking(work, sync)
#define sys_port_trace_k_work_cancel_sync_exit(work, sync, ret)

#define sys_port_trace_k_work_queue_init(queue)
#define sys_port_trace_k_work_queue_start_enter(queue)
#define sys_port_trace_k_work_queue_start_exit(queue)
#define sys_port_trace_k_work_queue_stop_enter(queue, timeout)
#define sys_port_trace_k_work_queue_stop_blocking(queue, timeout)
#define sys_port_trace_k_work_queue_stop_exit(queue, timeout, ret)
#define sys_port_trace_k_work_queue_drain_enter(queue)
#define sys_port_trace_k_work_queue_drain_exit(queue, ret)
#define sys_port_trace_k_work_queue_unplug_enter(queue)
#define sys_port_trace_k_work_queue_unplug_exit(queue, ret)

#define sys_port_trace_k_work_delayable_init(dwork)
#define sys_port_trace_k_work_schedule_for_queue_enter(queue, dwork, delay)
#define sys_port_trace_k_work_schedule_for_queue_exit(queue, dwork, delay, ret)
#define sys_port_trace_k_work_schedule_enter(dwork, delay)
#define sys_port_trace_k_work_schedule_exit(dwork, delay, ret)
#define sys_port_trace_k_work_reschedule_for_queue_enter(queue, dwork, delay)
#define sys_port_trace_k_work_reschedule_for_queue_exit(queue, dwork, delay, ret)
#define sys_port_trace_k_work_reschedule_enter(dwork, delay)
#define sys_port_trace_k_work_reschedule_exit(dwork, delay, ret)
#define sys_port_trace_k_work_flush_delayable_enter(dwork, sync)
#define sys_port_trace_k_work_flush_delayable_exit(dwork, sync, ret)
#define sys_port_trace_k_work_cancel_delayable_enter(dwork)
#define sys_port_trace_k_work_cancel_delayable_exit(dwork, ret)
#define sys_port_trace_k_work_cancel_delayable_sync_enter(dwork, sync)
#define sys_port_trace_k_work_cancel_delayable_sync_exit(dwork, sync, ret)

#define sys_port_trace_k_work_poll_init_enter(work)
#define sys_port_trace_k_work_poll_init_exit(work)
#define sys_port_trace_k_work_poll_submit_to_queue_enter(work_q, work, timeout)
#define sys_port_trace_k_work_poll_submit_to_queue_blocking(work_q, work, timeout)
#define sys_port_trace_k_work_poll_submit_to_queue_exit(work_q, work, timeout, ret)
#define sys_port_trace_k_work_poll_submit_enter(work, timeout)
#define sys_port_trace_k_work_poll_submit_exit(work, timeout, ret)
#define sys_port_trace_k_work_poll_cancel_enter(work)
#define sys_port_trace_k_work_poll_cancel_exit(work, ret)

#define sys_port_trace_k_poll_api_event_init(event)
#define sys_port_trace_k_poll_api_poll_enter(events)
#define sys_port_trace_k_poll_api_poll_exit(events, ret)
#define sys_port_trace_k_poll_api_signal_init(signal)
#define sys_port_trace_k_poll_api_signal_reset(signal)
#define sys_port_trace_k_poll_api_signal_check(signal)
#define sys_port_trace_k_poll_api_signal_raise(signal, ret)

#define sys_port_trace_k_sem_init(sem, ret) sys_trace_k_sem_init(sem, ret)
#define sys_port_trace_k_sem_give_enter(sem) sys_trace_k_sem_give_enter(sem)
#define sys_port_trace_k_sem_give_exit(sem)
#define sys_port_trace_k_sem_take_enter(sem, timeout) sys_trace_k_sem_take_enter(sem, timeout)
#define sys_port_trace_k_sem_take_blocking(sem, timeout) sys_trace_k_sem_take_blocking(sem, timeout)
#define sys_port_trace_k_sem_take_exit(sem, timeout, ret)                                          \
	sys_trace_k_sem_take_exit(sem, timeout, ret)
#define sys_port_trace_k_sem_reset(sem) sys_trace_k_sem_reset(sem)

#define sys_port_trace_k_mutex_init(mutex, ret) sys_trace_k_mutex_init(mutex, ret)
#define sys_port_trace_k_mutex_lock_enter(mutex, timeout)                                          \
	sys_trace_k_mutex_lock_enter(mutex, timeout)
#define sys_port_trace_k_mutex_lock_blocking(mutex, timeout)                                       \
	sys_trace_k_mutex_lock_blocking(mutex, timeout)
#define sys_port_trace_k_mutex_lock_exit(mutex, timeout, ret)                                      \
	sys_trace_k_mutex_lock_exit(mutex, timeout, ret)
#define sys_port_trace_k_mutex_unlock_enter(mutex) sys_trace_k_mutex_unlock_enter(mutex)
#define sys_port_trace_k_mutex_unlock_exit(mutex, ret) sys_trace_k_mutex_unlock_exit(mutex, ret)

#define sys_port_trace_k_condvar_init(condvar, ret) sys_trace_k_condvar_init(condvar, ret)
#define sys_port_trace_k_condvar_signal_enter(condvar) sys_trace_k_condvar_signal_enter(condvar)
#define sys_port_trace_k_condvar_signal_blocking(condvar, timeout)                                 \
	sys_trace_k_condvar_signal_blocking(condvar)
#define sys_port_trace_k_condvar_signal_exit(condvar, ret)                                         \
	sys_trace_k_condvar_signal_exit(condvar, ret)
#define sys_port_trace_k_condvar_broadcast_enter(condvar)                                          \
	sys_trace_k_condvar_broadcast_enter(condvar)
#define sys_port_trace_k_condvar_broadcast_exit(condvar, ret)                                      \
	sys_trace_k_condvar_broadcast_exit(condvar, ret)
#define sys_port_trace_k_condvar_wait_enter(condvar)                                               \
	sys_trace_k_condvar_wait_enter(condvar, mutex, timeout)
#define sys_port_trace_k_condvar_wait_exit(condvar, ret)                                           \
	sys_trace_k_condvar_wait_exit(condvar, mutex, timeout, ret)

#define sys_port_trace_k_queue_init(queue) sys_trace_k_queue_init(queue)
#define sys_port_trace_k_queue_cancel_wait(queue) sys_trace_k_queue_cancel_wait(queue)
#define sys_port_trace_k_queue_queue_insert_enter(queue, alloc)                                    \
	sys_trace_k_queue_queue_insert_enter(queue, alloc, data);
#define sys_port_trace_k_queue_queue_insert_blocking(queue, alloc, timeout)                        \
	sys_trace_k_queue_queue_insert_enter(queue, alloc, data);
#define sys_port_trace_k_queue_queue_insert_exit(queue, alloc, ret)                                \
	sys_trace_k_queue_queue_insert_exit(queue, alloc, data, ret);
#define sys_port_trace_k_queue_append_enter(queue) sys_trace_k_queue_append_enter(queue, data)
#define sys_port_trace_k_queue_append_exit(queue) sys_trace_k_queue_append_exit(queue, data)
#define sys_port_trace_k_queue_alloc_append_enter(queue)                                           \
	sys_trace_k_queue_alloc_append_enter(queue, data)
#define sys_port_trace_k_queue_alloc_append_exit(queue, ret)                                       \
	sys_trace_k_queue_alloc_append_exit(queue, data, ret)
#define sys_port_trace_k_queue_prepend_enter(queue) sys_trace_k_queue_prepend_enter(queue, data)
#define sys_port_trace_k_queue_prepend_exit(queue) sys_trace_k_queue_prepend_exit(queue, data)
#define sys_port_trace_k_queue_alloc_prepend_enter(queue)                                          \
	sys_trace_k_queue_alloc_prepend_enter(queue, data)
#define sys_port_trace_k_queue_alloc_prepend_exit(queue, ret)                                      \
	sys_trace_k_queue_alloc_prepend_exit(queue, data, ret)
#define sys_port_trace_k_queue_insert_enter(queue) sys_trace_k_queue_insert_enter(queue, prev, data)
#define sys_port_trace_k_queue_insert_blocking(queue, timeout)                                     \
	sys_trace_k_queue_insert_blocking(queue, prev, data)
#define sys_port_trace_k_queue_insert_exit(queue) sys_trace_k_queue_insert_exit(queue, prev, data)
#define sys_port_trace_k_queue_append_list_enter(queue)
#define sys_port_trace_k_queue_append_list_exit(queue, ret)                                        \
	sys_trace_k_queue_append_list_exit(queue, head, tail, ret)
#define sys_port_trace_k_queue_merge_slist_enter(queue)                                            \
	sys_trace_k_queue_merge_slist_enter(queue, list)
#define sys_port_trace_k_queue_merge_slist_exit(queue, ret)                                        \
	sys_trace_k_queue_merge_slist_exit(queue, list, ret)
#define sys_port_trace_k_queue_get_enter(queue, timeout)
#define sys_port_trace_k_queue_get_blocking(queue, timeout)                                        \
	sys_trace_k_queue_get_blocking(queue, timeout)
#define sys_port_trace_k_queue_get_exit(queue, timeout, ret)                                       \
	sys_trace_k_queue_get_exit(queue, timeout, ret)
#define sys_port_trace_k_queue_remove_enter(queue) sys_trace_k_queue_remove_enter(queue, data)
#define sys_port_trace_k_queue_remove_exit(queue, ret)                                             \
	sys_trace_k_queue_remove_exit(queue, data, ret)
#define sys_port_trace_k_queue_unique_append_enter(queue)                                          \
	sys_trace_k_queue_unique_append_enter(queue, data)
#define sys_port_trace_k_queue_unique_append_exit(queue, ret)                                      \
	sys_trace_k_queue_unique_append_exit(queue, data, ret)
#define sys_port_trace_k_queue_peek_head(queue, ret) sys_trace_k_queue_peek_head(queue, ret)
#define sys_port_trace_k_queue_peek_tail(queue, ret) sys_trace_k_queue_peek_tail(queue, ret)

/* FIFO */

#define sys_port_trace_k_fifo_init_enter(fifo) sys_trace_k_fifo_init_enter(fifo)

#define sys_port_trace_k_fifo_init_exit(fifo) sys_trace_k_fifo_init_exit(fifo)

#define sys_port_trace_k_fifo_cancel_wait_enter(fifo) sys_trace_k_fifo_cancel_wait_enter(fifo)

#define sys_port_trace_k_fifo_cancel_wait_exit(fifo) sys_trace_k_fifo_cancel_wait_exit(fifo)

#define sys_port_trace_k_fifo_put_enter(fifo, data) sys_trace_k_fifo_put_enter(fifo, data)

#define sys_port_trace_k_fifo_put_exit(fifo, data) sys_trace_k_fifo_put_exit(fifo, data)

#define sys_port_trace_k_fifo_alloc_put_enter(fifo, data)                                          \
	sys_trace_k_fifo_alloc_put_enter(fifo, data)

#define sys_port_trace_k_fifo_alloc_put_exit(fifo, data, ret)                                      \
	sys_trace_k_fifo_alloc_put_exit(fifo, data, ret)

#define sys_port_trace_k_fifo_put_list_enter(fifo, head, tail)                                     \
	sys_trace_k_fifo_put_list_enter(fifo, head, tail)

#define sys_port_trace_k_fifo_put_list_exit(fifo, head, tail)                                      \
	sys_trace_k_fifo_put_list_exit(fifo, head, tail)

#define sys_port_trace_k_fifo_put_slist_enter(fifo, list)                                          \
	sys_trace_k_fifo_put_slist_enter(fifo, list)

#define sys_port_trace_k_fifo_put_slist_exit(fifo, list) sys_trace_k_fifo_put_slist_exit(fifo, list)

#define sys_port_trace_k_fifo_get_enter(fifo, timeout) sys_trace_k_fifo_get_enter(fifo, timeout)

#define sys_port_trace_k_fifo_get_exit(fifo, timeout, ret)                                         \
	sys_trace_k_fifo_get_exit(fifo, timeout, ret)

#define sys_port_trace_k_fifo_peek_head_enter(fifo) sys_trace_k_fifo_peek_head_enter(fifo)

#define sys_port_trace_k_fifo_peek_head_exit(fifo, ret) sys_trace_k_fifo_peek_head_exit(fifo, ret)

#define sys_port_trace_k_fifo_peek_tail_enter(fifo) sys_trace_k_fifo_peek_tail_enter(fifo)

#define sys_port_trace_k_fifo_peek_tail_exit(fifo, ret) sys_trace_k_fifo_peek_tail_exit(fifo, ret)

/* LIFO */
#define sys_port_trace_k_lifo_init_enter(lifo) sys_trace_k_lifo_init_enter(lifo)

#define sys_port_trace_k_lifo_init_exit(lifo) sys_trace_k_lifo_init_exit(lifo)

#define sys_port_trace_k_lifo_put_enter(lifo, data) sys_trace_k_lifo_put_enter(lifo, data)

#define sys_port_trace_k_lifo_put_exit(lifo, data) sys_trace_k_lifo_put_exit(lifo, data)

#define sys_port_trace_k_lifo_alloc_put_enter(lifo, data)                                          \
	sys_trace_k_lifo_alloc_put_enter(lifo, data)

#define sys_port_trace_k_lifo_alloc_put_exit(lifo, data, ret)                                      \
	sys_trace_k_lifo_alloc_put_exit(lifo, data, ret)

#define sys_port_trace_k_lifo_get_enter(lifo, timeout) sys_trace_k_lifo_get_enter(lifo, timeout)

#define sys_port_trace_k_lifo_get_exit(lifo, timeout, ret)                                         \
	sys_trace_k_lifo_get_exit(lifo, timeout, ret)

/* Stack */
#define sys_port_trace_k_stack_init(stack) sys_trace_k_stack_init(stack, buffer, num_entries)

#define sys_port_trace_k_stack_alloc_init_enter(stack)                                             \
	sys_trace_k_stack_alloc_init_enter(stack, num_entries)

#define sys_port_trace_k_stack_alloc_init_exit(stack, ret)                                         \
	sys_trace_k_stack_alloc_init_exit(stack, num_entries, ret)

#define sys_port_trace_k_stack_cleanup_enter(stack) sys_trace_k_stack_cleanup_enter(stack)

#define sys_port_trace_k_stack_cleanup_exit(stack, ret) sys_trace_k_stack_cleanup_exit(stack, ret)

#define sys_port_trace_k_stack_push_enter(stack) sys_trace_k_stack_push_enter(stack, data)

#define sys_port_trace_k_stack_push_exit(stack, ret) sys_trace_k_stack_push_exit(stack, data, ret)

#define sys_port_trace_k_stack_pop_enter(stack, timeout)

#define sys_port_trace_k_stack_pop_blocking(stack, timeout)                                        \
	sys_trace_k_stack_pop_blocking(stack, data, timeout)

#define sys_port_trace_k_stack_pop_exit(stack, timeout, ret)                                       \
	sys_trace_k_stack_pop_exit(stack, data, timeout, ret)

/* Message Queue */
#define sys_port_trace_k_msgq_init(msgq) sys_trace_k_msgq_init(msgq)

#define sys_port_trace_k_msgq_alloc_init_enter(msgq)                                               \
	sys_trace_k_msgq_alloc_init_enter(msgq, msg_size, max_msgs)

#define sys_port_trace_k_msgq_alloc_init_exit(msgq, ret)                                           \
	sys_trace_k_msgq_alloc_init_exit(msgq, msg_size, max_msgs, ret)

#define sys_port_trace_k_msgq_cleanup_enter(msgq) sys_trace_k_msgq_cleanup_enter(msgq)

#define sys_port_trace_k_msgq_cleanup_exit(msgq, ret) sys_trace_k_msgq_cleanup_exit(msgq, ret)

#define sys_port_trace_k_msgq_put_enter(msgq, timeout)                                             \
	sys_trace_k_msgq_put_enter(msgq, data, timeout)

#define sys_port_trace_k_msgq_put_blocking(msgq, timeout)                                          \
	sys_trace_k_msgq_put_blocking(msgq, data, timeout)
#define sys_port_trace_k_msgq_put_exit(msgq, timeout, ret)                                         \
	sys_trace_k_msgq_put_exit(msgq, data, timeout, ret)
#define sys_port_trace_k_msgq_get_enter(msgq, timeout)                                             \
	sys_trace_k_msgq_get_enter(msgq, data, timeout)
#define sys_port_trace_k_msgq_get_blocking(msgq, timeout)                                          \
	sys_trace_k_msgq_get_blocking(msgq, data, timeout)
#define sys_port_trace_k_msgq_get_exit(msgq, timeout, ret)                                         \
	sys_trace_k_msgq_get_exit(msgq, data, timeout, ret)
#define sys_port_trace_k_msgq_peek(msgq, ret) sys_trace_k_msgq_peek(msgq, data, ret)
#define sys_port_trace_k_msgq_purge(msgq) sys_trace_k_msgq_purge(msgq)

#define sys_port_trace_k_mbox_init(mbox) sys_trace_k_mbox_init(mbox)
#define sys_port_trace_k_mbox_message_put_enter(mbox, timeout)                                     \
	sys_trace_k_mbox_message_put_enter(mbox, tx_msg, timeout)
#define sys_port_trace_k_mbox_message_put_blocking(mbox, timeout)                                  \
	sys_trace_k_mbox_message_put_blocking(mbox, tx_msg, timeout)
#define sys_port_trace_k_mbox_message_put_exit(mbox, timeout, ret)                                 \
	sys_trace_k_mbox_message_put_exit(mbox, tx_msg, timeout, ret)
#define sys_port_trace_k_mbox_put_enter(mbox, timeout)                                             \
	sys_trace_k_mbox_put_enter(mbox, tx_msg, timeout)
#define sys_port_trace_k_mbox_put_exit(mbox, timeout, ret)                                         \
	sys_trace_k_mbox_put_exit(mbox, tx_msg, timeout, ret)
#define sys_port_trace_k_mbox_async_put_enter(mbox, sem) sys_trace_k_mbox_async_put_enter(mbox, sem)
#define sys_port_trace_k_mbox_async_put_exit(mbox, sem) sys_trace_k_mbox_async_put_exit(mbox, sem)
#define sys_port_trace_k_mbox_get_enter(mbox, timeout)                                             \
	sys_trace_k_mbox_get_enter(mbox, rx_msg, buffer, timeout)
#define sys_port_trace_k_mbox_get_blocking(mbox, timeout)                                          \
	sys_trace_k_mbox_get_blocking(mbox, rx_msg, buffer, timeout)
#define sys_port_trace_k_mbox_get_exit(mbox, timeout, ret)                                         \
	sys_trace_k_mbox_get_exit(mbox, rx_msg, buffer, timeout, ret)
#define sys_port_trace_k_mbox_data_get(rx_msg) sys_trace_k_mbox_data_get(mbox, rx_msg, buffer)

#define sys_port_trace_k_pipe_init(pipe, buffer, size) sys_trace_k_pipe_init(pipe, buffer, size)
#define sys_port_trace_k_pipe_reset_enter(pipe) sys_trace_k_pipe_reset_enter(pipe)
#define sys_port_trace_k_pipe_reset_exit(pipe)  sys_trace_k_pipe_reset_exit(pipe)
#define sys_port_trace_k_pipe_close_enter(pipe) sys_trace_k_pipe_close_enter(pipe)
#define sys_port_trace_k_pipe_close_exit(pipe) sys_trace_k_pipe_close_exit(pipe)
#define sys_port_trace_k_pipe_write_enter(pipe, data, len, timeout) \
	sys_trace_k_pipe_write_enter(pipe, data, len, timeout)
#define sys_port_trace_k_pipe_write_blocking(pipe, timeout) \
	sys_trace_k_pipe_write_blocking(pipe, timeout)
#define sys_port_trace_k_pipe_write_exit(pipe, ret) \
	sys_trace_k_pipe_write_exit(pipe, ret)
#define sys_port_trace_k_pipe_read_enter(pipe, data, size, timeout) \
	sys_trace_k_pipe_read_enter(pipe, data, size, timeout)
#define sys_port_trace_k_pipe_read_blocking(pipe, timeout) \
	sys_trace_k_pipe_read_blocking(pipe, timeout)
#define sys_port_trace_k_pipe_read_exit(pipe, ret) \
	sys_trace_k_pipe_read_exit(pipe, ret)

#define sys_port_trace_k_pipe_cleanup_enter(pipe) sys_trace_k_pipe_cleanup_enter(pipe)
#define sys_port_trace_k_pipe_cleanup_exit(pipe, ret) sys_trace_k_pipe_cleanup_exit(pipe, ret)
#define sys_port_trace_k_pipe_alloc_init_enter(pipe) sys_trace_k_pipe_alloc_init_enter(pipe, size)
#define sys_port_trace_k_pipe_alloc_init_exit(pipe, ret)                                           \
	sys_trace_k_pipe_alloc_init_exit(pipe, size, ret)
#define sys_port_trace_k_pipe_flush_enter(pipe) sys_trace_k_pipe_flush_enter(pipe)
#define sys_port_trace_k_pipe_flush_exit(pipe)  sys_trace_k_pipe_flush_exit(pipe)
#define sys_port_trace_k_pipe_buffer_flush_enter(pipe)  \
	sys_trace_k_pipe_buffer_flush_enter(pipe)
#define sys_port_trace_k_pipe_buffer_flush_exit(pipe)   \
	sys_trace_k_pipe_buffer_flush_exit(pipe)

#define sys_port_trace_k_pipe_put_enter(pipe, timeout)                                             \
	sys_trace_k_pipe_put_enter(pipe, data, bytes_to_write, bytes_written, min_xfer, timeout)
#define sys_port_trace_k_pipe_put_blocking(pipe, timeout)                                          \
	sys_trace_k_pipe_put_blocking(pipe, data, bytes_to_write, bytes_written, min_xfer, timeout)
#define sys_port_trace_k_pipe_put_exit(pipe, timeout, ret)                                         \
	sys_trace_k_pipe_put_exit(pipe, data, bytes_to_write, bytes_written, min_xfer, timeout, ret)
#define sys_port_trace_k_pipe_get_enter(pipe, timeout)                                             \
	sys_trace_k_pipe_get_enter(pipe, data, bytes_to_read, bytes_read, min_xfer, timeout)
#define sys_port_trace_k_pipe_get_blocking(pipe, timeout)                                          \
	sys_trace_k_pipe_get_blocking(pipe, data, bytes_to_read, bytes_read, min_xfer, timeout)
#define sys_port_trace_k_pipe_get_exit(pipe, timeout, ret)                                         \
	sys_trace_k_pipe_get_exit(pipe, data, bytes_to_read, bytes_read, min_xfer, timeout, ret)

#define sys_port_trace_k_heap_init(h) sys_trace_k_heap_init(h, mem, bytes)
#define sys_port_trace_k_heap_aligned_alloc_enter(h, timeout)                                      \
	sys_trace_k_heap_aligned_alloc_enter(h, bytes, timeout)
#define sys_port_trace_k_heap_aligned_alloc_blocking(h, timeout)                                   \
	sys_trace_k_heap_aligned_alloc_blocking(h, bytes, timeout)
#define sys_port_trace_k_heap_aligned_alloc_exit(h, timeout, ret)                                  \
	sys_trace_k_heap_aligned_alloc_exit(h, bytes, timeout, ret)
#define sys_port_trace_k_heap_alloc_enter(h, timeout)                                              \
	sys_trace_k_heap_alloc_enter(h, bytes, timeout)
#define sys_port_trace_k_heap_alloc_exit(h, timeout, ret)                                          \
	sys_trace_k_heap_alloc_exit(h, bytes, timeout, ret)
#define sys_port_trace_k_heap_calloc_enter(h, timeout)                                             \
	sys_trace_k_heap_calloc_enter(h, num, size, timeout)
#define sys_port_trace_k_heap_calloc_exit(h, timeout, ret)                                         \
	sys_trace_k_heap_calloc_exit(h, num, size, timeout, ret)
#define sys_port_trace_k_heap_free(h) sys_trace_k_heap_free(h, mem)
#define sys_port_trace_k_heap_realloc_enter(h, ptr, bytes, timeout)                                \
	sys_trace_k_heap_realloc_enter(h, ptr, bytes, timeout)
#define sys_port_trace_k_heap_realloc_exit(h, ptr, bytes, timeout, ret)                            \
	sys_trace_k_heap_realloc_exit(h, ptr, bytes, timeout, ret)
#define sys_port_trace_k_heap_sys_k_aligned_alloc_enter(heap)                                      \
	sys_trace_k_heap_sys_k_aligned_alloc_enter(heap, align, size)
#define sys_port_trace_k_heap_sys_k_aligned_alloc_exit(heap, ret)                                  \
	sys_trace_k_heap_sys_k_aligned_alloc_exit(heap, align, size, ret)
#define sys_port_trace_k_heap_sys_k_malloc_enter(heap)                                             \
	sys_trace_k_heap_sys_k_malloc_enter(heap, size)
#define sys_port_trace_k_heap_sys_k_malloc_exit(heap, ret)                                         \
	sys_trace_k_heap_sys_k_malloc_exit(heap, size, ret)
#define sys_port_trace_k_heap_sys_k_free_enter(heap, heap_ref)                                     \
	sys_trace_k_heap_sys_k_free_enter(heap, heap_ref)
#define sys_port_trace_k_heap_sys_k_free_exit(heap, heap_ref)                                      \
	sys_trace_k_heap_sys_k_free_exit(heap, heap_ref)
#define sys_port_trace_k_heap_sys_k_calloc_enter(heap)                                             \
	sys_trace_k_heap_sys_k_calloc_enter(heap, nmemb, size)
#define sys_port_trace_k_heap_sys_k_calloc_exit(heap, ret)                                         \
	sys_trace_k_heap_sys_k_calloc_exit(heap, nmemb, size, ret)
#define sys_port_trace_k_heap_sys_k_realloc_enter(heap, ptr)                                       \
	sys_trace_k_heap_sys_k_realloc_enter(heap, ptr, size)
#define sys_port_trace_k_heap_sys_k_realloc_exit(heap, ptr, ret)                                   \
	sys_trace_k_heap_sys_k_realloc_exit(heap, ptr, size, ret)

#define sys_port_trace_k_mem_slab_init(slab, rc)                                                   \
	sys_trace_k_mem_slab_init(slab, buffer, block_size, num_blocks, rc)
#define sys_port_trace_k_mem_slab_alloc_enter(slab, timeout)                                       \
	sys_trace_k_mem_slab_alloc_enter(slab, mem, timeout)
#define sys_port_trace_k_mem_slab_alloc_blocking(slab, timeout)                                    \
	sys_trace_k_mem_slab_alloc_blocking(slab, mem, timeout)
#define sys_port_trace_k_mem_slab_alloc_exit(slab, timeout, ret)                                   \
	sys_trace_k_mem_slab_alloc_exit(slab, mem, timeout, ret)
#define sys_port_trace_k_mem_slab_free_enter(slab)
#define sys_port_trace_k_mem_slab_free_exit(slab) sys_trace_k_mem_slab_free_exit(slab, mem)

#define sys_port_trace_k_timer_init(timer) sys_trace_k_timer_init(timer, expiry_fn, stop_fn)
#define sys_port_trace_k_timer_start(timer, duration, period)					   \
	sys_trace_k_timer_start(timer, duration, period)
#define sys_port_trace_k_timer_stop(timer) sys_trace_k_timer_stop(timer)
#define sys_port_trace_k_timer_status_sync_enter(timer)
#define sys_port_trace_k_timer_status_sync_blocking(timer, timeout)                                \
	sys_trace_k_timer_status_sync_blocking(timer)
#define sys_port_trace_k_timer_status_sync_exit(timer, result)                                     \
	sys_trace_k_timer_status_sync_exit(timer, result)

#define sys_port_trace_k_event_init(event) sys_trace_k_event_init(event)
#define sys_port_trace_k_event_post_enter(event, events, events_mask)   \
	sys_trace_k_event_post_enter(event, events, events_mask)
#define sys_port_trace_k_event_post_exit(event, events, events_mask)   \
	sys_trace_k_event_post_exit(event, events, events_mask)
#define sys_port_trace_k_event_wait_enter(event, events, options, timeout)   \
	sys_trace_k_event_wait_enter(event, events, options, timeout)
#define sys_port_trace_k_event_wait_blocking(event, events, options, timeout) \
	sys_trace_k_event_wait_blocking(event, events, options, timeout)
#define sys_port_trace_k_event_wait_exit(event, events, ret)   \
	sys_trace_k_event_wait_exit(event, events, ret)

#define sys_port_trace_k_thread_abort_exit(thread) sys_trace_k_thread_abort_exit(thread)

#define sys_port_trace_k_thread_abort_enter(thread) sys_trace_k_thread_abort_enter(thread)

#define sys_port_trace_k_thread_resume_exit(thread) sys_trace_k_thread_resume_exit(thread)

#define sys_port_trace_pm_system_suspend_enter(ticks)
#define sys_port_trace_pm_system_suspend_exit(ticks, state)

#define sys_port_trace_pm_device_runtime_get_enter(dev)
#define sys_port_trace_pm_device_runtime_get_exit(dev, ret)
#define sys_port_trace_pm_device_runtime_put_enter(dev)
#define sys_port_trace_pm_device_runtime_put_exit(dev, ret)
#define sys_port_trace_pm_device_runtime_put_async_enter(dev, delay)
#define sys_port_trace_pm_device_runtime_put_async_exit(dev, delay, ret)
#define sys_port_trace_pm_device_runtime_enable_enter(dev)
#define sys_port_trace_pm_device_runtime_enable_exit(dev, ret)
#define sys_port_trace_pm_device_runtime_disable_enter(dev)
#define sys_port_trace_pm_device_runtime_disable_exit(dev, ret)

void sys_trace_idle(void);
void sys_trace_isr_enter(void);
void sys_trace_isr_exit(void);

void sys_trace_k_thread_abort_exit(struct k_thread *thread);
void sys_trace_k_thread_abort_enter(struct k_thread *thread);
void sys_trace_k_thread_resume_exit(struct k_thread *thread);
void sys_trace_k_thread_sched_wakeup(struct k_thread *thread);
void sys_trace_k_thread_sched_abort(struct k_thread *thread);
void sys_trace_k_thread_sched_set_priority(struct k_thread *thread, int prio);
void sys_trace_k_thread_sched_ready(struct k_thread *thread);
void sys_trace_k_thread_sched_pend(struct k_thread *thread);
void sys_trace_k_thread_sched_resume(struct k_thread *thread);
void sys_trace_k_thread_sched_suspend(struct k_thread *thread);

void sys_trace_k_thread_foreach_enter(k_thread_user_cb_t user_cb, void *user_data);
void sys_trace_k_thread_foreach_exit(k_thread_user_cb_t user_cb, void *user_data);
void sys_trace_k_thread_foreach_unlocked_enter(k_thread_user_cb_t user_cb, void *user_data);
void sys_trace_k_thread_foreach_unlocked_exit(k_thread_user_cb_t user_cb, void *user_data);
void sys_trace_k_thread_create(struct k_thread *new_thread, size_t stack_size, int prio);
void sys_trace_k_thread_user_mode_enter(k_thread_entry_t entry, void *p1, void *p2, void *p3);
void sys_trace_k_thread_heap_assign(struct k_thread *thread, struct k_heap *heap);
void sys_trace_k_thread_join_blocking(struct k_thread *thread, k_timeout_t timeout);
void sys_trace_k_thread_join_exit(struct k_thread *thread, k_timeout_t timeout, int ret);
void sys_trace_k_thread_sleep_enter(k_timeout_t timeout);
void sys_trace_k_thread_sleep_exit(k_timeout_t timeout, int ret);
void sys_trace_k_thread_msleep_enter(int32_t ms);
void sys_trace_k_thread_msleep_exit(int32_t ms, int ret);
void sys_trace_k_thread_usleep_enter(int32_t us);
void sys_trace_k_thread_usleep_exit(int32_t us, int ret);

void sys_trace_k_thread_yield(void);
void sys_trace_k_thread_wakeup(struct k_thread *thread);
void sys_trace_k_thread_abort(struct k_thread *thread);
void sys_trace_k_thread_start(struct k_thread *thread);
void sys_trace_k_thread_priority_set(struct k_thread *thread);
void sys_trace_k_thread_suspend(struct k_thread *thread);
void sys_trace_k_thread_resume(struct k_thread *thread);
void sys_trace_k_thread_sched_lock(void);
void sys_trace_k_thread_sched_unlock(void);
void sys_trace_k_thread_name_set(struct k_thread *thread, int ret);
void sys_trace_k_thread_switched_out(void);
void sys_trace_k_thread_switched_in(void);
void sys_trace_k_thread_ready(struct k_thread *thread);
void sys_trace_k_thread_pend(struct k_thread *thread);
void sys_trace_k_thread_info(struct k_thread *thread);

void sys_trace_k_sem_init(struct k_sem *sem, int ret);
void sys_trace_k_sem_give_enter(struct k_sem *sem);
void sys_trace_k_sem_take_enter(struct k_sem *sem, k_timeout_t timeout);
void sys_trace_k_sem_take_blocking(struct k_sem *sem, k_timeout_t timeout);
void sys_trace_k_sem_take_exit(struct k_sem *sem, k_timeout_t timeout, int ret);
void sys_trace_k_sem_reset(struct k_sem *sem);

void sys_trace_k_mutex_init(struct k_mutex *mutex, int ret);
void sys_trace_k_mutex_lock_enter(struct k_mutex *mutex, k_timeout_t timeout);
void sys_trace_k_mutex_lock_blocking(struct k_mutex *mutex, k_timeout_t timeout);
void sys_trace_k_mutex_lock_exit(struct k_mutex *mutex, k_timeout_t timeout, int ret);
void sys_trace_k_mutex_unlock_enter(struct k_mutex *mutex);
void sys_trace_k_mutex_unlock_exit(struct k_mutex *mutex, int ret);

void sys_trace_k_condvar_init(struct k_condvar *condvar, int ret);
void sys_trace_k_condvar_signal_enter(struct k_condvar *condvar);
void sys_trace_k_condvar_signal_blocking(struct k_condvar *condvar);
void sys_trace_k_condvar_signal_exit(struct k_condvar *condvar, int ret);
void sys_trace_k_condvar_broadcast_enter(struct k_condvar *condvar);
void sys_trace_k_condvar_broadcast_exit(struct k_condvar *condvar, int ret);
void sys_trace_k_condvar_wait_enter(struct k_condvar *condvar, struct k_mutex *mutex,
				    k_timeout_t timeout);
void sys_trace_k_condvar_wait_exit(struct k_condvar *condvar, struct k_mutex *mutex,
				   k_timeout_t timeout, int ret);

void sys_trace_k_queue_init(struct k_queue *queue);
void sys_trace_k_queue_cancel_wait(struct k_queue *queue);
void sys_trace_k_queue_queue_insert_enter(struct k_queue *queue, bool alloc, void *data);
void sys_trace_k_queue_queue_insert_blocking(struct k_queue *queue, bool alloc, void *data);
void sys_trace_k_queue_queue_insert_exit(struct k_queue *queue, bool alloc, void *data, int ret);
void sys_trace_k_queue_append_enter(struct k_queue *queue, void *data);
void sys_trace_k_queue_append_exit(struct k_queue *queue, void *data);
void sys_trace_k_queue_alloc_append_enter(struct k_queue *queue, void *data);
void sys_trace_k_queue_alloc_append_exit(struct k_queue *queue, void *data, int ret);
void sys_trace_k_queue_prepend_enter(struct k_queue *queue, void *data);
void sys_trace_k_queue_prepend_exit(struct k_queue *queue, void *data);
void sys_trace_k_queue_alloc_prepend_enter(struct k_queue *queue, void *data);
void sys_trace_k_queue_alloc_prepend_exit(struct k_queue *queue, void *data, int ret);
void sys_trace_k_queue_insert_enter(struct k_queue *queue, void *prev, void *data);
void sys_trace_k_queue_insert_exit(struct k_queue *queue, void *prev, void *data);
void sys_trace_k_queue_append_list_exit(struct k_queue *queue, void *head, void *tail, int ret);
void sys_trace_k_queue_merge_slist_enter(struct k_queue *queue, sys_slist_t *list);
void sys_trace_k_queue_merge_slist_exit(struct k_queue *queue, sys_slist_t *list, int ret);
void sys_trace_k_queue_get_blocking(struct k_queue *queue, k_timeout_t timeout);
void sys_trace_k_queue_get_exit(struct k_queue *queue, k_timeout_t timeout, void *ret);
void sys_trace_k_queue_remove_enter(struct k_queue *queue, void *data);
void sys_trace_k_queue_remove_exit(struct k_queue *queue, void *data, bool ret);
void sys_trace_k_queue_unique_append_enter(struct k_queue *queue, void *data);
void sys_trace_k_queue_unique_append_exit(struct k_queue *queue, void *data, bool ret);
void sys_trace_k_queue_peek_head(struct k_queue *queue, void *ret);
void sys_trace_k_queue_peek_tail(struct k_queue *queue, void *ret);

void sys_trace_k_fifo_init_enter(struct k_fifo *fifo);
void sys_trace_k_fifo_init_exit(struct k_fifo *fifo);
void sys_trace_k_fifo_cancel_wait_enter(struct k_fifo *fifo);
void sys_trace_k_fifo_cancel_wait_exit(struct k_fifo *fifo);
void sys_trace_k_fifo_put_enter(struct k_fifo *fifo, void *data);
void sys_trace_k_fifo_put_exit(struct k_fifo *fifo, void *data);
void sys_trace_k_fifo_alloc_put_enter(struct k_fifo *fifo, void *data);
void sys_trace_k_fifo_alloc_put_exit(struct k_fifo *fifo, void *data, int ret);
void sys_trace_k_fifo_put_list_enter(struct k_fifo *fifo, void *head, void *tail);
void sys_trace_k_fifo_put_list_exit(struct k_fifo *fifo, void *head, void *tail);
void sys_trace_k_fifo_put_slist_enter(struct k_fifo *fifo, sys_slist_t *list);
void sys_trace_k_fifo_put_slist_exit(struct k_fifo *fifo, sys_slist_t *list);
void sys_trace_k_fifo_get_enter(struct k_fifo *fifo, k_timeout_t timeout);
void sys_trace_k_fifo_get_exit(struct k_fifo *fifo, k_timeout_t timeout, void *ret);
void sys_trace_k_fifo_peek_head_enter(struct k_fifo *fifo);
void sys_trace_k_fifo_peek_head_exit(struct k_fifo *fifo, void *ret);
void sys_trace_k_fifo_peek_tail_enter(struct k_fifo *fifo);
void sys_trace_k_fifo_peek_tail_exit(struct k_fifo *fifo, void *ret);

void sys_trace_k_lifo_init_enter(struct k_lifo *lifo);
void sys_trace_k_lifo_init_exit(struct k_lifo *lifo);
void sys_trace_k_lifo_put_enter(struct k_lifo *lifo, void *data);
void sys_trace_k_lifo_put_exit(struct k_lifo *lifo, void *data);
void sys_trace_k_lifo_alloc_put_enter(struct k_lifo *lifo, void *data);
void sys_trace_k_lifo_alloc_put_exit(struct k_lifo *lifo, void *data, int ret);
void sys_trace_k_lifo_get_enter(struct k_lifo *lifo, k_timeout_t timeout);
void sys_trace_k_lifo_get_exit(struct k_lifo *lifo, k_timeout_t timeout, void *ret);

void sys_trace_k_stack_init(struct k_stack *stack, stack_data_t *buffer, uint32_t num_entries);
void sys_trace_k_stack_alloc_init_enter(struct k_stack *stack, uint32_t num_entries);
void sys_trace_k_stack_alloc_init_exit(struct k_stack *stack, uint32_t num_entries, int ret);
void sys_trace_k_stack_cleanup_enter(struct k_stack *stack);
void sys_trace_k_stack_cleanup_exit(struct k_stack *stack, int ret);
void sys_trace_k_stack_push_enter(struct k_stack *stack, stack_data_t data);
void sys_trace_k_stack_push_exit(struct k_stack *stack, stack_data_t data, int ret);
void sys_trace_k_stack_pop_blocking(struct k_stack *stack, stack_data_t *data, k_timeout_t timeout);
void sys_trace_k_stack_pop_exit(struct k_stack *stack, stack_data_t *data, k_timeout_t timeout,
				int ret);

void sys_trace_k_mbox_init(struct k_mbox *mbox);
void sys_trace_k_mbox_message_put_enter(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
					k_timeout_t timeout);
void sys_trace_k_mbox_message_put_blocking(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
					   k_timeout_t timeout);
void sys_trace_k_mbox_message_put_exit(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
				       k_timeout_t timeout, int ret);
void sys_trace_k_mbox_put_enter(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
				k_timeout_t timeout);
void sys_trace_k_mbox_put_exit(struct k_mbox *mbox, struct k_mbox_msg *tx_msg, k_timeout_t timeout,
			       int ret);
void sys_trace_k_mbox_async_put_enter(struct k_mbox *mbox, struct k_sem *sem);
void sys_trace_k_mbox_async_put_exit(struct k_mbox *mbox, struct k_sem *sem);
void sys_trace_k_mbox_get_enter(struct k_mbox *mbox, struct k_mbox_msg *rx_msg, void *buffer,
				k_timeout_t timeout);
void sys_trace_k_mbox_get_blocking(struct k_mbox *mbox, struct k_mbox_msg *rx_msg, void *buffer,
				   k_timeout_t timeout);
void sys_trace_k_mbox_get_exit(struct k_mbox *mbox, struct k_mbox_msg *rx_msg, void *buffer,
			       k_timeout_t timeout, int ret);
void sys_trace_k_mbox_data_get(struct k_mbox *mbox, struct k_mbox_msg *rx_msg, void *buffer);

void sys_trace_k_pipe_init(struct k_pipe *pipe, unsigned char *buffer, size_t size);
void sys_trace_k_pipe_reset_enter(struct k_pipe *pipe);
void sys_trace_k_pipe_reset_exit(struct k_pipe *pipe);
void sys_trace_k_pipe_close_enter(struct k_pipe *pipe);
void sys_trace_k_pipe_close_exit(struct k_pipe *pipe);
void sys_trace_k_pipe_write_enter(struct k_pipe *pipe, const void *data, size_t len,
				  k_timeout_t timeout);
void sys_trace_k_pipe_write_blocking(struct k_pipe *pipe, k_timeout_t timeout);
void sys_trace_k_pipe_write_exit(struct k_pipe *pipe, int ret);
void sys_trace_k_pipe_read_enter(struct k_pipe *pipe, const void *data, size_t len,
				 k_timeout_t timeout);
void sys_trace_k_pipe_read_blocking(struct k_pipe *pipe, k_timeout_t timeout);
void sys_trace_k_pipe_read_exit(struct k_pipe *pipe, int ret);

void sys_trace_k_pipe_cleanup_enter(struct k_pipe *pipe);
void sys_trace_k_pipe_cleanup_exit(struct k_pipe *pipe, int ret);
void sys_trace_k_pipe_alloc_init_enter(struct k_pipe *pipe, size_t size);
void sys_trace_k_pipe_alloc_init_exit(struct k_pipe *pipe, size_t size, int ret);
void sys_trace_k_pipe_flush_enter(struct k_pipe *pipe);
void sys_trace_k_pipe_flush_exit(struct k_pipe *pipe);
void sys_trace_k_pipe_buffer_flush_enter(struct k_pipe *pipe);
void sys_trace_k_pipe_buffer_flush_exit(struct k_pipe *pipe);
void sys_trace_k_pipe_put_enter(struct k_pipe *pipe, const void *data, size_t bytes_to_write,
				size_t *bytes_written, size_t min_xfer, k_timeout_t timeout);
void sys_trace_k_pipe_put_blocking(struct k_pipe *pipe, const void *data, size_t bytes_to_write,
				   size_t *bytes_written, size_t min_xfer, k_timeout_t timeout);
void sys_trace_k_pipe_put_exit(struct k_pipe *pipe, const void *data, size_t bytes_to_write,
			       size_t *bytes_written, size_t min_xfer, k_timeout_t timeout,
			       int ret);
void sys_trace_k_pipe_get_enter(struct k_pipe *pipe, void *data, size_t bytes_to_read,
				size_t *bytes_read, size_t min_xfer, k_timeout_t timeout);
void sys_trace_k_pipe_get_blocking(struct k_pipe *pipe, void *data, size_t bytes_to_read,
				   size_t *bytes_read, size_t min_xfer, k_timeout_t timeout);
void sys_trace_k_pipe_get_exit(struct k_pipe *pipe, void *data, size_t bytes_to_read,
			       size_t *bytes_read, size_t min_xfer, k_timeout_t timeout, int ret);

void sys_trace_k_msgq_init(struct k_msgq *msgq);
void sys_trace_k_msgq_alloc_init_enter(struct k_msgq *msgq, size_t msg_size, uint32_t max_msgs);
void sys_trace_k_msgq_alloc_init_exit(struct k_msgq *msgq, size_t msg_size, uint32_t max_msgs,
				      int ret);
void sys_trace_k_msgq_cleanup_enter(struct k_msgq *msgq);
void sys_trace_k_msgq_cleanup_exit(struct k_msgq *msgq, int ret);
void sys_trace_k_msgq_put_enter(struct k_msgq *msgq, const void *data, k_timeout_t timeout);
void sys_trace_k_msgq_put_blocking(struct k_msgq *msgq, const void *data, k_timeout_t timeout);
void sys_trace_k_msgq_put_exit(struct k_msgq *msgq, const void *data, k_timeout_t timeout, int ret);
void sys_trace_k_msgq_get_enter(struct k_msgq *msgq, const void *data, k_timeout_t timeout);
void sys_trace_k_msgq_get_blocking(struct k_msgq *msgq, const void *data, k_timeout_t timeout);
void sys_trace_k_msgq_get_exit(struct k_msgq *msgq, const void *data, k_timeout_t timeout, int ret);
void sys_trace_k_msgq_peek(struct k_msgq *msgq, void *data, int ret);
void sys_trace_k_msgq_purge(struct k_msgq *msgq);

void sys_trace_k_heap_init(struct k_heap *h, void *mem, size_t bytes);
void sys_trace_k_heap_alloc_enter(struct k_heap *h, size_t bytes, k_timeout_t timeout);
void sys_trace_k_heap_alloc_exit(struct k_heap *h, size_t bytes, k_timeout_t timeout, void *ret);
void sys_trace_k_heap_calloc_enter(struct k_heap *h, size_t num, size_t size, k_timeout_t timeout);
void sys_trace_k_heap_calloc_exit(struct k_heap *h, size_t num, size_t size, k_timeout_t timeout,
				  void *ret);
void sys_trace_k_heap_aligned_alloc_enter(struct k_heap *h, size_t bytes, k_timeout_t timeout);
void sys_trace_k_heap_aligned_alloc_blocking(struct k_heap *h, size_t bytes, k_timeout_t timeout);
void sys_trace_k_heap_aligned_alloc_exit(struct k_heap *h, size_t bytes, k_timeout_t timeout,
					 void *ret);
void sys_trace_k_heap_free(struct k_heap *h, void *mem);
void sys_trace_k_heap_realloc_enter(struct k_heap *h, void *ptr, size_t bytes, k_timeout_t timeout);
void sys_trace_k_heap_realloc_exit(struct k_heap *h, void *ptr, size_t bytes, k_timeout_t timeout,
				   void *ret);
void sys_trace_k_heap_sys_k_aligned_alloc_enter(struct k_heap *h, size_t align, size_t size);
void sys_trace_k_heap_sys_k_aligned_alloc_exit(struct k_heap *h, size_t align, size_t size,
					       void *ret);
void sys_trace_k_heap_sys_k_malloc_enter(struct k_heap *h, size_t size);
void sys_trace_k_heap_sys_k_malloc_exit(struct k_heap *h, size_t size, void *ret);
void sys_trace_k_heap_sys_k_free_enter(struct k_heap *h, struct k_heap **heap_ref);
void sys_trace_k_heap_sys_k_free_exit(struct k_heap *h, struct k_heap **heap_ref);
void sys_trace_k_heap_sys_k_calloc_enter(struct k_heap *h, size_t nmemb, size_t size);
void sys_trace_k_heap_sys_k_calloc_exit(struct k_heap *h, size_t nmemb, size_t size, void *ret);
void sys_trace_k_heap_sys_k_realloc_enter(struct k_heap *h, void *ptr, size_t bytes);
void sys_trace_k_heap_sys_k_realloc_exit(struct k_heap *h, void *ptr, size_t bytes, void *ret);

void sys_trace_k_mem_slab_init(struct k_mem_slab *slab, void *buffer, size_t block_size,
			       uint32_t num_blocks, int ret);
void sys_trace_k_mem_slab_alloc_enter(struct k_mem_slab *slab, void **mem, k_timeout_t timeout);
void sys_trace_k_mem_slab_alloc_blocking(struct k_mem_slab *slab, void **mem, k_timeout_t timeout);
void sys_trace_k_mem_slab_alloc_exit(struct k_mem_slab *slab, void **mem, k_timeout_t timeout,
				     int ret);
void sys_trace_k_mem_slab_free_exit(struct k_mem_slab *slab, void *mem);

void sys_trace_k_timer_init(struct k_timer *timer, k_timer_expiry_t expiry_fn,
			    k_timer_expiry_t stop_fn);
void sys_trace_k_timer_start(struct k_timer *timer, k_timeout_t duration, k_timeout_t period);
void sys_trace_k_timer_stop(struct k_timer *timer);
void sys_trace_k_timer_status_sync_blocking(struct k_timer *timer);
void sys_trace_k_timer_status_sync_exit(struct k_timer *timer, uint32_t result);

void sys_trace_k_event_init(struct k_event *event);

#define sys_port_trace_socket_init(sock, family, type, proto)
#define sys_port_trace_socket_close_enter(sock)
#define sys_port_trace_socket_close_exit(sock, ret)
#define sys_port_trace_socket_shutdown_enter(sock, how)
#define sys_port_trace_socket_shutdown_exit(sock, ret)
#define sys_port_trace_socket_bind_enter(sock, addr, addrlen)
#define sys_port_trace_socket_bind_exit(sock, ret)
#define sys_port_trace_socket_connect_enter(sock, addr, addrlen)
#define sys_port_trace_socket_connect_exit(sock, ret)
#define sys_port_trace_socket_listen_enter(sock, backlog)
#define sys_port_trace_socket_listen_exit(sock, ret)
#define sys_port_trace_socket_accept_enter(sock)
#define sys_port_trace_socket_accept_exit(sock, addr, addrlen, ret)
#define sys_port_trace_socket_sendto_enter(sock, len, flags, dest_addr, addrlen)
#define sys_port_trace_socket_sendto_exit(sock, ret)
#define sys_port_trace_socket_sendmsg_enter(sock, msg, flags)
#define sys_port_trace_socket_sendmsg_exit(sock, ret)
#define sys_port_trace_socket_recvfrom_enter(sock, max_len, flags, addr, addrlen)
#define sys_port_trace_socket_recvfrom_exit(sock, src_addr, addrlen, ret)
#define sys_port_trace_socket_recvmsg_enter(sock, msg, flags)
#define sys_port_trace_socket_recvmsg_exit(sock, msg, ret)
#define sys_port_trace_socket_fcntl_enter(sock, cmd, flags)
#define sys_port_trace_socket_fcntl_exit(sock, ret)
#define sys_port_trace_socket_ioctl_enter(sock, req)
#define sys_port_trace_socket_ioctl_exit(sock, ret)
#define sys_port_trace_socket_poll_enter(fds, nfds, timeout)
#define sys_port_trace_socket_poll_exit(fds, nfds, ret)
#define sys_port_trace_socket_getsockopt_enter(sock, level, optname)
#define sys_port_trace_socket_getsockopt_exit(sock, level, optname, optval, optlen, ret)
#define sys_port_trace_socket_setsockopt_enter(sock, level, optname, optval, optlen)
#define sys_port_trace_socket_setsockopt_exit(sock, ret)
#define sys_port_trace_socket_getpeername_enter(sock)
#define sys_port_trace_socket_getpeername_exit(sock, addr, addrlen, ret)
#define sys_port_trace_socket_getsockname_enter(sock)
#define sys_port_trace_socket_getsockname_exit(sock, addr, addrlen, ret)
#define sys_port_trace_socket_socketpair_enter(family, type, proto, sv)
#define sys_port_trace_socket_socketpair_exit(sockA, sockB, ret)

#define sys_port_trace_net_recv_data_enter(iface, pkt)
#define sys_port_trace_net_recv_data_exit(iface, pkt, ret)
#define sys_port_trace_net_send_data_enter(pkt)
#define sys_port_trace_net_send_data_exit(pkt, ret)
#define sys_port_trace_net_rx_time(pkt, end_time)
#define sys_port_trace_net_tx_time(pkt, end_time)

#define sys_trace_sys_init_enter(...)
#define sys_trace_sys_init_exit(...)

#define sys_trace_named_event(name, arg0, arg1)
#define sys_port_trace_gpio_pin_interrupt_configure_enter(port, pin, flags)
#define sys_port_trace_gpio_pin_interrupt_configure_exit(port, pin, ret)
#define sys_port_trace_gpio_pin_configure_enter(port, pin, flags)
#define sys_port_trace_gpio_pin_configure_exit(port, pin, ret)
#define sys_port_trace_gpio_port_get_direction_enter(port, map, inputs, outputs)
#define sys_port_trace_gpio_port_get_direction_exit(port, map, ret)
#define sys_port_trace_gpio_pin_get_config_enter(port, pin, flags)
#define sys_port_trace_gpio_pin_get_config_exit(port, pin, ret)
#define sys_port_trace_gpio_port_get_raw_enter(port, value)
#define sys_port_trace_gpio_port_get_raw_exit(port, ret)
#define sys_port_trace_gpio_port_set_masked_raw_enter(port, mask, value)
#define sys_port_trace_gpio_port_set_masked_raw_exit(port, ret)
#define sys_port_trace_gpio_port_set_bits_raw_enter(port, pins)
#define sys_port_trace_gpio_port_set_bits_raw_exit(port, ret)
#define sys_port_trace_gpio_port_clear_bits_raw_enter(port, pins)
#define sys_port_trace_gpio_port_clear_bits_raw_exit(port, ret)
#define sys_port_trace_gpio_port_toggle_bits_enter(port, pins)
#define sys_port_trace_gpio_port_toggle_bits_exit(port, ret)
#define sys_port_trace_gpio_init_callback_enter(callback, handler, pin_mask)
#define sys_port_trace_gpio_init_callback_exit(callback)
#define sys_port_trace_gpio_add_callback_enter(port, callback)
#define sys_port_trace_gpio_add_callback_exit(port, ret)
#define sys_port_trace_gpio_remove_callback_enter(port, callback)
#define sys_port_trace_gpio_remove_callback_exit(port, ret)
#define sys_port_trace_gpio_get_pending_int_enter(dev)
#define sys_port_trace_gpio_get_pending_int_exit(dev, ret)
#define sys_port_trace_gpio_fire_callbacks_enter(list, port, pins)
#define sys_port_trace_gpio_fire_callback(port, cb)

#endif /* ZEPHYR_TRACE_TEST_H */
