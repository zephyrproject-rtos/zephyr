/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/arch/xtensa/syscall.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/llext/symbol.h>
#include <xtensa_internal.h>

#ifdef CONFIG_XTENSA_SYSCALL_USE_HELPER
uintptr_t xtensa_syscall_helper_args_6(uintptr_t arg1, uintptr_t arg2,
				       uintptr_t arg3, uintptr_t arg4,
				       uintptr_t arg5, uintptr_t arg6,
				       uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t p1 __asm__(SYSCALL_REG_ARG1) = arg1;
	register uintptr_t p2 __asm__(SYSCALL_REG_ARG2) = arg2;
	register uintptr_t p3 __asm__(SYSCALL_REG_ARG3) = arg3;
	register uintptr_t p4 __asm__(SYSCALL_REG_ARG4) = arg4;
	register uintptr_t p5 __asm__(SYSCALL_REG_ARG5) = arg5;
	register uintptr_t p6 __asm__(SYSCALL_REG_ARG6) = arg6;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (p1), "r" (p2), "r" (p3),
			   "r" (p4), "r" (p5), "r" (p6)
			 : "memory");

	return a2;
}
EXPORT_SYMBOL(xtensa_syscall_helper_args_6);

uintptr_t xtensa_syscall_helper_args_5(uintptr_t arg1, uintptr_t arg2,
				       uintptr_t arg3, uintptr_t arg4,
				       uintptr_t arg5, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t p1 __asm__(SYSCALL_REG_ARG1) = arg1;
	register uintptr_t p2 __asm__(SYSCALL_REG_ARG2) = arg2;
	register uintptr_t p3 __asm__(SYSCALL_REG_ARG3) = arg3;
	register uintptr_t p4 __asm__(SYSCALL_REG_ARG4) = arg4;
	register uintptr_t p5 __asm__(SYSCALL_REG_ARG5) = arg5;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (p1), "r" (p2), "r" (p3),
			   "r" (p4), "r" (p5)
			 : "memory");

	return a2;
}
EXPORT_SYMBOL(xtensa_syscall_helper_args_5);

uintptr_t xtensa_syscall_helper_args_4(uintptr_t arg1, uintptr_t arg2,
				       uintptr_t arg3, uintptr_t arg4,
				       uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t p1 __asm__(SYSCALL_REG_ARG1) = arg1;
	register uintptr_t p2 __asm__(SYSCALL_REG_ARG2) = arg2;
	register uintptr_t p3 __asm__(SYSCALL_REG_ARG3) = arg3;
	register uintptr_t p4 __asm__(SYSCALL_REG_ARG4) = arg4;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (p1), "r" (p2), "r" (p3),
			   "r" (p4)
			 : "memory");

	return a2;
}
EXPORT_SYMBOL(xtensa_syscall_helper_args_4);

#ifdef CONFIG_XTENSA_SYSCALL_USE_HELPER_ALL
uintptr_t xtensa_syscall_helper_args_3(uintptr_t arg1, uintptr_t arg2,
				       uintptr_t arg3, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t p1 __asm__(SYSCALL_REG_ARG1) = arg1;
	register uintptr_t p2 __asm__(SYSCALL_REG_ARG2) = arg2;
	register uintptr_t p3 __asm__(SYSCALL_REG_ARG3) = arg3;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (p1), "r" (p2), "r" (p3)
			 : "memory");

	return a2;
}
EXPORT_SYMBOL(xtensa_syscall_helper_args_3);

uintptr_t xtensa_syscall_helper_args_2(uintptr_t arg1, uintptr_t arg2,
				       uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t p1 __asm__(SYSCALL_REG_ARG1) = arg1;
	register uintptr_t p2 __asm__(SYSCALL_REG_ARG2) = arg2;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (p1), "r" (p2)
			 : "memory");

	return a2;
}
EXPORT_SYMBOL(xtensa_syscall_helper_args_2);

uintptr_t xtensa_syscall_helper_args_1(uintptr_t arg1, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t p1 __asm__(SYSCALL_REG_ARG1) = arg1;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (p1)
			 : "memory");

	return a2;
}
EXPORT_SYMBOL(xtensa_syscall_helper_args_1);

uintptr_t xtensa_syscall_helper_args_0(uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2)
			 : "memory");

	return a2;
}
EXPORT_SYMBOL(xtensa_syscall_helper_args_0);
#endif /* CONFIG_XTENSA_SYSCALL_USE_HELPER_ALL */

#endif /* CONFIG_XTENSA_SYSCALL_USE_HELPER */

#if XCHAL_HAVE_THREADPTR == 0
#include <xtensa/config/core-isa.h>
#include <xtensa/config/core.h>

bool xtensa_is_user_context(void)
{
	uint32_t ret;

	__asm__ volatile(".global xtensa_is_user_context_epc\n"
			 "        xtensa_is_user_context_epc:\n"
			 "                syscall\n"
			 "                mov %0, a2\n"
			 : "=r"(ret) : : "a2");

	return ret != 0;
}
EXPORT_SYMBOL(xtensa_is_user_context);
#endif /* XCHAL_HAVE_THREADPTR */

size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
{
	if (maxsize == 0) {
		*err_arg = 0;

		return 0;
	}

	/* Xtensa cannot safely recover from a fault while examining a string.
	 * For MMU systems, an unmapped address can trigger an infinite DTLB
	 * miss storm if its L2 page table does not exist. For MPU systems,
	 * an access fault terminates the thread. Validate that the requested
	 * range is readable in kernel mode before calling strnlen().
	 *
	 * Only kernel mode readability is checked here: per
	 * the k_usermode_string_nlen() contract, user mode accessibility
	 * is verified separately by the syscall marshalling layer (e.g.
	 * via K_SYSCALL_MEMORY_READ()) once the length is known, so
	 * kernel mode callers must be able to measure kernel strings.
	 */
	if (!xtensa_buffer_is_kernel_readable(s, maxsize)) {
		/*
		 * API says we need to set err_arg to -1 if there are
		 * any errors.
		 */
		*err_arg = -1;

		return 0;
	}

	/* No error and we can proceed to getting the string length. */
	*err_arg = 0;

	return strnlen(s, maxsize);
}
