/**
  ******************************************************************************
  * @file    stm32f2xx_ll_bus.h
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
#ifndef __STM32F2xx_LL_BUS_H
#define __STM32F2xx_LL_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx.h"

/** @addtogroup STM32F2xx_LL_Driver
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
#define LL_AHB1_GRP1_PERIPH_GPIOA          RCC_AHB1ENR_GPIOAEN
#define LL_AHB1_GRP1_PERIPH_GPIOB          RCC_AHB1ENR_GPIOBEN
#define LL_AHB1_GRP1_PERIPH_GPIOC          RCC_AHB1ENR_GPIOCEN
#define LL_AHB1_GRP1_PERIPH_GPIOD          RCC_AHB1ENR_GPIODEN
#define LL_AHB1_GRP1_PERIPH_GPIOE          RCC_AHB1ENR_GPIOEEN
#define LL_AHB1_GRP1_PERIPH_GPIOF          RCC_AHB1ENR_GPIOFEN
#define LL_AHB1_GRP1_PERIPH_GPIOG          RCC_AHB1ENR_GPIOGEN
#define LL_AHB1_GRP1_PERIPH_GPIOH          RCC_AHB1ENR_GPIOHEN
#define LL_AHB1_GRP1_PERIPH_GPIOI          RCC_AHB1ENR_GPIOIEN
#define LL_AHB1_GRP1_PERIPH_CRC            RCC_AHB1ENR_CRCEN
#define LL_AHB1_GRP1_PERIPH_BKPSRAM        RCC_AHB1ENR_BKPSRAMEN
#define LL_AHB1_GRP1_PERIPH_DMA1           RCC_AHB1ENR_DMA1EN
#define LL_AHB1_GRP1_PERIPH_DMA2           RCC_AHB1ENR_DMA2EN
#if defined(ETH)
#define LL_AHB1_GRP1_PERIPH_ETHMAC         RCC_AHB1ENR_ETHMACEN
#define LL_AHB1_GRP1_PERIPH_ETHMACTX       RCC_AHB1ENR_ETHMACTXEN
#define LL_AHB1_GRP1_PERIPH_ETHMACRX       RCC_AHB1ENR_ETHMACRXEN
#define LL_AHB1_GRP1_PERIPH_ETHMACPTP      RCC_AHB1ENR_ETHMACPTPEN
#endif /* ETH */
#define LL_AHB1_GRP1_PERIPH_OTGHS          RCC_AHB1ENR_OTGHSEN
#define LL_AHB1_GRP1_PERIPH_OTGHSULPI      RCC_AHB1ENR_OTGHSULPIEN
#define LL_AHB1_GRP1_PERIPH_FLITF          RCC_AHB1LPENR_FLITFLPEN
#define LL_AHB1_GRP1_PERIPH_SRAM1          RCC_AHB1LPENR_SRAM1LPEN
#define LL_AHB1_GRP1_PERIPH_SRAM2          RCC_AHB1LPENR_SRAM2LPEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_AHB2_GRP1_PERIPH  AHB2 GRP1 PERIPH
  * @{
  */
#define LL_AHB2_GRP1_PERIPH_ALL            0xFFFFFFFFU
#if defined(DCMI)
#define LL_AHB2_GRP1_PERIPH_DCMI           RCC_AHB2ENR_DCMIEN
#endif /* DCMI */
#if defined(CRYP)
#define LL_AHB2_GRP1_PERIPH_CRYP           RCC_AHB2ENR_CRYPEN
#endif /* CRYP */
#if defined(HASH)
#define LL_AHB2_GRP1_PERIPH_HASH           RCC_AHB2ENR_HASHEN
#endif /* HASH */
#define LL_AHB2_GRP1_PERIPH_RNG            RCC_AHB2ENR_RNGEN
#define LL_AHB2_GRP1_PERIPH_OTGFS          RCC_AHB2ENR_OTGFSEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_AHB3_GRP1_PERIPH  AHB3 GRP1 PERIPH
  * @{
  */
#define LL_AHB3_GRP1_PERIPH_ALL            0xFFFFFFFFU
#define LL_AHB3_GRP1_PERIPH_FSMC           RCC_AHB3ENR_FSMCEN
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
#define LL_APB1_GRP1_PERIPH_TIM5           RCC_APB1ENR_TIM5EN
#define LL_APB1_GRP1_PERIPH_TIM6           RCC_APB1ENR_TIM6EN
#define LL_APB1_GRP1_PERIPH_TIM7           RCC_APB1ENR_TIM7EN
#define LL_APB1_GRP1_PERIPH_TIM12          RCC_APB1ENR_TIM12EN
#define LL_APB1_GRP1_PERIPH_TIM13          RCC_APB1ENR_TIM13EN
#define LL_APB1_GRP1_PERIPH_TIM14          RCC_APB1ENR_TIM14EN
#define LL_APB1_GRP1_PERIPH_WWDG           RCC_APB1ENR_WWDGEN
#define LL_APB1_GRP1_PERIPH_SPI2           RCC_APB1ENR_SPI2EN
#define LL_APB1_GRP1_PERIPH_SPI3           RCC_APB1ENR_SPI3EN
#define LL_APB1_GRP1_PERIPH_USART2         RCC_APB1ENR_USART2EN
#define LL_APB1_GRP1_PERIPH_USART3         RCC_APB1ENR_USART3EN
#define LL_APB1_GRP1_PERIPH_UART4          RCC_APB1ENR_UART4EN
#define LL_APB1_GRP1_PERIPH_UART5          RCC_APB1ENR_UART5EN
#define LL_APB1_GRP1_PERIPH_I2C1           RCC_APB1ENR_I2C1EN
#define LL_APB1_GRP1_PERIPH_I2C2           RCC_APB1ENR_I2C2EN
#define LL_APB1_GRP1_PERIPH_I2C3           RCC_APB1ENR_I2C3EN
#define LL_APB1_GRP1_PERIPH_CAN1           RCC_APB1ENR_CAN1EN
#define LL_APB1_GRP1_PERIPH_CAN2           RCC_APB1ENR_CAN2EN
#define LL_APB1_GRP1_PERIPH_PWR            RCC_APB1ENR_PWREN
#define LL_APB1_GRP1_PERIPH_DAC1           RCC_APB1ENR_DACEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_APB2_GRP1_PERIPH  APB2 GRP1 PERIPH
  * @{
  */
#define LL_APB2_GRP1_PERIPH_ALL            0xFFFFFFFFU
#define LL_APB2_GRP1_PERIPH_TIM1           RCC_APB2ENR_TIM1EN
#define LL_APB2_GRP1_PERIPH_TIM8           RCC_APB2ENR_TIM8EN
#define LL_APB2_GRP1_PERIPH_USART1         RCC_APB2ENR_USART1EN
#define LL_APB2_GRP1_PERIPH_USART6         RCC_APB2ENR_USART6EN
#define LL_APB2_GRP1_PERIPH_ADC1           RCC_APB2ENR_ADC1EN
#define LL_APB2_GRP1_PERIPH_ADC2           RCC_APB2ENR_ADC2EN
#define LL_APB2_GRP1_PERIPH_ADC3           RCC_APB2ENR_ADC3EN
#define LL_APB2_GRP1_PERIPH_SDIO           RCC_APB2ENR_SDIOEN
#define LL_APB2_GRP1_PERIPH_SPI1           RCC_APB2ENR_SPI1EN
#define LL_APB2_GRP1_PERIPH_SYSCFG         RCC_APB2ENR_SYSCFGEN
#define LL_APB2_GRP1_PERIPH_TIM9           RCC_APB2ENR_TIM9EN
#define LL_APB2_GRP1_PERIPH_TIM10          RCC_APB2ENR_TIM10EN
#define LL_APB2_GRP1_PERIPH_TIM11          RCC_APB2ENR_TIM11EN
#define LL_APB2_GRP1_PERIPH_ADC            RCC_APB2RSTR_ADCRST
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
  * @rmtoll AHB1ENR      GPIOAEN            LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      GPIOBEN            LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      GPIOCEN            LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      GPIODEN            LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      GPIOEEN            LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      GPIOFEN            LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      GPIOGEN            LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      GPIOHEN            LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      GPIOIEN            LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      CRCEN              LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      BKPSRAMEN          LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      DMA1EN             LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      DMA2EN             LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      ETHMACEN           LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      ETHMACTXEN         LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      ETHMACRXEN         LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      ETHMACPTPEN        LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      OTGHSEN            LL_AHB1_GRP1_EnableClock\n
  *         AHB1ENR      OTGHSULPIEN        LL_AHB1_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_BKPSRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMAC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACTX (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACRX (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACPTP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHS
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHSULPI
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->AHB1ENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->AHB1ENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if AHB1 peripheral clock is enabled or not
  * @rmtoll AHB1ENR      GPIOAEN            LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      GPIOBEN            LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      GPIOCEN            LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      GPIODEN            LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      GPIOEEN            LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      GPIOFEN            LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      GPIOGEN            LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      GPIOHEN            LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      GPIOIEN            LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      CRCEN              LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      BKPSRAMEN          LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      DMA1EN             LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      DMA2EN             LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      ETHMACEN           LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      ETHMACTXEN         LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      ETHMACRXEN         LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      ETHMACPTPEN        LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      OTGHSEN            LL_AHB1_GRP1_IsEnabledClock\n
  *         AHB1ENR      OTGHSULPIEN        LL_AHB1_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_BKPSRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMAC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACTX (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACRX (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACPTP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHS
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHSULPI
  *
  *         (*) value not defined in all devices.
  * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_AHB1_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->AHB1ENR, Periphs) == Periphs);
}

/**
  * @brief  Disable AHB1 peripherals clock.
  * @rmtoll AHB1ENR      GPIOAEN            LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      GPIOBEN            LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      GPIOCEN            LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      GPIODEN            LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      GPIOEEN            LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      GPIOFEN            LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      GPIOGEN            LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      GPIOHEN            LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      GPIOIEN            LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      CRCEN              LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      BKPSRAMEN          LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      DMA1EN             LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      DMA2EN             LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      ETHMACEN           LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      ETHMACTXEN         LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      ETHMACRXEN         LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      ETHMACPTPEN        LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      OTGHSEN            LL_AHB1_GRP1_DisableClock\n
  *         AHB1ENR      OTGHSULPIEN        LL_AHB1_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_BKPSRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMAC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACTX (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACRX (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACPTP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHS
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHSULPI
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_DisableClock(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHB1ENR, Periphs);
}

/**
  * @brief  Force AHB1 peripherals reset.
  * @rmtoll AHB1RSTR     GPIOARST      LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     GPIOBRST      LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     GPIOCRST      LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     GPIODRST      LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     GPIOERST      LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     GPIOFRST      LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     GPIOGRST      LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     GPIOHRST      LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     GPIOIRST      LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     CRCRST        LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     DMA1RST       LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     DMA2RST       LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     ETHMACRST     LL_AHB1_GRP1_ForceReset\n
  *         AHB1RSTR     OTGHSRST      LL_AHB1_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMAC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHS
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_ForceReset(uint32_t Periphs)
{
  SET_BIT(RCC->AHB1RSTR, Periphs);
}

/**
  * @brief  Release AHB1 peripherals reset.
  * @rmtoll AHB1RSTR     GPIOARST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     GPIOBRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     GPIOCRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     GPIODRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     GPIOERST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     GPIOFRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     GPIOGRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     GPIOHRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     GPIOIRST      LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     CRCRST        LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     DMA1RST       LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     DMA2RST       LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     ETHMACRST     LL_AHB1_GRP1_ReleaseReset\n
  *         AHB1RSTR     OTGHSRST      LL_AHB1_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMAC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHS
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHB1RSTR, Periphs);
}

/**
  * @brief  Enable AHB1 peripheral clocks in low-power mode
  * @rmtoll AHB1LPENR    GPIOALPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    GPIOBLPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    GPIOCLPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    GPIODLPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    GPIOELPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    GPIOFLPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    GPIOGLPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    GPIOHLPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    GPIOILPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    CRCLPEN        LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    BKPSRAMLPEN    LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    FLITFLPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    SRAM1LPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    SRAM2LPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    BKPSRAMLPEN    LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    DMA1LPEN       LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    DMA2LPEN       LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    ETHMACLPEN     LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    ETHMACTXLPEN   LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    ETHMACRXLPEN   LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    ETHMACPTPLPEN  LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    OTGHSLPEN      LL_AHB1_GRP1_EnableClockLowPower\n
  *         AHB1LPENR    OTGHSULPILPEN  LL_AHB1_GRP1_EnableClockLowPower
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_BKPSRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLITF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_SRAM1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_SRAM2
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMAC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACTX (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACRX (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACPTP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHS
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHSULPI
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_EnableClockLowPower(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->AHB1LPENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->AHB1LPENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AHB1 peripheral clocks in low-power mode
  * @rmtoll AHB1LPENR    GPIOALPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    GPIOBLPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    GPIOCLPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    GPIODLPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    GPIOELPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    GPIOFLPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    GPIOGLPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    GPIOHLPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    GPIOILPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    CRCLPEN        LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    BKPSRAMLPEN    LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    FLITFLPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    SRAM1LPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    SRAM2LPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    BKPSRAMLPEN    LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    DMA1LPEN       LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    DMA2LPEN       LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    ETHMACLPEN     LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    ETHMACTXLPEN   LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    ETHMACRXLPEN   LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    ETHMACPTPLPEN  LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    OTGHSLPEN      LL_AHB1_GRP1_DisableClockLowPower\n
  *         AHB1LPENR    OTGHSULPILPEN  LL_AHB1_GRP1_DisableClockLowPower
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB1_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_BKPSRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_FLITF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_SRAM1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_SRAM2
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMAC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACTX (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACRX (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ETHMACPTP (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHS
  *         @arg @ref LL_AHB1_GRP1_PERIPH_OTGHSULPI
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_DisableClockLowPower(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHB1LPENR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_AHB2 AHB2
  * @{
  */

/**
  * @brief  Enable AHB2 peripherals clock.
  * @rmtoll AHB2ENR      DCMIEN       LL_AHB2_GRP1_EnableClock\n
  *         AHB2ENR      CRYPEN       LL_AHB2_GRP1_EnableClock\n
  *         AHB2ENR      HASHEN       LL_AHB2_GRP1_EnableClock\n
  *         AHB2ENR      RNGEN        LL_AHB2_GRP1_EnableClock\n
  *         AHB2ENR      OTGFSEN      LL_AHB2_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DCMI (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_HASH (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_RNG
  *         @arg @ref LL_AHB2_GRP1_PERIPH_OTGFS
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB2_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->AHB2ENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->AHB2ENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if AHB2 peripheral clock is enabled or not
  * @rmtoll AHB2ENR      DCMIEN       LL_AHB2_GRP1_IsEnabledClock\n
  *         AHB2ENR      CRYPEN       LL_AHB2_GRP1_IsEnabledClock\n
  *         AHB2ENR      HASHEN       LL_AHB2_GRP1_IsEnabledClock\n
  *         AHB2ENR      RNGEN        LL_AHB2_GRP1_IsEnabledClock\n
  *         AHB2ENR      OTGFSEN      LL_AHB2_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DCMI (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_HASH (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_RNG
  *         @arg @ref LL_AHB2_GRP1_PERIPH_OTGFS
  *
  *         (*) value not defined in all devices.
  * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_AHB2_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->AHB2ENR, Periphs) == Periphs);
}

/**
  * @brief  Disable AHB2 peripherals clock.
  * @rmtoll AHB2ENR      DCMIEN       LL_AHB2_GRP1_DisableClock\n
  *         AHB2ENR      CRYPEN       LL_AHB2_GRP1_DisableClock\n
  *         AHB2ENR      HASHEN       LL_AHB2_GRP1_DisableClock\n
  *         AHB2ENR      RNGEN        LL_AHB2_GRP1_DisableClock\n
  *         AHB2ENR      OTGFSEN      LL_AHB2_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DCMI (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_HASH (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_RNG
  *         @arg @ref LL_AHB2_GRP1_PERIPH_OTGFS
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB2_GRP1_DisableClock(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHB2ENR, Periphs);
}

/**
  * @brief  Force AHB2 peripherals reset.
  * @rmtoll AHB2RSTR     DCMIRST      LL_AHB2_GRP1_ForceReset\n
  *         AHB2RSTR     CRYPRST      LL_AHB2_GRP1_ForceReset\n
  *         AHB2RSTR     HASHRST      LL_AHB2_GRP1_ForceReset\n
  *         AHB2RSTR     RNGRST       LL_AHB2_GRP1_ForceReset\n
  *         AHB2RSTR     OTGFSRST     LL_AHB2_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DCMI (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_HASH (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_RNG
  *         @arg @ref LL_AHB2_GRP1_PERIPH_OTGFS
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB2_GRP1_ForceReset(uint32_t Periphs)
{
  SET_BIT(RCC->AHB2RSTR, Periphs);
}

/**
  * @brief  Release AHB2 peripherals reset.
  * @rmtoll AHB2RSTR     DCMIRST      LL_AHB2_GRP1_ReleaseReset\n
  *         AHB2RSTR     CRYPRST      LL_AHB2_GRP1_ReleaseReset\n
  *         AHB2RSTR     HASHRST      LL_AHB2_GRP1_ReleaseReset\n
  *         AHB2RSTR     RNGRST       LL_AHB2_GRP1_ReleaseReset\n
  *         AHB2RSTR     OTGFSRST     LL_AHB2_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DCMI (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_HASH (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_RNG
  *         @arg @ref LL_AHB2_GRP1_PERIPH_OTGFS
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB2_GRP1_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHB2RSTR, Periphs);
}

/**
  * @brief  Enable AHB2 peripheral clocks in low-power mode
  * @rmtoll AHB2LPENR    DCMILPEN     LL_AHB2_GRP1_EnableClockLowPower\n
  *         AHB2LPENR    CRYPLPEN     LL_AHB2_GRP1_EnableClockLowPower\n
  *         AHB2LPENR    HASHLPEN     LL_AHB2_GRP1_EnableClockLowPower\n
  *         AHB2LPENR    RNGLPEN      LL_AHB2_GRP1_EnableClockLowPower\n
  *         AHB2LPENR    OTGFSLPEN    LL_AHB2_GRP1_EnableClockLowPower
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DCMI (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_HASH (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_RNG
  *         @arg @ref LL_AHB2_GRP1_PERIPH_OTGFS
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB2_GRP1_EnableClockLowPower(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->AHB2LPENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->AHB2LPENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AHB2 peripheral clocks in low-power mode
  * @rmtoll AHB2LPENR    DCMILPEN     LL_AHB2_GRP1_DisableClockLowPower\n
  *         AHB2LPENR    CRYPLPEN     LL_AHB2_GRP1_DisableClockLowPower\n
  *         AHB2LPENR    HASHLPEN     LL_AHB2_GRP1_DisableClockLowPower\n
  *         AHB2LPENR    RNGLPEN      LL_AHB2_GRP1_DisableClockLowPower\n
  *         AHB2LPENR    OTGFSLPEN    LL_AHB2_GRP1_DisableClockLowPower
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DCMI (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_CRYP (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_HASH (*)
  *         @arg @ref LL_AHB2_GRP1_PERIPH_RNG
  *         @arg @ref LL_AHB2_GRP1_PERIPH_OTGFS
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB2_GRP1_DisableClockLowPower(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHB2LPENR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_AHB3 AHB3
  * @{
  */

/**
  * @brief  Enable AHB3 peripherals clock.
  * @rmtoll AHB3ENR      FSMCEN         LL_AHB3_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_FSMC
  * @retval None
*/
__STATIC_INLINE void LL_AHB3_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->AHB3ENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->AHB3ENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if AHB3 peripheral clock is enabled or not
  * @rmtoll AHB3ENR      FSMCEN        LL_AHB3_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_FSMC
  * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_AHB3_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->AHB3ENR, Periphs) == Periphs);
}

/**
  * @brief  Disable AHB3 peripherals clock.
  * @rmtoll AHB3ENR      FSMCEN        LL_AHB3_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_FSMC
  * @retval None
*/
__STATIC_INLINE void LL_AHB3_GRP1_DisableClock(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHB3ENR, Periphs);
}

/**
  * @brief  Force AHB3 peripherals reset.
  * @rmtoll AHB3RSTR     FSMCRST       LL_AHB3_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB3_GRP1_PERIPH_FSMC
  * @retval None
*/
__STATIC_INLINE void LL_AHB3_GRP1_ForceReset(uint32_t Periphs)
{
  SET_BIT(RCC->AHB3RSTR, Periphs);
}

/**
  * @brief  Release AHB3 peripherals reset.
  * @rmtoll AHB3RSTR     FSMCRST       LL_AHB3_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB3_GRP1_PERIPH_FSMC
  * @retval None
*/
__STATIC_INLINE void LL_AHB3_GRP1_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHB3RSTR, Periphs);
}

/**
  * @brief  Enable AHB3 peripheral clocks in low-power mode
  * @rmtoll AHB3LPENR    FSMCLPEN      LL_AHB3_GRP1_EnableClockLowPower
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_FSMC
  * @retval None
*/
__STATIC_INLINE void LL_AHB3_GRP1_EnableClockLowPower(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->AHB3LPENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->AHB3LPENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AHB3 peripheral clocks in low-power mode
  * @rmtoll AHB3LPENR    FSMCLPEN      LL_AHB3_GRP1_DisableClockLowPower
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_FSMC
  * @retval None
*/
__STATIC_INLINE void LL_AHB3_GRP1_DisableClockLowPower(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHB3LPENR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_APB1 APB1
  * @{
  */

/**
  * @brief  Enable APB1 peripherals clock.
  * @rmtoll APB1ENR     TIM2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     TIM3EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     TIM4EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     TIM5EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     TIM6EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     TIM7EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     TIM12EN       LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     TIM13EN       LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     TIM14EN       LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     WWDGEN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     SPI2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     SPI3EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     USART2EN      LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     USART3EN      LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     UART4EN       LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     UART5EN       LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     I2C1EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     I2C2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     I2C3EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     CAN1EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     CAN2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     PWREN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     DACEN         LL_APB1_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM12
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM13
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN1
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN2
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
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
  * @rmtoll APB1ENR     TIM2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     TIM3EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     TIM4EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     TIM5EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     TIM6EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     TIM7EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     TIM12EN       LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     TIM13EN       LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     TIM14EN       LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     WWDGEN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     SPI2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     SPI3EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     USART2EN      LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     USART3EN      LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     UART4EN       LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     UART5EN       LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     I2C1EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     I2C2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     I2C3EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     CAN1EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     CAN2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     PWREN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     DACEN         LL_APB1_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM12
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM13
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN1
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN2
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_APB1_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->APB1ENR, Periphs) == Periphs);
}

/**
  * @brief  Disable APB1 peripherals clock.
  * @rmtoll APB1ENR     TIM2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     TIM3EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     TIM4EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     TIM5EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     TIM6EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     TIM7EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     TIM12EN       LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     TIM13EN       LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     TIM14EN       LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     WWDGEN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     SPI2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     SPI3EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     USART2EN      LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     USART3EN      LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     UART4EN       LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     UART5EN       LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     I2C1EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     I2C2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     I2C3EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     CAN1EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     CAN2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     PWREN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     DACEN         LL_APB1_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM12
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM13
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN1
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN2
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_DisableClock(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB1ENR, Periphs);
}

/**
  * @brief  Force APB1 peripherals reset.
  * @rmtoll APB1RSTR     TIM2RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM3RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM4RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM5RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM6RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM7RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM12RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM13RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM14RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     WWDGRST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     SPI2RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     SPI3RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USART2RST      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USART3RST      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     UART4RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     UART5RST       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     I2C1RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     I2C2RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     I2C3RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     CAN1RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     CAN2RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     PWRRST         LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     DACRST         LL_APB1_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM12
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM13
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN1
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN2
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_ForceReset(uint32_t Periphs)
{
  SET_BIT(RCC->APB1RSTR, Periphs);
}

/**
  * @brief  Release APB1 peripherals reset.
  * @rmtoll APB1RSTR     TIM2RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM3RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM4RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM5RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM6RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM7RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM12RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM13RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM14RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     WWDGRST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     SPI2RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     SPI3RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USART2RST      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USART3RST      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     UART4RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     UART5RST       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     I2C1RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     I2C2RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     I2C3RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     CAN1RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     CAN2RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     PWRRST         LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     DACRST         LL_APB1_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM12
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM13
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN1
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN2
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB1RSTR, Periphs);
}

/**
  * @brief  Enable APB1 peripheral clocks in low-power mode
  * @rmtoll APB1LPENR     TIM2LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     TIM3LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     TIM4LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     TIM5LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     TIM6LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     TIM7LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     TIM12LPEN       LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     TIM13LPEN       LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     TIM14LPEN       LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     WWDGLPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     SPI2LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     SPI3LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     USART2LPEN      LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     USART3LPEN      LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     UART4LPEN       LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     UART5LPEN       LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     I2C1LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     I2C2LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     I2C3LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     CAN1LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     CAN2LPEN        LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     PWRLPEN         LL_APB1_GRP1_EnableClockLowPower\n
  *         APB1LPENR     DACLPEN         LL_APB1_GRP1_EnableClockLowPower
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM12
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM13
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN1
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN2
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_EnableClockLowPower(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->APB1LPENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB1LPENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB1 peripheral clocks in low-power mode
  * @rmtoll APB1LPENR     TIM2LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     TIM3LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     TIM4LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     TIM5LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     TIM6LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     TIM7LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     TIM12LPEN       LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     TIM13LPEN       LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     TIM14LPEN       LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     WWDGLPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     SPI2LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     SPI3LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     USART2LPEN      LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     USART3LPEN      LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     UART4LPEN       LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     UART5LPEN       LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     I2C1LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     I2C2LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     I2C3LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     CAN1LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     CAN2LPEN        LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     PWRLPEN         LL_APB1_GRP1_DisableClockLowPower\n
  *         APB1LPENR     DACLPEN         LL_APB1_GRP1_DisableClockLowPower
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM4
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM5
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM12
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM13
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM14
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN1
  *         @arg @ref LL_APB1_GRP1_PERIPH_CAN2
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_DisableClockLowPower(uint32_t Periphs)
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
  * @rmtoll APB2ENR     TIM1EN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     TIM8EN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     USART1EN      LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     USART6EN      LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     ADC1EN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     ADC2EN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     ADC3EN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     SDIOEN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     SPI1EN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     SYSCFGEN      LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     TIM9EN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     TIM10EN       LL_APB2_GRP1_EnableClock\n
  *         APB2ENR     TIM11EN       LL_APB2_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC2
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC3
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
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
  * @rmtoll APB2ENR     TIM1EN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     TIM8EN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     USART1EN      LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     USART6EN      LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     ADC1EN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     ADC2EN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     ADC3EN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     SDIOEN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     SPI1EN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     SYSCFGEN      LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     TIM9EN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     TIM10EN       LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR     TIM11EN       LL_APB2_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC2
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC3
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_APB2_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->APB2ENR, Periphs) == Periphs);
}

/**
  * @brief  Disable APB2 peripherals clock.
  * @rmtoll APB2ENR     TIM1EN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     TIM8EN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     USART1EN      LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     USART6EN      LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     ADC1EN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     ADC2EN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     ADC3EN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     SDIOEN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     SPI1EN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     SYSCFGEN      LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     TIM9EN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     TIM10EN       LL_APB2_GRP1_DisableClock\n
  *         APB2ENR     TIM11EN       LL_APB2_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC2
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC3
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_DisableClock(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB2ENR, Periphs);
}

/**
  * @brief  Force APB2 peripherals reset.
  * @rmtoll APB2RSTR     TIM1RST        LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     TIM8RST        LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     USART1RST      LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     USART6RST      LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     ADCRST         LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     SDIORST        LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     SPI1RST        LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     SYSCFGRST      LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     TIM9RST        LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     TIM10RST       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     TIM11RST       LL_APB2_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_ForceReset(uint32_t Periphs)
{
  SET_BIT(RCC->APB2RSTR, Periphs);
}

/**
  * @brief  Release APB2 peripherals reset.
  * @rmtoll APB2RSTR     TIM1RST        LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     TIM8RST        LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     USART1RST      LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     USART6RST      LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     ADCRST         LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     SDIORST        LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     SPI1RST        LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     SYSCFGRST      LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     TIM9RST        LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     TIM10RST       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     TIM11RST       LL_APB2_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB2RSTR, Periphs);
}

/**
  * @brief  Enable APB2 peripheral clocks in low-power mode
  * @rmtoll APB2LPENR     TIM1LPEN        LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     TIM8LPEN        LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     USART1LPEN      LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     USART6LPEN      LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     ADC1LPEN        LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     ADC2LPEN        LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     ADC3LPEN        LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     SDIOLPEN        LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     SPI1LPEN        LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     SYSCFGLPEN      LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     TIM9LPEN        LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     TIM10LPEN       LL_APB2_GRP1_EnableClockLowPower\n
  *         APB2LPENR     TIM11LPEN       LL_APB2_GRP1_EnableClockLowPower
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC2
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC3
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_EnableClockLowPower(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->APB2LPENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB2LPENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB2 peripheral clocks in low-power mode
  * @rmtoll APB2LPENR     TIM1LPEN        LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     TIM8LPEN        LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     USART1LPEN      LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     USART6LPEN      LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     ADC1LPEN        LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     ADC2LPEN        LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     ADC3LPEN        LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     SDIOLPEN        LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     SPI1LPEN        LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     SYSCFGLPEN      LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     TIM9LPEN        LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     TIM10LPEN       LL_APB2_GRP1_DisableClockLowPower\n
  *         APB2LPENR     TIM11LPEN       LL_APB2_GRP1_DisableClockLowPower
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC2
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC3
  *         @arg @ref LL_APB2_GRP1_PERIPH_SDIO
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM9
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM10
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM11
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_DisableClockLowPower(uint32_t Periphs)
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

#endif /* __STM32F2xx_LL_BUS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
