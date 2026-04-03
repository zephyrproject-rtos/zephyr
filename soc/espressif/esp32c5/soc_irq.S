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
 * ESP32-C5 CLIC context save/restore.
 *
 * On CLIC, mret restores mintstatus.mil from mcause.mpil ONLY when
 * mcause.interrupt (bit 31) is set. With RISCV_ALWAYS_SWITCH_THROUGH_ECALL,
 * context switches use ecall which saves mcause with bit 31 clear. When
 * this mcause is restored before mret, the hardware skips mil restore,
 * leaving it stuck at the ISR level and blocking same-level interrupts.
 *
 * Save: clear mpil for exceptions so mret restores mil to 0.
 * Restore: force bit 31 so mret always restores mil from mpil.
 *
 * Same workaround as Nuclei ECLIC (soc/gd/gd32/gd32vf103/soc_irq.S).
 */

SECTION_FUNC(exception.other, __soc_save_context)

	csrr t0, mcause
	bltz t0, 1f
	li t1, 0xFF00FFFF
	and t0, t0, t1
1:
	sw t0, __soc_esf_t_mcause_OFFSET(a0)
	csrr t0, 0x347
	sw t0, __soc_esf_t_mintthresh_OFFSET(a0)
	ret

SECTION_FUNC(exception.other, __soc_restore_context)

	lw t0, __soc_esf_t_mintthresh_OFFSET(a0)
	csrw 0x347, t0
	lw t0, __soc_esf_t_mcause_OFFSET(a0)
	li t1, 1 << RISCV_MCAUSE_IRQ_POS
	or t0, t0, t1
	csrw mcause, t0
	ret
