/*
 * Copyright (c) 2018 Yurii Hamann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the ST STM32F7 family processors.
 *
 * Based on reference manual:
 * RM0385 Reference manual STM32F75xxx and STM32F74xxx
 *   advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 2.2.2: Memory map and register boundary addresses
 */

#ifndef _STM32F7_SOC_H_
#define _STM32F7_SOC_H_

#ifndef _ASMLANGUAGE

#include <stm32f7xx.h>

/* The STM32 HAL headers define these, but they conflict with the Zephyr can.h */
#undef CAN_MODE_NORMAL
#undef CAN_MODE_LOOPBACK

/* Add generated devicetree information and STM32 helper macros */
#include <st_stm32_dt.h>

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F7_SOC_H_ */
