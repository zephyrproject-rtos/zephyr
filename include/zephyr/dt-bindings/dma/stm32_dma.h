/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_STM32_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_STM32_DMA_H_

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

/** DMA  Memory data size config on bits 13, 14 */
#define STM32_DMA_CH_CFG_MEM_WIDTH(val)		((val & 0x3) << 13)
#define STM32_DMA_MEM_8BITS			STM32_DMA_CH_CFG_MEM_WIDTH(0)
#define STM32_DMA_MEM_16BITS			STM32_DMA_CH_CFG_MEM_WIDTH(1)
#define STM32_DMA_MEM_32BITS			STM32_DMA_CH_CFG_MEM_WIDTH(2)

/** DMA  Peripheral increment offset config on bit 15 */
#define STM32_DMA_CH_CFG_PERIPH_INC_FIXED(val)	((val & 0x1) << 15)

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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_STM32_DMA_H_ */
