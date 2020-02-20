/*
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_STM32_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_STM32_DMA_H_

/* macros for channel-config */
#if defined(CONFIG_DMA_STM32_V1)
#define STM32_DMA_CONFIG_DIRECTION(config)		(config & 0x3 << 6)
#define STM32_DMA_CONFIG_PERIPH_ADDR_INC(config)	(config & 0x1 << 9)
#define STM32_DMA_CONFIG_MEM_ADDR_INC(config)		(config & 0x1 << 10)
#define STM32_DMA_CONFIG_PERIPH_DATA_SIZE(config)	(config & (0x3 << 11))
#define STM32_DMA_CONFIG_MEM_DATA_SIZE(config)		(config & (0x3 << 13))
#define STM32_DMA_CONFIG_PERIPH_INC_FIXED(config)	(config & 0x1 << 15)
#define STM32_DMA_CONFIG_PRIORITY(config)		((config >> 16) & 0x3)
#else
/* direction defined on bits 6-7 */
/* 0 -> MEM_TO_MEM, 1 -> MEM_TO_PERIPH, 2 -> PERIPH_TO_MEM */
#define STM32_DMA_CONFIG_DIRECTION(config)		((config >> 6) & 0x3)
/* periph increment defined on bit 9 as true/false */
#define STM32_DMA_CONFIG_PERIPH_ADDR_INC(config)	((config >> 9) & 0x1)
/* mem increment defined on bit 10 as true/false */
#define STM32_DMA_CONFIG_MEM_ADDR_INC(config)		((config >> 10) & 0x1)
/* perih data size defined on bits 11-12 */
/* 0 -> 1 byte, 1 -> 2 bytes, 2 -> 4 bytes */
#define STM32_DMA_CONFIG_PERIPH_DATA_SIZE(config)	\
	((((config >> 11) & 0x3) == 0) ? 1 : (((config >> 11) & 0x3) * 2))
/* memory data size defined on bits 13, 14 */
/* 0 -> 1 byte, 1 -> 2 bytes, 2 -> 4 bytes */
#define STM32_DMA_CONFIG_MEM_DATA_SIZE(config)		\
	((((config >> 13) & 0x3) == 0) ? 1 : (((config >> 13) & 0x3) * 2))
/* priority increment offset defined on bit 15 */
#define STM32_DMA_CONFIG_PERIPH_INC_FIXED(config)	((config >> 15) & 0x1)
/* priority defined on bits 16-17 as 0, 1, 2, 3 */
#define STM32_DMA_CONFIG_PRIORITY(config)		((config >> 16) & 0x3)
#endif /* CONFIG_DMA_STM32_V1 */

/* macros for features */
#define STM32_DMA_FEATURES_FIFO_THRESHOLD(features)	(features & 0x3)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_STM32_DMA_H_ */
