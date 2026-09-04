/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/sys/process_state.h>
#include <zephyr/sys/process.h>

#if defined(CONFIG_SYS_PROCESS_EXEC) && defined(CONFIG_USERSPACE) && \
	(CONFIG_SYS_PROCESS_STATE_AREA > 0)

#define PROCESS_STATE_UNKNOWN ((struct process_state *)-1)
#define PROCESS_STATE_ARENA_ALIGN 16

#ifdef CONFIG_THREAD_LOCAL_STORAGE
static __thread struct process_state *process_state_cache = PROCESS_STATE_UNKNOWN;
#endif /* CONFIG_THREAD_LOCAL_STORAGE */

/* laid out on first use by whichever thread of the process comes first */
static struct process_state *process_state_init(void *area, size_t size)
{
	struct process_state *lp = area;
	size_t arena = ROUND_UP(sizeof(*lp), PROCESS_STATE_ARENA_ALIGN);

	if (size < arena + 2 * PROCESS_STATE_ARENA_ALIGN) {
		return NULL;
	}
	if (atomic_cas(&lp->init, 0, 1)) {
		sys_heap_init(&lp->heap, (uint8_t *)area + arena, size - arena);
		atomic_set(&lp->init, 2);
	}
	while (atomic_get(&lp->init) != 2) {
		k_yield();
	}

	return lp;
}

struct process_state *process_state_get(void)
{
	struct process_state *lp;
	void *area;
	size_t size;

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	if (process_state_cache != PROCESS_STATE_UNKNOWN) {
		return process_state_cache;
	}
#endif /* CONFIG_THREAD_LOCAL_STORAGE */
	lp = (sys_process_state_area(&area, &size) == 0) ? process_state_init(area, size) : NULL;
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	process_state_cache = lp;
#endif /* CONFIG_THREAD_LOCAL_STORAGE */

	return lp;
}

static void futex_lock(struct k_futex *f)
{
	while (!atomic_cas(&f->val, 0, 1)) {
		(void)k_futex_wait(f, 1, K_FOREVER);
	}
}

static void futex_unlock(struct k_futex *f)
{
	atomic_set(&f->val, 0);
	(void)k_futex_wake(f, false);
}

void process_state_lock(struct process_state *lp)
{
	futex_lock(&lp->lock);
}

void process_state_unlock(struct process_state *lp)
{
	futex_unlock(&lp->lock);
}

void process_state_heap_lock(struct process_state *lp)
{
	futex_lock(&lp->heap_lock);
}

void process_state_heap_unlock(struct process_state *lp)
{
	futex_unlock(&lp->heap_lock);
}

void z_process_state_start(int argc, char **argv, char **envp)
{
	struct process_state *lp = process_state_get();

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	if (lp != NULL) {
		lp->envp = envp;
	}
}
#else
void z_process_state_start(int argc, char **argv, char **envp)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	ARG_UNUSED(envp);
}
#endif
