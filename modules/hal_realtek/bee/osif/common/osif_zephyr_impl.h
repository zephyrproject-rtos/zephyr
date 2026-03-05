/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file osif_zephyr_impl.h
 * @brief Internal prototypes for Zephyr-based OSIF implementation.
 *
 * This header exposes the Zephyr-based OSIF functions intended to be
 * assigned to chip-specific Patch function pointers.
 */

#ifndef OSIF_ZEPHYR_IMPL_H
#define OSIF_ZEPHYR_IMPL_H

#include <zephyr/kernel.h>
#include "mem_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OSIF_TASK_MIN_PRIORITY 0
#define OSIF_TASK_MAX_PRIORITY 6

/* Timer Status Enumeration */
typedef enum {
	OSIF_TIMER_NOT_ACTIVE = 0,
	OSIF_TIMER_ACTIVE = 1
} osif_timer_status_t;

/* Timer Type Enumeration */
typedef enum {
	OSIF_TIMER_ONCE = 0,    /* One-shot timer */
	OSIF_TIMER_PERIODIC = 1 /* Repeating timer */
} osif_timer_type_t;

/* Scheduler State Enumeration */
typedef enum {
	OSIF_SCHEDULER_SUSPENDED = 0,
	OSIF_SCHEDULER_NOT_STARTED = 1,
	OSIF_SCHEDULER_RUNNING = 2
} osif_sched_state_t;

struct osif_timer {
	struct k_timer ztimer;
	uint32_t interval_ms;
	uint32_t timer_id;
	uint8_t type;
	uint8_t status;
	const char *name;
	bool allocated;
};

struct task_signal {
	struct k_poll_signal *poll_signal;
	struct k_poll_event *poll_event;
	int32_t signal_results;
};

struct osif_task {
	struct k_thread zthread;
	struct k_poll_signal poll_signal;
	struct k_poll_event poll_event;
	int32_t signal_results;
	void *stack_start;
	const char *name;
};

struct osif_task_delete_context {
	struct k_work work;
	struct osif_task *task_to_free;
	void *stack_to_free;
};

/************************************************************
 * MEMORY MANAGEMENT - Internal APIs
 ************************************************************/
void *os_mem_alloc_intern_zephyr(RAM_TYPE ram_type, size_t size, const char *p_func,
				 uint32_t file_line);

void *os_mem_zalloc_intern_zephyr(RAM_TYPE ram_type, size_t size, const char *p_func,
				  uint32_t file_line);

void *os_mem_aligned_alloc_intern_zephyr(RAM_TYPE ram_type, size_t size, uint8_t alignment,
					 const char *p_func, uint32_t file_line);
void os_mem_free_zephyr(void *block);

void os_mem_aligned_free_zephyr(void *block);

size_t os_mem_peek_zephyr(RAM_TYPE ram_type);

/**
 * @brief  Chip-specific Interface
 */
struct k_heap *get_heap_by_type(RAM_TYPE ram_type);

struct k_heap *get_heap_by_address(void *ptr);

void print_heap_stats(const char *heap_name, struct sys_heap *heap, size_t capacity);

void os_mem_peek_printf_zephyr(void);

/************************************************************
 * SCHEDULER - Internal APIs
 ************************************************************/
void os_delay_zephyr(uint32_t ms);
uint64_t os_sys_time_get_zephyr(void);
uint64_t os_sys_tick_get_zephyr(void);
bool os_sched_start_zephyr(void);
bool os_sched_stop_zephyr(void);
bool os_sched_suspend_zephyr(void);
bool os_sched_resume_zephyr(void);
bool os_sched_state_get_zephyr(long *p_state);
bool os_sched_is_start_zephyr(void);

/************************************************************
 * TASK MANAGEMENT - Internal APIs
 ************************************************************/
bool os_task_create_zephyr(void **handle_ptr, const char *name, void (*routine)(void *),
			   void *param, uint16_t stack_size, uint16_t priority);
bool os_task_delete_zephyr(void *handle);
bool os_task_suspend_zephyr(void *handle);
bool os_task_resume_zephyr(void *handle);
bool os_task_yield_zephyr(void);
bool os_task_handle_get_zephyr(void **handle_ptr);
bool os_task_priority_get_zephyr(void *handle, uint16_t *priority);
bool os_task_priority_set_zephyr(void *handle, uint16_t priority);
bool os_task_signal_create_zephyr(void *handle, uint32_t count);
bool os_task_notify_take_zephyr(long clear_count_on_exit, uint32_t wait_ticks, uint32_t *notify);
bool os_task_notify_give_zephyr(void *handle);
void os_task_status_dump_zephyr(void);
bool os_alloc_secure_ctx_zephyr(uint32_t stack_size);
bool os_task_signal_send_zephyr(void *handle, uint32_t signal);
bool os_task_signal_recv_zephyr(uint32_t *p_signal, uint32_t wait_ms);
bool os_task_signal_clear_zephyr(void *handle);

/************************************************************
 * SYNCHRONIZATION - Internal APIs
 ************************************************************/
uint32_t os_lock_zephyr(void);
void os_unlock_zephyr(uint32_t flags);

bool os_sem_create_zephyr(void **handle_ptr, const char *name, uint32_t init_count,
			  uint32_t max_count);
bool os_sem_delete_zephyr(void *handle);
bool os_sem_take_zephyr(void *handle, uint32_t wait_ms);
bool os_sem_give_zephyr(void *handle);

bool os_mutex_create_zephyr(void **handle_ptr);
bool os_mutex_delete_zephyr(void *handle);
bool os_mutex_take_zephyr(void *handle, uint32_t wait_ms);
bool os_mutex_give_zephyr(void *handle);

/************************************************************
 * MESSAGE QUEUE - Internal APIs
 ************************************************************/
bool os_msg_queue_create_intern_zephyr(void **handle_ptr, const char *name, uint32_t msg_num,
				       uint32_t msg_size, const char *func, uint32_t line);
bool os_msg_queue_delete_intern_zephyr(void *handle, const char *func, uint32_t line);
bool os_msg_queue_peek_intern_zephyr(void *handle, uint32_t *msg_num, const char *func,
				     uint32_t line);
bool os_msg_send_intern_zephyr(void *handle, void *msg, uint32_t wait_ms, const char *func,
			       uint32_t line);
bool os_msg_recv_intern_zephyr(void *handle, void *msg, uint32_t wait_ms, const char *func,
			       uint32_t line);

/************************************************************
 * TIMER - Internal APIs
 ************************************************************/
bool os_timer_create_zephyr(void **handle_ptr, const char *timer_name, uint32_t timer_id,
			    uint32_t interval_ms, bool reload, void (*timer_callback)());
bool os_timer_start_zephyr(void **handle_ptr);
bool os_timer_restart_zephyr(void **handle_ptr, uint32_t interval_ms);
bool os_timer_stop_zephyr(void **handle_ptr);
bool os_timer_delete_zephyr(void **handle_ptr);
bool os_timer_id_get_zephyr(void **handle_ptr, uint32_t *timer_id);
bool os_timer_is_timer_active_zephyr(void **handle_ptr);
bool os_timer_state_get_zephyr(void **handle_ptr, uint32_t *timer_state);
bool os_timer_get_auto_reload_zephyr(void **handle_ptr, long *autoreload);
bool os_timer_get_timer_number_zephyr(void **handle_ptr, uint8_t *timer_num);
bool os_timer_dump_zephyr(void);
void os_timer_init_zephyr(void);

#ifdef __cplusplus
}
#endif

#endif /* OSIF_ZEPHYR_IMPL_H */
