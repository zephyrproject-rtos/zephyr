/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "os_wrapper.h"
#include <zephyr/kernel_structs.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(os_if_task);

struct rtos_task_delete_context {
	struct k_work work;
	k_tid_t thread;
	void *stack;
};

static void deferred_task_delete_handler(struct k_work *work)
{
	struct rtos_task_delete_context *ctx =
		CONTAINER_OF(work, struct rtos_task_delete_context, work);

	k_thread_abort(ctx->thread);

	if (ctx->stack) {
		k_free(ctx->stack);
	}
	k_free(ctx->thread);
	k_free(ctx);
}

int rtos_sched_start(void)
{
	LOG_WRN("Not Support");
	return RTK_SUCCESS;
}

int rtos_sched_stop(void)
{
	LOG_ERR("Not Support");
	return RTK_FAIL;
}

int rtos_sched_suspend(void)
{
	if (k_is_in_isr()) {
		return RTK_FAIL;
	}
	k_sched_lock();
	return RTK_SUCCESS;
}

int rtos_sched_resume(void)
{
	if (k_is_in_isr()) {
		return RTK_FAIL;
	}
	k_sched_unlock();
	return RTK_SUCCESS;
}

int rtos_sched_get_state(void)
{
	if (k_is_pre_kernel()) {
		return RTOS_SCHED_NOT_STARTED;
	}

	if (_current->base.sched_locked != 0U) {
		return RTOS_SCHED_SUSPENDED;
	}

	return RTOS_SCHED_RUNNING;
}

int rtos_task_create(rtos_task_t *pp_handle, const char *p_name, void (*p_routine)(void *),
		     void *p_param, uint16_t stack_size_in_byte, uint16_t priority)
{
	k_tid_t p_thread;
	k_thread_stack_t *p_stack;
	/* higher value, lower priority. */
	int switch_priority = RTOS_TASK_MAX_PRIORITIES - priority;

	if (priority >= RTOS_TASK_MAX_PRIORITIES) {
		return RTK_FAIL;
	}

#if (K_HEAP_MEM_POOL_SIZE > 0)
	p_thread = k_malloc(sizeof(struct k_thread));
	if (p_thread == NULL) {
		return RTK_FAIL;
	}

	/* use k_malloc to avoid k_thread_stack_alloc is empty */
	p_stack = (k_thread_stack_t *)k_malloc(K_KERNEL_STACK_LEN(stack_size_in_byte));
	if (p_stack == NULL) {
		k_free(p_thread);
		LOG_ERR("Alloc stack fail for %s", p_name);
		return RTK_FAIL;
	}
#else
	LOG_ERR("k_malloc not support.");
	return RTK_FAIL;
#endif

	k_thread_create(p_thread, p_stack, stack_size_in_byte, (k_thread_entry_t)p_routine, p_param,
			NULL, NULL, switch_priority, 0, K_FOREVER);
	k_thread_name_set(p_thread, p_name);
#ifdef CONFIG_THREAD_CUSTOM_DATA
	p_thread->custom_data = p_thread;
#endif

	if (pp_handle) {
		*pp_handle = p_thread;
	}

	k_thread_start(p_thread);
	return RTK_SUCCESS;
}

int rtos_task_delete(rtos_task_t p_handle)
{
	k_tid_t p_free = (k_tid_t)p_handle;
	k_tid_t p_curr = k_current_get();
	bool is_self_delete = (p_free == NULL) || (p_curr == p_free);

	if (is_self_delete) {
#if (K_HEAP_MEM_POOL_SIZE > 0)
		struct rtos_task_delete_context *ctx =
			k_malloc(sizeof(struct rtos_task_delete_context));

		if (ctx) {
			ctx->thread = p_curr;
			ctx->stack = (void *)p_curr->stack_info.start;
			k_work_init(&ctx->work, deferred_task_delete_handler);
			k_work_submit(&ctx->work);
		}
#else
		LOG_ERR("k_malloc not support.");
#endif
		k_sleep(K_FOREVER);
		CODE_UNREACHABLE;
	} else {
		k_thread_abort(p_free);
		k_free((void *)p_free->stack_info.start);
		k_free(p_free);
	}

	return RTK_SUCCESS;
}

int rtos_task_suspend(rtos_task_t p_handle)
{
	if (p_handle == NULL) {
		return RTK_FAIL;
	}
	k_thread_suspend((k_tid_t)p_handle);
	return RTK_SUCCESS;
}

int rtos_task_resume(rtos_task_t p_handle)
{
	if (p_handle == NULL) {
		return RTK_FAIL;
	}
	k_thread_resume((k_tid_t)p_handle);
	return RTK_SUCCESS;
}

int rtos_task_yield(void)
{
	if (k_is_in_isr()) {
		return RTK_FAIL;
	}
	k_yield();
	return RTK_SUCCESS;
}

rtos_task_t rtos_task_handle_get(void)
{
	return (rtos_task_t)k_current_get();
}

int rtos_task_priority_set(rtos_task_t p_handle, uint16_t priority)
{
	int switch_priority = RTOS_TASK_MAX_PRIORITIES - priority;

	if (p_handle == NULL) {
		p_handle = k_current_get();
	}

	k_thread_priority_set(p_handle, switch_priority);
	return RTK_SUCCESS;
}

uint32_t rtos_task_priority_get(rtos_task_t p_handle)
{
	if (p_handle == NULL) {
		p_handle = k_current_get();
	}
	int priority = k_thread_priority_get(p_handle);

	return RTOS_TASK_MAX_PRIORITIES - priority;
}

void thread_abort_hook(struct k_thread *p_free)
{
	/* Deferred deletion handles cleanup via workqueue. */
}

static void thread_status_cb(const struct k_thread *thread, void *user_data)
{
	const char *name = k_thread_name_get((k_tid_t)thread);
	int prio = k_thread_priority_get((k_tid_t)thread);
	unsigned int state = thread->base.thread_state;
	size_t unused;

	k_thread_stack_space_get(thread, &unused);

	LOG_INF("  %-16s  prio=%-4d  state=0x%x  stack_free=%u/%u", name ? name : "(anon)", prio,
		state, (unsigned int)unused, (unsigned int)thread->stack_info.size);
}

void rtos_task_out_current_status(void)
{
	LOG_INF("=== Thread Status Dump ===");
	k_thread_foreach(thread_status_cb, NULL);
}

void rtos_create_secure_context(uint32_t size)
{
	/* Zephyr manages TrustZone secure contexts implicitly via kernel.
	 * No manual allocation is needed.
	 */
	(void)size;
}
