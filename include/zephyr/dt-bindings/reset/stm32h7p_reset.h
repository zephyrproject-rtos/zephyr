/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32H7P_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32H7P_RESET_H_

#include "stm32-common.h"

/** @cond INTERNAL_HIDDEN */

/* RCC bus reset register offset */
#define STM32_RESET_BUS_AHB1  0x80
#define STM32_RESET_BUS_AHB2  0x84
#define STM32_RESET_BUS_AHB3  0xA4
#define STM32_RESET_BUS_AHB4  0x88
#define STM32_RESET_BUS_AHB5  0x7C
#define STM32_RESET_BUS_APB1L 0x90
#define STM32_RESET_BUS_APB1H 0x94
#define STM32_RESET_BUS_APB2  0x98
#define STM32_RESET_BUS_APB4  0x9C

/** INTERNAL_HIDDEN @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32H7P_RESET_H_ */
