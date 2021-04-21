/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WATCHDOG_IWDG_STM32_H_
#define ZEPHYR_DRIVERS_WATCHDOG_IWDG_STM32_H_

#include <zephyr/types.h>

/**
 * @brief Driver for Independent Watchdog (IWDG) for STM32 MCUs
 *
 * The driver targets all STM32 SoCs. For details please refer to
 * an appropriate reference manual and look for chapter called:
 *
 *   Independent watchdog (IWDG)
 *
 */

/* driver data */
struct iwdg_stm32_data {
	/* IWDG peripheral instance. */
	IWDG_TypeDef *Instance;
};

#define IWDG_STM32_DATA(dev)					\
	((struct iwdg_stm32_data * const)(dev)->data)

#define IWDG_STM32_STRUCT(dev)					\
	((IWDG_TypeDef *)(IWDG_STM32_DATA(dev))->Instance)

#endif	/* ZEPHYR_DRIVERS_WATCHDOG_IWDG_STM32_H_ */
