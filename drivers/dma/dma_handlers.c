/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dma.h>
#include <zephyr/internal/syscall_handler.h>

/* Both of these APIs are assuming that the drive implementations are checking
 * the validity of the channel ID and returning -errno if it's bogus
 */

static inline int z_vrfy_dma_start(const struct device *dev, uint32_t channel)
{
	K_OOPS(K_SYSCALL_DRIVER_DMA(dev, start));
	return z_impl_dma_start((const struct device *)dev, channel);
}
#include <zephyr/syscalls/dma_start_mrsh.c>

static inline int z_vrfy_dma_stop(const struct device *dev, uint32_t channel)
{
	K_OOPS(K_SYSCALL_DRIVER_DMA(dev, stop));
	return z_impl_dma_stop((const struct device *)dev, channel);
}
#include <zephyr/syscalls/dma_stop_mrsh.c>
