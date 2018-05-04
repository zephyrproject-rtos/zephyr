/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <dma.h>
#include <syscall_handler.h>

/* Both of these APIs are assuming that the drive implementations are checking
 * the validity of the channel ID and returning -errno if it's bogus
 */

Z_SYSCALL_HANDLER(dma_start, dev, channel)
{
	Z_OOPS(Z_SYSCALL_DRIVER_DMA(dev, start));
	return _impl_dma_start((struct device *)dev, channel);
}

Z_SYSCALL_HANDLER(dma_stop, dev, channel)
{
	Z_OOPS(Z_SYSCALL_DRIVER_DMA(dev, stop));
	return _impl_dma_stop((struct device *)dev, channel);
}

