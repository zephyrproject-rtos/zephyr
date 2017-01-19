/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _STM32_CLOCK_CONTROL_H_
#define _STM32_CLOCK_CONTROL_H_

#include <clock_control.h>

/* common clock control device name for all STM32 chips */
#define STM32_CLOCK_CONTROL_NAME "stm32-cc"

#ifdef CONFIG_SOC_SERIES_STM32F1X
#include "stm32f1_clock_control.h"
#elif CONFIG_SOC_SERIES_STM32F4X
#include "stm32f4_clock_control.h"
#elif CONFIG_SOC_SERIES_STM32L4X
#include "stm32l4x_clock_control.h"
#endif

#endif /* _STM32_CLOCK_CONTROL_H_ */
