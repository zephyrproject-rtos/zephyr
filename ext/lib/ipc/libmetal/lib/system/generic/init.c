/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	generic/init.c
 * @brief	Generic libmetal initialization.
 */

#include <metal/sys.h>
#include <metal/utilities.h>
#include <metal/device.h>

struct metal_state _metal;

int metal_sys_init(const struct metal_init_params *params)
{
	metal_unused(params);
	metal_bus_register(&metal_generic_bus);
	return 0;
}

void metal_sys_finish(void)
{
	metal_bus_unregister(&metal_generic_bus);
}
