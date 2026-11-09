/*
 * SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

GTEXT(test_switch)
GTEXT(test_svc)

.syntax unified

/* r0/r1: switch handles, r2: stack offset, r3: activate FP context.
 * Only the leaf test_save_context is called with the offset SP. Restore
 * the ABI alignment before returning to the test framework.
 */
SECTION_FUNC(TEXT, test_switch)
	push {r4-r11, lr}
	sub sp, sp, #4
#ifdef CONFIG_FPU
	vpush {s16-s17}
#endif
	mov r4, sp
	mov r7, r3
	sub sp, sp, r2
	sub sp, sp, #8
	ldr r6, =0x89abcdef
	str r6, [sp]
	ldr r8, =0x12345678
	ldr r6, =test_call_sp
	mov r5, sp
	str r5, [r6]
#ifdef CONFIG_FPU
	cmp r7, #0
	beq 1f
	ldr r6, =0x3f800000
	vmov s16, r6
	b 2f
1:
	mrs r6, control
	bic r6, r6, #4
#if defined(CONFIG_ARMV8_M_SE) && !defined(CONFIG_ARM_NONSECURE_FIRMWARE)
	bic r6, r6, #8
#endif
	msr control, r6
	isb
2:
#endif
	bl test_save_context
	mov r5, sp
	ldr r6, =test_restored_sp
	str r5, [r6]
	ldr r6, =test_restored_reg
	str r8, [r6]
	ldr r5, [sp]
	ldr r6, =test_restored_canary
	str r5, [r6]
#ifdef CONFIG_FPU
	mrs r5, control
	ldr r6, =test_restored_control
	str r5, [r6]
	cmp r7, #0
	beq 3f
	vmov r5, s16
	ldr r6, =test_restored_fp
	str r5, [r6]
3:
#endif
	mov sp, r4
#ifdef CONFIG_FPU
	vpop {s16-s17}
#endif
	add sp, sp, #4
	pop {r4-r11, pc}

SECTION_FUNC(TEXT, test_svc)
	push {r3, lr}
	bl test_prepare_restore
	pop {r3, lr}
	ldr r0, =arm_m_cs_ptrs
	str lr, [r0, #8]
	b arm_m_exc_exit
