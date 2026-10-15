/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Thread stacks mapped into virtual memory at thread creation
 * (CONFIG_THREAD_STACK_MEM_MAPPED). The common thread code reaches this
 * through the hooks in kthread.h.
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <kthread.h>

/* Mapping of a thread whose cleanup was deferred; the thread object may be
 * reused before the cleanup runs. Written and read under the thread cleanup
 * lock of thread.c.
 */
static void *cleanup_stack_addr;
static size_t cleanup_stack_sz;

static void unmap_thread_stack(void *addr, size_t sz)
{
	if (addr != NULL) {
		k_mem_unmap_phys_guard(addr, sz, false);
	}
}

void *z_thread_stack_map(struct k_thread *thread, k_thread_stack_t *stack, size_t obj_size)
{
	/* K_MEM_MAP_UNINIT is used to mimic the behavior of non-mapped
	 * stack. If CONFIG_INIT_STACKS is enabled, the stack will be
	 * cleared later.
	 */
	void *stack_mapped = k_mem_map_phys_guard((uintptr_t)stack, obj_size,
				K_MEM_PERM_RW | K_MEM_CACHE_WB | K_MEM_MAP_UNINIT,
				false);

	__ASSERT_NO_MSG((uintptr_t)stack_mapped != 0);

	/* Need to store the info on mapped stack so we can remove the mappings
	 * when the thread ends.
	 */
	thread->stack_info.mapped.addr = stack_mapped;
	thread->stack_info.mapped.sz = obj_size;

	return stack_mapped;
}

bool z_thread_stack_is_unmapped(const struct k_thread *thread)
{
	return thread->stack_info.mapped.addr == NULL;
}

void z_thread_stack_cleanup_stash(struct k_thread *thread)
{
	/* Note that the permission of the stack should have been
	 * stripped of user thread access due to the thread having
	 * already exited from a memory domain. That is done via
	 * k_thread_abort().
	 */

	/* Stash the address and size so the region can be unmapped
	 * later.
	 */
	cleanup_stack_addr = thread->stack_info.mapped.addr;
	cleanup_stack_sz = thread->stack_info.mapped.sz;

	/* The stack is now considered un-usable. This should prevent any functions
	 * from looking directly into the mapped stack if they are made to be aware
	 * of memory mapped stacks, e.g., z_stack_space_get().
	 */
	thread->stack_info.mapped.addr = NULL;
	thread->stack_info.mapped.sz = 0;
}

void z_thread_stack_cleanup_stashed(void)
{
	unmap_thread_stack(cleanup_stack_addr, cleanup_stack_sz);

	cleanup_stack_addr = NULL;
	cleanup_stack_sz = 0;
}

void z_thread_stack_unmap(struct k_thread *thread)
{
	unmap_thread_stack(thread->stack_info.mapped.addr,
			   thread->stack_info.mapped.sz);

	/* The stack is now unmapped and thus un-usable. Clear the mapping
	 * info so nothing looks into the stack afterwards, and so that
	 * a repeated cleanup of the same thread object is a no-op,
	 * e.g., z_stack_space_get().
	 */
	thread->stack_info.mapped.addr = NULL;
	thread->stack_info.mapped.sz = 0;
}
