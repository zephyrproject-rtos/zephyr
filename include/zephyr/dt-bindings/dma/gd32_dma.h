/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GD32_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GD32_DMA_H_

/* macros for channel-cfg */

/* direction defined on bits 6-7 */
#define GD32_DMA_CH_CFG_DIRECTION(val) ((val & 0x3) << 6)
#define GD32_DMA_MEMORY_TO_MEMORY      GD32_DMA_CH_CFG_DIRECTION(0)
#define GD32_DMA_MEMORY_TO_PERIPH      GD32_DMA_CH_CFG_DIRECTION(1)
#define GD32_DMA_PERIPH_TO_MEMORY      GD32_DMA_CH_CFG_DIRECTION(2)

/* periph increase defined on bit 9 as true/false */
#define GD32_DMA_CH_CFG_PERIPH_ADDR_INC(val) ((val & 0x1) << 9)
#define GD32_DMA_NO_PERIPH_ADDR_INC	     GD32_DMA_CH_CFG_PERIPH_ADDR_INC(0)
#define GD32_DMA_PERIPH_ADDR_INC	     GD32_DMA_CH_CFG_PERIPH_ADDR_INC(1)

/* memory increase defined on bit 10 as true/false */
#define GD32_DMA_CH_CFG_MEMORY_ADDR_INC(val) ((val & 0x1) << 10)
#define GD32_DMA_NO_MEMORY_ADDR_INC	     GD32_DMA_CH_CFG_MEMORY_ADDR_INC(0)
#define GD32_DMA_MEMORY_ADDR_INC	     GD32_DMA_CH_CFG_MEMORY_ADDR_INC(1)

/* periph data size defined on bits 11-12 */
#define GD32_DMA_CH_CFG_PERIPH_WIDTH(val) ((val & 0x3) << 11)
#define GD32_DMA_PERIPH_WIDTH_8BIT	  GD32_DMA_CH_CFG_PERIPH_WIDTH(0)
#define GD32_DMA_PERIPH_WIDTH_16BIT	  GD32_DMA_CH_CFG_PERIPH_WIDTH(1)
#define GD32_DMA_PERIPH_WIDTH_32BIT	  GD32_DMA_CH_CFG_PERIPH_WIDTH(2)

/* memory data size defined on bits 13-14 */
#define GD32_DMA_CH_CFG_MEMORY_WIDTH(val) ((val & 0x3) << 13)
#define GD32_DMA_MEMORY_WIDTH_8BIT	  GD32_DMA_CH_CFG_PERIPH_WIDTH(0)
#define GD32_DMA_MEMORY_WIDTH_16BIT	  GD32_DMA_CH_CFG_PERIPH_WIDTH(1)
#define GD32_DMA_MEMORY_WIDTH_32BIT	  GD32_DMA_CH_CFG_PERIPH_WIDTH(2)

/* priority increment offset defined on bit 15 */
#define GD32_DMA_CH_CFG_PERIPH_INC_FIXED(val) ((val & 0x1) << 15)

/* priority defined on bits 16-17 as 0, 1, 2, 3 */
#define GD32_DMA_CH_CFG_PRIORITY(val) ((val & 0x3) << 16)
#define GD32_DMA_PRIORITY_LOW	      GD32_DMA_CH_CFG_PRIORITY(0)
#define GD32_DMA_PRIORITY_MEDIUM      GD32_DMA_CH_CFG_PRIORITY(1)
#define GD32_DMA_PRIORITY_HIGH	      GD32_DMA_CH_CFG_PRIORITY(2)
#define GD32_DMA_PRIORITY_VERY_HIGH   GD32_DMA_CH_CFG_PRIORITY(3)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GD32_DMA_H_ */
