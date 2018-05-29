/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fault handlers for ARM Cortex-M
 *
 * Fault handlers for ARM Cortex-M processors.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(_Fault)

GTEXT(__hard_fault)
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
/* HardFault is used for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
GTEXT(__mpu_fault)
GTEXT(__bus_fault)
GTEXT(__usage_fault)
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
GTEXT(__secure_fault)
#endif /* CONFIG_ARM_SECURE_FIRMWARE*/
GTEXT(__debug_monitor)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
GTEXT(__reserved)

/**
 *
 * @brief Fault handler installed in the fault and reserved vectors
 *
 * Entry point for the hard fault, MPU fault, bus fault, usage fault, debug
 * monitor and reserved exceptions.
 *
 * Save the values of the MSP and PSP in r0 and r1 respectively, so the first
 * and second parameters to the _Fault() C function that will handle the rest.
 * This has to be done because at this point we do not know if the fault
 * happened while handling an exception or not, and thus the ESF could be on
 * either stack. _Fault() will find out where the ESF resides.
 *
 * Provides these symbols:
 *
 *   __hard_fault
 *   __mpu_fault
 *   __bus_fault
 *   __usage_fault
 *   __secure_fault
 *   __debug_monitor
 *   __reserved
 */

SECTION_SUBSEC_FUNC(TEXT,__fault,__hard_fault)
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
/* HardFault is used for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
SECTION_SUBSEC_FUNC(TEXT,__fault,__mpu_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,__bus_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,__usage_fault)
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
SECTION_SUBSEC_FUNC(TEXT,__fault,__secure_fault)
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
SECTION_SUBSEC_FUNC(TEXT,__fault,__debug_monitor)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
SECTION_SUBSEC_FUNC(TEXT,__fault,__reserved)

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

#if !defined(CONFIG_ARM_SECURE_FIRMWARE)
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
	 * in Secure state. Instead, we use the EXC_RETURN SPSEL flag.
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
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	/* In ARM Secure firmware, the stack pointer that is retrieved
	 * above points to the Secure stack. However, the exeption may
	 * have occurred in Non-Secure state.
	 * To determine this we need to inspect the EXC_RETURN value
	 * located in the LR. Therefore, we supply the LR value as an
	 * argument to the fault handler.
	 */
	mov r1, lr
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
	push {lr}
	bl _Fault

	pop {pc}

	.end
