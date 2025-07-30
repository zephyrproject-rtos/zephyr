/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifndef _ASMLANGUAGE
#include <kernel_internal.h>
#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/zsr.h>

#ifdef __cplusplus
extern "C" {
#endif

K_KERNEL_STACK_ARRAY_DECLARE(z_interrupt_stacks, CONFIG_MP_MAX_NUM_CPUS,
			     CONFIG_ISR_STACK_SIZE);

static ALWAYS_INLINE void arch_kernel_init(void)
{
#ifdef CONFIG_SOC_PER_CORE_INIT_HOOK
	soc_per_core_init_hook();
#endif /* CONFIG_SOC_PER_CORE_INIT_HOOK */
}

void xtensa_switch(void *switch_to, void **switched_from);

static ALWAYS_INLINE void arch_switch(void *switch_to, void **switched_from)
{
	xtensa_switch(switch_to, switched_from);
}

#ifdef CONFIG_KERNEL_COHERENCE
/**
 * @brief Invalidate cache between two stack addresses.
 *
 * This invalidates the cache lines between two stack addresses,
 * beginning with the cache line including the start address, up to
 * but not including the cache line containing the end address.
 * Not invalidating the last cache line is due to the usage in
 * arch_cohere_stacks() where it invalidates the unused portion of
 * stack. If the stack pointer happens to be in the middle of
 * a cache line, the cache line containing the stack pointer
 * address will be flushed, and then immediately invalidated.
 * If we are swapping back into the same thread (e.g. after
 * handling interrupt), that cache line, being invalidated, needs
 * to be retrieved from main memory. This creates unnecessary
 * data move between main memory and cache.
 *
 * @param s_addr Starting address of memory region to have cache invalidated.
 * @param e_addr Ending address of memory region to have cache invalidated.
 */
static ALWAYS_INLINE void xtensa_cohere_stacks_cache_invd(size_t s_addr, size_t e_addr)
{
	const size_t first = ROUND_DOWN(s_addr, XCHAL_DCACHE_LINESIZE);
	const size_t last = ROUND_DOWN(e_addr, XCHAL_DCACHE_LINESIZE);
	size_t line;

	for (line = first; line < last; line += XCHAL_DCACHE_LINESIZE) {
		__asm__ volatile("dhi %0, 0" :: "r"(line));
	}
}

/**
 * @brief Flush cache between two stack addresses.
 *
 * This flushes the cache lines between two stack addresses,
 * beginning with the cache line including the start address,
 * and ending with the cache line including the end address.
 * Note that, contrary to xtensa_cohere_stacks_cache_invd(),
 * the last cache line will be flushed instead of being
 * ignored.
 *
 * @param s_addr Starting address of memory region to have cache invalidated.
 * @param e_addr Ending address of memory region to have cache invalidated.
 */
static ALWAYS_INLINE void xtensa_cohere_stacks_cache_flush(size_t s_addr, size_t e_addr)
{
	const size_t first = ROUND_DOWN(s_addr, XCHAL_DCACHE_LINESIZE);
	const size_t last = ROUND_UP(e_addr, XCHAL_DCACHE_LINESIZE);
	size_t line;

	for (line = first; line < last; line += XCHAL_DCACHE_LINESIZE) {
		__asm__ volatile("dhwb %0, 0" :: "r"(line));
	}
}

/**
 * @brief Flush and invalidate cache between two stack addresses.
 *
 * This flushes the cache lines between two stack addresses,
 * beginning with the cache line including the start address,
 * and ending with the cache line including the end address.
 * Note that, contrary to xtensa_cohere_stacks_cache_invd(),
 * the last cache line will be flushed and invalidated instead
 * of being ignored.
 *
 * @param s_addr Starting address of memory region to have cache manipulated.
 * @param e_addr Ending address of memory region to have cache manipulated.
 */
static ALWAYS_INLINE void xtensa_cohere_stacks_cache_flush_invd(size_t s_addr, size_t e_addr)
{
	const size_t first = ROUND_DOWN(s_addr, XCHAL_DCACHE_LINESIZE);
	const size_t last = ROUND_UP(e_addr, XCHAL_DCACHE_LINESIZE);
	size_t line;

	for (line = first; line < last; line += XCHAL_DCACHE_LINESIZE) {
		__asm__ volatile("dhwbi %0, 0" :: "r"(line));
	}
}

static ALWAYS_INLINE void arch_cohere_stacks(struct k_thread *old_thread,
					     void *old_switch_handle,
					     struct k_thread *new_thread)
{
#ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	ARG_UNUSED(old_thread);
	ARG_UNUSED(old_switch_handle);
	ARG_UNUSED(new_thread);

	/* This kconfig option ensures that a living thread will never
	 * be executed in a different CPU so we can safely return without
	 * invalidate and/or flush threads cache.
	 */
	return;
#endif /* CONFIG_SCHED_CPU_MASK_PIN_ONLY */

#if !defined(CONFIG_SCHED_CPU_MASK_PIN_ONLY)
	int32_t curr_cpu = _current_cpu->id;

	size_t ostack = old_thread->stack_info.start;
	size_t oend   = ostack + old_thread->stack_info.size;
	size_t osp    = (size_t) old_switch_handle;

	size_t nstack = new_thread->stack_info.start;
	size_t nend   = nstack + new_thread->stack_info.size;
	size_t nsp    = (size_t) new_thread->switch_handle;

	uint32_t flush_end = 0;

#ifdef CONFIG_USERSPACE
	/* End of old_thread privileged stack. */
	void *o_psp_end = old_thread->arch.psp;
#endif

	__asm__ volatile("wsr %0, " ZSR_FLUSH_STR :: "r"(flush_end));

	if (old_switch_handle != NULL) {
		int32_t a0save;

		__asm__ volatile("mov %0, a0;"
				 "call0 xtensa_spill_reg_windows;"
				 "mov a0, %0"
				 : "=r"(a0save));
	}

	/* The "live" area (the region between the switch handle,
	 * which is the stack pointer, and the top of the stack
	 * memory) of the inbound stack needs to be invalidated if we
	 * last ran on another cpu: it may contain data that was
	 * modified there, and our cache may be stale.
	 *
	 * The corresponding "dead area" of the inbound stack can be
	 * ignored.  We may have cached data in that region, but by
	 * definition any unused stack memory will always be written
	 * before being read (well, unless the code has an
	 * uninitialized data error) so our stale cache will be
	 * automatically overwritten as needed.
	 */
	if (curr_cpu != new_thread->arch.last_cpu) {
		xtensa_cohere_stacks_cache_invd(nsp, nend);
	}
	old_thread->arch.last_cpu = curr_cpu;

	/* Dummy threads appear at system initialization, but don't
	 * have stack_info data and will never be saved.  Ignore.
	 */
	if (old_thread->base.thread_state & _THREAD_DUMMY) {
		return;
	}

	/* For the outbound thread, we obviousy want to flush any data
	 * in the live area (for the benefit of whichever CPU runs
	 * this thread next).  But we ALSO have to invalidate the dead
	 * region of the stack.  Those lines may have DIRTY data in
	 * our own cache, and we cannot be allowed to write them back
	 * later on top of the stack's legitimate owner!
	 *
	 * This work comes in two flavors.  In interrupts, the
	 * outgoing context has already been saved for us, so we can
	 * do the flush right here.  In direct context switches, we
	 * are still using the stack, so we do the invalidate of the
	 * bottom here, (and flush the line containing SP to handle
	 * the overlap).  The remaining flush of the live region
	 * happens in the assembly code once the context is pushed, up
	 * to the stack top stashed in a special register.
	 */
	if (old_switch_handle != NULL) {
#ifdef CONFIG_USERSPACE
		if (o_psp_end == NULL)
#endif
		{
			xtensa_cohere_stacks_cache_flush(osp, oend);
			xtensa_cohere_stacks_cache_invd(ostack, osp);
		}
	} else {
		/* When in a switch, our current stack is the outbound
		 * stack.  Flush the single line containing the stack
		 * bottom (which is live data) before invalidating
		 * everything below that.  Remember that the 16 bytes
		 * below our SP are the calling function's spill area
		 * and may be live too.
		 */
		__asm__ volatile("mov %0, a1" : "=r"(osp));
		osp -= 16;
		xtensa_cohere_stacks_cache_flush(osp, osp + 16);

#ifdef CONFIG_USERSPACE
		if (o_psp_end == NULL)
#endif
		{
			xtensa_cohere_stacks_cache_invd(ostack, osp);

			flush_end = oend;
		}
	}

#ifdef CONFIG_USERSPACE
	/* User threads need a bit more processing due to having
	 * privileged stack for handling syscalls. The privileged
	 * stack always immediately precedes the thread stack.
	 *
	 * Note that, with userspace enabled, we need to swap
	 * page table during context switch via function calls.
	 * This means that the stack is being actively used
	 * unlike the non-userspace case mentioned above.
	 * Therefore we need to set ZSR_FLUSH_STR to make sure
	 * we flush the cached data in the stack.
	 */
	if (o_psp_end != NULL) {
		/* Start of old_thread privileged stack.
		 *
		 * struct xtensa_thread_stack_header wholly contains
		 * a array for the privileged stack, so we can use
		 * its size to calculate where the start is.
		 */
		size_t o_psp_start = (size_t)o_psp_end - sizeof(struct xtensa_thread_stack_header);

		if ((osp >= ostack) && (osp < oend)) {
			/* osp in user stack. */
			xtensa_cohere_stacks_cache_invd(o_psp_start, osp);

			flush_end = oend;
		} else if ((osp >= o_psp_start) && (osp < ostack)) {
			/* osp in privileged stack. */
			xtensa_cohere_stacks_cache_flush(ostack, oend);
			xtensa_cohere_stacks_cache_invd(o_psp_start, osp);

			flush_end = (size_t)old_thread->arch.psp;
		}
	}
#endif /* CONFIG_USERSPACE */

	flush_end = ROUND_DOWN(flush_end, XCHAL_DCACHE_LINESIZE);
	__asm__ volatile("wsr %0, " ZSR_FLUSH_STR :: "r"(flush_end));

#endif /* !CONFIG_SCHED_CPU_MASK_PIN_ONLY */
}
#endif

static inline bool arch_is_in_isr(void)
{
	uint32_t nested;

#if defined(CONFIG_SMP)
	/*
	 * Lock interrupts on SMP to ensure that the caller does not migrate
	 * to another CPU before we get to read the nested field.
	 */
	unsigned int key;

	key = arch_irq_lock();
#endif

	nested = arch_curr_cpu()->nested;

#if defined(CONFIG_SMP)
	arch_irq_unlock(key);
#endif

	return nested != 0U;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_FUNC_H_ */
