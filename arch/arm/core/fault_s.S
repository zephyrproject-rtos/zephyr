/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fault handlers for ARM Cortex-M and Cortex-R
 *
 * Fault handlers for ARM Cortex-M and Cortex-R processors.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_fault)

GTEXT(z_arm_hard_fault)
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
/* HardFault is used for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
GTEXT(z_arm_mpu_fault)
GTEXT(z_arm_bus_fault)
GTEXT(z_arm_usage_fault)
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
GTEXT(z_arm_secure_fault)
#endif /* CONFIG_ARM_SECURE_FIRMWARE*/
GTEXT(z_arm_debug_monitor)
#elif defined(CONFIG_ARMV7_R)
GTEXT(z_arm_undef_instruction)
GTEXT(z_arm_prefetch_abort)
GTEXT(z_arm_data_abort)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
GTEXT(z_arm_reserved)

/**
 *
 * @brief Fault handler installed in the fault and reserved vectors
 *
 * Entry point for the hard fault, MPU fault, bus fault, usage fault, debug
 * monitor and reserved exceptions.
 *
 * Save the values of the MSP and PSP in r0 and r1 respectively, so the first
 * and second parameters to the z_arm_fault() C function that will handle the
 * rest.  This has to be done because at this point we do not know if the fault
 * happened while handling an exception or not, and thus the ESF could be on
 * either stack. z_arm_fault() will find out where the ESF resides.
 *
 * Provides these symbols:
 *
 *   z_arm_hard_fault
 *   z_arm_mpu_fault
 *   z_arm_bus_fault
 *   z_arm_usage_fault
 *   z_arm_secure_fault
 *   z_arm_debug_monitor
 *   z_arm_reserved
 */

SECTION_SUBSEC_FUNC(TEXT,__fault,z_arm_hard_fault)
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
/* HardFault is used for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
SECTION_SUBSEC_FUNC(TEXT,__fault,z_arm_mpu_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,z_arm_bus_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,z_arm_usage_fault)
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
SECTION_SUBSEC_FUNC(TEXT,__fault,z_arm_secure_fault)
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
SECTION_SUBSEC_FUNC(TEXT,__fault,z_arm_debug_monitor)
#elif defined(CONFIG_ARMV7_R)
SECTION_SUBSEC_FUNC(TEXT,__fault,z_arm_undef_instruction)
SECTION_SUBSEC_FUNC(TEXT,__fault,z_arm_prefetch_abort)
SECTION_SUBSEC_FUNC(TEXT,__fault,z_arm_data_abort)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
SECTION_SUBSEC_FUNC(TEXT,__fault,z_arm_reserved)

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	/* force unlock interrupts */
	cpsie i

	/* Use EXC_RETURN state to find out if stack frame is on the
	 * MSP or PSP
	 */
	ldr r0, =0x4
	mov r1, lr
	tst r1, r0
	beq _stack_frame_msp
	mrs r0, PSP
	bne _stack_frame_endif
_stack_frame_msp:
	mrs r0, MSP
_stack_frame_endif:

#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* force unlock interrupts */
	eors.n r0, r0
	msr BASEPRI, r0

#if !defined(CONFIG_ARM_SECURE_FIRMWARE) && \
	!defined(CONFIG_ARM_NONSECURE_FIRMWARE)
	/* this checks to see if we are in a nested exception */
	ldr ip, =_SCS_ICSR
	ldr ip, [ip]
	ands.w ip, #_SCS_ICSR_RETTOBASE

	ite eq			/* is the RETTOBASE bit zero ? */
		mrseq r0, MSP	/* if so, we're not returning to thread mode,
				 * thus this is a nested exception: the stack
				 * frame is on the MSP */
		mrsne r0, PSP	/* if not, we are returning to thread mode, thus
				 *  this is not a nested exception: the stack
				 * frame is on the PSP */
#else
	/* RETTOBASE flag is not banked between security states.
	 * Therefore, we cannot rely on this flag, to obtain the SP
	 * of the current security state.
	 * Instead, we use the EXC_RETURN.SPSEL flag.
	 */
 	ldr r0, =0x4
	mov r1, lr
	tst r1, r0
	beq _s_stack_frame_msp
	mrs r0, PSP
	bne _s_stack_frame_endif
_s_stack_frame_msp:
	mrs r0, MSP
_s_stack_frame_endif:
#endif /* CONFIG_ARM_SECURE_FIRMWARE || CONFIG_ARM_NONSECURE_FIRMWARE */
#elif defined(CONFIG_ARMV7_R)
	/*
	 * Pass null for the esf to z_arm_fault for now.  A future PR will add
	 * better exception debug for Cortex-R  that subsumes what esf
	 * provides.
	 */
	mov r0, #0
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

#if defined(CONFIG_ARM_SECURE_FIRMWARE) || \
	defined(CONFIG_ARM_NONSECURE_FIRMWARE)
	/* The stack pointer that is retrieved above, points to the stack,
	 * where the exception is taken. However, the exeption may have
	 * occurred in the alternative security state.
	 *
	 * To determine this we need to inspect the EXC_RETURN value
	 * located in the LR. Therefore, we supply the LR value as an
	 * argument to the fault handler.
	 */
	mov r1, lr
#endif /* CONFIG_ARM_SECURE_FIRMWARE || CONFIG_ARM_NONSECURE_FIRMWARE */
	push {r0, lr}
	bl z_arm_fault

#if defined(CONFIG_CPU_CORTEX_M)
	pop {r0, pc}
#elif defined(CONFIG_CPU_CORTEX_R)
	pop {r0, lr}
	subs pc, lr, #8
#endif

	.end
