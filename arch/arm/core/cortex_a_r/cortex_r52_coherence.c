/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Andrei-Edward Popa
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-R52 software cache coherence.
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>

void arch_cohere_stacks(struct k_thread *old_thread, void *old_switch_handle,
			struct k_thread *new_thread)
{
	ARG_UNUSED(old_switch_handle);

	uintptr_t start = new_thread->stack_info.start;
	uintptr_t end = start + new_thread->stack_info.size;
	uint32_t last_cpu = new_thread->arch.last_cpu;
	uint32_t cpu = arch_curr_cpu()->id;

	old_thread->arch.last_cpu = cpu;
	if (last_cpu == cpu) {
		return;
	}
	if (last_cpu != UINT32_MAX) {
		/* Only live stack contents can be read after migration. */
		start = MAX(start, new_thread->callee_saved.psp);
	}

	/* R52 writes through L1. The DSB in z_arm_context_switch() completes
	 * outgoing stores before publishing switch_handle. A new thread needs
	 * its entire stack invalidated, including when storage has been reused.
	 */
	(void)arch_dcache_invd_range((void *)start, end - start);
#ifdef CONFIG_STACK_SENTINEL
	/* The sentinel lies below the live stack and is read by the kernel. */
	if (start != new_thread->stack_info.start) {
		(void)arch_dcache_invd_range((void *)new_thread->stack_info.start,
					     sizeof(uint32_t));
	}
#endif
	barrier_dsync_fence_full();
}
