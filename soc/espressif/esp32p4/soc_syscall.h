/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_RISCV_ESP32P4_SOC_SYSCALL_H
#define SOC_RISCV_ESP32P4_SOC_SYSCALL_H

#ifdef _ASMLANGUAGE

#include <riscv/csr_clic.h>

/* Mask all interrupt levels through mintthresh, keeping mstatus.MIE set.
 * Clobbers t0.
 */

/* clang-format off */
.macro SOC_SYSCALL_INTMASK
	li t0, CLIC_INT_THRESH(NLBITS_MASK)
	csrw MINTTHRESH_CSR, t0
.endm
/* clang-format on */

#endif /* _ASMLANGUAGE */

#endif /* SOC_RISCV_ESP32P4_SOC_SYSCALL_H */
