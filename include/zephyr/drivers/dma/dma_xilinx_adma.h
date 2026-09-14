/*
 * Copyright (c) 2026 Advanced Micro Devices.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_XILINX_ADMA_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_XILINX_ADMA_H_

#include <zephyr/drivers/dma.h>

/**
 * @file
 * @brief Xilinx ADMA DMA driver specific values.
 *
 * @defgroup dma_xilinx_adma Xilinx ADMA specifics
 * @ingroup dma_interface
 * @{
 */

/**
 * @brief Write-Only mode channel direction.
 *
 * Instead of copying from a real source, the channel repeatedly writes a
 * fixed pattern to the destination address/size.
 *
 * Set dma_config.channel_direction to this value and point
 * dma_block_config.source_address at an 8-byte (2 x uint32_t) pattern
 * buffer instead of a real source buffer. Not valid together with
 * scatter-gather (multi-block) transfers.
 */
#define XILINX_ADMA_DIR_WRITE_ONLY (DMA_CHANNEL_DIRECTION_PRIV_START)

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_XILINX_ADMA_H_ */
