/**
  ******************************************************************************
  * @file    stm32l1xx_ll_bus.h
  * @author  MCD Application Team
  * @brief   Header file of BUS LL module.

  @verbatim                
                      ##### RCC Limitations #####
  ==============================================================================
    [..]  
      A delay between an RCC peripheral clock enable and the effective peripheral 
      enabling should be taken into account in order to manage the peripheral read/write 
      from/to registers.
      (+) This delay depends on the peripheral mapping.
        (++) AHB & APB peripherals, 1 dummy read is necessary

    [..]  
      Workarounds:
      (#) For AHB & APB peripherals, a dummy read to the peripheral register has been
          inserted in each LL_{BUS}_GRP{x}_EnableClock() function.

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L1xx_LL_BUS_H
#define __STM32L1xx_LL_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx.h"

/** @addtogroup STM32L1xx_LL_Driver
  * @{
  */

#if defined(RCC)

/** @defgroup BUS_LL BUS
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup BUS_LL_Exported_Constants BUS Exported Constants
  * @{
  */

/** @defgroup BUS_LL_EC_AHB1_GRP1_PERIPH  AHB1 GRP1 PERIPH
  * @{
  */
#define LL_AHB1_GRP1_PERIPH_ALL            0xFFFFFFFFU
#define LL_AHB1_GRP1_PERIPH_GPIOA          RCC_AHBENR_GPIOAEN
#define LL_AHB1_GRP1_PERIPH_GPIOB          RCC_AHBENR_GPIOBEN
#define LL_AHB1_GRP1_PERIPH_GPIOC          RCC_AHBENR_GPIOCEN
#define LL_AHB1_GRP1_PERIPH_GPIOD          RCC_AHBENR_GPIODEN
#if defined(GPIOE)
#define LL_AHB1_GRP1_PERIPH_GPIOE          RCC_AHBENR_GPIOEEN
#endif/*GPIOE*/
#define LL_AHB1_GRP1_PERIPH_GPIOH          RCC_AHBENR_GPIOHEN
#if defined(GPIOF)
#define LL_AHB1_GRP1_PERIPH_GPIOF          RCC_AHBENR_GPIOFEN
#endif/*GPIOF*/
#if defined(GPIOG)
#define LL_AHB1_GRP1_PERIPH_GPIOG          RCC_AHBENR_GPIOGEN
#endif/*GPIOG*/
#define LL_AHB1_GRP1_PERIPH_SRAM           RCC_AHBLPENR_SRAMLPEN
#define LL_AHB1_GRP1_PERIPH_CRC            RCC_AHBENR_CRCEN
#define LL_AHB1_GRP1_PERIPH_FLASH          RCC_AHBENR_FLITFEN
#define LL_AHB1_GRP1_PERIPH_DMA1           RCC_AHBENR_DMA1EN
#if defined(DMA2)
#define LL_AHB1_GRP1_PERIPH_DMA2           RCC_AHBENR_DMA2EN
#endif/*DMA2*/
#if defined(AES)
#define LL_AHB1_GRP1_PERIPH_CRYP           RCC_AHBENR_AESEN
#endif/*AES*/
#if defined(FSMC_Bank1)
#define LL_AHB1_GRP1_PERIPH_FSMC           RCC_AHBENR_FSMCEN
#endif/*FSMC_Bank1*/
/**
  * @}
  */

/** @defgroup BUS_LL_EC_APB1_GRP1_PERIPH  APB1 GRP1 PERIPH
  * @{
  */
#define LL_APB1_GRP1_PERIPH_ALL            0xFFFFFFFFU
#define LL_APB1_GRP1_PERIPH_TIM2           RCC_APB1ENR_TIM2EN
#define LL_APB1_GRP1_PERIPH_TIM3           RCC_APB1ENR_TIM3EN
#define LL_APB1_GRP1_PERIPH_TIM4           RCC_APB1ENR_TIM4EN
#if defined(TIM5)
#define LL_APB1_GRP1_PERIPH_TIM5           RCC_APB1ENR_TIM5EN
#endif /*TIM5*/
#define LL_APB1_GRP1_PERIPH_TIM6           RCC_APB1ENR_TIM6EN
#define LL_APB1_GRP1_PERIPH_TIM7           RCC_APB1ENR_TIM7EN
#if defined(LCD)
#define LL_APB1_GRP1_PERIPH_LCD            RCC_APB1ENR_LCDEN
#endif /*LCD*/
#define LL_APB1_GRP1_PERIPH_WWDG           RCC_APB1ENR_WWDGEN
#define LL_APB1_GRP1_PERIPH_SPI2           RCC_APB1ENR_SPI2EN
#if defined(SPI3)
#define LL_APB1_GRP1_PERIPH_SPI3           RCC_APB1ENR_SPI3EN
#endif /*SPI3*/
#define LL_APB1_GRP1_PERIPH_USART2         RCC_APB1ENR_USART2EN
#define LL_APB1_GRP1_PERIPH_USART3         RCC_APB1ENR_USART3EN
#if defined(UART4)
#define LL_APB1_GRP1_PERIPH_UART4          RCC_APB1ENR_UART4EN
#endif /*UART4*/
#if defined(UART5)
#define LL_APB1_GRP1_PERIPH_UART5          RCC_APB1ENR_UART5EN
#endif /*UART5*/
#define LL_APB1_GRP1_PERIPH_I2C1           RCC_APB1ENR_I2C1EN
#define LL_APB1_GRP1_PERIPH_I2C2           RCC_APB1ENR_I2C2EN
#define LL_APB1_GRP1_PERIPH_USB            RCC_APB1ENR_USBEN
#define LL_APB1_GRP1_PERIPH_PWR            RCC_APB1ENR_PWREN
#define LL_APB1_GRP1_PERIPH_DAC1           RCC_APB1ENR_DACEN
#define LL_APB1_GRP1_PERIPH_COMP           RCC_APB1ENR_COMPEN
#if defined(OPAMP)
/* Note: Peripherals COMP and OPAMP share the same clock domain */
#define LL_APB1_GRP1_PERIPH_OPAMP          LL_APB1_GRP1_PERIPH_COMP
#endif
/**
  * @}
  */

/** @defgroup BUS_LL_EC_APB2_GRP1_PERIPH  APB2 GRP1 PERIPH
  * @{
  */
#define LL_APB2_GRP1_PERIPH_ALL            0xFFFFFFFFU
#define LL_APB2_GRP1_PERIPH_SYSCFG         RCC_APB2ENR_SYSCFGEN
#define LL_APB2_GRP1_PERIPH_TIM9           RCC_APB2ENR_TIM9EN
#define LL_APB2_GRP1_PERIPH_TIM10          RCC_APB2ENR_TIM10EN
#define LL_APB2_GRP1_PERIPH_TIM11          RCC_APB2ENR_TIM11EN
#define LL_APB2_GRP1_PERIPH_ADC1           RCC_APB2ENR_ADC1EN
#if defined(SDIO)
#define LL_APB2_GRP1_PERIPH_SDIO           RCC_APB2ENR_SDIOEN
#endif /*SDIO*/
#define LL_APB2_GRP1_PERIPH_SPI1           RCC_APB2ENR_SPI1EN
#define LL_APB2_GRP1_PERIPH_USART1         RCC_APB2ENR_USART1EN
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @defgroup BUS_LL_Exported_Functions BUS Exported Functions
  * @{
  */

/** @defgroup BUS_LL_EF_AHB1 AHB1
  * @{
  */

/**
  * @brief  Enable AHB1 peripherals clock.
  * @rmtoll AHBENR       GPIOAEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIOBEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIOCEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIODEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIOEEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIOHEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIOFEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIOGEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       CRCEN         LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       FLITFEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       DMA1EN        LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       DMA2EN        LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       AESEN         LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       FSMCEN        LL_AHB1_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLASH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2 (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FSMC (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->AHBENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->AHBENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if AHB1 peripheral clock is enabled or not
  * @rmtoll AHBENR       GPIOAEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIOBEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIOCEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIODEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIOEEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIOHEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIOFEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIOGEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       CRCEN         LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       FLITFEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       DMA1EN        LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       DMA2EN        LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       AESEN         LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       FSMCEN        LL_AHB1_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLASH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2 (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FSMC (*)
  *
  *         (*) value not defined in all devices.
  * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_AHB1_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->AHBENR, Periphs) == Periphs);
}

/**
  * @brief  Disable AHB1 peripherals clock.
  * @rmtoll AHBENR       GPIOAEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIOBEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIOCEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIODEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIOEEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIOHEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIOFEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIOGEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       CRCEN         LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       FLITFEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       DMA1EN        LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       DMA2EN        LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       AESEN         LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       FSMCEN        LL_AHB1_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLASH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2 (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FSMC (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_DisableClock(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHBENR, Periphs);
}

/**
  * @brief  Force AHB1 peripherals reset.
  * @rmtoll AHBRSTR      GPIOARST      LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      GPIOBRST      LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      GPIOCRST      LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      GPIODRST      LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      GPIOERST      LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      GPIOHRST      LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      GPIOFRST      LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      GPIOGRST      LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      CRCRST        LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      FLITFRST      LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      DMA1RST       LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      DMA2RST       LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      AESRST        LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      FSMCRST       LL_AHB1_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLASH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2 (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FSMC (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_ForceReset(uint32_t Periphs)
{
  SET_BIT(RCC->AHBRSTR, Periphs);
}

/**
  * @brief  Release AHB1 peripherals reset.
  * @rmtoll AHBRSTR      GPIOARST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      GPIOBRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      GPIOCRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      GPIODRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      GPIOERST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      GPIOHRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      GPIOFRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      GPIOGRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      CRCRST        LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      FLITFRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      DMA1RST       LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      DMA2RST       LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      AESRST        LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      FSMCRST       LL_AHB1_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLASH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2 (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FSMC (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHBRSTR, Periphs);
}

/**
  * @brief  Enable AHB1 peripherals clock during Low Power (Sleep) mode.
  * @rmtoll AHBLPENR     GPIOALPEN     LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     GPIOBLPEN     LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     GPIOCLPEN     LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     GPIODLPEN     LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     GPIOELPEN     LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     GPIOHLPEN     LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     GPIOFLPEN     LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     GPIOGLPEN     LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     CRCLPEN       LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     FLITFLPEN     LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     SRAMLPEN      LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     DMA1LPEN      LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     DMA2LPEN      LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     AESLPEN       LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBLPENR     FSMCLPEN      LL_AHB1_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLASH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_SRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2 (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FSMC (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->AHBLPENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->AHBLPENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AHB1 peripherals clock during Low Power (Sleep) mode.
  * @rmtoll AHBLPENR     GPIOALPEN     LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     GPIOBLPEN     LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     GPIOCLPEN     LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     GPIODLPEN     LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     GPIOELPEN     LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     GPIOHLPEN     LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     GPIOFLPEN     LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     GPIOGLPEN     LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     CRCLPEN       LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     FLITFLPEN     LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     SRAMLPEN      LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     DMA1LPEN      LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     DMA2LPEN      LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     AESLPEN       LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBLPENR     FSMCLPEN      LL_AHB1_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLASH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_SRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2 (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FSMC (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_DisableClockSleep(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHBLPENR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_APB1 APB1
  * @{
  */

/**
  * @brief  Enable APB1 peripherals clock.
  * @rmtoll APB1ENR      TIM2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      TIM3EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      TIM4EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      TIM5EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      TIM6EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      TIM7EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      LCDEN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      WWDGEN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      SPI2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      SPI3EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      USART2EN      LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      USART3EN      LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      UART4EN       LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      UART5EN       LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      I2C1EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      I2C2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      USBEN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      PWREN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      DACEN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      COMPEN        LL_APB1_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  *         @arg @ref LL_APB1_GRP1_PERIPH_COMP
  *         @arg @ref LL_APB1_GRP1_PERIPH_OPAMP (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->APB1ENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB1ENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if APB1 peripheral clock is enabled or not
  * @rmtoll APB1ENR      TIM2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      TIM3EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      TIM4EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      TIM5EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      TIM6EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      TIM7EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      LCDEN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      WWDGEN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      SPI2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      SPI3EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      USART2EN      LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      USART3EN      LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      UART4EN       LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      UART5EN       LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      I2C1EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      I2C2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      USBEN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      PWREN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      DACEN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      COMPEN        LL_APB1_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  *         @arg @ref LL_APB1_GRP1_PERIPH_COMP
  *         @arg @ref LL_APB1_GRP1_PERIPH_OPAMP (*)
  *
  *         (*) value not defined in all devices.
  * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_APB1_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->APB1ENR, Periphs) == Periphs);
}

/**
  * @brief  Disable APB1 peripherals clock.
  * @rmtoll APB1ENR      TIM2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      TIM3EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      TIM4EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      TIM5EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      TIM6EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      TIM7EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      LCDEN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      WWDGEN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      SPI2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      SPI3EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      USART2EN      LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      USART3EN      LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      UART4EN       LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      UART5EN       LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      I2C1EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      I2C2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      USBEN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      PWREN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      DACEN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      COMPEN        LL_APB1_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  *         @arg @ref LL_APB1_GRP1_PERIPH_COMP
  *         @arg @ref LL_APB1_GRP1_PERIPH_OPAMP (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_DisableClock(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB1ENR, Periphs);
}

/**
  * @brief  Force APB1 peripherals reset.
  * @rmtoll APB1RSTR     TIM2RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM3RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM4RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM5RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM6RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM7RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     LCDRST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     WWDGRST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     SPI2RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     SPI3RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USART2RST     LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USART3RST     LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     UART4RST      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     UART5RST      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     I2C1RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     I2C2RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USBRST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     PWRRST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     DACRST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     COMPRST       LL_APB1_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  *         @arg @ref LL_APB1_GRP1_PERIPH_COMP
  *         @arg @ref LL_APB1_GRP1_PERIPH_OPAMP (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_ForceReset(uint32_t Periphs)
{
  SET_BIT(RCC->APB1RSTR, Periphs);
}

/**
  * @brief  Release APB1 peripherals reset.
  * @rmtoll APB1RSTR     TIM2RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM3RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM4RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM5RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM6RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM7RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     LCDRST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     WWDGRST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     SPI2RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     SPI3RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USART2RST     LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USART3RST     LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     UART4RST      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     UART5RST      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     I2C1RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     I2C2RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USBRST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     PWRRST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     DACRST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     COMPRST       LL_APB1_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  *         @arg @ref LL_APB1_GRP1_PERIPH_COMP
  *         @arg @ref LL_APB1_GRP1_PERIPH_OPAMP (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB1RSTR, Periphs);
}

/**
  * @brief  Enable APB1 peripherals clock during Low Power (Sleep) mode.
  * @rmtoll APB1LPENR    TIM2LPEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    TIM3LPEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    TIM4LPEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    TIM5LPEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    TIM6LPEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    TIM7LPEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    LCDLPEN       LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    WWDGLPEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    SPI2LPEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    SPI3LPEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    USART2LPEN    LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    USART3LPEN    LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    UART4LPEN     LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    UART5LPEN     LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    I2C1LPEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    I2C2LPEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    USBLPEN       LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    PWRLPEN       LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    DACLPEN       LL_APB1_GRP1_EnableClockSleep\n
  *         APB1LPENR    COMPLPEN      LL_APB1_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  *         @arg @ref LL_APB1_GRP1_PERIPH_COMP
  *         @arg @ref LL_APB1_GRP1_PERIPH_OPAMP (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->APB1LPENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB1LPENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB1 peripherals clock during Low Power (Sleep) mode.
  * @rmtoll APB1LPENR    TIM2LPEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    TIM3LPEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    TIM4LPEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    TIM5LPEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    TIM6LPEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    TIM7LPEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    LCDLPEN       LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    WWDGLPEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    SPI2LPEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    SPI3LPEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    USART2LPEN    LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    USART3LPEN    LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    UART4LPEN     LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    UART5LPEN     LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    I2C1LPEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    I2C2LPEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    USBLPEN       LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    PWRLPEN       LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    DACLPEN       LL_APB1_GRP1_DisableClockSleep\n
  *         APB1LPENR    COMPLPEN      LL_APB1_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  *         @arg @ref LL_APB1_GRP1_PERIPH_COMP
  *         @arg @ref LL_APB1_GRP1_PERIPH_OPAMP (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_DisableClockSleep(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB1LPENR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_APB2 APB2
  * @{
  */

/**
  * @brief  Enable APB2 peripherals clock.
  * @rmtoll APB2ENR      SYSCFGEN      LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      TIM9EN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      TIM10EN       LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      TIM11EN       LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      ADC1EN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      SDIOEN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      SPI1EN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      USART1EN      LL_APB2_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->APB2ENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB2ENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if APB2 peripheral clock is enabled or not
  * @rmtoll APB2ENR      SYSCFGEN      LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      TIM9EN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      TIM10EN       LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      TIM11EN       LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      ADC1EN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      SDIOEN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      SPI1EN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      USART1EN      LL_APB2_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *
  *         (*) value not defined in all devices.
  * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_APB2_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->APB2ENR, Periphs) == Periphs);
}

/**
  * @brief  Disable APB2 peripherals clock.
  * @rmtoll APB2ENR      SYSCFGEN      LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      TIM9EN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      TIM10EN       LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      TIM11EN       LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      ADC1EN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      SDIOEN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      SPI1EN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      USART1EN      LL_APB2_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_DisableClock(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB2ENR, Periphs);
}

/**
  * @brief  Force APB2 peripherals reset.
  * @rmtoll APB2RSTR     SYSCFGRST     LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     TIM9RST       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     TIM10RST      LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     TIM11RST      LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     ADC1RST       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     SDIORST       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     SPI1RST       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     USART1RST     LL_APB2_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_ForceReset(uint32_t Periphs)
{
  SET_BIT(RCC->APB2RSTR, Periphs);
}

/**
  * @brief  Release APB2 peripherals reset.
  * @rmtoll APB2RSTR     SYSCFGRST     LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     TIM9RST       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     TIM10RST      LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     TIM11RST      LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     ADC1RST       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     SDIORST       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     SPI1RST       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     USART1RST     LL_APB2_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB2RSTR, Periphs);
}

/**
  * @brief  Enable APB2 peripherals clock during Low Power (Sleep) mode.
  * @rmtoll APB2LPENR    SYSCFGLPEN    LL_APB2_GRP1_EnableClockSleep\n
  *         APB2LPENR    TIM9LPEN      LL_APB2_GRP1_EnableClockSleep\n
  *         APB2LPENR    TIM10LPEN     LL_APB2_GRP1_EnableClockSleep\n
  *         APB2LPENR    TIM11LPEN     LL_APB2_GRP1_EnableClockSleep\n
  *         APB2LPENR    ADC1LPEN      LL_APB2_GRP1_EnableClockSleep\n
  *         APB2LPENR    SDIOLPEN      LL_APB2_GRP1_EnableClockSleep\n
  *         APB2LPENR    SPI1LPEN      LL_APB2_GRP1_EnableClockSleep\n
  *         APB2LPENR    USART1LPEN    LL_APB2_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->APB2LPENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB2LPENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB2 peripherals clock during Low Power (Sleep) mode.
  * @rmtoll APB2LPENR    SYSCFGLPEN    LL_APB2_GRP1_DisableClockSleep\n
  *         APB2LPENR    TIM9LPEN      LL_APB2_GRP1_DisableClockSleep\n
  *         APB2LPENR    TIM10LPEN     LL_APB2_GRP1_DisableClockSleep\n
  *         APB2LPENR    TIM11LPEN     LL_APB2_GRP1_DisableClockSleep\n
  *         APB2LPENR    ADC1LPEN      LL_APB2_GRP1_DisableClockSleep\n
  *         APB2LPENR    SDIOLPEN      LL_APB2_GRP1_DisableClockSleep\n
  *         APB2LPENR    SPI1LPEN      LL_APB2_GRP1_DisableClockSleep\n
  *         APB2LPENR    USART1LPEN    LL_APB2_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_DisableClockSleep(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB2LPENR, Periphs);
}

/**
  * @}
  */


/**
  * @}
  */

/**
  * @}
  */

#endif /* defined(RCC) */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32L1xx_LL_BUS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
