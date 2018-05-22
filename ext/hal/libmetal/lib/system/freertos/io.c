/*
 * Copyright (c) 2017, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	freertos/io.c
 * @brief	FreeRTOS libmetal io operations
 */

#include <metal/io.h>

void metal_sys_io_mem_map(struct metal_io_region *io)
{
	unsigned long p;
	size_t psize;
	void *va;

	va = io->virt;
	psize = io->size;
	if (psize) {
		if (psize >> io->page_shift)
			psize = (size_t)1 << io->page_shift;
		for (p = 0; p <= (io->size >> io->page_shift); p++) {
			metal_machine_io_mem_map(va, io->physmap[p],
						 psize, io->mem_flags);
			va += psize;
		}
	}
}
