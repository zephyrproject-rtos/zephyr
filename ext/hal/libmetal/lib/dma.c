/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <string.h>
#include <metal/device.h>
#include <metal/log.h>
#include <metal/dma.h>
#include <metal/atomic.h>

int metal_dma_map(struct metal_device *dev,
		  uint32_t dir,
		  struct metal_sg *sg_in,
		  int nents_in,
		  struct metal_sg *sg_out)
{
	int nents_out;

	if (!dev || !sg_in || !sg_out)
		return -EINVAL;
	if (!dev->bus->ops.dev_dma_map)
		return -ENODEV;

	/* memory barrier */
	if (dir == METAL_DMA_DEV_R)
		/* If it is device read, apply memory write fence. */
		atomic_thread_fence(memory_order_release);
	else
		/* If it is device write or device r/w,
		   apply memory r/w fence. */
		atomic_thread_fence(memory_order_acq_rel);
	nents_out = dev->bus->ops.dev_dma_map(dev->bus,
			dev, dir, sg_in, nents_in, sg_out);
	return nents_out;
}

void metal_dma_unmap(struct metal_device *dev,
		  uint32_t dir,
		  struct metal_sg *sg,
		  int nents)
{
	/* memory barrier */
	if (dir == METAL_DMA_DEV_R)
		/* If it is device read, apply memory write fence. */
		atomic_thread_fence(memory_order_release);
	else
		/* If it is device write or device r/w,
		   apply memory r/w fence. */
		atomic_thread_fence(memory_order_acq_rel);

	if (!dev || !dev->bus->ops.dev_dma_unmap || !sg)
		return;
	dev->bus->ops.dev_dma_unmap(dev->bus,
			dev, dir, sg, nents);
}
