/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_INTEL_LPSS_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_INTEL_LPSS_H_

#define DMA_INTEL_LPSS_OFFSET		0x800
#define DMA_INTEL_LPSS_REMAP_LOW	0x240
#define DMA_INTEL_LPSS_REMAP_HI		0x244
#define DMA_INTEL_LPSS_TX_CHAN		0
#define DMA_INTEL_LPSS_RX_CHAN		1
#define DMA_INTEL_LPSS_ADDR_RIGHT_SHIFT	32

void dma_intel_lpss_isr(const struct device *dev);
int dma_intel_lpss_setup(const struct device *dev);
void dma_intel_lpss_set_base(const struct device *dev, uintptr_t base);

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_INTEL_LPSS_H_ */
