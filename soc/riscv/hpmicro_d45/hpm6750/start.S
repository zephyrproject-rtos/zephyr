/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/toolchain.h>
#include "hpm_csr_regs.h"

GTEXT(_start)

SECTION_FUNC(init, _start)
	/* Disable linker relaxation before GP register initialization. */
	.option push
	.option norelax
	/* reset mstatus to 0*/
	csrrw x0, mstatus, x0

#ifdef __nds_execit
	/* Initialize EXEC.IT table */
	la t0, _ITB_BASE_
	csrw uitb, t0
#endif

#ifdef __riscv_flen
	/* Enable FPU */
	li t0, CSR_MSTATUS_FS_MASK
	csrrs t0, mstatus, t0

	/* Initialize FCSR */
	fscsr zero
#endif
	call _init_noncache
	/* System reset handler */
	call __start
	.option pop
