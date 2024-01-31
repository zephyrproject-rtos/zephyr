/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nuclei_csr.h"

#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/csr.h>

GTEXT(__nuclei_start)
SECTION_FUNC(init, __nuclei_start)
	/* Disable Global Interrupt */
	csrc mstatus, MSTATUS_MIE
	/* Jump to logical address first to ensure correct operation of RAM region  */
	la	a0, __nuclei_start
	li	a1, 1
	slli	a1, a1, 29 # 0x2000 0000
	bleu	a1, a0, _start0800
	srli	a1, a1, 2  # 0x0800 0000
	bleu	a1, a0, _start0800
	la	a0, _start0800
	add	a0, a0, a1
	jr	a0

_start0800:
	/* Set the the NMI base to share with mtvec by setting CSR_MMISC_CTL */
	li	t0, 0x200
	csrs	CSR_MMISC_CTL, t0

	/* Disable performance counter */
	csrsi	mcountinhibit, 0x5

	/* Jump to common start */
	tail __start
