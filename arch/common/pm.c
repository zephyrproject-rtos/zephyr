/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

unsigned int __weak arch_pm_state_set_prepare(void)
{
	return 0;
}

void __weak arch_pm_state_set_finish(unsigned int key)
{
	ARG_UNUSED(key);
}
