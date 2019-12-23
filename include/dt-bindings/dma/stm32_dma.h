/*
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* macros for channel-config */
#define STM32_DMA_CONFIG_PERIPH_ADDR_INC(config)	(config & 0x1 << 9)
#define STM32_DMA_CONFIG_MEM_ADDR_INC(config)		(config & 0x1 << 10)
#define STM32_DMA_CONFIG_PERIPH_INC_FIXED(config)	(config & 0x1 << 15)
#define STM32_DMA_CONFIG_PRIORITY(config)		((config >> 16) & 0x3)

/* macros for features */
#define STM32_DMA_FEATURES_FIFO_THRESHOLD(features)	(features & 0x3)
