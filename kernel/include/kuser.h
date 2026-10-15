/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Hooks through which the common kernel code reaches the user mode support
 * in kernel/userspace/. Each hook has a variant at the end of this file for
 * builds without CONFIG_USERSPACE that compiles to nothing, so the callers
 * carry no user mode conditionals.
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KUSER_H_
#define ZEPHYR_KERNEL_INCLUDE_KUSER_H_

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <kernel_internal.h>

#ifdef CONFIG_USERSPACE

/* True when the stack object can host a user mode thread. */
bool z_stack_is_user_capable(k_thread_stack_t *stack);

/* This spinlock:
 *
 * - Protects the full set of active k_mem_domain objects and their contents
 * - Serializes calls to arch_mem_domain_* APIs
 *
 * If architecture code needs to access k_mem_domain structures or the
 * partitions they contain at any other point, this spinlock should be held.
 * Uniprocessor systems can get away with just locking interrupts but this is
 * not recommended.
 */
extern struct k_spinlock z_mem_domain_lock;

/* Memory domain setup hook, called from z_setup_new_thread() */
void z_mem_domain_init_thread(struct k_thread *thread);

/* Memory domain teardown hook, called from z_thread_abort() */
void z_mem_domain_exit_thread(struct k_thread *thread);

/* Register the thread and stack kernel objects of a new thread, record the
 * stack ownership and grant the thread access to itself.
 */
void z_thread_user_init(struct k_thread *thread, k_thread_stack_t *stack, uint32_t options);

/* Attach a new thread to its memory domain and inherit the creator's
 * permissions when K_INHERIT_PERMS is set in @p options.
 */
void z_thread_user_perms_init(struct k_thread *thread, uint32_t options);

/* Release the user mode state of a thread being aborted. */
void z_thread_user_exit(struct k_thread *thread);

/* Attach the dummy thread to the default memory domain. */
void z_thread_user_dummy_init(struct k_thread *dummy_thread);

/* Apply the K_THREAD_ACCESS_GRANT() assignments of the static threads. */
void z_thread_user_grant_static(void);

/* Drop the current thread to user mode and run @p entry there. */
FUNC_NORETURN void z_thread_user_mode_enter(k_thread_entry_t entry, void *p1, void *p2, void *p3);

/* Free a dynamically allocated stack that is a kernel object. Returns false
 * when @p stack is not a kernel object and the caller has to free it.
 */
bool z_stack_user_free(k_thread_stack_t *stack);

#else /* !CONFIG_USERSPACE */

static inline bool z_stack_is_user_capable(k_thread_stack_t *stack)
{
	ARG_UNUSED(stack);

	return false;
}

static inline void z_thread_user_init(struct k_thread *thread, k_thread_stack_t *stack,
				      uint32_t options)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(stack);
	ARG_UNUSED(options);
}

static inline void z_thread_user_perms_init(struct k_thread *thread, uint32_t options)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(options);
}

static inline void z_thread_user_exit(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

static inline void z_thread_user_dummy_init(struct k_thread *dummy_thread)
{
	ARG_UNUSED(dummy_thread);
}

static inline void z_thread_user_grant_static(void)
{
}

static inline FUNC_NORETURN void z_thread_user_mode_enter(k_thread_entry_t entry, void *p1,
							  void *p2, void *p3)
{
	/* Without user mode the thread carries on in supervisor mode; its
	 * stack is not reset.
	 */
	z_thread_entry(entry, p1, p2, p3);
}

static inline bool z_stack_user_free(k_thread_stack_t *stack)
{
	ARG_UNUSED(stack);

	return false;
}

#endif /* CONFIG_USERSPACE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KUSER_H_ */
