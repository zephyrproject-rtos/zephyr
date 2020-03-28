/*
 * Copyright (c) 2019 Linaro Limited
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32H7_SOC_H_
#define _STM32H7_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <autoconf.h>
#include <devicetree.h>

#ifdef CONFIG_STM32H7_DUAL_CORE
#define LL_HSEM_ID_0   (0U) /* HW semaphore 0 */
#define LL_HSEM_MASK_0 (1 << LL_HSEM_ID_0)
#define LL_HSEM_ID_1   (1U) /* HW semaphore 1 */
#define LL_HSEM_MASK_1 (1 << LL_HSEM_ID_1)
#endif /* CONFIG_STM32H7_DUAL_CORE */

#include <stm32h7xx.h>

#ifdef CONFIG_USE_STM32_LL_ADC
#include <stm32h7xx_ll_adc.h>
#endif
#ifdef CONFIG_USE_STM32_LL_BDMA
#include <stm32h7xx_ll_bdma.h>
#endif
#ifdef CONFIG_USE_STM32_LL_BUS
#include <stm32h7xx_ll_bus.h>
#endif
#ifdef CONFIG_USE_STM32_LL_COMP
#include <stm32h7xx_ll_comp.h>
#endif
#ifdef CONFIG_USE_STM32_LL_CORTEX
#include <stm32h7xx_ll_cortex.h>
#endif
#ifdef CONFIG_USE_STM32_LL_CRC
#include <stm32h7xx_ll_crc.h>
#endif
#ifdef CONFIG_USE_STM32_LL_CRS
#include <stm32h7xx_ll_crs.h>
#endif
#ifdef CONFIG_USE_STM32_LL_DAC
#include <stm32h7xx_ll_dac.h>
#endif
#ifdef CONFIG_USE_STM32_LL_DELAYBLOCK
#include <stm32h7xx_ll_delayblock.h>
#endif
#ifdef CONFIG_USE_STM32_LL_DMA
#include <stm32h7xx_ll_dma.h>
#endif
#ifdef CONFIG_USE_STM32_LL_DMA2D
#include <stm32h7xx_ll_dma2d.h>
#endif
#ifdef CONFIG_USE_STM32_LL_DMAMUX
#include <stm32h7xx_ll_dmamux.h>
#endif
#ifdef CONFIG_USE_STM32_LL_EXTI
#include <stm32h7xx_ll_exti.h>
#endif
#ifdef CONFIG_USE_STM32_LL_FMC
#include <stm32h7xx_ll_fmc.h>
#endif
#ifdef CONFIG_USE_STM32_LL_GPIO
#include <stm32h7xx_ll_gpio.h>
#endif
#ifdef CONFIG_USE_STM32_LL_HRTIM
#include <stm32h7xx_ll_hrtim.h>
#endif
#ifdef CONFIG_USE_STM32_LL_HSEM
#include <stm32h7xx_ll_hsem.h>
#endif
#ifdef CONFIG_USE_STM32_LL_I2C
#include <stm32h7xx_ll_i2c.h>
#endif
#ifdef CONFIG_USE_STM32_LL_IWDG
#include <stm32h7xx_ll_iwdg.h>
#endif
#ifdef CONFIG_USE_STM32_LL_LPTIM
#include <stm32h7xx_ll_lptim.h>
#endif
#ifdef CONFIG_USE_STM32_LL_LPUART
#include <stm32h7xx_ll_lpuart.h>
#endif
#ifdef CONFIG_USE_STM32_LL_MDMA
#include <stm32h7xx_ll_mdma.h>
#endif
#ifdef CONFIG_USE_STM32_LL_OPAMP
#include <stm32h7xx_ll_opamp.h>
#endif
#ifdef CONFIG_USE_STM32_LL_PWR
#include <stm32h7xx_ll_pwr.h>
#endif
#ifdef CONFIG_USE_STM32_LL_RCC
#include <stm32h7xx_ll_rcc.h>
#endif
#ifdef CONFIG_USE_STM32_LL_RNG
#include <stm32h7xx_ll_rng.h>
#endif
#ifdef CONFIG_USE_STM32_LL_RTC
#include <stm32h7xx_ll_rtc.h>
#endif
#ifdef CONFIG_USE_STM32_LL_SDMMC
#include <stm32h7xx_ll_sdmmc.h>
#endif
#ifdef CONFIG_USE_STM32_LL_SPI
#include <stm32h7xx_ll_spi.h>
#endif
#ifdef CONFIG_USE_STM32_LL_SWPMI
#include <stm32h7xx_ll_swpmi.h>
#endif
#ifdef CONFIG_USE_STM32_LL_SYSTEM
#include <stm32h7xx_ll_system.h>
#endif
#ifdef CONFIG_USE_STM32_LL_TIM
#include <stm32h7xx_ll_tim.h>
#endif
#ifdef CONFIG_USE_STM32_LL_USART
#include <stm32h7xx_ll_usart.h>
#endif
#ifdef CONFIG_USE_STM32_LL_USB
#include <stm32h7xx_ll_usb.h>
#endif
#ifdef CONFIG_USE_STM32_LL_UTILS
#include <stm32h7xx_ll_utils.h>
#endif
#ifdef CONFIG_USE_STM32_LL_WWDG
#include <stm32h7xx_ll_wwdg.h>
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _STM32H7_SOC_H_ */
