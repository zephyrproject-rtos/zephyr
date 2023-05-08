/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief Basic C++ destructor module for globals
 *
 */

#include <zephyr/toolchain.h>

__weak void *__dso_handle;

/**
 * @brief Register destructor for a global object
 *
 * @param destructor the global object destructor function
 * @param objptr global object pointer
 * @param dso Dynamic Shared Object handle for shared libraries
 *
 * Function does nothing at the moment, assuming the global objects
 * do not need to be deleted
 *
 * @retval 0 on success.
 */
int __cxa_atexit(void (*destructor)(void *), void *objptr, void *dso)
{
	ARG_UNUSED(destructor);
	ARG_UNUSED(objptr);
	ARG_UNUSED(dso);
	return 0;
}
