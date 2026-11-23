/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 reset controller devicetree helper macros for STM32WB0
 * @ingroup reset_controller_stm32
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32WB0_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32WB0_RESET_H_

#include "stm32-common.h"

/** @cond INTERNAL_HIDDEN */

/* RCC bus reset register offset */
#define STM32_RESET_BUS_AHB0	0x30
#define STM32_RESET_BUS_APB0	0x34
#define STM32_RESET_BUS_APB1	0x38
#define STM32_RESET_BUS_APB2	0x3C

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32WB0_RESET_H_ */
