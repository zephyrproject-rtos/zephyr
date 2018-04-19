/*
 * Copyright (c) 2017, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/device.c
 * @brief	Zephyr libmetal device definitions.
 */

#include <metal/device.h>
#include <metal/sys.h>
#include <metal/utilities.h>

int metal_generic_dev_sys_open(struct metal_device *dev)
{
	metal_unused(dev);

	/* Since Zephyr runs bare-metal there is no mapping that needs to be
	 * done of IO regions
	 */
	return 0;
}

struct metal_bus metal_generic_bus = {
	.name = "generic",
	.ops  = {
		.bus_close = NULL,
		.dev_open  = metal_generic_dev_open,
		.dev_close = NULL,
		.dev_irq_ack = NULL,
		.dev_dma_map = NULL,
		.dev_dma_unmap = NULL,
	},
};
