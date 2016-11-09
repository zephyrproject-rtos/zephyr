/**
  ******************************************************************************
  * @file    stm32l0xx_ll_bus.h
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
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
#ifndef __STM32L0xx_LL_BUS_H
#define __STM32L0xx_LL_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx.h"

/** @addtogroup STM32L0xx_LL_Driver
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
#define LL_AHB1_GRP1_PERIPH_DMA1           RCC_AHBENR_DMA1EN      /*!< DMA1 clock enable */
#define LL_AHB1_GRP1_PERIPH_MIF            RCC_AHBENR_MIFEN       /*!< MIF clock enable */
#define LL_AHB1_GRP1_PERIPH_SRAM           RCC_AHBSMENR_SRAMSMEN  /*!< Sleep Mode SRAM clock enable */
#define LL_AHB1_GRP1_PERIPH_CRC            RCC_AHBENR_CRCEN       /*!< CRC clock enable */
#if defined(TSC)
#define LL_AHB1_GRP1_PERIPH_TSC            RCC_AHBENR_TSCEN       /*!< TSC clock enable */
#endif /*TSC*/
#if defined(RNG)
#define LL_AHB1_GRP1_PERIPH_RNG            RCC_AHBENR_RNGEN       /*!< RNG clock enable */
#endif /*RNG*/
#if defined(AES)
#define LL_AHB1_GRP1_PERIPH_CRYP           RCC_AHBENR_CRYPEN      /*!< CRYP clock enable */
#endif /*AES*/
/**
  * @}
  */


/** @defgroup BUS_LL_EC_APB1_GRP1_PERIPH  APB1 GRP1 PERIPH
  * @{
  */
#define LL_APB1_GRP1_PERIPH_ALL            (uint32_t)0xFFFFFFFFU
#define LL_APB1_GRP1_PERIPH_TIM2           RCC_APB1ENR_TIM2EN     /*!< TIM2 clock enable */
#if defined(TIM3)
#define LL_APB1_GRP1_PERIPH_TIM3           RCC_APB1ENR_TIM3EN     /*!< TIM3 clock enable */
#endif
#if defined(TIM6)
#define LL_APB1_GRP1_PERIPH_TIM6           RCC_APB1ENR_TIM6EN     /*!< TIM6 clock enable */
#endif
#if defined(TIM7)
#define LL_APB1_GRP1_PERIPH_TIM7           RCC_APB1ENR_TIM7EN     /*!< TIM7 clock enable */
#endif
#if defined(LCD)
#define LL_APB1_GRP1_PERIPH_LCD            RCC_APB1ENR_LCDEN      /*!< LCD clock enable */
#endif /*LCD*/
#define LL_APB1_GRP1_PERIPH_WWDG           RCC_APB1ENR_WWDGEN     /*!< WWDG clock enable */
#if defined(SPI2)
#define LL_APB1_GRP1_PERIPH_SPI2           RCC_APB1ENR_SPI2EN     /*!< SPI2 clock enable */
#endif
#define LL_APB1_GRP1_PERIPH_USART2         RCC_APB1ENR_USART2EN   /*!< USART2 clock enable */
#define LL_APB1_GRP1_PERIPH_LPUART1        RCC_APB1ENR_LPUART1EN  /*!< LPUART1 clock enable */
#if defined(USART4)
#define LL_APB1_GRP1_PERIPH_USART4         RCC_APB1ENR_USART4EN   /*!< USART4 clock enable */
#endif
#if defined(USART5)
#define LL_APB1_GRP1_PERIPH_USART5         RCC_APB1ENR_USART5EN   /*!< USART5 clock enable */
#endif
#define LL_APB1_GRP1_PERIPH_I2C1           RCC_APB1ENR_I2C1EN     /*!< I2C1 clock enable */
#if defined(I2C2)
#define LL_APB1_GRP1_PERIPH_I2C2           RCC_APB1ENR_I2C2EN     /*!< I2C2 clock enable */
#endif
#if defined(USB)
#define LL_APB1_GRP1_PERIPH_USB            RCC_APB1ENR_USBEN      /*!< USB clock enable */
#endif /*USB*/
#if defined(CRS)
#define LL_APB1_GRP1_PERIPH_CRS            RCC_APB1ENR_CRSEN      /*!< CRS clock enable */
#endif /*CRS*/
#define LL_APB1_GRP1_PERIPH_PWR            RCC_APB1ENR_PWREN      /*!< PWR clock enable */
#if defined(DAC)
#define LL_APB1_GRP1_PERIPH_DAC1           RCC_APB1ENR_DACEN      /*!< DAC clock enable */
#endif
#if defined(I2C3)
#define LL_APB1_GRP1_PERIPH_I2C3           RCC_APB1ENR_I2C3EN     /*!< I2C3 clock enable */
#endif
#define LL_APB1_GRP1_PERIPH_LPTIM1         RCC_APB1ENR_LPTIM1EN   /*!< LPTIM1 clock enable */
/**
  * @}
  */




/** @defgroup BUS_LL_EC_APB2_GRP1_PERIPH  APB2 GRP1 PERIPH
  * @{
  */
#define LL_APB2_GRP1_PERIPH_ALL            (uint32_t)0xFFFFFFFFU
#define LL_APB2_GRP1_PERIPH_SYSCFG         RCC_APB2ENR_SYSCFGEN  /*!< SYSCFG clock enable */
#define LL_APB2_GRP1_PERIPH_TIM21          RCC_APB2ENR_TIM21EN   /*!< TIM21 clock enable */
#if defined(TIM22)
#define LL_APB2_GRP1_PERIPH_TIM22          RCC_APB2ENR_TIM22EN   /*!< TIM22 clock enable */
#endif
#define LL_APB2_GRP1_PERIPH_FW             RCC_APB2ENR_FWEN      /*!< FireWall clock enable */
#define LL_APB2_GRP1_PERIPH_ADC1           RCC_APB2ENR_ADC1EN    /*!< ADC1 clock enable */
#define LL_APB2_GRP1_PERIPH_SPI1           RCC_APB2ENR_SPI1EN    /*!< SPI1 clock enable */
#if defined(USART1)
#define LL_APB2_GRP1_PERIPH_USART1         RCC_APB2ENR_USART1EN  /*!< USART1 clock enable */
#endif
#define LL_APB2_GRP1_PERIPH_DBGMCU         RCC_APB2ENR_DBGMCUEN  /*!< DBGMCU clock enable */

/**
  * @}
  */



/** @defgroup BUS_LL_EC_IOP_GRP1_PERIPH  IOP GRP1 PERIPH
  * @{
  */
#define LL_IOP_GRP1_PERIPH_ALL             (uint32_t)0xFFFFFFFFU
#define LL_IOP_GRP1_PERIPH_GPIOA           RCC_IOPENR_GPIOAEN    /*!< GPIO port A control */
#define LL_IOP_GRP1_PERIPH_GPIOB           RCC_IOPENR_GPIOBEN    /*!< GPIO port B control */
#define LL_IOP_GRP1_PERIPH_GPIOC           RCC_IOPENR_GPIOCEN    /*!< GPIO port C control */
#if defined(GPIOD)
#define LL_IOP_GRP1_PERIPH_GPIOD           RCC_IOPENR_GPIODEN    /*!< GPIO port D control */
#endif /*GPIOD*/
#if defined(GPIOE)
#define LL_IOP_GRP1_PERIPH_GPIOE           RCC_IOPENR_GPIOEEN    /*!< GPIO port H control */
#endif /*GPIOE*/
#if defined(GPIOH)
#define LL_IOP_GRP1_PERIPH_GPIOH           RCC_IOPENR_GPIOHEN    /*!< GPIO port H control */
#endif /*GPIOH*/
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
  * @rmtoll AHBENR      DMAEN        LL_AHB1_GRP1_EnableClock\n
  *         AHBENR      MIFEN        LL_AHB1_GRP1_EnableClock\n
  *         AHBENR      CRCEN        LL_AHB1_GRP1_EnableClock\n
  *         AHBENR      TSCEN        LL_AHB1_GRP1_EnableClock\n
  *         AHBENR      RNGEN        LL_AHB1_GRP1_EnableClock\n
  *         AHBENR      CRYPEN       LL_AHB1_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_MIF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_RNG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
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
  * @rmtoll AHBENR      DMAEN        LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR      MIFEN        LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR      CRCEN        LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR      TSCEN        LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR      RNGEN        LL_AHB1_GRP1_IsEnabledClock\n
  *         AHBENR      CRYPEN       LL_AHB1_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_MIF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_RNG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
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
  * @rmtoll AHBENR      DMAEN        LL_AHB1_GRP1_DisableClock\n
  *         AHBENR      MIFEN        LL_AHB1_GRP1_DisableClock\n
  *         AHBENR      CRCEN        LL_AHB1_GRP1_DisableClock\n
  *         AHBENR      TSCEN        LL_AHB1_GRP1_DisableClock\n
  *         AHBENR      RNGEN        LL_AHB1_GRP1_DisableClock\n
  *         AHBENR      CRYPEN       LL_AHB1_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_MIF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_RNG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
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
  * @rmtoll AHBRSTR      DMARST        LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      MIFRST        LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      CRCRST        LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      TSCRST        LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      RNGRST        LL_AHB1_GRP1_ForceReset\n
  *         AHBRSTR      CRYPRST       LL_AHB1_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_MIF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_RNG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
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
  * @rmtoll AHBRSTR      DMARST        LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      MIFRST        LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      CRCRST        LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      TSCRST        LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      RNGRST        LL_AHB1_GRP1_ReleaseReset\n
  *         AHBRSTR      CRYPRST       LL_AHB1_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_MIF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_RNG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
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
  * @rmtoll AHBSMENR     DMASMEN       LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBSMENR     MIFSMEN       LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBSMENR     SRAMSMEN      LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBSMENR     CRCSMEN       LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBSMENR     TSCSMEN       LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBSMENR     RNGSMEN       LL_AHB1_GRP1_EnableClockSleep\n
  *         AHBSMENR     CRYPSMEN      LL_AHB1_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_MIF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_SRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_RNG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->AHBSMENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->AHBSMENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AHB1 peripherals clock during Low Power (Sleep) mode.
  * @rmtoll AHBSMENR     DMASMEN       LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBSMENR     MIFSMEN       LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBSMENR     SRAMSMEN      LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBSMENR     CRCSMEN       LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBSMENR     TSCSMEN       LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBSMENR     RNGSMEN       LL_AHB1_GRP1_DisableClockSleep\n
  *         AHBSMENR     CRYPSMEN      LL_AHB1_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB1_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB1_GRP1_PERIPH_MIF
  *         @arg @ref LL_AHB1_GRP1_PERIPH_SRAM
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRC
  *         @arg @ref LL_AHB1_GRP1_PERIPH_TSC (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_RNG (*)
  *         @arg @ref LL_AHB1_GRP1_PERIPH_CRYP (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_AHB1_GRP1_DisableClockSleep(uint32_t Periphs)
{
  CLEAR_BIT(RCC->AHBSMENR, Periphs);
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
  *         APB1ENR     TIM6EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     TIM7EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     LCDEN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     WWDGEN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     SPI2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     USART2EN      LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     LPUART1EN     LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     USART4EN      LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     USART5EN      LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     I2C1EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     I2C2EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     USBEN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     CRSEN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     PWREN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     DACEN         LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     I2C3EN        LL_APB1_GRP1_EnableClock\n
  *         APB1ENR     LPTIM1EN      LL_APB1_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPUART1
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
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
  * @rmtoll APB1ENR     TIM2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     TIM3EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     TIM6EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     TIM7EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     LCDEN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     WWDGEN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     SPI2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     USART2EN      LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     LPUART1EN     LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     USART4EN      LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     USART5EN      LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     I2C1EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     I2C2EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     USBEN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     CRSEN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     PWREN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     DACEN         LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     I2C3EN        LL_APB1_GRP1_IsEnabledClock\n
  *         APB1ENR     LPTIM1EN      LL_APB1_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPUART1
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
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
  * @rmtoll APB1ENR     TIM2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     TIM3EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     TIM6EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     TIM7EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     LCDEN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     WWDGEN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     SPI2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     USART2EN      LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     LPUART1EN     LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     USART4EN      LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     USART5EN      LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     I2C1EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     I2C2EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     USBEN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     CRSEN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     PWREN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     DACEN         LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     I2C3EN        LL_APB1_GRP1_DisableClock\n
  *         APB1ENR     LPTIM1EN      LL_APB1_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPUART1
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
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
  * @rmtoll APB1RSTR     TIM2RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM3RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM6RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     TIM7RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     LCDRST         LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     WWDGRST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     SPI2RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USART2RST      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     LPUART1RST     LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USART4RST      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USART5RST      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     I2C1RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     I2C2RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     USBRST         LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     CRSRST         LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     PWRRST         LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     DACRST         LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     I2C3RST        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTR     LPTIM1RST      LL_APB1_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPUART1
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
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
  * @rmtoll APB1RSTR     TIM2RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM3RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM6RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     TIM7RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     LCDRST         LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     WWDGRST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     SPI2RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USART2RST      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     LPUART1RST     LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USART4RST      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USART5RST      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     I2C1RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     I2C2RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     USBRST         LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     CRSRST         LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     PWRRST         LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     DACRST         LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     I2C3RST        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTR     LPTIM1RST      LL_APB1_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPUART1
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
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
  * @rmtoll APB1SMENR    TIM2SMEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    TIM3SMEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    TIM6SMEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    TIM7SMEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    LCDSMEN       LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    WWDGSMEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    SPI2SMEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    USART2SMEN    LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    LPUART1SMEN   LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    USART4SMEN    LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    USART5SMEN    LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    I2C1SMEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    I2C2SMEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    USBSMEN       LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    CRSSMEN       LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    PWRSMEN       LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    DACSMEN       LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    I2C3SMEN      LL_APB1_GRP1_EnableClockSleep\n
  *         APB1SMENR    LPTIM1SMEN    LL_APB1_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPUART1
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->APB1SMENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB1SMENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB1 peripherals clock during Low Power (Sleep) mode.
  * @rmtoll APB1SMENR    TIM2SMEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    TIM3SMEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    TIM6SMEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    TIM7SMEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    LCDSMEN       LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    WWDGSMEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    SPI2SMEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    USART2SMEN    LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    LPUART1SMEN   LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    USART4SMEN    LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    USART5SMEN    LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    I2C1SMEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    I2C2SMEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    USBSMEN       LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    CRSSMEN       LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    PWRSMEN       LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    DACSMEN       LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    I2C3SMEN      LL_APB1_GRP1_DisableClockSleep\n
  *         APB1SMENR    LPTIM1SMEN    LL_APB1_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM2
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM6 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_TIM7 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LCD (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPUART1
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART4 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART5 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_USB (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_CRS (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_PWR
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC1 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3 (*)
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB1_GRP1_DisableClockSleep(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB1SMENR, Periphs);
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
  *         APB2ENR      TIM21EN       LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      TIM22EN       LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      FWEN          LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      ADCEN         LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      SPI1EN        LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      USART1EN      LL_APB2_GRP1_EnableClock\n
  *         APB2ENR      DBGEN         LL_APB2_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM21
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM22  (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_FW
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1 (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_DBGMCU
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
  *         APB2ENR      TIM21EN       LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      TIM22EN       LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      FWEN          LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      ADCEN         LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      SPI1EN        LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      USART1EN      LL_APB2_GRP1_IsEnabledClock\n
  *         APB2ENR      DBGEN         LL_APB2_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM21
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM22  (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_FW
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1 (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_DBGMCU
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
  *         APB2ENR      TIM21EN       LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      TIM22EN       LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      FWEN          LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      ADCEN         LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      SPI1EN        LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      USART1EN      LL_APB2_GRP1_DisableClock\n
  *         APB2ENR      DBGEN         LL_APB2_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM21
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM22  (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_FW
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1  (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_DBGMCU
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
  *         APB2RSTR     TIM21RST      LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     TIM22RST      LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     ADCRST        LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     SPI1RST       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     USART1RST     LL_APB2_GRP1_ForceReset\n
  *         APB2RSTR     DBGRST        LL_APB2_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM21
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM22  (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1 (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_DBGMCU
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
  *         APB2RSTR     TIM21RST      LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     TIM22RST      LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     ADCRST        LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     SPI1RST       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     USART1RST     LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTR     DBGRST        LL_APB2_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM21
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM22  (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1  (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_DBGMCU
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
  * @rmtoll APB2SMENR    SYSCFGSMEN    LL_APB2_GRP1_EnableClockSleep\n
  *         APB2SMENR    TIM21SMEN     LL_APB2_GRP1_EnableClockSleep\n
  *         APB2SMENR    TIM22SMEN     LL_APB2_GRP1_EnableClockSleep\n
  *         APB2SMENR    ADCSMEN       LL_APB2_GRP1_EnableClockSleep\n
  *         APB2SMENR    SPI1SMEN      LL_APB2_GRP1_EnableClockSleep\n
  *         APB2SMENR    USART1SMEN    LL_APB2_GRP1_EnableClockSleep\n
  *         APB2SMENR    DBGSMEN       LL_APB2_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM21
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM22  (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1  (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_DBGMCU
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->APB2SMENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB2SMENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB2 peripherals clock during Low Power (Sleep) mode.
  * @rmtoll APB2SMENR    SYSCFGSMEN    LL_APB2_GRP1_DisableClockSleep\n
  *         APB2SMENR    TIM21SMEN     LL_APB2_GRP1_DisableClockSleep\n
  *         APB2SMENR    TIM22SMEN     LL_APB2_GRP1_DisableClockSleep\n
  *         APB2SMENR    ADCSMEN       LL_APB2_GRP1_DisableClockSleep\n
  *         APB2SMENR    SPI1SMEN      LL_APB2_GRP1_DisableClockSleep\n
  *         APB2SMENR    USART1SMEN    LL_APB2_GRP1_DisableClockSleep\n
  *         APB2SMENR    DBGSMEN       LL_APB2_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM21
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM22  (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADC1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART1  (*)
  *         @arg @ref LL_APB2_GRP1_PERIPH_DBGMCU
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_APB2_GRP1_DisableClockSleep(uint32_t Periphs)
{
  CLEAR_BIT(RCC->APB2SMENR, Periphs);
}

/**
  * @}
  */
/** @defgroup BUS_LL_EF_IOP IOP
  * @{
  */

/**
  * @brief  Enable IOP peripherals clock.
  * @rmtoll IOPENR       GPIOAEN       LL_IOP_GRP1_EnableClock\n
  *         IOPENR       GPIOBEN       LL_IOP_GRP1_EnableClock\n
  *         IOPENR       GPIOCEN       LL_IOP_GRP1_EnableClock\n
  *         IOPENR       GPIODEN       LL_IOP_GRP1_EnableClock\n
  *         IOPENR       GPIOEEN       LL_IOP_GRP1_EnableClock\n
  *         IOPENR       GPIOHEN       LL_IOP_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOH (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_IOP_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->IOPENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->IOPENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if IOP peripheral clock is enabled or not
  * @rmtoll IOPENR       GPIOAEN       LL_IOP_GRP1_IsEnabledClock\n
  *         IOPENR       GPIOBEN       LL_IOP_GRP1_IsEnabledClock\n
  *         IOPENR       GPIOCEN       LL_IOP_GRP1_IsEnabledClock\n
  *         IOPENR       GPIODEN       LL_IOP_GRP1_IsEnabledClock\n
  *         IOPENR       GPIOEEN       LL_IOP_GRP1_IsEnabledClock\n
  *         IOPENR       GPIOHEN       LL_IOP_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOH (*)
  *
  *         (*) value not defined in all devices.
  * @retval State of Periphs (1 or 0).
*/
__STATIC_INLINE uint32_t LL_IOP_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->IOPENR, Periphs) == Periphs);
}

/**
  * @brief  Disable IOP peripherals clock.
  * @rmtoll IOPENR       GPIOAEN       LL_IOP_GRP1_DisableClock\n
  *         IOPENR       GPIOBEN       LL_IOP_GRP1_DisableClock\n
  *         IOPENR       GPIOCEN       LL_IOP_GRP1_DisableClock\n
  *         IOPENR       GPIODEN       LL_IOP_GRP1_DisableClock\n
  *         IOPENR       GPIOEEN       LL_IOP_GRP1_DisableClock\n
  *         IOPENR       GPIOHEN       LL_IOP_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOH (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_IOP_GRP1_DisableClock(uint32_t Periphs)
{
  CLEAR_BIT(RCC->IOPENR, Periphs);
}

/**
  * @brief  Disable IOP peripherals clock.
  * @rmtoll IOPRSTR      GPIOASMEN     LL_IOP_GRP1_ForceReset\n
  *         IOPRSTR      GPIOBSMEN     LL_IOP_GRP1_ForceReset\n
  *         IOPRSTR      GPIOCSMEN     LL_IOP_GRP1_ForceReset\n
  *         IOPRSTR      GPIODSMEN     LL_IOP_GRP1_ForceReset\n
  *         IOPRSTR      GPIOESMEN     LL_IOP_GRP1_ForceReset\n
  *         IOPRSTR      GPIOHSMEN     LL_IOP_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_IOP_GRP1_PERIPH_ALL
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOH (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_IOP_GRP1_ForceReset(uint32_t Periphs)
{
  SET_BIT(RCC->IOPRSTR, Periphs);
}

/**
  * @brief  Release IOP peripherals reset.
  * @rmtoll IOPRSTR      GPIOASMEN     LL_IOP_GRP1_ReleaseReset\n
  *         IOPRSTR      GPIOBSMEN     LL_IOP_GRP1_ReleaseReset\n
  *         IOPRSTR      GPIOCSMEN     LL_IOP_GRP1_ReleaseReset\n
  *         IOPRSTR      GPIODSMEN     LL_IOP_GRP1_ReleaseReset\n
  *         IOPRSTR      GPIOESMEN     LL_IOP_GRP1_ReleaseReset\n
  *         IOPRSTR      GPIOHSMEN     LL_IOP_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_IOP_GRP1_PERIPH_ALL
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOH (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_IOP_GRP1_ReleaseReset(uint32_t Periphs)
{
  CLEAR_BIT(RCC->IOPRSTR, Periphs);
}

/**
  * @brief  Enable IOP peripherals clock during Low Power (Sleep) mode.
  * @rmtoll IOPSMENR     GPIOARST      LL_IOP_GRP1_EnableClockSleep\n
  *         IOPSMENR     GPIOBRST      LL_IOP_GRP1_EnableClockSleep\n
  *         IOPSMENR     GPIOCRST      LL_IOP_GRP1_EnableClockSleep\n
  *         IOPSMENR     GPIODRST      LL_IOP_GRP1_EnableClockSleep\n
  *         IOPSMENR     GPIOERST      LL_IOP_GRP1_EnableClockSleep\n
  *         IOPSMENR     GPIOHRST      LL_IOP_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOH (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_IOP_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  SET_BIT(RCC->IOPSMENR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->IOPSMENR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable IOP peripherals clock during Low Power (Sleep) mode.
  * @rmtoll IOPSMENR     GPIOARST      LL_IOP_GRP1_DisableClockSleep\n
  *         IOPSMENR     GPIOBRST      LL_IOP_GRP1_DisableClockSleep\n
  *         IOPSMENR     GPIOCRST      LL_IOP_GRP1_DisableClockSleep\n
  *         IOPSMENR     GPIODRST      LL_IOP_GRP1_DisableClockSleep\n
  *         IOPSMENR     GPIOERST      LL_IOP_GRP1_DisableClockSleep\n
  *         IOPSMENR     GPIOHRST      LL_IOP_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOD (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOE (*)
  *         @arg @ref LL_IOP_GRP1_PERIPH_GPIOH (*)
  *
  *         (*) value not defined in all devices.
  * @retval None
*/
__STATIC_INLINE void LL_IOP_GRP1_DisableClockSleep(uint32_t Periphs)
{
  CLEAR_BIT(RCC->IOPSMENR, Periphs);
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

#endif /* __STM32L0xx_LL_BUS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
