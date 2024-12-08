/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_MCUX_PXP_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_MCUX_PXP_H_

#define DMA_MCUX_PXP_CMD_MASK  0xE0
#define DMA_MCUX_PXP_CMD_SHIFT 0x5

#define DMA_MCUX_PXP_FMT_MASK  0x1F
#define DMA_MCUX_PXP_FMT_SHIFT 0x0

/*
 * In order to configure the PXP for rotation, the user should
 * supply a format and command as the DMA slot parameter, like so:
 * dma_slot = (DMA_MCUX_PXP_FTM(DMA_MCUX_PXP_FMT_RGB565) |
 *            DMA_MCUX_PXP_CMD(DMA_MCUX_PXP_CMD_ROTATE_90))
 * head block source address: input buffer address
 * head block destination address: output buffer address
 * source data size: input buffer size in bytes
 * source burst length: height of source buffer in pixels
 * dest data size: output buffer size in bytes
 * dest burst length: height of destination buffer in pixels
 */

#define DMA_MCUX_PXP_FMT(x) ((x << DMA_MCUX_PXP_FMT_SHIFT) & DMA_MCUX_PXP_FMT_MASK)
#define DMA_MCUX_PXP_CMD(x) ((x << DMA_MCUX_PXP_CMD_SHIFT) & DMA_MCUX_PXP_CMD_MASK)

#define DMA_MCUX_PXP_CMD_ROTATE_0   0
#define DMA_MCUX_PXP_CMD_ROTATE_90  1
#define DMA_MCUX_PXP_CMD_ROTATE_180 2
#define DMA_MCUX_PXP_CMD_ROTATE_270 3

#define DMA_MCUX_PXP_FMT_RGB565   0
#define DMA_MCUX_PXP_FMT_RGB888   1
#define DMA_MCUX_PXP_FMT_ARGB8888 2

#define DMA_MCUX_PXP_FLIP_MASK  0x3
#define DMA_MCUX_PXP_FLIP_SHIFT 0x0

/*
 * In order to configure the PXP to flip, the user should
 * supply a flip setting as the DMA linked_channel parameter, like so:
 * linked_channel |= DMA_MCUX_PXP_FLIP(DMA_MCUX_PXP_FLIP_HORIZONTAL)
 */

#define DMA_MCUX_PXP_FLIP(x) ((x << DMA_MCUX_PXP_FLIP_SHIFT) & DMA_MCUX_PXP_FLIP_MASK)

#define DMA_MCUX_PXP_FLIP_DISABLE    0
#define DMA_MCUX_PXP_FLIP_HORIZONTAL 1
#define DMA_MCUX_PXP_FLIP_VERTICAL   2
#define DMA_MCUX_PXP_FLIP_BOTH       3

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_MCUX_PXP_H_ */
