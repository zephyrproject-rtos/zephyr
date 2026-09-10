/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

int __weak pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	return -ENOTSUP;
}

int __weak pm_cpu_off(void)
{
	return -ENOTSUP;
}
