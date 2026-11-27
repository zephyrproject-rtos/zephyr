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
	struct sbiret ret = sbi_hart_start(_kernel.cpus[cpuid].arch.hartid, entry_point, 0);

	return sbi_err_to_errno(ret.error);
}

int pm_cpu_off(void)
{
	struct sbiret ret = sbi_hart_stop();

	return sbi_err_to_errno(ret.error);
}
