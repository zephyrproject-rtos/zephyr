/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fault handlers for ARM Cortex-M
 *
 * Fault handlers for ARM Cortex-M processors.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

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
WTEXT(z_arm_debug_monitor)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
GTEXT(z_arm_exc_spurious)

/**
 *
 * @brief Fault handler installed in the fault vectors
 *
 * Entry point for the HardFault, MemManageFault, BusFault, UsageFault,
 * SecureFault and Debug Monitor exceptions.
 *
 * The function supplies the values of
 * - the MSP
 * - the PSP
 * - the EXC_RETURN value
 * - callee saved register state (r4-r11, psp)
 * as parameters to the z_arm_fault() C function that will perform the
 * rest of the fault handling:
 *    (i.e. z_arm_fault(MSP, PSP, EXC_RETURN, CALLEE_REGS)).
 * Provides these symbols:
 *
 *   z_arm_hard_fault
 *   z_arm_mpu_fault
 *   z_arm_bus_fault
 *   z_arm_usage_fault
 *   z_arm_secure_fault
 *   z_arm_debug_monitor
 *   z_arm_exc_spurious
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
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
SECTION_SUBSEC_FUNC(TEXT,__fault,z_arm_exc_spurious)

	mrs r0, MSP
	mrs r1, PSP
	push {r0, lr}
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/* Build _callee_saved_t. To match the struct
	 * definition we push the psp & then r11-r4
	 */
	push { r1, r2 }
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	mov  r3, r11
	mov  r2, r10
	push {r2, r3}
	mov  r3, r9
	mov  r2, r8
	push {r2, r3}
	push {r4-r7}
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	push {r4-r11}
#endif
	mov  r3, sp /* pointer to _callee_saved_t */
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
	mov r2, lr /* EXC_RETURN */
	bl z_arm_fault
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/* We do not need to restore any register state here
	 * because we did not use any callee-saved registers
	 * in this routine. Therefore, we can just reset
	 * the MSP to its value prior to entering the function
	 */
	add sp, #40
#endif
	pop {r0, pc}

	.end
