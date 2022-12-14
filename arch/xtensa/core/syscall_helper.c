/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/xtensa/syscall.h>

uintptr_t arch_syscall_invoke6_helper(uintptr_t arg1, uintptr_t arg2,
				      uintptr_t arg3, uintptr_t arg4,
				      uintptr_t arg5, uintptr_t arg6,
				      uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;
	register uintptr_t a5 __asm__("%a5") = arg4;
	register uintptr_t a8 __asm__("%a8") = arg5;
	register uintptr_t a9 __asm__("%a9") = arg6;

	__asm__ volatile("syscall\n\t"
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3), "r" (a4),
			   "r" (a5), "r" (a8), "r" (a9)
			 : "memory");

	return a2;
}

uintptr_t arch_syscall_invoke5_helper(uintptr_t arg1, uintptr_t arg2,
				      uintptr_t arg3, uintptr_t arg4,
				      uintptr_t arg5, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;
	register uintptr_t a5 __asm__("%a5") = arg4;
	register uintptr_t a8 __asm__("%a8") = arg5;

	__asm__ volatile("syscall\n\t"
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3), "r" (a4),
			   "r" (a5), "r" (a8)
			 : "memory");

	return a2;
}

uintptr_t arch_syscall_invoke4_helper(uintptr_t arg1, uintptr_t arg2,
				      uintptr_t arg3, uintptr_t arg4,
				      uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;
	register uintptr_t a5 __asm__("%a5") = arg4;

	__asm__ volatile("syscall\n\t"
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3), "r" (a4),
			   "r" (a5)
			 : "memory");

	return a2;
}

uintptr_t arch_syscall_invoke3_helper(uintptr_t arg1, uintptr_t arg2,
				      uintptr_t arg3, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;

	__asm__ volatile("syscall\n\t"
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3), "r" (a4)
			 : "memory");

	return a2;
}

uintptr_t arch_syscall_invoke2_helper(uintptr_t arg1, uintptr_t arg2,
				      uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;

	__asm__ volatile("syscall\n\t"
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3)
			 : "memory");

	return a2;
}

uintptr_t arch_syscall_invoke1_helper(uintptr_t arg1, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;

	__asm__ volatile("syscall\n\t"
			 : "=r" (a2)
			 : "r" (a2), "r" (a6)
			 : "memory");

	return a2;
}

uintptr_t arch_syscall_invoke0_helper(uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;

	__asm__ volatile("syscall\n\t"
			 : "=r" (a2)
			 : "r" (a2)
			 : "memory");

	return a2;
}
