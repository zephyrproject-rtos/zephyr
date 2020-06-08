/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief Basic C++ destructor module for globals for ARM
 */

#include <toolchain.h>

EXTERN_C int __cxa_atexit(void (*destructor)(void *), void *objptr, void *dso);

/**
 * @brief Register destructor for a global object
 *
 * @param objptr global object pointer
 * @param destructor the global object destructor function
 * @param dso Dynamic Shared Object handle for shared libraries
 *
 * Wrapper for __cxa_atexit()
 */
int __aeabi_atexit(void *objptr, void (*destructor)(void *), void *dso)
{
	return __cxa_atexit(destructor, objptr, dso);
}
