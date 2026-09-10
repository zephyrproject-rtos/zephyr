/*
 * Copyright (c) 2018 qianfan Zhao <qianfanguijin@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the stm32f2 family processors.
 *
 * Based on reference manual:
 *   stm32f2X advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 2.2: Memory organization
 */


#ifndef _STM32F2_SOC_H_
#define _STM32F2_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32f2xx.h>

/* The STM32 HAL headers define these, but they conflict with the Zephyr can.h */
#undef CAN_MODE_NORMAL
#undef CAN_MODE_LOOPBACK

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F2_SOC_H_ */
