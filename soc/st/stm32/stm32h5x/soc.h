/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32H5 family processors.
 *
 */


#ifndef _STM32H5_SOC_H_
#define _STM32H5_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32h5xx.h>

/* PAGESIZE is defined by HAL legacy headers, but conflict with POSIX */
#undef PAGESIZE

#endif /* !_ASMLANGUAGE */

#endif /* _STM32H5_SOC_H_ */
