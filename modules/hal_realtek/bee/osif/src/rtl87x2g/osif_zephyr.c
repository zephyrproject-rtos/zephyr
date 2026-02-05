/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "zephyr/kernel.h"
#include <zephyr/devicetree.h>

#include "os_queue.h"
#include "os_msg.h"
#include "os_mem.h"
#include "os_sched.h"
#include "os_sync.h"
#include "os_timer.h"
#include "os_task.h"

#include "osif_zephyr.h"
#include "mem_types.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(osif);

#ifdef CONFIG_BT
BUILD_ASSERT(CONFIG_NUM_METAIRQ_PRIORITIES == 1,
	     "When CONFIG_BT is enabled, CONFIG_NUM_METAIRQ_PRIORITIES must be set to 1");
#endif

/* Memory Regions Definitions */
#define DATA_ON_HEAP_ADDR DT_REG_ADDR(DT_NODELABEL(tcm_heap))
#define DATA_ON_HEAP_SIZE DT_REG_SIZE(DT_NODELABEL(tcm_heap))

#define BUFFER_RAM_ADDR DT_REG_ADDR(DT_NODELABEL(buffer_ram))
#define BUFFER_RAM_SIZE DT_REG_SIZE(DT_NODELABEL(buffer_ram))

#define BUFFER_ON_GLOBAL_SIZE (1024 + 512)
#define BUFFER_ON_HEAP_ADDR   (BUFFER_RAM_ADDR + BUFFER_ON_GLOBAL_SIZE)
#define BUFFER_ON_HEAP_SIZE   (BUFFER_RAM_SIZE - BUFFER_ON_GLOBAL_SIZE)

/* Helper Macro for Timeout Conversion */
#define OSIF_WAIT_TO_TICKS(ms) ((ms == 0xFFFFFFFFUL) ? K_FOREVER : K_MSEC(ms))

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

static struct k_heap *get_heap_by_type(RAM_TYPE ram_type)
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

void *os_mem_alloc_intern_zephyr(RAM_TYPE ram_type, size_t size, const char *p_func,
				 uint32_t file_line)
{
	struct k_heap *h = get_heap_by_type(ram_type);
	void *ptr;

	if (!h) {
		return NULL;
	}

	ptr = k_heap_alloc(h, size, K_NO_WAIT);
	if (ptr == NULL) {
		LOG_ERR("Mem alloc failed! Type: %d, Size: %zu, Caller: %s:%u", ram_type, size,
			p_func, file_line);
	}
	return ptr;
}

void *os_mem_zalloc_intern_zephyr(RAM_TYPE ram_type, size_t size, const char *p_func,
				  uint32_t file_line)
{
	void *ptr = os_mem_alloc_intern_zephyr(ram_type, size, p_func, file_line);

	if (ptr) {
		memset(ptr, 0, size);
	}
	return ptr;
}

void *os_mem_aligned_alloc_intern_zephyr(RAM_TYPE ram_type, size_t size, uint8_t alignment,
					 const char *p_func, uint32_t file_line)
{
	struct k_heap *h = get_heap_by_type(ram_type);
	void *ptr;

	if (!h) {
		return NULL;
	}

	ptr = k_heap_aligned_alloc(h, alignment, size, K_NO_WAIT);
	if (ptr == NULL) {
		LOG_ERR("Aligned alloc failed! Type: %d, Caller: %s:%u", ram_type, p_func,
			file_line);
	}
	return ptr;
}

void os_mem_free_zephyr(void *block)
{
	if (block == NULL) {
		return;
	}

	/* Check pointer range to decide which heap to free from */
	if ((uintptr_t)block >= BUFFER_ON_HEAP_ADDR &&
	    (uintptr_t)block < BUFFER_ON_HEAP_ADDR + BUFFER_ON_HEAP_SIZE) {
		k_heap_free(&buffer_on_heap, block);
	} else if ((uintptr_t)block >= DATA_ON_HEAP_ADDR &&
		   (uintptr_t)block < DATA_ON_HEAP_ADDR + DATA_ON_HEAP_SIZE) {
		k_heap_free(&data_on_heap, block);
	} else {
		LOG_ERR("Invalid pointer free attempt: %p", block);
	}
}

void os_mem_aligned_free_zephyr(void *block)
{
	os_mem_free_zephyr(block);
}

size_t os_mem_peek_zephyr(RAM_TYPE ram_type)
{
#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	struct sys_memory_stats stats;
	struct k_heap *h = get_heap_by_type(ram_type);

	if (h) {
		sys_heap_runtime_stats_get(&h->heap, &stats);
		return stats.free_bytes;
	}
	LOG_ERR("Invalid heap type");
	return 0;
#else
	ARG_UNUSED(ram_type);
	LOG_WRN_ONCE("Cannot peek heap usage: CONFIG_SYS_HEAP_RUNTIME_STATS is disabled.");
	return 0;
#endif
}

static void print_heap_stats(const char *heap_name, struct sys_heap *heap, size_t capacity)
{
#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	struct sys_memory_stats stats;

	sys_heap_runtime_stats_get(heap, &stats);
	LOG_INF("[%s] Cap: %zu | Alloc: %zu | Free: %zu | MaxAlloc: %zu", heap_name, capacity,
		stats.allocated_bytes, stats.free_bytes, stats.max_allocated_bytes);
#else
	ARG_UNUSED(heap);
	LOG_INF("[%s] Cap: %zu | Stats: N/A (Enable CONFIG_SYS_HEAP_RUNTIME_STATS)", heap_name,
		capacity);
#endif
}

void os_mem_peek_printf_zephyr(void)
{
	print_heap_stats("DATA_ON", &data_on_heap.heap, DATA_ON_HEAP_SIZE);
	print_heap_stats("BUFFER_ON", &buffer_on_heap.heap, BUFFER_ON_HEAP_SIZE);
}

/************************************************************
 * SCHEDULER
 ************************************************************/
void os_delay_zephyr(uint32_t ms)
{
	k_sleep(K_MSEC(ms));
}

uint64_t os_sys_time_get_zephyr(void)
{
	return (uint64_t)k_uptime_get();
}

uint64_t os_sys_tick_get_zephyr(void)
{
	return (uint64_t)k_uptime_ticks();
}

bool os_sched_start_zephyr(void)
{
	__ASSERT(false, "Scheduler start is not supported in Zephyr!");
	return false;
}

bool os_sched_stop_zephyr(void)
{
	__ASSERT(false, "Scheduler stop is not supported in Zephyr!");
	return false;
}

bool os_sched_suspend_zephyr(void)
{
	if (k_is_in_isr()) {
		return false;
	}
	k_sched_lock();
	return true;
}

bool os_sched_resume_zephyr(void)
{
	if (k_is_in_isr()) {
		return false;
	}
	k_sched_unlock();
	return true;
}

bool os_sched_state_get_zephyr(long *p_state)
{
	if (k_is_pre_kernel()) {
		*p_state = OSIF_SCHEDULER_NOT_STARTED;
	} else if (_current->base.sched_locked != 0U) {
		*p_state = OSIF_SCHEDULER_SUSPENDED;
	} else {
		*p_state = OSIF_SCHEDULER_RUNNING;
	}
	return true;
}

bool os_sched_is_start_zephyr(void)
{
	return k_is_pre_kernel() ? false : true;
}

/************************************************************
 * TASK / THREAD MANAGEMENT
 ************************************************************/
K_MEM_SLAB_DEFINE(osif_task_slab, sizeof(struct osif_task), CONFIG_REALTEK_BEE_OSIF_TASK_MAX_COUNT,
		  4);

bool os_task_create_zephyr(void **handle_ptr, const char *name, void (*routine)(void *),
			   void *param, uint16_t stack_size, uint16_t priority)
{
	struct osif_task *task;
	k_thread_stack_t *stack_buffer;
	int zephyr_prio;
	k_timeout_t startup_delay = K_NO_WAIT;
	k_tid_t thread_id;

	if (priority > OSIF_TASK_MAX_PRIORITY || priority < OSIF_TASK_MIN_PRIORITY) {
		LOG_ERR("Invalid priority. OSIF Task Priority is expected to %d ~ %d.",
			OSIF_TASK_MIN_PRIORITY, OSIF_TASK_MAX_PRIORITY);
		return false;
	}

	if (k_mem_slab_alloc(&osif_task_slab, (void **)&task, K_NO_WAIT) != 0) {
		LOG_ERR("Exceeded max number of tasks: %d!",
			CONFIG_REALTEK_BEE_OSIF_TASK_MAX_COUNT);
		return false;
	}
	memset(task, 0, sizeof(struct osif_task));

	/* Dynamic stack allocation */
	stack_buffer = (k_thread_stack_t *)k_heap_aligned_alloc(
		&data_on_heap, Z_KERNEL_STACK_OBJ_ALIGN, stack_size, K_NO_WAIT);

	if (stack_buffer == NULL) {
		k_mem_slab_free(&osif_task_slab, (void *)task);
		LOG_ERR("Alloc thread stack failed (heap full)");
		return false;
	}

	task->stack_start = stack_buffer;

	k_poll_signal_init(&task->poll_signal);
	k_poll_event_init(&task->poll_event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &task->poll_signal);
	task->signal_results = 0U;

	/*
	 * Priority Handling:
	 * low_stack_task requires higher priority (Meta-IRQ) to preempt cooperative threads.
	 */
	if (name && strcmp(name, "low_stack_task") == 0) {
		zephyr_prio = K_HIGHEST_THREAD_PRIO;
		startup_delay = K_MSEC(10);
	} else {
		zephyr_prio = OSIF_TASK_MAX_PRIORITY - priority;
	}

	thread_id =
		k_thread_create(&task->zthread, stack_buffer, stack_size, (k_thread_entry_t)routine,
				param, NULL, NULL, zephyr_prio, 0, startup_delay);

	*handle_ptr = task;

	if (thread_id != NULL && name != NULL) {
		k_thread_name_set(thread_id, name);
	}

	return true;
}

void osif_task_delete_handler(struct k_work *work)
{
	struct osif_task_delete_context *ctx =
		CONTAINER_OF(work, struct osif_task_delete_context, work);

	k_thread_abort(&ctx->task_to_free->zthread);

	if (ctx->stack_to_free) {
		k_heap_free(&data_on_heap, ctx->stack_to_free);
	}

	if (ctx->task_to_free) {
		k_mem_slab_free(&osif_task_slab, (void *)ctx->task_to_free);
	}

	k_heap_free(&data_on_heap, ctx);
}

bool os_task_delete_zephyr(void *handle)
{
	struct osif_task *task;
	k_tid_t current_tid = k_current_get();
	bool is_self_delete = false;

	if (handle == NULL) {
		task = CONTAINER_OF(current_tid, struct osif_task, zthread);
		is_self_delete = true;
	} else {
		task = (struct osif_task *)handle;
		if (&task->zthread == current_tid) {
			is_self_delete = true;
		}
	}

	if (is_self_delete) {
		struct osif_task_delete_context *ctx = k_heap_alloc(
			&data_on_heap, sizeof(struct osif_task_delete_context), K_NO_WAIT);

		if (!ctx) {
			k_thread_abort(current_tid);
			return true;
		}

		ctx->task_to_free = task;
		ctx->stack_to_free = task->stack_start;

		k_work_init(&ctx->work, osif_task_delete_handler);

		k_work_submit(&ctx->work);

		k_sleep(K_FOREVER);

		return true;

	} else {
		k_thread_abort(&task->zthread);

		if (task->stack_start) {
			k_heap_free(&data_on_heap, task->stack_start);
		}
		k_mem_slab_free(&osif_task_slab, (void *)task);

		return true;
	}
}

bool os_task_suspend_zephyr(void *handle)
{
	struct osif_task *task = (struct osif_task *)handle;

	if (!task) {
		return false;
	}

	k_thread_suspend(&task->zthread);
	return true;
}

bool os_task_resume_zephyr(void *handle)
{
	struct osif_task *task = (struct osif_task *)handle;

	if (!task) {
		return false;
	}

	k_thread_resume(&task->zthread);
	return true;
}

bool os_task_yield_zephyr(void)
{
	if (k_is_in_isr()) {
		return false;
	}
	k_yield();
	return true;
}

bool os_task_handle_get_zephyr(void **handle_ptr)
{
	k_tid_t thread;

	if (!handle_ptr) {
		return false;
	}

	thread = k_current_get();
	*handle_ptr = CONTAINER_OF(thread, struct osif_task, zthread);
	return true;
}

bool os_task_priority_get_zephyr(void *handle, uint16_t *priority)
{
	struct osif_task *task = (struct osif_task *)handle;
	k_tid_t thread;
	int zephyr_prio;

	if (!priority) {
		return false;
	}

	thread = task ? &task->zthread : k_current_get();
	zephyr_prio = k_thread_priority_get(thread);
	*priority = (uint16_t)(OSIF_TASK_MAX_PRIORITY - zephyr_prio);

	return true;
}

bool os_task_priority_set_zephyr(void *handle, uint16_t priority)
{
	struct osif_task *task = (struct osif_task *)handle;
	k_tid_t thread;
	int switch_priority;

	if (priority > OSIF_TASK_MAX_PRIORITY || priority < OSIF_TASK_MIN_PRIORITY) {
		return false;
	}

	thread = task ? &task->zthread : k_current_get();
	switch_priority = OSIF_TASK_MAX_PRIORITY - priority;
	k_thread_priority_set(thread, switch_priority);

	return true;
}

bool os_task_signal_create_zephyr(void *handle, uint32_t count)
{
	ARG_UNUSED(count);
	ARG_UNUSED(handle);
	return true;
}

bool os_task_notify_take_zephyr(long clear_count_on_exit, uint32_t wait_ticks, uint32_t *notify)
{
	k_tid_t thread;
	struct osif_task *task;
	int retval;
	int key;

	if (notify == NULL) {
		LOG_ERR("%s: input notify pointer cannot be NULL!", __func__);
		return false;
	}

	if (k_is_in_isr()) {
		LOG_ERR("%s cannot be called in ISR!", __func__);
		return false;
	}

	thread = k_current_get();
	task = CONTAINER_OF(thread, struct osif_task, zthread);

	retval = k_poll(&task->poll_event, 1, OSIF_WAIT_TO_TICKS(wait_ticks));

	if (retval == -EAGAIN) {
		return false; /* Timeout */
	} else if (retval != 0) {
		LOG_ERR("k_poll error: %d", retval);
		return false;
	}

	__ASSERT(task->poll_event.state == K_POLL_STATE_SIGNALED, "event state not signaled!");
	__ASSERT(task->poll_event.signal->signaled == 1, "event signaled is not 1");

	key = irq_lock();
	/* Reset the states to facilitate the next trigger */
	task->poll_event.signal->signaled = 0;
	task->poll_event.state = K_POLL_STATE_NOT_READY;
	*notify = task->signal_results;

	/* Clear signal flags as the thread is ready now */
	if (clear_count_on_exit == true) {
		task->signal_results = 0;
	}
	irq_unlock(key);

	return true;
}

bool os_task_notify_give_zephyr(void *handle)
{
	struct osif_task *task = (struct osif_task *)handle;
	int key;

	if (task == NULL) {
		LOG_ERR("%s: Target task handle is a NULL pointer!", __func__);
		return false;
	}

	key = irq_lock();
	task->signal_results |= 0x1;
	irq_unlock(key);

	k_poll_signal_raise(&task->poll_signal, 0x1);

	return true;
}

/* Stubs */
void os_task_status_dump_zephyr(void)
{
}

bool os_alloc_secure_ctx_zephyr(uint32_t stack_size)
{
	/*
	 * Zephyr manages TrustZone secure contexts implicitly via kernel.
	 * No manual allocation is needed for the non-secure thread.
	 */
	(void)stack_size;

	return true;
}

bool os_task_signal_send_zephyr(void *handle, uint32_t signal)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(signal);
	__ASSERT(false, "os_task_signal_send() is invalid in Zephyr");
	return false;
}

bool os_task_signal_recv_zephyr(uint32_t *p_signal, uint32_t wait_ms)
{
	ARG_UNUSED(p_signal);
	ARG_UNUSED(wait_ms);
	__ASSERT(false, "os_task_signal_recv() is invalid in Zephyr");
	return false;
}

bool os_task_signal_clear_zephyr(void *handle)
{
	ARG_UNUSED(handle);
	__ASSERT(false, "os_task_signal_clear() is invalid in Zephyr");
	return false;
}

/************************************************************
 * CRITICAL SECTION
 ************************************************************/
uint32_t os_lock_zephyr(void)
{
	return irq_lock();
}

void os_unlock_zephyr(uint32_t flags)
{
	irq_unlock(flags);
}

/************************************************************
 * SEMAPHORE & MUTEX
 ************************************************************/
K_MEM_SLAB_DEFINE(osif_sem_slab, sizeof(struct k_sem), CONFIG_REALTEK_BEE_OSIF_SEM_MAX_COUNT, 4);

bool os_sem_create_zephyr(void **handle_ptr, const char *name, uint32_t init_count,
			  uint32_t max_count)
{
	struct k_sem *sem;

	ARG_UNUSED(name);

	if (k_mem_slab_alloc(&osif_sem_slab, (void **)&sem, K_NO_WAIT) != 0) {
		LOG_ERR("Exceeded max number of semaphores: %d!",
			CONFIG_REALTEK_BEE_OSIF_SEM_MAX_COUNT);
		return false;
	}

	memset(sem, 0, sizeof(struct k_sem));
	k_sem_init(sem, init_count, max_count);
	*handle_ptr = sem;
	return true;
}

bool os_sem_delete_zephyr(void *handle)
{
	if (!handle) {
		return false;
	}
	k_mem_slab_free(&osif_sem_slab, handle);
	return true;
}

bool os_sem_take_zephyr(void *handle, uint32_t wait_ms)
{
	if (!handle) {
		return false;
	}
	return k_sem_take((struct k_sem *)handle, OSIF_WAIT_TO_TICKS(wait_ms)) == 0;
}

bool os_sem_give_zephyr(void *handle)
{
	struct k_sem *sem = (struct k_sem *)handle;

	if (!sem) {
		return false;
	}

	if (k_sem_count_get(sem) >= sem->limit) {
		LOG_WRN("Sem give ignored: limit reached");
		return false;
	}
	k_sem_give(sem);
	return true;
}

K_MEM_SLAB_DEFINE(osif_mutex_slab, sizeof(struct k_mutex), CONFIG_REALTEK_BEE_OSIF_MUTEX_MAX_COUNT,
		  4);

bool os_mutex_create_zephyr(void **handle_ptr)
{
	struct k_mutex *mutex;

	if (k_mem_slab_alloc(&osif_mutex_slab, (void **)&mutex, K_NO_WAIT) != 0) {
		LOG_ERR("Exceeded max number of mutexs: %d!",
			CONFIG_REALTEK_BEE_OSIF_MUTEX_MAX_COUNT);
		return false;
	}

	memset(mutex, 0, sizeof(struct k_mutex));
	k_mutex_init(mutex);
	*handle_ptr = mutex;
	return true;
}

bool os_mutex_delete_zephyr(void *handle)
{
	if (!handle) {
		return false;
	}
	k_mem_slab_free(&osif_mutex_slab, handle);
	return true;
}

bool os_mutex_take_zephyr(void *handle, uint32_t wait_ms)
{
	if (!handle) {
		return false;
	}
	return k_mutex_lock((struct k_mutex *)handle, OSIF_WAIT_TO_TICKS(wait_ms)) == 0;
}

bool os_mutex_give_zephyr(void *handle)
{
	if (!handle) {
		return false;
	}
	return k_mutex_unlock((struct k_mutex *)handle) == 0;
}

/************************************************************
 * MESSAGE QUEUE
 ************************************************************/
K_MEM_SLAB_DEFINE(osif_msgq_slab, sizeof(struct k_msgq), CONFIG_REALTEK_BEE_OSIF_MSGQ_MAX_COUNT, 4);

bool os_msg_queue_create_intern_zephyr(void **handle_ptr, const char *name, uint32_t msg_num,
				       uint32_t msg_size, const char *func, uint32_t line)
{
	struct k_msgq *msgq;
	void *queue_buffer;
	size_t total_size = msg_size * msg_num;

	ARG_UNUSED(name);
	ARG_UNUSED(func);
	ARG_UNUSED(line);

	if (handle_ptr == NULL) {
		return false;
	}

	if (k_mem_slab_alloc(&osif_msgq_slab, (void **)&msgq, K_NO_WAIT) != 0) {
		LOG_ERR("Exceeded max number of msgqs: %d!",
			CONFIG_REALTEK_BEE_OSIF_MSGQ_MAX_COUNT);
		return false;
	}
	memset(msgq, 0, sizeof(struct k_msgq));

	queue_buffer = k_heap_aligned_alloc(&data_on_heap, 4, total_size, K_NO_WAIT);
	if (!queue_buffer) {
		k_mem_slab_free(&osif_msgq_slab, (void *)msgq);
		LOG_ERR("alloc queue buffer failed");
		return false;
	}

	k_msgq_init(msgq, queue_buffer, msg_size, msg_num);
	msgq->flags = K_MSGQ_FLAG_ALLOC;
	*handle_ptr = msgq;
	return true;
}

bool os_msg_queue_delete_intern_zephyr(void *handle, const char *func, uint32_t line)
{
	struct k_msgq *msgq = (struct k_msgq *)handle;

	ARG_UNUSED(func);
	ARG_UNUSED(line);

	if (!msgq) {
		return false;
	}

	if ((msgq->flags & K_MSGQ_FLAG_ALLOC) != 0U) {
		k_heap_free(&data_on_heap, msgq->buffer_start);
		msgq->buffer_start = NULL;
		msgq->flags &= ~K_MSGQ_FLAG_ALLOC;
		k_mem_slab_free(&osif_msgq_slab, (void *)msgq);
		return true;
	}
	return false;
}

bool os_msg_queue_peek_intern_zephyr(void *handle, uint32_t *msg_num, const char *func,
				     uint32_t line)
{
	ARG_UNUSED(func);
	ARG_UNUSED(line);

	if (handle) {
		*msg_num = k_msgq_num_used_get((struct k_msgq *)handle);
		return true;
	}
	return false;
}

bool os_msg_send_intern_zephyr(void *handle, void *msg, uint32_t wait_ms, const char *func,
			       uint32_t line)
{
	ARG_UNUSED(func);
	ARG_UNUSED(line);

	if (!handle) {
		return false;
	}
	return k_msgq_put((struct k_msgq *)handle, msg, OSIF_WAIT_TO_TICKS(wait_ms)) == 0;
}

bool os_msg_recv_intern_zephyr(void *handle, void *msg, uint32_t wait_ms, const char *func,
			       uint32_t line)
{
	ARG_UNUSED(func);
	ARG_UNUSED(line);

	if (!handle) {
		return false;
	}
	return k_msgq_get((struct k_msgq *)handle, msg, OSIF_WAIT_TO_TICKS(wait_ms)) == 0;
}

/************************************************************
 * SOFTWARE TIMER
 ************************************************************/
static struct osif_timer osif_timer_pool[CONFIG_REALTEK_BEE_OSIF_TIMER_MAX_COUNT];

bool os_timer_create_zephyr(void **handle_ptr, const char *timer_name, uint32_t timer_id,
			    uint32_t interval_ms, bool reload, void (*timer_callback)())
{
	struct osif_timer *timer = NULL;
	uint32_t key;
	int i;

	if (reload != OSIF_TIMER_ONCE && reload != OSIF_TIMER_PERIODIC) {
		return false;
	}

	key = irq_lock();
	for (i = 0; i < CONFIG_REALTEK_BEE_OSIF_TIMER_MAX_COUNT; i++) {
		if (!osif_timer_pool[i].allocated) {
			timer = &osif_timer_pool[i];
			timer->allocated = true;
			break;
		}
	}
	irq_unlock(key);

	if (timer == NULL) {
		LOG_ERR("Exceeded max number of timers: %d!",
			CONFIG_REALTEK_BEE_OSIF_TIMER_MAX_COUNT);
		*handle_ptr = NULL;
		return false;
	}

	timer->name = timer_name;
	timer->timer_id = timer_id;
	timer->interval_ms = interval_ms;
	timer->status = OSIF_TIMER_NOT_ACTIVE;
	timer->type = (uint8_t)reload;

	k_timer_init(&timer->ztimer, (k_timer_expiry_t)timer_callback, NULL);

	*handle_ptr = timer;

	return true;
}

bool os_timer_start_zephyr(void **handle_ptr)
{
	struct osif_timer *timer;
	k_timeout_t period;

	if (!handle_ptr || !*handle_ptr) {
		LOG_ERR("%s: Invalid timer handle (NULL)", __func__);
		return false;
	}

	timer = (struct osif_timer *)*handle_ptr;
	period = (timer->type == OSIF_TIMER_PERIODIC) ? K_MSEC(timer->interval_ms) : K_NO_WAIT;

	k_timer_start(&timer->ztimer, K_MSEC(timer->interval_ms), period);
	timer->status = OSIF_TIMER_ACTIVE;

	return true;
}

bool os_timer_restart_zephyr(void **handle_ptr, uint32_t interval_ms)
{
	struct osif_timer *timer;
	k_timeout_t period;
	uint32_t key;

	if (!handle_ptr || !*handle_ptr) {
		LOG_ERR("%s: Invalid timer handle (NULL)", __func__);
		return false;
	}

	timer = (struct osif_timer *)*handle_ptr;

	key = irq_lock();
	timer->interval_ms = interval_ms;
	irq_unlock(key);

	period = (timer->type == OSIF_TIMER_PERIODIC) ? K_MSEC(interval_ms) : K_NO_WAIT;
	k_timer_start(&timer->ztimer, K_MSEC(interval_ms), period);

	timer->status = OSIF_TIMER_ACTIVE;

	return true;
}

bool os_timer_stop_zephyr(void **handle_ptr)
{
	struct osif_timer *timer;

	if (!handle_ptr || !*handle_ptr) {
		return false;
	}

	timer = (struct osif_timer *)*handle_ptr;

	if (timer->status == OSIF_TIMER_NOT_ACTIVE) {
		return false;
	}

	k_timer_stop(&timer->ztimer);
	timer->status = OSIF_TIMER_NOT_ACTIVE;
	return true;
}

bool os_timer_delete_zephyr(void **handle_ptr)
{
	struct osif_timer *timer;
	uint32_t key;

	if (!handle_ptr || !*handle_ptr) {
		return false;
	}
	timer = (struct osif_timer *)*handle_ptr;

	k_timer_stop(&timer->ztimer);

	key = irq_lock();
	timer->status = OSIF_TIMER_NOT_ACTIVE;
	timer->allocated = false;
	irq_unlock(key);

	*handle_ptr = NULL;

	return true;
}

bool os_timer_id_get_zephyr(void **handle_ptr, uint32_t *timer_id)
{
	struct osif_timer *timer;

	if (!handle_ptr || !*handle_ptr) {
		return false;
	}

	timer = (struct osif_timer *)*handle_ptr;
	*timer_id = timer->timer_id;
	return true;
}

bool os_timer_is_timer_active_zephyr(void **handle_ptr)
{
	struct osif_timer *timer;

	if (!handle_ptr || !*handle_ptr) {
		return false;
	}

	timer = (struct osif_timer *)*handle_ptr;
	return (k_timer_remaining_get(&timer->ztimer) > 0);
}

bool os_timer_state_get_zephyr(void **handle_ptr, uint32_t *timer_state)
{
	struct osif_timer *timer;

	if (!handle_ptr || !*handle_ptr) {
		return false;
	}

	timer = (struct osif_timer *)*handle_ptr;
	*timer_state = (k_timer_remaining_get(&timer->ztimer) > 0) ? OSIF_TIMER_ACTIVE
								   : OSIF_TIMER_NOT_ACTIVE;
	return true;
}

bool os_timer_get_auto_reload_zephyr(void **handle_ptr, long *autoreload)
{
	struct osif_timer *timer;

	if (!handle_ptr || !*handle_ptr) {
		return false;
	}
	timer = (struct osif_timer *)*handle_ptr;

	*autoreload = timer->type;
	return true;
}

bool os_timer_get_timer_number_zephyr(void **handle_ptr, uint8_t *timer_num)
{
	struct osif_timer *timer;
	long index;

	if (!handle_ptr || !*handle_ptr) {
		return false;
	}

	timer = (struct osif_timer *)*handle_ptr;

	index = timer - osif_timer_pool;

	if (index >= 0 && index < CONFIG_REALTEK_BEE_OSIF_TIMER_MAX_COUNT) {
		*timer_num = (uint8_t)index;
		return true;
	}

	return false;
}

bool os_timer_dump_zephyr(void)
{
	__ASSERT(false, "os_timer_dump() is invalid in Zephyr");
	return false;
}

void os_timer_init_zephyr(void)
{
	__ASSERT(false, "os_timer_init() is invalid in Zephyr");
}

/*
 * OSIF PATCH Function Assignments
 */
void osif_mem_func_init_zephyr(void)
{
	os_mem_alloc_intern = os_mem_alloc_intern_zephyr;
	os_mem_zalloc_intern = os_mem_zalloc_intern_zephyr;
	os_mem_aligned_alloc_intern = os_mem_aligned_alloc_intern_zephyr;
	os_mem_free = os_mem_free_zephyr;
	os_mem_aligned_free = os_mem_aligned_free_zephyr;
	os_mem_peek = os_mem_peek_zephyr;
	os_mem_peek_printf = os_mem_peek_printf_zephyr;
	os_mem_check_heap_usage = os_mem_peek_printf_zephyr;
}

void osif_msg_func_init_zephyr(void)
{
	os_msg_queue_create_intern = os_msg_queue_create_intern_zephyr;
	os_msg_queue_delete_intern = os_msg_queue_delete_intern_zephyr;
	os_msg_queue_peek_intern = os_msg_queue_peek_intern_zephyr;
	os_msg_send_intern = os_msg_send_intern_zephyr;
	os_msg_recv_intern = os_msg_recv_intern_zephyr;
}

void osif_sched_func_init_zephyr(void)
{
	os_delay = os_delay_zephyr;
	os_sys_time_get = os_sys_time_get_zephyr;
	os_sys_tick_get = os_sys_tick_get_zephyr;
	os_sched_start = os_sched_start_zephyr;
	os_sched_stop = os_sched_stop_zephyr;
	os_sched_suspend = os_sched_suspend_zephyr;
	os_sched_resume = os_sched_resume_zephyr;
	os_sched_state_get = os_sched_state_get_zephyr;
	os_sched_is_start = os_sched_is_start_zephyr;
}

void osif_sync_func_init_zephyr(void)
{
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
}

void osif_task_func_init_zephyr(void)
{
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
}

void osif_timer_func_init_zephyr(void)
{
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
}

void os_zephyr_patch_init(void)
{
	osif_mem_func_init_zephyr();
	osif_msg_func_init_zephyr();
	osif_sched_func_init_zephyr();
	osif_sync_func_init_zephyr();
	osif_task_func_init_zephyr();
	osif_timer_func_init_zephyr();

	osif_heap_init_zephyr();
}
