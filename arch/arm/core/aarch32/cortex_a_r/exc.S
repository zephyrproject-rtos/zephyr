/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception handlers for ARM Cortex-A and Cortex-R
 *
 * Exception handlers for ARM Cortex-A and Cortex-R processors.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_fault_undef_instruction)
GTEXT(z_arm_fault_prefetch)
GTEXT(z_arm_fault_data)

GTEXT(z_arm_undef_instruction)
GTEXT(z_arm_prefetch_abort)
GTEXT(z_arm_data_abort)

/**
 *
 * @brief Generic exception handler
 *
 * A generic exception handler that simply invokes z_arm_fault() with currently
 * unused arguments.
 *
 * Provides these symbols:
 *
 *   z_arm_undef_instruction
 *   z_arm_prefetch_abort
 *   z_arm_data_abort
 */

SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_undef_instruction)
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_prefetch_abort)
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_data_abort)

	/*
	 * Pass null for the esf to z_arm_fault for now. A future PR will add
	 * better exception debug for Cortex-R that subsumes what esf
	 * provides.
	 */
	mov r0, #0
	bl z_arm_fault

	pop {r0, lr}
	subs pc, lr, #8

	.end
