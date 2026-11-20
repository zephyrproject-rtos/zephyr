/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * User mode side of the thread lifecycle: kernel object registration,
 * permissions, memory domain membership and the drop to user mode. The
 * common thread code reaches this through the hooks in kuser.h.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/iterable_sections.h>
#include <string.h>
#include <kernel_internal.h>
#include <ksmp.h>
#include <kthread.h>
#include <kuser.h>

bool z_stack_is_user_capable(k_thread_stack_t *stack)
{
	return k_object_find(stack) != NULL;
}

void z_thread_user_init(struct k_thread *thread, k_thread_stack_t *stack, uint32_t options)
{
	__ASSERT((options & K_USER) == 0U || z_stack_is_user_capable(stack),
		 "user thread %p with kernel-only stack %p", thread, stack);
	k_object_init(thread);
	k_object_init(stack);
	thread->stack_obj = stack;
	thread->syscall_frame = NULL;

	/* Any given thread has access to itself */
	k_object_access_grant(thread, thread);

#ifdef CONFIG_DYNAMIC_OBJECTS
	/* Until it has switched away from them for the last time, the thread
	 * runs on its stack and the kernel uses its thread object, whatever
	 * permissions are dropped on them meanwhile. Keep dynamically
	 * allocated ones from being freed until then, see
	 * z_thread_user_hold_put(), and record their metadata for that, as
	 * k_object_free() may meanwhile take them off the list that
	 * k_object_find() looks them up in. Only for a thread that is a
	 * kernel object: releasing them reads the thread object, which then
	 * stays intact until that point.
	 */
	struct k_object *ko = k_object_find(thread);

	thread->held_thread_obj = NULL;
	thread->held_stack_obj = NULL;

	if (ko != NULL) {
		thread->held_thread_obj = z_object_hold(ko);
		thread->held_stack_obj = z_object_hold(k_object_find(stack));
	}
#endif /* CONFIG_DYNAMIC_OBJECTS */
}

void z_thread_user_perms_init(struct k_thread *thread, struct k_thread *parent, uint32_t options)
{
	z_mem_domain_init_thread(thread, parent);

	if ((options & K_INHERIT_PERMS) != 0U) {
		k_thread_perms_inherit(parent, thread);
	}
}

void z_thread_user_exit(struct k_thread *thread)
{
	z_mem_domain_exit_thread(thread);
	k_thread_perms_all_clear(thread);
	k_object_uninit(thread->stack_obj);
	k_object_uninit(thread);
}

#ifdef CONFIG_DYNAMIC_OBJECTS
void z_thread_user_hold_get(struct k_thread *thread, struct z_thread_user_hold *hold)
{
	hold->thread = thread;
	hold->thread_obj = thread->held_thread_obj;
	hold->stack_obj = thread->held_stack_obj;

	thread->held_thread_obj = NULL;
	thread->held_stack_obj = NULL;
}

void z_thread_user_hold_put(struct z_thread_user_hold *hold)
{
	if ((hold->thread_obj == NULL) && (hold->stack_obj == NULL)) {
		return;
	}

	/* The thread may still be saving its context on the CPU it last
	 * ran on, so wait for that as k_thread_join() does. Its thread
	 * object is a static or held kernel object, and it is not reused
	 * before this runs. But k_thread_abort() lets its owner overwrite it
	 * once the thread is off the CPU, in which case it no longer reads
	 * as dead.
	 */
	if (z_is_thread_dead(hold->thread)) {
		z_sched_switch_spin(hold->thread);
	}

	z_object_hold_release(hold->stack_obj);
	z_object_hold_release(hold->thread_obj);

	hold->thread = NULL;
	hold->thread_obj = NULL;
	hold->stack_obj = NULL;
}

void z_thread_user_release(struct k_thread *thread)
{
	struct z_thread_user_hold hold;

	z_thread_user_hold_get(thread, &hold);
	z_thread_user_hold_put(&hold);
}
#endif /* CONFIG_DYNAMIC_OBJECTS */

void z_thread_user_dummy_init(struct k_thread *dummy_thread)
{
	dummy_thread->mem_domain_info.mem_domain = &k_mem_domain_default;
}

void z_thread_user_grant_static(void)
{
	STRUCT_SECTION_FOREACH(k_object_assignment, pos) {
		for (int i = 0; pos->objects[i] != NULL; i++) {
			k_object_access_grant(pos->objects[i], pos->thread);
		}
	}
}

FUNC_NORETURN void z_thread_user_mode_enter(k_thread_entry_t entry, void *p1, void *p2, void *p3)
{
	__ASSERT(z_stack_is_user_capable(_current->stack_obj),
		 "dropping to user mode with kernel-only stack object");
#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
	memset(_current->userspace_local_data, 0, sizeof(struct _thread_userspace_local_data));
#endif /* CONFIG_THREAD_USERSPACE_LOCAL_DATA */
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	arch_tls_stack_setup(_current,
			     (char *)(_current->stack_info.start +
				      _current->stack_info.size));
#endif /* CONFIG_THREAD_LOCAL_STORAGE */
	arch_user_mode_enter(entry, p1, p2, p3);
}

bool z_stack_user_free(k_thread_stack_t *stack)
{
	if (k_object_find(stack) == NULL) {
		return false;
	}

	k_object_free(stack);

	return true;
}
