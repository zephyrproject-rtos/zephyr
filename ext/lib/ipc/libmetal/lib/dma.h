/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	dma.h
 * @brief	DMA primitives for libmetal.
 */

#ifndef __METAL_DMA__H__
#define __METAL_DMA__H__

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup dma DMA Interfaces
 *  @{ */

#include <stdint.h>
#include <metal/sys.h>

#define METAL_DMA_DEV_R  1 /**< DMA direction, device read */
#define METAL_DMA_DEV_W  2 /**< DMA direction, device write */
#define METAL_DMA_DEV_WR 3 /**< DMA direction, device read/write */

/**
 * @brief scatter/gather list element structure
 */
struct metal_sg {
	void *virt; /**< CPU virtual address */
	struct metal_io_region *io; /**< IO region */
	int len; /**< length */
};

struct metal_device;

/**
 * @brief      Map memory for DMA transaction.
 *             After the memory is DMA mapped, the memory should be
 *             accessed by the DMA device but not the CPU.
 *
 * @param[in]  dev       DMA device
 * @param[in]  dir       DMA direction
 * @param[in]  sg_in     sg list of memory to map
 * @param[in]  nents_in  number of sg list entries of memory to map
 * @param[out] sg_out    sg list of mapped memory
 * @return     number of mapped sg entries, -error on failure.
 */
int metal_dma_map(struct metal_device *dev,
		  uint32_t dir,
		  struct metal_sg *sg_in,
		  int nents_in,
		  struct metal_sg *sg_out);

/**
 * @brief      Unmap DMA memory
 *             After the memory is DMA unmapped, the memory should
 *             be accessed by the CPU but not the DMA device.
 *
 * @param[in]  dev       DMA device
 * @param[in]  dir       DMA direction
 * @param[in]  sg        sg list of mapped DMA memory
 * @param[in]  nents     number of sg list entries of DMA memory
 */
void metal_dma_unmap(struct metal_device *dev,
		  uint32_t dir,
		  struct metal_sg *sg,
		  int nents);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_DMA__H__ */
