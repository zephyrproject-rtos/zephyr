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
}

void z_thread_user_perms_init(struct k_thread *thread, uint32_t options)
{
	z_mem_domain_init_thread(thread);

	if ((options & K_INHERIT_PERMS) != 0U) {
		k_thread_perms_inherit(_current, thread);
	}
}

void z_thread_user_exit(struct k_thread *thread)
{
	z_mem_domain_exit_thread(thread);
	k_thread_perms_all_clear(thread);
	k_object_uninit(thread->stack_obj);
	k_object_uninit(thread);
}

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
