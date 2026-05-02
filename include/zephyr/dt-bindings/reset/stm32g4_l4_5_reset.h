/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32G4_L4_5_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32G4_L4_5_RESET_H_

#include "stm32-common.h"

/* RCC bus reset register offset */
#define STM32_RESET_BUS_AHB1  0x28
#define STM32_RESET_BUS_AHB2  0x2C
#define STM32_RESET_BUS_AHB3  0x30
#define STM32_RESET_BUS_APB1L 0x38
#define STM32_RESET_BUS_APB1H 0x3C
#define STM32_RESET_BUS_APB2  0x40

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32G4_L4_5_RESET_H_ */
