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
#include <kernel_arch_data.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void FatalErrorHandler(void);
extern void ReservedInterruptHandler(unsigned int intNo);
extern void z_xtensa_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

/* Defined in xtensa_context.S */
extern void z_xt_coproc_init(void);

extern K_KERNEL_STACK_ARRAY_DEFINE(z_interrupt_stacks, CONFIG_MP_NUM_CPUS,
				   CONFIG_ISR_STACK_SIZE);

static ALWAYS_INLINE void arch_kernel_init(void)
{
	_cpu_t *cpu0 = &_kernel.cpus[0];

#ifdef CONFIG_KERNEL_COHERENCE
	/* Make sure we don't have live data for unexpected cached
	 * regions due to boot firmware
	 */
	xthal_dcache_all_writeback_inv();
#endif

	cpu0->nested = 0;

	/* The asm2 scheme keeps the kernel pointer in MISC0 for easy
	 * access.  That saves 4 bytes of immediate value to store the
	 * address when compared to the legacy scheme.  But in SMP
	 * this record is a per-CPU thing and having it stored in a SR
	 * already is a big win.
	 */
	WSR(CONFIG_XTENSA_KERNEL_CPU_PTR_SR, cpu0);

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

/* FIXME: we don't have a framework for including this from the SoC
 * layer, so we define it in the arch code here.
 */
#if defined(CONFIG_SOC_FAMILY_INTEL_ADSP) && defined(CONFIG_KERNEL_COHERENCE)
static inline bool arch_mem_coherent(void *ptr)
{
	size_t addr = (size_t) ptr;

	return addr >= 0x80000000 && addr < 0xa0000000;
}
#endif

#ifdef CONFIG_KERNEL_COHERENCE
static inline void arch_cohere_stacks(struct k_thread *old_thread,
				      void *old_switch_handle,
				      struct k_thread *new_thread)
{
	size_t ostack = old_thread->stack_info.start;
	size_t osz    = old_thread->stack_info.size;
	size_t osp    = (size_t) old_switch_handle;

	size_t nstack = new_thread->stack_info.start;
	size_t nsz    = new_thread->stack_info.size;
	size_t nsp    = (size_t) new_thread->switch_handle;

	xthal_dcache_region_invalidate((void *)nsp, (nstack + nsz) - nsp);

	/* FIXME: dummy initializion threads don't have stack info set
	 * up and explode the logic above.  Find a way to get this
	 * test out of the hot paths!
	 */
	if (old_thread->base.thread_state & _THREAD_DUMMY) {
		return;
	}

	/* In interrupt context, we have a valid frame already from
	 * the interrupt entry code, but for arch_switch() that hasn't
	 * happened yet.  It will do the flush itself, we just have to
	 * calculate the boundary for it.
	 */
	if (old_switch_handle != NULL) {
		xthal_dcache_region_writeback((void *)osp,
					      (ostack + osz) - osp);
	} else {
		/* FIXME: hardcoding EXCSAVE3 is bad, should be
		 * configurable a-la XTENSA_KERNEL_CPU_PTR_SR.
		 */
		uint32_t end = ostack + osz;

		__asm__ volatile("wsr.EXCSAVE3 %0" :: "r"(end));
	}
}
#endif

#ifdef __cplusplus
}
#endif

static inline bool arch_is_in_isr(void)
{
	return arch_curr_cpu()->nested != 0U;
}

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_KERNEL_ARCH_FUNC_H_ */
