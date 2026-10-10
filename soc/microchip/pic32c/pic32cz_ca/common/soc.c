/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/init.h>

void soc_early_init_hook(void)
{
	sys_cache_data_enable();
}
