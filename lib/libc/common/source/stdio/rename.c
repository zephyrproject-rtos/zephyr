/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/util_macro.h>

int rename(const char *old, const char *newp)
{
	return zvfs_rename(old, newp);
}
