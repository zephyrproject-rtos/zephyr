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

/* function exported to the soc power.c */
int stm32wba_init(void);

#endif /* !_ASMLANGUAGE */

#endif /* _STM32WBA_SOC_H_ */
