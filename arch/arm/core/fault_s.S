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
#include <sections.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(_Fault)

GTEXT(__hard_fault)
#if defined(CONFIG_ARMV6_M)
#elif defined(CONFIG_ARMV7_M)
GTEXT(__mpu_fault)
GTEXT(__bus_fault)
GTEXT(__usage_fault)
GTEXT(__debug_monitor)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
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
 *   __debug_monitor
 *   __reserved
 */

SECTION_SUBSEC_FUNC(TEXT,__fault,__hard_fault)
#if defined(CONFIG_ARMV6_M)
#elif defined(CONFIG_ARMV7_M)
SECTION_SUBSEC_FUNC(TEXT,__fault,__mpu_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,__bus_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,__usage_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,__debug_monitor)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
SECTION_SUBSEC_FUNC(TEXT,__fault,__reserved)

#if defined(CONFIG_ARMV6_M)
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

#elif defined(CONFIG_ARMV7_M)
	/* force unlock interrupts */
	eors.n r0, r0
	msr BASEPRI, r0

	/* this reimplements _ScbIsNestedExc() */
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
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

	push {lr}
	bl _Fault

	pop {pc}

	.end
