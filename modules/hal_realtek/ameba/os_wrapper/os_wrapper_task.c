/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "os_wrapper.h"
#include "os_wrapper_deferred.h"
#include <zephyr/kernel_structs.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(os_if_task);

/* Per-thread metadata stored in k_thread.custom_data. */
struct rtos_task_meta {
	void *stack_orig;
};

/* Self-delete reaper on sysworkq; the join makes the k_thread safe to free. */
static void reap_self_deleted_thread(void *stack, void *meta, void *thread)
{
	k_thread_join((k_tid_t)thread, K_FOREVER);
	k_free(stack);
	k_free(meta);
	k_free(thread);
}

int rtos_sched_start(void)
{
	LOG_WRN("%s Not Support", __func__);
	return RTK_SUCCESS;
}

int rtos_sched_stop(void)
{
	LOG_ERR("%s Not Support", __func__);
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
		     void *p_param, size_t stack_size_in_byte, uint16_t priority)
{
	k_tid_t p_thread;
	k_thread_stack_t *p_stack;
	struct rtos_task_meta *meta;
	/* Map p in [0, MAX-1] to (MAX-1) - p so K_IDLE_PRIO is never used. */
	int switch_priority = (RTOS_TASK_MAX_PRIORITIES - 1) - priority;

	if (p_routine == NULL) {
		LOG_ERR("%s: NULL routine", __func__);
		return RTK_FAIL;
	}
	if (priority >= RTOS_TASK_MAX_PRIORITIES) {
		LOG_ERR("%s: priority %u >= max %d", __func__, priority, RTOS_TASK_MAX_PRIORITIES);
		return RTK_FAIL;
	}

#if (K_HEAP_MEM_POOL_SIZE > 0)
	p_thread = k_malloc(sizeof(struct k_thread));
	if (p_thread == NULL) {
		return RTK_FAIL;
	}

	/* The initial SP must be 8-byte aligned (AAPCS); k_malloc may give only 4. */
	p_stack = (k_thread_stack_t *)k_aligned_alloc(ARCH_STACK_PTR_ALIGN,
						      K_KERNEL_STACK_LEN(stack_size_in_byte));
	if (p_stack == NULL) {
		k_free(p_thread);
		LOG_ERR("Alloc stack fail for %s", p_name);
		return RTK_FAIL;
	}

	meta = k_malloc(sizeof(struct rtos_task_meta));
	if (meta == NULL) {
		k_free(p_stack);
		k_free(p_thread);
		return RTK_FAIL;
	}
	meta->stack_orig = p_stack;
#else
	LOG_ERR("%s <<< k_malloc not support. >>>", __func__);
	return RTK_FAIL;
#endif

	k_thread_create(p_thread, p_stack, stack_size_in_byte, (k_thread_entry_t)p_routine, p_param,
			NULL, NULL, switch_priority, 0, K_FOREVER);
	k_thread_name_set(p_thread, p_name);

	p_thread->custom_data = meta;

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
		struct rtos_task_meta *meta = (struct rtos_task_meta *)p_curr->custom_data;

		if (meta != NULL) {
			void *stack_orig = meta->stack_orig;

			p_curr->custom_data = NULL;
			if (deferred_submit(reap_self_deleted_thread, stack_orig, meta,
					    p_curr) != 0) {
				/* We run on the stack to free, so leak it. */
				LOG_ERR("%s: deferred_submit failed, resources leak",
					__func__);
			}
		} else {
			LOG_WRN("%s: self-delete of thread with no wrapper meta",
				__func__);
		}
		k_thread_abort(p_curr);
		CODE_UNREACHABLE;
	} else {
		struct rtos_task_meta *meta = (struct rtos_task_meta *)p_free->custom_data;

		p_free->custom_data = NULL;
		k_thread_abort(p_free);
		if (meta) {
			k_free(meta->stack_orig);
			k_free(meta);
		}
		k_free(p_free);
	}

	return RTK_SUCCESS;
}

int rtos_task_suspend(rtos_task_t p_handle)
{
	k_tid_t tid = (p_handle != NULL) ? (k_tid_t)p_handle : k_current_get();

	k_thread_suspend(tid);
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
	if (priority >= RTOS_TASK_MAX_PRIORITIES) {
		LOG_ERR("%s: priority %u >= max %d", __func__, priority, RTOS_TASK_MAX_PRIORITIES);
		return RTK_FAIL;
	}

	int switch_priority = (RTOS_TASK_MAX_PRIORITIES - 1) - priority;

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
	int rtk_prio = (RTOS_TASK_MAX_PRIORITIES - 1) - priority;

	/* Clamp for coop/idle threads outside [0, MAX-1]. */
	if (rtk_prio < 0) {
		rtk_prio = 0;
	} else if (rtk_prio >= RTOS_TASK_MAX_PRIORITIES) {
		rtk_prio = RTOS_TASK_MAX_PRIORITIES - 1;
	}
	return (uint32_t)rtk_prio;
}

void thread_abort_hook(struct k_thread *p_free)
{
	ARG_UNUSED(p_free);
}

#if defined(CONFIG_THREAD_MONITOR) && defined(CONFIG_INIT_STACKS) &&                               \
	defined(CONFIG_THREAD_STACK_INFO)
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
#endif /* CONFIG_THREAD_MONITOR && CONFIG_INIT_STACKS && CONFIG_THREAD_STACK_INFO */

void rtos_task_out_current_status(void)
{
	LOG_INF("=== Thread Status Dump ===");
#if defined(CONFIG_THREAD_MONITOR) && defined(CONFIG_INIT_STACKS) &&                               \
	defined(CONFIG_THREAD_STACK_INFO)
	k_thread_foreach(thread_status_cb, NULL);
#else
	LOG_WRN("Thread status dump requires CONFIG_THREAD_MONITOR, CONFIG_INIT_STACKS, "
		"CONFIG_THREAD_STACK_INFO");
#endif
}

void rtos_create_secure_context(uint32_t size)
{
	/* Zephyr manages TrustZone contexts implicitly. */
	(void)size;
}
