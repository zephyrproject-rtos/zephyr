/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 reset controller devicetree helper macros for STM32U0
 * @ingroup reset_controller_stm32
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32U0_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32U0_RESET_H_

#include "stm32-common.h"

/** @cond INTERNAL_HIDDEN */

/* RCC bus reset register offset */
#define STM32_RESET_BUS_IOP   0x2C
#define STM32_RESET_BUS_AHB1  0x28
#define STM32_RESET_BUS_APB1L 0x38
#define STM32_RESET_BUS_APB1H 0x40

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32U0_RESET_H_ */
