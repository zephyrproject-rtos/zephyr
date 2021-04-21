/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32MP1 family processors.
 *
 * Based on reference manual:
 *   STM32MP157 advanced ARM(r)-based 32-bit MPUs
 *
 * Chapter 2.2.2: Memory map and register boundary addresses
 */


#ifndef _STM32MP1SOC_H_
#define _STM32MP1SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32mp1xx.h>

/* Add generated devicetree information and STM32 helper macros */
#include <st_stm32_dt.h>

#endif /* !_ASMLANGUAGE */

#endif /* _STM32MP1SOC_H_ */
