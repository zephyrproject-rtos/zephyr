/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <metal/config.h>

int metal_ver_major(void)
{
	return METAL_VER_MAJOR;
}

int metal_ver_minor(void)
{
       return METAL_VER_MINOR;
}

int metal_ver_patch(void)
{
       return METAL_VER_PATCH;
}

const char *metal_ver(void)
{
       return METAL_VER;
}
