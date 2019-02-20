/**
  ******************************************************************************
  * @file    stm32mp1xx_ll_bus.h
  * @author  MCD Application Team
  * @version $VERSION$
  * @date    $DATE$
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
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32MP1xx_LL_BUS_H
#define STM32MP1xx_LL_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx.h"

/** @addtogroup STM32MP1xx_LL_Driver
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

/** @defgroup BUS_LL_EC_AHB2_GRP1_PERIPH  AHB2 GRP1 PERIPH
  * @{
  */
#define LL_AHB2_GRP1_PERIPH_ALL            0x00010127U
#define LL_AHB2_GRP1_PERIPH_DMA1           RCC_MC_AHB2ENSETR_DMA1EN
#define LL_AHB2_GRP1_PERIPH_DMA2           RCC_MC_AHB2ENSETR_DMA2EN
#define LL_AHB2_GRP1_PERIPH_DMAMUX         RCC_MC_AHB2ENSETR_DMAMUXEN
#define LL_AHB2_GRP1_PERIPH_ADC12          RCC_MC_AHB2ENSETR_ADC12EN
#define LL_AHB2_GRP1_PERIPH_USBO           RCC_MC_AHB2ENSETR_USBOEN
#define LL_AHB2_GRP1_PERIPH_SDMMC3         RCC_MC_AHB2ENSETR_SDMMC3EN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_AHB3_GRP1_PERIPH  AHB3 GRP1 PERIPH
  * @{
  */
#define LL_AHB3_GRP1_PERIPH_DCMI           RCC_MC_AHB3ENSETR_DCMIEN
#if defined(CRYP2)
#define LL_AHB3_GRP1_PERIPH_CRYP2          RCC_MC_AHB3ENSETR_CRYP2EN
#define LL_AHB3_GRP1_PERIPH_ALL            0x000018F1U
#else /*!CRYP2*/
#define LL_AHB3_GRP1_PERIPH_ALL            0x000018E1U
#endif /* CRYP2 */
#define LL_AHB3_GRP1_PERIPH_HASH2          RCC_MC_AHB3ENSETR_HASH2EN
#define LL_AHB3_GRP1_PERIPH_RNG2           RCC_MC_AHB3ENSETR_RNG2EN
#define LL_AHB3_GRP1_PERIPH_CRC2           RCC_MC_AHB3ENSETR_CRC2EN
#define LL_AHB3_GRP1_PERIPH_HSEM           RCC_MC_AHB3ENSETR_HSEMEN
#define LL_AHB3_GRP1_PERIPH_IPCC           RCC_MC_AHB3ENSETR_IPCCEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_AHB4_GRP1_PERIPH  AHB4 GRP1 PERIPH
  * @{
  */
#define LL_AHB4_GRP1_PERIPH_ALL            0x000007FFU
#define LL_AHB4_GRP1_PERIPH_GPIOA          RCC_MC_AHB4ENSETR_GPIOAEN
#define LL_AHB4_GRP1_PERIPH_GPIOB          RCC_MC_AHB4ENSETR_GPIOBEN
#define LL_AHB4_GRP1_PERIPH_GPIOC          RCC_MC_AHB4ENSETR_GPIOCEN
#define LL_AHB4_GRP1_PERIPH_GPIOD          RCC_MC_AHB4ENSETR_GPIODEN
#define LL_AHB4_GRP1_PERIPH_GPIOE          RCC_MC_AHB4ENSETR_GPIOEEN
#define LL_AHB4_GRP1_PERIPH_GPIOF          RCC_MC_AHB4ENSETR_GPIOFEN
#define LL_AHB4_GRP1_PERIPH_GPIOG          RCC_MC_AHB4ENSETR_GPIOGEN
#define LL_AHB4_GRP1_PERIPH_GPIOH          RCC_MC_AHB4ENSETR_GPIOHEN
#define LL_AHB4_GRP1_PERIPH_GPIOI          RCC_MC_AHB4ENSETR_GPIOIEN
#define LL_AHB4_GRP1_PERIPH_GPIOJ          RCC_MC_AHB4ENSETR_GPIOJEN
#define LL_AHB4_GRP1_PERIPH_GPIOK          RCC_MC_AHB4ENSETR_GPIOKEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_AHB5_GRP1_PERIPH  AHB5 GRP1 PERIPH
  * @{
  */
/**
  * @note LL_AHB5_GRP1_PERIPH_ALL only contains reset values (not enable)
  */
#define LL_AHB5_GRP1_PERIPH_GPIOZ          RCC_MC_AHB5ENSETR_GPIOZEN
#if defined(CRYP1)
#define LL_AHB5_GRP1_PERIPH_ALL            0x00010071U
#define LL_AHB5_GRP1_PERIPH_CRYP1          RCC_MC_AHB5ENSETR_CRYP1EN
#else /* !CRYP1 */
#define LL_AHB5_GRP1_PERIPH_ALL            0x00010061U
#endif /* CRYP1 */
#define LL_AHB5_GRP1_PERIPH_HASH1          RCC_MC_AHB5ENSETR_HASH1EN
#define LL_AHB5_GRP1_PERIPH_RNG1           RCC_MC_AHB5ENSETR_RNG1EN
#define LL_AHB5_GRP1_PERIPH_BKPSRAM        RCC_MC_AHB5ENSETR_BKPSRAMEN
#define LL_AHB5_GRP1_PERIPH_AXIMC          RCC_MC_AHB5ENSETR_AXIMC
/**
  * @}
  */

/** @defgroup BUS_LL_EC_AHB6_GRP1_PERIPH  AHB6 GRP1 PERIPH
  * @{
  */
#define LL_AHB6_GRP1_PERIPH_MDMA           RCC_MC_AHB6ENSETR_MDMAEN
#define LL_AHB6_GRP1_PERIPH_GPU            RCC_MC_AHB6ENSETR_GPUEN
#define LL_AHB6_GRP1_PERIPH_ETH1CK         RCC_MC_AHB6ENSETR_ETHCKEN
#define LL_AHB6_GRP1_PERIPH_ETH1TX         RCC_MC_AHB6ENSETR_ETHTXEN
#define LL_AHB6_GRP1_PERIPH_ETH1RX         RCC_MC_AHB6ENSETR_ETHRXEN
#define LL_AHB6_GRP1_PERIPH_ETH1MAC        RCC_MC_AHB6ENSETR_ETHMACEN
#define LL_AHB6_GRP1_PERIPH_ETH1STP        RCC_MC_AHB6LPENSETR_ETHSTPEN
#define LL_AHB6_GRP1_PERIPH_FMC            RCC_MC_AHB6ENSETR_FMCEN
#define LL_AHB6_GRP1_PERIPH_QSPI           RCC_MC_AHB6ENSETR_QSPIEN
#define LL_AHB6_GRP1_PERIPH_SDMMC1         RCC_MC_AHB6ENSETR_SDMMC1EN
#define LL_AHB6_GRP1_PERIPH_SDMMC2         RCC_MC_AHB6ENSETR_SDMMC2EN
#define LL_AHB6_GRP1_PERIPH_CRC1           RCC_MC_AHB6ENSETR_CRC1EN
#define LL_AHB6_GRP1_PERIPH_USBH           RCC_MC_AHB6ENSETR_USBHEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_AXI_GRP1_PERIPH  AXI GRP1 PERIPH
  * @{
  */
#define LL_AXI_GRP1_PERIPH_ALL            0x00000001U
#define LL_AXI_GRP1_PERIPH_SYSRAMEN       RCC_MC_AXIMENSETR_SYSRAMEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_MLAHB_GRP1_PERIPH  MLAHB GRP1 PERIPH
  * @{
  */
#define LL_MLAHB_GRP1_PERIPH_ALL           0x00000010U
#define LL_MLAHB_GRP1_PERIPH_RETRAMEN      RCC_MC_MLAHBENSETR_RETRAMEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_APB1_GRP1_PERIPH  APB1 GRP1 PERIPH
  * @{
  */
#define LL_APB1_GRP1_PERIPH_ALL            0xADEFDBFFU
#define LL_APB1_GRP1_PERIPH_TIM2           RCC_MC_APB1ENSETR_TIM2EN
#define LL_APB1_GRP1_PERIPH_TIM3           RCC_MC_APB1ENSETR_TIM3EN
#define LL_APB1_GRP1_PERIPH_TIM4           RCC_MC_APB1ENSETR_TIM4EN
#define LL_APB1_GRP1_PERIPH_TIM5           RCC_MC_APB1ENSETR_TIM5EN
#define LL_APB1_GRP1_PERIPH_TIM6           RCC_MC_APB1ENSETR_TIM6EN
#define LL_APB1_GRP1_PERIPH_TIM7           RCC_MC_APB1ENSETR_TIM7EN
#define LL_APB1_GRP1_PERIPH_TIM12          RCC_MC_APB1ENSETR_TIM12EN
#define LL_APB1_GRP1_PERIPH_TIM13          RCC_MC_APB1ENSETR_TIM13EN
#define LL_APB1_GRP1_PERIPH_TIM14          RCC_MC_APB1ENSETR_TIM14EN
#define LL_APB1_GRP1_PERIPH_LPTIM1         RCC_MC_APB1ENSETR_LPTIM1EN
#define LL_APB1_GRP1_PERIPH_SPI2           RCC_MC_APB1ENSETR_SPI2EN
#define LL_APB1_GRP1_PERIPH_SPI3           RCC_MC_APB1ENSETR_SPI3EN
#define LL_APB1_GRP1_PERIPH_USART2         RCC_MC_APB1ENSETR_USART2EN
#define LL_APB1_GRP1_PERIPH_USART3         RCC_MC_APB1ENSETR_USART3EN
#define LL_APB1_GRP1_PERIPH_UART4          RCC_MC_APB1ENSETR_UART4EN
#define LL_APB1_GRP1_PERIPH_UART5          RCC_MC_APB1ENSETR_UART5EN
#define LL_APB1_GRP1_PERIPH_UART7          RCC_MC_APB1ENSETR_UART7EN
#define LL_APB1_GRP1_PERIPH_UART8          RCC_MC_APB1ENSETR_UART8EN
#define LL_APB1_GRP1_PERIPH_I2C1           RCC_MC_APB1ENSETR_I2C1EN
#define LL_APB1_GRP1_PERIPH_I2C2           RCC_MC_APB1ENSETR_I2C2EN
#define LL_APB1_GRP1_PERIPH_I2C3           RCC_MC_APB1ENSETR_I2C3EN
#define LL_APB1_GRP1_PERIPH_I2C5           RCC_MC_APB1ENSETR_I2C5EN
#define LL_APB1_GRP1_PERIPH_SPDIF          RCC_MC_APB1ENSETR_SPDIFEN
#define LL_APB1_GRP1_PERIPH_CEC            RCC_MC_APB1ENSETR_CECEN
#define LL_APB1_GRP1_PERIPH_WWDG1          RCC_MC_APB1ENSETR_WWDG1EN
#define LL_APB1_GRP1_PERIPH_DAC12          RCC_MC_APB1ENSETR_DAC12EN
#define LL_APB1_GRP1_PERIPH_MDIOS          RCC_MC_APB1ENSETR_MDIOSEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_APB2_GRP1_PERIPH  APB2 GRP1 PERIPH
  * @{
  */
/**
  * @note LL_APB2_GRP1_PERIPH_ALL only contains reset values (not enable)
  */
#define LL_APB2_GRP1_PERIPH_ALL            0x117271FU
#define LL_APB2_GRP1_PERIPH_TIM1           RCC_MC_APB2ENSETR_TIM1EN
#define LL_APB2_GRP1_PERIPH_TIM8           RCC_MC_APB2ENSETR_TIM8EN
#define LL_APB2_GRP1_PERIPH_TIM15          RCC_MC_APB2ENSETR_TIM15EN
#define LL_APB2_GRP1_PERIPH_TIM16          RCC_MC_APB2ENSETR_TIM16EN
#define LL_APB2_GRP1_PERIPH_TIM17          RCC_MC_APB2ENSETR_TIM17EN
#define LL_APB2_GRP1_PERIPH_SPI1           RCC_MC_APB2ENSETR_SPI1EN
#define LL_APB2_GRP1_PERIPH_SPI4           RCC_MC_APB2ENSETR_SPI4EN
#define LL_APB2_GRP1_PERIPH_SPI5           RCC_MC_APB2ENSETR_SPI5EN
#define LL_APB2_GRP1_PERIPH_USART6         RCC_MC_APB2ENSETR_USART6EN
#define LL_APB2_GRP1_PERIPH_SAI1           RCC_MC_APB2ENSETR_SAI1EN
#define LL_APB2_GRP1_PERIPH_SAI2           RCC_MC_APB2ENSETR_SAI2EN
#define LL_APB2_GRP1_PERIPH_SAI3           RCC_MC_APB2ENSETR_SAI3EN
#define LL_APB2_GRP1_PERIPH_DFSDM1         RCC_MC_APB2ENSETR_DFSDMEN
#define LL_APB2_GRP1_PERIPH_ADFSDM1        RCC_MC_APB2ENSETR_ADFSDMEN
#define LL_APB2_GRP1_PERIPH_FDCAN          RCC_MC_APB2ENSETR_FDCANEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_APB3_GRP1_PERIPH  APB3 GRP1 PERIPH
  * @{
  */
/**
  * @note LL_APB3_GRP1_PERIPH_ALL only contains reset values (not enable)
  */
#define LL_APB3_GRP1_PERIPH_ALL            0x0003290FU
#define LL_APB3_GRP1_PERIPH_LPTIM2         RCC_MC_APB3ENSETR_LPTIM2EN
#define LL_APB3_GRP1_PERIPH_LPTIM3         RCC_MC_APB3ENSETR_LPTIM3EN
#define LL_APB3_GRP1_PERIPH_LPTIM4         RCC_MC_APB3ENSETR_LPTIM4EN
#define LL_APB3_GRP1_PERIPH_LPTIM5         RCC_MC_APB3ENSETR_LPTIM5EN
#define LL_APB3_GRP1_PERIPH_SAI4           RCC_MC_APB3ENSETR_SAI4EN
#define LL_APB3_GRP1_PERIPH_SYSCFG         RCC_MC_APB3ENSETR_SYSCFGEN
#define LL_APB3_GRP1_PERIPH_VREF           RCC_MC_APB3ENSETR_VREFEN
#define LL_APB3_GRP1_PERIPH_TMPSENS        RCC_MC_APB3ENSETR_TMPSENSEN
#define LL_APB3_GRP1_PERIPH_PMBCTRL        RCC_MC_APB3ENSETR_PMBCTRLEN
#define LL_APB3_GRP1_PERIPH_HDP            RCC_MC_APB3ENSETR_HDPEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_APB4_GRP1_PERIPH  APB4 GRP1 PERIPH
  * @{
  */
/**
  * @note LL_APB4_GRP1_PERIPH_ALL only contains reset values (not enable)
  */
#define LL_APB4_GRP1_PERIPH_ALL            0x00010111U
#define LL_APB4_GRP1_PERIPH_LTDC           RCC_MC_APB4ENSETR_LTDCEN
#define LL_APB4_GRP1_PERIPH_DSI            RCC_MC_APB4ENSETR_DSIEN
#define LL_APB4_GRP1_PERIPH_DDRPERFM       RCC_MC_APB4ENSETR_DDRPERFMEN
#define LL_APB4_GRP1_PERIPH_USBPHY         RCC_MC_APB4ENSETR_USBPHYEN
#define LL_APB4_GRP1_PERIPH_STGENRO        RCC_MC_APB4ENSETR_STGENROEN
#define LL_APB4_GRP1_PERIPH_STGENROSTP     RCC_MC_APB4LPENSETR_STGENROSTPEN
/**
  * @}
  */

/** @defgroup BUS_LL_EC_APB5_GRP1_PERIPH  APB5 GRP1 PERIPH
  * @{
  */
/**
  * @note LL_APB5_GRP1_PERIPH_ALL only contains reset values (not enable)
  */
#define LL_APB5_GRP1_PERIPH_ALL            0x0010001DU
#define LL_APB5_GRP1_PERIPH_SPI6           RCC_MC_APB5ENSETR_SPI6EN
#define LL_APB5_GRP1_PERIPH_I2C4           RCC_MC_APB5ENSETR_I2C4EN
#define LL_APB5_GRP1_PERIPH_I2C6           RCC_MC_APB5ENSETR_I2C6EN
#define LL_APB5_GRP1_PERIPH_USART1         RCC_MC_APB5ENSETR_USART1EN
#define LL_APB5_GRP1_PERIPH_RTCAPB         RCC_MC_APB5ENSETR_RTCAPBEN
#define LL_APB5_GRP1_PERIPH_TZC1           RCC_MC_APB5ENSETR_TZC1EN
#define LL_APB5_GRP1_PERIPH_TZC2           RCC_MC_APB5ENSETR_TZC2EN
#define LL_APB5_GRP1_PERIPH_TZPC           RCC_MC_APB5ENSETR_TZPCEN
#define LL_APB5_GRP1_PERIPH_BSEC           RCC_MC_APB5ENSETR_BSECEN
#define LL_APB5_GRP1_PERIPH_STGEN          RCC_MC_APB5ENSETR_STGENEN
#define LL_APB5_GRP1_PERIPH_STGENSTP       RCC_MC_APB5LPENSETR_STGENSTPEN
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

/** @defgroup BUS_LL_EF_AHB2 AHB2
  * @{
  */

/**
  * @brief  Enable AHB2 peripherals clock.
  * @rmtoll MC_AHB2ENSETR      DMA1          LL_AHB2_GRP1_EnableClock\n
  *         MC_AHB2ENSETR      DMA2          LL_AHB2_GRP1_EnableClock\n
  *         MC_AHB2ENSETR      DMAMUX        LL_AHB2_GRP1_EnableClock\n
  *         MC_AHB2ENSETR      ADC12         LL_AHB2_GRP1_EnableClock\n
  *         MC_AHB2ENSETR      USBO          LL_AHB2_GRP1_EnableClock\n
  *         MC_AHB2ENSETR      SDMMC3        LL_AHB2_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMAMUX
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ADC12
  *         @arg @ref LL_AHB2_GRP1_PERIPH_USBO
  *         @arg @ref LL_AHB2_GRP1_PERIPH_SDMMC3
  * @retval None
  */
__STATIC_INLINE void LL_AHB2_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AHB2ENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AHB2ENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if AHB2 peripheral clock is enabled or not
  * @rmtoll MC_AHB2ENSETR      DMA1          LL_AHB2_GRP1_IsEnabledClock\n
  *         MC_AHB2ENSETR      DMA2          LL_AHB2_GRP1_IsEnabledClock\n
  *         MC_AHB2ENSETR      DMAMUX        LL_AHB2_GRP1_IsEnabledClock\n
  *         MC_AHB2ENSETR      ADC12         LL_AHB2_GRP1_IsEnabledClock\n
  *         MC_AHB2ENSETR      USBO          LL_AHB2_GRP1_IsEnabledClock\n
  *         MC_AHB2ENSETR      SDMMC3        LL_AHB2_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMAMUX
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ADC12
  *         @arg @ref LL_AHB2_GRP1_PERIPH_USBO
  *         @arg @ref LL_AHB2_GRP1_PERIPH_SDMMC3
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_AHB2_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_AHB2ENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable AHB2 peripherals clock.
  * @rmtoll MC_AHB2ENCLRR      DMA1          LL_AHB2_GRP1_DisableClock\n
  *         MC_AHB2ENCLRR      DMA2          LL_AHB2_GRP1_DisableClock\n
  *         MC_AHB2ENCLRR      DMAMUX        LL_AHB2_GRP1_DisableClock\n
  *         MC_AHB2ENCLRR      ADC12         LL_AHB2_GRP1_DisableClock\n
  *         MC_AHB2ENCLRR      USBO          LL_AHB2_GRP1_DisableClock\n
  *         MC_AHB2ENCLRR      SDMMC3        LL_AHB2_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMAMUX
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ADC12
  *         @arg @ref LL_AHB2_GRP1_PERIPH_USBO
  *         @arg @ref LL_AHB2_GRP1_PERIPH_SDMMC3
  * @retval None
  */
__STATIC_INLINE void LL_AHB2_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AHB2ENCLRR, Periphs);
}

/**
  * @brief  Force AHB2 peripherals reset.
  * @rmtoll AHB2RSTSETR     DMA1        LL_AHB2_GRP1_ForceReset\n
  *         AHB2RSTSETR     DMA2        LL_AHB2_GRP1_ForceReset\n
  *         AHB2RSTSETR     DMAMUX      LL_AHB2_GRP1_ForceReset\n
  *         AHB2RSTSETR     ADC12       LL_AHB2_GRP1_ForceReset\n
  *         AHB2RSTSETR     USBO        LL_AHB2_GRP1_ForceReset\n
  *         AHB2RSTSETR     SDMMC3      LL_AHB2_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMAMUX
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ADC12
  *         @arg @ref LL_AHB2_GRP1_PERIPH_USBO
  *         @arg @ref LL_AHB2_GRP1_PERIPH_SDMMC3
  * @retval None
  */
__STATIC_INLINE void LL_AHB2_GRP1_ForceReset(uint32_t Periphs)
{
  WRITE_REG(RCC->AHB2RSTSETR, Periphs);
}

/**
  * @brief  Release AHB2 peripherals reset.
  * @rmtoll AHB2RSTCLRR     DMA1        LL_AHB2_GRP1_ReleaseReset\n
  *         AHB2RSTCLRR     DMA2        LL_AHB2_GRP1_ReleaseReset\n
  *         AHB2RSTCLRR     DMAMUX      LL_AHB2_GRP1_ReleaseReset\n
  *         AHB2RSTCLRR     ADC12       LL_AHB2_GRP1_ReleaseReset\n
  *         AHB2RSTCLRR     USBO        LL_AHB2_GRP1_ReleaseReset\n
  *         AHB2RSTCLRR     SDMMC3      LL_AHB2_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMAMUX
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ADC12
  *         @arg @ref LL_AHB2_GRP1_PERIPH_USBO
  *         @arg @ref LL_AHB2_GRP1_PERIPH_SDMMC3
  * @retval None
  */
__STATIC_INLINE void LL_AHB2_GRP1_ReleaseReset(uint32_t Periphs)
{
  WRITE_REG(RCC->AHB2RSTCLRR, Periphs);
}

/**
  * @brief  Enable AHB2 peripheral clocks in Sleep mode
  * @rmtoll MC_AHB2LPENSETR    DMA1       LL_AHB2_GRP1_EnableClockSleep\n
  *         MC_AHB2LPENSETR    DMA2       LL_AHB2_GRP1_EnableClockSleep\n
  *         MC_AHB2LPENSETR    DMAMUX     LL_AHB2_GRP1_EnableClockSleep\n
  *         MC_AHB2LPENSETR    ADC12      LL_AHB2_GRP1_EnableClockSleep\n
  *         MC_AHB2LPENSETR    USBO       LL_AHB2_GRP1_EnableClockSleep\n
  *         MC_AHB2LPENCLRR    SDMMC3     LL_AHB2_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMAMUX
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ADC12
  *         @arg @ref LL_AHB2_GRP1_PERIPH_USBO
  *         @arg @ref LL_AHB2_GRP1_PERIPH_SDMMC3
  * @retval None
  */
__STATIC_INLINE void LL_AHB2_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AHB2LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AHB2LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AHB2 peripheral clocks in Sleep mode
  * @rmtoll MC_AHB2LPENCLRR    DMA1       LL_AHB2_GRP1_DisableClockSleep\n
  *         MC_AHB2LPENCLRR    DMA2       LL_AHB2_GRP1_DisableClockSleep\n
  *         MC_AHB2LPENCLRR    DMAMUX     LL_AHB2_GRP1_DisableClockSleep\n
  *         MC_AHB2LPENCLRR    ADC12      LL_AHB2_GRP1_DisableClockSleep\n
  *         MC_AHB2LPENCLRR    USBO       LL_AHB2_GRP1_DisableClockSleep\n
  *         MC_AHB2LPENCLRR    SDMMC3     LL_AHB2_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA1
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMA2
  *         @arg @ref LL_AHB2_GRP1_PERIPH_DMAMUX
  *         @arg @ref LL_AHB2_GRP1_PERIPH_ADC12
  *         @arg @ref LL_AHB2_GRP1_PERIPH_USBO
  *         @arg @ref LL_AHB2_GRP1_PERIPH_SDMMC3
  * @retval None
  */
__STATIC_INLINE void LL_AHB2_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AHB2LPENCLRR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_AHB3 AHB3
  * @{
  */

/**
  * @brief  Enable AHB3 peripherals clock.
  * @rmtoll MC_AHB3ENSETR      DCMI       LL_AHB3_GRP1_EnableClock\n
  *         MC_AHB3ENSETR      CRYP2      LL_AHB3_GRP1_EnableClock\n
  *         MC_AHB3ENSETR      HASH2      LL_AHB3_GRP1_EnableClock\n
  *         MC_AHB3ENSETR      RNG2       LL_AHB3_GRP1_EnableClock\n
  *         MC_AHB3ENSETR      CRC2       LL_AHB3_GRP1_EnableClock\n
  *         MC_AHB3ENSETR      HSEM       LL_AHB3_GRP1_EnableClock\n
  *         MC_AHB3ENSETR      IPCC       LL_AHB3_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_DCMI
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRYP2 (*)
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HASH2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_RNG2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRC2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HSEM
  *         @arg @ref LL_AHB3_GRP1_PERIPH_IPCC
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB3_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AHB3ENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AHB3ENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if AHB3 peripheral clock is enabled or not
  * @rmtoll MC_AHB3ENSETR      DCMI       LL_AHB3_GRP1_IsEnabledClock\n
  *         MC_AHB3ENSETR      CRYP2      LL_AHB3_GRP1_IsEnabledClock\n
  *         MC_AHB3ENSETR      HASH2      LL_AHB3_GRP1_IsEnabledClock\n
  *         MC_AHB3ENSETR      RNG2       LL_AHB3_GRP1_IsEnabledClock\n
  *         MC_AHB3ENSETR      CRC2       LL_AHB3_GRP1_IsEnabledClock\n
  *         MC_AHB3ENSETR      HSEM       LL_AHB3_GRP1_IsEnabledClock\n
  *         MC_AHB3ENSETR      IPCC       LL_AHB3_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_DCMI
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRYP2 (*)
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HASH2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_RNG2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRC2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HSEM
  *         @arg @ref LL_AHB3_GRP1_PERIPH_IPCC
  *
  *         (*) value not defined in all devices.
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_AHB3_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_AHB3ENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable AHB3 peripherals clock.
  * @rmtoll MC_AHB3ENCLRR      DCMI       LL_AHB3_GRP1_DisableClock\n
  *         MC_AHB3ENCLRR      CRYP2      LL_AHB3_GRP1_DisableClock\n
  *         MC_AHB3ENCLRR      HASH2      LL_AHB3_GRP1_DisableClock\n
  *         MC_AHB3ENCLRR      RNG2       LL_AHB3_GRP1_DisableClock\n
  *         MC_AHB3ENCLRR      CRC2       LL_AHB3_GRP1_DisableClock\n
  *         MC_AHB3ENCLRR      HSEM       LL_AHB3_GRP1_DisableClock\n
  *         MC_AHB3ENCLRR      IPCC       LL_AHB3_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_DCMI
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRYP2 (*)
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HASH2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_RNG2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRC2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HSEM
  *         @arg @ref LL_AHB3_GRP1_PERIPH_IPCC
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB3_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AHB3ENCLRR, Periphs);
}

/**
  * @brief  Force AHB3 peripherals reset.
  * @rmtoll AHB3RSTSETR     DCMI       LL_AHB3_GRP1_ForceReset\n
  *         AHB3RSTSETR     CRYP2      LL_AHB3_GRP1_ForceReset\n
  *         AHB3RSTSETR     HASH2      LL_AHB3_GRP1_ForceReset\n
  *         AHB3RSTSETR     RNG2       LL_AHB3_GRP1_ForceReset\n
  *         AHB3RSTSETR     CRC2       LL_AHB3_GRP1_ForceReset\n
  *         AHB3RSTSETR     HSEM       LL_AHB3_GRP1_ForceReset\n
  *         AHB3RSTSETR     IPCC       LL_AHB3_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB3_GRP1_PERIPH_DCMI
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRYP2 (*)
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HASH2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_RNG2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRC2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HSEM
  *         @arg @ref LL_AHB3_GRP1_PERIPH_IPCC
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB3_GRP1_ForceReset(uint32_t Periphs)
{
  WRITE_REG(RCC->AHB3RSTSETR, Periphs);
}

/**
  * @brief  Release AHB3 peripherals reset.
  * @rmtoll AHB3RSTCLRR     DCMI       LL_AHB3_GRP1_ReleaseReset\n
  *         AHB3RSTCLRR     CRYP2      LL_AHB3_GRP1_ReleaseReset\n
  *         AHB3RSTCLRR     HASH2      LL_AHB3_GRP1_ReleaseReset\n
  *         AHB3RSTCLRR     RNG2       LL_AHB3_GRP1_ReleaseReset\n
  *         AHB3RSTCLRR     CRC2       LL_AHB3_GRP1_ReleaseReset\n
  *         AHB3RSTCLRR     HSEM       LL_AHB3_GRP1_ReleaseReset\n
  *         AHB3RSTCLRR     IPCC       LL_AHB3_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB3_GRP1_PERIPH_DCMI
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRYP2 (*)
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HASH2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_RNG2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRC2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HSEM
  *         @arg @ref LL_AHB3_GRP1_PERIPH_IPCC
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB3_GRP1_ReleaseReset(uint32_t Periphs)
{
  WRITE_REG(RCC->AHB3RSTCLRR, Periphs);
}

/**
  * @brief  Enable AHB3 peripheral clocks in Sleep mode
  * @rmtoll MC_AHB3LPENSETR    DCMI       LL_AHB3_GRP1_EnableClockSleep\n
  *         MC_AHB3LPENSETR    CRYP2      LL_AHB3_GRP1_EnableClockSleep\n
  *         MC_AHB3LPENSETR    HASH2      LL_AHB3_GRP1_EnableClockSleep\n
  *         MC_AHB3LPENSETR    RNG2       LL_AHB3_GRP1_EnableClockSleep\n
  *         MC_AHB3LPENSETR    CRC2       LL_AHB3_GRP1_EnableClockSleep\n
  *         MC_AHB3LPENSETR    HSEM       LL_AHB3_GRP1_EnableClockSleep\n
  *         MC_AHB3LPENSETR    IPCC       LL_AHB3_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_DCMI
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRYP2 (*)
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HASH2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_RNG2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRC2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HSEM
  *         @arg @ref LL_AHB3_GRP1_PERIPH_IPCC
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB3_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AHB3LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AHB3LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AHB3 peripheral clocks in Sleep mode
  * @rmtoll MC_AHB3LPENCLRR    DCMI       LL_AHB3_GRP1_DisableClockSleep\n
  *         MC_AHB3LPENCLRR    CRYP2      LL_AHB3_GRP1_DisableClockSleep\n
  *         MC_AHB3LPENCLRR    HASH2      LL_AHB3_GRP1_DisableClockSleep\n
  *         MC_AHB3LPENCLRR    RNG2       LL_AHB3_GRP1_DisableClockSleep\n
  *         MC_AHB3LPENCLRR    CRC2       LL_AHB3_GRP1_DisableClockSleep\n
  *         MC_AHB3LPENCLRR    HSEM       LL_AHB3_GRP1_DisableClockSleep\n
  *         MC_AHB3LPENCLRR    IPCC       LL_AHB3_GRP1_DisableClockSleep
  *
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB3_GRP1_PERIPH_DCMI
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRYP2 (*)
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HASH2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_RNG2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_CRC2
  *         @arg @ref LL_AHB3_GRP1_PERIPH_HSEM
  *         @arg @ref LL_AHB3_GRP1_PERIPH_IPCC
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB3_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AHB3LPENCLRR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_AHB4 AHB4
  * @{
  */

/**
  * @brief  Enable AHB4 peripherals clock.
  * @rmtoll MC_AHB4ENSETR      GPIOA       LL_AHB4_GRP1_EnableClock\n
  *         MC_AHB4ENSETR      GPIOB       LL_AHB4_GRP1_EnableClock\n
  *         MC_AHB4ENSETR      GPIOC       LL_AHB4_GRP1_EnableClock\n
  *         MC_AHB4ENSETR      GPIOD       LL_AHB4_GRP1_EnableClock\n
  *         MC_AHB4ENSETR      GPIOE       LL_AHB4_GRP1_EnableClock\n
  *         MC_AHB4ENSETR      GPIOF       LL_AHB4_GRP1_EnableClock\n
  *         MC_AHB4ENSETR      GPIOG       LL_AHB4_GRP1_EnableClock\n
  *         MC_AHB4ENSETR      GPIOH       LL_AHB4_GRP1_EnableClock\n
  *         MC_AHB4ENSETR      GPIOI       LL_AHB4_GRP1_EnableClock\n
  *         MC_AHB4ENSETR      GPIOJ       LL_AHB4_GRP1_EnableClock\n
  *         MC_AHB4ENSETR      GPIOK       LL_AHB4_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOJ
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOK
  * @retval None
  */
__STATIC_INLINE void LL_AHB4_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AHB4ENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AHB4ENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if AHB4 peripheral clock is enabled or not
  * @rmtoll MC_AHB4ENSETR      GPIOA       LL_AHB4_GRP1_IsEnabledClock\n
  *         MC_AHB4ENSETR      GPIOB       LL_AHB4_GRP1_IsEnabledClock\n
  *         MC_AHB4ENSETR      GPIOC       LL_AHB4_GRP1_IsEnabledClock\n
  *         MC_AHB4ENSETR      GPIOD       LL_AHB4_GRP1_IsEnabledClock\n
  *         MC_AHB4ENSETR      GPIOE       LL_AHB4_GRP1_IsEnabledClock\n
  *         MC_AHB4ENSETR      GPIOF       LL_AHB4_GRP1_IsEnabledClock\n
  *         MC_AHB4ENSETR      GPIOG       LL_AHB4_GRP1_IsEnabledClock\n
  *         MC_AHB4ENSETR      GPIOH       LL_AHB4_GRP1_IsEnabledClock\n
  *         MC_AHB4ENSETR      GPIOI       LL_AHB4_GRP1_IsEnabledClock\n
  *         MC_AHB4ENSETR      GPIOJ       LL_AHB4_GRP1_IsEnabledClock\n
  *         MC_AHB4ENSETR      GPIOK       LL_AHB4_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOJ
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOK
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_AHB4_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_AHB4ENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable AHB4 peripherals clock.
  * @rmtoll MC_AHB4ENCLRR      GPIOA       LL_AHB4_GRP1_DisableClock\n
  *         MC_AHB4ENCLRR      GPIOB       LL_AHB4_GRP1_DisableClock\n
  *         MC_AHB4ENCLRR      GPIOC       LL_AHB4_GRP1_DisableClock\n
  *         MC_AHB4ENCLRR      GPIOD       LL_AHB4_GRP1_DisableClock\n
  *         MC_AHB4ENCLRR      GPIOE       LL_AHB4_GRP1_DisableClock\n
  *         MC_AHB4ENCLRR      GPIOF       LL_AHB4_GRP1_DisableClock\n
  *         MC_AHB4ENCLRR      GPIOG       LL_AHB4_GRP1_DisableClock\n
  *         MC_AHB4ENCLRR      GPIOH       LL_AHB4_GRP1_DisableClock\n
  *         MC_AHB4ENCLRR      GPIOI       LL_AHB4_GRP1_DisableClock\n
  *         MC_AHB4ENCLRR      GPIOJ       LL_AHB4_GRP1_DisableClock\n
  *         MC_AHB4ENCLRR      GPIOK       LL_AHB4_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOJ
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOK
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB4_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AHB4ENCLRR, Periphs);
}

/**
  * @brief  Force AHB4 peripherals reset.
  * @rmtoll AHB4RSTSETR     GPIOA       LL_AHB4_GRP1_ForceReset\n
  *         AHB4RSTSETR     GPIOB       LL_AHB4_GRP1_ForceReset\n
  *         AHB4RSTSETR     GPIOC       LL_AHB4_GRP1_ForceReset\n
  *         AHB4RSTSETR     GPIOD       LL_AHB4_GRP1_ForceReset\n
  *         AHB4RSTSETR     GPIOE       LL_AHB4_GRP1_ForceReset\n
  *         AHB4RSTSETR     GPIOF       LL_AHB4_GRP1_ForceReset\n
  *         AHB4RSTSETR     GPIOG       LL_AHB4_GRP1_ForceReset\n
  *         AHB4RSTSETR     GPIOH       LL_AHB4_GRP1_ForceReset\n
  *         AHB4RSTSETR     GPIOI       LL_AHB4_GRP1_ForceReset\n
  *         AHB4RSTSETR     GPIOJ       LL_AHB4_GRP1_ForceReset\n
  *         AHB4RSTSETR     GPIOK       LL_AHB4_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOJ
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOK
  * @retval None
  */
__STATIC_INLINE void LL_AHB4_GRP1_ForceReset(uint32_t Periphs)
{
  WRITE_REG(RCC->AHB4RSTSETR, Periphs);
}

/**
  * @brief  Release AHB4 peripherals reset.
  * @rmtoll AHB4RSTCLRR     GPIOA       LL_AHB4_GRP1_ReleaseReset\n
  *         AHB4RSTCLRR     GPIOB       LL_AHB4_GRP1_ReleaseReset\n
  *         AHB4RSTCLRR     GPIOC       LL_AHB4_GRP1_ReleaseReset\n
  *         AHB4RSTCLRR     GPIOD       LL_AHB4_GRP1_ReleaseReset\n
  *         AHB4RSTCLRR     GPIOE       LL_AHB4_GRP1_ReleaseReset\n
  *         AHB4RSTCLRR     GPIOF       LL_AHB4_GRP1_ReleaseReset\n
  *         AHB4RSTCLRR     GPIOG       LL_AHB4_GRP1_ReleaseReset\n
  *         AHB4RSTCLRR     GPIOH       LL_AHB4_GRP1_ReleaseReset\n
  *         AHB4RSTCLRR     GPIOI       LL_AHB4_GRP1_ReleaseReset\n
  *         AHB4RSTCLRR     GPIOJ       LL_AHB4_GRP1_ReleaseReset\n
  *         AHB4RSTCLRR     GPIOK       LL_AHB4_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOJ
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOK
  * @retval None
  */
__STATIC_INLINE void LL_AHB4_GRP1_ReleaseReset(uint32_t Periphs)
{
  WRITE_REG(RCC->AHB4RSTCLRR, Periphs);
}

/**
  * @brief  Enable AHB4 peripheral clocks in Sleep mode
  * @rmtoll MC_AHB4LPENSETR    GPIOA       LL_AHB4_GRP1_EnableClockSleep\n
  *         MC_AHB4LPENSETR    GPIOB       LL_AHB4_GRP1_EnableClockSleep\n
  *         MC_AHB4LPENSETR    GPIOC       LL_AHB4_GRP1_EnableClockSleep\n
  *         MC_AHB4LPENSETR    GPIOD       LL_AHB4_GRP1_EnableClockSleep\n
  *         MC_AHB4LPENSETR    GPIOE       LL_AHB4_GRP1_EnableClockSleep\n
  *         MC_AHB4LPENSETR    GPIOF       LL_AHB4_GRP1_EnableClockSleep\n
  *         MC_AHB4LPENSETR    GPIOG       LL_AHB4_GRP1_EnableClockSleep\n
  *         MC_AHB4LPENSETR    GPIOH       LL_AHB4_GRP1_EnableClockSleep\n
  *         MC_AHB4LPENSETR    GPIOI       LL_AHB4_GRP1_EnableClockSleep\n
  *         MC_AHB4LPENSETR    GPIOJ       LL_AHB4_GRP1_EnableClockSleep\n
  *         MC_AHB4LPENSETR    GPIOK       LL_AHB4_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOJ
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOK
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB4_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AHB4LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AHB4LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AHB4 peripheral clocks in Sleep modes
  * @rmtoll MC_AHB4LPENCLRR    GPIOA       LL_AHB4_GRP1_DisableClockSleep\n
  *         MC_AHB4LPENCLRR    GPIOB       LL_AHB4_GRP1_DisableClockSleep\n
  *         MC_AHB4LPENCLRR    GPIOC       LL_AHB4_GRP1_DisableClockSleep\n
  *         MC_AHB4LPENCLRR    GPIOD       LL_AHB4_GRP1_DisableClockSleep\n
  *         MC_AHB4LPENCLRR    GPIOE       LL_AHB4_GRP1_DisableClockSleep\n
  *         MC_AHB4LPENCLRR    GPIOF       LL_AHB4_GRP1_DisableClockSleep\n
  *         MC_AHB4LPENCLRR    GPIOG       LL_AHB4_GRP1_DisableClockSleep\n
  *         MC_AHB4LPENCLRR    GPIOH       LL_AHB4_GRP1_DisableClockSleep\n
  *         MC_AHB4LPENCLRR    GPIOI       LL_AHB4_GRP1_DisableClockSleep\n
  *         MC_AHB4LPENCLRR    GPIOJ       LL_AHB4_GRP1_DisableClockSleep\n
  *         MC_AHB4LPENCLRR    GPIOK       LL_AHB4_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOA
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOB
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOC
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOD
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOE
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOF
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOG
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOH
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOI
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOJ
  *         @arg @ref LL_AHB4_GRP1_PERIPH_GPIOK
  * @retval None
  */
__STATIC_INLINE void LL_AHB4_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AHB4LPENCLRR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_AHB5 AHB5
  * @{
  */

/**
  * @brief  Enable AHB5 peripherals clock.
  * @rmtoll MC_AHB5ENSETR      GPIOZ        LL_AHB5_GRP1_EnableClock\n
  *         MC_AHB5ENSETR      CRYP1        LL_AHB5_GRP1_EnableClock\n
  *         MC_AHB5ENSETR      HASH1        LL_AHB5_GRP1_EnableClock\n
  *         MC_AHB5ENSETR      RNG1         LL_AHB5_GRP1_EnableClock\n
  *         MC_AHB5ENSETR      BKPSRAM      LL_AHB5_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB5_GRP1_PERIPH_GPIOZ
  *         @arg @ref LL_AHB5_GRP1_PERIPH_CRYP1 (*)
  *         @arg @ref LL_AHB5_GRP1_PERIPH_HASH1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_RNG1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_BKPSRAM
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB5_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AHB5ENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AHB5ENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if AHB5 peripheral clock is enabled or not
  * @rmtoll MC_AHB5ENSETR      GPIOZ        LL_AHB5_GRP1_IsEnabledClock\n
  *         MC_AHB5ENSETR      CRYP1        LL_AHB5_GRP1_IsEnabledClock\n
  *         MC_AHB5ENSETR      HASH1        LL_AHB5_GRP1_IsEnabledClock\n
  *         MC_AHB5ENSETR      RNG1         LL_AHB5_GRP1_IsEnabledClock\n
  *         MC_AHB5ENSETR      BKPSRAM      LL_AHB5_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB5_GRP1_PERIPH_GPIOZ
  *         @arg @ref LL_AHB5_GRP1_PERIPH_CRYP1 (*)
  *         @arg @ref LL_AHB5_GRP1_PERIPH_HASH1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_RNG1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_BKPSRAM
  *
  *         (*) value not defined in all devices.
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_AHB5_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_AHB5ENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable AHB5 peripherals clock.
  * @rmtoll MC_AHB5ENCLRR      GPIOZ        LL_AHB5_GRP1_DisableClock\n
  *         MC_AHB5ENCLRR      CRYP1        LL_AHB5_GRP1_DisableClock\n
  *         MC_AHB5ENCLRR      HASH1        LL_AHB5_GRP1_DisableClock\n
  *         MC_AHB5ENCLRR      RNG1         LL_AHB5_GRP1_DisableClock\n
  *         MC_AHB5ENCLRR      BKPSRAM      LL_AHB5_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB5_GRP1_PERIPH_GPIOZ
  *         @arg @ref LL_AHB5_GRP1_PERIPH_CRYP1 (*)
  *         @arg @ref LL_AHB5_GRP1_PERIPH_HASH1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_RNG1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_BKPSRAM
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB5_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AHB5ENCLRR, Periphs);
}

/**
  * @brief  Force AHB5 peripherals reset.
  * @rmtoll AHB5RSTSETR     GPIOZ       LL_AHB5_GRP1_ForceReset\n
  *         AHB5RSTSETR     CRYP1       LL_AHB5_GRP1_ForceReset\n
  *         AHB5RSTSETR     HASH1       LL_AHB5_GRP1_ForceReset\n
  *         AHB5RSTSETR     RNG1        LL_AHB5_GRP1_ForceReset\n
  *         AHB5RSTSETR     AXIMC       LL_AHB5_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB5_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB5_GRP1_PERIPH_GPIOZ
  *         @arg @ref LL_AHB5_GRP1_PERIPH_CRYP1 (*)
  *         @arg @ref LL_AHB5_GRP1_PERIPH_HASH1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_RNG1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_AXIMC
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB5_GRP1_ForceReset(uint32_t Periphs)
{
  WRITE_REG(RCC->AHB5RSTSETR, Periphs);
}

/**
  * @brief  Release AHB5 peripherals reset.
  * @rmtoll AHB5RSTCLRR     GPIOZ       LL_AHB5_GRP1_ReleaseReset\n
  *         AHB5RSTCLRR     CRYP1       LL_AHB5_GRP1_ReleaseReset\n
  *         AHB5RSTCLRR     HASH1       LL_AHB5_GRP1_ReleaseReset\n
  *         AHB5RSTCLRR     RNG1        LL_AHB5_GRP1_ReleaseReset\n
  *         AHB5RSTCLRR     AXIMC       LL_AHB5_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB5_GRP1_PERIPH_ALL
  *         @arg @ref LL_AHB5_GRP1_PERIPH_GPIOZ
  *         @arg @ref LL_AHB5_GRP1_PERIPH_CRYP1 (*)
  *         @arg @ref LL_AHB5_GRP1_PERIPH_HASH1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_RNG1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_AXIMC
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB5_GRP1_ReleaseReset(uint32_t Periphs)
{
  WRITE_REG(RCC->AHB5RSTCLRR, Periphs);
}

/**
  * @brief  Enable AHB5 peripheral clocks in Sleep mode
  * @rmtoll MC_AHB5LPENSETR    GPIOZ        LL_AHB5_GRP1_EnableClockSleep\n
  *         MC_AHB5LPENSETR    CRYP1        LL_AHB5_GRP1_EnableClockSleep\n
  *         MC_AHB5LPENSETR    HASH1        LL_AHB5_GRP1_EnableClockSleep\n
  *         MC_AHB5LPENSETR    RNG1         LL_AHB5_GRP1_EnableClockSleep\n
  *         MC_AHB5LPENSETR    BKPSRAM      LL_AHB5_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB5_GRP1_PERIPH_GPIOZ
  *         @arg @ref LL_AHB5_GRP1_PERIPH_CRYP1 (*)
  *         @arg @ref LL_AHB5_GRP1_PERIPH_HASH1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_RNG1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_BKPSRAM
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB5_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AHB5LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AHB5LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AHB5 peripheral clocks in Sleep mode
  * @rmtoll MC_AHB5LPENCLRR    GPIOZ        LL_AHB5_GRP1_DisableClockSleep\n
  *         MC_AHB5LPENCLRR    CRYP1        LL_AHB5_GRP1_DisableClockSleep\n
  *         MC_AHB5LPENCLRR    HASH1        LL_AHB5_GRP1_DisableClockSleep\n
  *         MC_AHB5LPENCLRR    RNG1         LL_AHB5_GRP1_DisableClockSleep\n
  *         MC_AHB5LPENCLRR    BKPSRAM      LL_AHB5_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB5_GRP1_PERIPH_GPIOZ
  *         @arg @ref LL_AHB5_GRP1_PERIPH_CRYP1 (*)
  *         @arg @ref LL_AHB5_GRP1_PERIPH_HASH1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_RNG1
  *         @arg @ref LL_AHB5_GRP1_PERIPH_BKPSRAM
  *
  *         (*) value not defined in all devices.
  * @retval None
  */
__STATIC_INLINE void LL_AHB5_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AHB5LPENCLRR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_AHB6 AHB6
  * @{
  */

/**
  * @brief  Enable AHB6 peripherals clock.
  * @rmtoll MC_AHB6ENSETR      MDMA        LL_AHB6_GRP1_EnableClock\n
  *         MC_AHB6ENSETR      GPU         LL_AHB6_GRP1_EnableClock\n
  *         MC_AHB6ENSETR      ETH1CK      LL_AHB6_GRP1_EnableClock\n
  *         MC_AHB6ENSETR      ETH1TX      LL_AHB6_GRP1_EnableClock\n
  *         MC_AHB6ENSETR      ETH1RX      LL_AHB6_GRP1_EnableClock\n
  *         MC_AHB6ENSETR      ETH1MAC     LL_AHB6_GRP1_EnableClock\n
  *         MC_AHB6ENSETR      FMC         LL_AHB6_GRP1_EnableClock\n
  *         MC_AHB6ENSETR      QSPI        LL_AHB6_GRP1_EnableClock\n
  *         MC_AHB6ENSETR      SDMMC1      LL_AHB6_GRP1_EnableClock\n
  *         MC_AHB6ENSETR      SDMMC2      LL_AHB6_GRP1_EnableClock\n
  *         MC_AHB6ENSETR      CRC1        LL_AHB6_GRP1_EnableClock\n
  *         MC_AHB6ENSETR      USBH        LL_AHB6_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB6_GRP1_PERIPH_MDMA
  *         @arg @ref LL_AHB6_GRP1_PERIPH_GPU
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1CK
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1TX
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1RX
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1MAC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_FMC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_QSPI
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC2
  *         @arg @ref LL_AHB6_GRP1_PERIPH_CRC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_USBH
  * @retval None
  */
__STATIC_INLINE void LL_AHB6_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AHB6ENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AHB6ENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if AHB6 peripheral clock is enabled or not
  * @rmtoll MC_AHB6ENSETR      MDMA        LL_AHB6_GRP1_IsEnabledClock\n
  *         MC_AHB6ENSETR      GPU         LL_AHB6_GRP1_IsEnabledClock\n
  *         MC_AHB6ENSETR      ETH1CK      LL_AHB6_GRP1_IsEnabledClock\n
  *         MC_AHB6ENSETR      ETH1TX      LL_AHB6_GRP1_IsEnabledClock\n
  *         MC_AHB6ENSETR      ETH1RX      LL_AHB6_GRP1_IsEnabledClock\n
  *         MC_AHB6ENSETR      ETH1MAC     LL_AHB6_GRP1_IsEnabledClock\n
  *         MC_AHB6ENSETR      FMC         LL_AHB6_GRP1_IsEnabledClock\n
  *         MC_AHB6ENSETR      QSPI        LL_AHB6_GRP1_IsEnabledClock\n
  *         MC_AHB6ENSETR      SDMMC1      LL_AHB6_GRP1_IsEnabledClock\n
  *         MC_AHB6ENSETR      SDMMC2      LL_AHB6_GRP1_IsEnabledClock\n
  *         MC_AHB6ENSETR      CRC1        LL_AHB6_GRP1_IsEnabledClock\n
  *         MC_AHB6ENSETR      USBH        LL_AHB6_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB6_GRP1_PERIPH_MDMA
  *         @arg @ref LL_AHB6_GRP1_PERIPH_GPU
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1CK
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1TX
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1RX
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1MAC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_FMC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_QSPI
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC2
  *         @arg @ref LL_AHB6_GRP1_PERIPH_CRC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_USBH
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_AHB6_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_AHB6ENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable AHB6 peripherals clock.
  * @rmtoll MC_AHB6ENCLRR      MDMA         LL_AHB6_GRP1_DisableClock\n
  *         MC_AHB6ENCLRR      GPU          LL_AHB6_GRP1_DisableClock\n
  *         MC_AHB6ENCLRR      ETH1CK       LL_AHB6_GRP1_DisableClock\n
  *         MC_AHB6ENCLRR      ETH1TX       LL_AHB6_GRP1_DisableClock\n
  *         MC_AHB6ENCLRR      ETH1RX       LL_AHB6_GRP1_DisableClock\n
  *         MC_AHB6ENCLRR      ETH1MAC      LL_AHB6_GRP1_DisableClock\n
  *         MC_AHB6ENCLRR      FMC          LL_AHB6_GRP1_DisableClock\n
  *         MC_AHB6ENCLRR      QSPI         LL_AHB6_GRP1_DisableClock\n
  *         MC_AHB6ENCLRR      SDMMC1       LL_AHB6_GRP1_DisableClock\n
  *         MC_AHB6ENCLRR      SDMMC2       LL_AHB6_GRP1_DisableClock\n
  *         MC_AHB6ENCLRR      CRC1         LL_AHB6_GRP1_DisableClock\n
  *         MC_AHB6ENCLRR      USBH         LL_AHB6_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB6_GRP1_PERIPH_MDMA
  *         @arg @ref LL_AHB6_GRP1_PERIPH_GPU
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1CK
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1TX
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1RX
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1MAC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_FMC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_QSPI
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC2
  *         @arg @ref LL_AHB6_GRP1_PERIPH_CRC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_USBH
  * @retval None
  */
__STATIC_INLINE void LL_AHB6_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AHB6ENCLRR, Periphs);
}

/**
  * @brief  Force AHB6 peripherals reset.
  * @rmtoll AHB6RSTSETR     GPU         LL_AHB6_GRP1_ForceReset\n
  *         AHB6RSTSETR     ETH1MAC     LL_AHB6_GRP1_ForceReset\n
  *         AHB6RSTSETR     FMC         LL_AHB6_GRP1_ForceReset\n
  *         AHB6RSTSETR     QSPI        LL_AHB6_GRP1_ForceReset\n
  *         AHB6RSTSETR     SDMMC1      LL_AHB6_GRP1_ForceReset\n
  *         AHB6RSTSETR     SDMMC2      LL_AHB6_GRP1_ForceReset\n
  *         AHB6RSTSETR     CRC1        LL_AHB6_GRP1_ForceReset\n
  *         AHB6RSTSETR     USBH        LL_AHB6_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB6_GRP1_PERIPH_GPU
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1MAC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_FMC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_QSPI
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC2
  *         @arg @ref LL_AHB6_GRP1_PERIPH_CRC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_USBH
  * @retval None
  */
__STATIC_INLINE void LL_AHB6_GRP1_ForceReset(uint32_t Periphs)
{
  WRITE_REG(RCC->AHB6RSTSETR, Periphs);
}

/**
  * @brief  Release AHB6 peripherals reset.
  * @rmtoll AHB6RSTCLRR     ETH1MAC     LL_AHB6_GRP1_ReleaseReset\n
  *         AHB6RSTCLRR     FMC         LL_AHB6_GRP1_ReleaseReset\n
  *         AHB6RSTCLRR     QSPI        LL_AHB6_GRP1_ReleaseReset\n
  *         AHB6RSTCLRR     SDMMC1      LL_AHB6_GRP1_ReleaseReset\n
  *         AHB6RSTCLRR     SDMMC2      LL_AHB6_GRP1_ReleaseReset\n
  *         AHB6RSTCLRR     CRC1        LL_AHB6_GRP1_ReleaseReset\n
  *         AHB6RSTCLRR     USBH        LL_AHB6_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1MAC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_FMC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_QSPI
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC2
  *         @arg @ref LL_AHB6_GRP1_PERIPH_CRC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_USBH
  * @retval None
  */
__STATIC_INLINE void LL_AHB6_GRP1_ReleaseReset(uint32_t Periphs)
{
  WRITE_REG(RCC->AHB6RSTCLRR, Periphs);
}

/**
  * @brief  Enable AHB6 peripheral clocks in Sleep mode
  * @rmtoll MC_AHB6LPENSETR    MDMA         LL_AHB6_GRP1_EnableClockSleep\n
  *         MC_AHB6LPENSETR    GPU          LL_AHB6_GRP1_EnableClockSleep\n
  *         MC_AHB6LPENSETR    ETH1CK       LL_AHB6_GRP1_EnableClockSleep\n
  *         MC_AHB6LPENSETR    ETH1TX       LL_AHB6_GRP1_EnableClockSleep\n
  *         MC_AHB6LPENSETR    ETH1RX       LL_AHB6_GRP1_EnableClockSleep\n
  *         MC_AHB6LPENSETR    ETH1MAC      LL_AHB6_GRP1_EnableClockSleep\n
  *         MC_AHB6LPENSETR    FMC          LL_AHB6_GRP1_EnableClockSleep\n
  *         MC_AHB6LPENSETR    QSPI         LL_AHB6_GRP1_EnableClockSleep\n
  *         MC_AHB6LPENSETR    SDMMC1       LL_AHB6_GRP1_EnableClockSleep\n
  *         MC_AHB6LPENSETR    SDMMC2       LL_AHB6_GRP1_EnableClockSleep\n
  *         MC_AHB6LPENSETR    CRC1         LL_AHB6_GRP1_EnableClockSleep\n
  *         MC_AHB6LPENSETR    USBH         LL_AHB6_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB6_GRP1_PERIPH_MDMA
  *         @arg @ref LL_AHB6_GRP1_PERIPH_GPU
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1CK
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1TX
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1RX
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1MAC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_FMC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_QSPI
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC2
  *         @arg @ref LL_AHB6_GRP1_PERIPH_CRC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_USBH
  * @retval None
  */
__STATIC_INLINE void LL_AHB6_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AHB6LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AHB6LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AHB6 peripheral clocks in Sleep modes
  * @rmtoll MC_AHB6LPENCLRR    MDMA         LL_AHB6_GRP1_DisableClockSleep\n
  *         MC_AHB6LPENCLRR    GPU          LL_AHB6_GRP1_DisableClockSleep\n
  *         MC_AHB6LPENCLRR    ETH1CK       LL_AHB6_GRP1_DisableClockSleep\n
  *         MC_AHB6LPENCLRR    ETH1TX       LL_AHB6_GRP1_DisableClockSleep\n
  *         MC_AHB6LPENCLRR    ETH1RX       LL_AHB6_GRP1_DisableClockSleep\n
  *         MC_AHB6LPENCLRR    ETH1MAC      LL_AHB6_GRP1_DisableClockSleep\n
  *         MC_AHB6LPENCLRR    FMC          LL_AHB6_GRP1_DisableClockSleep\n
  *         MC_AHB6LPENCLRR    QSPI         LL_AHB6_GRP1_DisableClockSleep\n
  *         MC_AHB6LPENCLRR    SDMMC1       LL_AHB6_GRP1_DisableClockSleep\n
  *         MC_AHB6LPENCLRR    SDMMC2       LL_AHB6_GRP1_DisableClockSleep\n
  *         MC_AHB6LPENCLRR    CRC1         LL_AHB6_GRP1_DisableClockSleep\n
  *         MC_AHB6LPENCLRR    USBH         LL_AHB6_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB6_GRP1_PERIPH_MDMA
  *         @arg @ref LL_AHB6_GRP1_PERIPH_GPU
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1CK
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1TX
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1RX
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1MAC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_FMC
  *         @arg @ref LL_AHB6_GRP1_PERIPH_QSPI
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_SDMMC2
  *         @arg @ref LL_AHB6_GRP1_PERIPH_CRC1
  *         @arg @ref LL_AHB6_GRP1_PERIPH_USBH
  * @retval None
  */
__STATIC_INLINE void LL_AHB6_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AHB6LPENCLRR, Periphs);
}

/**
  * @brief  Enable AHB6 peripheral clocks in Stop mode
  * @rmtoll MC_AHB6LPENSETR    ETH1STP       LL_AHB6_GRP1_EnableClockStop
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1STP
  * @retval None
  */
__STATIC_INLINE void LL_AHB6_GRP1_EnableClockStop(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AHB6LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AHB6LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AHB6 peripheral clocks in Sleep modes
  * @rmtoll MC_AHB6LPENCLRR    ETH1STP       LL_AHB6_GRP1_DisableClockStop
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AHB6_GRP1_PERIPH_ETH1STP
  * @retval None
  */
__STATIC_INLINE void LL_AHB6_GRP1_DisableClockStop(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AHB6LPENCLRR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_AXI AXI
  * @{
  */

/**
  * @brief  Enable AXI peripherals clock.
  * @rmtoll MC_AXIMENSETR      SYSRAMEN      LL_AXI_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AXI_GRP1_PERIPH_SYSRAMEN
  * @retval None
  */
__STATIC_INLINE void LL_AXI_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AXIMENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AXIMENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if AXI peripheral clock is enabled or not
  * @rmtoll MC_AXIMENSETR      SYSRAMEN      LL_AXI_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AXI_GRP1_PERIPH_SYSRAMEN
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_AXI_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_AXIMENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable AXI peripherals clock.
  * @rmtoll MC_AXIMENCLRR      SYSRAMEN      LL_AXI_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AXI_GRP1_PERIPH_SYSRAMEN
  * @retval None
  */
__STATIC_INLINE void LL_AXI_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AXIMENCLRR, Periphs);
}

/**
  * @brief  Enable AXI peripheral clocks in Sleep mode
  * @rmtoll MC_AXIMLPENSETR    SYSRAMEN      LL_AXI_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AXI_GRP1_PERIPH_SYSRAMEN
  * @retval None
  */
__STATIC_INLINE void LL_AXI_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_AXIMLPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_AXIMLPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable AXI peripheral clocks in Sleep modes
  * @rmtoll MC_AXIMLPENCLRR    SYSRAMEN      LL_AXI_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_AXI_GRP1_PERIPH_SYSRAMEN
  * @retval None
  */
__STATIC_INLINE void LL_AXI_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_AXIMLPENCLRR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_MLAHB MLAHB
  * @{
  */

/**
  * @brief  Enable MLAHB peripherals clock.
  * @rmtoll MC_MLAHBENSETR      RETRAMEN        LL_MLAHB_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_MLAHB_GRP1_PERIPH_RETRAMEN
  * @retval None
  */
__STATIC_INLINE void LL_MLAHB_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_MLAHBENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_MLAHBENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if MLAHB peripheral clock is enabled or not
  * @rmtoll MC_MLAHBENSETR      RETRAMEN        LL_MLAHB_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_MLAHB_GRP1_PERIPH_RETRAMEN
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_MLAHB_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_MLAHBENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable MLAHB peripherals clock.
  * @rmtoll MC_MLAHBENCLRR      RETRAMEN         LL_MLAHB_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_MLAHB_GRP1_PERIPH_RETRAMEN
  * @retval None
  */
__STATIC_INLINE void LL_MLAHB_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_MLAHBENCLRR, Periphs);
}

/**
  * @brief  Enable MLAHB peripheral clocks in Sleep mode
  * @rmtoll MC_MLAHBLPENSETR    RETRAMEN         LL_MLAHB_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_MLAHB_GRP1_PERIPH_RETRAMEN
  * @retval None
  */
__STATIC_INLINE void LL_MLAHB_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_MLAHBLPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_MLAHBLPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable MLAHB peripheral clocks in Sleep modes
  * @rmtoll MC_MLAHBLPENCLRR    RETRAMEN         LL_MLAHB_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_MLAHB_GRP1_PERIPH_RETRAMEN
  * @retval None
  */
__STATIC_INLINE void LL_MLAHB_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_MLAHBLPENCLRR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_APB1 APB1
  * @{
  */

/**
  * @brief  Enable APB1 peripherals clock.
  * @rmtoll MC_APB1ENSETR      TIM2       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      TIM3       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      TIM4       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      TIM5       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      TIM6       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      TIM7       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      TIM12      LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      TIM13      LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      TIM14      LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      LPTIM1     LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      SPI2       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      SPI3       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      USART2     LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      USART3     LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      UART4      LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      UART5      LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      UART7      LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      UART8      LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      I2C1       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      I2C2       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      I2C3       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      I2C5       LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      SPDIF      LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      CEC        LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      WWDG1      LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      DAC12      LL_APB1_GRP1_EnableClock\n
  *         MC_APB1ENSETR      MDIOS      LL_APB1_GRP1_EnableClock
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
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART7
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART8
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C5
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPDIF
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG1
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC12
  *         @arg @ref LL_APB1_GRP1_PERIPH_MDIOS
  * @retval None
  */
__STATIC_INLINE void LL_APB1_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB1ENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB1ENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if APB1 peripheral clock is enabled or not
  * @rmtoll MC_APB1ENSETR      TIM2       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      TIM3       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      TIM4       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      TIM5       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      TIM6       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      TIM7       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      TIM12      LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      TIM13      LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      TIM14      LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      LPTIM1     LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      SPI2       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      SPI3       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      USART2     LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      USART3     LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      UART4      LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      UART5      LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      UART7      LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      UART8      LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      I2C1       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      I2C2       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      I2C3       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      I2C5       LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      SPDIF      LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      CEC        LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      WWDG1      LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      DAC12      LL_APB1_GRP1_IsEnabledClock\n
  *         MC_APB1ENSETR      MDIOS      LL_APB1_GRP1_IsEnabledClock
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
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART7
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART8
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C5
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPDIF
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG1
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC12
  *         @arg @ref LL_APB1_GRP1_PERIPH_MDIOS
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_APB1_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_APB1ENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable APB1 peripherals clock.
  * @rmtoll MC_APB1ENCLRR      TIM2       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      TIM3       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      TIM4       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      TIM5       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      TIM6       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      TIM7       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      TIM12      LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      TIM13      LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      TIM14      LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      LPTIM1     LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      SPI2       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      SPI3       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      USART2     LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      USART3     LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      UART4      LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      UART5      LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      UART7      LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      UART8      LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      I2C1       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      I2C2       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      I2C3       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      I2C5       LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      SPDIF      LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      CEC        LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      DAC12      LL_APB1_GRP1_DisableClock\n
  *         MC_APB1ENCLRR      MDIOS      LL_APB1_GRP1_DisableClock
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
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART7
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART8
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C5
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPDIF
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC12
  *         @arg @ref LL_APB1_GRP1_PERIPH_MDIOS
  * @retval None
  */
__STATIC_INLINE void LL_APB1_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB1ENCLRR, Periphs);
}

/**
  * @brief  Force APB1 peripherals reset.
  * @rmtoll APB1RSTSETR     TIM2       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     TIM3       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     TIM4       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     TIM5       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     TIM6       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     TIM7       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     TIM12      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     TIM13      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     TIM14      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     LPTIM1     LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     SPI2       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     SPI3       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     USART2     LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     USART3     LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     UART4      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     UART5      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     UART7      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     UART8      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     I2C1       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     I2C2       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     I2C3       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     I2C5       LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     SPDIF      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     CEC        LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     DAC12      LL_APB1_GRP1_ForceReset\n
  *         APB1RSTSETR     MCDIOS     LL_APB1_GRP1_ForceReset
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
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART7
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART8
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C5
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPDIF
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC12
  *         @arg @ref LL_APB1_GRP1_PERIPH_MDIOS
  * @retval None
  */
__STATIC_INLINE void LL_APB1_GRP1_ForceReset(uint32_t Periphs)
{
  WRITE_REG(RCC->APB1RSTSETR, Periphs);
}

/**
  * @brief  Release APB1 peripherals reset.
  * @rmtoll APB1RSTCLRR     TIM2       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     TIM3       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     TIM4       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     TIM5       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     TIM6       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     TIM7       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     TIM12      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     TIM13      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     TIM14      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     LPTIM1     LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     SPI2       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     SPI3       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     USART2     LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     USART3     LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     UART4      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     UART5      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     UART7      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     UART8      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     I2C1       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     I2C2       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     I2C3       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     I2C5       LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     SPDIF      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     CEC        LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     DAC12      LL_APB1_GRP1_ReleaseReset\n
  *         APB1RSTCLRR     MDIOS      LL_APB1_GRP1_ReleaseReset
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
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART7
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART8
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C5
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPDIF
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC12
  *         @arg @ref LL_APB1_GRP1_PERIPH_MDIOS
  * @retval None
  */
__STATIC_INLINE void LL_APB1_GRP1_ReleaseReset(uint32_t Periphs)
{
  WRITE_REG(RCC->APB1RSTCLRR, Periphs);
}

/**
  * @brief  Enable APB1 peripheral clocks in Sleep mode
  * @rmtoll MC_APB1LPENSETR    TIM2       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    TIM3       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    TIM4       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    TIM5       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    TIM6       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    TIM7       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    TIM12      LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    TIM13      LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    TIM14      LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    LPTIM1     LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    SPI2       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    SPI3       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    USART2     LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    USART3     LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    UART4      LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    UART5      LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    UART7      LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    UART8      LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    I2C1       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    I2C2       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    I2C3       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    I2C5       LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    SPDIF      LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    CEC        LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    WWDG1      LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    DAC12      LL_APB1_GRP1_EnableClockSleep\n
  *         MC_APB1LPENSETR    MDIOS      LL_APB1_GRP1_EnableClockSleep
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
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART7
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART8
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C5
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPDIF
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG1
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC12
  *         @arg @ref LL_APB1_GRP1_PERIPH_MDIOS
  * @retval None
  */
__STATIC_INLINE void LL_APB1_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB1LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB1LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB1 peripheral clocks in Sleep modes
  * @rmtoll MC_APB1LPENCLRR    TIM2       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    TIM3       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    TIM4       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    TIM5       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    TIM6       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    TIM7       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    TIM12      LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    TIM13      LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    TIM14      LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    LPTIM1     LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    SPI2       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    SPI3       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    USART2     LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    USART3     LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    UART4      LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    UART5      LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    UART7      LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    UART8      LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    I2C1       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    I2C2       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    I2C3       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    I2C5       LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    SPDIF      LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    CEC        LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    WWDG1      LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    DAC12      LL_APB1_GRP1_DisableClockSleep\n
  *         MC_APB1LPENCLRR    MDIOS      LL_APB1_GRP1_DisableClockSleep
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
  *         @arg @ref LL_APB1_GRP1_PERIPH_LPTIM1
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI2
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPI3
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART2
  *         @arg @ref LL_APB1_GRP1_PERIPH_USART3
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART4
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART5
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART7
  *         @arg @ref LL_APB1_GRP1_PERIPH_UART8
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C1
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C2
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C3
  *         @arg @ref LL_APB1_GRP1_PERIPH_I2C5
  *         @arg @ref LL_APB1_GRP1_PERIPH_SPDIF
  *         @arg @ref LL_APB1_GRP1_PERIPH_CEC
  *         @arg @ref LL_APB1_GRP1_PERIPH_WWDG1
  *         @arg @ref LL_APB1_GRP1_PERIPH_DAC12
  *         @arg @ref LL_APB1_GRP1_PERIPH_MDIOS
  * @retval None
  */
__STATIC_INLINE void LL_APB1_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB1LPENCLRR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_APB2 APB2
  * @{
  */

/**
  * @brief  Enable APB2 peripherals clock.
  * @rmtoll MC_APB2ENSETR      TIM1       LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      TIM8       LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      TIM15      LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      TIM16      LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      TIM17      LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      SPI1       LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      SPI4       LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      SPI5       LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      USART6     LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      SAI1       LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      SAI2       LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      SAI3       LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      DFSDM1     LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      ADFSDM1    LL_APB2_GRP1_EnableClock\n
  *         MC_APB2ENSETR      FDCAN      LL_APB2_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM15
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM16
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM17
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI4
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI5
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI2
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI3
  *         @arg @ref LL_APB2_GRP1_PERIPH_DFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_FDCAN
  * @retval None
  */
__STATIC_INLINE void LL_APB2_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB2ENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB2ENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if APB2 peripheral clock is enabled or not
  * @rmtoll MC_APB2ENSETR      TIM1       LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      TIM8       LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      TIM15      LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      TIM16      LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      TIM17      LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      SPI1       LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      SPI4       LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      SPI5       LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      USART6     LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      SAI1       LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      SAI2       LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      SAI3       LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      DFSDM1     LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      ADFSDM1    LL_APB2_GRP1_IsEnabledClock\n
  *         MC_APB2ENSETR      FDCAN      LL_APB2_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM15
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM16
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM17
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI4
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI5
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI2
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI3
  *         @arg @ref LL_APB2_GRP1_PERIPH_DFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_FDCAN
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_APB2_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_APB2ENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable APB2 peripherals clock.
  * @rmtoll MC_APB2ENCLRR      TIM1       LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      TIM8       LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      TIM15      LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      TIM16      LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      TIM17      LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      SPI1       LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      SPI4       LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      SPI5       LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      USART6     LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      SAI1       LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      SAI2       LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      SAI3       LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      DFSDM1     LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      ADFSDM1    LL_APB2_GRP1_DisableClock\n
  *         MC_APB2ENCLRR      FDCAN      LL_APB2_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM15
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM16
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM17
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI4
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI5
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI2
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI3
  *         @arg @ref LL_APB2_GRP1_PERIPH_DFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_FDCAN
  * @retval None
  */
__STATIC_INLINE void LL_APB2_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB2ENCLRR, Periphs);
}

/**
  * @brief  Force APB2 peripherals reset.
  * @rmtoll APB2RSTSETR     TIM1       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     TIM8       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     TIM15      LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     TIM16      LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     TIM17      LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     SPI1       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     SPI4       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     SPI5       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     USART6     LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     SAI1       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     SAI2       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     SAI3       LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     DFSDM1     LL_APB2_GRP1_ForceReset\n
  *         APB2RSTSETR     FDCAN      LL_APB2_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM15
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM16
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM17
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI4
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI5
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI2
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI3
  *         @arg @ref LL_APB2_GRP1_PERIPH_DFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_FDCAN
  * @retval None
  */
__STATIC_INLINE void LL_APB2_GRP1_ForceReset(uint32_t Periphs)
{
  WRITE_REG(RCC->APB2RSTSETR, Periphs);
}

/**
  * @brief  Release APB2 peripherals reset.
  * @rmtoll APB2RSTCLRR     TIM1       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     TIM8       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     TIM15      LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     TIM16      LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     TIM17      LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     SPI1       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     SPI4       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     SPI5       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     USART6     LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     SAI1       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     SAI2       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     SAI3       LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     DFSDM1     LL_APB2_GRP1_ReleaseReset\n
  *         APB2RSTCLRR     FDCAN      LL_APB2_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM15
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM16
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM17
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI4
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI5
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI2
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI3
  *         @arg @ref LL_APB2_GRP1_PERIPH_DFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_FDCAN
  * @retval None
  */
__STATIC_INLINE void LL_APB2_GRP1_ReleaseReset(uint32_t Periphs)
{
  WRITE_REG(RCC->APB2RSTCLRR, Periphs);
}

/**
  * @brief  Enable APB2 peripheral clocks in Sleep mode
  * @rmtoll MC_APB2LPENSETR    TIM1       LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    TIM8       LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    TIM15      LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    TIM16      LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    TIM17      LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    SPI1       LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    SPI4       LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    SPI5       LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    USART6     LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    SAI1       LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    SAI2       LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    SAI3       LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    DFSDM1     LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    ADFSDM1    LL_APB2_GRP1_EnableClockSleep\n
  *         MC_APB2LPENSETR    FDCAN      LL_APB2_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM15
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM16
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM17
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI4
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI5
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI2
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI3
  *         @arg @ref LL_APB2_GRP1_PERIPH_DFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_FDCAN
  * @retval None
  */
__STATIC_INLINE void LL_APB2_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB2LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB2LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB2 peripheral clocks in Sleep modes
  * @rmtoll MC_APB2LPENCLRR    TIM1       LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    TIM8       LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    TIM15      LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    TIM16      LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    TIM17      LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    SPI1       LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    SPI4       LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    SPI5       LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    USART6     LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    SAI1       LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    SAI2       LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    SAI3       LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    DFSDM1     LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    ADFSDM1    LL_APB2_GRP1_DisableClockSleep\n
  *         MC_APB2LPENCLRR    FDCAN      LL_APB2_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM8
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM15
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM16
  *         @arg @ref LL_APB2_GRP1_PERIPH_TIM17
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI4
  *         @arg @ref LL_APB2_GRP1_PERIPH_SPI5
  *         @arg @ref LL_APB2_GRP1_PERIPH_USART6
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI1
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI2
  *         @arg @ref LL_APB2_GRP1_PERIPH_SAI3
  *         @arg @ref LL_APB2_GRP1_PERIPH_DFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_ADFSDM1
  *         @arg @ref LL_APB2_GRP1_PERIPH_FDCAN
  * @retval None
  */
__STATIC_INLINE void LL_APB2_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB2LPENCLRR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_APB3 APB3
  * @{
  */

/**
  * @brief  Enable APB3 peripherals clock.
  * @rmtoll MC_APB3ENSETR      LPTIM2     LL_APB3_GRP1_EnableClock\n
  *         MC_APB3ENSETR      LPTIM3     LL_APB3_GRP1_EnableClock\n
  *         MC_APB3ENSETR      LPTIM4     LL_APB3_GRP1_EnableClock\n
  *         MC_APB3ENSETR      LPTIM5     LL_APB3_GRP1_EnableClock\n
  *         MC_APB3ENSETR      SAI4       LL_APB3_GRP1_EnableClock\n
  *         MC_APB3ENSETR      SYSCFG     LL_APB3_GRP1_EnableClock\n
  *         MC_APB3ENSETR      VREF       LL_APB3_GRP1_EnableClock\n
  *         MC_APB3ENSETR      TMPSENS    LL_APB3_GRP1_EnableClock\n
  *         MC_APB3ENSETR      PMBCTRL    LL_APB3_GRP1_EnableClock\n
  *         MC_APB3ENSETR      HDP        LL_APB3_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM2
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM3
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM4
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM5
  *         @arg @ref LL_APB3_GRP1_PERIPH_SAI4
  *         @arg @ref LL_APB3_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB3_GRP1_PERIPH_VREF
  *         @arg @ref LL_APB3_GRP1_PERIPH_TMPSENS
  *         @arg @ref LL_APB3_GRP1_PERIPH_PMBCTRL
  *         @arg @ref LL_APB3_GRP1_PERIPH_HDP
  * @retval None
  */
__STATIC_INLINE void LL_APB3_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB3ENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB3ENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if APB3 peripheral clock is enabled or not
  * @rmtoll MC_APB3ENSETR      LPTIM2      LL_APB3_GRP1_IsEnabledClock\n
  *         MC_APB3ENSETR      LPTIM3      LL_APB3_GRP1_IsEnabledClock\n
  *         MC_APB3ENSETR      LPTIM4      LL_APB3_GRP1_IsEnabledClock\n
  *         MC_APB3ENSETR      LPTIM5      LL_APB3_GRP1_IsEnabledClock\n
  *         MC_APB3ENSETR      SAI4        LL_APB3_GRP1_IsEnabledClock\n
  *         MC_APB3ENSETR      SYSCFG      LL_APB3_GRP1_IsEnabledClock\n
  *         MC_APB3ENSETR      VREF        LL_APB3_GRP1_IsEnabledClock\n
  *         MC_APB3ENSETR      TMPSENS     LL_APB3_GRP1_IsEnabledClock\n
  *         MC_APB3ENSETR      PMBCTRL     LL_APB3_GRP1_IsEnabledClock\n
  *         MC_APB3ENSETR      HDP         LL_APB3_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM2
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM3
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM4
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM5
  *         @arg @ref LL_APB3_GRP1_PERIPH_SAI4
  *         @arg @ref LL_APB3_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB3_GRP1_PERIPH_VREF
  *         @arg @ref LL_APB3_GRP1_PERIPH_TMPSENS
  *         @arg @ref LL_APB3_GRP1_PERIPH_PMBCTRL
  *         @arg @ref LL_APB3_GRP1_PERIPH_HDP
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_APB3_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_APB3ENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable APB3 peripherals clock.
  * @rmtoll MC_APB3ENCLRR      LPTIM2      LL_APB3_GRP1_DisableClock\n
  *         MC_APB3ENCLRR      LPTIM3      LL_APB3_GRP1_DisableClock\n
  *         MC_APB3ENCLRR      LPTIM4      LL_APB3_GRP1_DisableClock\n
  *         MC_APB3ENCLRR      LPTIM5      LL_APB3_GRP1_DisableClock
  *         MC_APB3ENCLRR      SAI4        LL_APB3_GRP1_DisableClock
  *         MC_APB3ENCLRR      SYSCFG      LL_APB3_GRP1_DisableClock
  *         MC_APB3ENCLRR      VREF        LL_APB3_GRP1_DisableClock
  *         MC_APB3ENCLRR      TMPSENS     LL_APB3_GRP1_DisableClock
  *         MC_APB3ENCLRR      PMBCTRL     LL_APB3_GRP1_DisableClock
  *         MC_APB3ENCLRR      HDP         LL_APB3_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM2
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM3
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM4
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM5
  *         @arg @ref LL_APB3_GRP1_PERIPH_SAI4
  *         @arg @ref LL_APB3_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB3_GRP1_PERIPH_VREF
  *         @arg @ref LL_APB3_GRP1_PERIPH_TMPSENS
  *         @arg @ref LL_APB3_GRP1_PERIPH_PMBCTRL
  *         @arg @ref LL_APB3_GRP1_PERIPH_HDP
  * @retval None
  */
__STATIC_INLINE void LL_APB3_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB3ENCLRR, Periphs);
}

/**
  * @brief  Force APB3 peripherals reset.
  * @rmtoll APB3RSTSETR     LPTIM2      LL_APB3_GRP1_ForceReset\n
  *         APB3RSTSETR     LPTIM3      LL_APB3_GRP1_ForceReset\n
  *         APB3RSTSETR     LPTIM4      LL_APB3_GRP1_ForceReset\n
  *         APB3RSTSETR     LPTIM5      LL_APB3_GRP1_ForceReset\n
  *         APB3RSTSETR     SAI4        LL_APB3_GRP1_ForceReset\n
  *         APB3RSTSETR     SYSCFG      LL_APB3_GRP1_ForceReset\n
  *         APB3RSTSETR     VREF        LL_APB3_GRP1_ForceReset\n
  *         APB3RSTSETR     TMPSENS     LL_APB3_GRP1_ForceReset\n
  *         APB3RSTSETR     PMBCTRL     LL_APB3_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB3_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM2
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM3
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM4
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM5
  *         @arg @ref LL_APB3_GRP1_PERIPH_SAI4
  *         @arg @ref LL_APB3_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB3_GRP1_PERIPH_VREF
  *         @arg @ref LL_APB3_GRP1_PERIPH_TMPSENS
  *         @arg @ref LL_APB3_GRP1_PERIPH_PMBCTRL
  * @retval None
  */
__STATIC_INLINE void LL_APB3_GRP1_ForceReset(uint32_t Periphs)
{
  WRITE_REG(RCC->APB3RSTSETR, Periphs);
}

/**
  * @brief  Release APB3 peripherals reset.
  * @rmtoll APB3RSTCLRR     LPTIM2       LL_APB3_GRP1_ReleaseReset\n
  *         APB3RSTCLRR     LPTIM3       LL_APB3_GRP1_ReleaseReset\n
  *         APB3RSTCLRR     LPTIM4       LL_APB3_GRP1_ReleaseReset\n
  *         APB3RSTCLRR     LPTIM5       LL_APB3_GRP1_ReleaseReset\n
  *         APB3RSTCLRR     SAI4         LL_APB3_GRP1_ReleaseReset\n
  *         APB3RSTCLRR     SYSCFG       LL_APB3_GRP1_ReleaseReset\n
  *         APB3RSTCLRR     VREF         LL_APB3_GRP1_ReleaseReset\n
  *         APB3RSTCLRR     TMPSENS      LL_APB3_GRP1_ReleaseReset\n
  *         APB3RSTCLRR     PMBCTRL      LL_APB3_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB3_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM2
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM3
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM4
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM5
  *         @arg @ref LL_APB3_GRP1_PERIPH_SAI4
  *         @arg @ref LL_APB3_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB3_GRP1_PERIPH_VREF
  *         @arg @ref LL_APB3_GRP1_PERIPH_TMPSENS
  *         @arg @ref LL_APB3_GRP1_PERIPH_PMBCTRL
  * @retval None
  */
__STATIC_INLINE void LL_APB3_GRP1_ReleaseReset(uint32_t Periphs)
{
  WRITE_REG(RCC->APB3RSTCLRR, Periphs);
}

/**
  * @brief  Enable APB3 peripheral clocks in Sleep mode
  * @rmtoll MC_APB3LPENSETR    LPTIM2       LL_APB3_GRP1_EnableClockSleep\n
  *         MC_APB3LPENSETR    LPTIM3       LL_APB3_GRP1_EnableClockSleep\n
  *         MC_APB3LPENSETR    LPTIM4       LL_APB3_GRP1_EnableClockSleep\n
  *         MC_APB3LPENSETR    LPTIM5       LL_APB3_GRP1_EnableClockSleep\n
  *         MC_APB3LPENSETR    SAI4         LL_APB3_GRP1_EnableClockSleep\n
  *         MC_APB3LPENSETR    LPTIM5       LL_APB3_GRP1_EnableClockSleep\n
  *         MC_APB3LPENSETR    SYSCFG       LL_APB3_GRP1_EnableClockSleep\n
  *         MC_APB3LPENSETR    VREF         LL_APB3_GRP1_EnableClockSleep\n
  *         MC_APB3LPENSETR    TMPSENS      LL_APB3_GRP1_EnableClockSleep\n
  *         MC_APB3LPENSETR    PMBCTRL      LL_APB3_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM2
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM3
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM4
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM5
  *         @arg @ref LL_APB3_GRP1_PERIPH_SAI4
  *         @arg @ref LL_APB3_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB3_GRP1_PERIPH_VREF
  *         @arg @ref LL_APB3_GRP1_PERIPH_TMPSENS
  *         @arg @ref LL_APB3_GRP1_PERIPH_PMBCTRL
  * @retval None
  */
__STATIC_INLINE void LL_APB3_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB3LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB3LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB3 peripheral clocks in Sleep modes
  * @rmtoll MC_APB3LPENCLRR    LPTIM2      LL_APB3_GRP1_DisableClockSleep\n
  *         MC_APB3LPENCLRR    LPTIM3      LL_APB3_GRP1_DisableClockSleep\n
  *         MC_APB3LPENCLRR    LPTIM4      LL_APB3_GRP1_DisableClockSleep\n
  *         MC_APB3LPENCLRR    LPTIM5      LL_APB3_GRP1_DisableClockSleep\n
  *         MC_APB3LPENCLRR    SAI4        LL_APB3_GRP1_DisableClockSleep\n
  *         MC_APB3LPENCLRR    SYSCFG      LL_APB3_GRP1_DisableClockSleep\n
  *         MC_APB3LPENCLRR    VREF        LL_APB3_GRP1_DisableClockSleep\n
  *         MC_APB3LPENCLRR    TMPSENS     LL_APB3_GRP1_DisableClockSleep\n
  *         MC_APB3LPENCLRR    PMBCTRL     LL_APB3_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM2
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM3
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM4
  *         @arg @ref LL_APB3_GRP1_PERIPH_LPTIM5
  *         @arg @ref LL_APB3_GRP1_PERIPH_SAI4
  *         @arg @ref LL_APB3_GRP1_PERIPH_SYSCFG
  *         @arg @ref LL_APB3_GRP1_PERIPH_VREF
  *         @arg @ref LL_APB3_GRP1_PERIPH_TMPSENS
  *         @arg @ref LL_APB3_GRP1_PERIPH_PMBCTRL
  * @retval None
  */
__STATIC_INLINE void LL_APB3_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB3LPENCLRR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_APB4 APB4
  * @{
  */

/**
  * @brief  Enable APB4 peripherals clock.
  * @rmtoll MC_APB4ENSETR      LTDC         LL_APB4_GRP1_EnableClock\n
  *         MC_APB4ENSETR      DSI          LL_APB4_GRP1_EnableClock\n
  *         MC_APB4ENSETR      DDRPERFM     LL_APB4_GRP1_EnableClock\n
  *         MC_APB4ENSETR      USBPHY       LL_APB4_GRP1_EnableClock\n
  *         MC_APB4ENSETR      STGENRO      LL_APB4_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB4_GRP1_PERIPH_LTDC
  *         @arg @ref LL_APB4_GRP1_PERIPH_DSI
  *         @arg @ref LL_APB4_GRP1_PERIPH_DDRPERFM
  *         @arg @ref LL_APB4_GRP1_PERIPH_USBPHY
  *         @arg @ref LL_APB4_GRP1_PERIPH_STGENRO
  * @retval None
  */
__STATIC_INLINE void LL_APB4_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB4ENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB4ENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if APB4 peripheral clock is enabled or not
  * @rmtoll MC_APB4ENSETR      LTDC          LL_APB4_GRP1_IsEnabledClock\n
  *         MC_APB4ENSETR      DSI           LL_APB4_GRP1_IsEnabledClock\n
  *         MC_APB4ENSETR      DDRPERFM      LL_APB4_GRP1_IsEnabledClock\n
  *         MC_APB4ENSETR      USBPHY        LL_APB4_GRP1_IsEnabledClock\n
  *         MC_APB4ENSETR      STGENRO       LL_APB4_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB4_GRP1_PERIPH_LTDC
  *         @arg @ref LL_APB4_GRP1_PERIPH_DSI
  *         @arg @ref LL_APB4_GRP1_PERIPH_DDRPERFM
  *         @arg @ref LL_APB4_GRP1_PERIPH_USBPHY
  *         @arg @ref LL_APB4_GRP1_PERIPH_STGENRO
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_APB4_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_APB4ENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable APB4 peripherals clock.
  * @rmtoll MC_APB4ENCLRR      LTDC          LL_APB4_GRP1_DisableClock\n
  *         MC_APB4ENCLRR      DSI           LL_APB4_GRP1_DisableClock\n
  *         MC_APB4ENCLRR      DDRPERFM      LL_APB4_GRP1_DisableClock\n
  *         MC_APB4ENCLRR      USBPHY        LL_APB4_GRP1_DisableClock\n
  *         MC_APB4ENCLRR      STGENRO       LL_APB4_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB4_GRP1_PERIPH_LTDC
  *         @arg @ref LL_APB4_GRP1_PERIPH_DSI
  *         @arg @ref LL_APB4_GRP1_PERIPH_DDRPERFM
  *         @arg @ref LL_APB4_GRP1_PERIPH_USBPHY
  *         @arg @ref LL_APB4_GRP1_PERIPH_STGENRO
  * @retval None
  */
__STATIC_INLINE void LL_APB4_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB4ENCLRR, Periphs);
}

/**
  * @brief  Force APB4 peripherals reset.
  * @rmtoll APB4RSTSETR     LTDC          LL_APB4_GRP1_ForceReset\n
  *         APB4RSTSETR     DSI           LL_APB4_GRP1_ForceReset\n
  *         APB4RSTSETR     DDRPERFM      LL_APB4_GRP1_ForceReset\n
  *         APB4RSTSETR     USBPHY        LL_APB4_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB4_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB4_GRP1_PERIPH_LTDC
  *         @arg @ref LL_APB4_GRP1_PERIPH_DSI
  *         @arg @ref LL_APB4_GRP1_PERIPH_DDRPERFM
  *         @arg @ref LL_APB4_GRP1_PERIPH_USBPHY
  * @retval None
  */
__STATIC_INLINE void LL_APB4_GRP1_ForceReset(uint32_t Periphs)
{
  WRITE_REG(RCC->APB4RSTSETR, Periphs);
}

/**
  * @brief  Release APB4 peripherals reset.
  * @rmtoll APB4RSTCLRR     LTDC           LL_APB4_GRP1_ReleaseReset\n
  *         APB4RSTCLRR     DSI            LL_APB4_GRP1_ReleaseReset\n
  *         APB4RSTCLRR     DDRPERFM       LL_APB4_GRP1_ReleaseReset\n
  *         APB4RSTCLRR     USBPHY         LL_APB4_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB4_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB4_GRP1_PERIPH_LTDC
  *         @arg @ref LL_APB4_GRP1_PERIPH_DSI
  *         @arg @ref LL_APB4_GRP1_PERIPH_DDRPERFM
  *         @arg @ref LL_APB4_GRP1_PERIPH_USBPHY
  * @retval None
  */
__STATIC_INLINE void LL_APB4_GRP1_ReleaseReset(uint32_t Periphs)
{
  WRITE_REG(RCC->APB4RSTCLRR, Periphs);
}

/**
  * @brief  Enable APB4 peripheral clocks in Sleep mode
  * @rmtoll MC_APB4LPENSETR    LTDC           LL_APB4_GRP1_EnableClockSleep\n
  *         MC_APB4LPENSETR    DSI            LL_APB4_GRP1_EnableClockSleep\n
  *         MC_APB4LPENSETR    DDRPERFM       LL_APB4_GRP1_EnableClockSleep\n
  *         MC_APB4LPENSETR    USBPHY         LL_APB4_GRP1_EnableClockSleep\n
  *         MC_APB4LPENSETR    STGENRO        LL_APB4_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB4_GRP1_PERIPH_LTDC
  *         @arg @ref LL_APB4_GRP1_PERIPH_DSI
  *         @arg @ref LL_APB4_GRP1_PERIPH_DDRPERFM
  *         @arg @ref LL_APB4_GRP1_PERIPH_USBPHY
  *         @arg @ref LL_APB4_GRP1_PERIPH_STGENRO
  * @retval None
  */
__STATIC_INLINE void LL_APB4_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB4LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB4LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB4 peripheral clocks in Sleep modes
  * @rmtoll MC_APB4LPENCLRR    LTDC          LL_APB4_GRP1_DisableClockSleep\n
  *         MC_APB4LPENCLRR    DSI           LL_APB4_GRP1_DisableClockSleep\n
  *         MC_APB4LPENCLRR    DDRPERFM      LL_APB4_GRP1_DisableClockSleep\n
  *         MC_APB4LPENCLRR    USBPHY        LL_APB4_GRP1_DisableClockSleep\n
  *         MC_APB4LPENCLRR    STGENRO       LL_APB4_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB4_GRP1_PERIPH_LTDC
  *         @arg @ref LL_APB4_GRP1_PERIPH_DSI
  *         @arg @ref LL_APB4_GRP1_PERIPH_DDRPERFM
  *         @arg @ref LL_APB4_GRP1_PERIPH_USBPHY
  *         @arg @ref LL_APB4_GRP1_PERIPH_STGENRO
  * @retval None
  */
__STATIC_INLINE void LL_APB4_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB4LPENCLRR, Periphs);
}

/**
  * @brief  Enable APB4 peripheral clocks in Stop mode
  * @rmtoll MC_APB4LPENSETR    STGENROSTPEN           LL_APB4_GRP1_EnableClockStop
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB4_GRP1_PERIPH_STGENROSTP
  * @retval None
  */
__STATIC_INLINE void LL_APB4_GRP1_EnableClockStop(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB4LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB4LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB4 peripheral clocks in Stop modes
  * @rmtoll MC_APB4LPENCLRR    STGENROSTPEN          LL_APB4_GRP1_DisableClockStop\n
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB4_GRP1_PERIPH_STGENROSTP
  * @retval None
  */
__STATIC_INLINE void LL_APB4_GRP1_DisableClockStop(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB4LPENCLRR, Periphs);
}

/**
  * @}
  */

/** @defgroup BUS_LL_EF_APB5 APB5
  * @{
  */

/**
  * @brief  Enable APB5 peripherals clock.
  * @rmtoll MC_APB5ENSETR      SPI6          LL_APB5_GRP1_EnableClock\n
  *         MC_APB5ENSETR      I2C4          LL_APB5_GRP1_EnableClock\n
  *         MC_APB5ENSETR      I2C6          LL_APB5_GRP1_EnableClock\n
  *         MC_APB5ENSETR      USART1        LL_APB5_GRP1_EnableClock\n
  *         MC_APB5ENSETR      RTCAPB        LL_APB5_GRP1_EnableClock\n
  *         MC_APB5ENSETR      TZC1          LL_APB5_GRP1_EnableClock\n
  *         MC_APB5ENSETR      TZC2          LL_APB5_GRP1_EnableClock\n
  *         MC_APB5ENSETR      TZPC          LL_APB5_GRP1_EnableClock\n
  *         MC_APB5ENSETR      BSEC          LL_APB5_GRP1_EnableClock\n
  *         MC_APB5ENSETR      STGEN         LL_APB5_GRP1_EnableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB5_GRP1_PERIPH_SPI6
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C4
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C6
  *         @arg @ref LL_APB5_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB5_GRP1_PERIPH_RTCAPB
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZC1
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZC2
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZPC
  *         @arg @ref LL_APB5_GRP1_PERIPH_BSEC
  *         @arg @ref LL_APB5_GRP1_PERIPH_STGEN
  * @retval None
  */
__STATIC_INLINE void LL_APB5_GRP1_EnableClock(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB5ENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB5ENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Check if APB5 peripheral clock is enabled or not
  * @rmtoll MC_APB5ENSETR      SPI6          LL_APB5_GRP1_IsEnabledClock\n
  *         MC_APB5ENSETR      I2C4          LL_APB5_GRP1_IsEnabledClock\n
  *         MC_APB5ENSETR      I2C6          LL_APB5_GRP1_IsEnabledClock\n
  *         MC_APB5ENSETR      USART1        LL_APB5_GRP1_IsEnabledClock\n
  *         MC_APB5ENSETR      RTCAPB        LL_APB5_GRP1_IsEnabledClock\n
  *         MC_APB5ENSETR      TZC1          LL_APB5_GRP1_IsEnabledClock\n
  *         MC_APB5ENSETR      TZC2          LL_APB5_GRP1_IsEnabledClock\n
  *         MC_APB5ENSETR      TZPC          LL_APB5_GRP1_IsEnabledClock\n
  *         MC_APB5ENSETR      BSEC          LL_APB5_GRP1_IsEnabledClock\n
  *         MC_APB5ENSETR      STGEN         LL_APB5_GRP1_IsEnabledClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB5_GRP1_PERIPH_SPI6
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C4
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C6
  *         @arg @ref LL_APB5_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB5_GRP1_PERIPH_RTCAPB
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZC1
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZC2
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZPC
  *         @arg @ref LL_APB5_GRP1_PERIPH_BSEC
  *         @arg @ref LL_APB5_GRP1_PERIPH_STGEN
  * @retval State of Periphs (1 or 0).
  */
__STATIC_INLINE uint32_t LL_APB5_GRP1_IsEnabledClock(uint32_t Periphs)
{
  return (READ_BIT(RCC->MC_APB5ENSETR, Periphs) == Periphs);
}

/**
  * @brief  Disable APB5 peripherals clock.
  * @rmtoll MC_APB5ENCLRR      SPI6         LL_APB5_GRP1_DisableClock\n
  *         MC_APB5ENCLRR      I2C4         LL_APB5_GRP1_DisableClock\n
  *         MC_APB5ENCLRR      I2C6         LL_APB5_GRP1_DisableClock\n
  *         MC_APB5ENCLRR      USART1       LL_APB5_GRP1_DisableClock\n
  *         MC_APB5ENCLRR      RTCAPB       LL_APB5_GRP1_DisableClock\n
  *         MC_APB5ENCLRR      TZC1         LL_APB5_GRP1_DisableClock\n
  *         MC_APB5ENCLRR      TZC2         LL_APB5_GRP1_DisableClock\n
  *         MC_APB5ENCLRR      TZPC         LL_APB5_GRP1_DisableClock\n
  *         MC_APB5ENCLRR      BSEC         LL_APB5_GRP1_DisableClock\n
  *         MC_APB5ENCLRR      STGEN        LL_APB5_GRP1_DisableClock
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB5_GRP1_PERIPH_SPI6
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C4
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C6
  *         @arg @ref LL_APB5_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB5_GRP1_PERIPH_RTCAPB
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZC1
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZC2
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZPC
  *         @arg @ref LL_APB5_GRP1_PERIPH_BSEC
  *         @arg @ref LL_APB5_GRP1_PERIPH_STGEN
  * @retval None
  */
__STATIC_INLINE void LL_APB5_GRP1_DisableClock(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB5ENCLRR, Periphs);
}

/**
  * @brief  Force APB5 peripherals reset.
  * @rmtoll APB5RSTSETR     SPI6          LL_APB5_GRP1_ForceReset\n
  *         APB5RSTSETR     I2C4          LL_APB5_GRP1_ForceReset\n
  *         APB5RSTSETR     I2C6          LL_APB5_GRP1_ForceReset\n
  *         APB5RSTSETR     USART1        LL_APB5_GRP1_ForceReset\n
  *         APB5RSTSETR     STGEN         LL_APB5_GRP1_ForceReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB5_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB5_GRP1_PERIPH_SPI6
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C4
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C6
  *         @arg @ref LL_APB5_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB5_GRP1_PERIPH_STGEN
  * @retval None
  */
__STATIC_INLINE void LL_APB5_GRP1_ForceReset(uint32_t Periphs)
{
  WRITE_REG(RCC->APB5RSTSETR, Periphs);
}

/**
  * @brief  Release APB5 peripherals reset.
  * @rmtoll APB5RSTCLRR     SPI6          LL_APB5_GRP1_ReleaseReset\n
  *         APB5RSTCLRR     I2C4          LL_APB5_GRP1_ReleaseReset\n
  *         APB5RSTCLRR     I2C6          LL_APB5_GRP1_ReleaseReset\n
  *         APB5RSTCLRR     USART1        LL_APB5_GRP1_ReleaseReset\n
  *         APB5RSTCLRR     STGEN         LL_APB5_GRP1_ReleaseReset
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB5_GRP1_PERIPH_ALL
  *         @arg @ref LL_APB5_GRP1_PERIPH_SPI6
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C4
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C6
  *         @arg @ref LL_APB5_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB5_GRP1_PERIPH_STGEN
  * @retval None
  */
__STATIC_INLINE void LL_APB5_GRP1_ReleaseReset(uint32_t Periphs)
{
  WRITE_REG(RCC->APB5RSTCLRR, Periphs);
}

/**
  * @brief  Enable APB5 peripheral clocks in Sleep mode
  * @rmtoll MC_APB5LPENSETR    SPI6         LL_APB5_GRP1_EnableClockSleep\n
  *         MC_APB5LPENSETR    I2C4         LL_APB5_GRP1_EnableClockSleep\n
  *         MC_APB5LPENSETR    I2C6         LL_APB5_GRP1_EnableClockSleep\n
  *         MC_APB5LPENSETR    USART1       LL_APB5_GRP1_EnableClockSleep\n
  *         MC_APB5LPENSETR    RTCAPB       LL_APB5_GRP1_EnableClockSleep\n
  *         MC_APB5LPENSETR    TZC1         LL_APB5_GRP1_EnableClockSleep\n
  *         MC_APB5LPENSETR    TZC2         LL_APB5_GRP1_EnableClockSleep\n
  *         MC_APB5LPENSETR    TZPC         LL_APB5_GRP1_EnableClockSleep\n
  *         MC_APB5LPENSETR    BSEC         LL_APB5_GRP1_EnableClockSleep\n
  *         MC_APB5LPENSETR    STGEN        LL_APB5_GRP1_EnableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB5_GRP1_PERIPH_SPI6
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C4
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C6
  *         @arg @ref LL_APB5_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB5_GRP1_PERIPH_RTCAPB
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZC1
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZC2
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZPC
  *         @arg @ref LL_APB5_GRP1_PERIPH_BSEC
  *         @arg @ref LL_APB5_GRP1_PERIPH_STGEN
  * @retval None
  */
__STATIC_INLINE void LL_APB5_GRP1_EnableClockSleep(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB5LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB5LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB5 peripheral clocks in Sleep modes
  * @rmtoll MC_APB5LPENCLRR    SPI6         LL_APB5_GRP1_DisableClockSleep\n
  *         MC_APB5LPENCLRR    I2C4         LL_APB5_GRP1_DisableClockSleep\n
  *         MC_APB5LPENCLRR    I2C6         LL_APB5_GRP1_DisableClockSleep\n
  *         MC_APB5LPENCLRR    USART1       LL_APB5_GRP1_DisableClockSleep\n
  *         MC_APB5LPENCLRR    RTCAPB       LL_APB5_GRP1_DisableClockSleep\n
  *         MC_APB5LPENCLRR    TZC1         LL_APB5_GRP1_DisableClockSleep\n
  *         MC_APB5LPENCLRR    TZC2         LL_APB5_GRP1_DisableClockSleep\n
  *         MC_APB5LPENCLRR    TZPC         LL_APB5_GRP1_DisableClockSleep\n
  *         MC_APB5LPENCLRR    BSEC         LL_APB5_GRP1_DisableClockSleep\n
  *         MC_APB5LPENCLRR    STGEN        LL_APB5_GRP1_DisableClockSleep
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB5_GRP1_PERIPH_SPI6
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C4
  *         @arg @ref LL_APB5_GRP1_PERIPH_I2C6
  *         @arg @ref LL_APB5_GRP1_PERIPH_USART1
  *         @arg @ref LL_APB5_GRP1_PERIPH_RTCAPB
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZC1
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZC2
  *         @arg @ref LL_APB5_GRP1_PERIPH_TZPC
  *         @arg @ref LL_APB5_GRP1_PERIPH_BSEC
  *         @arg @ref LL_APB5_GRP1_PERIPH_STGEN
  * @retval None
  */
__STATIC_INLINE void LL_APB5_GRP1_DisableClockSleep(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB5LPENCLRR, Periphs);
}

/**
  * @brief  Enable APB5 peripheral clocks in Stop mode
  * @rmtoll MC_APB5LPENSETR    STGENSTP           LL_APB5_GRP1_EnableClockStop
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB5_GRP1_PERIPH_STGENSTP
  * @retval None
  */
__STATIC_INLINE void LL_APB5_GRP1_EnableClockStop(uint32_t Periphs)
{
  __IO uint32_t tmpreg;
  WRITE_REG(RCC->MC_APB5LPENSETR, Periphs);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->MC_APB5LPENSETR, Periphs);
  (void)tmpreg;
}

/**
  * @brief  Disable APB5 peripheral clocks in Stop modes
  * @rmtoll MC_APB5LPENCLRR    STGENSTP          LL_APB5_GRP1_DisableClockStop\n
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_APB5_GRP1_PERIPH_STGENSTP
  * @retval None
  */
__STATIC_INLINE void LL_APB5_GRP1_DisableClockStop(uint32_t Periphs)
{
  WRITE_REG(RCC->MC_APB5LPENCLRR, Periphs);
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

#endif /* STM32MP1xx_LL_BUS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
