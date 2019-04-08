/*
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_STM32_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_STM32_DMA_H_

#define STM32_DMA_MEMORY_TO_MEMORY        0
#define STM32_DMA_MEMORY_TO_PERIPH        1
#define STM32_DMA_PERIPH_TO_MEMORY        2

#define STM32_DMA_PRIORITY_VERYHIGH       0
#define STM32_DMA_PRIORITY_HIGH           1
#define STM32_DMA_PRIORITY_MEDIUM         2
#define STM32_DMA_PRIORITY_LOW            3

#define STM32_DMA_SRC_NO_INCREMENT        0
#define STM32_DMA_SRC_INCREMENT           1

#define STM32_DMA_DST_NO_INCREMENT        0
#define STM32_DMA_DST_INCREMENT           1

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_STM32_DMA_H_ */
