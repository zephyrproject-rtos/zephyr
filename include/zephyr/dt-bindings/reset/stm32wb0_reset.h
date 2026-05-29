/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32WB0_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32WB0_RESET_H_

#include "stm32-common.h"

/* RCC bus reset register offset */
#define STM32_RESET_BUS_AHB0	0x30
#define STM32_RESET_BUS_APB0	0x34
#define STM32_RESET_BUS_APB1	0x38
#define STM32_RESET_BUS_APB2	0x3C

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32WB0_RESET_H_ */
