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

/* ESP32-C61 CLIC context save/restore.
 *
 * On CLIC the previous interrupt level lives in mcause.mpil and the preemption
 * threshold in mintthresh; mret reloads mintstatus.mil from mcause.mpil only
 * when mcause.interrupt (bit 31) is set. A context switch returns through the
 * exception epilogue to a different thread, so the per-thread mcause and
 * mintthresh are saved on entry and restored on exit.
 *
 * On restore, mcause.interrupt is set to match the entry type of the trap
 * being returned from rather than the one the incoming thread was suspended
 * in: cleared on exception entry (mil is already 0), set on interrupt entry,
 * with mpil cleared for exception-saved frames so they resume at mil 0.
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
	csrr t2, mcause
	bltz t2, 1f
	li t1, ~RISCV_MCAUSE_IRQ_BIT
	and t0, t0, t1
	j 2f
1:
	bltz t0, 2f
	li t1, 0xFF00FFFF
	and t0, t0, t1
	li t1, RISCV_MCAUSE_IRQ_BIT
	or t0, t0, t1
2:	csrw mcause, t0
	ret
