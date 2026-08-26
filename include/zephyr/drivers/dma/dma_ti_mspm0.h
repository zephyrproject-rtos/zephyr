/*
 * SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public channel direction extensions for the TI MSPM0 DMA driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_TI_MSPM0_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_TI_MSPM0_H_

#include <zephyr/drivers/dma.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TI MSPM0 DMA channel direction extensions
 * @defgroup dma_ti_mspm0_interface TI MSPM0 DMA
 * @ingroup dma_interface
 * @since 4.5
 *
 * @{
 */

/**
 * @brief TI MSPM0 DMA channel directions, for dma_config.channel_direction.
 */
enum dma_ti_mspm0_direction {
	/** Fills a memory block with the pattern in dma_block_config.source_address. */
	DMA_TI_MSPM0_DIRECTION_FILL = DMA_CHANNEL_DIRECTION_PRIV_START,
	/**
	 * Reads a table of {addr, data} pairs from dma_block_config.source_address and writes each
	 * word to its addr.
	 */
	DMA_TI_MSPM0_DIRECTION_TABLE,
	/**
	 * Gathers data via an address table at dma_block_config.source_address into
	 * dma_block_config.dest_address.
	 */
	DMA_TI_MSPM0_DIRECTION_GATHER,
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_TI_MSPM0_H_ */
