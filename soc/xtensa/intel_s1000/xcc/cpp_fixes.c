/*
 * Copyright (c) 2019, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#ifdef __clang__

int atexit(void (*function)(void))
{
	return 0;
}

#endif /* __clang__ */
