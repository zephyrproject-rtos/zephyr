/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stack frames for debugging purposes
 *
 * This file contains a routine useful for debugging that gets a pointer to
 * the current interrupt stack frame.
 */

#include <kernel.h>
#include <kernel_structs.h>

NANO_ISF *sys_debug_current_isf_get(void)
{
	return _kernel.arch.isf;
}
