/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32WBA series processors.
 *
 */


#ifndef _STM32WBA_SOC_H_
#define _STM32WBA_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32wbaxx.h>

/* Functions to support PM suspend and resume for radio
 * drivers
 */
void stm32wba_radio_pm_resume(void);
void stm32wba_radio_pm_suspend(void);

/* function exported to the soc power.c */
void stm32wba_init(void);

void stm32_power_init(void);

#endif /* !_ASMLANGUAGE */

#endif /* _STM32WBA_SOC_H_ */
