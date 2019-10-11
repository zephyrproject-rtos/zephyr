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

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <stm32f7xx.h>

/* Add include for DTS generated information */
#include <generated_dts_board.h>

#ifdef CONFIG_EXTI_STM32
#include <stm32f7xx_ll_exti.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32f7xx_ll_utils.h>
#include <stm32f7xx_ll_bus.h>
#include <stm32f7xx_ll_rcc.h>
#include <stm32f7xx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32f7xx_ll_usart.h>
#endif

#ifdef CONFIG_I2C_STM32
#include <stm32f7xx_ll_i2c.h>
#endif

#if defined(CONFIG_SPI_STM32) || defined(CONFIG_I2S_STM32)
#include <stm32f7xx_ll_spi.h>
#endif

#ifdef CONFIG_ENTROPY_STM32_RNG
#include <stm32f7xx_ll_rng.h>
#endif

#if defined(CONFIG_COUNTER_RTC_STM32)
#include <stm32f7xx_ll_rtc.h>
#include <stm32f7xx_ll_exti.h>
#include <stm32f7xx_ll_pwr.h>
#endif

#ifdef CONFIG_GPIO_STM32
#include <stm32f7xx_ll_gpio.h>
#endif

#ifdef CONFIG_IWDG_STM32
#include <stm32f7xx_ll_iwdg.h>
#endif

#ifdef CONFIG_WWDG_STM32
#include <stm32f7xx_ll_wwdg.h>
#endif

#ifdef CONFIG_ADC_STM32
#include <stm32f7xx_ll_adc.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F7_SOC_H_ */
