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

/* Add include for DTS generated information */
#include <devicetree.h>

#include <stm32mp1xx_ll_hsem.h>

#ifdef CONFIG_EXTI_STM32
#include <stm32mp1xx_ll_exti.h>
#endif

#ifdef CONFIG_GPIO_STM32
#include <stm32mp1xx_ll_gpio.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32mp1xx_ll_utils.h>
#include <stm32mp1xx_ll_bus.h>
#include <stm32mp1xx_ll_rcc.h>
#include <stm32mp1xx_ll_system.h>
#endif

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32mp1xx_ll_usart.h>
#endif

#ifdef CONFIG_SPI_STM32
#include <stm32mp1xx_ll_spi.h>
#endif

#ifdef CONFIG_IPM_STM32_IPCC
#include <stm32mp1xx_ll_ipcc.h>
#endif

#ifdef CONFIG_I2C_STM32
#include <stm32mp1xx_ll_i2c.h>
#endif

#ifdef CONFIG_WWDG_STM32
#include <stm32mp1xx_ll_wwdg.h>
#endif

#ifdef CONFIG_PWM_STM32
#include <stm32mp1xx_ll_tim.h>
#endif /* CONFIG_PWM_STM32 */

#endif /* !_ASMLANGUAGE */

#endif /* _STM32MP1SOC_H_ */
