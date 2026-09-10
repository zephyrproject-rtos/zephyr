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
#define STM32_PWR_WKUP_EVT_SRC_0	0
#define STM32_PWR_WKUP_EVT_SRC_1	1
#define STM32_PWR_WKUP_EVT_SRC_2	2

/**
 * @brief STM32 wake-up line source selection bitmask
 *
 * @note Up to four sources supported for now (0~3)
 */
#define STM32_PWR_WKUP_LINE_SRC_MASK	(0x3)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_STM32_PWR_H_ */
