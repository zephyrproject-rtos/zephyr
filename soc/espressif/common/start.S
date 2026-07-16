/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Boot entry point. The ROM enters here on a stack of its own choosing, which
 * on some layouts overlaps .bss, so this must run before any compiler-generated
 * prologue can spill to it. The stack set up here is scratch, used only until
 * z_cstart() installs the kernel stack.
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/csr.h>
#include <soc/soc_caps.h>
#include "memory.h"

#define CSR_MTVT 0x307

GTEXT(start_riscv)
GTEXT(_vector_table)
#if SOC_INT_CLIC_SUPPORTED
GTEXT(_mtvt_table)
#endif

GTEXT(__start)

	.section .iram1, "ax"
	.balign 4
	.global __start
	.type __start, @function

__start:
#ifdef CONFIG_RISCV_GP
	/* Must precede any access the linker could relax to gp-relative. */
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop
#endif

	li sp, DRAM_STACK_START

	csrci mstatus, MSTATUS_MIE

	la t0, _vector_table
	csrw mtvec, t0

#if SOC_INT_CLIC_SUPPORTED
	la t0, _mtvt_table
	csrw CSR_MTVT, t0
#endif

	tail start_riscv

	.size __start, . - __start
