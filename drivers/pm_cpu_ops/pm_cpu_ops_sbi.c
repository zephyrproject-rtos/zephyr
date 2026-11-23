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
	struct sbiret ret = sbi_ecall(_kernel.cpus[cpuid].arch.hartid, entry_point, 0, 0, 0, 0,
				      SBI_FUNC_HART_START, SBI_EXT_HSM);

	return sbi_err_to_errno(ret.error);
}

int pm_cpu_off(void)
{
	struct sbiret ret = sbi_ecall(0, 0, 0, 0, 0, 0, SBI_FUNC_HART_STOP, SBI_EXT_HSM);

	return sbi_err_to_errno(ret.error);
}
