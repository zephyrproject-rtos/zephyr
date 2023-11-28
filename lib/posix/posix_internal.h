/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_POSIX_POSIX_INTERNAL_H_
#define ZEPHYR_LIB_POSIX_POSIX_INTERNAL_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/slist.h>

/*
 * Bit used to mark a pthread object as initialized. Initialization status is
 * verified (against internal status) in lock / unlock / destroy functions.
 */
#define PTHREAD_OBJ_MASK_INIT 0x80000000

struct posix_thread {
	struct k_thread thread;

	/* List node for ready_q, run_q, or done_q */
	sys_dnode_t q_node;

	/* List of keys that thread has called pthread_setspecific() on */
	sys_slist_t key_list;

	/* Dynamic stack */
	k_thread_stack_t *dynamic_stack;

	/* Exit status */
	void *retval;

	/* Pthread cancellation */
	uint8_t cancel_state;
	bool cancel_pending;

	/* Detach state */
	uint8_t detachstate;

	/* Queue ID (internal-only) */
	uint8_t qid;
};

typedef struct pthread_key_obj {
	/* List of pthread_key_data objects that contain thread
	 * specific data for the key
	 */
	sys_slist_t key_data_l;

	/* Optional destructor that is passed to pthread_key_create() */
	void (*destructor)(void *value);
} pthread_key_obj;

typedef struct pthread_thread_data {
	sys_snode_t node;

	/* Key and thread specific data passed to pthread_setspecific() */
	pthread_key_obj *key;
	void *spec_data;
} pthread_thread_data;

static inline bool is_pthread_obj_initialized(uint32_t obj)
{
	return (obj & PTHREAD_OBJ_MASK_INIT) != 0;
}

static inline uint32_t mark_pthread_obj_initialized(uint32_t obj)
{
	return obj | PTHREAD_OBJ_MASK_INIT;
}

static inline uint32_t mark_pthread_obj_uninitialized(uint32_t obj)
{
	return obj & ~PTHREAD_OBJ_MASK_INIT;
}

struct posix_thread *to_posix_thread(pthread_t pth);

/* get and possibly initialize a posix_mutex */
struct k_mutex *to_posix_mutex(pthread_mutex_t *mu);

typedef int32_t z_pid_t;

/**
 * @brief Get the process identifier (PID) of the calling process.
 *
 * @return The current PID
 */
__syscall z_pid_t z_getpid(void);

/**
 * @brief Grant a thread signal-related permissions
 *
 * Grant @p tid permission to
 * - change the global process identifier (PID)
 * - install signal handlers
 * - send signals
 *
 * @param tid the thread identifier for which permission should be granted
 */
void z_thread_access_grant_signal_mgmt(k_tid_t tid);

/**
 * @brief Set the global process identifier
 *
 * Set the global process identifier.
 *
 * In the case that @ref CONFIG_USERSPACE is selected, and the calling thread is not running in
 * kernel mode, @ref z_thread_access_grant_signal_mgmt must be called in advance to grant the
 * calling thread permission.
 *
 * @note Valid @p pid values lie in the range `[2,INT32_MAX]` for compatibility with other POSIX
 * systems.
 *
 * @param pid The desired process identifier
 * @retval 0 on success
 * @retval -EINVAL if @p pid is invalid
 * @retval -EPERM if the calling thread lacks permission
 */
__syscall int z_pid_set(z_pid_t pid);

/**
 * @brief Send a signal to a process
 *
 * In the case that @ref CONFIG_USERSPACE is selected, and the calling thread is not running in
 * kernel mode, @ref z_thread_access_grant_signal_mgmt must be called in advance to grant the
 * calling thread permission.
 *
 * @param pid The desired process to signal
 * @param sig The desired signal to send
 * @retval 0 on success
 * @retval -EINVAL if @p pid is invalid
 * @retval -EPERM if the calling thread lacks permission
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/functions/kill.html">kill</a>
 */
__syscall int z_kill(z_pid_t pid, int sig);

/**
 * @brief Manage signal handlers for the calling process
 *
 * Prior to calling, the user should populate @p func with the desired signal handler to install.
 *
 * Upon success, the location pointed to by @p func will contain the previous value of the signal
 * handler for @p sig.
 *
 * @note In the case that @ref CONFIG_USERSPACE is selected, and the calling thread is not running
 * in kernel mode, @ref z_thread_access_grant_signal_mgmt must be called in advance to grant the
 * calling thread permission.
 *
 * @param sig The desired signal to handle
 * @param[inout] func The desired signal handler to use (and where the old)
 * @retval 0 on success
 * @retval -EINVAL if @p sig is invalid
 * @retval -EPERM if the calling thread lacks permission
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/functions/signal.html">signal</a>
 */
__syscall int z_signal(int sig, sighandler_t *func);

#include <syscalls/posix_internal.h>

#endif
