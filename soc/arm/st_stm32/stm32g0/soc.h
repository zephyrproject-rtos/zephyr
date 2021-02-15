/*
 * Copyright (c) 2019 Philippe Retornaz <philippe@shapescale.com>
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32G0 family processors.
 *
 * Based on reference manual:
 *   STM32G0X advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 2.2: Memory organization
 */


#ifndef _STM32G0_SOC_H_
#define _STM32G0_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32g0xx.h>

/* Add generated devicetree information and STM32 helper macros */
#include <st_stm32_dt.h>

#endif /* !_ASMLANGUAGE */

#endif /* _STM32G0_SOC_H_ */
