/* Copyright 2026 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/irq.h>
#include <zephyr/autoconf.h>

/*
 * The per-context interrupt threshold CSR mintthresh (0x347) only exists on
 * ESP32-P4 rev >= v3.0. On pre-v3 silicon accessing it is an illegal
 * instruction, so skip the save/restore there.
 */
#if !defined(CONFIG_SOC_ESP32P4_REV_1_3)
#define SOC_HAS_MINTTHRESH 1
#endif

/* Exports */
GTEXT(__soc_handle_irq)
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)

SECTION_FUNC(exception.other, __soc_handle_irq)

	/* int status clearing is done at ISR */
	ret

/*
 * ESP32-P4 CLIC context save/restore.
 *
 * On CLIC, mret restores mintstatus.mil from mcause.mpil only when
 * mcause.interrupt (bit 31) is set. On ESP32-P4, an mret whose
 * mcause.interrupt does not match the current trap's entry type corrupts
 * the CLIC state: after a mismatched mret out of an exception-entered
 * trap (e.g. ecall-based context switch), the illegal-instruction
 * exception is never delivered again.
 *
 * Hence mcause.interrupt is forced to match the entry type: cleared on
 * exception entry (mil is already 0), set on interrupt entry, with mpil
 * cleared for exception-saved frames so they resume at mil 0.
 */

SECTION_FUNC(exception.other, __soc_save_context)

	csrr t0, mcause
	sw t0, __soc_esf_t_mcause_OFFSET(a0)
#ifdef SOC_HAS_MINTTHRESH
	/* Save mintthresh (CSR 0x347): per-context interrupt threshold. */
	csrr t0, 0x347
	sw t0, __soc_esf_t_mintthresh_OFFSET(a0)
#endif
	ret

SECTION_FUNC(exception.other, __soc_restore_context)

#ifdef SOC_HAS_MINTTHRESH
	/* Restore mintthresh (CSR 0x347) for the resumed context. */
	lw t0, __soc_esf_t_mintthresh_OFFSET(a0)
	csrw 0x347, t0
#endif
	lw t0, __soc_esf_t_mcause_OFFSET(a0)
	/* live mcause = entry type of this trap */
	csrr t2, mcause
	bltz t2, 1f
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
