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
#include <zephyr/arch/xtensa/cache.h>
#include <zsr.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void z_xtensa_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

K_KERNEL_STACK_ARRAY_DECLARE(z_interrupt_stacks, CONFIG_MP_NUM_CPUS,
			     CONFIG_ISR_STACK_SIZE);

static ALWAYS_INLINE void arch_kernel_init(void)
{
	_cpu_t *cpu0 = &_kernel.cpus[0];

#ifdef CONFIG_KERNEL_COHERENCE
	/* Make sure we don't have live data for unexpected cached
	 * regions due to boot firmware
	 */
	z_xtensa_cache_flush_inv_all();
#endif

	cpu0->nested = 0;

	/* The asm2 scheme keeps the kernel pointer in a scratch SR
	 * (see zsr.h for generation specifics) for easy access.  That
	 * saves 4 bytes of immediate value to store the address when
	 * compared to the legacy scheme.  But in SMP this record is a
	 * per-CPU thing and having it stored in a SR already is a big
	 * win.
	 */
	WSR(ZSR_CPU_STR, cpu0);

#ifdef CONFIG_INIT_STACKS
	memset(Z_KERNEL_STACK_BUFFER(z_interrupt_stacks[0]), 0xAA,
	       K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[0]));
#endif
}

void xtensa_switch(void *switch_to, void **switched_from);

static inline void arch_switch(void *switch_to, void **switched_from)
{
	return xtensa_switch(switch_to, switched_from);
}

#ifdef CONFIG_KERNEL_COHERENCE
static ALWAYS_INLINE void arch_cohere_stacks(struct k_thread *old_thread,
					     void *old_switch_handle,
					     struct k_thread *new_thread)
{
	int32_t curr_cpu = _current_cpu->id;

	size_t ostack = old_thread->stack_info.start;
	size_t osz    = old_thread->stack_info.size;
	size_t osp    = (size_t) old_switch_handle;

	size_t nstack = new_thread->stack_info.start;
	size_t nsz    = new_thread->stack_info.size;
	size_t nsp    = (size_t) new_thread->switch_handle;

	if (old_switch_handle != NULL) {
		int32_t a0save;

		__asm__ volatile("mov %0, a0;"
				 "call0 xtensa_spill_reg_windows;"
				 "mov a0, %0"
				 : "=r"(a0save));
	}

	/* The following option ensures that a living thread will never
	 * be executed in a different CPU so we can safely return without
	 * invalidate and/or flush threads cache.
	 */
	if (IS_ENABLED(CONFIG_SCHED_CPU_MASK_PIN_ONLY)) {
		return;
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
		z_xtensa_cache_inv((void *)nsp, (nstack + nsz) - nsp);
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
		z_xtensa_cache_flush((void *)osp, (ostack + osz) - osp);
		z_xtensa_cache_inv((void *)ostack, osp - ostack);
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
		z_xtensa_cache_flush((void *)osp, 1);
		z_xtensa_cache_inv((void *)ostack, osp - ostack);

		uint32_t end = ostack + osz;

		__asm__ volatile("wsr %0, " ZSR_FLUSH_STR :: "r"(end));
	}
}
#endif

static inline bool arch_is_in_isr(void)
{
	return arch_curr_cpu()->nested != 0U;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_FUNC_H_ */
