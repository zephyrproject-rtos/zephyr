/*
* Copyright (c) 2026 Alexios Lyrakis <alexios.lyrakis@gmail.com>
* SPDX-License-Identifier: Apache-2.0
*
* Minimal M-mode SBI runtime.
* Handles machine timer interrupts (MTIP) and SBI ecalls from S-mode.
*/

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/irq.h>

#ifdef CONFIG_RISCV_S_MODE

/* MTIMECMP register for hart 0 */
#define MTIMECMP0  (CLINT_BASE + 0x4000)

/* S-mode ecall exception cause */
#define CAUSE_ECALL_SMODE  9

#include <zephyr/arch/riscv/sbi.h>

/*
* Small dedicated stack for use inside the M-mode handler.
* S-mode uses its own stack; we need a separate one so we
* do not corrupt it when we enter M-mode.
*/
.section .noinit.mmode_stack
.align 4
m_mode_stack:
    .space 256
GDATA(m_mode_stack_top)
m_mode_stack_top:

GTEXT(__mmode_sbi_handler)
SECTION_FUNC(TEXT, __mmode_sbi_handler)

    /*
    * mscratch holds the M-mode stack pointer (initialised in reset.S).
    * Atomically swap sp and mscratch: now sp = M-mode stack,
    * mscratch = S-mode sp (saved for later restore).
    */
    csrrw sp, mscratch, sp

    /* Save all registers we will use */
    addi sp, sp, -80
    sd ra,  0(sp)
    sd t0,  8(sp)
    sd t1, 16(sp)
    sd t2, 24(sp)
    sd a0, 32(sp)
    sd a1, 40(sp)
    sd a6, 48(sp)
    sd a7, 56(sp)

    csrr t0, mcause

    /* Bit 63 set means interrupt, clear means exception */
    bltz t0, m_handle_interrupt

    /* Exception path: only S-mode ecall (cause == 9) is expected */
    li t1, CAUSE_ECALL_SMODE
    bne t0, t1, m_unhandled

    /* --- SBI ecall from S-mode --- */

    /*
    * Advance mepc past the ecall instruction (4 bytes) so that
    * when we mret, S-mode resumes at the instruction after ecall.
    */
    csrr t0, mepc
    addi t0, t0, 4
    csrw mepc, t0

    /* Dispatch on SBI extension ID in a7 */
    li t0, SBI_EXT_TIME
    beq a7, t0, m_ext_time

    /* Unknown extension — a1 is undefined on error per SBI spec */
    li a0, SBI_ERR_NOT_SUPPORTED
    sd a0, 32(sp)
    j m_return

m_ext_time:
    /* Dispatch on function ID in a6 */
    li t0, SBI_FUNC_SET_TIMER
    beq a6, t0, m_set_timer

    /* Unknown function within SBI_EXT_TIME — a1 undefined on error */
    li a0, SBI_ERR_NOT_SUPPORTED
    sd a0, 32(sp)
    j m_return

m_set_timer:
    /* a0 = timer deadline (absolute time value) */
    li t0, MTIMECMP0
    sd a0, 0(t0)     /* write new deadline to mtimecmp[0] */

    /*
    * Clear STIP: S-mode already handled the previous interrupt,
    * no need to re-fire before the new deadline expires.
    */
    li t0, MIP_STIP
    csrc mip, t0

    /* Return success: a0 = SBI_SUCCESS, a1 = 0 (no return value) */
    li a0, SBI_SUCCESS
    li a1, 0
    sd a0, 32(sp)    /* update saved a0/a1 so m_return restores them */
    sd a1, 40(sp)
    j m_return

 m_handle_interrupt:
    /* Clear sign bit to get the raw interrupt cause number */
    slli t0, t0, 1
    srli t0, t0, 1

    li t1, IRQ_M_TIMER
    bne t0, t1, m_unhandled

    /* --- MTIP: machine timer fired --- */

    /*
        * Disable MTIP by pushing mtimecmp far into the future.
        * If we don't do this it will keep firing immediately on mret.
        */
    li t0, MTIMECMP0
    li t1, -1
    sd t1, 0(t0)

    /* Forward to S-mode by raising STIP */
    li t0, MIP_STIP
    csrs mip, t0

    j m_return

m_unhandled:
	/*
	 * Unexpected trap in M-mode. We have no recovery path — spin
	 * forever. This will show as a hang in QEMU and signals a fatal
	 * M-mode error during debugging.
	 */
m_unhandled_loop:
	wfi
	j m_unhandled_loop

m_return:
    ld ra,  0(sp)
    ld t0,  8(sp)
    ld t1, 16(sp)
    ld t2, 24(sp)
    ld a0, 32(sp)
    ld a1, 40(sp)
    ld a6, 48(sp)
    ld a7, 56(sp)
    addi sp, sp, 80

    /* Restore S-mode sp, put M-mode stack pointer back into mscratch */
    csrrw sp, mscratch, sp

    mret

#endif /* CONFIG_RISCV_S_MODE */
