/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Charlie, Xingyu Hu and etc.;
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/zvm/arm/asm.h>

#include "../include/zvm_offsets_short_arch.h"
#include "../core/macro_priv.inc"

_ASM_FILE_PROLOGUE

.macro save_registers_context base
    stp x0, x1,   [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x0_x1]
    stp x2, x3,   [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x2_x3]
    stp x4, x5,   [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x4_x5]
    stp x6, x7,   [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x6_x7]
    stp x8, x9,   [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x8_x9]
    stp x10, x11, [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x10_x11]
    stp x12, x13, [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x12_x13]
    stp x14, x15, [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x14_x15]
	stp x16, x17, [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x16_x17]
    stp x18, lr,  [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x18_lr]

    stp x19, x20, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x19_20]
    stp x21, x22, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x21_x22]
    stp x23, x24, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x23_x24]
    stp x25, x26, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x25_x26]
    stp x27, x28, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x27_x28]

    mrs x4, sp_el0
    stp x29, x4,  [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x29_sp_el0]
    mrs x4, sp_el1
    str x4,       [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_sp_elx]
.endm

.macro load_registers_context base
    ldp x0, x1,   [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x0_x1]
    ldp x2, x3,   [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x2_x3]
    ldp x4, x5,   [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x4_x5]
    ldp x6, x7,   [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x6_x7]
    ldp x8, x9,   [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x8_x9]
    ldp x10, x11, [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x10_x11]
    ldp x12, x13, [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x12_x13]
    ldp x14, x15, [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x14_x15]
    ldp x16, x17, [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x16_x17]
    ldp x18, lr,  [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x18_lr]

    ldp x19, x20, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x19_20]
    ldp x21, x22, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x21_x22]
    ldp x23, x24, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x23_x24]
    ldp x25, x26, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x25_x26]
    ldp x27, x28, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x27_x28]

    ldr x4, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_sp_elx]
    msr sp_el1, x4
    ldr x4, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x29_sp_el0 + 0x08]
    msr sp_el0, x4
	ldr x4, [\base, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x4_x5]
	ldr x29, [\base, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x29_sp_el0]
.endm

/**
 * @brief VM entry function, where switch to vm context.
 * @x0: vcpu info.
 * @x1: host cpu context
 */
GTEXT(guest_vm_entry)
SECTION_SUBSEC_FUNC(TEXT, __hyp_section, guest_vm_entry)

	stp	x16, x17, [x1, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x16_x17]
    /* If there are some errors that ready to process, return! */
    mrs	x16, isr_el1
	cbz	x16, no_host_isr
	mov	x0, #ARM_VM_EXCEPTION_IRQ
	ldp	x16, x17, [x1, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x16_x17]
	ret

no_host_isr:
	/* save hyp context */
    save_registers_context x1

	/* load guest context */
    add x29, x0, 	#_vcpu_arch_to_ctxt
    ldr x29, 		[x29]
    load_registers_context x29
	isb
    eret


/**
 * @brief VM exit function, where switch to hypervisor context.
 * @x0: vcpu info.
 * @x1: the exception type
 */
GTEXT(guest_vm_exit)
SECTION_SUBSEC_FUNC(TEXT, __hyp_section, guest_vm_exit)
	add x0, x0, #_vcpu_arch_to_ctxt
	ldr x0, [x0]

	ldp x18, lr,  [sp], #16

	/* store guest context */
	save_registers_context x0
	ldp x4, x5,   [sp], #16
	stp	x4, x5,   [x0, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x0_x1]

	/* irq exit? jump over! */
	cmp x1, #ARM_VM_EXCEPTION_IRQ
	b.eq vm_isr_in_sync
    /* If there are irq that need to process when sync occur! */
    mrs	x0, isr_el1
	cbz	x0, vm_isr_in_sync
	mov x1, #ARM_VM_EXCEPTION_IRQ_IN_SYNC
vm_isr_in_sync:
	/* Save exception type for next usage. */
	mov x0, x1
	stp	x0, x1, [sp, #-16]!

	bl get_zvm_host_context
	mov x1, x0

	/* load host context */
    load_registers_context x1
	isb
	ldp x0, x1,   [sp], #16
	ret
