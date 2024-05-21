/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32_PWR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32_PWR_H_

/**
 * @brief STM32 power controller
 * @{
 */

/**
 * @name flags for wake-up pins sources
 * @{
 */

#define STM32_PWR_WKUP_PIN_SRC_0	0
#define STM32_PWR_WKUP_PIN_SRC_1	1
#define STM32_PWR_WKUP_PIN_SRC_2	(1 << 2)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32_PWR_H_ */
