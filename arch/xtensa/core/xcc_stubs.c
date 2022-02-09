/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

__weak int atexit(void (*function)(void))
{
	ARG_UNUSED(function);

	return 0;
}
