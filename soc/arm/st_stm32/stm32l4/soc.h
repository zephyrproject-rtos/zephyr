/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the STM32L4 family processors.
 *
 * Based on reference manual:
 *   STM32L4x1, STM32L4x2, STM32L431xx STM32L443xx STM32L433xx, STM32L4x5,
 *   STM32l4x6 advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 2.2.2: Memory map and register boundary addresses
 */


#ifndef _STM32L4X_SOC_H_
#define _STM32L4X_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <autoconf.h>
#include <stm32l4xx.h>

/* Add include for DTS generated information */
#include <generated_dts_board.h>

#ifdef CONFIG_EXTI_STM32
#include <stm32l4xx_ll_exti.h>
#endif

#ifdef CONFIG_GPIO_STM32
/* Required to enable VDDio2 for port G */
#include <stm32l4xx_ll_pwr.h>
#endif

#ifdef CONFIG_SERIAL_HAS_DRIVER
#include <stm32l4xx_ll_usart.h>
#include <stm32l4xx_ll_lpuart.h>
#endif

#ifdef CONFIG_CLOCK_CONTROL_STM32_CUBE
#include <stm32l4xx_ll_utils.h>
#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_rcc.h>
#include <stm32l4xx_ll_system.h>
#include <stm32l4xx_ll_pwr.h>
#endif /* CONFIG_CLOCK_CONTROL_STM32_CUBE */

#ifdef CONFIG_SPI_STM32
#include <stm32l4xx_ll_spi.h>
#endif

#ifdef CONFIG_I2C_STM32
#include <stm32l4xx_ll_i2c.h>
#endif

#ifdef CONFIG_IWDG_STM32
#include <stm32l4xx_ll_iwdg.h>
#endif

#ifdef CONFIG_WWDG_STM32
#include <stm32l4xx_ll_wwdg.h>
#endif

#ifdef CONFIG_ENTROPY_STM32_RNG
#include <stm32l4xx_ll_rng.h>
#endif

#if defined(CONFIG_COUNTER_RTC_STM32)
#include <stm32l4xx_ll_rtc.h>
#include <stm32l4xx_ll_exti.h>
#include <stm32l4xx_ll_pwr.h>
#endif

#ifdef CONFIG_USB
/* Required to remove USB transceiver supply isolation */
#include <stm32l4xx_ll_pwr.h>
#endif /* CONFIG_USB */

#ifdef CONFIG_GPIO_STM32
#include <stm32l4xx_ll_gpio.h>
#endif

#ifdef CONFIG_ADC_STM32
#include <stm32l4xx_ll_adc.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32L4X_SOC_H_ */
