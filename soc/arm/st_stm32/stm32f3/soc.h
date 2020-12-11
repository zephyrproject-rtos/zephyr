/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32F3 family processors.
 *
 * Based on reference manual:
 *   STM32F303xB/C/D/E, STM32F303x6/8, STM32F328x8, STM32F358xC,
 *   STM32F398xE advanced ARM(r)-based MCUs
 *   STM32F37xx advanced ARM(r)-based MCUs
 *
 * Chapter 3.3: Memory organization
 */


#ifndef _STM32F3_SOC_H_
#define _STM32F3_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32f3xx.h>

/* Add generated devicetree information and STM32 helper macros */
#include <st_stm32_dt.h>

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F3_SOC_H_ */
