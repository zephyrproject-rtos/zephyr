/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32_PWR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32_PWR_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @brief STM32 power controller
 * @{
 */

/**
 * @name flags for wake-up pins sources
 * @{
 */

/* Use this flag on series where wake-up event source is fixed/not configurable */
#define STM32_PWR_WKUP_PIN_NOT_MUXED	STM32_PWR_WKUP_EVT_SRC_0
#define STM32_PWR_WKUP_EVT_SRC_0	BIT(0)
#define STM32_PWR_WKUP_EVT_SRC_1	BIT(1)
#define STM32_PWR_WKUP_EVT_SRC_2	BIT(2)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32_PWR_H_ */
