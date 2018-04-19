/*
 * Copyright (c) 2017, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/init.c
 * @brief	Zephyr libmetal initialization.
 */

#include <metal/device.h>
#include <metal/sys.h>
#include <metal/utilities.h>

struct metal_state _metal;

int metal_sys_init(const struct metal_init_params *params)
{
	metal_bus_register(&metal_generic_bus);
	return 0;
}

void metal_sys_finish(void)
{
	metal_bus_unregister(&metal_generic_bus);
}
