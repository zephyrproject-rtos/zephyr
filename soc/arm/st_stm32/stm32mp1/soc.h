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

#include <autoconf.h>
#include <stm32mp1xx.h>

/* ARM CMSIS definitions must be included before kernel_includes.h.
 * Therefore, it is essential to include kernel_includes.h after including
 * core SOC-specific headers.
 */
#include <kernel_includes.h>

#ifdef CONFIG_EXTI_STM32
#include <stm32mp1xx_ll_exti.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32MP1SOC_H_ */
