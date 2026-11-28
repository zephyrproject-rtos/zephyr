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
#include <zephyr/devicetree.h>
#include <zephyr/version.h>
#include "asm_macros.inc"

#ifdef CONFIG_64BIT
#define REGBYTES 8
#else
#define REGBYTES 4
#endif

#define OFF_RA  (0 * REGBYTES)
#define OFF_T0  (1 * REGBYTES)
#define OFF_T1  (2 * REGBYTES)
#define OFF_T2  (3 * REGBYTES)
#define OFF_A0  (4 * REGBYTES)
#define OFF_A1  (5 * REGBYTES)
#define OFF_A6  (6 * REGBYTES)
#define OFF_A7  (7 * REGBYTES)

#if defined(CONFIG_RISCV_ISA_RV32E)
#error "Internal SBI requires the full RV32I/RV64I register set (uses a6/a7)."
#else
/* Helper macro for stacking/unstacking GPRs */
#define DO_STACK_GPRS(op) \
	RV_E(	op   ra, (OFF_RA)(sp)	);\
	RV_E(	op   t0, (OFF_T0)(sp)	);\
	RV_E(	op   t1, (OFF_T1)(sp)	);\
	RV_E(	op   t2, (OFF_T2)(sp)	);\
	RV_E(	op   a0, (OFF_A0)(sp)	);\
	RV_E(	op   a1, (OFF_A1)(sp)	);\
	RV_I(	op   a6, (OFF_A6)(sp)	);\
	RV_I(	op   a7, (OFF_A7)(sp)	)
#endif

/* MTIMECMP base address from devicetree (hart 0) */
#define MTIMECMP0  DT_REG_ADDR_BY_NAME(DT_INST(0, riscv_machine_timer), mtimecmp)

/* QEMU virt "sifive_test" device: write here to exit or reset QEMU */
#define QEMU_TEST_FINISHER  0x00100000
#define FINISHER_EXIT       0x5555

/* S-mode ecall exception cause */
#define CAUSE_ECALL_SMODE  9

#include <zephyr/arch/riscv/sbi.h>

/* SBI v0.3 introduced SRST. The major version occupies bits 30:24. */
#define ZEPHYR_SBI_SPEC_VERSION 3
/* Provisional 'ZEPH' value pending allocation of a Zephyr SBI implementation ID. */
#define ZEPHYR_SBI_IMPL_ID 0x5a455048

/*
* Small dedicated stack for use inside the M-mode handler.
* S-mode uses its own stack; we need a separate one so we
* do not corrupt it when we enter M-mode.
*/
.section .noinit.mmode_stack
.align 4
m_mode_stack:
    .space CONFIG_RISCV_M_MODE_STACK_SIZE
GDATA(m_mode_stack_top)
m_mode_stack_top:

GTEXT(__m_mode_sbi_handler)
SECTION_FUNC(TEXT, __m_mode_sbi_handler)

    /*
    * mscratch holds the M-mode stack pointer (initialised in reset.S).
    * Atomically swap sp and mscratch: now sp = M-mode stack,
    * mscratch = S-mode sp (saved for later restore).
    */
    csrrw sp, mscratch, sp

    /* Save all registers we will use */
    addi sp, sp, -8 * REGBYTES
    DO_STACK_GPRS(sr)

    csrr t0, mcause

    /* Bit 63 set means interrupt, clear means exception */
    bltz t0, m_mode_handle_interrupt

    /* Exception path: only S-mode ecall (cause == 9) is expected */
    li t1, CAUSE_ECALL_SMODE
    bne t0, t1, m_mode_unhandled

    /* --- SBI ecall from S-mode --- */

    /*
    * Advance mepc past the ecall instruction (4 bytes) so that
    * when we mret, S-mode resumes at the instruction after ecall.
    */
    csrr t0, mepc
    addi t0, t0, 4
    csrw mepc, t0

    /* Dispatch on SBI extension ID in a7 */
    li t0, SBI_EXT_BASE
    beq a7, t0, m_mode_ext_base

    li t0, SBI_EXT_TIME
    beq a7, t0, m_mode_ext_time

#ifdef CONFIG_QEMU_TARGET
    li t0, SBI_EXT_SRST
    beq a7, t0, m_mode_ext_srst
#endif

m_mode_not_supported:
    li a0, SBI_ERR_NOT_SUPPORTED
    sr a0, OFF_A0(sp)
    j m_mode_return

m_mode_ext_base:
    li t0, SBI_FUNC_GET_SPEC_VERSION
    beq a6, t0, m_mode_base_spec_version
    li t0, SBI_FUNC_GET_IMPL_ID
    beq a6, t0, m_mode_base_impl_id
    li t0, SBI_FUNC_GET_IMPL_VERSION
    beq a6, t0, m_mode_base_impl_version
    li t0, SBI_FUNC_PROBE_EXTENSION
    beq a6, t0, m_mode_base_probe
    li t0, SBI_FUNC_GET_MVENDORID
    beq a6, t0, m_mode_base_mvendorid
    li t0, SBI_FUNC_GET_MARCHID
    beq a6, t0, m_mode_base_marchid
    li t0, SBI_FUNC_GET_MIMPID
    bne a6, t0, m_mode_not_supported
    csrr a1, mimpid
    j m_mode_success

m_mode_base_spec_version:
    li a1, ZEPHYR_SBI_SPEC_VERSION
    j m_mode_success

m_mode_base_impl_id:
    li a1, ZEPHYR_SBI_IMPL_ID
    j m_mode_success

m_mode_base_impl_version:
    li a1, KERNEL_VERSION_NUMBER
    j m_mode_success

m_mode_base_mvendorid:
    csrr a1, mvendorid
    j m_mode_success

m_mode_base_marchid:
    csrr a1, marchid
    j m_mode_success

m_mode_base_probe:
    li a1, 1
    li t0, SBI_EXT_BASE
    beq a0, t0, m_mode_success
    li t0, SBI_EXT_TIME
    beq a0, t0, m_mode_success
#ifdef CONFIG_QEMU_TARGET
    li t0, SBI_EXT_SRST
    beq a0, t0, m_mode_success
#endif
    li a1, 0
    j m_mode_success

m_mode_ext_time:
    /* Dispatch on function ID in a6 */
    li t0, SBI_FUNC_SET_TIMER
    bne a6, t0, m_mode_not_supported

    /* a0 = timer deadline (absolute time value) */
    li t0, MTIMECMP0
    /* write new deadline to mtimecmp[0] */
#ifdef CONFIG_64BIT
    sd a0, 0(t0)
#else
    li t1, -1
    sw t1, REGBYTES(t0)  /* avoid false triggering */
    sw a0, 0(t0)
    sw a1, REGBYTES(t0)
#endif /* CONFIG_64BIT */

    /*
    * Clear STIP: S-mode already handled the previous interrupt,
    * no need to re-fire before the new deadline expires.
    */
    li t0, MIP_STIP
    csrc mip, t0

    li a1, 0
m_mode_success:
    li a0, SBI_SUCCESS
    sr a0, OFF_A0(sp)    /* update saved a0/a1 so m_mode_return restores them */
    sr a1, OFF_A1(sp)
    j m_mode_return

#ifdef CONFIG_QEMU_TARGET
m_mode_ext_srst:
    /* Only function 0 (SBI_FUNC_SYSTEM_RESET) is defined */
    li t0, SBI_FUNC_SYSTEM_RESET
    bne a6, t0, m_mode_not_supported
    /* This runtime only implements shutdown. */
    bnez a0, m_mode_not_supported

    /*
     * Write FINISHER_EXIT to the QEMU virt "sifive_test" device.
     * QEMU exits immediately; the loop below is unreachable in practice.
     */
    li t0, QEMU_TEST_FINISHER
    li t1, FINISHER_EXIT
    sw t1, 0(t0)

m_mode_srst_loop:
    wfi
    j m_mode_srst_loop

#endif /* CONFIG_QEMU_TARGET */

m_mode_handle_interrupt:
    /* Clear sign bit to get the raw interrupt cause number */
    slli t0, t0, 1
    srli t0, t0, 1

    li t1, IRQ_M_TIMER
    bne t0, t1, m_mode_unhandled

    /* --- MTIP: machine timer fired --- */

    /* Disable MTIP by pushing mtimecmp far into the future.
     * If we don't do this it will keep firing immediately on mret.
     */
    li t0, MTIMECMP0
    li t1, -1
#ifdef CONFIG_64BIT
    sd t1, 0(t0)
#else
    sw t1, 0(t0)
    sw t1, REGBYTES(t0)
#endif

    /* Forward to S-mode by raising STIP */
    li t0, MIP_STIP
    csrs mip, t0

    j m_mode_return

m_mode_unhandled:
	/*
	 * Unexpected trap in M-mode. We have no recovery path — spin
	 * forever. This will show as a hang in QEMU and signals a fatal
	 * M-mode error during debugging.
	 */
m_mode_unhandled_loop:
	wfi
	j m_mode_unhandled_loop

m_mode_return:
    DO_STACK_GPRS(lr)
    addi sp, sp, 8 * REGBYTES

    /* Restore S-mode sp, put M-mode stack pointer back into mscratch */
    csrrw sp, mscratch, sp

    mret
