/*
 * Userspace and service handler hooks
 *
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <syscall.h>
#include <kernel_structs.h>
#include <arch/riscv/csr.h>

/* exports */
GTEXT(z_riscv_do_syscall)
GTEXT(arch_user_string_nlen)
GTEXT(z_riscv_user_string_nlen_fault_start)
GTEXT(z_riscv_user_string_nlen_fault_end)
GTEXT(z_riscv_user_string_nlen_fixup)
GTEXT(z_riscv_do_syscall_start)
GTEXT(z_riscv_do_syscall_end)

/* Imports */
GDATA(_k_syscall_table)

SECTION_FUNC(TEXT,z_riscv_do_syscall)
	la	t0, _k_syscall_table

	slli	t1, a7, RV_REGSHIFT	# Determine offset from indice value
	add	t0, t0, t1		# Table addr + offset = function addr
	RV_OP_LOADREG t3, 0x00(t0)	# Load function address

	/* Execute syscall function */
	jalr 	t3

	/* Return to ISR environment to switch-back in user mode */
z_riscv_do_syscall_start:
	ECALL
z_riscv_do_syscall_end:

/*
 * size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
 */
SECTION_FUNC(TEXT, arch_user_string_nlen)
	li	a5, 0		# Counter
	sw	a5, 0(a2)	# Init error value to 0

loop:
	add	a4, a0, a5	# Determine character address
z_riscv_user_string_nlen_fault_start:
	lbu	a4, 0(a4)	# Load string's character
z_riscv_user_string_nlen_fault_end:
	beqz	a4, exit	# Test string's end of line

	bne	a5, a1, continue	# Check if max length is reached

exit:
	mv	a0, a5		# Return counter value (length)
	ret

continue:
	addi	a5, a5, 1	# Increment counter
	j	loop

z_riscv_user_string_nlen_fixup:
	li	a4, -1		# Put error to -1
	sw	a4, 0(a2)
	j	exit
