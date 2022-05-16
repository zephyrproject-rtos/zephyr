/*
 * Userspace and service handler hooks
 *
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/riscv/csr.h>
#include "asm_macros.inc"

/* exports */
GTEXT(arch_user_string_nlen)
GTEXT(z_riscv_user_string_nlen_fault_start)
GTEXT(z_riscv_user_string_nlen_fault_end)
GTEXT(z_riscv_user_string_nlen_fixup)

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
