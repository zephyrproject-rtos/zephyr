/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dma.h>
#include <zephyr/internal/syscall_handler.h>

/* All of these APIs are assuming that the drive implementations are checking
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

static int z_vrfy_dma_chan_filter(const struct device * dev, int channel, void * filter_param)
{
	K_OOPS(K_SYSCALL_DRIVER_DMA(dev, chan_filter));
	return z_impl_dma_chan_filter((const struct device *)dev, channel, filter_param);
}
#include <zephyr/syscalls/dma_chan_filter_mrsh.c>

static inline int z_vrfy_dma_request_channel(const struct device * dev, void * filter_param)
{
	/* uses 'chan_filter' op in the implementation */
	K_OOPS(K_SYSCALL_DRIVER_DMA(dev, chan_filter));
	return z_impl_dma_request_channel((const struct device *)dev, filter_param);
}
#include <zephyr/syscalls/dma_request_channel_mrsh.c>

static inline void z_vrfy_dma_release_channel(const struct device * dev, uint32_t channel)
{
	K_OOPS(K_SYSCALL_DRIVER_DMA(dev, chan_release));
	z_impl_dma_release_channel((const struct device *)dev, channel);
}
#include <zephyr/syscalls/dma_release_channel_mrsh.c>

static inline int z_vrfy_dma_suspend(const struct device * dev, uint32_t channel)
{
	K_OOPS(K_SYSCALL_DRIVER_DMA(dev, suspend));
	return z_impl_dma_suspend((const struct device *)dev, channel);
}
#include <zephyr/syscalls/dma_suspend_mrsh.c>

static inline int z_vrfy_dma_resume(const struct device * dev, uint32_t channel)
{
	K_OOPS(K_SYSCALL_DRIVER_DMA(dev, resume));
	return z_impl_dma_resume((const struct device *)dev, channel);
}
#include <zephyr/syscalls/dma_resume_mrsh.c>
