/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32H5_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32H5_RESET_H_

#include "stm32-common.h"

/* RCC bus reset register offset */
#define STM32_RESET_BUS_AHB1  0x60
#define STM32_RESET_BUS_AHB2  0x64
#define STM32_RESET_BUS_AHB4  0x6C
#define STM32_RESET_BUS_APB1L 0x74
#define STM32_RESET_BUS_APB1H 0x78
#define STM32_RESET_BUS_APB2  0x7C
#define STM32_RESET_BUS_APB3  0x80

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32H5_RESET_H_ */
