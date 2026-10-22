/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_ESP32C5_SOC_SYSCALL_H
#define SOC_RISCV_ESP32C5_SOC_SYSCALL_H

#ifdef _ASMLANGUAGE

#include <riscv/csr_clic.h>

/* A syscall is an ECALL exception. On these CLIC parts, an interrupt or
 * another asynchronously reported event (the load access fault included)
 * taken while the exception is still open locks the CPU up, with no fault
 * delivered to software. Precise synchronous traps, such as the M-mode ECALL
 * used to switch out of the syscall body, nest normally. The kernel needs
 * the syscall body to run with mstatus.MIE set, so interrupts cannot be
 * masked that way. Raise the CLIC preemption threshold (mintthresh) to the
 * highest interrupt level instead: every interrupt level is then at or below
 * the threshold and none is taken, while MIE stays set. The per-context
 * mintthresh is saved and restored across the syscall by __soc_save_context /
 * __soc_restore_context (RISCV_SOC_CONTEXT_SAVE).
 *
 * mintthresh holds the level in its upper NLBITS bits with the remaining low
 * bits set to one. CLIC_INT_THRESH() from the HAL builds that encoding and
 * NLBITS_MASK is the highest level the core implements.
 *
 * The IRQ wrapper invokes this where the caller-saved temporaries are free, so
 * the macro uses t0 as scratch.
 */

/* clang-format off */
.macro SOC_SYSCALL_INTMASK
	li t0, CLIC_INT_THRESH(NLBITS_MASK)
	csrw MINTTHRESH_CSR, t0
.endm
/* clang-format on */

#endif /* _ASMLANGUAGE */

#endif /* SOC_RISCV_ESP32C5_SOC_SYSCALL_H */
