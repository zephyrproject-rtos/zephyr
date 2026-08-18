/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

.text
.globl arch_stack_jump
arch_stack_jump:
	movq	%rdi, %rsp
	movq	%rdx, %rdi
	pushq	$0
	jmpq	*%rsi
