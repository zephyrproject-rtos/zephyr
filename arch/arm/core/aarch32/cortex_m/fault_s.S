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

#include <toolchain.h>
#include <linker/sections.h>

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
 * as parameters to the z_arm_fault() C function that will perform the
 * rest of the fault handling (i.e. z_arm_fault(MSP, PSP, EXC_RETURN)).

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
	mov r2, lr /* EXC_RETURN */

	push {r0, lr}

	bl z_arm_fault

	pop {r0, pc}

	.end
