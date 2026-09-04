/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-process C library state
 *
 * An image process loaded with @kconfig{CONFIG_SYS_PROCESS_STATE_AREA} set owns
 * an area of process memory for the C library's state: its malloc heap, the
 * environment it was entered with, and slots other libraries fill with state
 * of their own, allocated from that heap. Processes without an area share the
 * boot image's state.
 */

#ifndef ZEPHYR_INCLUDE_SYS_PROCESS_STATE_H_
#define ZEPHYR_INCLUDE_SYS_PROCESS_STATE_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Slots for other libraries' per-process state */
enum process_state_slot {
	PROCESS_STATE_SLOT_ENV,		/**< The environment (POSIX getenv() and friends) */
	PROCESS_STATE_SLOT_STDIO,	/**< Standard I/O streams */
	PROCESS_STATE_SLOT_POSIX,	/**< POSIX per-process attributes (umask and friends) */
	PROCESS_STATE_SLOT_COUNT,
};

/** Whether processes can have C library state of their own in this build */
#if defined(CONFIG_SYS_PROCESS_EXEC) && defined(CONFIG_USERSPACE) && \
	(CONFIG_SYS_PROCESS_STATE_AREA > 0)
#define PROCESS_STATE_SUPPORTED 1
#else
#define PROCESS_STATE_SUPPORTED 0
#endif

#if PROCESS_STATE_SUPPORTED
/** The per-process state block, at the start of the process's area */
struct process_state {
	/** @cond INTERNAL_HIDDEN */
	atomic_t init;
	struct k_futex lock;
	struct k_futex heap_lock;
	void *slot[PROCESS_STATE_SLOT_COUNT];
	/** The environment vector the process was entered with */
	char **envp;
	/** The process's malloc heap; the arena follows the block */
	struct sys_heap heap;
	/** @endcond */
};

/**
 * @brief The calling process's C library state
 *
 * @return the process's state block, or NULL when the boot state is shared
 */
struct process_state *process_state_get(void);

/**
 * @brief Take the process state lock (a futex: process memory, any mode)
 *
 * The heap has its own lock, so the state lock may be held across malloc().
 */
void process_state_lock(struct process_state *lp);

/** @brief Release the process state lock */
void process_state_unlock(struct process_state *lp);

/** @brief Take the process heap lock; never held while taking the state lock */
void process_state_heap_lock(struct process_state *lp);

/** @brief Release the process heap lock */
void process_state_heap_unlock(struct process_state *lp);
#else
/** No per-process state in this configuration: the block is never instantiated */
struct process_state;

static inline struct process_state *process_state_get(void)
{
	return NULL;
}

static inline void process_state_lock(struct process_state *lp)
{
	ARG_UNUSED(lp);
}

static inline void process_state_unlock(struct process_state *lp)
{
	ARG_UNUSED(lp);
}

static inline void process_state_heap_lock(struct process_state *lp)
{
	ARG_UNUSED(lp);
}

static inline void process_state_heap_unlock(struct process_state *lp)
{
	ARG_UNUSED(lp);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_PROCESS_STATE_H_ */
