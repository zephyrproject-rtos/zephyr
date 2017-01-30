/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32_LL_CLOCK_H_
#define _STM32_LL_CLOCK_H_

void config_pll_init(LL_UTILS_PLLInitTypeDef *pllinit);

/* Section for functions not available in every Cube packages */
void LL_RCC_MSI_Disable(void);

#endif /* _STM32_LL_CLOCK_H_ */
