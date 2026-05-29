/*
 * Copyright (c) 2025 Henrik Brix Andersen <henrik@brixandersen.dk>
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500

#include <unistd.h>

#include "hwinfo_native_bottom.h"

long native_hwinfo_gethostid_bottom(void)
{
	return gethostid();
}
