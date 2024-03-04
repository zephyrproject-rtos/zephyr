/*
 * Copyright (c) 2024 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "opensbi.h"

#include "zephyr/arch/riscv/arch.h"

sbi_ret_t user_mode_sbi_ecall(int ext, int fid, unsigned long arg0, unsigned long arg1,
			      unsigned long arg2, unsigned long arg3, unsigned long arg4,
			      unsigned long arg5)
{
	uint32_t irq = arch_irq_lock();

	volatile sbi_ret_t ret;

	register unsigned long a0 __asm__("a0") = (unsigned long)(arg0);
	register unsigned long a1 __asm__("a1") = (unsigned long)(arg1);
	register unsigned long a2 __asm__("a2") = (unsigned long)(arg2);
	register unsigned long a3 __asm__("a3") = (unsigned long)(arg3);
	register unsigned long a4 __asm__("a4") = (unsigned long)(arg4);
	register unsigned long a5 __asm__("a5") = (unsigned long)(arg5);
	register unsigned long a6 __asm__("a6") = (unsigned long)(fid);
	register unsigned long a7 __asm__("a7") = (unsigned long)(ext);
	__asm__ volatile("ecall"
			 : "+r"(a0), "+r"(a1)
			 : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
			 : "memory");
	ret.error = (int32_t)a0;
	ret.value = a1;

	if (irq) {
		arch_irq_unlock(irq);
	}

	return ret;
}
