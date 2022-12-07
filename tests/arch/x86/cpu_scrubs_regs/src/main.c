/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/ztest.h>
#include "test_syscalls.h"

#define DB_VAL 0xDEADBEEF

void z_impl_test_cpu_write_reg(void)
{
	/* User thread CPU write registers system call for testing
	*
	* Verify the following
	* - Write 0xDEADBEEF values during system call into registers
	* - In main test we will read that registers to verify
	* that all of them were scrubbed and do not contain any sensitive data
	*/

	/* Part below is made to test that kernel scrubs CPU registers
	 * after returning from the system call
	 */
#ifndef CONFIG_X86_64
	__asm__ volatile (
		"movl $0xDEADBEEF, %%eax;\n\t"
		"movl $0xDEADBEEF, %%ebx;\n\t"
		"movl $0xDEADBEEF, %%ecx;\n\t"
		"movl $0xDEADBEEF, %%edx;\n\t"
		"movl $0xDEADBEEF, %%edi;\n\t"
		: : : "eax", "ebx", "ecx", "edx", "edi"
		);
#else
	__asm__ volatile (
		"movq $0xDEADBEEF, %%rax;\n\t"
		"movq $0xDEADBEEF, %%rcx;\n\t"
		"movq $0xDEADBEEF, %%rdx;\n\t"
		"movq $0xDEADBEEF, %%rsi;\n\t"
		"movq $0xDEADBEEF, %%rdi;\n\t"
		"movq $0xDEADBEEF, %%r8;\n\t"
		"movq $0xDEADBEEF, %%r9;\n\t"
		"movq $0xDEADBEEF, %%r10;\n\t"
		"movq $0xDEADBEEF, %%r11;\n\t"
		: : : "rax", "rcx", "rdx", "rsi", "rdi",
		      "r8",  "r9",  "r10", "r11"
		);
#endif
}

static inline void z_vrfy_test_cpu_write_reg(void)
{
	z_impl_test_cpu_write_reg();
}
#include <syscalls/test_cpu_write_reg_mrsh.c>

/**
 * @brief Test CPU scrubs registers after system call
 *
 * @details - Call from user mode a syscall test_x86_cpu_write_reg(),
 * the system call function writes into registers 0xDEADBEEF value
 * - Then in main test function below check registers values,
 * if no 0xDEADBEEF value detected, that means CPU scrubbed registers
 * before exit from the system call.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(x86_cpu_scrubs_regs, test_syscall_cpu_scrubs_regs)
{
#ifndef CONFIG_X86_64
	int x86_reg_val[5];

	test_cpu_write_reg();
	__asm__ volatile (
		"\t movl %%eax,%0" : "=r"(x86_reg_val[0]));
	__asm__ volatile (
		"\t movl %%ebx,%0" : "=r"(x86_reg_val[1]));
	__asm__ volatile (
		"\t movl %%ecx,%0" : "=r"(x86_reg_val[2]));
	__asm__ volatile (
		"\t movl %%edx,%0" : "=r"(x86_reg_val[3]));
	__asm__ volatile (
		"\t movl %%edi,%0" : "=r"(x86_reg_val[4]));

	for (int i = 0; i < 5; i++) {
		zassert_not_equal(x86_reg_val[i], DB_VAL,
				"reg val is 0xDEADBEEF, "
				"not scrubbed after system call.");
	}
#else
	long x86_64_reg_val[9];

	test_cpu_write_reg();

	__asm__ volatile(
		"\t movq %%rax,%0" : "=r"(x86_64_reg_val[0]));
	__asm__ volatile(
		"\t movq %%rcx,%0" : "=r"(x86_64_reg_val[1]));
	__asm__ volatile(
		"\t movq %%rdx,%0" : "=r"(x86_64_reg_val[2]));
	__asm__ volatile(
		"\t movq %%rsi,%0" : "=r"(x86_64_reg_val[3]));
	__asm__ volatile(
		"\t movq %%rdi,%0" : "=r"(x86_64_reg_val[4]));
	__asm__ volatile(
		"\t movq %%r8,%0" : "=r"(x86_64_reg_val[5]));
	__asm__ volatile(
		"\t movq %%r9,%0" : "=r"(x86_64_reg_val[6]));
	__asm__ volatile(
		"\t movq %%r10,%0" : "=r"(x86_64_reg_val[7]));
	__asm__ volatile(
		"\t movq %%r11,%0" : "=r"(x86_64_reg_val[8]));

	for (int i = 0; i < 9; i++) {
		zassert_not_equal(x86_64_reg_val[i], DB_VAL,
				"register value is 0xDEADBEEF, "
				"not scrubbed after system call.");
	}
#endif
}

ZTEST_SUITE(x86_cpu_scrubs_regs, NULL, NULL, NULL, NULL, NULL);
