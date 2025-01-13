/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Charlie, Xingyu Hu and etc.;
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/arm64/tpidrro_el0.h>
#include <zephyr/arch/arm64/cpu.h>
#include <offsets.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/zvm/arm/asm.h>

_ASM_FILE_PROLOGUE

/*
 * +------------------+------------------+-------------------------+
 * |     Address      |  Exception type  |       Description       |
 * +------------------+------------------+-------------------------+
 * | VBAR_ELn + 0x000 | Synchronous      | Current EL with SP0     |
 * |          + 0x080 | IRQ / vIRQ       |                         |
 * |          + 0x100 | FIQ / vFIQ       |                         |
 * |          + 0x180 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x200 | Synchronous      | Current EL with SPx     |
 * |          + 0x280 | IRQ / vIRQ       |                         |
 * |          + 0x300 | FIQ / vFIQ       |                         |
 * |          + 0x380 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x400 | Synchronous      | Lower EL using AArch64  |
 * |          + 0x480 | IRQ / vIRQ       |                         |
 * |          + 0x500 | FIQ / vFIQ       |                         |
 * |          + 0x580 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x600 | Synchronous      | Lower EL using AArch32  |
 * |          + 0x680 | IRQ / vIRQ       |                         |
 * |          + 0x700 | FIQ / vFIQ       |                         |
 * |          + 0x780 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 */

GDATA(_hyp_vector_table)
SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,_hyp_vector_table)
	/* The whole table must be 2K aligned */
	.align 11

	/* Current EL with SP0 / Synchronous */
	.align 7
	b	.
	/* Current EL with SP0 / IRQ */
	.align 7
	b	.
	/* Current EL with SP0 / FIQ */
	.align 7
	b 	.
	/* Current EL with SP0 / SError */
	.align 7
	b	.

    /* Current EL with SPx / Synchronous */
	.align 7
	b	z_arm64_cur_sync_exc
	/* Current EL with SPx / IRQ */
	.align 7
	b	z_arm64_cur_irq_exc
	/* Current EL with SPx / FIQ */
	.align 7
	b 	.
	/* Current EL with SPx / SError */
	.align 7
	b	.

	/* Lower EL using AArch64 / Synchronous */
	.align 7
	b	z_arm64_hyp_sync_exc
	/* Lower EL using AArch64 / IRQ */
	.align 7
	b	z_arm64_hyp_irq_exc
	/* Lower EL using AArch64 / FIQ */
	.align 7
	b	.
	/* Lower EL using AArch64 / SError */
	.align 7
	b	.

	/* Lower EL using AArch32 / Synchronous */
	.align 7
	b	.
	/* Lower EL using AArch32 / IRQ */
	.align 7
	b	.
	/* Lower EL using AArch32 / FIQ */
	.align 7
	b	.
	/* Lower EL using AArch32 / SError */
	.align 7
	b	.


GTEXT(z_arm64_cur_sync_exc)
SECTION_FUNC(TEXT, z_arm64_cur_sync_exc)

	/* @TODO Error handling */
	b .

GTEXT(z_arm64_cur_irq_exc)
SECTION_FUNC(TEXT, z_arm64_cur_irq_exc)
	stp	x0, x1, [sp, #-16]!
	stp	x18, lr, [sp, #-16]!

	mov x0, sp
	bl z_vm_lower_irq_handler

	mov x1, #ARM_VM_EXCEPTION_IRQ
	b guest_vm_exit


GTEXT(z_arm64_hyp_sync_exc)
SECTION_FUNC(TEXT, z_arm64_hyp_sync_exc)
	/* using x0,x1,x18,x30 for store useful value. */
	stp	x0, x1, [sp, #-16]!
	stp	x18, lr, [sp, #-16]!

	mrs x0, esr_el2
	bl z_vm_lower_sync_handler

	mov x1, #ARM_VM_EXCEPTION_SYNC
	b guest_vm_exit

GTEXT(z_arm64_hyp_irq_exc)
SECTION_FUNC(TEXT, z_arm64_hyp_irq_exc)
	stp	x0, x1, [sp, #-16]!
	stp	x18, lr, [sp, #-16]!

	mov x0, sp
	bl z_vm_lower_irq_handler

	mov x1, #ARM_VM_EXCEPTION_IRQ
	b guest_vm_exit
