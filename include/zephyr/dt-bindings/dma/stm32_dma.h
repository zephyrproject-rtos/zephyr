/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_STM32_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_STM32_DMA_H_

/**
 * @name custom DMA flags for channel configuration
 * @{
 */
/** DMA  cyclic mode config on bit 5*/
#define STM32_DMA_CH_CFG_MODE(val)		((val & 0x1) << 5)
#define STM32_DMA_MODE_NORMAL			STM32_DMA_CH_CFG_MODE(0)
#define STM32_DMA_MODE_CYCLIC			STM32_DMA_CH_CFG_MODE(1)

/** DMA  transfer direction config on bits 6-7 */
#define STM32_DMA_CH_CFG_DIRECTION(val)		((val & 0x3) << 6)
#define STM32_DMA_MEMORY_TO_MEMORY		STM32_DMA_CH_CFG_DIRECTION(0)
#define STM32_DMA_MEMORY_TO_PERIPH		STM32_DMA_CH_CFG_DIRECTION(1)
#define STM32_DMA_PERIPH_TO_MEMORY		STM32_DMA_CH_CFG_DIRECTION(2)
#define STM32_DMA_PERIPH_TO_PERIPH		STM32_DMA_CH_CFG_DIRECTION(3)

/** DMA  Peripheral increment Address config on bit 9 */
#define STM32_DMA_CH_CFG_PERIPH_ADDR_INC(val)	((val & 0x1) << 9)
#define STM32_DMA_PERIPH_NO_INC			STM32_DMA_CH_CFG_PERIPH_ADDR_INC(0)
#define STM32_DMA_PERIPH_INC			STM32_DMA_CH_CFG_PERIPH_ADDR_INC(1)

/** DMA  Memory increment Address config on bit 10 */
#define STM32_DMA_CH_CFG_MEM_ADDR_INC(val)	((val & 0x1) << 10)
#define STM32_DMA_MEM_NO_INC			STM32_DMA_CH_CFG_MEM_ADDR_INC(0)
#define STM32_DMA_MEM_INC			STM32_DMA_CH_CFG_MEM_ADDR_INC(1)

/** DMA  Peripheral data size config on bits 11, 12 */
#define STM32_DMA_CH_CFG_PERIPH_WIDTH(val)	((val & 0x3) << 11)
#define STM32_DMA_PERIPH_8BITS			STM32_DMA_CH_CFG_PERIPH_WIDTH(0)
#define STM32_DMA_PERIPH_16BITS			STM32_DMA_CH_CFG_PERIPH_WIDTH(1)
#define STM32_DMA_PERIPH_32BITS			STM32_DMA_CH_CFG_PERIPH_WIDTH(2)
#define STM32_DMA_PERIPH_64BITS			STM32_DMA_CH_CFG_PERIPH_WIDTH(3)

/** DMA  Memory data size config on bits 13, 14 */
#define STM32_DMA_CH_CFG_MEM_WIDTH(val)		((val & 0x3) << 13)
#define STM32_DMA_MEM_8BITS			STM32_DMA_CH_CFG_MEM_WIDTH(0)
#define STM32_DMA_MEM_16BITS			STM32_DMA_CH_CFG_MEM_WIDTH(1)
#define STM32_DMA_MEM_32BITS			STM32_DMA_CH_CFG_MEM_WIDTH(2)
#define STM32_DMA_MEM_64BITS			STM32_DMA_CH_CFG_MEM_WIDTH(3)

/** DMA  Peripheral increment offset config on bit 15 */
#define STM32_DMA_CH_CFG_PERIPH_INC_FIXED(val)	((val & 0x1) << 15)
#define STM32_DMA_OFFSET_LINKED_BUS		STM32_DMA_CH_CFG_PERIPH_INC_FIXED(0)
#define STM32_DMA_OFFSET_FIXED_4		STM32_DMA_CH_CFG_PERIPH_INC_FIXED(1)

/** DMA  Priority config  on bits 16, 17*/
#define STM32_DMA_CH_CFG_PRIORITY(val)		((val & 0x3) << 16)
#define STM32_DMA_PRIORITY_LOW			STM32_DMA_CH_CFG_PRIORITY(0)
#define STM32_DMA_PRIORITY_MEDIUM		STM32_DMA_CH_CFG_PRIORITY(1)
#define STM32_DMA_PRIORITY_HIGH			STM32_DMA_CH_CFG_PRIORITY(2)
#define STM32_DMA_PRIORITY_VERY_HIGH		STM32_DMA_CH_CFG_PRIORITY(3)

/** DMA  FIFO threshold feature */
#define STM32_DMA_FIFO_1_4			0U
#define STM32_DMA_FIFO_HALF			1U
#define STM32_DMA_FIFO_3_4			2U
#define STM32_DMA_FIFO_FULL			3U

/* DMA  usual combination for peripheral transfer */
#define STM32_DMA_PERIPH_TX	(STM32_DMA_MEMORY_TO_PERIPH | STM32_DMA_MEM_INC)
#define STM32_DMA_PERIPH_RX	(STM32_DMA_PERIPH_TO_MEMORY | STM32_DMA_MEM_INC)

#define STM32_DMA_16BITS	(STM32_DMA_PERIPH_16BITS | STM32_DMA_MEM_16BITS)

/** @} */

/**
 * @name custom HPDMA-specific configuration flags
 * @{
 */
/* HPDMA  Source Burst Increment config on bit 18 */
#define STM32_DMA_CH_CFG_SRC_BURST_INC(val) ((val & 0x1) << 18)
#define STM32_DMA_SRC_BURST_NO_INC             STM32_DMA_CH_CFG_SRC_BURST_INC(0)
#define STM32_DMA_SRC_BURST_INC                STM32_DMA_CH_CFG_SRC_BURST_INC(1)

/* HPDMA  Source Port config on bit 19 */
#define STM32_DMA_CH_CFG_SRC_PORT(val) ((val & 0x1) << 19)
#define STM32_DMA_SRC_PORT_0              STM32_DMA_CH_CFG_SRC_PORT(0)
#define STM32_DMA_SRC_PORT_1              STM32_DMA_CH_CFG_SRC_PORT(1)

/* HPDMA  Destination Burst Increment config on bit 20 */
#define STM32_DMA_CH_CFG_DEST_BURST_INC(val) ((val & 0x1) << 20)
#define STM32_DMA_DEST_BURST_NO_INC          STM32_DMA_CH_CFG_DEST_BURST_INC(0)
#define STM32_DMA_DEST_BURST_INC             STM32_DMA_CH_CFG_DEST_BURST_INC(1)

/* HPDMA  Destination Port config on bit 21 */
#define STM32_DMA_CH_CFG_DEST_PORT(val) ((val & 0x1) << 21)
#define STM32_DMA_DEST_PORT_0           STM32_DMA_CH_CFG_DEST_PORT(0)
#define STM32_DMA_DEST_PORT_1           STM32_DMA_CH_CFG_DEST_PORT(1)

/* HPDMA  Hardware Request Type config on bit 22 */
#define STM32_DMA_CH_CFG_HW_REQUEST_MODE(val) ((val & 0x1) << 22)
#define STM32_DMA_HW_BURST_REQUEST_MODE       STM32_DMA_CH_CFG_HW_REQUEST_MODE(0)
#define STM32_DMA_HW_BLOCK_REQUEST_MODE       STM32_DMA_CH_CFG_HW_REQUEST_MODE(1)

/* HPDMA  Control Mode config on bit 23 */
#define STM32_DMA_CH_CFG_CTRL_MODE(val) ((val & 0x1) << 23)
#define STM32_DMA_CTRL_MODE_DMA         STM32_DMA_CH_CFG_CTRL_MODE(0)
#define STM32_DMA_CTRL_MODE_PERIPH      STM32_DMA_CH_CFG_CTRL_MODE(1)

/* HPDMA  Transfer Complete Event config on bits 24, 25 */
#define STM32_DMA_CH_CFG_TC_EVENT_MODE(val) ((val & 0x3) << 24)
#define STM32_DMA_TC_EVENT_BLOCK            STM32_DMA_CH_CFG_TC_EVENT_MODE(0)
#define STM32_DMA_TC_EVENT_LLI              STM32_DMA_CH_CFG_TC_EVENT_MODE(2)
#define STM32_DMA_TC_EVENT_CHANNEL          STM32_DMA_CH_CFG_TC_EVENT_MODE(3)

/* HPDMA  Prevent Packing/Unpacking config on bit 26 */
#define STM32_DMA_CH_CFG_PREVENT_PACKING(val) ((val & 0x1) << 26)
#define STM32_DMA_ALLOW_PACKING               STM32_DMA_CH_CFG_PREVENT_PACKING(0)
#define STM32_DMA_PREVENT_PACKING             STM32_DMA_CH_CFG_PREVENT_PACKING(1)

/* HPDMA  Prevent Additional Transfers config on bit 27 */
#define STM32_DMA_CH_CFG_PREVENT_ADD_TRANSFERS(val) ((val & 0x1) << 27)
#define STM32_DMA_ALLOW_ADDITIONAL_TRANSFERS        STM32_DMA_CH_CFG_PREVENT_ADD_TRANSFERS(0)
#define STM32_DMA_PREVENT_ADDITIONAL_TRANSFERS      STM32_DMA_CH_CFG_PREVENT_ADD_TRANSFERS(1)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_STM32_DMA_H_ */
