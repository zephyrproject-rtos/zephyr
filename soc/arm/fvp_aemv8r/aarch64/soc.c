/*
 * Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#ifdef CONFIG_SOC_FVP_AEMV8R_SIMULATE_CPU_PM

/*
 * fvp can work on the mode that all core run together when it start.
 * So an implementation for FVP is needed.
 */
int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	return 0;
}

#endif
