/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_STM32_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_STM32_H_

/* @brief linked_channel value to inform zephyr dma driver that
 * DMA channel will be handled by HAL
 */
#define STM32_DMA_HAL_OVERRIDE      0x7F

/* @brief gives the first DMA channel : 0 or 1 in the register map
 * when counting channels from 1 to N or from 0 to N-1
 */
#if defined(CONFIG_DMA_STM32U5)
/* from DTS the dma stream id is in range 0..N-1 */
#define STM32_DMA_STREAM_OFFSET 0
#elif !defined(CONFIG_DMA_STM32_V1)
/* from DTS the dma stream id is in range 1..N */
/* so decrease to set range from 0 from now on */
#define STM32_DMA_STREAM_OFFSET 1
#elif defined(CONFIG_DMA_STM32_V1) && defined(CONFIG_DMAMUX_STM32)
/* typically on the stm32H7 series, DMA V1 with mux */
#define STM32_DMA_STREAM_OFFSET 1
#else
/* from DTS the dma stream id is in range 0..N-1 */
#define STM32_DMA_STREAM_OFFSET 0
#endif /* ! CONFIG_DMA_STM32_V1 */

/* macro for dma slot (only for dma-v1 or dma-v2 types) */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v2bis)
#define STM32_DMA_SLOT(id, dir, slot) 0
#define STM32_DMA_SLOT_BY_IDX(id, idx, slot) 0
#else
#define STM32_DMA_SLOT(id, dir, slot) DT_INST_DMAS_CELL_BY_NAME(id, dir, slot)
#define STM32_DMA_SLOT_BY_IDX(id, idx, slot) DT_INST_DMAS_CELL_BY_IDX(id, idx, slot)
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v2) || \
	DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v2bis) || \
	DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dmamux)
#define STM32_DMA_FEATURES(id, dir) 0
#else
#define STM32_DMA_FEATURES(id, dir)						\
		DT_INST_DMAS_CELL_BY_NAME(id, dir, features)
#endif

#define STM32_DMA_CTLR(id, dir)						\
		DT_INST_DMAS_CTLR_BY_NAME(id, dir)
#define STM32_DMA_CHANNEL_CONFIG(id, dir)					\
		DT_INST_DMAS_CELL_BY_NAME(id, dir, channel_config)
#define STM32_DMA_CHANNEL_CONFIG_BY_IDX(id, idx)				\
		DT_INST_DMAS_CELL_BY_IDX(id, idx, channel_config)

/* macros for channel-config */
/* direction defined on bits 6-7 */
/* 0 -> MEM_TO_MEM, 1 -> MEM_TO_PERIPH, 2 -> PERIPH_TO_MEM */
#define STM32_DMA_CONFIG_DIRECTION(config)		((config >> 6) & 0x3)
/* periph increment defined on bit 9 as true/false */
#define STM32_DMA_CONFIG_PERIPHERAL_ADDR_INC(config)	((config >> 9) & 0x1)
/* mem increment defined on bit 10 as true/false */
#define STM32_DMA_CONFIG_MEMORY_ADDR_INC(config)	((config >> 10) & 0x1)
/* periph data size defined on bits 11-12 */
/* 0 -> 1 byte, 1 -> 2 bytes, 2 -> 4 bytes */
#define STM32_DMA_CONFIG_PERIPHERAL_DATA_SIZE(config)	\
						(1 << ((config >> 11) & 0x3))
/* memory data size defined on bits 13, 14 */
/* 0 -> 1 byte, 1 -> 2 bytes, 2 -> 4 bytes */
#define STM32_DMA_CONFIG_MEMORY_DATA_SIZE(config)	\
						(1 << ((config >> 13) & 0x3))
/* priority increment offset defined on bit 15 */
#define STM32_DMA_CONFIG_PERIPHERAL_INC_FIXED(config)	((config >> 15) & 0x1)
/* priority defined on bits 16-17 as 0, 1, 2, 3 */
#define STM32_DMA_CONFIG_PRIORITY(config)		((config >> 16) & 0x3)

/* macro for features (only for dma-v1) */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_dma_v1)
#define STM32_DMA_FEATURES_FIFO_THRESHOLD(features)	(features & 0x3)
#else
#define STM32_DMA_FEATURES_FIFO_THRESHOLD(features)	0
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_STM32_H_ */
