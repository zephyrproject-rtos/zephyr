/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32L5 family processors.
 *
 * Based on reference manual:
 *   STM32L5X advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 2.2: Memory organization
 */


#ifndef _STM32L5_SOC_H_
#define _STM32L5_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <autoconf.h>
#include <stm32l5xx.h>

/* Add include for DTS generated information */
#include <devicetree.h>

#endif /* !_ASMLANGUAGE */

#endif /* _STM32L5_SOC_H_ */
