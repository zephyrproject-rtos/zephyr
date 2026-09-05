/*
 * Copyright (c) 2026 BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/arch/riscv/sbi.h"
#include "zephyr/kernel_structs.h"
#include <zephyr/kernel.h>

int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	register unsigned long a0 __asm__("a0") = _kernel.cpus[cpuid].arch.hartid;
	register unsigned long a1 __asm__("a1") = entry_point;
	register unsigned long a6 __asm__("a6") = SBI_FUNC_HART_START;
	register unsigned long a7 __asm__("a7") = SBI_EXT_HSM;

	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1) : "r"(a6), "r"(a7) : "memory");

	return sbi_err_to_errno(a0);
}

int pm_cpu_off(void)
{
	register unsigned long a0 __asm__("a0") = 0;
	register unsigned long a6 __asm__("a6") = SBI_FUNC_HART_STOP;
	register unsigned long a7 __asm__("a7") = SBI_EXT_HSM;

	__asm__ volatile("ecall" : "=r"(a0) : "r"(a6), "r"(a7) : "memory");

	return sbi_err_to_errno(a0);
}
