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

/* Memory domain setup hook, called from z_setup_new_thread(): make the new
 * thread a member of the memory domain its parent is in.
 */
void z_mem_domain_init_thread(struct k_thread *thread, struct k_thread *parent);

/* Memory domain teardown hook, called from z_thread_abort() */
void z_mem_domain_exit_thread(struct k_thread *thread);

/* Register the thread and stack kernel objects of a new thread, record the
 * stack ownership and grant the thread access to itself.
 */
void z_thread_user_init(struct k_thread *thread, k_thread_stack_t *stack, uint32_t options);

/* Make a new thread a member of its parent's memory domain and inherit the
 * parent's permissions when K_INHERIT_PERMS is set in @p options.
 */
void z_thread_user_perms_init(struct k_thread *thread, struct k_thread *parent,
			      uint32_t options);

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

static inline void z_thread_user_perms_init(struct k_thread *thread, struct k_thread *parent,
					    uint32_t options)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(parent);
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

#ifdef CONFIG_DYNAMIC_OBJECTS

/* Keep a dynamically allocated object from being freed when the last
 * permission on it goes, or by k_object_free(), until
 * z_object_hold_release(). Several holds can be taken. Returns @p ko if a
 * hold was taken, NULL for @p ko NULL or an object that is never freed.
 */
struct k_object *z_object_hold(struct k_object *ko);

/* Drop a hold taken by z_object_hold(), and free the object if it was the
 * last one and no thread has permission on the object anymore, or if
 * k_object_free() was called on it meanwhile. @p ko is what z_object_hold()
 * returned: once k_object_free() has been called, k_object_find() no longer
 * finds the object.
 */
void z_object_hold_release(struct k_object *ko);

/* The thread and stack objects that z_thread_user_init() keeps allocated
 * for a thread until it has switched away from them for the last time.
 */
struct z_thread_user_hold {
	struct k_thread *thread;
	struct k_object *thread_obj;
	struct k_object *stack_obj;
};

/* Move to @p hold the objects held for the dead @p thread, so that a
 * further z_thread_user_hold_get() finds none. Called with the scheduler
 * lock held from the abort path, while @p thread is intact.
 */
void z_thread_user_hold_get(struct k_thread *thread, struct z_thread_user_hold *hold);

/* Release the objects recorded in @p hold, once the thread is off the CPU,
 * which on SMP is waited for here.
 */
void z_thread_user_hold_put(struct z_thread_user_hold *hold);

/* z_thread_user_hold_get() and z_thread_user_hold_put() at once, for a dead
 * thread that is already off the CPU.
 */
void z_thread_user_release(struct k_thread *thread);

#else /* !CONFIG_DYNAMIC_OBJECTS */

static inline void z_thread_user_release(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

#endif /* CONFIG_DYNAMIC_OBJECTS */

#endif /* ZEPHYR_KERNEL_INCLUDE_KUSER_H_ */
