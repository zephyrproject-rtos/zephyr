/*
 * Copyright (c) 2024 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INFINEON_AURIX_TC3X_SOC_H_
#define ZEPHYR_SOC_INFINEON_AURIX_TC3X_SOC_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE

void aurix_cpu_endinit_enable(bool enabled);
void aurix_safety_endinit_enable(bool enabled);

static inline int aurix_enable_clock(uintptr_t clc, uint32_t timeout)
{
	aurix_cpu_endinit_enable(false);
	sys_write32(sys_read32(clc) & ~BIT(0), clc);
	aurix_cpu_endinit_enable(true);
	return WAIT_FOR((sys_read32(clc) & 0x2) == 0, timeout, k_busy_wait(1));
}

static inline int aurix_disable_clock(uintptr_t clc, uint32_t timeout)
{
	aurix_cpu_endinit_enable(false);
	sys_write32(sys_read32(clc) | BIT(0), clc);
	aurix_cpu_endinit_enable(true);
	return WAIT_FOR((sys_read32(clc) & 0x2) == 0x2, timeout, k_busy_wait(1));
}

static inline int aurix_kernel_reset(mem_addr_t krst0, uint32_t timeout)
{
	aurix_cpu_endinit_enable(false);
	sys_write32(1, krst0 + 8);
	sys_write32(1, krst0);
	sys_write32(1, krst0 + 4);
	aurix_cpu_endinit_enable(true);
	return WAIT_FOR((sys_read32(krst0) & 0x2) == 0x2, timeout, k_busy_wait(1));
}

#endif
#endif

