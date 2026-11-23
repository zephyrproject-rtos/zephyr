/*
 * Copyright (c) 2024 STMicroelectronics
 * Copyright (c) 2026 Anders Frandsen <anfran@anfran.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 reset controller devicetree helper macros for STM32WL3
 * @ingroup reset_controller_stm32
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32WL3_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32WL3_RESET_H_

#include "stm32-common.h"

/** @cond INTERNAL_HIDDEN */

/* RCC bus reset register offsets */
#define STM32_RESET_BUS_AHB0	0x30
#define STM32_RESET_BUS_APB0	0x34
#define STM32_RESET_BUS_APB1	0x38
#define STM32_RESET_BUS_APB2	0x40

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32WL3_RESET_H_ */
