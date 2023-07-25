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

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <swap_macros.h>
#include <zephyr/arch/arc/asm-compat/assembler.h>
#ifdef CONFIG_ARC_EARLY_SOC_INIT
  #include <soc_ctrl.h>
#endif

GDATA(z_interrupt_stacks)
GDATA(z_main_stack)
GDATA(_VectorTable)

/* use one of the available interrupt stacks during init */


#define INIT_STACK z_interrupt_stacks
#define INIT_STACK_SIZE CONFIG_ISR_STACK_SIZE

GTEXT(__reset)
GTEXT(__start)

/**
 * @brief Reset vector
 *
 * Ran when the system comes out of reset. The processor is at supervisor level.
 *
 * Locking interrupts prevents anything from interrupting the CPU.
 *
 * When these steps are completed, jump to _PrepC(), which will finish setting
 * up the system for running C code.
 */

SECTION_SUBSEC_FUNC(TEXT,_reset_and__start,__reset)
SECTION_SUBSEC_FUNC(TEXT,_reset_and__start,__start)
	/* lock interrupts: will get unlocked when switch to main task
	 * also make sure the processor in the correct status
	 */
	mov_s r0, 0
	kflag r0

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	sflag r0
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
	MOVR r0, _VectorTable
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	sr r0, [_ARC_V2_IRQ_VECT_BASE_S]
#else
	SRR r0, [_ARC_V2_IRQ_VECT_BASE]
#endif

	lr r0, [_ARC_V2_STATUS32]
	bset r0, r0, _ARC_V2_STATUS32_DZ_BIT
	kflag r0

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

/* Invalidate icache */
	lr r0, [_ARC_V2_I_CACHE_BUILD]
	and.f r0, r0, 0xff
	bz.nd done_icache_invalidate

	mov_s r2, 0
	sr r2, [_ARC_V2_IC_IVIC]
	/* writing to IC_IVIC needs 3 NOPs */
	nop_s
	nop_s
	nop_s
done_icache_invalidate:

/* Invalidate dcache */
	lr r3, [_ARC_V2_D_CACHE_BUILD]
	and.f r3, r3, 0xff
	bz.nd done_dcache_invalidate

	mov_s r1, 1
	sr r1, [_ARC_V2_DC_IVDC]

done_dcache_invalidate:

#ifdef CONFIG_ARC_EARLY_SOC_INIT
	soc_early_asm_init_percpu
#endif

	_dsp_extension_probe

/*
 * Init ARC internal architecture state
 * Force to initialize internal architecture state to reset values
 * For scenarios where board hardware is not re-initialized between tests,
 * some settings need to be restored to its default initial states as a
 * substitution of normal hardware reset sequence.
 */
#ifdef CONFIG_INIT_ARCH_HW_AT_BOOT
	/* Set MPU (v4 or v8) registers to default */
#if CONFIG_ARC_MPU_VER == 4 || CONFIG_ARC_MPU_VER == 8
	/* Set default reset value to _ARC_V2_MPU_EN register */
#define ARC_MPU_EN_RESET_VALUE 0x400181C0
	mov_s r1, ARC_MPU_EN_RESET_VALUE
	sr r1, [_ARC_V2_MPU_EN]
	/* Get MPU region numbers */
	lr r3, [_ARC_V2_MPU_BUILD]
	lsr_s r3, r3, 8
	and r3, r3, 0xff
	mov_s r1, 0
	mov_s r2, 0
	/* Set all MPU regions by iterating index */
mpu_regions_reset:
	brge r2, r3, done_mpu_regions_reset
	sr r2, [_ARC_V2_MPU_INDEX]
	sr r1, [_ARC_V2_MPU_RSTART]
	sr r1, [_ARC_V2_MPU_REND]
	sr r1, [_ARC_V2_MPU_RPER]
	add_s r2, r2, 1
	b_s mpu_regions_reset
done_mpu_regions_reset:
#endif
#endif

#if defined(CONFIG_SMP) || CONFIG_MP_MAX_NUM_CPUS  > 1
	_get_cpu_id r0
	breq r0, 0, _master_core_startup

/*
 * Non-masters wait for master core (core 0) to boot enough
 */
_slave_core_wait:
#if CONFIG_MP_MAX_NUM_CPUS == 1
	kflag	1
#endif
	ld r1, [arc_cpu_wake_flag]
	brne r0, r1, _slave_core_wait

	LDR sp, arc_cpu_sp
	/* signal master core that slave core runs */
	st 0, [arc_cpu_wake_flag]

#if defined(CONFIG_ARC_FIRQ_STACK)
	push r0
	jl z_arc_firq_stack_set
	pop r0
#endif
	j z_arc_slave_start

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

	mov_s r0, z_interrupt_stacks
	mov_s r1, 0xaa
	mov_s r2, CONFIG_ISR_STACK_SIZE
	jl memset

#endif /* CONFIG_INIT_STACKS */

	mov_s sp, INIT_STACK
	add sp, sp, INIT_STACK_SIZE

#if defined(CONFIG_ARC_FIRQ_STACK)
	jl z_arc_firq_stack_set
#endif

	j _PrepC
