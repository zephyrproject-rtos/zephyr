/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

.text
.globl arch_stack_jump
arch_stack_jump:
	movl	4(%esp), %eax
	movl	8(%esp), %ecx
	movl	12(%esp), %edx
	movl	%eax, %esp
	pushl	%edx
	pushl	$0
	jmp	*%ecx
