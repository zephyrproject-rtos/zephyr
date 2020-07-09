/*
 * Copyright (c) 2020 RISE Research Institutes of Sweden <www.ri.se>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/cpu.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <sys/util.h>
#include <kernel.h>

#ifdef CONFIG_USERSPACE

GTEXT(arch_user_string_nlen)
GTEXT(z_riscv_userspace_enter)
GTEXT(z_riscv_user_string_nlen_fault_start)
GTEXT(z_riscv_user_string_nlen_fault_end)
GTEXT(z_riscv_user_string_nlen_fixup)
GTEXT(z_riscv_is_user_context)
GTEXT(z_riscv_is_user_context_fault_start)
GTEXT(z_riscv_is_user_context_fault_end)
GTEXT(z_riscv_is_user_context_fixup)

/*
 * This function is the entry point to user mode from privileged execution.
 * The conversion is one way, and threads which transition to user mode do
 * not transition back later, unless they are doing system calls.
 */
SECTION_FUNC(TEXT, z_riscv_userspace_enter)
	li t1, MSTATUS_MPP_M
	csrc mstatus, t1
	li t1, MSTATUS_MPIE_EN
	csrs mstatus, t1
	csrw mepc, ra
	mret

/*
 * size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
 *
 * This function must be implemented for all architectures supporting
 * userspace. A fault may occur during the execution of this function, which
 * must not cause a fatal kernel error. The ##_fault_start, ##_fault_end and
 * ##_fixup labels are used to prevent this. See include/exc_handle.h for more
 * information.
 */
SECTION_FUNC(TEXT, arch_user_string_nlen)
        mv t0, zero

strlen_loop:
z_riscv_user_string_nlen_fault_start:
	/* load byte from given word, might fault */
	lb t1, (a0)

z_riscv_user_string_nlen_fault_end:
	/* if NULL byte, end of string reached */
	beqz t1, strlen_done

	/* if maxsize reached, return -1 */
	beq t0, a1, strlen_fail

	/* continue parsing string; t0 = length, a0 = position */
	addi t0, t0, 1
	addi a0, a0, 1
	j strlen_loop

strlen_done:
	mv a0, t0
	sw zero, (a2)
	ret

strlen_fail:
z_riscv_user_string_nlen_fixup:
	lb t1, (a0)
	li t2, -1
	sw t2, (a2)
	ret

/*
 * Called by arch_is_user_context() to determine the current privilege level.
 * This information is not directly accessible from standard RISC-V hardware.
 * As a workaround, this implementation attempts to read a CSR which is only
 * accessible to machine mode. If this does not raise an exception, the current
 * execution state is machine mode.
 */
SECTION_FUNC(TEXT, z_riscv_is_user_context)

z_riscv_is_user_context_fault_start:
	csrr t0, mstatus

z_riscv_is_user_context_fault_end:
	li a0, 0
	ret

z_riscv_is_user_context_fixup:
	li a0, 1
	ret

#endif /* CONFIG_USERSPACE */
