/*
 * Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_SOC_TCC_TCC7045_SOC_H_
#define ZEPHYR_SOC_TCC_TCC7045_SOC_H_

#include "soc_reg_phys.h"

#define TCC_NULL_PTR (void *)0

#define TCC_ON  1U
#define TCC_OFF 0U

#define SYS_PWR_EN (GPIO_GPC(2UL))

int32_t soc_div64_to_32(uint64_t *pullDividend, uint32_t uiDivisor, uint32_t *puiRem);

#endif /* ZEPHYR_SOC_TCC_TCC7045_SOC_H_ */
