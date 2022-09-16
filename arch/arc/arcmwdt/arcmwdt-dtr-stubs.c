/*
 * Copyright (c) 2021 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

__weak void *__dso_handle;

int __cxa_atexit(void (*destructor)(void *), void *objptr, void *dso)
{
	ARG_UNUSED(destructor);
	ARG_UNUSED(objptr);
	ARG_UNUSED(dso);
	return 0;
}

int atexit(void (*function)(void))
{
	return 0;
}
