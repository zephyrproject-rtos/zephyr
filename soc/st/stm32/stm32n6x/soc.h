/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32N6 family processors.
 *
 */


#ifndef _STM32N6_SOC_H_
#define _STM32N6_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32n6xx.h>

/**
 * Configure the external VCORE supply and select run-mode VOS0.
 *
 * The STM32N6 resets the run voltage selection after STOP. Call this before
 * configuring the high-speed clock tree at boot and after STOP exit.
 */
void stm32n6_configure_run_power(void);

#endif /* !_ASMLANGUAGE */

#endif /* _STM32N6_SOC_H_ */
