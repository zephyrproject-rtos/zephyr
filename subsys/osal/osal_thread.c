/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/osal/osal.h>
#include <zephyr/logging/log.h>

#include "osal_priv.h"

LOG_MODULE_DECLARE(osal, CONFIG_OSAL_LOG_LEVEL);

/*
 * Foreign entry points take a single void* argument, while Zephyr threads take
 * three. The control block carries the real entry and arg so a trampoline can
 * bridge the two. The stack pointer is kept for cleanup.
 */
struct osal_thread_cb {
	struct k_thread thread;
	k_thread_stack_t *stack;
	osal_thread_entry_t entry;
	void *arg;
#ifndef CONFIG_OSAL_DYNAMIC_ALLOC
	int slot; /* index into the static stack pool */
#endif
};

#ifndef CONFIG_OSAL_DYNAMIC_ALLOC
/*
 * Static path: a fixed pool of pre-declared stacks plus a control-block slab.
 * A free bitmap tracks stack-slot ownership under a spinlock.
 */
#define OSAL_THREAD_CB_SZ ROUND_UP(sizeof(struct osal_thread_cb), sizeof(void *))

K_THREAD_STACK_ARRAY_DEFINE(osal_stack_pool, CONFIG_OSAL_MAX_THREADS,
			    CONFIG_OSAL_THREAD_STACK_SIZE);
K_MEM_SLAB_DEFINE(osal_thread_slab, OSAL_THREAD_CB_SZ,
		  CONFIG_OSAL_MAX_THREADS, sizeof(void *));
static ATOMIC_DEFINE(osal_stack_used, CONFIG_OSAL_MAX_THREADS);

static int stack_slot_take(void)
{
	for (int i = 0; i < CONFIG_OSAL_MAX_THREADS; i++) {
		if (!atomic_test_and_set_bit(osal_stack_used, i)) {
			return i;
		}
	}
	return -1;
}

static void stack_slot_give(int slot)
{
	atomic_clear_bit(osal_stack_used, slot);
}
#endif

static void thread_trampoline(void *p1, void *p2, void *p3)
{
	struct osal_thread_cb *t = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	t->entry(t->arg);
}

/*
 * Foreign priorities are typically "higher number == higher priority".
 * Map onto Zephyr preemptive priorities where lower == higher, clamped
 * into the valid range.
 */
static int map_priority(int foreign_prio)
{
	int prio = CONFIG_NUM_PREEMPT_PRIORITIES - 1 - foreign_prio;

	if (prio < 0) {
		prio = 0;
	}
	if (prio > CONFIG_NUM_PREEMPT_PRIORITIES - 1) {
		prio = CONFIG_NUM_PREEMPT_PRIORITIES - 1;
	}

	return prio;
}

/*
 * Allocate the control block and a stack for the requested size, per mode.
 * On success *eff_size is set to the usable stack size to pass to
 * k_thread_create: the requested size in dynamic mode, the pool slot's
 * usable size in static mode (the static buffer is fixed-size).
 */
static struct osal_thread_cb *thread_alloc(uint32_t stack_size, size_t *eff_size)
{
	struct osal_thread_cb *t;

#ifdef CONFIG_OSAL_DYNAMIC_ALLOC
	t = osal_cb_alloc(sizeof(struct osal_thread_cb));
	if (t == NULL) {
		return NULL;
	}
	t->stack = k_thread_stack_alloc(stack_size, 0);
	if (t->stack == NULL) {
		osal_cb_free(t);
		return NULL;
	}
	*eff_size = stack_size;
#else
	int slot;

	if (stack_size > K_THREAD_STACK_SIZEOF(osal_stack_pool[0])) {
		LOG_ERR("stack size %u exceeds static pool slot %zu", stack_size,
			(size_t)K_THREAD_STACK_SIZEOF(osal_stack_pool[0]));
		return NULL;
	}
	if (k_mem_slab_alloc(&osal_thread_slab, (void **)&t, K_NO_WAIT) != 0) {
		return NULL;
	}
	slot = stack_slot_take();
	if (slot < 0) {
		k_mem_slab_free(&osal_thread_slab, t);
		return NULL;
	}
	t->slot = slot;
	t->stack = osal_stack_pool[slot];
	*eff_size = K_THREAD_STACK_SIZEOF(osal_stack_pool[slot]);
#endif

	return t;
}

static void thread_free(struct osal_thread_cb *t)
{
#ifdef CONFIG_OSAL_DYNAMIC_ALLOC
	k_thread_stack_free(t->stack);
	osal_cb_free(t);
#else
	stack_slot_give(t->slot);
	k_mem_slab_free(&osal_thread_slab, t);
#endif
}

osal_thread_t osal_thread_create(osal_thread_entry_t entry, const char *name,
				 void *arg, uint32_t stack_size, int priority)
{
	struct osal_thread_cb *t;
	k_tid_t tid;
	size_t eff_size;

	if (entry == NULL || stack_size == 0) {
		return NULL;
	}

	t = thread_alloc(stack_size, &eff_size);
	if (t == NULL) {
		LOG_ERR("thread alloc failed");
		return NULL;
	}

	t->entry = entry;
	t->arg = arg;

	tid = k_thread_create(&t->thread, t->stack, eff_size, thread_trampoline,
			      t, NULL, NULL, map_priority(priority), 0, K_NO_WAIT);
	if (tid == NULL) {
		thread_free(t);
		return NULL;
	}

	if (name != NULL && IS_ENABLED(CONFIG_THREAD_NAME)) {
		k_thread_name_set(tid, name);
	}

	return (osal_thread_t)t;
}

void osal_thread_delete(osal_thread_t thread)
{
	struct osal_thread_cb *t = thread;

	if (t == NULL || (struct k_thread *)t == k_current_get()) {
		/*
		 * Self-delete. k_thread_abort() on the current thread does not
		 * return, so the control block and stack cannot be reclaimed
		 * here. A thread should instead return from its entry function
		 * to be cleaned up; self-delete is a best-effort abort only.
		 */
		k_thread_abort(k_current_get());
		return;
	}

	k_thread_abort(&t->thread);
	thread_free(t);
}

osal_thread_t osal_thread_current(void)
{
	return (osal_thread_t)k_current_get();
}

void osal_thread_yield(void)
{
	k_yield();
}

/*
 * Priority and suspend/resume operate on a handle returned by
 * osal_thread_create(). Its control block starts with the k_thread, so the
 * handle aliases a struct k_thread *.
 */
void osal_thread_suspend(osal_thread_t thread)
{
	if (thread == NULL) {
		return;
	}

	k_thread_suspend((struct k_thread *)thread);
}

void osal_thread_resume(osal_thread_t thread)
{
	if (thread == NULL) {
		return;
	}

	k_thread_resume((struct k_thread *)thread);
}

int osal_thread_priority_get(osal_thread_t thread)
{
	int zprio;

	if (thread == NULL) {
		return 0;
	}

	/*
	 * Invert the create-time mapping back to the foreign scale. Clamp the
	 * Zephyr priority into the preemptive range first so cooperative
	 * (negative) priorities and externally-set values map to a valid,
	 * bounded foreign value. Note the round trip is lossy when the
	 * create-time priority was itself clamped.
	 */
	zprio = k_thread_priority_get((struct k_thread *)thread);
	zprio = CLAMP(zprio, 0, CONFIG_NUM_PREEMPT_PRIORITIES - 1);

	return CONFIG_NUM_PREEMPT_PRIORITIES - 1 - zprio;
}

void osal_thread_priority_set(osal_thread_t thread, int priority)
{
	if (thread == NULL) {
		return;
	}

	k_thread_priority_set((struct k_thread *)thread, map_priority(priority));
}
