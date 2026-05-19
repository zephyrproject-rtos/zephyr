/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_AMEBA_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_AMEBA_DMA_H_

/**
 * @file
 * @brief Ameba SoC DMA devicetree binding flags
 *
 * This file defines bitfield flags used to configure DMA channels
 * in devicetree for Ameba SoCs.
 */

/**
 * @name DMA channel configuration flags
 * @{
 */

/**
 * @brief Set DMA transfer direction bits [1:0].
 *
 * @param val Direction value encoded in bits [1:0].
 */
#define AMEBA_DMA_CH_DIR_SET(val) ((val & 0x3) << 0)

/**
 * @brief DMA transfer direction: memory to memory.
 */
#define AMEBA_DMA_MEMORY_TO_MEMORY AMEBA_DMA_CH_DIR_SET(0)

/**
 * @brief DMA transfer direction: memory to peripheral.
 */
#define AMEBA_DMA_MEMORY_TO_PERIPH AMEBA_DMA_CH_DIR_SET(1)

/**
 * @brief DMA transfer direction: peripheral to memory.
 */
#define AMEBA_DMA_PERIPH_TO_MEMORY AMEBA_DMA_CH_DIR_SET(2)

/**
 * @brief DMA transfer direction: peripheral to peripheral.
 */
#define AMEBA_DMA_PERIPH_TO_PERIPH AMEBA_DMA_CH_DIR_SET(3)

/**
 * @brief Set source address adjustment bits [3:2].
 *
 * @param val Address adjustment value encoded in bits [3:2].
 */
#define AMEBA_DMA_SRC_ADDR_ADJ_SET(val) ((val & 0x3) << 2)

/**
 * @brief Source address increment after each transfer.
 */
#define AMEBA_DMA_SRC_ADDR_ADJ_INC AMEBA_DMA_SRC_ADDR_ADJ_SET(0)

/**
 * @brief Source address does not change.
 */
#define AMEBA_DMA_SRC_ADDR_ADJ_NO_CHANGE AMEBA_DMA_SRC_ADDR_ADJ_SET(2)

/**
 * @brief Set destination address adjustment bits [5:4].
 *
 * @param val Address adjustment value encoded in bits [5:4].
 */
#define AMEBA_DMA_DST_ADDR_ADJ_SET(val) ((val & 0x3) << 4)

/**
 * @brief Destination address increment after each transfer.
 */
#define AMEBA_DMA_DST_ADDR_ADJ_INC AMEBA_DMA_DST_ADDR_ADJ_SET(0)

/**
 * @brief Destination address does not change.
 */
#define AMEBA_DMA_DST_ADDR_ADJ_NO_CHANGE AMEBA_DMA_DST_ADDR_ADJ_SET(2)

/**
 * @brief Set source data width bits [8:6].
 *
 * @param val Data width value encoded in bits [8:6].
 */
#define AMEBA_DMA_SRC_DATA_SIZE_SET(val) ((val & 0x7) << 6)

/**
 * @brief Source data width: 8 bits.
 */
#define AMEBA_DMA_SRC_WIDTH_8BITS AMEBA_DMA_SRC_DATA_SIZE_SET(0)

/**
 * @brief Source data width: 16 bits.
 */
#define AMEBA_DMA_SRC_WIDTH_16BITS AMEBA_DMA_SRC_DATA_SIZE_SET(1)

/**
 * @brief Source data width: 32 bits.
 */
#define AMEBA_DMA_SRC_WIDTH_32BITS AMEBA_DMA_SRC_DATA_SIZE_SET(2)

/**
 * @brief Set destination data width bits [11:9].
 *
 * @param val Data width value encoded in bits [11:9].
 */
#define AMEBA_DMA_DST_DATA_SIZE_SET(val) ((val & 0x7) << 9)

/**
 * @brief Destination data width: 8 bits.
 */
#define AMEBA_DMA_DST_WIDTH_8BITS AMEBA_DMA_DST_DATA_SIZE_SET(0)

/**
 * @brief Destination data width: 16 bits.
 */
#define AMEBA_DMA_DST_WIDTH_16BITS AMEBA_DMA_DST_DATA_SIZE_SET(1)

/**
 * @brief Destination data width: 32 bits.
 */
#define AMEBA_DMA_DST_WIDTH_32BITS AMEBA_DMA_DST_DATA_SIZE_SET(2)

/**
 * @brief Set source burst length bits [16:12].
 *
 * @param val Burst length value encoded in bits [16:12].
 */
#define AMEBA_DMA_SRC_BURST_SET(val) ((val & 0x1F) << 12)

/**
 * @brief Source burst length: 1 transfer.
 */
#define AMEBA_DMA_SRC_BURST_ONE AMEBA_DMA_SRC_BURST_SET(0)

/**
 * @brief Source burst length: 4 transfers.
 */
#define AMEBA_DMA_SRC_BURST_FOUR AMEBA_DMA_SRC_BURST_SET(1)

/**
 * @brief Source burst length: 8 transfers.
 */
#define AMEBA_DMA_SRC_BURST_EIGHT AMEBA_DMA_SRC_BURST_SET(2)

/**
 * @brief Source burst length: 16 transfers.
 */
#define AMEBA_DMA_SRC_BURST_SIXTEEN AMEBA_DMA_SRC_BURST_SET(3)

/**
 * @brief Set destination burst length bits [21:17].
 *
 * @param val Burst length value encoded in bits [21:17].
 */
#define AMEBA_DMA_DST_BURST_SET(val) ((val & 0x1F) << 17)

/**
 * @brief Destination burst length: 1 transfer.
 */
#define AMEBA_DMA_DST_BURST_ONE AMEBA_DMA_DST_BURST_SET(0)

/**
 * @brief Destination burst length: 4 transfers.
 */
#define AMEBA_DMA_DST_BURST_FOUR AMEBA_DMA_DST_BURST_SET(1)

/**
 * @brief Destination burst length: 8 transfers.
 */
#define AMEBA_DMA_DST_BURST_EIGHT AMEBA_DMA_DST_BURST_SET(2)

/**
 * @brief Destination burst length: 16 transfers.
 */
#define AMEBA_DMA_DST_BURST_SIXTEEN AMEBA_DMA_DST_BURST_SET(3)

/**
 * @brief Set channel priority bits [24:22].
 *
 * @param val Channel priority value in range 0–7.
 */
#define AMEBA_DMA_CH_PRIORITY_SET(val) ((val & 0x7) << 22)

/**
 * @brief Typical peripheral TX configuration.
 *
 * Memory to peripheral, source address increment, destination address fixed.
 */
#define AMEBA_DMA_PERIPH_TX                                                                        \
	(AMEBA_DMA_MEMORY_TO_PERIPH | AMEBA_DMA_SRC_ADDR_ADJ_INC | AMEBA_DMA_DST_ADDR_ADJ_NO_CHANGE)

/**
 * @brief Typical peripheral RX configuration.
 *
 * Peripheral to memory, source address fixed, destination address increment.
 */
#define AMEBA_DMA_PERIPH_RX                                                                        \
	(AMEBA_DMA_PERIPH_TO_MEMORY | AMEBA_DMA_SRC_ADDR_ADJ_NO_CHANGE | AMEBA_DMA_DST_ADDR_ADJ_INC)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_AMEBA_DMA_H_ */
