/*
 * Copyright (c) 2025, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_SYS_THREAD_H_
#define ZEPHYR_INCLUDE_ZEPHYR_SYS_THREAD_H_

#include <stdint.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/thread_once.h>

#ifdef __cplusplus
extern "C" {
#endif

struct k_condvar;
struct k_mutex;
struct k_thread;
struct k_sig_set;

/**
 * @brief Create the child stopped
 *
 * The child is fully constructed but its initial thread is not started; the
 * caller completes setup (process group, signal state, priority) and starts
 * it with k_thread_start() on the returned child handle.
 */
#define SYS_CLONE_PAUSED BIT64(1)

/**
 * @brief Create a thread in the calling process, not a new process
 *
 * The new thread joins the caller's thread group instead of leading a new
 * process; without @kconfig{CONFIG_PROCESS} it is simply a new thread. The
 * handle returned through @a child is the thread handle.
 * sys_thread_create() is a convenience wrapper over this tier.
 */
#define SYS_CLONE_THREAD BIT64(2)

/**
 * @brief Arguments for sys_clone()
 *
 * Without @ref SYS_CLONE_THREAD the initial thread becomes the new process's
 * thread group leader: its handle is the new process's @ref k_pid_t.
 */
struct sys_clone_args {
	/** SYS_CLONE_* flags; zero clones nothing from the caller */
	uint64_t flags;
	/** Entry point of the initial thread */
	k_thread_entry_t entry;
	/** Entry parameters */
	void *p1, *p2, *p3;
	/** Initial thread object, or NULL to draw one from the system pool */
	struct k_thread *thread;
	/** Initial thread stack (pool-drawn threads may pass NULL for a pool stack) */
	k_thread_stack_t *stack;
	/** Initial thread stack size, in bytes */
	size_t stack_size;
	/** Stack guard size for pool-drawn stacks, in bytes */
	size_t guard_size;
	/** Initial thread priority */
	int prio;
	/** Initial thread options (e.g. K_USER) */
	uint32_t options;
	/**
	 * Initial signal mask of the created thread, or NULL for the
	 * thread-creation inheritance rules. As with k_sig_mask(), a
	 * user-mode request cannot mask K_SIG_KILL or K_SIG_STOP; a
	 * kernel-mode caller may. Requires CONFIG_SIGNAL.
	 */
	const struct k_sig_set *sigmask;
};

/**
 * @brief The creation primitive: a new process, or a thread in this one
 *
 * Without @ref SYS_CLONE_THREAD, allocates a process record from the system
 * pool, creates the initial thread from @a args, and starts it as the leader
 * of the new process - a child of the caller, joining the caller's process
 * group. @a args->flags selects what the child clones from the caller; zero
 * clones nothing (a fresh process). With @ref SYS_CLONE_THREAD, creates a
 * thread in the calling process instead. Higher-level thread and process
 * creation APIs are library constructs over this call.
 *
 * @retval 0 on success
 * @retval -EINVAL if @a args is invalid (including unsupported flags)
 * @retval -EAGAIN if no process record is available
 * @retval -EBUSY if @a args->thread still leads an unreaped process
 * @retval -ENOTSUP for a process-tier clone without @kconfig{CONFIG_SYS_PROCESS}
 */
__syscall int sys_clone(const struct sys_clone_args *args, k_pid_t *child);


#if defined(CONFIG_SYS_THREAD) || defined(__DOXYGEN__)

/**
 * @defgroup sys_thread_apis System Thread Pool APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Flag indicating the synchronization object is process-shared.
 *
 * When passed to a pool allocation, the created object is accessible to any thread that is able
 * to access the object in memory. Passing this option is equivalent to calling
 * @ref k_object_access_all_grant after initialization.
 */
#define SYS_THREAD_PROCESS_SHARED 1

/**
 * @brief Flag indicating the synchronization object is process-private.
 *
 * This is the default sharing mode for synchronization objects and may be omitted.
 */
#define SYS_THREAD_PROCESS_PRIVATE 0

/**
 * @brief Create a thread using resource from the system pool.
 *
 * Allocates a @ref k_thread and from the system pool and starts it.
 *
 * If @p stack is @c NULL, a stack will be allocated for the thread from the system pool using
 * the specified @p stack_size and @p guard_size via @ref sys_thread_stack_alloc. If
 * @p stack_size is zero, a thread stack is allocated using a default size of
 * @kconfig{CONFIG_SYS_THREAD_STACK_SIZE}, or a suitable size when @kconfig{CONFIG_MMU} is
 * enabled.
 *
 * If @p stack is not @c NULL, then the provided stack is used rather than allocating one.
 *
 * When the thread terminates and is joined, both the thread object and its associated stack are
 * automatically returned to their respective pools.
 *
 * @param[out] th Address of a pointer that will be set to the newly
 *                created thread.
 * @param stack Stack to use, or @c NULL to allocate one from the pool.
 * @param stack_size Desired or actual stack size in bytes.
 * @param guard_size Desired or actual guard size in bytes.
 * @param entry Thread entry point.
 * @param p1 First argument to @p entry.
 * @param p2 Second argument to @p entry.
 * @param p3 Third argument to @p entry.
 * @param prio Thread priority.
 * @param options Thread creation option flags (e.g. @ref K_USER).
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument was invalid.
 * @retval -ENOMEM if the pool is exhausted.
 *
 * @note The @p guard_size parameter is currently unused, as Zephyr thread stacks have fixed guard
 * region sizes when they are present.
 */
__syscall int sys_thread_create(struct k_thread **th, k_thread_stack_t *stack, uint32_t stack_size,
				uint32_t guard_size, k_thread_entry_t entry, void *p1, void *p2,
				void *p3, int prio, uint32_t options);

/**
 * @brief As sys_thread_create(), but the thread is created paused
 *
 * The caller completes setup (memory domain membership, object permissions) and then starts
 * the thread with k_thread_start(). Kernel-mode only (not a system call).
 */
int sys_thread_create_paused(struct k_thread **th, k_thread_stack_t *stack, uint32_t stack_size,
			     uint32_t guard_size, k_thread_entry_t entry, void *p1, void *p2,
			     void *p3, int prio, uint32_t options);

/**
 * @brief Allocate and initialize a thread stack from the system pool.
 *
 * If @p stack_size is less than or equal to @kconfig{CONFIG_SYS_THREAD_STACK_SIZE}, a stack of size
 * @kconfig{CONFIG_SYS_THREAD_STACK_SIZE} will be allocated from the system pool. If @p stack_size
 * is greater than @kconfig{CONFIG_SYS_THREAD_STACK_SIZE}, then a thread will be allocated from
 * the system heap when @kconfig{CONFIG_SYS_THREAD_STACK_ALLOC_HEAP} is enabled, or using
 * anonymous memory maps, when @kconfig{CONFIG_SYS_THREAD_STACK_ALLOC_ANON} is enabled.
 *
 * The @p flags parameter must include @ref
 * K_USER when allocating a user-capable thread stack from a kernel thread. The @ref K_USER flag is
 * implicit when allocating a thread stack from a user thread.
 *
 * @param[out] stack Address of a pointer that will be set to the newly allocated stack.
 * @param[in,out] stack_size Desired or actual stack size in bytes.
 * @param guard_size Desired or actual guard size in bytes.
 * @param flags Allocation flags.
 *
 * @return 0 on success.
 * @retval -ENOMEM if the requested allocation cannot be satisfied.
 * @retval -EINVAL if an argument was invalid.
 *
 * @note The @p guard_size parameter is currently unused, as Zephyr thread stacks have fixed guard
 * region sizes when they are present.
 */
__syscall int sys_thread_stack_alloc(k_thread_stack_t **stack, uint32_t *stack_size,
				     uint32_t guard_size, uint32_t flags);

/**
 * @brief Free a thread stack obtained from the system pool.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the system thread stack pool is disabled
 * (@kconfig{CONFIG_SYS_THREAD_STACK_MAX} == 0).
 * @retval -EBUSY if the stack is still in use by a thread.
 */
__syscall int sys_thread_stack_destroy(k_thread_stack_t *stack);

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief Destroy a thread obtained from the system pool.
 *
 * Returns @p th (and its associated stack, if applicable) to the system pool.
 * This call is mainly used by the kernel when a thread terminates or is joined.
 *
 * @note This function is safe to call even if @p th or its stack were
 *       statically allocated; in that case it is a no-op for those objects.
 *
 * @param th Pointer to the thread to destroy.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the system thread and stack pools are disabled
 * (@kconfig{CONFIG_SYS_THREAD_STACK_MAX} == 0 and @kconfig{CONFIG_SYS_THREAD_THREAD_MAX} == 0).
 */
__syscall int sys_thread_destroy(struct k_thread *th);

/**
 * INTERNAL_HIDDEN @endcond
 */

/** @} */

#endif /* defined(CONFIG_SYS_THREAD) || defined(__DOXYGEN__) */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/thread.h>

#endif /* ZEPHYR_INCLUDE_ZEPHYR_SYS_THREAD_H_ */
