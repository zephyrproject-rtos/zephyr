/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file osif_zephyr_patch.c
 * @brief RTL87x2G specific OSIF implementation and patch initialization.
 *
 * This file implements RTL87x2G-specific OSIF interfaces and initializes the
 * OSIF patch function pointers required by RTL87x2G internal modules.
 *
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#include "os_queue.h"
#include "os_msg.h"
#include "os_mem.h"
#include "os_sched.h"
#include "os_sync.h"
#include "os_timer.h"
#include "os_task.h"
#include "mem_types.h"

#include "osif_zephyr.h"
#include "osif_zephyr_impl.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(osif);

/*
 * ============================================================================
 * Device Tree Configuration Check
 * ============================================================================
 */
#if !DT_NODE_EXISTS(DT_NODELABEL(tcm_heap))
#error "Devicetree node 'tcm_heap' is missing! Please define it in your board dts."
#endif

#if !DT_NODE_EXISTS(DT_NODELABEL(buffer_ram))
#error "Devicetree node 'buffer_ram' is missing! Please define it in your board dts."
#endif

/* Memory Regions Definitions */
#define DATA_ON_HEAP_ADDR DT_REG_ADDR(DT_NODELABEL(tcm_heap))
#define DATA_ON_HEAP_SIZE DT_REG_SIZE(DT_NODELABEL(tcm_heap))

#define BUFFER_RAM_ADDR DT_REG_ADDR(DT_NODELABEL(buffer_ram))
#define BUFFER_RAM_SIZE DT_REG_SIZE(DT_NODELABEL(buffer_ram))

#define BUFFER_ON_GLOBAL_SIZE (1024 + 512)
#define BUFFER_ON_HEAP_ADDR   (BUFFER_RAM_ADDR + BUFFER_ON_GLOBAL_SIZE)
#define BUFFER_ON_HEAP_SIZE   (BUFFER_RAM_SIZE - BUFFER_ON_GLOBAL_SIZE)

/************************************************************
 * MEMORY MANAGEMENT
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

void os_mem_peek_printf_zephyr(void)
{
	print_heap_stats("DATA_ON", &data_on_heap.heap, DATA_ON_HEAP_SIZE);
	print_heap_stats("BUFFER_ON", &buffer_on_heap.heap, BUFFER_ON_HEAP_SIZE);
}

/************************************************************
 * OSIF Patch Function Pointer Assignments
 ************************************************************/
void os_zephyr_patch_init(void)
{
	/* Memory Management */
	os_mem_alloc_intern = os_mem_alloc_intern_zephyr;
	os_mem_zalloc_intern = os_mem_zalloc_intern_zephyr;
	os_mem_aligned_alloc_intern = os_mem_aligned_alloc_intern_zephyr;
	os_mem_free = os_mem_free_zephyr;
	os_mem_aligned_free = os_mem_aligned_free_zephyr;
	os_mem_peek = os_mem_peek_zephyr;
	os_mem_peek_printf = os_mem_peek_printf_zephyr;
	os_mem_check_heap_usage = os_mem_peek_printf_zephyr;

	/* Message Queue */
	os_msg_queue_create_intern = os_msg_queue_create_intern_zephyr;
	os_msg_queue_delete_intern = os_msg_queue_delete_intern_zephyr;
	os_msg_queue_peek_intern = os_msg_queue_peek_intern_zephyr;
	os_msg_send_intern = os_msg_send_intern_zephyr;
	os_msg_recv_intern = os_msg_recv_intern_zephyr;

	/* Scheduler */
	os_delay = os_delay_zephyr;
	os_sys_time_get = os_sys_time_get_zephyr;
	os_sys_tick_get = os_sys_tick_get_zephyr;
	os_sched_start = os_sched_start_zephyr;
	os_sched_stop = os_sched_stop_zephyr;
	os_sched_suspend = os_sched_suspend_zephyr;
	os_sched_resume = os_sched_resume_zephyr;
	os_sched_state_get = os_sched_state_get_zephyr;
	os_sched_is_start = os_sched_is_start_zephyr;

	/* Synchronization */
	os_lock = os_lock_zephyr;
	os_unlock = os_unlock_zephyr;
	os_sem_create = os_sem_create_zephyr;
	os_sem_delete = os_sem_delete_zephyr;
	os_sem_take = os_sem_take_zephyr;
	os_sem_give = os_sem_give_zephyr;
	os_mutex_create = os_mutex_create_zephyr;
	os_mutex_delete = os_mutex_delete_zephyr;
	os_mutex_take = os_mutex_take_zephyr;
	os_mutex_give = os_mutex_give_zephyr;

	/* Task Management */
	os_task_create = os_task_create_zephyr;
	os_task_delete = os_task_delete_zephyr;
	os_task_suspend = os_task_suspend_zephyr;
	os_task_resume = os_task_resume_zephyr;
	os_task_yield = os_task_yield_zephyr;
	os_task_handle_get = os_task_handle_get_zephyr;
	os_task_priority_get = os_task_priority_get_zephyr;
	os_task_priority_set = os_task_priority_set_zephyr;
	os_task_signal_send = os_task_signal_send_zephyr;
	os_task_signal_recv = os_task_signal_recv_zephyr;
	os_task_signal_clear = os_task_signal_clear_zephyr;
	os_task_signal_create = os_task_signal_create_zephyr;
	os_task_notify_take = os_task_notify_take_zephyr;
	os_task_notify_give = os_task_notify_give_zephyr;
	os_task_status_dump = os_task_status_dump_zephyr;
	os_alloc_secure_ctx = os_alloc_secure_ctx_zephyr;

	/* Timer */
	os_timer_create = os_timer_create_zephyr;
	os_timer_start = os_timer_start_zephyr;
	os_timer_restart = os_timer_restart_zephyr;
	os_timer_stop = os_timer_stop_zephyr;
	os_timer_delete = os_timer_delete_zephyr;
	os_timer_id_get = os_timer_id_get_zephyr;
	os_timer_is_timer_active = os_timer_is_timer_active_zephyr;
	os_timer_state_get = os_timer_state_get_zephyr;
	os_timer_get_auto_reload = os_timer_get_auto_reload_zephyr;
	os_timer_get_timer_number = os_timer_get_timer_number_zephyr;
	os_timer_dump = os_timer_dump_zephyr;
	os_timer_init = os_timer_init_zephyr;

	/* Heap Initialization */
	osif_heap_init_zephyr();
}
