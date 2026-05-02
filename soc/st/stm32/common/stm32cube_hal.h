/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_ST_STM32_COMMON_STM32CUBE_HAL_H
#define ZEPHYR_SOC_ST_STM32_COMMON_STM32CUBE_HAL_H

#ifdef CONFIG_STM32_HAL2

#include <stm32_hal.h>

typedef hal_status_t		stm32_status_t;

#else /* CONFIG_STM32_HAL2 */

typedef HAL_StatusTypeDef	stm32_status_t;

#endif /* CONFIG_STM32_HAL2 */

#endif /* ZEPHYR_SOC_ST_STM32_COMMON_STM32CUBE_HAL_H */
