/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_STM32_LL_CLOCK_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_STM32_LL_CLOCK_H_

void config_pll_init(LL_UTILS_PLLInitTypeDef *pllinit);
void config_enable_default_clocks(void);

/* Section for functions not available in every Cube packages */
void LL_RCC_MSI_Disable(void);

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_STM32_LL_CLOCK_H_ */
