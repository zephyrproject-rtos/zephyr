/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Realtek Bee DMA Configuration Macros
 *
 * This header contains helper macros to extract DMA configuration parameters
 * (such as direction, data size, burst size, etc.) from the Devicetree
 * `config` cell for the Realtek Bee series SoCs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_BEE_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_BEE_H_

/**
 * @defgroup dma_bee_macros Realtek Bee DMA Macros
 * @brief Macros for extracting DMA configuration from Devicetree
 * @{
 */

/**
 * @brief Get the DMA controller device node identifier.
 *
 * @param id  The instance identifier.
 * @param dir The direction name (e.g., rx, tx).
 * @return The controller device identifier.
 */
#define BEE_DMA_CTLR(id, dir)           DT_INST_DMAS_CTLR_BY_NAME(id, dir)

/**
 * @brief Get the raw configuration value from Devicetree.
 *
 * @param id  The instance identifier.
 * @param dir The direction name (e.g., rx, tx).
 * @return The integer value of the 'config' cell.
 */
#define BEE_DMA_CHANNEL_CONFIG(id, dir) DT_INST_DMAS_CELL_BY_NAME(id, dir, config)

/* -------------------------------------------------------------------------- */
/* Channel Configuration Extraction Macros                                    */
/* -------------------------------------------------------------------------- */

/**
 * @brief Extract DMA transfer direction.
 *
 * Located at bits 0-1 of the config.
 * - 0: Memory to Memory
 * - 1: Memory to Peripheral
 * - 2: Peripheral to Memory
 *
 * @param config The configuration value.
 * @return The direction code (0-3).
 */
#define BEE_DMA_CONFIG_DIRECTION(config) ((config >> 0) & 0x3)

/**
 * @brief Extract Source Address Increment mode.
 *
 * Located at bits 2-3 of the config.
 * - 0: Increment
 * - 1: Decrement
 * - 2: No Change
 *
 * @param config The configuration value.
 * @return The source address increment mode (0-3).
 */
#define BEE_DMA_CONFIG_SOURCE_ADDR_INC(config) ((config >> 2) & 0x3)

/**
 * @brief Extract Destination Address Increment mode.
 *
 * Located at bits 4-5 of the config.
 * - 0: Increment
 * - 1: Decrement
 * - 2: No Change
 *
 * @param config The configuration value.
 * @return The destination address increment mode (0-3).
 */
#define BEE_DMA_CONFIG_DESTINATION_ADDR_INC(config) ((config >> 4) & 0x3)

/**
 * @brief Extract Source Data Size in bytes.
 *
 * Located at bits 6-7 of the config.
 * - 0 -> 1 byte
 * - 1 -> 2 bytes
 * - 2 -> 4 bytes
 *
 * @param config The configuration value.
 * @return The data size in bytes (1, 2, or 4).
 */
#define BEE_DMA_CONFIG_SOURCE_DATA_SIZE(config) (1 << ((config >> 6) & 0x3))

/**
 * @brief Extract Destination Data Size in bytes.
 *
 * Located at bits 8-9 of the config.
 * - 0 -> 1 byte
 * - 1 -> 2 bytes
 * - 2 -> 4 bytes
 *
 * @param config The configuration value.
 * @return The data size in bytes (1, 2, or 4).
 */
#define BEE_DMA_CONFIG_DESTINATION_DATA_SIZE(config) (1 << ((config >> 8) & 0x3))

/**
 * @brief Extract Source MSIZE.
 *
 * Located at bits 10-12 of the config.
 * Maps the 3-bit index to the actual msize:
 * - 0 -> msize
 * - 1 -> msize 4
 * - 2 -> msize 8
 * - 3 -> msize 16
 * - 4 -> msize 32
 * - 5 -> msize 64
 * - 6 -> msize 128
 * - 7 -> msize 256
 *
 * @param config The configuration value.
 * @return The burst size.
 */
#define BEE_DMA_CONFIG_SOURCE_MSIZE(config)                                                        \
	((1 << (((config >> 10) & 0x7) + 1)) - (((config >> 10) & 0x7) == 0 ? 1 : 0))

/**
 * @brief Extract Destination MSIZE.
 *
 * Located at bits 13-15 of the config.
 * Maps the 3-bit index to the actual msize:
 * - 0 -> msize
 * - 1 -> msize 4
 * - 2 -> msize 8
 * - 3 -> msize 16
 * - 4 -> msize 32
 * - 5 -> msize 64
 * - 6 -> msize 128
 * - 7 -> msize 256
 *
 * @param config The configuration value.
 * @return The burst size.
 */
#define BEE_DMA_CONFIG_DESTINATION_MSIZE(config)                                                   \
	((1 << (((config >> 13) & 0x7) + 1)) - (((config >> 13) & 0x7) == 0 ? 1 : 0))

/**
 * @brief Extract Channel Priority.
 *
 * Located at bits 16-20 of the config.
 * Range: 0-9.
 *
 * @param config The configuration value.
 * @return The priority level.
 */
#define BEE_DMA_CONFIG_PRIORITY(config) ((config >> 16) & 0x1f)

/** @} */ /* End of dma_bee_macros group */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_BEE_H_ */
