/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal eBPF tracing glue for TRACING_CUSTOM backends.
 */

#ifndef ZEPHYR_CUSTOM_TRACING_H_
#define ZEPHYR_CUSTOM_TRACING_H_

#include <zephyr/ebpf/ebpf_tracepoint.h>

/** @cond INTERNAL_HIDDEN */

/* Direct tracing hooks provided by subsys/ebpf/ebpf_tracing.c. */
void sys_trace_isr_enter(void);
void sys_trace_isr_exit(void);
void sys_trace_isr_exit_to_scheduler(void);
void sys_trace_idle(void);
void sys_trace_idle_exit(void);

/* Init tracing is not wired yet; keep the kernel hooks as no-ops. */
#define sys_trace_sys_init_enter(entry, level) do { } while (false)
#define sys_trace_sys_init_exit(entry, level, result) do { } while (false)

/*
 * Zephyr's object-tracing dispatcher expands the trace call before
 * passing it through the type-mask macro. Our trace hooks are statement
 * macros that contain commas in designated initializers, so the mask
 * wrapper must be variadic to avoid argument splitting during expansion.
 */
#ifdef sys_port_trace_type_mask_k_thread
#undef sys_port_trace_type_mask_k_thread
#define sys_port_trace_type_mask_k_thread(...) __VA_ARGS__
#endif
#ifdef sys_port_trace_type_mask_k_sem
#undef sys_port_trace_type_mask_k_sem
#define sys_port_trace_type_mask_k_sem(...) __VA_ARGS__
#endif
#ifdef sys_port_trace_type_mask_k_mutex
#undef sys_port_trace_type_mask_k_mutex
#define sys_port_trace_type_mask_k_mutex(...) __VA_ARGS__
#endif

/* ====== eBPF-wired tracepoints ====== */

#define sys_port_trace_k_mutex_init(mutex, ret) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_MUTEX_INIT)) { \
			struct ebpf_ctx_thread _ctx = {0}; \
			ebpf_tracepoint_dispatch(EBPF_TP_MUTEX_INIT, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_mutex_lock_enter(mutex, timeout) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_MUTEX_LOCK_ENTER)) { \
			struct ebpf_ctx_thread _ctx = {0}; \
			ebpf_tracepoint_dispatch(EBPF_TP_MUTEX_LOCK_ENTER, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_mutex_lock_exit(mutex, timeout, ret) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_MUTEX_LOCK_EXIT)) { \
			struct ebpf_ctx_thread _ctx = {0}; \
			ebpf_tracepoint_dispatch(EBPF_TP_MUTEX_LOCK_EXIT, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_mutex_unlock_enter(mutex) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_MUTEX_UNLOCK_ENTER)) { \
			struct ebpf_ctx_thread _ctx = {0}; \
			ebpf_tracepoint_dispatch(EBPF_TP_MUTEX_UNLOCK_ENTER, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_mutex_unlock_exit(mutex, ret) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_MUTEX_UNLOCK_EXIT)) { \
			struct ebpf_ctx_thread _ctx = {0}; \
			ebpf_tracepoint_dispatch(EBPF_TP_MUTEX_UNLOCK_EXIT, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_sem_give_enter(sem) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_SEM_GIVE_ENTER)) { \
			struct ebpf_ctx_thread _ctx = {0}; \
			ebpf_tracepoint_dispatch(EBPF_TP_SEM_GIVE_ENTER, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_sem_init(sem, ret) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_SEM_INIT)) { \
			struct ebpf_ctx_thread _ctx = {0}; \
			ebpf_tracepoint_dispatch(EBPF_TP_SEM_INIT, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_sem_take_enter(sem, timeout) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_SEM_TAKE_ENTER)) { \
			struct ebpf_ctx_thread _ctx = {0}; \
			ebpf_tracepoint_dispatch(EBPF_TP_SEM_TAKE_ENTER, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_sem_take_exit(sem, timeout, ret) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_SEM_TAKE_EXIT)) { \
			struct ebpf_ctx_thread _ctx = {0}; \
			ebpf_tracepoint_dispatch(EBPF_TP_SEM_TAKE_EXIT, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_abort(thread) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_ABORT)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)(thread), \
				.priority = (thread)->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_ABORT, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_create(new_thread) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_CREATE)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)(new_thread), \
				.priority = (new_thread)->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_CREATE, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_pend(thread) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_PEND)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)(thread), \
				.priority = (thread)->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_PEND, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_ready(thread) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_READY)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)(thread), \
				.priority = (thread)->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_READY, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_resume_enter(thread) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_RESUME)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)(thread), \
				.priority = (thread)->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_RESUME, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_sleep_enter(timeout) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_SLEEP_ENTER)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)_current, \
				.priority = _current->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_SLEEP_ENTER, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_sleep_exit(timeout, ret) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_SLEEP_EXIT)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)_current, \
				.priority = _current->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_SLEEP_EXIT, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_suspend_enter(thread) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_SUSPEND)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)(thread), \
				.priority = (thread)->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_SUSPEND, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_switched_in() \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_SWITCHED_IN)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)_current, \
				.priority = _current->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_SWITCHED_IN, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_switched_out() \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_SWITCHED_OUT)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)_current, \
				.priority = _current->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_SWITCHED_OUT, \
						 &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_wakeup(thread) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_WAKEUP)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)(thread), \
				.priority = (thread)->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_WAKEUP, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#define sys_port_trace_k_thread_yield() \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_THREAD_YIELD)) { \
			struct ebpf_ctx_thread _ctx = { \
				.thread_id = (uint32_t)(uintptr_t)_current, \
				.priority = _current->base.prio, \
			}; \
			ebpf_tracepoint_dispatch(EBPF_TP_THREAD_YIELD, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

/* ====== No-op macros for unwired tracepoints ====== */

#define sys_port_trace_gpio_add_callback_enter(port, callback)
#define sys_port_trace_gpio_add_callback_exit(port, ret)
#define sys_port_trace_gpio_fire_callback(port, callback)
#define sys_port_trace_gpio_fire_callbacks_enter(list, port, pins)
#define sys_port_trace_gpio_get_pending_int_enter(dev)
#define sys_port_trace_gpio_get_pending_int_exit(dev, ret)
#define sys_port_trace_gpio_init_callback_enter(callback, handler, pin_mask)
#define sys_port_trace_gpio_init_callback_exit(callback)
#define sys_port_trace_gpio_pin_configure_enter(port, pin, flags)
#define sys_port_trace_gpio_pin_configure_exit(port, pin, ret)
#define sys_port_trace_gpio_pin_get_config_enter(port, pin, flags)
#define sys_port_trace_gpio_pin_get_config_exit(port, pin, ret)
#define sys_port_trace_gpio_pin_interrupt_configure_enter(port, pin, flags)
#define sys_port_trace_gpio_pin_interrupt_configure_exit(port, pin, ret)
#define sys_port_trace_gpio_port_clear_bits_raw_enter(port, pins)
#define sys_port_trace_gpio_port_clear_bits_raw_exit(port, ret)
#define sys_port_trace_gpio_port_get_direction_enter(port, map, inputs, outputs)
#define sys_port_trace_gpio_port_get_direction_exit(port, ret)
#define sys_port_trace_gpio_port_get_raw_enter(port, value)
#define sys_port_trace_gpio_port_get_raw_exit(port, ret)
#define sys_port_trace_gpio_port_set_bits_raw_enter(port, pins)
#define sys_port_trace_gpio_port_set_bits_raw_exit(port, ret)
#define sys_port_trace_gpio_port_set_masked_raw_enter(port, mask, value)
#define sys_port_trace_gpio_port_set_masked_raw_exit(port, ret)
#define sys_port_trace_gpio_port_toggle_bits_enter(port, pins)
#define sys_port_trace_gpio_port_toggle_bits_exit(port, ret)
#define sys_port_trace_gpio_remove_callback_enter(port, callback)
#define sys_port_trace_gpio_remove_callback_exit(port, ret)
#define sys_port_trace_k_condvar_broadcast_enter(condvar)
#define sys_port_trace_k_condvar_broadcast_exit(condvar, ret)
#define sys_port_trace_k_condvar_init(condvar, ret)
#define sys_port_trace_k_condvar_signal_blocking(condvar, timeout)
#define sys_port_trace_k_condvar_signal_enter(condvar)
#define sys_port_trace_k_condvar_signal_exit(condvar, ret)
#define sys_port_trace_k_condvar_wait_enter(condvar, timeout)
#define sys_port_trace_k_condvar_wait_exit(condvar, timeout, ret)
#define sys_port_trace_k_event_init(event)
#define sys_port_trace_k_event_post_enter(event, events, events_mask)
#define sys_port_trace_k_event_post_exit(event, events, events_mask)
#define sys_port_trace_k_event_wait_blocking(event, events, options, timeout)
#define sys_port_trace_k_event_wait_enter(event, events, options, timeout)
#define sys_port_trace_k_event_wait_exit(event, events, ret)
#define sys_port_trace_k_fifo_alloc_put_enter(fifo, data)
#define sys_port_trace_k_fifo_alloc_put_exit(fifo, data, ret)
#define sys_port_trace_k_fifo_cancel_wait_enter(fifo)
#define sys_port_trace_k_fifo_cancel_wait_exit(fifo)
#define sys_port_trace_k_fifo_get_enter(fifo, timeout)
#define sys_port_trace_k_fifo_get_exit(fifo, timeout, ret)
#define sys_port_trace_k_fifo_init_enter(fifo)
#define sys_port_trace_k_fifo_init_exit(fifo)
#define sys_port_trace_k_fifo_peek_head_enter(fifo)
#define sys_port_trace_k_fifo_peek_head_exit(fifo, ret)
#define sys_port_trace_k_fifo_peek_tail_enter(fifo)
#define sys_port_trace_k_fifo_peek_tail_exit(fifo, ret)
#define sys_port_trace_k_fifo_put_enter(fifo, data)
#define sys_port_trace_k_fifo_put_exit(fifo, data)
#define sys_port_trace_k_fifo_put_list_enter(fifo, head, tail)
#define sys_port_trace_k_fifo_put_list_exit(fifo, head, tail)
#define sys_port_trace_k_fifo_put_slist_enter(fifo, list)
#define sys_port_trace_k_fifo_put_slist_exit(fifo, list)
#define sys_port_trace_k_heap_aligned_alloc_enter(h, timeout)
#define sys_port_trace_k_heap_aligned_alloc_exit(h, timeout, ret)
#define sys_port_trace_k_heap_alloc_enter(h, timeout)
#define sys_port_trace_k_heap_alloc_exit(h, timeout, ret)
#define sys_port_trace_k_heap_alloc_helper_blocking(h, timeout)
#define sys_port_trace_k_heap_calloc_enter(h, timeout)
#define sys_port_trace_k_heap_calloc_exit(h, timeout, ret)
#define sys_port_trace_k_heap_free(h)
#define sys_port_trace_k_heap_init(h)
#define sys_port_trace_k_heap_realloc_enter(h, ptr, bytes, timeout)
#define sys_port_trace_k_heap_realloc_exit(h, ptr, bytes, timeout, ret)
#define sys_port_trace_k_heap_sys_k_aligned_alloc_enter(heap)
#define sys_port_trace_k_heap_sys_k_aligned_alloc_exit(heap, ret)
#define sys_port_trace_k_heap_sys_k_calloc_enter(heap)
#define sys_port_trace_k_heap_sys_k_calloc_exit(heap, ret)
#define sys_port_trace_k_heap_sys_k_free_enter(heap, heap_ref)
#define sys_port_trace_k_heap_sys_k_free_exit(heap, heap_ref)
#define sys_port_trace_k_heap_sys_k_malloc_enter(heap)
#define sys_port_trace_k_heap_sys_k_malloc_exit(heap, ret)
#define sys_port_trace_k_heap_sys_k_realloc_enter(heap, ptr)
#define sys_port_trace_k_heap_sys_k_realloc_exit(heap, ptr, ret)
#define sys_port_trace_k_lifo_alloc_put_enter(lifo, data)
#define sys_port_trace_k_lifo_alloc_put_exit(lifo, data, ret)
#define sys_port_trace_k_lifo_get_enter(lifo, timeout)
#define sys_port_trace_k_lifo_get_exit(lifo, timeout, ret)
#define sys_port_trace_k_lifo_init_enter(lifo)
#define sys_port_trace_k_lifo_init_exit(lifo)
#define sys_port_trace_k_lifo_put_enter(lifo, data)
#define sys_port_trace_k_lifo_put_exit(lifo, data)
#define sys_port_trace_k_mbox_async_put_enter(mbox, sem)
#define sys_port_trace_k_mbox_async_put_exit(mbox, sem)
#define sys_port_trace_k_mbox_data_get(rx_msg)
#define sys_port_trace_k_mbox_get_blocking(mbox, timeout)
#define sys_port_trace_k_mbox_get_enter(mbox, timeout)
#define sys_port_trace_k_mbox_get_exit(mbox, timeout, ret)
#define sys_port_trace_k_mbox_init(mbox)
#define sys_port_trace_k_mbox_message_put_blocking(mbox, timeout)
#define sys_port_trace_k_mbox_message_put_enter(mbox, timeout)
#define sys_port_trace_k_mbox_message_put_exit(mbox, timeout, ret)
#define sys_port_trace_k_mbox_put_enter(mbox, timeout)
#define sys_port_trace_k_mbox_put_exit(mbox, timeout, ret)
#define sys_port_trace_k_mem_slab_alloc_blocking(slab, timeout)
#define sys_port_trace_k_mem_slab_alloc_enter(slab, timeout)
#define sys_port_trace_k_mem_slab_alloc_exit(slab, timeout, ret)
#define sys_port_trace_k_mem_slab_free_enter(slab)
#define sys_port_trace_k_mem_slab_free_exit(slab)
#define sys_port_trace_k_mem_slab_init(slab, rc)
#define sys_port_trace_k_msgq_alloc_init_enter(msgq)
#define sys_port_trace_k_msgq_alloc_init_exit(msgq, ret)
#define sys_port_trace_k_msgq_cleanup_enter(msgq)
#define sys_port_trace_k_msgq_cleanup_exit(msgq, ret)
#define sys_port_trace_k_msgq_get_blocking(msgq, timeout)
#define sys_port_trace_k_msgq_get_enter(msgq, timeout)
#define sys_port_trace_k_msgq_get_exit(msgq, timeout, ret)
#define sys_port_trace_k_msgq_init(msgq)
#define sys_port_trace_k_msgq_peek(msgq, ret)
#define sys_port_trace_k_msgq_purge(msgq)
#define sys_port_trace_k_msgq_put_blocking(msgq, timeout)
#define sys_port_trace_k_msgq_put_enter(msgq, timeout)
#define sys_port_trace_k_msgq_put_exit(msgq, timeout, ret)
#define sys_port_trace_k_msgq_put_front_blocking(msgq, timeout)
#define sys_port_trace_k_msgq_put_front_enter(msgq, timeout)
#define sys_port_trace_k_msgq_put_front_exit(msgq, timeout, ret)
#define sys_port_trace_k_mutex_lock_blocking(mutex, timeout)
#define sys_port_trace_k_pipe_close_enter(pipe)
#define sys_port_trace_k_pipe_close_exit(pipe)
#define sys_port_trace_k_pipe_init(pipe, buffer, size)
#define sys_port_trace_k_pipe_read_blocking(pipe, timeout)
#define sys_port_trace_k_pipe_read_enter(pipe, data, len, timeout)
#define sys_port_trace_k_pipe_read_exit(pipe, ret)
#define sys_port_trace_k_pipe_reset_enter(pipe)
#define sys_port_trace_k_pipe_reset_exit(pipe)
#define sys_port_trace_k_pipe_write_blocking(pipe, timeout)
#define sys_port_trace_k_pipe_write_enter(pipe, data, len, timeout)
#define sys_port_trace_k_pipe_write_exit(pipe, ret)
#define sys_port_trace_k_poll_api_event_init(event)
#define sys_port_trace_k_poll_api_poll_enter(events)
#define sys_port_trace_k_poll_api_poll_exit(events, ret)
#define sys_port_trace_k_poll_api_signal_check(signal)
#define sys_port_trace_k_poll_api_signal_init(signal)
#define sys_port_trace_k_poll_api_signal_raise(signal, ret)
#define sys_port_trace_k_poll_api_signal_reset(signal)
#define sys_port_trace_k_queue_alloc_append_enter(queue)
#define sys_port_trace_k_queue_alloc_append_exit(queue, ret)
#define sys_port_trace_k_queue_alloc_prepend_enter(queue)
#define sys_port_trace_k_queue_alloc_prepend_exit(queue, ret)
#define sys_port_trace_k_queue_append_enter(queue)
#define sys_port_trace_k_queue_append_exit(queue)
#define sys_port_trace_k_queue_append_list_enter(queue)
#define sys_port_trace_k_queue_append_list_exit(queue, ret)
#define sys_port_trace_k_queue_cancel_wait(queue)
#define sys_port_trace_k_queue_get_blocking(queue, timeout)
#define sys_port_trace_k_queue_get_enter(queue, timeout)
#define sys_port_trace_k_queue_get_exit(queue, timeout, ret)
#define sys_port_trace_k_queue_init(queue)
#define sys_port_trace_k_queue_insert_blocking(queue, timeout)
#define sys_port_trace_k_queue_insert_enter(queue)
#define sys_port_trace_k_queue_insert_exit(queue)
#define sys_port_trace_k_queue_merge_slist_enter(queue)
#define sys_port_trace_k_queue_merge_slist_exit(queue, ret)
#define sys_port_trace_k_queue_peek_head(queue, ret)
#define sys_port_trace_k_queue_peek_tail(queue, ret)
#define sys_port_trace_k_queue_prepend_enter(queue)
#define sys_port_trace_k_queue_prepend_exit(queue)
#define sys_port_trace_k_queue_queue_insert_blocking(queue, alloc, timeout)
#define sys_port_trace_k_queue_queue_insert_enter(queue, alloc)
#define sys_port_trace_k_queue_queue_insert_exit(queue, alloc, ret)
#define sys_port_trace_k_queue_remove_enter(queue)
#define sys_port_trace_k_queue_remove_exit(queue, ret)
#define sys_port_trace_k_queue_unique_append_enter(queue)
#define sys_port_trace_k_queue_unique_append_exit(queue, ret)
#define sys_port_trace_k_sem_give_exit(sem)
#define sys_port_trace_k_sem_reset(sem)
#define sys_port_trace_k_sem_take_blocking(sem, timeout)
#define sys_port_trace_k_stack_alloc_init_enter(stack)
#define sys_port_trace_k_stack_alloc_init_exit(stack, ret)
#define sys_port_trace_k_stack_cleanup_enter(stack)
#define sys_port_trace_k_stack_cleanup_exit(stack, ret)
#define sys_port_trace_k_stack_init(stack)
#define sys_port_trace_k_stack_pop_blocking(stack, timeout)
#define sys_port_trace_k_stack_pop_enter(stack, timeout)
#define sys_port_trace_k_stack_pop_exit(stack, timeout, ret)
#define sys_port_trace_k_stack_push_enter(stack)
#define sys_port_trace_k_stack_push_exit(stack, ret)
#define sys_port_trace_k_thread_abort_enter(thread)
#define sys_port_trace_k_thread_abort_exit(thread)
#define sys_port_trace_k_thread_busy_wait_enter(usec_to_wait)
#define sys_port_trace_k_thread_busy_wait_exit(usec_to_wait)
#define sys_port_trace_k_thread_foreach_enter()
#define sys_port_trace_k_thread_foreach_exit()
#define sys_port_trace_k_thread_foreach_unlocked_enter()
#define sys_port_trace_k_thread_foreach_unlocked_exit()
#define sys_port_trace_k_thread_info(thread)
#define sys_port_trace_k_thread_join_blocking(thread, timeout)
#define sys_port_trace_k_thread_join_enter(thread, timeout)
#define sys_port_trace_k_thread_join_exit(thread, timeout, ret)
#define sys_port_trace_k_thread_msleep_enter(ms)
#define sys_port_trace_k_thread_msleep_exit(ms, ret)
#define sys_port_trace_k_thread_name_set(thread, ret)
#define sys_port_trace_k_thread_priority_set(thread)
#define sys_port_trace_k_thread_resume_exit(thread)
#define sys_port_trace_k_thread_sched_abort(thread)
#define sys_port_trace_k_thread_sched_lock()
#define sys_port_trace_k_thread_sched_pend(thread)
#define sys_port_trace_k_thread_sched_priority_set(thread, prio)
#define sys_port_trace_k_thread_sched_ready(thread)
#define sys_port_trace_k_thread_sched_resume(thread)
#define sys_port_trace_k_thread_sched_suspend(thread)
#define sys_port_trace_k_thread_sched_unlock()
#define sys_port_trace_k_thread_sched_wakeup(thread)
#define sys_port_trace_k_thread_start(thread)
#define sys_port_trace_k_thread_suspend_exit(thread)
#define sys_port_trace_k_thread_user_mode_enter()
#define sys_port_trace_k_thread_usleep_enter(us)
#define sys_port_trace_k_thread_usleep_exit(us, ret)
#define sys_port_trace_k_timer_expiry_enter(timer)
#define sys_port_trace_k_timer_expiry_exit(timer)
#define sys_port_trace_k_timer_init(timer)
#define sys_port_trace_k_timer_start(timer, duration, period)
#define sys_port_trace_k_timer_status_sync_blocking(timer, timeout)
#define sys_port_trace_k_timer_status_sync_enter(timer)
#define sys_port_trace_k_timer_status_sync_exit(timer, result)
#define sys_port_trace_k_timer_stop(timer)
#define sys_port_trace_k_timer_stop_fn_expiry_enter(timer)
#define sys_port_trace_k_timer_stop_fn_expiry_exit(timer)
#define sys_port_trace_k_work_cancel_delayable_enter(dwork)
#define sys_port_trace_k_work_cancel_delayable_exit(dwork, ret)
#define sys_port_trace_k_work_cancel_delayable_sync_enter(dwork, sync)
#define sys_port_trace_k_work_cancel_delayable_sync_exit(dwork, sync, ret)
#define sys_port_trace_k_work_cancel_enter(work)
#define sys_port_trace_k_work_cancel_exit(work, ret)
#define sys_port_trace_k_work_cancel_sync_blocking(work, sync)
#define sys_port_trace_k_work_cancel_sync_enter(work, sync)
#define sys_port_trace_k_work_cancel_sync_exit(work, sync, ret)
#define sys_port_trace_k_work_delayable_init(dwork)
#define sys_port_trace_k_work_flush_blocking(work, timeout)
#define sys_port_trace_k_work_flush_delayable_enter(dwork, sync)
#define sys_port_trace_k_work_flush_delayable_exit(dwork, sync, ret)
#define sys_port_trace_k_work_flush_enter(work)
#define sys_port_trace_k_work_flush_exit(work, ret)
#define sys_port_trace_k_work_init(work)
#define sys_port_trace_k_work_poll_cancel_enter(work)
#define sys_port_trace_k_work_poll_cancel_exit(work, ret)
#define sys_port_trace_k_work_poll_init_enter(work)
#define sys_port_trace_k_work_poll_init_exit(work)
#define sys_port_trace_k_work_poll_submit_enter(work, timeout)
#define sys_port_trace_k_work_poll_submit_exit(work, timeout, ret)
#define sys_port_trace_k_work_poll_submit_to_queue_blocking(work_q, work, timeout)
#define sys_port_trace_k_work_poll_submit_to_queue_enter(work_q, work, timeout)
#define sys_port_trace_k_work_poll_submit_to_queue_exit(work_q, work, timeout, ret)
#define sys_port_trace_k_work_queue_drain_enter(queue)
#define sys_port_trace_k_work_queue_drain_exit(queue, ret)
#define sys_port_trace_k_work_queue_init(queue)
#define sys_port_trace_k_work_queue_start_enter(queue)
#define sys_port_trace_k_work_queue_start_exit(queue)
#define sys_port_trace_k_work_queue_stop_blocking(queue, timeout)
#define sys_port_trace_k_work_queue_stop_enter(queue, timeout)
#define sys_port_trace_k_work_queue_stop_exit(queue, timeout, ret)
#define sys_port_trace_k_work_queue_unplug_enter(queue)
#define sys_port_trace_k_work_queue_unplug_exit(queue, ret)
#define sys_port_trace_k_work_reschedule_enter(dwork, delay)
#define sys_port_trace_k_work_reschedule_exit(dwork, delay, ret)
#define sys_port_trace_k_work_reschedule_for_queue_enter(queue, dwork, delay)
#define sys_port_trace_k_work_reschedule_for_queue_exit(queue, dwork, delay, ret)
#define sys_port_trace_k_work_schedule_enter(dwork, delay)
#define sys_port_trace_k_work_schedule_exit(dwork, delay, ret)
#define sys_port_trace_k_work_schedule_for_queue_enter(queue, dwork, delay)
#define sys_port_trace_k_work_schedule_for_queue_exit(queue, dwork, delay, ret)
#define sys_port_trace_k_work_submit_enter(work)
#define sys_port_trace_k_work_submit_exit(work, ret)
#define sys_port_trace_k_work_submit_to_queue_enter(queue, work)
#define sys_port_trace_k_work_submit_to_queue_exit(queue, work, ret)
#define sys_port_trace_net_recv_data_enter(iface, pkt)
#define sys_port_trace_net_recv_data_exit(iface, pkt, ret)
#define sys_port_trace_net_rx_time(pkt, end_time)
#define sys_port_trace_net_send_data_enter(pkt)
#define sys_port_trace_net_send_data_exit(pkt, ret)
#define sys_port_trace_net_tx_time(pkt, end_time)
#define sys_port_trace_pm_device_runtime_disable_enter(dev)
#define sys_port_trace_pm_device_runtime_disable_exit(dev, ret)
#define sys_port_trace_pm_device_runtime_enable_enter(dev)
#define sys_port_trace_pm_device_runtime_enable_exit(dev, ret)
#define sys_port_trace_pm_device_runtime_get_enter(dev)
#define sys_port_trace_pm_device_runtime_get_exit(dev, ret)
#define sys_port_trace_pm_device_runtime_put_async_enter(dev, delay)
#define sys_port_trace_pm_device_runtime_put_async_exit(dev, delay, ret)
#define sys_port_trace_pm_device_runtime_put_enter(dev)
#define sys_port_trace_pm_device_runtime_put_exit(dev, ret)
#define sys_port_trace_pm_system_suspend_enter(ticks)
#define sys_port_trace_pm_system_suspend_exit(ticks, state)
#define sys_port_trace_rtio_chain_next_enter(rtio, iodev_sqe)
#define sys_port_trace_rtio_chain_next_exit(rtio, iodev_sqe)
#define sys_port_trace_rtio_cqe_acquire_enter(rtio)
#define sys_port_trace_rtio_cqe_acquire_exit(rtio, cqe)
#define sys_port_trace_rtio_cqe_consume_enter(rtio)
#define sys_port_trace_rtio_cqe_consume_exit(rtio, cqe)
#define sys_port_trace_rtio_cqe_release(rtio, cqe)
#define sys_port_trace_rtio_cqe_submit_enter(rtio, result, flags)
#define sys_port_trace_rtio_cqe_submit_exit(rtio)
#define sys_port_trace_rtio_sqe_acquire_enter(rtio)
#define sys_port_trace_rtio_sqe_acquire_exit(rtio, sqe)
#define sys_port_trace_rtio_sqe_cancel(sqe)
#define sys_port_trace_rtio_submit_enter(rtio, wait_count)
#define sys_port_trace_rtio_submit_exit(rtio)
#define sys_port_trace_rtio_txn_next_enter(rtio, iodev_sqe)
#define sys_port_trace_rtio_txn_next_exit(rtio, iodev_sqe)
#define sys_port_trace_socket_accept_enter(socket)
#define sys_port_trace_socket_accept_exit(socket, addr, addrlen, ret)
#define sys_port_trace_socket_bind_enter(socket, addr, addrlen)
#define sys_port_trace_socket_bind_exit(socket, ret)
#define sys_port_trace_socket_close_enter(socket)
#define sys_port_trace_socket_close_exit(socket, ret)
#define sys_port_trace_socket_connect_enter(socket, addr, addrlen)
#define sys_port_trace_socket_connect_exit(socket, ret)
#define sys_port_trace_socket_fcntl_enter(socket, cmd, flags)
#define sys_port_trace_socket_fcntl_exit(socket, ret)
#define sys_port_trace_socket_getpeername_enter(socket)
#define sys_port_trace_socket_getpeername_exit(socket, addr, addrlen, ret)
#define sys_port_trace_socket_getsockname_enter(socket)
#define sys_port_trace_socket_getsockname_exit(socket, addr, addrlen, ret)
#define sys_port_trace_socket_getsockopt_enter(socket, level, optname)
#define sys_port_trace_socket_getsockopt_exit(socket, level, optname, optval, optlen, ret)
#define sys_port_trace_socket_init(socket, family, type, proto)
#define sys_port_trace_socket_ioctl_enter(socket, req)
#define sys_port_trace_socket_ioctl_exit(socket, ret)
#define sys_port_trace_socket_listen_enter(socket, backlog)
#define sys_port_trace_socket_listen_exit(socket, ret)
#define sys_port_trace_socket_poll_enter(fds, nfds, timeout)
#define sys_port_trace_socket_poll_exit(fds, nfds, ret)
#define sys_port_trace_socket_recvfrom_enter(socket, max_len, flags, addr, addrlen)
#define sys_port_trace_socket_recvfrom_exit(socket, src_addr, addrlen, ret)
#define sys_port_trace_socket_recvmsg_enter(socket, msg, flags)
#define sys_port_trace_socket_recvmsg_exit(socket, msg, ret)
#define sys_port_trace_socket_sendmsg_enter(socket, msg, flags)
#define sys_port_trace_socket_sendmsg_exit(socket, ret)
#define sys_port_trace_socket_sendto_enter(socket, len, flags, dest_addr, addrlen)
#define sys_port_trace_socket_sendto_exit(socket, ret)
#define sys_port_trace_socket_setsockopt_enter(socket, level, optname, optval, optlen)
#define sys_port_trace_socket_setsockopt_exit(socket, ret)
#define sys_port_trace_socket_shutdown_enter(socket, how)
#define sys_port_trace_socket_shutdown_exit(socket, ret)
#define sys_port_trace_socket_socketpair_enter(family, type, proto, sv)
#define sys_port_trace_socket_socketpair_exit(socket_A, socket_B, ret)

/* Syscall tracing hooks */
#ifdef sys_port_trace_syscall_enter
#undef sys_port_trace_syscall_enter
#endif
#define sys_port_trace_syscall_enter(id, name, ...) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_SYSCALL_ENTER)) { \
			struct ebpf_ctx_syscall _ctx = { .syscall_id = (id) }; \
			ebpf_tracepoint_dispatch(EBPF_TP_SYSCALL_ENTER, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

#ifdef sys_port_trace_syscall_exit
#undef sys_port_trace_syscall_exit
#endif
#define sys_port_trace_syscall_exit(id, name, ...) \
	do { \
		if (ebpf_tracepoint_is_active(EBPF_TP_SYSCALL_EXIT)) { \
			struct ebpf_ctx_syscall _ctx = { .syscall_id = (id) }; \
			ebpf_tracepoint_dispatch(EBPF_TP_SYSCALL_EXIT, &_ctx, sizeof(_ctx)); \
		} \
	} while (false)

/** @endcond */

#endif /* ZEPHYR_CUSTOM_TRACING_H_ */
