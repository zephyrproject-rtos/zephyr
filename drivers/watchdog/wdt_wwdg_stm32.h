/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WATCHDOG_WWDG_STM32_H_
#define ZEPHYR_DRIVERS_WATCHDOG_WWDG_STM32_H_

#include <zephyr/types.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/clock_control.h>

/**
 * @brief Driver for System Window Watchdog (WWDG) for STM32 MCUs
 *
 * The driver targets all STM32 SoCs. For details please refer to
 * an appropriate reference manual and look for chapter called:
 *
 *   System window watchdog (WWDG)
 *
 */

/* driver configuration */
struct wwdg_stm32_config {
	struct stm32_pclken pclken;
	WWDG_TypeDef *Instance;
};

/* driver data */
struct wwdg_stm32_data {
	/* WWDG reset counter */
	uint8_t counter;

	/* WWDG user defined callback on EWI */
	wdt_callback_t callback;
};

#define WWDG_STM32_CFG(dev) \
	((const struct wwdg_stm32_config *const)(dev)->config)

#define WWDG_STM32_DATA(dev) \
	((struct wwdg_stm32_data *const)(dev)->data)

#define WWDG_STM32_STRUCT(dev) \
	((WWDG_TypeDef *)(WWDG_STM32_CFG(dev))->Instance)

#endif  /* ZEPHYR_DRIVERS_WATCHDOG_WWDG_STM32_H_ */
