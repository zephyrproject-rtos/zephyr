/**
  ******************************************************************************
  * @file    stm32l0xx_hal_rcc_ex.h
  * @author  MCD Application Team
  * @brief   Header file of RCC HAL Extension module.
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
#ifndef __STM32L0xx_HAL_RCC_EX_H
#define __STM32L0xx_HAL_RCC_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal_def.h"

/** @addtogroup STM32L0xx_HAL_Driver
  * @{
  */

/** @addtogroup RCCEx
  * @{
  */ 

/** @addtogroup RCCEx_Private_Constants
 * @{
 */


#if defined(CRS)
/* CRS IT Error Mask */
#define  RCC_CRS_IT_ERROR_MASK  ((uint32_t)(RCC_CRS_IT_TRIMOVF | RCC_CRS_IT_SYNCERR | RCC_CRS_IT_SYNCMISS))

/* CRS Flag Error Mask */
#define RCC_CRS_FLAG_ERROR_MASK ((uint32_t)(RCC_CRS_FLAG_TRIMOVF | RCC_CRS_FLAG_SYNCERR | RCC_CRS_FLAG_SYNCMISS))

#endif /* CRS */
/**
  * @}
  */

/** @addtogroup RCCEx_Private_Macros
  * @{
  */
#if defined (STM32L052xx) || defined(STM32L062xx)
#define IS_RCC_PERIPHCLOCK(__CLK__) ((__CLK__) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                                 RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_I2C2 | RCC_PERIPHCLK_RTC       |  \
                                                 RCC_PERIPHCLK_USB | RCC_PERIPHCLK_LPTIM1))
#elif defined (STM32L053xx) || defined(STM32L063xx)
#define IS_RCC_PERIPHCLOCK(__CLK__) ((__CLK__) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                                 RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_I2C2 | RCC_PERIPHCLK_RTC       |  \
                                                 RCC_PERIPHCLK_USB | RCC_PERIPHCLK_LPTIM1 | RCC_PERIPHCLK_LCD))
#elif defined (STM32L072xx) || defined(STM32L082xx)
#define IS_RCC_PERIPHCLOCK(__CLK__) ((__CLK__) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                                 RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_I2C2 | RCC_PERIPHCLK_RTC       |  \
                                                 RCC_PERIPHCLK_USB | RCC_PERIPHCLK_LPTIM1  | RCC_PERIPHCLK_I2C3 ))
#elif defined (STM32L073xx) || defined(STM32L083xx)
#define IS_RCC_PERIPHCLOCK(__CLK__) ((__CLK__) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                                 RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_I2C2 | RCC_PERIPHCLK_RTC  |  \
                                                 RCC_PERIPHCLK_USB | RCC_PERIPHCLK_LPTIM1  | RCC_PERIPHCLK_I2C3 | \
                                                 RCC_PERIPHCLK_LCD))
#endif

#if defined(STM32L011xx) || defined(STM32L021xx) || defined(STM32L031xx) || defined(STM32L041xx)
#define IS_RCC_PERIPHCLOCK(__CLK__) ((__CLK__) <= ( RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                                  RCC_PERIPHCLK_I2C1   |  RCC_PERIPHCLK_RTC    | \
                                                  RCC_PERIPHCLK_LPTIM1))
#elif defined(STM32L051xx) || defined(STM32L061xx)
#define IS_RCC_PERIPHCLOCK(__CLK__) ((__CLK__) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                                 RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_I2C2 | RCC_PERIPHCLK_RTC       |  \
                                                 RCC_PERIPHCLK_LPTIM1))
#elif defined(STM32L071xx) || defined(STM32L081xx)
#define IS_RCC_PERIPHCLOCK(__CLK__) ((__CLK__) <= (RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                                 RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_I2C2 | RCC_PERIPHCLK_RTC       |  \
                                                 RCC_PERIPHCLK_LPTIM1 | RCC_PERIPHCLK_I2C3))                               
#endif

#if defined (RCC_CCIPR_USART1SEL)
#define IS_RCC_USART1CLKSOURCE(__SOURCE__)  (((__SOURCE__) == RCC_USART1CLKSOURCE_PCLK2)  || \
                                             ((__SOURCE__) == RCC_USART1CLKSOURCE_SYSCLK) || \
                                             ((__SOURCE__) == RCC_USART1CLKSOURCE_LSE)    || \
                                             ((__SOURCE__) == RCC_USART1CLKSOURCE_HSI))
#endif /* RCC_CCIPR_USART1SEL */

#define IS_RCC_USART2CLKSOURCE(__SOURCE__)  (((__SOURCE__) == RCC_USART2CLKSOURCE_PCLK1)  || \
                                             ((__SOURCE__) == RCC_USART2CLKSOURCE_SYSCLK) || \
                                             ((__SOURCE__) == RCC_USART2CLKSOURCE_LSE)    || \
                                             ((__SOURCE__) == RCC_USART2CLKSOURCE_HSI))
    
#define IS_RCC_LPUART1CLKSOURCE(__SOURCE__) (((__SOURCE__) == RCC_LPUART1CLKSOURCE_PCLK1)  || \
                                             ((__SOURCE__) == RCC_LPUART1CLKSOURCE_SYSCLK) || \
                                             ((__SOURCE__) == RCC_LPUART1CLKSOURCE_LSE)    || \
                                             ((__SOURCE__) == RCC_LPUART1CLKSOURCE_HSI))

#define IS_RCC_I2C1CLKSOURCE(__SOURCE__) (((__SOURCE__) == RCC_I2C1CLKSOURCE_PCLK1) || \
                                          ((__SOURCE__) == RCC_I2C1CLKSOURCE_SYSCLK)|| \
                                          ((__SOURCE__) == RCC_I2C1CLKSOURCE_HSI))
                                          
#if defined(RCC_CCIPR_I2C3SEL)  
#define IS_RCC_I2C3CLKSOURCE(__SOURCE__)  (((__SOURCE__) == RCC_I2C3CLKSOURCE_PCLK1) || \
                                           ((__SOURCE__) == RCC_I2C3CLKSOURCE_SYSCLK)|| \
                                           ((__SOURCE__) == RCC_I2C3CLKSOURCE_HSI))
#endif /* RCC_CCIPR_I2C3SEL */
                                           
#if defined(USB)
#define IS_RCC_USBCLKSOURCE(__SOURCE__)  (((__SOURCE__) == RCC_USBCLKSOURCE_HSI48) || \
                                          ((__SOURCE__) == RCC_USBCLKSOURCE_PLL))
#endif /* USB */

#if defined(RNG)
#define IS_RCC_RNGCLKSOURCE(_SOURCE_)  (((_SOURCE_) == RCC_RNGCLKSOURCE_HSI48) || \
                                      ((_SOURCE_) == RCC_RNGCLKSOURCE_PLLCLK))
#endif /* RNG */
                                      
#if defined(RCC_CCIPR_HSI48SEL)
#define IS_RCC_HSI48MCLKSOURCE(__HSI48MCLK__) (((__HSI48MCLK__) == RCC_HSI48M_PLL) || ((__HSI48MCLK__) == RCC_HSI48M_HSI48))
#endif /* RCC_CCIPR_HSI48SEL */
                                          
#define IS_RCC_LPTIMCLK(__LPTIMCLK_)     (((__LPTIMCLK_) == RCC_LPTIM1CLKSOURCE_PCLK) || \
                                          ((__LPTIMCLK_) == RCC_LPTIM1CLKSOURCE_LSI)  || \
                                          ((__LPTIMCLK_) == RCC_LPTIM1CLKSOURCE_HSI)  || \
                                          ((__LPTIMCLK_) == RCC_LPTIM1CLKSOURCE_LSE))

#define IS_RCC_STOPWAKEUP_CLOCK(__SOURCE__) (((__SOURCE__) == RCC_STOP_WAKEUPCLOCK_MSI) || \
                                             ((__SOURCE__) == RCC_STOP_WAKEUPCLOCK_HSI))

#define IS_RCC_LSE_DRIVE(__DRIVE__) (((__DRIVE__) == RCC_LSEDRIVE_LOW)        || ((__SOURCE__) == RCC_LSEDRIVE_MEDIUMLOW) || \
                                     ((__DRIVE__) == RCC_LSEDRIVE_MEDIUMHIGH) || ((__SOURCE__) == RCC_LSEDRIVE_HIGH))

#if defined(CRS)

#define IS_RCC_CRS_SYNC_SOURCE(_SOURCE_) (((_SOURCE_) == RCC_CRS_SYNC_SOURCE_GPIO) || \
                                          ((_SOURCE_) == RCC_CRS_SYNC_SOURCE_LSE)  || \
                                          ((_SOURCE_) == RCC_CRS_SYNC_SOURCE_USB))
#define IS_RCC_CRS_SYNC_DIV(_DIV_) (((_DIV_) == RCC_CRS_SYNC_DIV1)  || ((_DIV_) == RCC_CRS_SYNC_DIV2)  || \
                                    ((_DIV_) == RCC_CRS_SYNC_DIV4)  || ((_DIV_) == RCC_CRS_SYNC_DIV8)  || \
                                    ((_DIV_) == RCC_CRS_SYNC_DIV16) || ((_DIV_) == RCC_CRS_SYNC_DIV32) || \
                                    ((_DIV_) == RCC_CRS_SYNC_DIV64) || ((_DIV_) == RCC_CRS_SYNC_DIV128))
#define IS_RCC_CRS_SYNC_POLARITY(_POLARITY_) (((_POLARITY_) == RCC_CRS_SYNC_POLARITY_RISING) || \
                                              ((_POLARITY_) == RCC_CRS_SYNC_POLARITY_FALLING))
#define IS_RCC_CRS_RELOADVALUE(_VALUE_) (((_VALUE_) <= 0xFFFF))
#define IS_RCC_CRS_ERRORLIMIT(_VALUE_) (((_VALUE_) <= 0xFF))
#define IS_RCC_CRS_HSI48CALIBRATION(_VALUE_) (((_VALUE_) <= 0x3F))
#define IS_RCC_CRS_FREQERRORDIR(_DIR_) (((_DIR_) == RCC_CRS_FREQERRORDIR_UP) || \
                                        ((_DIR_) == RCC_CRS_FREQERRORDIR_DOWN))
#endif /* CRS */
/**
  * @}
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
  uint32_t PeriphClockSelection;                /*!< The Extended Clock to be configured.
                                      This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */

  uint32_t RTCClockSelection;         /*!< specifies the RTC clock source.
                                       This parameter can be a value of @ref RCC_RTC_LCD_Clock_Source */

#if defined(LCD)

  uint32_t LCDClockSelection;         /*!< specifies the LCD clock source.
                                       This parameter can be a value of @ref RCC_RTC_LCD_Clock_Source */

#endif /* LCD */
#if defined(RCC_CCIPR_USART1SEL)
  uint32_t Usart1ClockSelection;   /*!< USART1 clock source      
                                        This parameter can be a value of @ref RCCEx_USART1_Clock_Source */
#endif /* RCC_CCIPR_USART1SEL */
  uint32_t Usart2ClockSelection;   /*!< USART2 clock source      
                                        This parameter can be a value of @ref RCCEx_USART2_Clock_Source */
                                   
  uint32_t Lpuart1ClockSelection;  /*!< LPUART1 clock source      
                                        This parameter can be a value of @ref RCCEx_LPUART1_Clock_Source */
                                   
  uint32_t I2c1ClockSelection;     /*!< I2C1 clock source      
                                        This parameter can be a value of @ref RCCEx_I2C1_Clock_Source */
                                   
#if defined(RCC_CCIPR_I2C3SEL)
  uint32_t I2c3ClockSelection;     /*!< I2C3 clock source      
                                        This parameter can be a value of @ref RCCEx_I2C3_Clock_Source */
#endif /* RCC_CCIPR_I2C3SEL */
  uint32_t LptimClockSelection;    /*!< LPTIM1 clock source
                                        This parameter can be a value of @ref RCCEx_LPTIM1_Clock_Source */
#if defined(USB)
  uint32_t UsbClockSelection;      /*!< Specifies USB and RNG Clock  Selection
                                        This parameter can be a value of @ref RCCEx_USB_Clock_Source */
#endif /* USB */
} RCC_PeriphCLKInitTypeDef;

#if defined (CRS)
/** 
  * @brief RCC_CRS Init structure definition  
  */
typedef struct
{
  uint32_t Prescaler;             /*!< Specifies the division factor of the SYNC signal.
                                     This parameter can be a value of @ref RCCEx_CRS_SynchroDivider */

  uint32_t Source;                /*!< Specifies the SYNC signal source.
                                     This parameter can be a value of @ref RCCEx_CRS_SynchroSource */

  uint32_t Polarity;              /*!< Specifies the input polarity for the SYNC signal source.
                                     This parameter can be a value of @ref RCCEx_CRS_SynchroPolarity */

  uint32_t ReloadValue;           /*!< Specifies the value to be loaded in the frequency error counter with each SYNC event.
                                      It can be calculated in using macro @ref __HAL_RCC_CRS_RELOADVALUE_CALCULATE(__FTARGET__, __FSYNC__)
                                     This parameter must be a number between 0 and 0xFFFF or a value of @ref RCCEx_CRS_ReloadValueDefault .*/

  uint32_t ErrorLimitValue;       /*!< Specifies the value to be used to evaluate the captured frequency error value.
                                     This parameter must be a number between 0 and 0xFF or a value of @ref RCCEx_CRS_ErrorLimitDefault */

  uint32_t HSI48CalibrationValue; /*!< Specifies a user-programmable trimming value to the HSI48 oscillator.
                                     This parameter must be a number between 0 and 0x3F or a value of @ref RCCEx_CRS_HSI48CalibrationDefault */

}RCC_CRSInitTypeDef;

/** 
  * @brief RCC_CRS Synchronization structure definition  
  */
typedef struct
{
  uint32_t ReloadValue;           /*!< Specifies the value loaded in the Counter reload value.
                                     This parameter must be a number between 0 and 0xFFFF */

  uint32_t HSI48CalibrationValue; /*!< Specifies value loaded in HSI48 oscillator smooth trimming.
                                     This parameter must be a number between 0 and 0x3F */

  uint32_t FreqErrorCapture;      /*!< Specifies the value loaded in the .FECAP, the frequency error counter 
                                                                    value latched in the time of the last SYNC event.
                                    This parameter must be a number between 0 and 0xFFFF */

  uint32_t FreqErrorDirection;    /*!< Specifies the value loaded in the .FEDIR, the counting direction of the 
                                                                    frequency error counter latched in the time of the last SYNC event. 
                                                                    It shows whether the actual frequency is below or above the target.
                                    This parameter must be a value of @ref RCCEx_CRS_FreqErrorDirection*/

}RCC_CRSSynchroInfoTypeDef;

#endif /* CRS */

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup RCCEx_Exported_Constants RCCEx Exported Constants
  * @{
  */


/** @defgroup RCCEx_EXTI_LINE_LSECSS  RCC LSE CSS external interrupt line
  * @{
  */
#define RCC_EXTI_LINE_LSECSS             (EXTI_IMR_IM19)         /*!< External interrupt line 19 connected to the LSE CSS EXTI Line */
/**
  * @}
  */

/** @defgroup RCCEx_Periph_Clock_Selection  RCCEx Periph Clock Selection
  * @{
  */
#if defined(RCC_CCIPR_USART1SEL)
#define RCC_PERIPHCLK_USART1           ((uint32_t)0x00000001)
#endif /* RCC_CCIPR_USART1SEL */
#define RCC_PERIPHCLK_USART2           ((uint32_t)0x00000002)
#define RCC_PERIPHCLK_LPUART1          ((uint32_t)0x00000004)
#define RCC_PERIPHCLK_I2C1             ((uint32_t)0x00000008)
#define RCC_PERIPHCLK_I2C2             ((uint32_t)0x00000010)
#define RCC_PERIPHCLK_RTC              ((uint32_t)0x00000020)
#if defined(USB)
#define RCC_PERIPHCLK_USB              ((uint32_t)0x00000040)
#endif /* USB */
#define RCC_PERIPHCLK_LPTIM1           ((uint32_t)0x00000080)
#if defined(LCD)
#define RCC_PERIPHCLK_LCD              ((uint32_t)0x00000800)
#endif /* LCD */
#if defined(RCC_CCIPR_I2C3SEL)
#define RCC_PERIPHCLK_I2C3             ((uint32_t)0x00000100)
#endif /* RCC_CCIPR_I2C3SEL */

/**
  * @}
  */

#if defined (RCC_CCIPR_USART1SEL)
/** @defgroup RCCEx_USART1_Clock_Source RCCEx USART1 Clock Source
  * @{
  */
#define RCC_USART1CLKSOURCE_PCLK2        (0x00000000U) 
#define RCC_USART1CLKSOURCE_SYSCLK       RCC_CCIPR_USART1SEL_0
#define RCC_USART1CLKSOURCE_HSI          RCC_CCIPR_USART1SEL_1
#define RCC_USART1CLKSOURCE_LSE          (RCC_CCIPR_USART1SEL_0 | RCC_CCIPR_USART1SEL_1)
/**
  * @}
  */
#endif /* RCC_CCIPR_USART1SEL */

/** @defgroup RCCEx_USART2_Clock_Source RCCEx USART2 Clock Source
  * @{
  */
#define RCC_USART2CLKSOURCE_PCLK1        (0x00000000U) 
#define RCC_USART2CLKSOURCE_SYSCLK       RCC_CCIPR_USART2SEL_0
#define RCC_USART2CLKSOURCE_HSI          RCC_CCIPR_USART2SEL_1
#define RCC_USART2CLKSOURCE_LSE          (RCC_CCIPR_USART2SEL_0 | RCC_CCIPR_USART2SEL_1)
/**
  * @}
  */

/** @defgroup RCCEx_LPUART1_Clock_Source RCCEx LPUART1 Clock Source
  * @{
  */
#define RCC_LPUART1CLKSOURCE_PCLK1        (0x00000000U) 
#define RCC_LPUART1CLKSOURCE_SYSCLK       RCC_CCIPR_LPUART1SEL_0
#define RCC_LPUART1CLKSOURCE_HSI          RCC_CCIPR_LPUART1SEL_1
#define RCC_LPUART1CLKSOURCE_LSE          (RCC_CCIPR_LPUART1SEL_0 | RCC_CCIPR_LPUART1SEL_1)
/**
  * @}
  */

/** @defgroup RCCEx_I2C1_Clock_Source RCCEx I2C1 Clock Source
  * @{
  */
#define RCC_I2C1CLKSOURCE_PCLK1          (0x00000000U) 
#define RCC_I2C1CLKSOURCE_SYSCLK         RCC_CCIPR_I2C1SEL_0
#define RCC_I2C1CLKSOURCE_HSI            RCC_CCIPR_I2C1SEL_1
/**
  * @}
  */

#if defined(RCC_CCIPR_I2C3SEL)  

/** @defgroup RCCEx_I2C3_Clock_Source RCCEx I2C3 Clock Source
  * @{
  */
#define RCC_I2C3CLKSOURCE_PCLK1          (0x00000000U) 
#define RCC_I2C3CLKSOURCE_SYSCLK         RCC_CCIPR_I2C3SEL_0
#define RCC_I2C3CLKSOURCE_HSI            RCC_CCIPR_I2C3SEL_1
/**
  * @}
  */
#endif /* RCC_CCIPR_I2C3SEL */ 

/** @defgroup RCCEx_TIM_PRescaler_Selection  RCCEx TIM Prescaler Selection
  * @{
  */
#define RCC_TIMPRES_DESACTIVATED        ((uint8_t)0x00)
#define RCC_TIMPRES_ACTIVATED           ((uint8_t)0x01)
/**
  * @}
  */

#if defined(USB)
/** @defgroup RCCEx_USB_Clock_Source RCCEx USB Clock Source
  * @{
  */
#define RCC_USBCLKSOURCE_HSI48           RCC_CCIPR_HSI48SEL
#define RCC_USBCLKSOURCE_PLL             (0x00000000U)
/**
  * @}
  */
#endif /* USB */
  
#if defined(RNG)
/** @defgroup RCCEx_RNG_Clock_Source RCCEx RNG Clock Source
  * @{
  */
#define RCC_RNGCLKSOURCE_HSI48           RCC_CCIPR_HSI48SEL
#define RCC_RNGCLKSOURCE_PLLCLK          (0x00000000U)
/**
  * @}
  */  
#endif /* RNG */

#if defined(RCC_CCIPR_HSI48SEL)
/** @defgroup RCCEx_HSI48M_Clock_Source RCCEx HSI48M Clock Source
  * @{
  */
#define RCC_FLAG_HSI48                   SYSCFG_CFGR3_VREFINT_RDYF

#define RCC_HSI48M_PLL                   (0x00000000U)
#define RCC_HSI48M_HSI48                 RCC_CCIPR_HSI48SEL

/**
  * @}
  */
#endif /* RCC_CCIPR_HSI48SEL */ 

/** @defgroup RCCEx_LPTIM1_Clock_Source RCCEx LPTIM1 Clock Source
  * @{
  */
#define RCC_LPTIM1CLKSOURCE_PCLK         (0x00000000U)
#define RCC_LPTIM1CLKSOURCE_LSI          RCC_CCIPR_LPTIM1SEL_0
#define RCC_LPTIM1CLKSOURCE_HSI          RCC_CCIPR_LPTIM1SEL_1
#define RCC_LPTIM1CLKSOURCE_LSE          RCC_CCIPR_LPTIM1SEL
/**
  * @}
  */

/** @defgroup RCCEx_StopWakeUp_Clock RCCEx StopWakeUp Clock
  * @{
  */

#define RCC_STOP_WAKEUPCLOCK_MSI         (0x00000000U)
#define RCC_STOP_WAKEUPCLOCK_HSI         RCC_CFGR_STOPWUCK
/**
  * @}
  */ 

/** @defgroup RCCEx_LSEDrive_Configuration RCCEx LSE Drive Configuration
  * @{
  */

#define RCC_LSEDRIVE_LOW                 (0x00000000U)
#define RCC_LSEDRIVE_MEDIUMLOW           RCC_CSR_LSEDRV_0
#define RCC_LSEDRIVE_MEDIUMHIGH          RCC_CSR_LSEDRV_1
#define RCC_LSEDRIVE_HIGH                RCC_CSR_LSEDRV
/**
  * @}
  */  

#if defined(CRS)

/** @defgroup RCCEx_CRS_Status RCCEx CRS Status
  * @{
  */
#define RCC_CRS_NONE      (0x00000000U)
#define RCC_CRS_TIMEOUT   ((uint32_t)0x00000001)
#define RCC_CRS_SYNCOK    ((uint32_t)0x00000002)
#define RCC_CRS_SYNCWARN  ((uint32_t)0x00000004)
#define RCC_CRS_SYNCERR   ((uint32_t)0x00000008)
#define RCC_CRS_SYNCMISS  ((uint32_t)0x00000010)
#define RCC_CRS_TRIMOVF   ((uint32_t)0x00000020)

/**
  * @}
  */

/** @defgroup RCCEx_CRS_SynchroSource RCCEx CRS Synchronization Source
  * @{
  */
#define RCC_CRS_SYNC_SOURCE_GPIO       ((uint32_t)0x00000000U) /*!< Synchro Signal source GPIO */
#define RCC_CRS_SYNC_SOURCE_LSE        CRS_CFGR_SYNCSRC_0      /*!< Synchro Signal source LSE */
#define RCC_CRS_SYNC_SOURCE_USB        CRS_CFGR_SYNCSRC_1      /*!< Synchro Signal source USB SOF (default)*/
/**
  * @}
  */

/** @defgroup RCCEx_CRS_SynchroDivider RCCEx CRS Synchronization Divider
  * @{
  */
#define RCC_CRS_SYNC_DIV1        ((uint32_t)0x00000000U)                   /*!< Synchro Signal not divided (default) */
#define RCC_CRS_SYNC_DIV2        CRS_CFGR_SYNCDIV_0                        /*!< Synchro Signal divided by 2 */
#define RCC_CRS_SYNC_DIV4        CRS_CFGR_SYNCDIV_1                        /*!< Synchro Signal divided by 4 */
#define RCC_CRS_SYNC_DIV8        (CRS_CFGR_SYNCDIV_1 | CRS_CFGR_SYNCDIV_0) /*!< Synchro Signal divided by 8 */
#define RCC_CRS_SYNC_DIV16       CRS_CFGR_SYNCDIV_2                        /*!< Synchro Signal divided by 16 */
#define RCC_CRS_SYNC_DIV32       (CRS_CFGR_SYNCDIV_2 | CRS_CFGR_SYNCDIV_0) /*!< Synchro Signal divided by 32 */
#define RCC_CRS_SYNC_DIV64       (CRS_CFGR_SYNCDIV_2 | CRS_CFGR_SYNCDIV_1) /*!< Synchro Signal divided by 64 */
#define RCC_CRS_SYNC_DIV128      CRS_CFGR_SYNCDIV                          /*!< Synchro Signal divided by 128 */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_SynchroPolarity RCCEx CRS Synchronization Polarity
  * @{
  */
#define RCC_CRS_SYNC_POLARITY_RISING   ((uint32_t)0x00000000U) /*!< Synchro Active on rising edge (default) */
#define RCC_CRS_SYNC_POLARITY_FALLING  CRS_CFGR_SYNCPOL        /*!< Synchro Active on falling edge */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_ReloadValueDefault RCCEx CRS Default Reload Value
  * @{
  */
#define RCC_CRS_RELOADVALUE_DEFAULT    ((uint32_t)0x0000BB7FU) /*!< The reset value of the RELOAD field corresponds 
                                                                    to a target frequency of 48 MHz and a synchronization signal frequency of 1 kHz (SOF signal from USB). */
/**
  * @}
  */
  
/** @defgroup RCCEx_CRS_ErrorLimitDefault RCCEx CRS Default Error Limit Value
  * @{
  */
#define RCC_CRS_ERRORLIMIT_DEFAULT     ((uint32_t)0x00000022U) /*!< Default Frequency error limit */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_HSI48CalibrationDefault RCCEx CRS Default HSI48 Calibration vakye
  * @{
  */
#define RCC_CRS_HSI48CALIBRATION_DEFAULT ((uint32_t)0x00000020U) /*!< The default value is 32, which corresponds to the middle of the trimming interval. 
                                                                      The trimming step is around 67 kHz between two consecutive TRIM steps. A higher TRIM value
                                                                      corresponds to a higher output frequency */  
/**
  * @}
  */

/** @defgroup RCCEx_CRS_FreqErrorDirection RCCEx CRS Frequency Error Direction
  * @{
  */
#define RCC_CRS_FREQERRORDIR_UP        ((uint32_t)0x00000000U)   /*!< Upcounting direction, the actual frequency is above the target */
#define RCC_CRS_FREQERRORDIR_DOWN      ((uint32_t)CRS_ISR_FEDIR) /*!< Downcounting direction, the actual frequency is below the target */
/**
  * @}
  */

/** @defgroup RCCEx_CRS_Interrupt_Sources RCCEx CRS Interrupt Sources
  * @{
  */
#define RCC_CRS_IT_SYNCOK              CRS_CR_SYNCOKIE           /*!< SYNC event OK */
#define RCC_CRS_IT_SYNCWARN            CRS_CR_SYNCWARNIE         /*!< SYNC warning */
#define RCC_CRS_IT_ERR                 CRS_CR_ERRIE              /*!< Error */
#define RCC_CRS_IT_ESYNC               CRS_CR_ESYNCIE            /*!< Expected SYNC */
#define RCC_CRS_IT_SYNCERR             CRS_CR_ERRIE              /*!< SYNC error */
#define RCC_CRS_IT_SYNCMISS            CRS_CR_ERRIE              /*!< SYNC missed */
#define RCC_CRS_IT_TRIMOVF             CRS_CR_ERRIE              /*!< Trimming overflow or underflow */

/**
  * @}
  */
  
/** @defgroup RCCEx_CRS_Flags RCCEx CRS Flags
  * @{
  */
#define RCC_CRS_FLAG_SYNCOK            CRS_ISR_SYNCOKF           /*!< SYNC event OK flag     */
#define RCC_CRS_FLAG_SYNCWARN          CRS_ISR_SYNCWARNF         /*!< SYNC warning flag      */
#define RCC_CRS_FLAG_ERR               CRS_ISR_ERRF              /*!< Error flag        */
#define RCC_CRS_FLAG_ESYNC             CRS_ISR_ESYNCF            /*!< Expected SYNC flag     */
#define RCC_CRS_FLAG_SYNCERR           CRS_ISR_SYNCERR           /*!< SYNC error */
#define RCC_CRS_FLAG_SYNCMISS          CRS_ISR_SYNCMISS          /*!< SYNC missed*/
#define RCC_CRS_FLAG_TRIMOVF           CRS_ISR_TRIMOVF           /*!< Trimming overflow or underflow */

/**
  * @}
  */

#endif /* CRS */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Macros RCCEx Exported Macros
 * @{
 */

/** @defgroup RCCEx_Peripheral_Clock_Enable_Disable AHB Peripheral Clock Enable Disable
  * @brief  Enable or disable the AHB peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */

#if defined(STM32L062xx) || defined(STM32L063xx)|| defined(STM32L082xx) || defined(STM32L083xx) || defined(STM32L041xx) || defined(STM32L021xx)
#define __HAL_RCC_AES_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_CRYPEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_CRYPEN);\
                                        UNUSED(tmpreg); \
                                      } while(0)
#define __HAL_RCC_AES_CLK_DISABLE()         CLEAR_BIT(RCC->AHBENR, (RCC_AHBENR_CRYPEN))

#define __HAL_RCC_AES_IS_CLK_ENABLED()        (READ_BIT(RCC->AHBENR, RCC_AHBENR_CRYPEN) != RESET)
#define __HAL_RCC_AES_IS_CLK_DISABLED()       (READ_BIT(RCC->AHBENR, RCC_AHBENR_CRYPEN) == RESET)

#endif /* STM32L062xx || STM32L063xx || STM32L072xx  || STM32L073xx || STM32L082xx  || STM32L083xx || STM32L041xx || STM32L021xx */

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) 
#define __HAL_RCC_TSC_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_TSCEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_TSCEN);\
                                        UNUSED(tmpreg); \
                                      } while(0)
#define __HAL_RCC_TSC_CLK_DISABLE()            CLEAR_BIT(RCC->AHBENR, (RCC_AHBENR_TSCEN))

#define __HAL_RCC_TSC_IS_CLK_ENABLED()        (READ_BIT(RCC->AHBENR, RCC_AHBENR_TSCEN) != RESET)
#define __HAL_RCC_TSC_IS_CLK_DISABLED()       (READ_BIT(RCC->AHBENR, RCC_AHBENR_TSCEN) == RESET)

#define __HAL_RCC_RNG_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_RNGEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_RNGEN);\
                                        UNUSED(tmpreg); \
                                      } while(0)
#define __HAL_RCC_RNG_CLK_DISABLE()           CLEAR_BIT(RCC->AHBENR, (RCC_AHBENR_RNGEN))

#define __HAL_RCC_RNG_IS_CLK_ENABLED()        (READ_BIT(RCC->AHBENR, RCC_AHBENR_RNGEN) != RESET)
#define __HAL_RCC_RNG_IS_CLK_DISABLED()       (READ_BIT(RCC->AHBENR, RCC_AHBENR_RNGEN) == RESET)
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx ) && !(STM32L071xx ) && !(STM32L081xx ) */

/**
  * @}
  */

/** @defgroup RCCEx_IOPORT_Clock_Enable_Disable IOPORT Peripheral Clock Enable Disable
  * @brief  Enable or disable the IOPORT peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before 
  *         using it.
  * @{
  */
#if defined(GPIOE)
#define __HAL_RCC_GPIOE_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIOEEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOEEN);\
                                        UNUSED(tmpreg); \
                                      } while(0)

#define __HAL_RCC_GPIOE_CLK_DISABLE()        CLEAR_BIT(RCC->IOPENR,(RCC_IOPENR_GPIOEEN))

#define __HAL_RCC_GPIOE_IS_CLK_ENABLED()        (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOEEN) != RESET)
#define __HAL_RCC_GPIOE_IS_CLK_DISABLED()       (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOEEN) == RESET)

#endif /* GPIOE */
#if defined(GPIOD)
#define __HAL_RCC_GPIOD_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIODEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIODEN);\
                                        UNUSED(tmpreg); \
                                      } while(0)
#define __HAL_RCC_GPIOD_CLK_DISABLE()        CLEAR_BIT(RCC->IOPENR,(RCC_IOPENR_GPIODEN))

#define __HAL_RCC_GPIOD_IS_CLK_ENABLED()        (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIODEN) != RESET)
#define __HAL_RCC_GPIOD_IS_CLK_DISABLED()       (READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIODEN) == RESET)

#endif  /* GPIOD */
/**
  * @}
  */

/** @defgroup RCCEx_APB1_Clock_Enable_Disable APB1 Peripheral Clock Enable Disable                
  * @brief  Enable or disable the APB1 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before 
  *         using it.   
  * @{
  */

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) 
#define __HAL_RCC_USB_CLK_ENABLE()        SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_USBEN))
#define __HAL_RCC_USB_CLK_DISABLE()       CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_USBEN))

#define __HAL_RCC_USB_IS_CLK_ENABLED()        (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USBEN) != RESET)
#define __HAL_RCC_USB_IS_CLK_DISABLED()       (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USBEN) == RESET)

#define __HAL_RCC_CRS_CLK_ENABLE()     SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_CRSEN))
#define __HAL_RCC_CRS_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR,(RCC_APB1ENR_CRSEN))

#define __HAL_RCC_CRS_IS_CLK_ENABLED()        (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_CRSEN) != RESET)
#define __HAL_RCC_CRS_IS_CLK_DISABLED()       (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_CRSEN) == RESET)

#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx ) && !(STM32L071xx ) && !(STM32L081xx ) */
       

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)
#define __HAL_RCC_LCD_CLK_ENABLE()          SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_LCDEN))
#define __HAL_RCC_LCD_CLK_DISABLE()         CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_LCDEN))

#define __HAL_RCC_LCD_IS_CLK_ENABLED()        (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LCDEN) != RESET)
#define __HAL_RCC_LCD_IS_CLK_DISABLED()       (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LCDEN) == RESET)

#endif /* STM32L053xx || STM32L063xx || STM32L073xx  || STM32L083xx */

#if defined(STM32L053xx) || defined(STM32L063xx) \
 || defined(STM32L052xx) || defined(STM32L062xx) \
 || defined(STM32L051xx) || defined(STM32L061xx)
#define __HAL_RCC_TIM2_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM2EN))
#define __HAL_RCC_TIM6_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM6EN))
#define __HAL_RCC_SPI2_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_SPI2EN))
#define __HAL_RCC_USART2_CLK_ENABLE()  SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_USART2EN))
#define __HAL_RCC_LPUART1_CLK_ENABLE() SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPUART1EN))
#define __HAL_RCC_I2C1_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C1EN))
#define __HAL_RCC_I2C2_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C2EN))
#define __HAL_RCC_DAC_CLK_ENABLE()     SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_DACEN))
#define __HAL_RCC_LPTIM1_CLK_ENABLE()  SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPTIM1EN))

#define __HAL_RCC_TIM2_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM2EN))
#define __HAL_RCC_TIM6_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM6EN))
#define __HAL_RCC_SPI2_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_SPI2EN))
#define __HAL_RCC_USART2_CLK_DISABLE()  CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_USART2EN))
#define __HAL_RCC_LPUART1_CLK_DISABLE() CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPUART1EN))
#define __HAL_RCC_I2C1_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C1EN))
#define __HAL_RCC_I2C2_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C2EN))
#define __HAL_RCC_DAC_CLK_DISABLE()     CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_DACEN))
#define __HAL_RCC_LPTIM1_CLK_DISABLE()  CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPTIM1EN))

#define __HAL_RCC_TIM2_IS_CLK_ENABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM2EN) != RESET)
#define __HAL_RCC_TIM6_IS_CLK_ENABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM6EN) != RESET)
#define __HAL_RCC_SPI2_IS_CLK_ENABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN) != RESET)
#define __HAL_RCC_USART2_IS_CLK_ENABLED()   (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN) != RESET)
#define __HAL_RCC_LPUART1_IS_CLK_ENABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN) != RESET)
#define __HAL_RCC_I2C1_IS_CLK_ENABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1EN) != RESET)
#define __HAL_RCC_I2C2_IS_CLK_ENABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C2EN) != RESET)
#define __HAL_RCC_DAC_IS_CLK_ENABLED()      (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_DACEN) != RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_ENABLED()   (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPTIM1EN) != RESET)
#define __HAL_RCC_TIM2_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM2EN) == RESET)
#define __HAL_RCC_TIM6_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM6EN) == RESET)
#define __HAL_RCC_SPI2_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN) == RESET)
#define __HAL_RCC_USART2_IS_CLK_DISABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN) == RESET)
#define __HAL_RCC_LPUART1_IS_CLK_DISABLED() (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN) == RESET)
#define __HAL_RCC_I2C1_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1EN) == RESET)
#define __HAL_RCC_I2C2_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C2EN) == RESET)
#define __HAL_RCC_DAC_IS_CLK_DISABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_DACEN) == RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_DISABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPTIM1EN) == RESET)

#endif /* STM32L051xx  || STM32L061xx  ||  */
       /* STM32L052xx  || STM32L062xx  ||  */
       /* STM32L053xx  || STM32L063xx  ||  */

#if defined(STM32L011xx) || defined(STM32L021xx) || defined(STM32L031xx) || defined(STM32L041xx)
#define __HAL_RCC_TIM2_CLK_ENABLE()     SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM2EN))
#define __HAL_RCC_USART2_CLK_ENABLE()   SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_USART2EN))
#define __HAL_RCC_LPUART1_CLK_ENABLE()  SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPUART1EN))
#define __HAL_RCC_I2C1_CLK_ENABLE()     SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C1EN))
#define __HAL_RCC_LPTIM1_CLK_ENABLE()   SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPTIM1EN))

#define __HAL_RCC_TIM2_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM2EN))
#define __HAL_RCC_USART2_CLK_DISABLE()  CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_USART2EN))
#define __HAL_RCC_LPUART1_CLK_DISABLE() CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPUART1EN))
#define __HAL_RCC_I2C1_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C1EN))
#define __HAL_RCC_LPTIM1_CLK_DISABLE()  CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPTIM1EN))

#define __HAL_RCC_TIM2_IS_CLK_ENABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM2EN) != RESET)
#define __HAL_RCC_USART2_IS_CLK_ENABLED()   (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN) != RESET)
#define __HAL_RCC_LPUART1_IS_CLK_ENABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN) != RESET)
#define __HAL_RCC_I2C1_IS_CLK_ENABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1EN) != RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_ENABLED()   (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPTIM1EN) != RESET)
#define __HAL_RCC_TIM2_IS_CLK_DISABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM2EN) == RESET)
#define __HAL_RCC_USART2_IS_CLK_DISABLED()   (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN) == RESET)
#define __HAL_RCC_LPUART1_IS_CLK_DISABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN) == RESET)
#define __HAL_RCC_I2C1_IS_CLK_DISABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1EN) == RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_DISABLED()   (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPTIM1EN) == RESET)

#endif /* STM32L011xx  || STM32L021xx  || STM32L031xx  || STM32L041xx   */


#if defined(STM32L073xx) || defined(STM32L083xx) \
 || defined(STM32L072xx) || defined(STM32L082xx) \
 || defined(STM32L071xx) || defined(STM32L081xx)
#define __HAL_RCC_TIM2_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM2EN))
#define __HAL_RCC_TIM3_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM3EN))
#define __HAL_RCC_TIM6_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM6EN))
#define __HAL_RCC_TIM7_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM7EN))
#define __HAL_RCC_SPI2_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_SPI2EN))
#define __HAL_RCC_USART2_CLK_ENABLE()  SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_USART2EN))
#define __HAL_RCC_USART4_CLK_ENABLE()  SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_USART4EN))
#define __HAL_RCC_USART5_CLK_ENABLE()  SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_USART5EN))
#define __HAL_RCC_LPUART1_CLK_ENABLE() SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPUART1EN))
#define __HAL_RCC_I2C1_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C1EN))
#define __HAL_RCC_I2C2_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C2EN))
#define __HAL_RCC_I2C3_CLK_ENABLE()    SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C3EN))
#define __HAL_RCC_DAC_CLK_ENABLE()     SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_DACEN))
#define __HAL_RCC_LPTIM1_CLK_ENABLE()  SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPTIM1EN))

#define __HAL_RCC_TIM2_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM2EN))
#define __HAL_RCC_TIM3_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM3EN))
#define __HAL_RCC_TIM6_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM6EN))
#define __HAL_RCC_TIM7_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_TIM7EN))
#define __HAL_RCC_SPI2_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_SPI2EN))
#define __HAL_RCC_USART2_CLK_DISABLE()  CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_USART2EN))
#define __HAL_RCC_USART4_CLK_DISABLE()  CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_USART4EN))
#define __HAL_RCC_USART5_CLK_DISABLE()  CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_USART5EN))
#define __HAL_RCC_LPUART1_CLK_DISABLE() CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPUART1EN))
#define __HAL_RCC_I2C1_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C1EN))
#define __HAL_RCC_I2C2_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C2EN))
#define __HAL_RCC_I2C3_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C3EN))
#define __HAL_RCC_DAC_CLK_DISABLE()     CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_DACEN))
#define __HAL_RCC_LPTIM1_CLK_DISABLE()  CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_LPTIM1EN))

#define __HAL_RCC_TIM2_IS_CLK_ENABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM2EN) != RESET)
#define __HAL_RCC_TIM3_IS_CLK_ENABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM3EN) != RESET)
#define __HAL_RCC_TIM6_IS_CLK_ENABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM6EN) != RESET)
#define __HAL_RCC_TIM7_IS_CLK_ENABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM7EN) != RESET)
#define __HAL_RCC_SPI2_IS_CLK_ENABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN) != RESET)
#define __HAL_RCC_USART2_IS_CLK_ENABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN) != RESET)
#define __HAL_RCC_USART4_IS_CLK_ENABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART4EN) != RESET)
#define __HAL_RCC_USART5_IS_CLK_ENABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART5EN) != RESET)
#define __HAL_RCC_LPUART1_IS_CLK_ENABLED() (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN) != RESET)
#define __HAL_RCC_I2C1_IS_CLK_ENABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1EN) != RESET)
#define __HAL_RCC_I2C2_IS_CLK_ENABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C2EN) != RESET)
#define __HAL_RCC_I2C3_IS_CLK_ENABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C3EN) != RESET)
#define __HAL_RCC_DAC_IS_CLK_ENABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_DACEN) != RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_ENABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPTIM1EN) != RESET)
#define __HAL_RCC_TIM2_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM2EN) == RESET)
#define __HAL_RCC_TIM3_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM3EN) == RESET)
#define __HAL_RCC_TIM6_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM6EN) == RESET)
#define __HAL_RCC_TIM7_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM7EN) == RESET)
#define __HAL_RCC_SPI2_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN) == RESET)
#define __HAL_RCC_USART2_IS_CLK_DISABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN) == RESET)
#define __HAL_RCC_USART4_IS_CLK_DISABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART4EN) == RESET)
#define __HAL_RCC_USART5_IS_CLK_DISABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_USART5EN) == RESET)
#define __HAL_RCC_LPUART1_IS_CLK_DISABLED() (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPUART1EN) == RESET)
#define __HAL_RCC_I2C1_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1EN) == RESET)
#define __HAL_RCC_I2C2_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C2EN) == RESET)
#define __HAL_RCC_I2C3_IS_CLK_DISABLED()    (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C3EN) == RESET)
#define __HAL_RCC_DAC_IS_CLK_DISABLED()     (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_DACEN) == RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_DISABLED()  (READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LPTIM1EN) == RESET)

#endif /* STM32L071xx  ||  STM32L081xx  || */
       /* STM32L072xx  ||  STM32L082xx  || */
       /* STM32L073xx  ||  STM32L083xx     */

 /**
  * @}
  */

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx) \
 || defined(STM32L052xx) || defined(STM32L062xx) || defined(STM32L072xx) || defined(STM32L082xx) \
 || defined(STM32L051xx) || defined(STM32L061xx) || defined(STM32L071xx) || defined(STM32L081xx) \
 || defined(STM32L031xx) || defined(STM32L041xx) || defined(STM32L011xx) || defined(STM32L021xx) 
/** @defgroup RCCEx_APB2_Clock_Enable_Disable APB2 Peripheral Clock Enable Disable     
  * @brief  Enable or disable the APB2 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before 
  *         using it.   
  * @{
  */
#define __HAL_RCC_TIM21_CLK_ENABLE()    SET_BIT(RCC->APB2ENR, (RCC_APB2ENR_TIM21EN))
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_CLK_ENABLE()    SET_BIT(RCC->APB2ENR, (RCC_APB2ENR_TIM22EN))
#endif
#define __HAL_RCC_ADC1_CLK_ENABLE()     SET_BIT(RCC->APB2ENR, (RCC_APB2ENR_ADC1EN))
#define __HAL_RCC_SPI1_CLK_ENABLE()     SET_BIT(RCC->APB2ENR, (RCC_APB2ENR_SPI1EN))
#define __HAL_RCC_USART1_CLK_ENABLE()   SET_BIT(RCC->APB2ENR, (RCC_APB2ENR_USART1EN))

#define __HAL_RCC_TIM21_CLK_DISABLE()    CLEAR_BIT(RCC->APB2ENR,  (RCC_APB2ENR_TIM21EN))
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_CLK_DISABLE()    CLEAR_BIT(RCC->APB2ENR,  (RCC_APB2ENR_TIM22EN))
#endif
#define __HAL_RCC_ADC1_CLK_DISABLE()     CLEAR_BIT(RCC->APB2ENR,  (RCC_APB2ENR_ADC1EN))
#define __HAL_RCC_SPI1_CLK_DISABLE()     CLEAR_BIT(RCC->APB2ENR,  (RCC_APB2ENR_SPI1EN))
#define __HAL_RCC_USART1_CLK_DISABLE()   CLEAR_BIT(RCC->APB2ENR,  (RCC_APB2ENR_USART1EN))
#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx)
#define __HAL_RCC_FIREWALL_CLK_ENABLE()  SET_BIT(RCC->APB2ENR, (RCC_APB2ENR_MIFIEN))
#define __HAL_RCC_FIREWALL_CLK_DISABLE() CLEAR_BIT(RCC->APB2ENR,  (RCC_APB2ENR_MIFIEN))
#endif /* !(STM32L011xx) && !(STM32L021xx) && !STM32L031xx && !STM32L041xx */

#define __HAL_RCC_TIM21_IS_CLK_ENABLED()    (READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM21EN) != RESET)
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_IS_CLK_ENABLED()    (READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM22EN) != RESET)
#endif
#define __HAL_RCC_ADC1_IS_CLK_ENABLED()     (READ_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN) != RESET)
#define __HAL_RCC_SPI1_IS_CLK_ENABLED()     (READ_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN) != RESET)
#define __HAL_RCC_USART1_IS_CLK_ENABLED()   (READ_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN) != RESET)

#define __HAL_RCC_TIM21_IS_CLK_DISABLED()    (READ_BIT(RCC->APB2ENR,  (RCC_APB2ENR_TIM21EN) == RESET)
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_IS_CLK_DISABLED()    (READ_BIT(RCC->APB2ENR,  (RCC_APB2ENR_TIM22EN) == RESET)
#endif
#define __HAL_RCC_ADC1_IS_CLK_DISABLED()     (READ_BIT(RCC->APB2ENR,  (RCC_APB2ENR_ADC1EN) == RESET)
#define __HAL_RCC_SPI1_IS_CLK_DISABLED()     (READ_BIT(RCC->APB2ENR,  (RCC_APB2ENR_SPI1EN) == RESET)
#define __HAL_RCC_USART1_IS_CLK_DISABLED()   (READ_BIT(RCC->APB2ENR,  (RCC_APB2ENR_USART1EN) == RESET)
#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx)
#define __HAL_RCC_FIREWALL_IS_CLK_ENABLED()  (READ_BIT(RCC->APB2ENR, RCC_APB2ENR_MIFIEN) != RESET)
#define __HAL_RCC_FIREWALL_IS_CLK_DISABLED() (READ_BIT(RCC->APB2ENR,  (RCC_APB2ENR_MIFIEN) == RESET)
#endif /* !(STM32L011xx) && !(STM32L021xx) && !STM32L031xx && !STM32L041xx */

#endif /* STM32L051xx  || STM32L061xx  || STM32L071xx  ||  STM32L081xx  || */
       /* STM32L052xx  || STM32L062xx  || STM32L072xx  ||  STM32L082xx  || */
       /* STM32L053xx  || STM32L063xx  || STM32L073xx  ||  STM32L083xx  || */
     /* STM32L031xx  || STM32L041xx  || STM32L011xx  || STM32L021xx      */
       
/**
  * @}
  */

/** @defgroup RCCEx_AHB_Force_Release_Reset AHB Peripheral Force Release Reset
  * @brief  Force or release AHB peripheral reset.
  * @{
  */
#if defined(STM32L062xx) || defined(STM32L063xx)|| defined(STM32L082xx) || defined(STM32L083xx) || defined(STM32L041xx) || defined(STM32L021xx)
#define __HAL_RCC_AES_FORCE_RESET()     SET_BIT(RCC->AHBRSTR, (RCC_AHBRSTR_CRYPRST))
#define __HAL_RCC_AES_RELEASE_RESET()   CLEAR_BIT(RCC->AHBRSTR, (RCC_AHBRSTR_CRYPRST))
#endif /* STM32L062xx || STM32L063xx || STM32L072xx  || STM32L073xx || STM32L082xx  || STM32L083xx || STM32L041xx || STM32L021xx*/

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) 
#define __HAL_RCC_TSC_FORCE_RESET()        SET_BIT(RCC->AHBRSTR, (RCC_AHBRSTR_TSCRST))
#define __HAL_RCC_TSC_RELEASE_RESET()      CLEAR_BIT(RCC->AHBRSTR, (RCC_AHBRSTR_TSCRST))
#define __HAL_RCC_RNG_FORCE_RESET()        SET_BIT(RCC->AHBRSTR, (RCC_AHBRSTR_RNGRST))
#define __HAL_RCC_RNG_RELEASE_RESET()      CLEAR_BIT(RCC->AHBRSTR, (RCC_AHBRSTR_RNGRST))
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx ) && !(STM32L071xx ) && !(STM32L081xx ) */

/**
  * @}
  */

/** @defgroup RCCEx_IOPORT_Force_Release_Reset IOPORT Peripheral Force Release Reset
  * @brief  Force or release IOPORT peripheral reset.
  * @{
  */
#if defined(STM32L073xx) || defined(STM32L083xx) \
 || defined(STM32L072xx) || defined(STM32L082xx) \
 || defined(STM32L071xx) || defined(STM32L081xx)
#define __HAL_RCC_GPIOE_FORCE_RESET()   SET_BIT(RCC->IOPRSTR, (RCC_IOPRSTR_GPIOERST))

#define __HAL_RCC_GPIOE_RELEASE_RESET() CLEAR_BIT(RCC->IOPRSTR,(RCC_IOPRSTR_GPIOERST))

#endif /* STM32L071xx  ||  STM32L081xx  || */
       /* STM32L072xx  ||  STM32L082xx  || */
       /* STM32L073xx  ||  STM32L083xx     */
#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx)
#define __HAL_RCC_GPIOD_FORCE_RESET()   SET_BIT(RCC->IOPRSTR, (RCC_IOPRSTR_GPIODRST))
#define __HAL_RCC_GPIOD_RELEASE_RESET() CLEAR_BIT(RCC->IOPRSTR,(RCC_IOPRSTR_GPIODRST))
#endif  /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) */ 
/**
  * @}
  */

/** @defgroup RCCEx_APB1_Force_Release_Reset APB1 Peripheral Force Release Reset     
  * @brief  Force or release APB1 peripheral reset.
  * @{
  */ 

#if defined(STM32L053xx) || defined(STM32L063xx) \
 || defined(STM32L052xx) || defined(STM32L062xx) \
 || defined(STM32L051xx) || defined(STM32L061xx)  
#define __HAL_RCC_TIM2_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM2RST))
#define __HAL_RCC_TIM6_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM6RST))
#define __HAL_RCC_LPTIM1_FORCE_RESET()   SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPTIM1RST))
#define __HAL_RCC_I2C1_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C1RST))
#define __HAL_RCC_I2C2_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C2RST))
#define __HAL_RCC_USART2_FORCE_RESET()   SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_LPUART1_FORCE_RESET()  SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPUART1RST))
#define __HAL_RCC_SPI2_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_SPI2RST))
#define __HAL_RCC_DAC_FORCE_RESET()      SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_DACRST))

#define __HAL_RCC_TIM2_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM2RST))
#define __HAL_RCC_TIM6_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM6RST))
#define __HAL_RCC_LPTIM1_RELEASE_RESET()   CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPTIM1RST))
#define __HAL_RCC_I2C1_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C1RST))
#define __HAL_RCC_I2C2_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C2RST))
#define __HAL_RCC_USART2_RELEASE_RESET()   CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_LPUART1_RELEASE_RESET()  CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPUART1RST))
#define __HAL_RCC_SPI2_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_SPI2RST))
#define __HAL_RCC_DAC_RELEASE_RESET()      CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_DACRST))
#endif /* STM32L051xx  || STM32L061xx  || */
       /* STM32L052xx  || STM32L062xx  || */
       /* STM32L053xx  || STM32L063xx     */
#if defined(STM32L011xx) || defined(STM32L021xx) || defined(STM32L031xx) || defined(STM32L041xx)
#define __HAL_RCC_TIM2_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM2RST))
#define __HAL_RCC_LPTIM1_FORCE_RESET()   SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPTIM1RST))
#define __HAL_RCC_I2C1_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C1RST))
#define __HAL_RCC_USART2_FORCE_RESET()   SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_LPUART1_FORCE_RESET()  SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPUART1RST))

#define __HAL_RCC_TIM2_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM2RST))
#define __HAL_RCC_LPTIM1_RELEASE_RESET()   CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPTIM1RST))
#define __HAL_RCC_I2C1_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C1RST))
#define __HAL_RCC_USART2_RELEASE_RESET()   CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_LPUART1_RELEASE_RESET()  CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPUART1RST))
#endif /* STM32L031xx  || STM32L041xx  || STM32L011xx  || STM32L021xx  */

#if defined(STM32L073xx) || defined(STM32L083xx) \
 || defined(STM32L072xx) || defined(STM32L082xx) \
 || defined(STM32L071xx) || defined(STM32L081xx) 
#define __HAL_RCC_TIM2_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM2RST))
#define __HAL_RCC_TIM3_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM3RST))
#define __HAL_RCC_TIM6_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM6RST))
#define __HAL_RCC_TIM7_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM7RST))
#define __HAL_RCC_LPTIM1_FORCE_RESET()   SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPTIM1RST))
#define __HAL_RCC_I2C1_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C1RST))
#define __HAL_RCC_I2C2_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C2RST))
#define __HAL_RCC_I2C3_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C3RST))
#define __HAL_RCC_USART2_FORCE_RESET()   SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_USART4_FORCE_RESET()   SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USART4RST))
#define __HAL_RCC_USART5_FORCE_RESET()   SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USART5RST))
#define __HAL_RCC_LPUART1_FORCE_RESET()  SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPUART1RST))
#define __HAL_RCC_SPI2_FORCE_RESET()     SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_SPI2RST))
#define __HAL_RCC_DAC_FORCE_RESET()      SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_DACRST))

#define __HAL_RCC_TIM2_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM2RST))
#define __HAL_RCC_TIM3_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM3RST))
#define __HAL_RCC_TIM6_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM6RST))
#define __HAL_RCC_TIM7_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_TIM7RST))
#define __HAL_RCC_LPTIM1_RELEASE_RESET()   CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPTIM1RST))
#define __HAL_RCC_I2C1_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C1RST))
#define __HAL_RCC_I2C2_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C2RST))
#define __HAL_RCC_I2C3_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_I2C3RST))
#define __HAL_RCC_USART2_RELEASE_RESET()   CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_USART4_RELEASE_RESET()   CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USART4RST))
#define __HAL_RCC_USART5_RELEASE_RESET()   CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USART5RST))
#define __HAL_RCC_LPUART1_RELEASE_RESET()  CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LPUART1RST))
#define __HAL_RCC_SPI2_RELEASE_RESET()     CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_SPI2RST))
#define __HAL_RCC_DAC_RELEASE_RESET()      CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_DACRST))
#endif /* STM32L071xx  ||  STM32L081xx  || */
       /* STM32L072xx  ||  STM32L082xx  || */
       /* STM32L073xx  ||  STM32L083xx  || */

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) 
#define __HAL_RCC_USB_FORCE_RESET()        SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USBRST))
#define __HAL_RCC_USB_RELEASE_RESET()      CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_USBRST))
#define __HAL_RCC_CRS_FORCE_RESET()        SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_CRSRST))
#define __HAL_RCC_CRS_RELEASE_RESET()      CLEAR_BIT(RCC->APB1RSTR,(RCC_APB1RSTR_CRSRST))
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx ) && !(STM32L071xx ) && !(STM32L081xx ) */

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)
#define __HAL_RCC_LCD_FORCE_RESET()           SET_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LCDRST))
#define __HAL_RCC_LCD_RELEASE_RESET()         CLEAR_BIT(RCC->APB1RSTR, (RCC_APB1RSTR_LCDRST))
#endif /* STM32L053xx || STM32L063xx || STM32L073xx  || STM32L083xx */

/**
  * @}
  */

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx) \
 || defined(STM32L052xx) || defined(STM32L062xx) || defined(STM32L072xx) || defined(STM32L082xx) \
 || defined(STM32L051xx) || defined(STM32L061xx) || defined(STM32L071xx) || defined(STM32L081xx)

/** @defgroup RCCEx_APB2_Force_Release_Reset APB2 Peripheral Force Release Reset       
  * @brief  Force or release APB2 peripheral reset.
  * @{
  */ 
#define __HAL_RCC_USART1_FORCE_RESET()     SET_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_USART1RST))
#define __HAL_RCC_ADC1_FORCE_RESET()       SET_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_ADC1RST))
#define __HAL_RCC_SPI1_FORCE_RESET()       SET_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_SPI1RST))
#define __HAL_RCC_TIM21_FORCE_RESET()      SET_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_TIM21RST))
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_FORCE_RESET()      SET_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_TIM22RST))
#endif

#define __HAL_RCC_USART1_RELEASE_RESET()     CLEAR_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_USART1RST))
#define __HAL_RCC_ADC1_RELEASE_RESET()       CLEAR_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_ADC1RST))
#define __HAL_RCC_SPI1_RELEASE_RESET()       CLEAR_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_SPI1RST))
#define __HAL_RCC_TIM21_RELEASE_RESET()      CLEAR_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_TIM21RST))
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_RELEASE_RESET()      CLEAR_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_TIM22RST))
#endif
#endif /* STM32L051xx  || STM32L061xx  || STM32L071xx  ||  STM32L081xx  || */
       /* STM32L052xx  || STM32L062xx  || STM32L072xx  ||  STM32L082xx  || */
       /* STM32L053xx  || STM32L063xx  || STM32L073xx  ||  STM32L083xx  || */
#if defined(STM32L011xx) || defined(STM32L021xx) || defined(STM32L031xx) || defined(STM32L041xx)
#define __HAL_RCC_ADC1_FORCE_RESET()       SET_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_ADC1RST))
#define __HAL_RCC_SPI1_FORCE_RESET()       SET_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_SPI1RST))
#define __HAL_RCC_TIM21_FORCE_RESET()      SET_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_TIM21RST))
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_FORCE_RESET()      SET_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_TIM22RST))
#endif
#define __HAL_RCC_ADC1_RELEASE_RESET()       CLEAR_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_ADC1RST))
#define __HAL_RCC_SPI1_RELEASE_RESET()       CLEAR_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_SPI1RST))
#define __HAL_RCC_TIM21_RELEASE_RESET()      CLEAR_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_TIM21RST))
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_RELEASE_RESET()      CLEAR_BIT(RCC->APB2RSTR, (RCC_APB2RSTR_TIM22RST))
#endif
#endif /* STM32L031xx  || STM32L041xx  || STM32L011xx  || STM32L021xx*/

/**
  * @}
  */

/** @defgroup RCCEx_AHB_Clock_Sleep_Enable_Disable AHB Peripheral Clock Sleep Enable Disable
  * @brief  Enable or disable the AHB peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) 
#define __HAL_RCC_TSC_CLK_SLEEP_ENABLE()           SET_BIT(RCC->AHBSMENR, (RCC_AHBSMENR_TSCSMEN))
#define __HAL_RCC_RNG_CLK_SLEEP_ENABLE()           SET_BIT(RCC->AHBSMENR, (RCC_AHBSMENR_RNGSMEN))
#define __HAL_RCC_TSC_CLK_SLEEP_DISABLE()          CLEAR_BIT(RCC->AHBSMENR, (RCC_AHBSMENR_TSCSMEN))
#define __HAL_RCC_RNG_CLK_SLEEP_DISABLE()          CLEAR_BIT(RCC->AHBSMENR, (RCC_AHBSMENR_RNGSMEN))

#define __HAL_RCC_TSC_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_TSCSMEN) != RESET)
#define __HAL_RCC_RNG_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_RNGSMEN) != RESET)
#define __HAL_RCC_TSC_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_TSCSMEN) == RESET)
#define __HAL_RCC_RNG_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->AHBSMENR, RCC_AHBSMENR_RNGSMEN) == RESET)
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) &&  !(STM32L041xx ) &&  !(STM32L051xx ) && !(STM32L061xx ) && !(STM32L071xx ) && !(STM32L081xx ) */
       
#if defined(STM32L062xx) || defined(STM32L063xx)|| defined(STM32L082xx) || defined(STM32L083xx) || defined(STM32L041xx)
#define __HAL_RCC_AES_CLK_SLEEP_ENABLE()          SET_BIT(RCC->AHBLPENR, (RCC_AHBSMENR_CRYPSMEN))
#define __HAL_RCC_AES_CLK_SLEEP_DISABLE()         CLEAR_BIT(RCC->AHBLPENR, (RCC_AHBSMENR_CRYPSMEN))

#define __HAL_RCC_AES_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->AHBLPENR, RCC_AHBSMENR_CRYPSMEN) != RESET)
#define __HAL_RCC_AES_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->AHBLPENR, RCC_AHBSMENR_CRYPSMEN) == RESET)
#endif /* STM32L062xx || STM32L063xx || STM32L072xx || STM32L073xx || STM32L082xx || STM32L083xx || STM32L041xx */

/**
  * @}
  */

/** @defgroup RCCEx_IOPORT_Clock_Sleep_Enable_Disable IOPORT Peripheral Clock Sleep Enable Disable
  * @brief  Enable or disable the IOPORT peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */
#if defined(STM32L073xx) || defined(STM32L083xx) \
 || defined(STM32L072xx) || defined(STM32L082xx) \
 || defined(STM32L071xx) || defined(STM32L081xx) 
#define __HAL_RCC_GPIOE_CLK_SLEEP_ENABLE()         SET_BIT(RCC->IOPSMENR, (RCC_IOPSMENR_GPIOESMEN))
#define __HAL_RCC_GPIOE_CLK_SLEEP_DISABLE()        CLEAR_BIT(RCC->IOPSMENR,(RCC_IOPSMENR_GPIOESMEN))

#define __HAL_RCC_GPIOE_IS_CLK_SLEEP_ENABLED()        (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOESMEN) != RESET)
#define __HAL_RCC_GPIOE_IS_CLK_SLEEP_DISABLED()       (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIOESMEN) == RESET)
#endif /* STM32L071xx  ||  STM32L081xx  || */
       /* STM32L072xx  ||  STM32L082xx  || */
       /* STM32L073xx  ||  STM32L083xx  || */
#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx)
#define __HAL_RCC_GPIOD_CLK_SLEEP_ENABLE()    SET_BIT(RCC->IOPSMENR, (RCC_IOPSMENR_GPIODSMEN))
#define __HAL_RCC_GPIOD_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->IOPSMENR,(RCC_IOPSMENR_GPIODSMEN))

#define __HAL_RCC_GPIOD_IS_CLK_SLEEP_ENABLED()        (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIODSMEN) != RESET)
#define __HAL_RCC_GPIOD_IS_CLK_SLEEP_DISABLED()       (READ_BIT(RCC->IOPSMENR, RCC_IOPSMENR_GPIODSMEN) == RESET)
#endif  /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) */ 
/**
  * @}
  */


/** @defgroup RCCEx_APB1_Clock_Sleep_Enable_Disable APB1 Peripheral Clock Sleep Enable Disable
  * @brief  Enable or disable the APB1 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */

#if defined(STM32L053xx) || defined(STM32L063xx) \
 || defined(STM32L052xx) || defined(STM32L062xx) \
 || defined(STM32L051xx) || defined(STM32L061xx) 
#define __HAL_RCC_TIM2_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM2SMEN))
#define __HAL_RCC_TIM6_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM6SMEN))
#define __HAL_RCC_SPI2_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_SPI2SMEN))
#define __HAL_RCC_USART2_CLK_SLEEP_ENABLE()  SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USART2SMEN))
#define __HAL_RCC_LPUART1_CLK_SLEEP_ENABLE() SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPUART1SMEN))
#define __HAL_RCC_I2C1_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C1SMEN))
#define __HAL_RCC_I2C2_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C2SMEN))
#define __HAL_RCC_DAC_CLK_SLEEP_ENABLE()     SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_DACSMEN))
#define __HAL_RCC_LPTIM1_CLK_SLEEP_ENABLE()  SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPTIM1SMEN))

#define __HAL_RCC_TIM2_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM2SMEN))
#define __HAL_RCC_TIM6_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM6SMEN))
#define __HAL_RCC_SPI2_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_SPI2SMEN))
#define __HAL_RCC_USART2_CLK_SLEEP_DISABLE()  CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USART2SMEN))
#define __HAL_RCC_LPUART1_CLK_SLEEP_DISABLE() CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPUART1SMEN))
#define __HAL_RCC_I2C1_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C1SMEN))
#define __HAL_RCC_I2C2_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C2SMEN))
#define __HAL_RCC_DAC_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_DACSMEN))
#define __HAL_RCC_LPTIM1_CLK_SLEEP_DISABLE()  CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPTIM1SMEN))

#define __HAL_RCC_TIM2_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM2SMEN) != RESET)
#define __HAL_RCC_TIM6_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM6SMEN) != RESET)
#define __HAL_RCC_SPI2_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_SPI2SMEN) != RESET)
#define __HAL_RCC_USART2_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USART2SMEN) != RESET)
#define __HAL_RCC_LPUART1_IS_CLK_SLEEP_ENABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPUART1SMEN) != RESET)
#define __HAL_RCC_I2C1_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C1SMEN) != RESET)
#define __HAL_RCC_I2C2_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C2SMEN) != RESET)
#define __HAL_RCC_DAC_IS_CLK_SLEEP_ENABLED()      (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_DACSMEN) != RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN) != RESET)
#define __HAL_RCC_TIM2_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM2SMEN) == RESET)
#define __HAL_RCC_TIM6_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM6SMEN) == RESET)
#define __HAL_RCC_SPI2_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_SPI2SMEN) == RESET)
#define __HAL_RCC_USART2_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USART2SMEN) == RESET)
#define __HAL_RCC_LPUART1_IS_CLK_SLEEP_DISABLED() (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPUART1SMEN) == RESET)
#define __HAL_RCC_I2C1_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C1SMEN) == RESET)
#define __HAL_RCC_I2C2_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C2SMEN) == RESET)
#define __HAL_RCC_DAC_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_DACSMEN) == RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN) == RESET)
#endif /* STM32L051xx  || STM32L061xx  || */
       /* STM32L052xx  || STM32L062xx  || */
       /* STM32L053xx  || STM32L063xx     */
       
#if defined(STM32L073xx) || defined(STM32L083xx) \
 || defined(STM32L072xx) || defined(STM32L082xx) \
 || defined(STM32L071xx) || defined(STM32L081xx)
#define __HAL_RCC_TIM2_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM2SMEN))
#define __HAL_RCC_TIM3_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM3SMEN))
#define __HAL_RCC_TIM6_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM6SMEN))
#define __HAL_RCC_TIM7_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM7SMEN))
#define __HAL_RCC_SPI2_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_SPI2SMEN))
#define __HAL_RCC_USART2_CLK_SLEEP_ENABLE()  SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USART2SMEN))
#define __HAL_RCC_USART4_CLK_SLEEP_ENABLE()  SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USART4SMEN))
#define __HAL_RCC_USART5_CLK_SLEEP_ENABLE()  SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USART5SMEN))
#define __HAL_RCC_LPUART1_CLK_SLEEP_ENABLE() SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPUART1SMEN))
#define __HAL_RCC_I2C1_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C1SMEN))
#define __HAL_RCC_I2C2_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C2SMEN))
#define __HAL_RCC_I2C3_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C3SMEN))
#define __HAL_RCC_DAC_CLK_SLEEP_ENABLE()     SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_DACSMEN))
#define __HAL_RCC_LPTIM1_CLK_SLEEP_ENABLE()  SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPTIM1SMEN))

#define __HAL_RCC_TIM2_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM2SMEN))
#define __HAL_RCC_TIM3_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM3SMEN))
#define __HAL_RCC_TIM6_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM6SMEN))
#define __HAL_RCC_TIM7_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM7SMEN))
#define __HAL_RCC_SPI2_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_SPI2SMEN))
#define __HAL_RCC_USART2_CLK_SLEEP_DISABLE()  CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USART2SMEN))
#define __HAL_RCC_USART4_CLK_SLEEP_DISABLE()  CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USART4SMEN))
#define __HAL_RCC_USART5_CLK_SLEEP_DISABLE()  CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USART5SMEN))
#define __HAL_RCC_LPUART1_CLK_SLEEP_DISABLE() CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPUART1SMEN))
#define __HAL_RCC_I2C1_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C1SMEN))
#define __HAL_RCC_I2C2_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C2SMEN))
#define __HAL_RCC_I2C3_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C3SMEN))
#define __HAL_RCC_DAC_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_DACSMEN))
#define __HAL_RCC_LPTIM1_CLK_SLEEP_DISABLE()  CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPTIM1SMEN))

#define __HAL_RCC_TIM2_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM2SMEN) != RESET)
#define __HAL_RCC_TIM3_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM3SMEN) != RESET)
#define __HAL_RCC_TIM6_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM6SMEN) != RESET)
#define __HAL_RCC_TIM7_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM7SMEN) != RESET)
#define __HAL_RCC_SPI2_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_SPI2SMEN) != RESET)
#define __HAL_RCC_USART2_IS_CLK_SLEEP_ENABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USART2SMEN) != RESET)
#define __HAL_RCC_USART4_IS_CLK_SLEEP_ENABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USART4SMEN) != RESET)
#define __HAL_RCC_USART5_IS_CLK_SLEEP_ENABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USART5SMEN) != RESET)
#define __HAL_RCC_LPUART1_IS_CLK_SLEEP_ENABLED() (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPUART1SMEN) != RESET)
#define __HAL_RCC_I2C1_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C1SMEN) != RESET)
#define __HAL_RCC_I2C2_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C2SMEN) != RESET)
#define __HAL_RCC_I2C3_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C3SMEN) != RESET)
#define __HAL_RCC_DAC_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_DACSMEN) != RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_SLEEP_ENABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN) != RESET)
#define __HAL_RCC_TIM2_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM2SMEN) == RESET)
#define __HAL_RCC_TIM3_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM3SMEN) == RESET)
#define __HAL_RCC_TIM6_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM6SMEN) == RESET)
#define __HAL_RCC_TIM7_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM7SMEN) == RESET)
#define __HAL_RCC_SPI2_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_SPI2SMEN) == RESET)
#define __HAL_RCC_USART2_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USART2SMEN) == RESET)
#define __HAL_RCC_USART4_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USART4SMEN) == RESET)
#define __HAL_RCC_USART5_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USART5SMEN) == RESET)
#define __HAL_RCC_LPUART1_IS_CLK_SLEEP_DISABLED() (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPUART1SMEN) == RESET)
#define __HAL_RCC_I2C1_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C1SMEN) == RESET)
#define __HAL_RCC_I2C2_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C2SMEN) == RESET)
#define __HAL_RCC_I2C3_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C3SMEN) == RESET)
#define __HAL_RCC_DAC_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_DACSMEN) == RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN) == RESET)
#endif /*  STM32L071xx  ||  STM32L081xx  || */
       /*  STM32L072xx  ||  STM32L082xx  || */
       /*  STM32L073xx  ||  STM32L083xx  || */

#if defined(STM32L011xx) || defined(STM32L021xx) || defined(STM32L031xx) || defined(STM32L041xx) 
#define __HAL_RCC_TIM2_CLK_SLEEP_ENABLE()     SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM2SMEN))
#define __HAL_RCC_USART2_CLK_SLEEP_ENABLE()   SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USART2SMEN))
#define __HAL_RCC_LPUART1_CLK_SLEEP_ENABLE()  SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPUART1SMEN))
#define __HAL_RCC_I2C1_CLK_SLEEP_ENABLE()     SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C1SMEN))
#define __HAL_RCC_LPTIM1_CLK_SLEEP_ENABLE()   SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPTIM1SMEN))

#define __HAL_RCC_TIM2_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_TIM2SMEN))
#define __HAL_RCC_USART2_CLK_SLEEP_DISABLE()  CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USART2SMEN))
#define __HAL_RCC_LPUART1_CLK_SLEEP_DISABLE() CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPUART1SMEN))
#define __HAL_RCC_I2C1_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_I2C1SMEN))
#define __HAL_RCC_LPTIM1_CLK_SLEEP_DISABLE()  CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LPTIM1SMEN))

#define __HAL_RCC_TIM2_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM2SMEN) != RESET)
#define __HAL_RCC_USART2_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USART2SMEN) != RESET)
#define __HAL_RCC_LPUART1_IS_CLK_SLEEP_ENABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPUART1SMEN) != RESET)
#define __HAL_RCC_I2C1_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C1SMEN) != RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN) != RESET)
#define __HAL_RCC_TIM2_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_TIM2SMEN) == RESET)
#define __HAL_RCC_USART2_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USART2SMEN) == RESET)
#define __HAL_RCC_LPUART1_IS_CLK_SLEEP_DISABLED()  (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPUART1SMEN) == RESET)
#define __HAL_RCC_I2C1_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_I2C1SMEN) == RESET)
#define __HAL_RCC_LPTIM1_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LPTIM1SMEN) == RESET)

#endif /*  STM32L031xx  ||  STM32L041xx || STM32L011xx  || STM32L021xx */

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) 
#define __HAL_RCC_USB_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USBSMEN))
#define __HAL_RCC_USB_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USBSMEN))
#define __HAL_RCC_CRS_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_CRSSMEN))
#define __HAL_RCC_CRS_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_CRSSMEN))

#define __HAL_RCC_USB_IS_CLK_SLEEP_ENABLED()        (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USBSMEN) != RESET)
#define __HAL_RCC_USB_IS_CLK_SLEEP_DISABLED()       (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_USBSMEN) == RESET)
#define __HAL_RCC_CRS_IS_CLK_SLEEP_ENABLED()        (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_CRSSMEN) != RESET)
#define __HAL_RCC_CRS_IS_CLK_SLEEP_DISABLED()       (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_CRSSMEN) == RESET)
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx )  && !(STM32L071xx ) && !(STM32L081xx ) */

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)
#define __HAL_RCC_LCD_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LCDSMEN))
#define __HAL_RCC_LCD_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LCDSMEN))

#define __HAL_RCC_LCD_IS_CLK_SLEEP_ENABLED()        (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LCDSMEN) != RESET)
#define __HAL_RCC_LCD_IS_CLK_SLEEP_DISABLED()       (READ_BIT(RCC->APB1SMENR, RCC_APB1SMENR_LCDSMEN) == RESET)
#endif /* STM32L053xx || STM32L063xx || STM32L073xx  || STM32L083xx */

/**
  * @}
  */

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx) \
 || defined(STM32L052xx) || defined(STM32L062xx) || defined(STM32L072xx) || defined(STM32L082xx) \
 || defined(STM32L051xx) || defined(STM32L061xx) || defined(STM32L071xx) || defined(STM32L081xx) \
 || defined(STM32L031xx) || defined(STM32L041xx) || defined(STM32L011xx) || defined(STM32L021xx) 

/** @defgroup RCCEx_APB2_Clock_Sleep_Enable_Disable APB2 Peripheral Clock Sleep Enable Disable
  * @brief  Enable or disable the APB2 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */
#define __HAL_RCC_TIM21_CLK_SLEEP_ENABLE()   SET_BIT(RCC->APB2SMENR, (RCC_APB2SMENR_TIM21SMEN))
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_CLK_SLEEP_ENABLE()   SET_BIT(RCC->APB2SMENR, (RCC_APB2SMENR_TIM22SMEN))
#endif
#define __HAL_RCC_ADC1_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB2SMENR, (RCC_APB2SMENR_ADC1SMEN))
#define __HAL_RCC_SPI1_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB2SMENR, (RCC_APB2SMENR_SPI1SMEN))
#define __HAL_RCC_USART1_CLK_SLEEP_ENABLE()  SET_BIT(RCC->APB2SMENR, (RCC_APB2SMENR_USART1SMEN))

#define __HAL_RCC_TIM21_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APB2SMENR,  (RCC_APB2SMENR_TIM21SMEN))
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APB2SMENR,  (RCC_APB2SMENR_TIM22SMEN))
#endif
#define __HAL_RCC_ADC1_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB2SMENR,  (RCC_APB2SMENR_ADC1SMEN))
#define __HAL_RCC_SPI1_CLK_SLEEP_DISABLE()    CLEAR_BIT(RCC->APB2SMENR,  (RCC_APB2SMENR_SPI1SMEN))
#define __HAL_RCC_USART1_CLK_SLEEP_DISABLE()  CLEAR_BIT(RCC->APB2SMENR,  (RCC_APB2SMENR_USART1SMEN))

#define __HAL_RCC_TIM21_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APB2SMENR, RCC_APB2SMENR_TIM21SMEN) != RESET)
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_IS_CLK_SLEEP_ENABLED()    (READ_BIT(RCC->APB2SMENR, RCC_APB2SMENR_TIM22SMEN) != RESET)
#endif
#define __HAL_RCC_ADC1_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APB2SMENR, RCC_APB2SMENR_ADC1SMEN) != RESET)
#define __HAL_RCC_SPI1_IS_CLK_SLEEP_ENABLED()     (READ_BIT(RCC->APB2SMENR, RCC_APB2SMENR_SPI1SMEN) != RESET)
#define __HAL_RCC_USART1_IS_CLK_SLEEP_ENABLED()   (READ_BIT(RCC->APB2SMENR, RCC_APB2SMENR_USART1SMEN) != RESET)

#define __HAL_RCC_TIM21_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB2SMENR,  (RCC_APB2SMENR_TIM21SMEN) == RESET)
#if !defined (STM32L011xx) && !defined (STM32L021xx)
#define __HAL_RCC_TIM22_IS_CLK_SLEEP_DISABLED()    (READ_BIT(RCC->APB2SMENR,  (RCC_APB2SMENR_TIM22SMEN) == RESET)
#endif
#define __HAL_RCC_ADC1_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APB2SMENR,  (RCC_APB2SMENR_ADC1SMEN) == RESET)
#define __HAL_RCC_SPI1_IS_CLK_SLEEP_DISABLED()     (READ_BIT(RCC->APB2SMENR,  (RCC_APB2SMENR_SPI1SMEN) == RESET)
#define __HAL_RCC_USART1_IS_CLK_SLEEP_DISABLED()   (READ_BIT(RCC->APB2SMENR,  (RCC_APB2SMENR_USART1SMEN) == RESET)

/**
  * @}
  */

#endif /* STM32L051xx  || STM32L061xx  || STM32L071xx  ||  STM32L081xx  || */
       /* STM32L052xx  || STM32L062xx  || STM32L072xx  ||  STM32L082xx  || */
       /* STM32L053xx  || STM32L063xx  || STM32L073xx  ||  STM32L083xx  || */
       /* STM32L031xx  || STM32L041xx  || STM32L011xx  ||  STM32L021xx   */
  

/**
  * @brief Enable interrupt on RCC LSE CSS EXTI Line 19.
  * @retval None
  */
#define __HAL_RCC_LSECSS_EXTI_ENABLE_IT()      SET_BIT(EXTI->IMR, RCC_EXTI_LINE_LSECSS)

/**
  * @brief Disable interrupt on RCC LSE CSS EXTI Line 19.
  * @retval None
  */
#define __HAL_RCC_LSECSS_EXTI_DISABLE_IT()     CLEAR_BIT(EXTI->IMR, RCC_EXTI_LINE_LSECSS)

/**
  * @brief Enable event on RCC LSE CSS EXTI Line 19.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_ENABLE_EVENT()   SET_BIT(EXTI->EMR, RCC_EXTI_LINE_LSECSS)

/**
  * @brief Disable event on RCC LSE CSS EXTI Line 19.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_DISABLE_EVENT()  CLEAR_BIT(EXTI->EMR, RCC_EXTI_LINE_LSECSS)


/**
  * @brief  RCC LSE CSS EXTI line configuration: set falling edge trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_ENABLE_FALLING_EDGE()  SET_BIT(EXTI->FTSR, RCC_EXTI_LINE_LSECSS)


/**
  * @brief Disable the RCC LSE CSS Extended Interrupt Falling Trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_DISABLE_FALLING_EDGE()  CLEAR_BIT(EXTI->FTSR, RCC_EXTI_LINE_LSECSS)


/**
  * @brief  RCC LSE CSS EXTI line configuration: set rising edge trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_ENABLE_RISING_EDGE()   SET_BIT(EXTI->RTSR, RCC_EXTI_LINE_LSECSS)

/**
  * @brief Disable the RCC LSE CSS Extended Interrupt Rising Trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_DISABLE_RISING_EDGE()  CLEAR_BIT(EXTI->RTSR, RCC_EXTI_LINE_LSECSS)

/**
  * @brief  RCC LSE CSS EXTI line configuration: set rising & falling edge trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_ENABLE_RISING_FALLING_EDGE()  \
  do {                                                      \
    __HAL_RCC_LSECSS_EXTI_ENABLE_RISING_EDGE();             \
    __HAL_RCC_LSECSS_EXTI_ENABLE_FALLING_EDGE();            \
  } while(0)  
  
/**
  * @brief Disable the RCC LSE CSS Extended Interrupt Rising & Falling Trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_DISABLE_RISING_FALLING_EDGE()  \
  do {                                                       \
    __HAL_RCC_LSECSS_EXTI_DISABLE_RISING_EDGE();             \
    __HAL_RCC_LSECSS_EXTI_DISABLE_FALLING_EDGE();            \
  } while(0)  

/**
  * @brief Check whether the specified RCC LSE CSS EXTI interrupt flag is set or not.
  * @retval EXTI RCC LSE CSS Line Status.
  */
#define __HAL_RCC_LSECSS_EXTI_GET_FLAG()       (EXTI->PR & (RCC_EXTI_LINE_LSECSS))

/**
  * @brief Clear the RCC LSE CSS EXTI flag.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_CLEAR_FLAG()     (EXTI->PR = (RCC_EXTI_LINE_LSECSS))

/**
  * @brief Generate a Software interrupt on selected EXTI line.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_GENERATE_SWIT()  SET_BIT(EXTI->SWIER, RCC_EXTI_LINE_LSECSS)


#if defined(LCD)
    
/** @defgroup RCCEx_LCD_Configuration LCD Configuration
  * @brief  Macros to configure clock source of LCD peripherals.
  * @{
  */  

/** @brief Macro to configures LCD clock (LCDCLK).
  *  @note   LCD and RTC use the same configuration
  *  @note   LCD can however be used in the Stop low power mode if the LSE or LSI is used as the
  *          LCD clock source.
  *    
  *  @param  __LCD_CLKSOURCE__ specifies the LCD clock source.
  *          This parameter can be one of the following values:
  *             @arg @ref RCC_RTCCLKSOURCE_LSE LSE selected as LCD clock
  *             @arg @ref RCC_RTCCLKSOURCE_LSI LSI selected as LCD clock
  *             @arg @ref RCC_RTCCLKSOURCE_HSE_DIV2 HSE divided by 2 selected as LCD clock
  *             @arg @ref RCC_RTCCLKSOURCE_HSE_DIV4 HSE divided by 4 selected as LCD clock
  *             @arg @ref RCC_RTCCLKSOURCE_HSE_DIV8 HSE divided by 8 selected as LCD clock
  *             @arg @ref RCC_RTCCLKSOURCE_HSE_DIV16 HSE divided by 16 selected as LCD clock
  */
#define __HAL_RCC_LCD_CONFIG(__LCD_CLKSOURCE__) __HAL_RCC_RTC_CONFIG(__LCD_CLKSOURCE__)

/** @brief Macro to get the LCD clock source.
  */
#define __HAL_RCC_GET_LCD_SOURCE()              __HAL_RCC_GET_RTC_SOURCE()

/** @brief Macro to get the LCD clock pre-scaler.
  */
#define  __HAL_RCC_GET_LCD_HSE_PRESCALER()      __HAL_RCC_GET_RTC_HSE_PRESCALER()

/**
  * @}
  */

#endif /* LCD */

/** @brief Macro to configure the I2C1 clock (I2C1CLK).
  *
  * @param  __I2C1_CLKSOURCE__ specifies the I2C1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_I2C1CLKSOURCE_PCLK1 PCLK1 selected as I2C1 clock  
  *            @arg @ref RCC_I2C1CLKSOURCE_HSI HSI selected as I2C1 clock
  *            @arg @ref RCC_I2C1CLKSOURCE_SYSCLK System Clock selected as I2C1 clock 
  */
#define __HAL_RCC_I2C1_CONFIG(__I2C1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_I2C1SEL, (uint32_t)(__I2C1_CLKSOURCE__))

/** @brief  Macro to get the I2C1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_I2C1CLKSOURCE_PCLK1 PCLK1 selected as I2C1 clock  
  *            @arg @ref RCC_I2C1CLKSOURCE_HSI HSI selected as I2C1 clock
  *            @arg @ref RCC_I2C1CLKSOURCE_SYSCLK System Clock selected as I2C1 clock
  */
#define __HAL_RCC_GET_I2C1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_I2C1SEL)))

#if defined(RCC_CCIPR_I2C3SEL)  
/** @brief Macro to configure the I2C3 clock (I2C3CLK).
  *
  * @param  __I2C3_CLKSOURCE__ specifies the I2C3 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_I2C3CLKSOURCE_PCLK1 PCLK1 selected as I2C3 clock  
  *            @arg @ref RCC_I2C3CLKSOURCE_HSI HSI selected as I2C3 clock
  *            @arg @ref RCC_I2C3CLKSOURCE_SYSCLK System Clock selected as I2C3 clock 
  */
#define __HAL_RCC_I2C3_CONFIG(__I2C3_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_I2C3SEL, (uint32_t)(__I2C3_CLKSOURCE__))

/** @brief  Macro to get the I2C3 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_I2C3CLKSOURCE_PCLK1 PCLK1 selected as I2C3 clock  
  *            @arg @ref RCC_I2C3CLKSOURCE_HSI HSI selected as I2C3 clock
  *            @arg @ref RCC_I2C3CLKSOURCE_SYSCLK System Clock selected as I2C3 clock
  */
#define __HAL_RCC_GET_I2C3_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_I2C3SEL)))

#endif /* RCC_CCIPR_I2C3SEL */ 

#if defined (RCC_CCIPR_USART1SEL)
/** @brief Macro to configure the USART1 clock (USART1CLK).
  *
  * @param  __USART1_CLKSOURCE__ specifies the USART1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_USART1CLKSOURCE_PCLK2 PCLK2 selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_HSI HSI selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_SYSCLK System Clock selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_LSE LSE selected as USART1 clock
  */
#define __HAL_RCC_USART1_CONFIG(__USART1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_USART1SEL, (uint32_t)(__USART1_CLKSOURCE__))

/** @brief  Macro to get the USART1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_USART1CLKSOURCE_PCLK2 PCLK2 selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_HSI HSI selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_SYSCLK System Clock selected as USART1 clock
  *            @arg @ref RCC_USART1CLKSOURCE_LSE LSE selected as USART1 clock
  */
#define __HAL_RCC_GET_USART1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_USART1SEL)))
#endif /* RCC_CCIPR_USART1SEL */

/** @brief Macro to configure the USART2 clock (USART2CLK).
  *
  * @param  __USART2_CLKSOURCE__ specifies the USART2 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_USART2CLKSOURCE_PCLK1 PCLK1 selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_HSI HSI selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_SYSCLK System Clock selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_LSE LSE selected as USART2 clock
  */
#define __HAL_RCC_USART2_CONFIG(__USART2_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_USART2SEL, (uint32_t)(__USART2_CLKSOURCE__))

/** @brief  Macro to get the USART2 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_USART2CLKSOURCE_PCLK1 PCLK1 selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_HSI HSI selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_SYSCLK System Clock selected as USART2 clock
  *            @arg @ref RCC_USART2CLKSOURCE_LSE LSE selected as USART2 clock
  */
#define __HAL_RCC_GET_USART2_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_USART2SEL)))

/** @brief Macro to configure the LPUART1 clock (LPUART1CLK).
  *
  * @param  __LPUART1_CLKSOURCE__ specifies the LPUART1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LPUART1CLKSOURCE_PCLK1 PCLK1 selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_HSI HSI selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_SYSCLK System Clock selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_LSE LSE selected as LPUART1 clock
  */
#define __HAL_RCC_LPUART1_CONFIG(__LPUART1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_LPUART1SEL, (uint32_t)(__LPUART1_CLKSOURCE__))

/** @brief  Macro to get the LPUART1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_LPUART1CLKSOURCE_PCLK1 PCLK1 selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_HSI HSI selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_SYSCLK System Clock selected as LPUART1 clock
  *            @arg @ref RCC_LPUART1CLKSOURCE_LSE LSE selected as LPUART1 clock
  */
#define __HAL_RCC_GET_LPUART1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_LPUART1SEL)))

/** @brief Macro to configure the LPTIM1 clock (LPTIM1CLK).
  *
  * @param  __LPTIM1_CLKSOURCE__ specifies the LPTIM1 clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LPTIM1CLKSOURCE_PCLK PCLK selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSI  HSI  selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_HSI  LSI  selected as LPTIM1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSE  LSE  selected as LPTIM1 clock
  */
#define __HAL_RCC_LPTIM1_CONFIG(__LPTIM1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_LPTIM1SEL, (uint32_t)(__LPTIM1_CLKSOURCE__))

/** @brief  Macro to get the LPTIM1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_LPTIM1CLKSOURCE_PCLK PCLK selected as LPUART1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSI  HSI selected as LPUART1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_HSI  System Clock selected as LPUART1 clock
  *            @arg @ref RCC_LPTIM1CLKSOURCE_LSE  LSE selected as LPUART1 clock
  */
#define __HAL_RCC_GET_LPTIM1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_LPTIM1SEL)))

#if defined(USB)
/** @brief  Macro to configure the USB clock (USBCLK).
  * @param  __USB_CLKSOURCE__ specifies the USB clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_USBCLKSOURCE_HSI48  HSI48 selected as USB clock
  *            @arg @ref RCC_USBCLKSOURCE_PLL PLL Clock selected as USB clock
  */
#define __HAL_RCC_USB_CONFIG(__USB_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_HSI48SEL, (uint32_t)(__USB_CLKSOURCE__))

/** @brief  Macro to get the USB clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_USBCLKSOURCE_HSI48  HSI48 selected as USB clock
  *            @arg @ref RCC_USBCLKSOURCE_PLL PLL Clock selected as USB clock
  */
#define __HAL_RCC_GET_USB_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_HSI48SEL)))
#endif /* USB */

#if defined(RNG)
/** @brief  Macro to configure the RNG clock (RNGCLK).
  * @param  __RNG_CLKSOURCE__ specifies the USB clock source.
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_RNGCLKSOURCE_HSI48  HSI48 selected as RNG clock
  *            @arg @ref RCC_RNGCLKSOURCE_PLLCLK PLL Clock selected as RNG clock
  */
#define __HAL_RCC_RNG_CONFIG(__RNG_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_HSI48SEL, (uint32_t)(__RNG_CLKSOURCE__))

/** @brief  Macro to get the RNG clock source.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_RNGCLKSOURCE_HSI48  HSI48 selected as RNG clock
  *            @arg @ref RCC_RNGCLKSOURCE_PLLCLK PLL Clock selected as RNG clock
  */
#define __HAL_RCC_GET_RNG_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_HSI48SEL)))
#endif /* RNG */

#if defined(RCC_CCIPR_HSI48SEL)
/** @brief Macro to select the HSI48M clock source 
  * @note   This macro can be replaced by either __HAL_RCC_RNG_CONFIG or
  *         __HAL_RCC_USB_CONFIG to configure respectively RNG or UBS clock sources.
  *
  * @param  __HSI48M_CLKSOURCE__ specifies the HSI48M clock source dedicated for 
  *          USB an RNG peripherals.                 
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_HSI48M_PLL A dedicated 48MHZ PLL output.
  *            @arg @ref RCC_HSI48M_HSI48 48MHZ issued from internal HSI48 oscillator. 
  */
#define __HAL_RCC_HSI48M_CONFIG(__HSI48M_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_HSI48SEL, (uint32_t)(__HSI48M_CLKSOURCE__))                    

/** @brief  Macro to get the HSI48M clock source.
  * @note   This macro can be replaced by either __HAL_RCC_GET_RNG_SOURCE or
  *         __HAL_RCC_GET_USB_SOURCE to get respectively RNG or UBS clock sources.
  * @retval The clock source can be one of the following values:
  *           @arg @ref RCC_HSI48M_PLL A dedicated 48MHZ PLL output.
  *            @arg @ref RCC_HSI48M_HSI48 48MHZ issued from internal HSI48 oscillator. 
  */
#define __HAL_RCC_GET_HSI48M_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_HSI48SEL)))       
#endif /* RCC_CCIPR_HSI48SEL */ 

/**
  * @brief    Macro to enable the force of the Internal High Speed oscillator (HSI)
  *           in STOP mode to be quickly available as kernel clock for USART and I2C.
  * @note     The Enable of this function has not effect on the HSION bit.
  */
#define __HAL_RCC_HSISTOP_ENABLE()  SET_BIT(RCC->CR, RCC_CR_HSIKERON)

/**
  * @brief    Macro to disable the force of the Internal High Speed oscillator (HSI)
  *           in STOP mode to be quickly available as kernel clock for USART and I2C.
  * @retval None
  */
#define __HAL_RCC_HSISTOP_DISABLE() CLEAR_BIT(RCC->CR, RCC_CR_HSIKERON)                   

/**
  * @brief  Macro to configures the External Low Speed oscillator (LSE) drive capability.
  * @param  __RCC_LSEDRIVE__ specifies the new state of the LSE drive capability.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LSEDRIVE_LOW LSE oscillator low drive capability.
  *            @arg @ref RCC_LSEDRIVE_MEDIUMLOW LSE oscillator medium low drive capability.
  *            @arg @ref RCC_LSEDRIVE_MEDIUMHIGH LSE oscillator medium high drive capability.
  *            @arg @ref RCC_LSEDRIVE_HIGH LSE oscillator high drive capability.
  * @retval None
  */ 
#define __HAL_RCC_LSEDRIVE_CONFIG(__RCC_LSEDRIVE__) (MODIFY_REG(RCC->CSR,\
        RCC_CSR_LSEDRV, (uint32_t)(__RCC_LSEDRIVE__) ))

/**
  * @brief  Macro to configures the wake up from stop clock.
  * @param  __RCC_STOPWUCLK__ specifies the clock source used after wake up from stop 
  *   This parameter can be one of the following values:
  *     @arg @ref RCC_STOP_WAKEUPCLOCK_MSI    MSI selected as system clock source
  *     @arg @ref RCC_STOP_WAKEUPCLOCK_HSI    HSI selected as system clock source
  * @retval None
  */
#define __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(__RCC_STOPWUCLK__) (MODIFY_REG(RCC->CFGR,\
        RCC_CFGR_STOPWUCK, (uint32_t)(__RCC_STOPWUCLK__) ))
        
#if defined(CRS)
/**
  * @brief  Enables the specified CRS interrupts.
  * @param  __INTERRUPT__ specifies the CRS interrupt sources to be enabled.
  *          This parameter can be any combination of the following values:
  *              @arg @ref RCC_CRS_IT_SYNCOK
  *              @arg @ref RCC_CRS_IT_SYNCWARN
  *              @arg @ref RCC_CRS_IT_ERR
  *              @arg @ref RCC_CRS_IT_ESYNC
  * @retval None
  */
#define __HAL_RCC_CRS_ENABLE_IT(__INTERRUPT__)   SET_BIT(CRS->CR, (__INTERRUPT__))

/**
  * @brief  Disables the specified CRS interrupts.
  * @param  __INTERRUPT__ specifies the CRS interrupt sources to be disabled.
  *          This parameter can be any combination of the following values:
  *              @arg @ref RCC_CRS_IT_SYNCOK
  *              @arg @ref RCC_CRS_IT_SYNCWARN
  *              @arg @ref RCC_CRS_IT_ERR
  *              @arg @ref RCC_CRS_IT_ESYNC
  * @retval None
  */
#define __HAL_RCC_CRS_DISABLE_IT(__INTERRUPT__)  CLEAR_BIT(CRS->CR,(__INTERRUPT__))

/** @brief  Check the CRS interrupt has occurred or not.
  * @param  __INTERRUPT__ specifies the CRS interrupt source to check.
  *         This parameter can be one of the following values:
  *              @arg @ref RCC_CRS_IT_SYNCOK
  *              @arg @ref RCC_CRS_IT_SYNCWARN
  *              @arg @ref RCC_CRS_IT_ERR
  *              @arg @ref RCC_CRS_IT_ESYNC
  * @retval The new state of __INTERRUPT__ (SET or RESET).
  */
#define __HAL_RCC_CRS_GET_IT_SOURCE(__INTERRUPT__)     ((CRS->CR & (__INTERRUPT__))? SET : RESET)

/** @brief  Clear the CRS interrupt pending bits
  *         bits to clear the selected interrupt pending bits.
  * @param  __INTERRUPT__ specifies the interrupt pending bit to clear.
  *         This parameter can be any combination of the following values:
  *              @arg @ref RCC_CRS_IT_SYNCOK
  *              @arg @ref RCC_CRS_IT_SYNCWARN
  *              @arg @ref RCC_CRS_IT_ERR
  *              @arg @ref RCC_CRS_IT_ESYNC
  *              @arg @ref RCC_CRS_IT_TRIMOVF
  *              @arg @ref RCC_CRS_IT_SYNCERR
  *              @arg @ref RCC_CRS_IT_SYNCMISS
  */
#define __HAL_RCC_CRS_CLEAR_IT(__INTERRUPT__)  do { \
                                                 if(((__INTERRUPT__) & RCC_CRS_IT_ERROR_MASK) != RESET) \
                                                 { \
                                                   WRITE_REG(CRS->ICR, CRS_ICR_ERRC | ((__INTERRUPT__) & ~RCC_CRS_IT_ERROR_MASK)); \
                                                 } \
                                                 else \
                                                 { \
                                                   WRITE_REG(CRS->ICR, (__INTERRUPT__)); \
                                                 } \
                                               } while(0)

/**
  * @brief  Checks whether the specified CRS flag is set or not.
  * @param  __FLAG__ specifies the flag to check.
  *          This parameter can be one of the following values:
  *              @arg @ref RCC_CRS_FLAG_SYNCOK
  *              @arg @ref RCC_CRS_FLAG_SYNCWARN
  *              @arg @ref RCC_CRS_FLAG_ERR
  *              @arg @ref RCC_CRS_FLAG_ESYNC
  *              @arg @ref RCC_CRS_FLAG_TRIMOVF
  *              @arg @ref RCC_CRS_FLAG_SYNCERR
  *              @arg @ref RCC_CRS_FLAG_SYNCMISS
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_RCC_CRS_GET_FLAG(__FLAG__)  ((CRS->ISR & (__FLAG__)) == (__FLAG__))

/**
  * @brief  Clears the CRS specified FLAG.
  * @param __FLAG__ specifies the flag to clear.
  *          This parameter can be one of the following values:
  *              @arg @ref RCC_CRS_FLAG_SYNCOK
  *              @arg @ref RCC_CRS_FLAG_SYNCWARN
  *              @arg @ref RCC_CRS_FLAG_ERR
  *              @arg @ref RCC_CRS_FLAG_ESYNC
  *              @arg @ref RCC_CRS_FLAG_TRIMOVF
  *              @arg @ref RCC_CRS_FLAG_SYNCERR
  *              @arg @ref RCC_CRS_FLAG_SYNCMISS
  * @retval None
  */
#define __HAL_RCC_CRS_CLEAR_FLAG(__FLAG__)     do { \
                                                 if(((__FLAG__) & RCC_CRS_FLAG_ERROR_MASK) != RESET) \
                                                 { \
                                                   WRITE_REG(CRS->ICR, CRS_ICR_ERRC | ((__FLAG__) & ~RCC_CRS_FLAG_ERROR_MASK)); \
                                                 } \
                                                 else \
                                                 { \
                                                   WRITE_REG(CRS->ICR, (__FLAG__)); \
                                                 } \
                                               } while(0)

/**
  * @brief  Enables the oscillator clock for frequency error counter.
  * @note   when the CEN bit is set the CRS_CFGR register becomes write-protected.
  * @retval None
  */
#define __HAL_RCC_CRS_FREQ_ERROR_COUNTER_ENABLE() SET_BIT(CRS->CR, CRS_CR_CEN)

/**
  * @brief  Disables the oscillator clock for frequency error counter.
  * @retval None
  */
#define __HAL_RCC_CRS_FREQ_ERROR_COUNTER_DISABLE()  CLEAR_BIT(CRS->CR, CRS_CR_CEN)

/**
  * @brief  Enables the automatic hardware adjustment of TRIM bits.
  * @note   When the AUTOTRIMEN bit is set the CRS_CFGR register becomes write-protected.
  * @retval None
  */
#define __HAL_RCC_CRS_AUTOMATIC_CALIB_ENABLE()  SET_BIT(CRS->CR, CRS_CR_AUTOTRIMEN)

/**
  * @brief  Enables or disables the automatic hardware adjustment of TRIM bits.
  * @retval None
  */
#define __HAL_RCC_CRS_AUTOMATIC_CALIB_DISABLE()  CLEAR_BIT(CRS->CR, CRS_CR_AUTOTRIMEN)

/**
  * @brief  Macro to calculate reload value to be set in CRS register according to target and sync frequencies
  * @note   The RELOAD value should be selected according to the ratio between the target frequency and the frequency 
  *             of the synchronization source after prescaling. It is then decreased by one in order to 
  *             reach the expected synchronization on the zero value. The formula is the following:
  *             RELOAD = (fTARGET / fSYNC) -1
  * @param  __FTARGET__ Target frequency (value in Hz)
  * @param  __FSYNC__   Synchronization signal frequency (value in Hz)
  * @retval None
  */
#define __HAL_RCC_CRS_RELOADVALUE_CALCULATE(__FTARGET__, __FSYNC__)  (((__FTARGET__) / (__FSYNC__)) - 1)

#endif /* CRS */


#if defined(RCC_CR_HSIOUTEN)
/** @brief  Enable he HSI OUT .
  * @note   After reset, the HSI output is not available
  */

#define __HAL_RCC_HSI_OUT_ENABLE()   SET_BIT(RCC->CR, RCC_CR_HSIOUTEN)

/** @brief  Disable the HSI OUT .
  * @note   After reset, the HSI output is not available
  */

#define __HAL_RCC_HSI_OUT_DISABLE()  CLEAR_BIT(RCC->CR, RCC_CR_HSIOUTEN)

#endif /* RCC_CR_HSIOUTEN */

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)\
     || defined(STM32L052xx) || defined(STM32L062xx) || defined(STM32L072xx) || defined(STM32L082xx)

/**
  * @brief  Enable the Internal High Speed oscillator for USB (HSI48).
  * @note   After enabling the HSI48, the application software should wait on 
  *         HSI48RDY flag to be set indicating that HSI48 clock is stable and can
  *         be used to clock the USB.
  * @note   The HSI48 is stopped by hardware when entering STOP and STANDBY modes.
  */
#define __HAL_RCC_HSI48_ENABLE()  do { SET_BIT(RCC->CRRCR, RCC_CRRCR_HSI48ON);            \
                                       SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);       \
                                       SET_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENREF_HSI48);  \
                                  } while (0)
/**
  * @brief  Disable the Internal High Speed oscillator for USB (HSI48).
  */
#define __HAL_RCC_HSI48_DISABLE()  do { CLEAR_BIT(RCC->CRRCR, RCC_CRRCR_HSI48ON);   \
                                        CLEAR_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENREF_HSI48);  \
                                   } while (0)

/** @brief  Macro to get the Internal 48Mhz High Speed oscillator (HSI48) state.
  * @retval The clock source can be one of the following values:
  *            @arg @ref RCC_HSI48_ON  HSI48 enabled
  *            @arg @ref RCC_HSI48_OFF HSI48 disabled
  */
#define __HAL_RCC_GET_HSI48_STATE() \
                  (((uint32_t)(READ_BIT(RCC->CRRCR, RCC_CRRCR_HSI48ON)) != RESET) ? RCC_HSI48_ON : RCC_HSI48_OFF)  

/** @brief  Enable or disable the HSI48M DIV6 OUT .
  * @note   After reset, the HSI48Mhz (divided by 6) output is not available
  */

#define __HAL_RCC_HSI48M_DIV6_OUT_ENABLE()   SET_BIT(RCC->CR, RCC_CRRCR_HSI48DIV6OUTEN)
#define __HAL_RCC_HSI48M_DIV6_OUT_DISABLE()  CLEAR_BIT(RCC->CR, RCC_CRRCR_HSI48DIV6OUTEN)

#endif /* STM32L071xx  ||  STM32L081xx  || */
       /* STM32L072xx  ||  STM32L082xx  || */
       /* STM32L073xx  ||  STM32L083xx     */
       

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


void              HAL_RCCEx_EnableLSECSS(void);
void              HAL_RCCEx_DisableLSECSS(void);
void              HAL_RCCEx_EnableLSECSS_IT(void);
void              HAL_RCCEx_LSECSS_IRQHandler(void);
void              HAL_RCCEx_LSECSS_Callback(void);


#if defined(SYSCFG_CFGR3_ENREF_HSI48)
void HAL_RCCEx_EnableHSI48_VREFINT(void);
void HAL_RCCEx_DisableHSI48_VREFINT(void);
#endif /* SYSCFG_CFGR3_ENREF_HSI48 */

/**
  * @}
  */

#if defined(CRS)

/** @addtogroup RCCEx_Exported_Functions_Group3
  * @{
  */

void              HAL_RCCEx_CRSConfig(RCC_CRSInitTypeDef *pInit);
void              HAL_RCCEx_CRSSoftwareSynchronizationGenerate(void);
void              HAL_RCCEx_CRSGetSynchronizationInfo(RCC_CRSSynchroInfoTypeDef *pSynchroInfo);
uint32_t          HAL_RCCEx_CRSWaitSynchronization(uint32_t Timeout);
void              HAL_RCCEx_CRS_IRQHandler(void);
void              HAL_RCCEx_CRS_SyncOkCallback(void);
void              HAL_RCCEx_CRS_SyncWarnCallback(void);
void              HAL_RCCEx_CRS_ExpectedSyncCallback(void);
void              HAL_RCCEx_CRS_ErrorCallback(uint32_t Error);

/**
  * @}
  */

#endif /* CRS */

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

#endif /* __STM32L0xx_HAL_RCC_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

