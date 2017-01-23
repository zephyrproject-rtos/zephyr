/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32_LL_CLOCK_H_
#define _STM32_LL_CLOCK_H_

void config_pll_init(LL_UTILS_PLLInitTypeDef *pllinit);

struct regval_map {
	int val;
	uint32_t reg;
};

uint32_t map_reg_val(const struct regval_map *map, size_t cnt, int val);

/*  */
void LL_AHB2_GRP1_EnableClock(uint32_t Periphs);
void LL_RCC_MSI_Disable(void);
#endif /* _STM32_LL_CLOCK_H_ */
