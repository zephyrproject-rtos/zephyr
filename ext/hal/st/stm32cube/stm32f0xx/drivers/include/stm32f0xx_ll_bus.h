/**
  ******************************************************************************
  * @file    stm32f0xx_ll_bus.h
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
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
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
#ifndef __STM32F0xx_LL_BUS_H
#define __STM32F0xx_LL_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"

/** @addtogroup STM32F0xx_LL_Driver
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
#define LL_AHB1_GRP1_PERIPH_ALL            (uint32_t)0xFFFFFFFFU
#define LL_AHB1_GRP1_PERIPH_DMA1           RCC_AHBENR_DMA1EN
#if defined(DMA2)
#define LL_AHB1_GRP1_PERIPH_DMA2           RCC_AHBENR_DMA2EN
#endif /*DMA2*/
#define LL_AHB1_GRP1_PERIPH_SRAM           RCC_AHBENR_SRAMEN
#define LL_AHB1_GRP1_PERIPH_FLASH          RCC_AHBENR_FLITFEN
#define LL_AHB1_GRP1_PERIPH_CRC            RCC_AHBENR_CRCEN
#define LL_AHB1_GRP1_PERIPH_GPIOA          RCC_AHBENR_GPIOAEN
#define LL_AHB1_GRP1_PERIPH_GPIOB          RCC_AHBENR_GPIOBEN
#define LL_AHB1_GRP1_PERIPH_GPIOC          RCC_AHBENR_GPIOCEN
#if defined(GPIOD)
#define LL_AHB1_GRP1_PERIPH_GPIOD          RCC_AHBENR_GPIODEN
#endif /*GPIOD*/
#if defined(GPIOE)
#define LL_AHB1_GRP1_PERIPH_GPIOE          RCC_AHBENR_GPIOEEN
#endif /*GPIOE*/
#define LL_AHB1_GRP1_PERIPH_GPIOF          RCC_AHBENR_GPIOFEN
#if defined(TSC)
#define LL_AHB1_GRP1_PERIPH_TSC            RCC_AHBENR_TSCEN
#endif /*TSC*/
/**
  * @}
  */

/** @defgroup BUS_LL_EC_APB1_GRP1_PERIPH  APB1 GRP1 PERIPH
  * @{
  */
#define LL_APB1_GRP1_PERIPH_ALL            (uint32_t)0xFFFFFFFFU
#if defined(TIM2)
#define LL_APB1_GRP1_PERIPH_TIM2           RCC_APB1ENR_TIM2EN
#endif /*TIM2*/
#define LL_APB1_GRP1_PERIPH_TIM3           RCC_APB1ENR_TIM3EN
#if defined(TIM6)
#define LL_APB1_GRP1_PERIPH_TIM6           RCC_APB1ENR_TIM6EN
#endif /*TIM6*/
#if defined(TIM7)
#define LL_APB1_GRP1_PERIPH_TIM7           RCC_APB1ENR_TIM7EN
#endif /*TIM7*/
#define LL_APB1_GRP1_PERIPH_TIM14          RCC_APB1ENR_TIM14EN
#define LL_APB1_GRP1_PERIPH_WWDG           RCC_APB1ENR_WWDGEN
#if defined(SPI2)
#define LL_APB1_GRP1_PERIPH_SPI2           RCC_APB1ENR_SPI2EN
#endif /*SPI2*/
#if defined(USART2)
#define LL_APB1_GRP1_PERIPH_USART2         RCC_APB1ENR_USART2EN
#endif /* USART2 */
#if defined(USART3)
#define LL_APB1_GRP1_PERIPH_USART3         RCC_APB1ENR_USART3EN
#endif /* USART3 */
#if defined(USART4)
#define LL_APB1_GRP1_PERIPH_USART4         RCC_APB1ENR_USART4EN
#endif /* USART4 */
#if defined(USART5)
#define LL_APB1_GRP1_PERIPH_USART5         RCC_APB1ENR_USART5EN
#endif /* USART5 */
#define LL_APB1_GRP1_PERIPH_I2C1           RCC_APB1ENR_I2C1EN
#if defined(I2C2)
#define LL_APB1_GRP1_PERIPH_I2C2           RCC_APB1ENR_I2C2EN
#endif /*I2C2*/
#if defined(USB)
#define LL_APB1_GRP1_PERIPH_USB            RCC_APB1ENR_USBEN
#endif /* USB */
#if defined(CAN)
#define LL_APB1_GRP1_PERIPH_CAN            RCC_APB1ENR_CANEN
#endif /*CAN*/
#if defined(CRS)
#define LL_APB1_GRP1_PERIPH_CRS            RCC_APB1ENR_CRSEN
#endif /*CRS*/
#define LL_APB1_GRP1_PERIPH_PWR            RCC_APB1ENR_PWREN
#if defined(DAC)
#define LL_APB1_GRP1_PERIPH_DAC1           RCC_APB1ENR_DACEN
#endif /*DAC*/
#if defined(CEC)
#define LL_APB1_GRP1_PERIPH_CEC            RCC_APB1ENR_CECEN
#endif /*CEC*/
/**
  * @}
  */

/** @defgroup BUS_LL_EC_APB1_GRP2_PERIPH  APB1 GRP2 PERIPH
  * @{
  */
#define LL_APB1_GRP2_PERIPH_ALL            (uint32_t)0xFFFFFFFFU
#define LL_APB1_GRP2_PERIPH_SYSCFG         RCC_APB2ENR_SYSCFGEN
#define LL_APB1_GRP2_PERIPH_ADC1           RCC_APB2ENR_ADC1EN
#if defined(USART8)
#define LL_APB1_GRP2_PERIPH_USART8         RCC_APB2ENR_USART8EN
#endif /*USART8*/
#if defined(USART7)
#define LL_APB1_GRP2_PERIPH_USART7         RCC_APB2ENR_USART7EN
#endif /*USART7*/
#if defined(USART6)
#define LL_APB1_GRP2_PERIPH_USART6         RCC_APB2ENR_USART6EN
#endif /*USART6*/
#define LL_APB1_GRP2_PERIPH_TIM1           RCC_APB2ENR_TIM1EN
#define LL_APB1_GRP2_PERIPH_SPI1           RCC_APB2ENR_SPI1EN
#define LL_APB1_GRP2_PERIPH_USART1         RCC_APB2ENR_USART1EN
#if defined(TIM15)
#define LL_APB1_GRP2_PERIPH_TIM15          RCC_APB2ENR_TIM15EN
#endif /*TIM15*/
#define LL_APB1_GRP2_PERIPH_TIM16          RCC_APB2ENR_TIM16EN
#define LL_APB1_GRP2_PERIPH_TIM17          RCC_APB2ENR_TIM17EN
#define LL_APB1_GRP2_PERIPH_DBGMCU         RCC_APB2ENR_DBGMCUEN
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
  * @rmtoll AHBENR       DMA1EN        LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       DMA2EN        LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       SRAMEN        LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       FLITFEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       CRCEN         LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIOAEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIOBEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIOCEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIODEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIOEEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       GPIOFEN       LL_AHB1_GRP1_EnableClock\n
  *         AHBENR       TSCEN         LL_AHB1_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2 (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_SRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLASH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
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
  * @rmtoll AHBENR       DMA1EN        LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       DMA2EN        LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       SRAMEN        LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       FLITFEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       CRCEN         LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIOAEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIOBEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIOCEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIODEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIOEEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       GPIOFEN       LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR       TSCEN         LL_AHB1_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2 (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_SRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLASH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
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
  * @rmtoll AHBENR       DMA1EN        LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       DMA2EN        LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       SRAMEN        LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       FLITFEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       CRCEN         LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIOAEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIOBEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIOCEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIODEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIOEEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       GPIOFEN       LL_AHB1_GRP1_DisableClock\n
  *         AHBENR       TSCEN         LL_AHB1_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2 (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_SRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLASH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
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
  *         AHBRSTR      GPIOFRST      LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      TSCRST        LL_AHB1_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
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
  *         AHBRSTR      GPIOFRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      TSCRST        LL_AHB1_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHBRSTR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_APB1_GRP1 APB1 GRP1
  * @{
  */

/**
  * @brief  Enable APB1 peripherals clock (available in register 1).
  * @rmtoll APB1ENR      TIM2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      TIM3EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      TIM6EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      TIM7EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      TIM14EN       LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      WWDGEN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      SPI2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      USART2EN      LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      USART3EN      LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      USART4EN      LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      USART5EN      LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      I2C1EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      I2C2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      USBEN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      CANEN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      CRSEN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      PWREN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      DACEN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR      CECEN         LL_APB1_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC (*)
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
  * @brief  Check if APB1 peripheral clock is enabled or not (available in register 1).
  * @rmtoll APB1ENR      TIM2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      TIM3EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      TIM6EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      TIM7EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      TIM14EN       LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      WWDGEN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      SPI2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      USART2EN      LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      USART3EN      LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      USART4EN      LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      USART5EN      LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      I2C1EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      I2C2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      USBEN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      CANEN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      CRSEN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      PWREN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      DACEN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR      CECEN         LL_APB1_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC (*)
  *
  *         (*) value not defined in all devices.
  * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_APB1_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->APB1ENR, Periphs) == Periphs);
}

/**
  * @brief  Disable APB1 peripherals clock (available in register 1).
  * @rmtoll APB1ENR      TIM2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      TIM3EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      TIM6EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      TIM7EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      TIM14EN       LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      WWDGEN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      SPI2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      USART2EN      LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      USART3EN      LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      USART4EN      LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      USART5EN      LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      I2C1EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      I2C2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      USBEN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      CANEN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      CRSEN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      PWREN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      DACEN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR      CECEN         LL_APB1_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_DisableClock(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB1ENR, Periphs);
}

/**
  * @brief  Force APB1 peripherals reset (available in register 1).
  * @rmtoll APB1RSTR     TIM2RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM3RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM6RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM7RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM14RST      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     WWDGRST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     SPI2RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USART2RST     LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USART3RST     LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USART4RST     LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USART5RST     LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     I2C1RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     I2C2RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USBRST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     CANRST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     CRSRST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     PWRRST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     DACRST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     CECRST        LL_APB1_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_ForceReset(uint32_t Periphs)
{
  SET_BIT(RCC->APB1RSTR, Periphs);
}

/**
  * @brief  Release APB1 peripherals reset (available in register 1).
  * @rmtoll APB1RSTR     TIM2RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM3RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM6RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM7RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM14RST      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     WWDGRST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     SPI2RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USART2RST     LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USART3RST     LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USART4RST     LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USART5RST     LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     I2C1RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     I2C2RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USBRST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     CANRST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     CRSRST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     PWRRST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     DACRST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     CECRST        LL_APB1_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB1RSTR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_APB1_GRP2 APB1 GRP2
  * @{
  */

/**
  * @brief  Enable APB1 peripherals clock (available in register 2).
  * @rmtoll APB2ENR      SYSCFGEN      LL_APB1_GRP2_EnableClock\n
  *         APB2ENR      ADC1EN        LL_APB1_GRP2_EnableClock\n
  *         APB2ENR      USART8EN      LL_APB1_GRP2_EnableClock\n
  *         APB2ENR      USART7EN      LL_APB1_GRP2_EnableClock\n
  *         APB2ENR      USART6EN      LL_APB1_GRP2_EnableClock\n
  *         APB2ENR      TIM1EN        LL_APB1_GRP2_EnableClock\n
  *         APB2ENR      SPI1EN        LL_APB1_GRP2_EnableClock\n
  *         APB2ENR      USART1EN      LL_APB1_GRP2_EnableClock\n
  *         APB2ENR      TIM15EN       LL_APB1_GRP2_EnableClock\n
  *         APB2ENR      TIM16EN       LL_APB1_GRP2_EnableClock\n
  *         APB2ENR      TIM17EN       LL_APB1_GRP2_EnableClock\n
  *         APB2ENR      DBGMCUEN      LL_APB1_GRP2_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP2_PERIPH_SYSCFG
  *         @arg @ref LL_APB1_GRP2_PERIPH_ADC1
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART8 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART7 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART6 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM1
  *         @arg @ref LL_APB1_GRP2_PERIPH_SPI1
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART1
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM15 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM16
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM17
  *         @arg @ref LL_APB1_GRP2_PERIPH_DBGMCU
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP2_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->APB2ENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB2ENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if APB1 peripheral clock is enabled or not (available in register 2).
  * @rmtoll APB2ENR      SYSCFGEN      LL_APB1_GRP2_IsEnabledClock\n
  *         APB2ENR      ADC1EN        LL_APB1_GRP2_IsEnabledClock\n
  *         APB2ENR      USART8EN      LL_APB1_GRP2_IsEnabledClock\n
  *         APB2ENR      USART7EN      LL_APB1_GRP2_IsEnabledClock\n
  *         APB2ENR      USART6EN      LL_APB1_GRP2_IsEnabledClock\n
  *         APB2ENR      TIM1EN        LL_APB1_GRP2_IsEnabledClock\n
  *         APB2ENR      SPI1EN        LL_APB1_GRP2_IsEnabledClock\n
  *         APB2ENR      USART1EN      LL_APB1_GRP2_IsEnabledClock\n
  *         APB2ENR      TIM15EN       LL_APB1_GRP2_IsEnabledClock\n
  *         APB2ENR      TIM16EN       LL_APB1_GRP2_IsEnabledClock\n
  *         APB2ENR      TIM17EN       LL_APB1_GRP2_IsEnabledClock\n
  *         APB2ENR      DBGMCUEN      LL_APB1_GRP2_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP2_PERIPH_SYSCFG
  *         @arg @ref LL_APB1_GRP2_PERIPH_ADC1
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART8 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART7 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART6 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM1
  *         @arg @ref LL_APB1_GRP2_PERIPH_SPI1
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART1
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM15 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM16
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM17
  *         @arg @ref LL_APB1_GRP2_PERIPH_DBGMCU
  *
  *         (*) value not defined in all devices.
  * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_APB1_GRP2_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->APB2ENR, Periphs) == Periphs);
}

/**
  * @brief  Disable APB1 peripherals clock (available in register 2).
  * @rmtoll APB2ENR      SYSCFGEN      LL_APB1_GRP2_DisableClock\n
  *         APB2ENR      ADC1EN        LL_APB1_GRP2_DisableClock\n
  *         APB2ENR      USART8EN      LL_APB1_GRP2_DisableClock\n
  *         APB2ENR      USART7EN      LL_APB1_GRP2_DisableClock\n
  *         APB2ENR      USART6EN      LL_APB1_GRP2_DisableClock\n
  *         APB2ENR      TIM1EN        LL_APB1_GRP2_DisableClock\n
  *         APB2ENR      SPI1EN        LL_APB1_GRP2_DisableClock\n
  *         APB2ENR      USART1EN      LL_APB1_GRP2_DisableClock\n
  *         APB2ENR      TIM15EN       LL_APB1_GRP2_DisableClock\n
  *         APB2ENR      TIM16EN       LL_APB1_GRP2_DisableClock\n
  *         APB2ENR      TIM17EN       LL_APB1_GRP2_DisableClock\n
  *         APB2ENR      DBGMCUEN      LL_APB1_GRP2_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP2_PERIPH_SYSCFG
  *         @arg @ref LL_APB1_GRP2_PERIPH_ADC1
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART8 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART7 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART6 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM1
  *         @arg @ref LL_APB1_GRP2_PERIPH_SPI1
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART1
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM15 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM16
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM17
  *         @arg @ref LL_APB1_GRP2_PERIPH_DBGMCU
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP2_DisableClock(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB2ENR, Periphs);
}

/**
  * @brief  Force APB1 peripherals reset (available in register 2).
  * @rmtoll APB2RSTR     SYSCFGRST     LL_APB1_GRP2_ForceReset\n
  *         APB2RSTR     ADC1RST       LL_APB1_GRP2_ForceReset\n
  *         APB2RSTR     USART8RST     LL_APB1_GRP2_ForceReset\n
  *         APB2RSTR     USART7RST     LL_APB1_GRP2_ForceReset\n
  *         APB2RSTR     USART6RST     LL_APB1_GRP2_ForceReset\n
  *         APB2RSTR     TIM1RST       LL_APB1_GRP2_ForceReset\n
  *         APB2RSTR     SPI1RST       LL_APB1_GRP2_ForceReset\n
  *         APB2RSTR     USART1RST     LL_APB1_GRP2_ForceReset\n
  *         APB2RSTR     TIM15RST      LL_APB1_GRP2_ForceReset\n
  *         APB2RSTR     TIM16RST      LL_APB1_GRP2_ForceReset\n
  *         APB2RSTR     TIM17RST      LL_APB1_GRP2_ForceReset\n
  *         APB2RSTR     DBGMCURST     LL_APB1_GRP2_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP2_PERIPH_ALL
  *         @arg @ref LL_APB1_GRP2_PERIPH_SYSCFG
  *         @arg @ref LL_APB1_GRP2_PERIPH_ADC1
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART8 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART7 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART6 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM1
  *         @arg @ref LL_APB1_GRP2_PERIPH_SPI1
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART1
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM15 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM16
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM17
  *         @arg @ref LL_APB1_GRP2_PERIPH_DBGMCU
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP2_ForceReset(uint32_t Periphs)
{
  SET_BIT(RCC->APB2RSTR, Periphs);
}

/**
  * @brief  Release APB1 peripherals reset (available in register 2).
  * @rmtoll APB2RSTR     SYSCFGRST     LL_APB1_GRP2_ReleaseReset\n
  *         APB2RSTR     ADC1RST       LL_APB1_GRP2_ReleaseReset\n
  *         APB2RSTR     USART8RST     LL_APB1_GRP2_ReleaseReset\n
  *         APB2RSTR     USART7RST     LL_APB1_GRP2_ReleaseReset\n
  *         APB2RSTR     USART6RST     LL_APB1_GRP2_ReleaseReset\n
  *         APB2RSTR     TIM1RST       LL_APB1_GRP2_ReleaseReset\n
  *         APB2RSTR     SPI1RST       LL_APB1_GRP2_ReleaseReset\n
  *         APB2RSTR     USART1RST     LL_APB1_GRP2_ReleaseReset\n
  *         APB2RSTR     TIM15RST      LL_APB1_GRP2_ReleaseReset\n
  *         APB2RSTR     TIM16RST      LL_APB1_GRP2_ReleaseReset\n
  *         APB2RSTR     TIM17RST      LL_APB1_GRP2_ReleaseReset\n
  *         APB2RSTR     DBGMCURST     LL_APB1_GRP2_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP2_PERIPH_ALL
  *         @arg @ref LL_APB1_GRP2_PERIPH_SYSCFG
  *         @arg @ref LL_APB1_GRP2_PERIPH_ADC1
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART8 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART7 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART6 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM1
  *         @arg @ref LL_APB1_GRP2_PERIPH_SPI1
  *         @arg @ref LL_APB1_GRP2_PERIPH_USART1
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM15 (*)
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM16
  *         @arg @ref LL_APB1_GRP2_PERIPH_TIM17
  *         @arg @ref LL_APB1_GRP2_PERIPH_DBGMCU
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP2_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB2RSTR, Periphs);
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

#endif /* __STM32F0xx_LL_BUS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
