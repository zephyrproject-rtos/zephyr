/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Reset handler
 *
 * Reset handler that prepares the system for running C code.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <swap_macros.h>

GDATA(_interrupt_stack)
GDATA(z_main_stack)
GDATA(_VectorTable)

/* use one of the available interrupt stacks during init */


#define INIT_STACK _interrupt_stack
#define INIT_STACK_SIZE CONFIG_ISR_STACK_SIZE

GTEXT(__reset)
GTEXT(__start)

/**
 *
 * @brief Reset vector
 *
 * Ran when the system comes out of reset. The processor is at supervisor level.
 *
 * Locking interrupts prevents anything from interrupting the CPU.
 *
 * When these steps are completed, jump to _PrepC(), which will finish setting
 * up the system for running C code.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT,__reset)
SECTION_FUNC(TEXT,__start)
	/* lock interrupts: will get unlocked when switch to main task
	 * also make sure the processor in the correct status
	 */
	mov_s r0, 0
	kflag r0

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	sflag r0
#endif

#if defined(CONFIG_BOOT_TIME_MEASUREMENT) && defined(CONFIG_ARCV2_TIMER)
	/*
	 * ARCV2 timer (timer0) is a free run timer, let it start to count
	 * here.
	 */
	mov_s r0, 0xffffffff
	sr r0, [_ARC_V2_TMR0_LIMIT]
	mov_s r0, 0
	sr r0, [_ARC_V2_TMR0_COUNT]
#endif
	/* interrupt related init */
#ifndef CONFIG_ARC_NORMAL_FIRMWARE
	/* IRQ_ACT and IRQ_CTRL should be initialized and set in secure mode */
	sr r0, [_ARC_V2_AUX_IRQ_ACT]
	sr r0, [_ARC_V2_AUX_IRQ_CTRL]
#endif
	sr r0, [_ARC_V2_AUX_IRQ_HINT]

	/* set the vector table base early,
	 * so that exception vectors can be handled.
	 */
	mov_s r0, _VectorTable
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	sr r0, [_ARC_V2_IRQ_VECT_BASE_S]
#else
	sr r0, [_ARC_V2_IRQ_VECT_BASE]
#endif

#if defined(CONFIG_USERSPACE)
	lr r0, [_ARC_V2_STATUS32]
	bset r0, r0, _ARC_V2_STATUS32_US_BIT
	kflag r0
#endif

#ifdef CONFIG_ARC_USE_UNALIGNED_MEM_ACCESS
	lr r0, [_ARC_V2_STATUS32]
	bset r0, r0, _ARC_V2_STATUS32_AD_BIT
	kflag r0
#endif

	mov_s r1, 1

invalidate_and_disable_icache:

	lr r0, [_ARC_V2_I_CACHE_BUILD]
	and.f r0, r0, 0xff
	bz.nd invalidate_dcache

	mov_s r2, 0
	sr r2, [_ARC_V2_IC_IVIC]
	/* writing to IC_IVIC needs 3 NOPs */
	nop_s
	nop_s
	nop_s
	sr r1, [_ARC_V2_IC_CTRL]

invalidate_dcache:

	lr r3, [_ARC_V2_D_CACHE_BUILD]
	and.f r3, r3, 0xff
	bz.nd done_cache_invalidate

	sr r1, [_ARC_V2_DC_IVDC]

done_cache_invalidate:

#if defined(CONFIG_SYS_POWER_DEEP_SLEEP_STATES) && \
	!defined(CONFIG_BOOTLOADER_CONTEXT_RESTORE)
        jl @_sys_resume_from_deep_sleep
#endif

#ifdef CONFIG_SMP
	_get_cpu_id r0
	breq r0, 0, _master_core_startup

/*
 * Non-masters wait for master core (core 0) to boot enough
 */
_slave_core_wait:
	ld r1, [arc_cpu_wake_flag]
	brne r0, r1, _slave_core_wait

	/* signal master core that slave core runs */
	st 0, [arc_cpu_wake_flag]

	/* get sp set by master core */
	_get_curr_cpu_irq_stack sp

#if defined(CONFIG_ARC_FIRQ_STACK)
	jl z_arc_firq_stack_set
#endif
	j z_arch_slave_start

_master_core_startup:
#endif

#ifdef CONFIG_INIT_STACKS
	/*
	 * use the main stack to call memset on the interrupt stack and the
	 * FIRQ stack when CONFIG_INIT_STACKS is enabled before switching to
	 * one of them for the rest of the early boot
	 */
	mov_s sp, z_main_stack
	add sp, sp, CONFIG_MAIN_STACK_SIZE

	mov_s r0, _interrupt_stack
	mov_s r1, 0xaa
	mov_s r2, CONFIG_ISR_STACK_SIZE
	jl memset

#endif /* CONFIG_INIT_STACKS */

	mov_s sp, INIT_STACK
	add sp, sp, INIT_STACK_SIZE

#if defined(CONFIG_ARC_FIRQ_STACK)
	jl z_arc_firq_stack_set
#endif

	j @_PrepC
