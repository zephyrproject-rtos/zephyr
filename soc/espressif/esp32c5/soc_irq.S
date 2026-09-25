/* Copyright 2026 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/irq.h>
#include <riscv/csr_clic.h>

/* Exports */
GTEXT(__soc_handle_irq)
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)

SECTION_FUNC(exception.other, __soc_handle_irq)

	/* int status clearing is done at ISR */
	ret

/*
 * ESP32-C5 CLIC context save/restore.
 *
 * On CLIC the previous interrupt level lives in mcause.mpil and the preemption
 * threshold in mintthresh; mret reloads mintstatus.mil from mcause.mpil only
 * when mcause.interrupt (bit 31) is set. A context switch returns through the
 * exception epilogue to a different thread, so the per-thread mcause and
 * mintthresh are saved on entry and restored on exit.
 *
 * The saved mcause cannot be written back verbatim: mcause.interrupt must
 * describe the trap being returned from, not the one the incoming thread was
 * suspended in. When a thread interrupted in user mode is resumed from an
 * ecall-entered trap (or the reverse), a mismatched mret leaves the core in a
 * state where the next trap taken from user mode is a double trap and the
 * chip resets with CPU_LOCKUP. Hence mcause.interrupt is forced to match the
 * live entry type: cleared on exception entry (mil is already 0), set on
 * interrupt entry, with mpil cleared for exception-saved frames so they
 * resume at mil 0.
 */

SECTION_FUNC(exception.other, __soc_save_context)

	csrr t0, mcause
	sw t0, __soc_esf_t_mcause_OFFSET(a0)
	csrr t0, MINTTHRESH_CSR
	sw t0, __soc_esf_t_mintthresh_OFFSET(a0)
	ret

SECTION_FUNC(exception.other, __soc_restore_context)

	lw t0, __soc_esf_t_mintthresh_OFFSET(a0)
	csrw MINTTHRESH_CSR, t0
	lw t0, __soc_esf_t_mcause_OFFSET(a0)
	/* live mcause = entry type of this trap */
	csrr t2, mcause
	bltz t2, 1f
	/* Exception-entered: keep mcause.interrupt clear; mil stays 0. */
	li t1, ~RISCV_MCAUSE_IRQ_BIT
	and t0, t0, t1
	j 2f
1:	/* Interrupt-entered: set mcause.interrupt so mret reloads mil from
	 * mpil; exception-saved frames resume at mil 0.
	 */
	bltz t0, 2f
	li t1, 0xFF00FFFF
	and t0, t0, t1
	li t1, RISCV_MCAUSE_IRQ_BIT
	or t0, t0, t1
2:	csrw mcause, t0
	ret
