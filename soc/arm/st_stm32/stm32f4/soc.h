/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the ST STM32F4 family processors.
 *
 * Based on reference manual:
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 2.3: Memory Map
 */

#ifndef _STM32F4_SOC_H_
#define _STM32F4_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32f4xx.h>

/* Add generated devicetree information and STM32 helper macros */
#include <st_stm32_dt.h>

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F4_SOC_H_ */
