/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file osif_zephyr_patch.c
 * @brief RTL8752H specific OSIF implementation and patch initialization.
 *
 * This file implements RTL8752H-specific OSIF interfaces and initializes the
 * OSIF patch function pointers required by RTL8752H internal modules.
 *
 * @note
 * The RTL8752H OSIF patch mechanism typically expects the patch function to:
 * 1. Return `true` (boolean) to indicate the patch is handled.
 * 2. Return the actual result via a pointer argument (e.g., `bool *p_result` or `void **pp`).
 *
 * Therefore, wrappers are implemented here to adapt the function signatures.
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/devicetree.h>

#include "osif_zephyr.h"
#include "osif_zephyr_impl.h"
#include "mem_types.h"
#include "os_patch.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(osif);

/*
 * ============================================================================
 * Device Tree Configuration Check
 * ============================================================================
 */
#if !DT_NODE_EXISTS(DT_NODELABEL(data_ram_heap))
#error "Devicetree node 'data_ram_heap' is missing! Please define it in your board dts."
#endif

#if !DT_NODE_EXISTS(DT_NODELABEL(buffer_ram_heap))
#error "Devicetree node 'buffer_ram_heap' is missing! Please define it in your board dts."
#endif

/* Memory Regions Definitions */
#define DATA_ON_HEAP_ADDR DT_REG_ADDR(DT_NODELABEL(data_ram_heap))
#define DATA_ON_HEAP_SIZE DT_REG_SIZE(DT_NODELABEL(data_ram_heap))

#define BUFFER_ON_HEAP_ADDR DT_REG_ADDR(DT_NODELABEL(buffer_ram_heap))
#define BUFFER_ON_HEAP_SIZE DT_REG_SIZE(DT_NODELABEL(buffer_ram_heap))

/************************************************************
 * Memory Management (rtl8752h specific implementation)
 ************************************************************/
static struct k_heap data_on_heap;
static struct k_heap buffer_on_heap;

void osif_heap_init_zephyr(void)
{
	k_heap_init(&data_on_heap, (void *)DATA_ON_HEAP_ADDR, DATA_ON_HEAP_SIZE);
	k_heap_init(&buffer_on_heap, (void *)BUFFER_ON_HEAP_ADDR, BUFFER_ON_HEAP_SIZE);
}

struct k_heap *get_heap_by_type(RAM_TYPE ram_type)
{
	switch (ram_type) {
	case RAM_TYPE_DATA_ON:
		return &data_on_heap;
	case RAM_TYPE_BUFFER_ON:
		return &buffer_on_heap;
	default:
		return NULL;
	}
}

struct k_heap *get_heap_by_address(void *ptr)
{
	uintptr_t addr = (uintptr_t)ptr;

	/* Check Buffer RAM range */
	if (addr >= (uintptr_t)BUFFER_ON_HEAP_ADDR &&
	    addr < (uintptr_t)BUFFER_ON_HEAP_ADDR + BUFFER_ON_HEAP_SIZE) {
		return &buffer_on_heap;
	}

	/* Check DATA RAM range */
	if (addr >= (uintptr_t)DATA_ON_HEAP_ADDR &&
	    addr < (uintptr_t)DATA_ON_HEAP_ADDR + DATA_ON_HEAP_SIZE) {
		return &data_on_heap;
	}

	/* Invalid address */
	LOG_ERR("Address %p not in any known heap range", ptr);
	return NULL;
}

static void os_mem_check_heap_usage_zephyr(void)
{
	print_heap_stats("DATA_ON", &data_on_heap.heap, DATA_ON_HEAP_SIZE);
	print_heap_stats("BUFFER_ON", &buffer_on_heap.heap, BUFFER_ON_HEAP_SIZE);
}

/************************************************************
 * Memory Management Wrapper Functions
 ************************************************************/
static bool wrapper_os_mem_alloc_intern(RAM_TYPE ram_type, size_t size, const char *p_func,
					uint32_t file_line, void **pp)
{
	*pp = os_mem_alloc_intern_zephyr(ram_type, size, p_func, file_line);
	return true;
}

static bool wrapper_os_mem_zalloc_intern(RAM_TYPE ram_type, size_t size, const char *p_func,
					 uint32_t file_line, void **pp)
{
	*pp = os_mem_zalloc_intern_zephyr(ram_type, size, p_func, file_line);
	return true;
}

static bool wrapper_os_mem_aligned_alloc_intern(RAM_TYPE ram_type, size_t size, uint8_t alignment,
						const char *p_func, uint32_t file_line, void **pp)
{
	*pp = os_mem_aligned_alloc_intern_zephyr(ram_type, size, alignment, p_func, file_line);
	return true;
}

static bool wrapper_os_mem_free(void *p_block)
{
	os_mem_free_zephyr(p_block);
	return true;
}

static bool wrapper_os_mem_aligned_free(void *p_block)
{
	os_mem_aligned_free_zephyr(p_block);
	return true;
}

static bool wrapper_os_mem_peek(RAM_TYPE ram_type, size_t *p_size)
{
	*p_size = os_mem_peek_zephyr(ram_type);
	return true;
}

/************************************************************
 * Message Queue Wrapper Functions
 ************************************************************/
static bool wrapper_os_msg_queue_create_intern(void **pp_handle, uint32_t msg_num,
					       uint32_t msg_size, const char *p_func,
					       uint32_t file_line, bool *p_result)
{
	*p_result = os_msg_queue_create_intern_zephyr(pp_handle, "MSGQ", msg_num, msg_size, p_func,
						      file_line);
	return true;
}

static bool wrapper_os_msg_queue_delete_intern(void *p_handle, const char *p_func,
					       uint32_t file_line, bool *p_result)
{
	*p_result = os_msg_queue_delete_intern_zephyr(p_handle, p_func, file_line);
	return true;
}

static bool wrapper_os_msg_queue_peek_intern(void *p_handle, uint32_t *p_msg_num,
					     const char *p_func, uint32_t file_line, bool *p_result)
{
	*p_result = os_msg_queue_peek_intern_zephyr(p_handle, p_msg_num, p_func, file_line);
	return true;
}

static bool wrapper_os_msg_send_intern(void *p_handle, void *p_msg, uint32_t wait_ms,
				       const char *p_func, uint32_t file_line, bool *p_result)
{
	*p_result = os_msg_send_intern_zephyr(p_handle, p_msg, wait_ms, p_func, file_line);
	return true;
}

static bool wrapper_os_msg_recv_intern(void *p_handle, void *p_msg, uint32_t wait_ms,
				       const char *p_func, uint32_t file_line, bool *p_result)
{
	*p_result = os_msg_recv_intern_zephyr(p_handle, p_msg, wait_ms, p_func, file_line);
	return true;
}

/************************************************************
 * Scheduler Wrapper Functions
 ************************************************************/
static bool wrapper_os_delay(uint32_t ms)
{
	os_delay_zephyr(ms);
	return true;
}

static bool wrapper_os_sys_time_get(uint64_t *p_time_ms)
{
	*p_time_ms = os_sys_time_get_zephyr();
	return true;
}

static bool wrapper_os_sys_tick_get(uint64_t *p_sys_tick)
{
	*p_sys_tick = os_sys_tick_get_zephyr();
	return true;
}

static bool wrapper_os_sched_start(bool *p_result)
{
	*p_result = os_sched_start_zephyr();
	return true;
}

static bool wrapper_os_sched_suspend(bool *p_result)
{
	*p_result = os_sched_suspend_zephyr();
	return true;
}

static bool wrapper_os_sched_resume(bool *p_result)
{
	*p_result = os_sched_resume_zephyr();
	return true;
}

static bool wrapper_os_sched_state_get(long *p_result)
{
	return os_sched_state_get_zephyr(p_result);
}

static bool wrapper_os_sched_is_start(bool *p_result)
{
	*p_result = os_sched_is_start_zephyr();
	return true;
}

/************************************************************
 * Synchronization Wrapper Functions
 ************************************************************/
static bool wrapper_os_lock(uint32_t *p_flags)
{
	*p_flags = os_lock_zephyr();
	return true;
}

static bool wrapper_os_unlock(uint32_t flags)
{
	os_unlock_zephyr(flags);
	return true;
}

static bool wrapper_os_sem_create(void **pp_handle, const char *p_name, uint32_t init_count,
				  uint32_t max_count, bool *p_result)
{
	*p_result = os_sem_create_zephyr(pp_handle, p_name, init_count, max_count);
	return true;
}

static bool wrapper_os_sem_delete(void *p_handle, bool *p_result)
{
	*p_result = os_sem_delete_zephyr(p_handle);
	return true;
}

static bool wrapper_os_sem_take(void *p_handle, uint32_t wait_ms, bool *p_result)
{
	*p_result = os_sem_take_zephyr(p_handle, wait_ms);
	return true;
}

static bool wrapper_os_sem_give(void *p_handle, bool *p_result)
{
	*p_result = os_sem_give_zephyr(p_handle);
	return true;
}

static bool wrapper_os_mutex_create(void **pp_handle, bool *p_result)
{
	*p_result = os_mutex_create_zephyr(pp_handle);
	return true;
}

static bool wrapper_os_mutex_delete(void *p_handle, bool *p_result)
{
	*p_result = os_mutex_delete_zephyr(p_handle);
	return true;
}

static bool wrapper_os_mutex_take(void *p_handle, uint32_t wait_ms, bool *p_result)
{
	*p_result = os_mutex_take_zephyr(p_handle, wait_ms);
	return true;
}

static bool wrapper_os_mutex_give(void *p_handle, bool *p_result)
{
	*p_result = os_mutex_give_zephyr(p_handle);
	return true;
}

/************************************************************
 * Task Management Wrapper Functions
 ************************************************************/
static bool wrapper_os_task_create(void **pp_handle, const char *p_name, void (*p_routine)(void *),
				   void *p_param, uint16_t stack_size, uint16_t priority,
				   bool *p_is_create_success)
{
	*p_is_create_success =
		os_task_create_zephyr(pp_handle, p_name, p_routine, p_param, stack_size, priority);
	return true;
}

static bool wrapper_os_task_delete(void *p_handle, bool *p_result)
{
	*p_result = os_task_delete_zephyr(p_handle);
	return true;
}

static bool wrapper_os_task_suspend(void *p_handle, bool *p_result)
{
	*p_result = os_task_suspend_zephyr(p_handle);
	return true;
}

static bool wrapper_os_task_resume(void *p_handle, bool *p_result)
{
	*p_result = os_task_resume_zephyr(p_handle);
	return true;
}

static bool wrapper_os_task_yield(bool *p_result)
{
	*p_result = os_task_yield_zephyr();
	return true;
}

static bool wrapper_os_task_handle_get(void **pp_handle, bool *p_result)
{
	*p_result = os_task_handle_get_zephyr(pp_handle);
	return true;
}

static bool wrapper_os_task_priority_get(void *p_handle, uint16_t *p_priority, bool *p_result)
{
	*p_result = os_task_priority_get_zephyr(p_handle, p_priority);
	return true;
}

static bool wrapper_os_task_priority_set(void *p_handle, uint16_t priority, bool *p_result)
{
	*p_result = os_task_priority_set_zephyr(p_handle, priority);
	return true;
}

static bool wrapper_os_task_signal_create(void *p_handle, uint32_t count, bool *p_result)
{
	*p_result = os_task_signal_create_zephyr(p_handle, count);
	return true;
}

static bool wrapper_os_task_notify_take(long xClearCountOnExit, uint32_t xTicksToWait,
					uint32_t *p_notify, bool *p_result)
{
	*p_result = os_task_notify_take_zephyr(xClearCountOnExit, xTicksToWait, p_notify);
	return true;
}

static bool wrapper_os_task_notify_give(void *p_handle, bool *p_result)
{
	*p_result = os_task_notify_give_zephyr(p_handle);
	return true;
}

static bool wrapper_os_task_status_dump(void)
{
	os_task_status_dump_zephyr();
	return true;
}

/************************************************************
 * Timer Wrapper Functions
 ************************************************************/
static bool wrapper_os_timer_create(void **pp_handle, const char *p_timer_name, uint32_t timer_id,
				    uint32_t interval_ms, bool reload, void (*p_timer_callback)(),
				    bool *p_result)
{
	*p_result = os_timer_create_zephyr(pp_handle, p_timer_name, timer_id, interval_ms, reload,
					   p_timer_callback);
	return true;
}

static bool wrapper_os_timer_id_get(void **pp_handle, uint32_t *p_timer_id, bool *p_result)
{
	*p_result = os_timer_id_get_zephyr(pp_handle, p_timer_id);
	return true;
}

static bool wrapper_os_timer_start(void **pp_handle, bool *p_result)
{
	*p_result = os_timer_start_zephyr(pp_handle);
	return true;
}

static bool wrapper_os_timer_restart(void **pp_handle, uint32_t interval_ms, bool *p_result)
{
	*p_result = os_timer_restart_zephyr(pp_handle, interval_ms);
	return true;
}

static bool wrapper_os_timer_stop(void **pp_handle, bool *p_result)
{
	*p_result = os_timer_stop_zephyr(pp_handle);
	return true;
}

static bool wrapper_os_timer_delete(void **pp_handle, bool *p_result)
{
	*p_result = os_timer_delete_zephyr(pp_handle);
	return true;
}

static bool wrapper_os_timer_dump(bool *p_result)
{
	*p_result = os_timer_dump_zephyr();
	return true;
}

static bool wrapper_os_timer_state_get(void **pp_handle, uint32_t *p_timer_state, bool *p_result)
{
	*p_result = os_timer_state_get_zephyr(pp_handle, p_timer_state);
	return true;
}

static bool wrapper_os_timer_is_timer_active(void **pp_handle, bool *is_active)
{
	*is_active = os_timer_is_timer_active_zephyr(pp_handle);
	return true;
}

static bool wrapper_os_timer_init(void)
{
	os_timer_init_zephyr();
	return true;
}

static bool wrapper_os_timer_get_auto_reload(void **pp_handle, long *p_autoreload, bool *p_result)
{
	*p_result = os_timer_get_auto_reload_zephyr(pp_handle, p_autoreload);
	return true;
}

static bool wrapper_os_timer_get_timer_number(void **pp_handle, uint8_t *p_timer_num,
					      bool *p_result)
{
	*p_result = os_timer_get_timer_number_zephyr(pp_handle, p_timer_num);
	return true;
}

/************************************************************
 * Initialize OSIF Patch Function Pointers
 ************************************************************/
void os_zephyr_patch_init(void)
{
	/* Memory Management */
	patch_osif_os_mem_alloc_intern = (BOOL_PATCH_FUNC)wrapper_os_mem_alloc_intern;
	patch_osif_os_mem_zalloc_intern = (BOOL_PATCH_FUNC)wrapper_os_mem_zalloc_intern;
	patch_osif_os_mem_aligned_alloc_intern =
		(BOOL_PATCH_FUNC)wrapper_os_mem_aligned_alloc_intern;
	patch_osif_os_mem_free = (BOOL_PATCH_FUNC)wrapper_os_mem_free;
	patch_osif_os_mem_aligned_free = (BOOL_PATCH_FUNC)wrapper_os_mem_aligned_free;
	patch_osif_os_mem_peek = (BOOL_PATCH_FUNC)wrapper_os_mem_peek;
	patch_osif_os_mem_check_heap_usage = (BOOL_PATCH_FUNC)os_mem_check_heap_usage_zephyr;

	/* Message Queue */
	patch_osif_os_msg_queue_create_intern = (BOOL_PATCH_FUNC)wrapper_os_msg_queue_create_intern;
	patch_osif_os_msg_queue_delete_intern = (BOOL_PATCH_FUNC)wrapper_os_msg_queue_delete_intern;
	patch_osif_os_msg_queue_peek_intern = (BOOL_PATCH_FUNC)wrapper_os_msg_queue_peek_intern;
	patch_osif_os_msg_send_intern = (BOOL_PATCH_FUNC)wrapper_os_msg_send_intern;
	patch_osif_os_msg_recv_intern = (BOOL_PATCH_FUNC)wrapper_os_msg_recv_intern;

	/* Scheduler */
	patch_osif_os_delay = (BOOL_PATCH_FUNC)wrapper_os_delay;
	patch_osif_os_sys_time_get = (BOOL_PATCH_FUNC)wrapper_os_sys_time_get;
	patch_osif_os_sys_tick_get = (BOOL_PATCH_FUNC)wrapper_os_sys_tick_get;
	patch_osif_os_sched_start = (BOOL_PATCH_FUNC)wrapper_os_sched_start;
	patch_osif_os_sched_suspend = (BOOL_PATCH_FUNC)wrapper_os_sched_suspend;
	patch_osif_os_sched_resume = (BOOL_PATCH_FUNC)wrapper_os_sched_resume;
	patch_osif_os_sched_state_get = (BOOL_PATCH_FUNC)wrapper_os_sched_state_get;
	patch_osif_os_sched_is_start = (BOOL_PATCH_FUNC)wrapper_os_sched_is_start;

	/* Synchronization */
	patch_osif_os_lock = (BOOL_PATCH_FUNC)wrapper_os_lock;
	patch_osif_os_unlock = (BOOL_PATCH_FUNC)wrapper_os_unlock;
	patch_osif_os_sem_create = (BOOL_PATCH_FUNC)wrapper_os_sem_create;
	patch_osif_os_sem_delete = (BOOL_PATCH_FUNC)wrapper_os_sem_delete;
	patch_osif_os_sem_take = (BOOL_PATCH_FUNC)wrapper_os_sem_take;
	patch_osif_os_sem_give = (BOOL_PATCH_FUNC)wrapper_os_sem_give;
	patch_osif_os_mutex_create = (BOOL_PATCH_FUNC)wrapper_os_mutex_create;
	patch_osif_os_mutex_delete = (BOOL_PATCH_FUNC)wrapper_os_mutex_delete;
	patch_osif_os_mutex_take = (BOOL_PATCH_FUNC)wrapper_os_mutex_take;
	patch_osif_os_mutex_give = (BOOL_PATCH_FUNC)wrapper_os_mutex_give;

	/* Task Management */
	patch_osif_os_task_create = (BOOL_PATCH_FUNC)wrapper_os_task_create;
	patch_osif_os_task_delete = (BOOL_PATCH_FUNC)wrapper_os_task_delete;
	patch_osif_os_task_suspend = (BOOL_PATCH_FUNC)wrapper_os_task_suspend;
	patch_osif_os_task_resume = (BOOL_PATCH_FUNC)wrapper_os_task_resume;
	patch_osif_os_task_yield = (BOOL_PATCH_FUNC)wrapper_os_task_yield;
	patch_osif_os_task_handle_get = (BOOL_PATCH_FUNC)wrapper_os_task_handle_get;
	patch_osif_os_task_priority_get = (BOOL_PATCH_FUNC)wrapper_os_task_priority_get;
	patch_osif_os_task_priority_set = (BOOL_PATCH_FUNC)wrapper_os_task_priority_set;
	patch_osif_os_task_signal_create = (BOOL_PATCH_FUNC)wrapper_os_task_signal_create;
	patch_osif_os_task_notify_take = (BOOL_PATCH_FUNC)wrapper_os_task_notify_take;
	patch_osif_os_task_notify_give = (BOOL_PATCH_FUNC)wrapper_os_task_notify_give;
	patch_osif_os_task_status_dump = (BOOL_PATCH_FUNC)wrapper_os_task_status_dump;

	/* Timer */
	patch_osif_os_timer_create = (BOOL_PATCH_FUNC)wrapper_os_timer_create;
	patch_osif_os_timer_id_get = (BOOL_PATCH_FUNC)wrapper_os_timer_id_get;
	patch_osif_os_timer_start = (BOOL_PATCH_FUNC)wrapper_os_timer_start;
	patch_osif_os_timer_restart = (BOOL_PATCH_FUNC)wrapper_os_timer_restart;
	patch_osif_os_timer_stop = (BOOL_PATCH_FUNC)wrapper_os_timer_stop;
	patch_osif_os_timer_delete = (BOOL_PATCH_FUNC)wrapper_os_timer_delete;
	patch_osif_os_timer_dump = (BOOL_PATCH_FUNC)wrapper_os_timer_dump;
	patch_osif_os_timer_state_get = (BOOL_PATCH_FUNC)wrapper_os_timer_state_get;
	patch_osif_os_timer_is_timer_active = (BOOL_PATCH_FUNC)wrapper_os_timer_is_timer_active;
	patch_osif_os_timer_init = (BOOL_PATCH_FUNC)wrapper_os_timer_init;
	patch_osif_os_timer_get_auto_reload = (BOOL_PATCH_FUNC)wrapper_os_timer_get_auto_reload;
	patch_osif_os_timer_number_get = (BOOL_PATCH_FUNC)wrapper_os_timer_get_timer_number;

	/* Heap Initialization */
	osif_heap_init_zephyr();
}
