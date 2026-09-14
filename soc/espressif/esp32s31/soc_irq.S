/* Copyright 2026 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/irq.h>

/* Exports */
GTEXT(__soc_handle_irq)
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)

SECTION_FUNC(exception.other, __soc_handle_irq)

	/* int status clearing is done at ISR */
	ret

/*
 * ESP32-S31 CLIC context save/restore.
 *
 * On CLIC, mret restores mintstatus.mil from mcause.mpil only when
 * mcause.interrupt (bit 31) is set. An mret whose mcause.interrupt does
 * not match the entry type of the trap being returned from corrupts the
 * CLIC state: after a mismatched mret out of an exception-entered trap
 * (e.g. ecall-based context switch), synchronous exceptions are never
 * delivered again.
 *
 * Hence mcause.interrupt is forced to match the entry type: cleared on
 * exception entry (mil is already 0), set on interrupt entry, with mpil
 * cleared for exception-saved frames so they resume at mil 0.
 *
 * The live mcause cannot tell the entry type: an exception nested in an
 * ISR (irq_offload, FPU trap) overwrites it. The entry type of each trap
 * is pushed on a per-CPU bit stack on save and popped on restore.
 */

	.section .bss.__soc_trap_entry_type
	.balign 4
__soc_trap_entry_type:
	.space 4 * CONFIG_MP_MAX_NUM_CPUS

SECTION_FUNC(exception.other, __soc_save_context)

	csrr t0, mcause
	sw t0, __soc_esf_t_mcause_OFFSET(a0)
	/* Push the entry type: 1 = interrupt, 0 = exception. */
	la t1, __soc_trap_entry_type
#if CONFIG_MP_MAX_NUM_CPUS > 1
	csrr t2, mhartid
	slli t2, t2, 2
	add t1, t1, t2
#endif
	lw t2, 0(t1)
	slli t2, t2, 1
	srli t0, t0, RISCV_MCAUSE_IRQ_POS
	or t2, t2, t0
	sw t2, 0(t1)
	/* Save mintthresh (CSR 0x347): per-context interrupt threshold. */
	csrr t0, 0x347
	sw t0, __soc_esf_t_mintthresh_OFFSET(a0)
	ret

SECTION_FUNC(exception.other, __soc_restore_context)

	/* Restore mintthresh (CSR 0x347) for the resumed context. */
	lw t0, __soc_esf_t_mintthresh_OFFSET(a0)
	csrw 0x347, t0
	/* Pop the entry type of the trap being returned from. */
	la t1, __soc_trap_entry_type
#if CONFIG_MP_MAX_NUM_CPUS > 1
	csrr t2, mhartid
	slli t2, t2, 2
	add t1, t1, t2
#endif
	lw t2, 0(t1)
	andi t3, t2, 1
	srli t2, t2, 1
	sw t2, 0(t1)
	lw t0, __soc_esf_t_mcause_OFFSET(a0)
	bnez t3, 1f
	/* Exception-entered: keep mcause.interrupt clear; mil stays 0. */
	li t1, ~RISCV_MCAUSE_IRQ_BIT
	and t0, t0, t1
	j 2f
1:	/* Interrupt-entered: set mcause.interrupt so mret reloads
	 * mil from mpil; exception-saved frames resume at mil 0.
	 */
	bltz t0, 2f
	li t1, 0xFF00FFFF
	and t0, t0, t1
	li t1, RISCV_MCAUSE_IRQ_BIT
	or t0, t0, t1
2:	csrw mcause, t0
	ret
