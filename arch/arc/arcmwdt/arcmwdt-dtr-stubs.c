/*
 * Copyright (c) 2021 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

__weak void *__dso_handle;

int atexit(void (*function)(void))
{
	return 0;
}
