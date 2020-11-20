/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32WB family processors.
 *
 * Based on reference manual:
 *   TODO: Provide reference when known
 *
 * Chapter 2.2.2: Memory map and register boundary addresses
 */


#ifndef _STM32WBX_SOC_H_
#define _STM32WBX_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <stm32wbxx.h>

/* Add include for DTS generated information */
#include <st_stm32_dt.h>

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32wbxx_ll_utils.h>
#include <stm32wbxx_ll_bus.h>
#include <stm32wbxx_ll_rcc.h>
#include <stm32wbxx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_STM32_LPTIM_TIMER
#include <stm32wbxx_ll_lptim.h>
#include <stm32wbxx_ll_system.h>
#endif

#ifdef CONFIG_DMA_STM32
#include <stm32wbxx_ll_dma.h>
#endif

#ifdef CONFIG_DMAMUX_STM32
#include <stm32wbxx_ll_dmamux.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32WBX_SOC_H_ */
