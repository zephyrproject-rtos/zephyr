/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F7_SOC_H_
#define _STM32F7_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <autoconf.h>
#include <stm32h7xx.h>

/* Add include for DTS generated information */
#include <st_stm32_dt.h>

#ifdef CONFIG_STM32H7_DUAL_CORE

#ifdef CONFIG_CPU_CORTEX_M4

#include <stm32h7xx_ll_bus.h>
#include <stm32h7xx_ll_pwr.h>
#include <stm32h7xx_ll_cortex.h>

#endif /* CONFIG_CPU_CORTEX_M4 */

#endif /* CONFIG_STM32H7_DUAL_CORE */

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F7_SOC_H7_ */
