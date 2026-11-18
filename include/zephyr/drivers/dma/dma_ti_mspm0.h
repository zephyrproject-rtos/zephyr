/*
 * SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public channel direction extensions for the TI MSPM0 DMA driver.
 * @ingroup dma_ti_mspm0_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_TI_MSPM0_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_TI_MSPM0_H_

#include <zephyr/drivers/dma.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dma_ti_mspm0_interface TI MSPM0 DMA
 * @brief Vendor channel directions for the TI MSPM0 DMA controller.
 * @ingroup dma_interface
 * @since 4.6
 * @version 0.1.0
 * @{
 */

/**
 * @brief Extended TI MSPM0 DMA channel directions, for dma_config.channel_direction.
 *
 * All extended modes:
 * - are only supported on full DMA channels,
 * - use the memory to memory block transfer mode, or the repeated block mode when
 *   dma_config.cyclic is set.
 */
enum dma_ti_mspm0_direction {
	/**
	 * Fills the destination with a pattern. The first element is the value of
	 * dma_block_config.source_address, which is used as a value and not as a pointer.
	 * Each following element changes the pattern by dma_config.source_data_size bytes,
	 * added or subtracted according to dma_block_config.source_addr_adj.
	 * DMA_ADDR_ADJ_NO_CHANGE keeps the pattern constant. Elements are
	 * dma_config.dest_data_size bytes wide.
	 */
	DMA_TI_MSPM0_DIRECTION_FILL = DMA_CHANNEL_DIRECTION_PRIV_START,
	/**
	 * Reads a table of {addr, data} entries starting at dma_block_config.source_address
	 * and writes each 32-bit data word to its 32-bit addr. Each entry is 8 bytes and the
	 * table must be 8-byte aligned. With DMA_ADDR_ADJ_DECREMENT, source_address points
	 * at the last entry and the table is read backwards.
	 */
	DMA_TI_MSPM0_DIRECTION_TABLE,
	/**
	 * Gathers data through an address table of 32-bit addresses starting at
	 * dma_block_config.source_address. The 32-bit word at each address is written to
	 * dma_block_config.dest_address in traversal order. With DMA_ADDR_ADJ_DECREMENT,
	 * source_address points at the last table entry and the table is read backwards.
	 */
	DMA_TI_MSPM0_DIRECTION_GATHER,
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_TI_MSPM0_H_ */
