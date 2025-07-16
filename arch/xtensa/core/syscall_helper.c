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
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;
	register uintptr_t a5 __asm__("%a5") = arg4;
	register uintptr_t a8 __asm__("%a8") = arg5;
	register uintptr_t a9 __asm__("%a9") = arg6;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3), "r" (a4),
			   "r" (a5), "r" (a8), "r" (a9)
			 : "memory");

	return a2;
}
EXPORT_SYMBOL(xtensa_syscall_helper_args_6);

uintptr_t xtensa_syscall_helper_args_5(uintptr_t arg1, uintptr_t arg2,
				       uintptr_t arg3, uintptr_t arg4,
				       uintptr_t arg5, uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;
	register uintptr_t a5 __asm__("%a5") = arg4;
	register uintptr_t a8 __asm__("%a8") = arg5;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3), "r" (a4),
			   "r" (a5), "r" (a8)
			 : "memory");

	return a2;
}
EXPORT_SYMBOL(xtensa_syscall_helper_args_5);

uintptr_t xtensa_syscall_helper_args_4(uintptr_t arg1, uintptr_t arg2,
				       uintptr_t arg3, uintptr_t arg4,
				       uintptr_t call_id)
{
	register uintptr_t a2 __asm__("%a2") = call_id;
	register uintptr_t a6 __asm__("%a6") = arg1;
	register uintptr_t a3 __asm__("%a3") = arg2;
	register uintptr_t a4 __asm__("%a4") = arg3;
	register uintptr_t a5 __asm__("%a5") = arg4;

	__asm__ volatile(XTENSA_SYSCALL_ASM
			 : "=r" (a2)
			 : "r" (a2), "r" (a6), "r" (a3), "r" (a4),
			   "r" (a5)
			 : "memory");

	return a2;
}
EXPORT_SYMBOL(xtensa_syscall_helper_args_4);

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
#endif /* XCHAL_HAVE_THREADPTR */

size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
{
	/* Check if we can actually read the whole length.
	 *
	 * arch_user_string_nlen() is supposed to naively go through
	 * the string passed from user thread, and relies on page faults
	 * to catch inaccessible strings, such that user thread can pass
	 * a string that is shorter than the max length this function
	 * caller expects. So at least we want to make sure kernel has
	 * access to the whole length, aka. memory being mapped.
	 * Note that arch_user_string_nlen() should never result in
	 * thread termination due to page faults, and must always
	 * return to the caller with err_arg set or cleared.
	 * For MMU systems, unmapped memory will result in a DTLB miss
	 * and that might trigger an infinite DTLB miss storm if
	 * the corresponding L2 page table never exists in the first
	 * place (which would result in DTLB misses through L1 page
	 * table), until some other exceptions occur to break
	 * the cycle.
	 * For MPU systems, this would simply results in access errors
	 * and the exception handler will terminate the thread.
	 */
	if (arch_buffer_validate(s, maxsize, 0)) {
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
