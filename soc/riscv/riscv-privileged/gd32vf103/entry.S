/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/csr.h>
#include <nuclei/nuclei_csr.h>

GTEXT(__nuclei_start)
SECTION_FUNC(vectors, __nuclei_start)
	/* Disable Global Interrupt */
	csrc mstatus, MSTATUS_MIE
	/* Jump to logical address first to ensure correct operation of RAM region  */
	la	a0, __nuclei_start
	li	a1, 1
	slli	a1, a1, 29
	bleu	a1, a0, _start0800
	srli	a1, a1, 2
	bleu	a1, a0, _start0800
	la	a0, _start0800
	add	a0, a0, a1
	jr	a0

_start0800:

#if defined(CONFIG_RISCV_GP)
	/* Initialize global pointer */
	.option push
	.option norelax
	la	gp, __global_pointer$
	.option pop
#endif

	.option norvc;

	/* Set the the NMI base to share with mtvec by setting CSR_MMISC_CTL */
	li	t0, 0x200
	csrs	CSR_MMISC_CTL, t0

	/* Initial the CSR MTVEC for the Trap ane NMI base addr */
	la	t0, trap_entry
	csrw	mtvec, t0

	/* Direct Mode: All exceptions set pc to BASE. */
	csrc	mtvec, 0x3

	/* Disable performance counter */
	csrsi	mcountinhibit, 0x5

	/* Jump to __reset */
	tail __reset

1:
	j 1b

.align 6
trap_entry:
	tail _isr_wrapper
