/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	nuttx/init.c
 * @brief	NuttX libmetal initialization.
 */

#include <metal/device.h>
#include <metal/irq.h>
#include <metal/sys.h>

struct metal_state _metal;

int metal_sys_init(const struct metal_init_params *params)
{
	int ret = metal_cntr_irq_init();
	if (ret >= 0)
		ret = metal_bus_register(&metal_generic_bus);
	return ret;
}

void metal_sys_finish(void)
{
	metal_bus_unregister(&metal_generic_bus);
}
