/*
 * Copyright (c) 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_SOC_H_
#if defined(CONFIG_SOC_SERIES_IMX_RT10XX)
#include "pinctrl_rt10xx.h"
#elif defined(CONFIG_SOC_SERIES_IMX_RT11XX)
#include "pinctrl_rt11xx.h"
#endif

#endif /* ZEPHYR_SOC_ARM_NXP_IMX_RT_PINCTRL_SOC_H_ */
