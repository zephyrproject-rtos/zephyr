/**
  ******************************************************************************
  * @file    stm32g0xx_hal_rcc_ex.h
  * @author  MCD Application Team
  * @brief   Header file of RCC HAL Extended module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics. 
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
#ifndef STM32G0xx_HAL_RCC_EX_H
#define STM32G0xx_HAL_RCC_EX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal_def.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @addtogroup RCCEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup RCCEx_Exported_Types RCCEx Exported Types
  * @{
  */

/**
  * @brief  RCC extended clocks structure definition
  */
typedef struct
{
  uint32_t PeriphClockSelection;   /*!< The Extended Clock to be configured.
                                        This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */

  uint32_t Usart1ClockSelection;   /*!< Specifies USART1 clock source.
                                        This parameter can be a value of @ref RCCEx_USART1_Clock_Source */
#if (RCC_CCIPR_USART2SEL)
  uint32_t Usart2ClockSelection;   /*!< Specifies USART2 clock source.
                                        This parameter can be a value of @ref RCCEx_USART2_Clock_Source */
#endif
#if (RCC_CCIPR_LPUART1SEL)
  uint32_t Lpuart1ClockSelection;  /*!< Specifies LPUART1 clock source
                                        This parameter can be a value of @ref RCCEx_LPUART1_Clock_Source */
#endif
  uint32_t I2c1ClockSelection;     /*!< Specifies I2C1 clock source
                                        This parameter can be a value of @ref RCCEx_I2C1_Clock_Source */

  uint32_t I2s1ClockSelection;     /*!< Specifies I2S1 clock source
                                        This parameter can be a value of @ref RCCEx_I2S1_Clock_Source */

#if (RCC_CCIPR_LPTIM1SEL)
  uint32_t Lptim1ClockSelection;   /*!< Specifies LPTIM1 clock source
                                        This parameter can be a value of @ref RCCEx_LPTIM1_Clock_Source */
#endif
#if (RCC_CCIPR_LPTIM2SEL)
  uint32_t Lptim2ClockSelection;   /*!< Specifies LPTIM2 clock source
                                        This parameter can be a value of @ref RCCEx_LPTIM2_Clock_Source */
#endif
#if defined(RNG)
  uint32_t RngClockSelection;      /*!< Specifies RNG clock source
                                        This parameter can be a value of @ref RCCEx_RNG_Clock_Source */
#endif
  uint32_t AdcClockSelection;      /*!< Specifies ADC interface clock source
                                        This parameter can be a value of @ref RCCEx_ADC_Clock_Source */
#if defined(CEC)
  uint32_t CecClockSelection;      /*!< Specifies CEC Clock clock source
                                        This parameter can be a value of @ref RCCEx_CEC_Clock_Source */
#endif
#if (RCC_CCIPR_TIM1SEL)
  uint32_t Tim1ClockSelection;      /*!< Specifies TIM1 Clock clock source
                                         This parameter can be a value of @ref RCCEx_TIM1_Clock_Source */
#endif
#if (RCC_CCIPR_TIM15SEL)
  uint32_t Tim15ClockSelection;     /*!< Specifies TIM15 Clock clock source
                                         This parameter can be a value of @ref RCCEx_TIM15_Clock_Source */
#endif
  uint32_t RTCClockSelection;      /*!< Specifies RTC clock source.
                                        This parameter can be a value of @ref RCC_RTC_Clock_Source */
} RCC_PeriphCLKInitTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Constants RCCEx Exported Constants
  * @{
  */

/** @defgroup RCCEx_LSCO_Clock_Source Low Speed Clock Source
  * @{
  */
#define RCC_LSCOSOURCE_LSI             0x00000000U           /*!< LSI selection for low speed clock output */
#define RCC_LSCOSOURCE_LSE             RCC_BDCR_LSCOSEL      /*!< LSE selection for low speed clock output */
/**
  * @}
  */

/** @defgroup RCCEx_Periph_Clock_Selection Periph Clock Selection
  * @{
  */
#define RCC_PERIPHCLK_USART1           0x00000001U
#if defined(RCC_CCIPR_USART2SEL)
#define RCC_PERIPHCLK_USART2           0x00000002U
#endif /* RCC_CCIPR_USART2SEL */
#if defined(RCC_CCIPR_LPUART1SEL)
#define RCC_PERIPHCLK_LPUART1          0x00000020U
#endif /* RCC_CCIPR_LPUART1SEL */
#define RCC_PERIPHCLK_I2C1             0x00000040U
#if defined(RCC_CCIPR_LPTIM1SEL)
#define RCC_PERIPHCLK_LPTIM1           0x00000200U
#endif /* RCC_CCIPR_LPTIM1SEL */
#if defined(RCC_CCIPR_LPTIM2SEL)
#define RCC_PERIPHCLK_LPTIM2           0x00000400U
#endif /* RCC_CCIPR_LPTIM2SEL */
#define RCC_PERIPHCLK_I2S1             0x00000800U
#define RCC_PERIPHCLK_ADC              0x00004000U
#define RCC_PERIPHCLK_RTC              0x00020000U
#if defined(RCC_CCIPR_RNGSEL)
#define RCC_PERIPHCLK_RNG              0x00040000U
#endif /* RCC_CCIPR_RNGSEL */
#if defined(RCC_CCIPR_CECSEL)
#define RCC_PERIPHCLK_CEC              0x00080000U
#endif /* RCC_CCIPR_CECSEL */
#if defined(RCC_CCIPR_TIM1SEL)
#define RCC_PERIPHCLK_TIM1             0x00200000U
#endif /* RCC_CCIPR_TIM1SEL */
#if defined(RCC_CCIPR_TIM15SEL)
#define RCC_PERIPHCLK_TIM15            0x00400000U
#endif /* RCC_CCIPR_TIM15SEL */

/**
  * @}
  */


/** @defgroup RCCEx_USART1_Clock_Source RCC USART1 Clock Source
  * @{
  */
#define RCC_USART1CLKSOURCE_PCLK1      0x00000000U                                      /*!< APB clock selected as USART 1 clock */
#define RCC_USART1CLKSOURCE_SYSCLK     RCC_CCIPR_USART1SEL_0                            /*!< SYSCLK clock selected as USART 1 clock */
#define RCC_USART1CLKSOURCE_HSI        RCC_CCIPR_USART1SEL_1                            /*!< HSI clock selected as USART 1 clock */
#define RCC_USART1CLKSOURCE_LSE        (RCC_CCIPR_USART1SEL_0 | RCC_CCIPR_USART1SEL_1)  /*!< LSE clock selected as USART 1 clock */
/**
  * @}
  */

#if defined(RCC_CCIPR_USART2SEL)
/** @defgroup RCCEx_USART2_Clock_Source RCC USART2 Clock Source
  * @{
  */
#define RCC_USART2CLKSOURCE_PCLK1      0x00000000U                                      /*!< APB clock selected as USART 2 clock */
#define RCC_USART2CLKSOURCE_SYSCLK     RCC_CCIPR_USART2SEL_0                            /*!< SYSCLK clock selected as USART 2 clock */
#define RCC_USART2CLKSOURCE_HSI        RCC_CCIPR_USART2SEL_1                            /*!< HSI clock selected as USART 2 clock */
#define RCC_USART2CLKSOURCE_LSE        (RCC_CCIPR_USART2SEL_0 | RCC_CCIPR_USART2SEL_1)  /*!< LSE clock selected as USART 2 clock */
/**
  * @}
  */
#endif /* RCC_CCIPR_USART2SEL */
#if defined(RCC_CCIPR_LPUART1SEL)
/** @defgroup RCCEx_LPUART1_Clock_Source RCC LPUART1 Clock Source
  * @{
  */
#define RCC_LPUART1CLKSOURCE_PCLK1     0x00000000U                                        /*!< APB clock selected as LPUART 1 clock */
#define RCC_LPUART1CLKSOURCE_SYSCLK    RCC_CCIPR_LPUART1SEL_0                             /*!< SYSCLK clock selected as LPUART 1 clock */
#define RCC_LPUART1CLKSOURCE_HSI       RCC_CCIPR_LPUART1SEL_1                             /*!< HSI clock selected as LPUART 1 clock */
#define RCC_LPUART1CLKSOURCE_LSE       (RCC_CCIPR_LPUART1SEL_0 | RCC_CCIPR_LPUART1SEL_1)  /*!< LSE clock selected as LPUART 1 clock */
/**
  * @}
  */
#endif /* RCC_CCIPR_LPUART1SEL */

/** @defgroup RCCEx_I2C1_Clock_Source RCC I2C1 Clock Source
  * @{
  */
#define RCC_I2C1CLKSOURCE_PCLK1        0x00000000U                                      /*!< APB clock selected as I2C1 clock */
#define RCC_I2C1CLKSOURCE_SYSCLK       RCC_CCIPR_I2C1SEL_0                              /*!< SYSCLK clock selected as I2C1 clock */
#define RCC_I2C1CLKSOURCE_HSI          RCC_CCIPR_I2C1SEL_1                              /*!< HSI clock selected as I2C1 clock */
/**
  * @}
  */

/** @defgroup RCCEx_I2S1_Clock_Source RCC I2S1 Clock Source
  * @{
  */
#define RCC_I2S1CLKSOURCE_SYSCLK       0x00000000U                                     /*!< SYSCLK clock selected as I2S1 clock */
#define RCC_I2S1CLKSOURCE_PLL          RCC_CCIPR_I2S1SEL_0                             /*!< PLL "P" selected as I2S1 clock */
#define RCC_I2S1CLKSOURCE_HSI          RCC_CCIPR_I2S1SEL_1                             /*!< HSI clock selected as I2S1 clock */
#define RCC_I2S1CLKSOURCE_EXT          RCC_CCIPR_I2S1SEL                               /*!< External I2S clock source selected as I2S1 clock */

/**
  * @}
  */

#if (RCC_CCIPR_LPTIM1SEL)
/** @defgroup RCCEx_LPTIM1_Clock_Source RCC LPTIM1 Clock Source
  * @{
  */
#define RCC_LPTIM1CLKSOURCE_PCLK1      0x00000000U               /*!< APB clock selected as LPTimer 1 clock */
#define RCC_LPTIM1CLKSOURCE_LSI        RCC_CCIPR_LPTIM1SEL_0     /*!< LSI clock selected as LPTimer 1 clock */
#define RCC_LPTIM1CLKSOURCE_HSI        RCC_CCIPR_LPTIM1SEL_1     /*!< HSI clock selected as LPTimer 1 clock */
#define RCC_LPTIM1CLKSOURCE_LSE        RCC_CCIPR_LPTIM1SEL       /*!< LSE clock selected as LPTimer 1 clock */
/**
  * @}
  */
#endif /* RCC_CCIPR_LPTIM1SEL */

#if (RCC_CCIPR_LPTIM2SEL)
/** @defgroup RCCEx_LPTIM2_Clock_Source RCC LPTIM2 Clock Source
  * @{
  */
#define RCC_LPTIM2CLKSOURCE_PCLK1      0x00000000U               /*!< APB clock selected as LPTimer 2 clock */
#define RCC_LPTIM2CLKSOURCE_LSI        RCC_CCIPR_LPTIM2SEL_0     /*!< LSI clock selected as LPTimer 2 clock */
#define RCC_LPTIM2CLKSOURCE_HSI        RCC_CCIPR_LPTIM2SEL_1     /*!< HSI clock selected as LPTimer 2 clock */
#define RCC_LPTIM2CLKSOURCE_LSE        RCC_CCIPR_LPTIM2SEL       /*!< LSE clock selected as LPTimer 2 clock */
/**
  * @}
  */
#endif /* RCC_CCIPR_LPTIM2SEL */

#if defined(RNG)
/** @defgroup RCCEx_RNG_Clock_Source RCC RNG Clock Source
  * @{
  */
#define RCC_RNGCLKSOURCE_NONE          0x00000000U                             /*!< No clock selected */
#define RCC_RNGCLKSOURCE_HSI_DIV8      RCC_CCIPR_RNGSEL_0                      /*!< HSI oscillator divided by 8 clock selected as RNG clock */
#define RCC_RNGCLKSOURCE_SYSCLK        RCC_CCIPR_RNGSEL_1                      /*!< SYSCLK selected as RNG clock */
#define RCC_RNGCLKSOURCE_PLL           (RCC_CCIPR_RNGSEL_0|RCC_CCIPR_RNGSEL_1) /*!< PLL "Q" selected as RNG clock */

/**
  * @}
  */

/** @defgroup RCCEx_RNG_Division_factor RCC RNG Division factor
  * @{
  */
#define RCC_RNGCLK_DIV1           0x00000000U                              /*!< RNG clock not divided  */
#define RCC_RNGCLK_DIV2           RCC_CCIPR_RNGDIV_0                       /*!< RNG clock divided by 2 */
#define RCC_RNGCLK_DIV4           RCC_CCIPR_RNGDIV_1                       /*!< RNG clock divided by 4 */
#define RCC_RNGCLK_DIV8           (RCC_CCIPR_RNGDIV_0|RCC_CCIPR_RNGDIV_1)  /*!< RNG clock divided by 8 */

/**
  * @}
  */
#endif /* RNG */

/** @defgroup RCCEx_ADC_Clock_Source RCC ADC Clock Source
  * @{
  */

#define RCC_ADCCLKSOURCE_SYSCLK       0x00000000U             /*!< SYSCLK used as ADC clock */
#define RCC_ADCCLKSOURCE_PLLADC       RCC_CCIPR_ADCSEL_0      /*!< PLL "P" (PLLADC) used as ADC clock */
#define RCC_ADCCLKSOURCE_HSI          RCC_CCIPR_ADCSEL_1      /*!< HSI used as ADC clock */
/**
  * @}
  */

#if defined(CEC)
/** @defgroup RCCEx_CEC_Clock_Source RCC CEC Clock Source
  * @{
  */
#define RCC_CECCLKSOURCE_HSI_DIV488          0x00000000U      /*!< HSI oscillator clock divided by 488 used as default CEC clock */
#define RCC_CECCLKSOURCE_LSE                 RCC_CCIPR_CECSEL /*!< LSE oscillator clock  used as CEC clock */
/**
  * @}
  */
#endif /* CEC */

#if (RCC_CCIPR_TIM1SEL)
/** @defgroup RCCEx_TIM1_Clock_Source RCC TIM1 Clock Source
  * @{
  */
#define RCC_TIM1CLKSOURCE_PCLK1       0x00000000U             /*!< APB clock selected as Timer 1 clock */
#define RCC_TIM1CLKSOURCE_PLL         RCC_CCIPR_TIM1SEL       /*!< PLL "Q" clock selected as Timer 1 clock */
/**
  * @}
  */
#endif /* RCC_CCIPR_TIM1SEL */

#if (RCC_CCIPR_TIM15SEL)
/** @defgroup RCCEx_TIM15_Clock_Source RCC TIM15 Clock Source
  * @{
  */
#define RCC_TIM15CLKSOURCE_PCLK1      0x00000000U             /*!< APB clock selected as Timer 15 clock */
#define RCC_TIM15CLKSOURCE_PLL        RCC_CCIPR_TIM15SEL      /*!< PLL "Q" clock selected as Timer 15 clock */

/**
  * @}
  */
#endif /* RCC_CCIPR_TIM15SEL */


/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Macros RCCEx Exported Macros
 * @{
 */


/** @brief  Macro to configure the I2C1 clock (I2C1CLK).
  *
  * @param  __I2C1_CLKSOURCE__  specifies the I2C1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_I2C1CLKSOURCE_HSI  HSI selected as I2C1 clock
  *            @arg @ref RCC_I2C1CLKSOURCE_SYSCLK  System Clock selected as I2C1 clock
  */
#define __HAL_RCC_I2C1_CONFIG(__I2C1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_I2C1SEL, (uint32_t)(__I2C1_CLKSOURCE__))

/** @brief  Macro to get the I2C1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_I2C1CLKSOURCE_HSI  HSI selected as I2C1 clock
  *            @arg @ref RCC_I2C1CLKSOURCE_SYSCLK  System Clock selected as I2C1 clock
  */
#define __HAL_RCC_GET_I2C1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_I2C1SEL)))

/** @brief  Macro to configure the I2S1 clock (I2S1CLK).
  *
  * @param  __I2S1_CLKSOURCE__  specifies the I2S1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_I2S1CLKSOURCE_SYSCLK  System Clock selected as I2S1 clock
  *            @arg @ref RCC_I2S1CLKSOURCE_PLL     PLLP Clock selected as I2S1 clock
  *            @arg @ref RCC_I2S1CLKSOURCE_HSI     HSI Clock selected as I2S1 clock
  *            @arg @ref RCC_I2S1CLKSOURCE_EXT     External clock selected as I2S1 clock
  */
#define __HAL_RCC_I2S1_CONFIG(__I2S1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_I2S1SEL, (uint32_t)(__I2S1_CLKSOURCE__))

/** @brief  Macro to get the I2S1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_I2S1CLKSOURCE_SYSCLK  System Clock selected as I2S1 clock
  *            @arg @ref RCC_I2S1CLKSOURCE_PLL     PLLP Clock selected as I2S1 clock
  *            @arg @ref RCC_I2S1CLKSOURCE_HSI     HSI Clock selected as I2S1 clock
  *            @arg @ref RCC_I2S1CLKSOURCE_EXT     External clock selected as I2S1 clock
  */
#define __HAL_RCC_GET_I2S1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_I2S1SEL)))


/** @brief  Macro to configure the USART1 clock (USART1CLK).
  *
  * @param  __USART1_CLKSOURCE__ specifies the USART1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_USART1CLKSOURCE_PCLK1   PCLK1 selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_HSI  HSI selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_SYSCLK  System Clock selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_LSE  LSE selected as USART1 clock
  */
#define __HAL_RCC_USART1_CONFIG(__USART1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_USART1SEL, (uint32_t)(__USART1_CLKSOURCE__))

/** @brief  Macro to get the USART1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_USART1CLKSOURCE_PCLK1   PCLK1 selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_HSI  HSI selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_SYSCLK  System Clock selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_LSE  LSE selected as USART1 clock
  */
#define __HAL_RCC_GET_USART1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_USART1SEL)))

#if defined(RCC_CCIPR_USART2SEL)
/** @brief  Macro to configure the USART2 clock (USART2CLK).
  *
  * @param  __USART2_CLKSOURCE__ specifies the USART2 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_USART2CLKSOURCE_PCLK1   PCLK1 selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_HSI  HSI selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_SYSCLK  System Clock selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_LSE  LSE selected as USART2 clock
  */
#define __HAL_RCC_USART2_CONFIG(__USART2_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_USART2SEL, (uint32_t)(__USART2_CLKSOURCE__))

/** @brief  Macro to get the USART2 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_USART2CLKSOURCE_PCLK1   PCLK1 selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_HSI  HSI selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_SYSCLK  System Clock selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_LSE  LSE selected as USART2 clock
  */
#define __HAL_RCC_GET_USART2_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_USART2SEL)))
#endif /* RCC_CCIPR_USART2SEL */

#if defined(RCC_CCIPR_LPUART1SEL)
/** @brief  Macro to configure the LPUART1 clock (LPUART1CLK).
  *
  * @param  __LPUART1_CLKSOURCE__ specifies the LPUART1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LPUART1CLKSOURCE_PCLK1   PCLK1 selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_HSI  HSI selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_SYSCLK  System Clock selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_LSE  LSE selected as LPUART1 clock
  */
#define __HAL_RCC_LPUART1_CONFIG(__LPUART1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_LPUART1SEL, (uint32_t)(__LPUART1_CLKSOURCE__))

/** @brief  Macro to get the LPUART1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_LPUART1CLKSOURCE_PCLK1  PCLK1 selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_HSI HSI selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_SYSCLK System Clock selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_LSE LSE selected as LPUART1 clock
  */
#define __HAL_RCC_GET_LPUART1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_LPUART1SEL)))
#endif /* RCC_CCIPR_LPUART1SEL */

#if (RCC_CCIPR_LPTIM1SEL)
/** @brief  Macro to configure the LPTIM1 clock (LPTIM1CLK).
  *
  * @param  __LPTIM1_CLKSOURCE__ specifies the LPTIM1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LPTIM1CLKSOURCE_PCLK1  PCLK1 selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSI  HSI  selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_HSI  LSI  selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSE  LSE  selected as LPTIM1 clock
  */
#define __HAL_RCC_LPTIM1_CONFIG(__LPTIM1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_LPTIM1SEL, (uint32_t)(__LPTIM1_CLKSOURCE__))

/** @brief  Macro to get the LPTIM1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_LPTIM1CLKSOURCE_PCLK1  PCLK1 selected as LPUART1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSI  HSI selected as LPUART1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_HSI  System Clock selected as LPUART1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSE  LSE selected as LPUART1 clock
  */
#define __HAL_RCC_GET_LPTIM1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_LPTIM1SEL)))
#endif /* RCC_CCIPR_LPTIM1SEL */

#if (RCC_CCIPR_LPTIM2SEL)
/** @brief  Macro to configure the LPTIM2 clock (LPTIM2CLK).
  *
  * @param  __LPTIM2_CLKSOURCE__ specifies the LPTIM2 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LPTIM2CLKSOURCE_PCLK1  PCLK1 selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_LSI  HSI  selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_HSI  LSI  selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_LSE  LSE  selected as LPTIM2 clock
  */
#define __HAL_RCC_LPTIM2_CONFIG(__LPTIM2_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_LPTIM2SEL, (uint32_t)(__LPTIM2_CLKSOURCE__))

/** @brief  Macro to get the LPTIM2 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_LPTIM2CLKSOURCE_PCLK1  PCLK1 selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_LSI  HSI selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_HSI  System Clock selected as LPTIM2 clock
  *            @arg @ref RCC_LPTIM2CLKSOURCE_LSE  LSE selected as LPTIM2 clock
  */
#define __HAL_RCC_GET_LPTIM2_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_LPTIM2SEL)))
#endif /* RCC_CCIPR_LPTIM2SEL */

#if defined(CEC)
/** @brief  Macro to configure the CEC clock (CECCLK).
  *
  * @param  __CEC_CLKSOURCE__ specifies the CEC clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_CECCLKSOURCE_HSI_DIV488 HSI_DIV_488 selected as CEC clock
  *            @arg @ref RCC_CECCLKSOURCE_LSE LSE selected as CEC clock
  */
#define __HAL_RCC_CEC_CONFIG(__CEC_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_CECSEL, (uint32_t)(__CEC_CLKSOURCE__))

/** @brief  Macro to get the CEC clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_CECCLKSOURCE_HSI_DIV488 HSI_DIV_488 Clock selected as CEC clock
  *            @arg @ref RCC_CECCLKSOURCE_LSE  LSE selected as CEC clock
  */
#define __HAL_RCC_GET_CEC_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_CECSEL)))
#endif /* CEC */

#if defined(RNG)
/** @brief  Macro to configure the RNG clock.
  *
  *
  * @param  __RNG_CLKSOURCE__ specifies the RNG clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_RNGCLKSOURCE_NONE  No clock selected as RNG clock
  *            @arg @ref RCC_RNGCLKSOURCE_HSI_DIV8 HSI Clock divided by 8 selected as RNG clock
  *            @arg @ref RCC_RNGCLKSOURCE_SYSCLK System Clock selected as RNG clock
  *            @arg @ref RCC_RNGCLKSOURCE_PLL PLLQ Output Clock selected as RNG clock
  */
#define __HAL_RCC_RNG_CONFIG(__RNG_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_RNGSEL, (uint32_t)(__RNG_CLKSOURCE__))

/** @brief  Macro to get the RNG clock.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_RNGCLKSOURCE_NONE  No clock selected as RNG clock
  *            @arg @ref RCC_RNGCLKSOURCE_HSI_DIV8 HSI Clock divide by 8 selected as RNG clock
  *            @arg @ref RCC_RNGCLKSOURCE_SYSCLK System clock selected as RNG clock
  *            @arg @ref RCC_RNGCLKSOURCE_PLL PLLQ Output Clock selected as RNG clock
  */
#define __HAL_RCC_GET_RNG_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_RNGSEL)))

/** @brief  Macro to configure the RNG clock.
  *
  *
  * @param  __RNG_CLKDIV__ specifies the RNG clock division factor.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_RNGCLK_DIV1  RNG Clock not divided
  *            @arg @ref RCC_RNGCLK_DIV2  RNG Clock divided by 2
  *            @arg @ref RCC_RNGCLK_DIV4  RNG Clock divided by 4
  *            @arg @ref RCC_RNGCLK_DIV8  RNG Clock divided by 8
  */
#define __HAL_RCC_RNGDIV_CONFIG(__RNG_CLKDIV__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_RNGDIV, (uint32_t)(__RNG_CLKDIV__))

/** @brief  Macro to get the RNG clock division factor.
  * @retval The division factor can be one of the following values:
  *            @arg @ref RCC_RNGCLK_DIV1   RNG Clock not divided
  *            @arg @ref RCC_RNGCLK_DIV2   RNG Clock divided by 2
  *            @arg @ref RCC_RNGCLK_DIV4   RNG Clock divided by 4
  *            @arg @ref RCC_RNGCLK_DIV8   RNG Clock divided by 8
  */
#define __HAL_RCC_GET_RNG_DIV() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_RNGDIV)))
#endif /* RNG */

/** @brief  Macro to configure the ADC interface clock
  * @param  __ADC_CLKSOURCE__ specifies the ADC digital interface clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_ADCCLKSOURCE_PLLADC PLL "P" (PLLADC) Clock selected as ADC clock
  *            @arg @ref RCC_ADCCLKSOURCE_SYSCLK System Clock selected as ADC clock
  *            @arg @ref RCC_ADCCLKSOURCE_HSI HSI Clock selected as ADC clock
  */
#define __HAL_RCC_ADC_CONFIG(__ADC_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_ADCSEL, (uint32_t)(__ADC_CLKSOURCE__))

/** @brief  Macro to get the ADC clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_ADCCLKSOURCE_PLLADC PLL "P" (PLLADC) Clock selected as ADC clock
  *            @arg @ref RCC_ADCCLKSOURCE_SYSCLK System Clock selected as ADC clock
  *            @arg @ref RCC_ADCCLKSOURCE_HSI HSI Clock selected as ADC clock
  */
#define __HAL_RCC_GET_ADC_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_ADCSEL)))

#if (RCC_CCIPR_TIM1SEL)
/** @brief  Macro to configure the TIM1 interface clock
  * @param  __TIM1_CLKSOURCE__ specifies the TIM1 digital interface clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_TIM1CLKSOURCE_PLL PLLQ Output Clock selected as TIM1 clock
  *            @arg @ref RCC_TIM1CLKSOURCE_PCLK1 System Clock selected as TIM1 clock
  */
#define __HAL_RCC_TIM1_CONFIG(__TIM1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_TIM1SEL, (uint32_t)(__TIM1_CLKSOURCE__))

/** @brief  Macro to get the TIM1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_TIM1CLKSOURCE_PLL PLLQ Output Clock selected as TIM1 clock
  *            @arg @ref RCC_TIM1CLKSOURCE_PCLK1  System Clock selected as TIM1 clock
  */
#define __HAL_RCC_GET_TIM1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_TIM1SEL)))
#endif /* RCC_CCIPR_TIM1SEL */

#if (RCC_CCIPR_TIM15SEL)
/** @brief  Macro to configure the TIM15 interface clock
  * @param  __TIM15_CLKSOURCE__ specifies the TIM15 digital interface clock source.
  *         This parameter can be one of the following values:
  *            @arg RCC_TIM15CLKSOURCE_PLL PLLQ Output Clock selected as TIM15 clock
  *            @arg RCC_TIM15CLKSOURCE_PCLK1  System Clock selected as TIM15 clock
  */
#define __HAL_RCC_TIM15_CONFIG(__TIM15_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_TIM15SEL, (uint32_t)(__TIM15_CLKSOURCE__))

/** @brief  Macro to get the TIM15 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_TIM15CLKSOURCE_PLL PLLQ Output Clock selected as TIM15 clock
  *            @arg @ref RCC_TIM15CLKSOURCE_PCLK1  System Clock selected as TIM15 clock
  */
#define __HAL_RCC_GET_TIM15_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_TIM15SEL)))
#endif /* RCC_CCIPR_TIM15SEL */

/** @defgroup RCCEx_Flags_Interrupts_Management Flags Interrupts Management
  * @brief macros to manage the specified RCC Flags and interrupts.
  * @{
  */



/**
  * @}
  */


/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup RCCEx_Exported_Functions
  * @{
  */

/** @addtogroup RCCEx_Exported_Functions_Group1
  * @{
  */

HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);
void              HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);
uint32_t          HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk);

/**
  * @}
  */

/** @addtogroup RCCEx_Exported_Functions_Group2
  * @{
  */

void              HAL_RCCEx_EnableLSCO(uint32_t LSCOSource);
void              HAL_RCCEx_DisableLSCO(void);

/**
  * @}
  */


/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup RCCEx_Private_Macros RCCEx Private Macros
  * @{
  */

#define IS_RCC_LSCOSOURCE(__SOURCE__) (((__SOURCE__) == RCC_LSCOSOURCE_LSI) || \
                                       ((__SOURCE__) == RCC_LSCOSOURCE_LSE))

#if defined(STM32G081xx)
#define IS_RCC_PERIPHCLOCK(__SELECTION__)  \
               ((((__SELECTION__) & RCC_PERIPHCLK_USART1)  == RCC_PERIPHCLK_USART1)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_USART2)  == RCC_PERIPHCLK_USART2)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_LPUART1) == RCC_PERIPHCLK_LPUART1) || \
                (((__SELECTION__) & RCC_PERIPHCLK_I2C1)    == RCC_PERIPHCLK_I2C1)    || \
                (((__SELECTION__) & RCC_PERIPHCLK_I2S1)    == RCC_PERIPHCLK_I2S1)    || \
                (((__SELECTION__) & RCC_PERIPHCLK_LPTIM1)  == RCC_PERIPHCLK_LPTIM1)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_LPTIM2)  == RCC_PERIPHCLK_LPTIM2)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_ADC)     == RCC_PERIPHCLK_ADC)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_RTC)     == RCC_PERIPHCLK_RTC)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_RNG)     == RCC_PERIPHCLK_RNG)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_CEC)     == RCC_PERIPHCLK_CEC)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_TIM1)    == RCC_PERIPHCLK_TIM1)    || \
                (((__SELECTION__) & RCC_PERIPHCLK_TIM15)   == RCC_PERIPHCLK_TIM15))
#elif defined(STM32G071xx)
#define IS_RCC_PERIPHCLOCK(__SELECTION__)  \
               ((((__SELECTION__) & RCC_PERIPHCLK_USART1)  == RCC_PERIPHCLK_USART1)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_USART2)  == RCC_PERIPHCLK_USART2)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_LPUART1) == RCC_PERIPHCLK_LPUART1) || \
                (((__SELECTION__) & RCC_PERIPHCLK_I2C1)    == RCC_PERIPHCLK_I2C1)    || \
                (((__SELECTION__) & RCC_PERIPHCLK_I2S1)    == RCC_PERIPHCLK_I2S1)    || \
                (((__SELECTION__) & RCC_PERIPHCLK_LPTIM1)  == RCC_PERIPHCLK_LPTIM1)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_LPTIM2)  == RCC_PERIPHCLK_LPTIM2)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_ADC)     == RCC_PERIPHCLK_ADC)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_RTC)     == RCC_PERIPHCLK_RTC)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_CEC)     == RCC_PERIPHCLK_CEC)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_TIM1)    == RCC_PERIPHCLK_TIM1)    || \
                (((__SELECTION__) & RCC_PERIPHCLK_TIM15)   == RCC_PERIPHCLK_TIM15))
#elif defined(STM32G070xx)
#define IS_RCC_PERIPHCLOCK(__SELECTION__)  \
               ((((__SELECTION__) & RCC_PERIPHCLK_USART1)  == RCC_PERIPHCLK_USART1)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_USART2)  == RCC_PERIPHCLK_USART2)  || \
                (((__SELECTION__) & RCC_PERIPHCLK_I2C1)    == RCC_PERIPHCLK_I2C1)    || \
                (((__SELECTION__) & RCC_PERIPHCLK_I2S1)    == RCC_PERIPHCLK_I2S1)    || \
                (((__SELECTION__) & RCC_PERIPHCLK_ADC)     == RCC_PERIPHCLK_ADC)     || \
                (((__SELECTION__) & RCC_PERIPHCLK_RTC)     == RCC_PERIPHCLK_RTC))
#endif /* STM32G081xx */

#define IS_RCC_USART1CLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_USART1CLKSOURCE_PCLK1)  || \
                ((__SOURCE__) == RCC_USART1CLKSOURCE_SYSCLK) || \
                ((__SOURCE__) == RCC_USART1CLKSOURCE_LSE)    || \
                ((__SOURCE__) == RCC_USART1CLKSOURCE_HSI))

#if (RCC_CCIPR_USART2SEL)
#define IS_RCC_USART2CLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_USART2CLKSOURCE_PCLK1)  || \
                ((__SOURCE__) == RCC_USART2CLKSOURCE_SYSCLK) || \
                ((__SOURCE__) == RCC_USART2CLKSOURCE_LSE)    || \
                ((__SOURCE__) == RCC_USART2CLKSOURCE_HSI))
#endif /* RCC_CCIPR_USART2SEL */

#if (RCC_CCIPR_LPUART1SEL)
#define IS_RCC_LPUART1CLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_LPUART1CLKSOURCE_PCLK1)  || \
                ((__SOURCE__) == RCC_LPUART1CLKSOURCE_SYSCLK) || \
                ((__SOURCE__) == RCC_LPUART1CLKSOURCE_LSE)    || \
                ((__SOURCE__) == RCC_LPUART1CLKSOURCE_HSI))
#endif /* RCC_CCIPR_LPUART1SEL */

#define IS_RCC_I2C1CLKSOURCE(__SOURCE__)   \
               (((__SOURCE__) == RCC_I2C1CLKSOURCE_PCLK1)   || \
                ((__SOURCE__) == RCC_I2C1CLKSOURCE_SYSCLK)  || \
                ((__SOURCE__) == RCC_I2C1CLKSOURCE_HSI))

#define IS_RCC_I2S1CLKSOURCE(__SOURCE__)   \
               (((__SOURCE__) == RCC_I2S1CLKSOURCE_SYSCLK)|| \
                ((__SOURCE__) == RCC_I2S1CLKSOURCE_PLL)   || \
                ((__SOURCE__) == RCC_I2S1CLKSOURCE_HSI)   || \
                ((__SOURCE__) == RCC_I2S1CLKSOURCE_EXT))

#if (RCC_CCIPR_LPTIM1SEL)
#define IS_RCC_LPTIM1CLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_LPTIM1CLKSOURCE_PCLK1)|| \
                ((__SOURCE__) == RCC_LPTIM1CLKSOURCE_LSI)  || \
                ((__SOURCE__) == RCC_LPTIM1CLKSOURCE_HSI)  || \
                ((__SOURCE__) == RCC_LPTIM1CLKSOURCE_LSE))
#endif /* RCC_CCIPR_LPTIM1SEL */

#if (RCC_CCIPR_LPTIM2SEL)
#define IS_RCC_LPTIM2CLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_LPTIM2CLKSOURCE_PCLK1)|| \
                ((__SOURCE__) == RCC_LPTIM2CLKSOURCE_LSI)  || \
                ((__SOURCE__) == RCC_LPTIM2CLKSOURCE_HSI)  || \
                ((__SOURCE__) == RCC_LPTIM2CLKSOURCE_LSE))
#endif /* RCC_CCIPR_LPTIM2SEL */

#define IS_RCC_ADCCLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_ADCCLKSOURCE_PLLADC)  || \
                ((__SOURCE__) == RCC_ADCCLKSOURCE_SYSCLK)  || \
                ((__SOURCE__) == RCC_ADCCLKSOURCE_HSI))

#if defined(RNG)
#define IS_RCC_RNGCLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_RNGCLKSOURCE_NONE)         || \
                ((__SOURCE__) == RCC_RNGCLKSOURCE_HSI_DIV8)     || \
                ((__SOURCE__) == RCC_RNGCLKSOURCE_SYSCLK)       || \
                ((__SOURCE__) == RCC_RNGCLKSOURCE_PLL))
#define IS_RCC_RNGDIV(__DIV__)  \
               (((__DIV__) == RCC_RNGCLK_DIV1) || \
                ((__DIV__) == RCC_RNGCLK_DIV2) || \
                ((__DIV__) == RCC_RNGCLK_DIV4) || \
                ((__DIV__) == RCC_RNGCLK_DIV8))
#endif /* RNG */

#if defined(CEC)
#define IS_RCC_CECCLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_CECCLKSOURCE_HSI_DIV488)|| \
                ((__SOURCE__) == RCC_CECCLKSOURCE_LSE))
#endif /* CEC */

#if (RCC_CCIPR_TIM1SEL)
#define IS_RCC_TIM1CLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_TIM1CLKSOURCE_PLL)     || \
                ((__SOURCE__) == RCC_TIM1CLKSOURCE_PCLK1))
#endif /* RCC_CCIPR_TIM1SEL */

#if (RCC_CCIPR_TIM15SEL)
#define IS_RCC_TIM15CLKSOURCE(__SOURCE__)  \
               (((__SOURCE__) == RCC_TIM15CLKSOURCE_PLL)     || \
                ((__SOURCE__) == RCC_TIM15CLKSOURCE_PCLK1))
#endif /* RCC_CCIPR_TIM15SEL */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32G0xx_HAL_RCC_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
