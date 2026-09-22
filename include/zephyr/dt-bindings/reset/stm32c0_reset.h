/*
 * Copyright (c) 2022 Google Inc
 * Copyright (c) Benjamin Björnsson <benjamin.bjornsson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 reset controller devicetree helper macros for STM32C0
 * @ingroup reset_controller_stm32
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32C0_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32C0_RESET_H_

#include "stm32-common.h"

/** @cond INTERNAL_HIDDEN */

/* RCC bus reset register offset */
#define STM32_RESET_BUS_IOP   0x24
#define STM32_RESET_BUS_AHB1  0x28
#define STM32_RESET_BUS_APB1L 0x2C
#define STM32_RESET_BUS_APB1H 0x30

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32C0_RESET_H_ */
