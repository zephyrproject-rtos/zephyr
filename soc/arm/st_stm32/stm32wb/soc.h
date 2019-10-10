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
#include <generated_dts_board.h>

#ifdef CONFIG_GPIO_STM32
#include <stm32wbxx_ll_gpio.h>
#endif

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32wbxx_ll_usart.h>
#include <stm32wbxx_ll_lpuart.h>
#endif

#if defined(CONFIG_COUNTER_RTC_STM32)
#include <stm32wbxx_ll_rtc.h>
#include <stm32wbxx_ll_exti.h>
#include <stm32wbxx_ll_pwr.h>
#include <stm32wbxx_ll_bus.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32wbxx_ll_utils.h>
#include <stm32wbxx_ll_bus.h>
#include <stm32wbxx_ll_rcc.h>
#include <stm32wbxx_ll_system.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_FLASH
#include <stm32wbxx_ll_hsem.h>
#endif /* CONFIG_FLASH */

#ifdef CONFIG_I2C_STM32
#include <stm32wbxx_ll_i2c.h>
#endif /* CONFIG_I2C_STM32 */

#ifdef CONFIG_SPI_STM32
#include <stm32wbxx_ll_spi.h>
#endif /* CONFIG_SPI_STM32 */

#ifdef CONFIG_ADC_STM32
#include <stm32wbxx_ll_adc.h>
#endif /* CONFIG_ADC_STM32 */

#ifdef CONFIG_IWDG_STM32
#include <stm32wbxx_ll_iwdg.h>
#endif /* CONFIG_IWDG_STM32 */

#ifdef CONFIG_WWDG_STM32
#include <stm32wbxx_ll_wwdg.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32WBX_SOC_H_ */
