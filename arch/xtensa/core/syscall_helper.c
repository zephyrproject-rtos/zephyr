/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/xtensa/syscall.h>

#ifdef CONFIG_XTENSA_SYSCALL_USE_HELPER
uintptr_t xtensa_syscall_helper(uintptr_t arg1, uintptr_t arg2,
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
