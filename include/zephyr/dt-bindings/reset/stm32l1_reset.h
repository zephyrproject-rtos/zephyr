/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 reset controller devicetree helper macros for STM32L1
 * @ingroup reset_controller_stm32
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32L1_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32L1_RESET_H_

#include "stm32-common.h"

/** @cond INTERNAL_HIDDEN */

/* RCC bus reset register offset */
#define STM32_RESET_BUS_AHB1 0x10
#define STM32_RESET_BUS_APB1 0x18
#define STM32_RESET_BUS_APB2 0x14

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32L1_RESET_H_ */
