/**
  ******************************************************************************
  * @file    stm32l0xx_hal_rcc_ex.h
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
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

/** @defgroup RCCEx RCCEx
  * @{
  */ 

/* Exported types ------------------------------------------------------------*/ 
 /** @defgroup RCCEx_Exported_Types RCCEx Exported Types
  * @{
  */

/** 
  * @brief  RCC extended clocks structure definition  
  */
#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx)  && !defined(STM32L081xx) 
typedef struct
{
  uint32_t PeriphClockSelection;   /*!< The Extended Clock to be configured.
                                        This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */
  uint32_t Usart1ClockSelection;   /*!< USART1 clock source      
                                        This parameter can be a value of @ref RCCEx_USART1_Clock_Source */
                                   
  uint32_t Usart2ClockSelection;   /*!< USART2 clock source      
                                        This parameter can be a value of @ref RCCEx_USART2_Clock_Source */
                                   
  uint32_t Lpuart1ClockSelection;  /*!< LPUART1 clock source      
                                        This parameter can be a value of @ref RCCEx_LPUART1_Clock_Source */
                                   
  uint32_t I2c1ClockSelection;     /*!< I2C1 clock source      
                                        This parameter can be a value of @ref RCCEx_I2C1_Clock_Source */
#if defined (STM32L072xx) || defined(STM32L073xx) || defined(STM32L082xx) || defined(STM32L083xx)
  uint32_t I2c3ClockSelection;     /*!< I2C3 clock source      
                                        This parameter can be a value of @ref RCCEx_I2C3_Clock_Source */
#endif
                                   
  uint32_t RTCClockSelection;      /*!< Specifies RTC Clock Prescalers Selection
                                        This parameter can be a value of @ref RCC_RTC_Clock_Source */
#if defined (STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)
  uint32_t LCDClockSelection;         /*!< specifies the LCD clock source.
                                           This parameter can be a value of @ref RCC_RTC_Clock_Source */
#endif
                                                                         
  uint32_t UsbClockSelection;      /*!< Specifies USB and RNG Clock  Selection
                                        This parameter can be a value of @ref RCCEx_USB_Clock_Source */

  uint32_t LptimClockSelection;    /*!< LPTIM1 clock source
                                        This parameter can be a value of @ref RCCEx_LPTIM1_Clock_Source */
  
}RCC_PeriphCLKInitTypeDef;


#else /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx) && !(STM32L041xx) && !(STM32L051xx) && !(STM32L061xx) && !(STM32L071xx) && !(STM32L081xx) */

typedef struct
{
  uint32_t PeriphClockSelection;   /*!< The Extended Clock to be configured.
                                        This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */
#if !defined(STM32L011xx) && !defined(STM32L021xx) &&  !defined (STM32L031xx) && !defined (STM32L041xx)
  uint32_t Usart1ClockSelection;   /*!< USART1 clock source      
                                        This parameter can be a value of @ref RCCEx_USART1_Clock_Source */
#endif
  uint32_t Usart2ClockSelection;   /*!< USART2 clock source      
                                        This parameter can be a value of @ref RCCEx_USART2_Clock_Source */
                                   
  uint32_t Lpuart1ClockSelection;  /*!< LPUART1 clock source      
                                        This parameter can be a value of @ref RCCEx_LPUART1_Clock_Source */
                                   
  uint32_t I2c1ClockSelection;     /*!< I2C1 clock source      
                                        This parameter can be a value of @ref RCCEx_I2C1_Clock_Source */
                                   
#if defined (STM32L071xx) || defined(STM32L081xx)
  uint32_t I2c3ClockSelection;     /*!< I2C3 clock source      
                                        This parameter can be a value of @ref RCCEx_I2C3_Clock_Source */
#endif

  uint32_t RTCClockSelection;      /*!< Specifies RTC Clock Prescalers Selection
                                        This parameter can be a value of @ref RCC_RTC_Clock_Source */
                                                                         
  uint32_t LptimClockSelection;    /*!< LPTIM1 clock source
                                        This parameter can be a value of @ref RCCEx_LPTIM1_Clock_Source */
  
}RCC_PeriphCLKInitTypeDef;

#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx) && !(STM32L041xx) && !(STM32L051xx) && !(STM32L061xx) && !(STM32L071xx) && !(STM32L081xx) */

/**
  * @}
  */

/** @defgroup RCCEx_Exported_Constants RCCEx Exported Constants
  * @{
  */
/**
  * @}
  */

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx)  && !defined(STM32L081xx)

/** @addtogroup RCCEx_Exported_Constants
  * @{
  */
/** 
  * @brief  RCC CRS Status definition  
  */  

#define  RCC_CRS_NONE       ((uint32_t) 0x00000000U)
#define  RCC_CRS_TIMEOUT    ((uint32_t) 0x00000001U)
#define  RCC_CRS_SYNCOK     ((uint32_t) 0x00000002U)
#define  RCC_CRS_SYNCWARN   ((uint32_t) 0x00000004U)
#define  RCC_CRS_SYNCERR    ((uint32_t) 0x00000008U)
#define  RCC_CRS_SYNCMISS   ((uint32_t) 0x00000010U)
#define  RCC_CRS_TRIMOVF    ((uint32_t) 0x00000020U)

/**
  * @}
  */

 /** @defgroup RCCEx_Exported_Types RCCEx Exported Types
  * @{
  */
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
                                      It can be calculated in using macro __HAL_RCC_CRS_CALCULATE_RELOADVALUE(_FTARGET_, _FSYNC_)
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
                                     This parameter must be a number between 0 and 0xFFFF*/

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

/**
  * @}
  */

#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx ) && !(STM32L071xx ) && !(STM32L081xx ) */
 
/* Exported constants --------------------------------------------------------*/
/** @addtogroup RCCEx_Exported_Constants
  * @{
  */

/** @defgroup RCCEx_Periph_Clock_Selection RCC Periph Clock Selection
  * @{
  */
#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) 

#define RCC_PERIPHCLK_USART1           ((uint32_t)0x00000001U)
#define RCC_PERIPHCLK_USART2           ((uint32_t)0x00000002U)
#define RCC_PERIPHCLK_LPUART1          ((uint32_t)0x00000004U)
#define RCC_PERIPHCLK_I2C1             ((uint32_t)0x00000008U)
#define RCC_PERIPHCLK_I2C2             ((uint32_t)0x00000010U)
#define RCC_PERIPHCLK_RTC              ((uint32_t)0x00000020U)
#define RCC_PERIPHCLK_USB              ((uint32_t)0x00000040U)
#define RCC_PERIPHCLK_LPTIM1           ((uint32_t)0x00000080U)
#if defined (STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)
#define RCC_PERIPHCLK_LCD              ((uint32_t)0x00000800U)
#endif
#if defined (STM32L072xx) || defined(STM32L073xx) || defined(STM32L082xx) || defined(STM32L083xx)
#define RCC_PERIPHCLK_I2C3             ((uint32_t)0x00000100U)
#endif


#else /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx) && !(STM32L041xx) && !(STM32L051xx) && !(STM32L061xx) && !(STM32L071xx) && !(STM32L081xx) */

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx)
#define RCC_PERIPHCLK_USART1           ((uint32_t)0x00000001U)
#endif
#define RCC_PERIPHCLK_USART2           ((uint32_t)0x00000002U)
#define RCC_PERIPHCLK_LPUART1          ((uint32_t)0x00000004U)
#define RCC_PERIPHCLK_I2C1             ((uint32_t)0x00000008U)
#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx)
#define RCC_PERIPHCLK_I2C2             ((uint32_t)0x00000010U)
#endif
#define RCC_PERIPHCLK_RTC              ((uint32_t)0x00000020U)
#define RCC_PERIPHCLK_LPTIM1           ((uint32_t)0x00000080U)
#if defined(STM32L071xx) || defined(STM32L081xx)
#define RCC_PERIPHCLK_I2C3             ((uint32_t)0x00000100U)
#endif

#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx) && !(STM32L041xx) && !(STM32L061xx) && !(STM32L071xx) && !(STM32L081xx) */
/**
  * @}
  */

/** @defgroup RCCEx_USART1_Clock_Source RCC USART1 Clock Source
  * @{
  */
#define RCC_USART1CLKSOURCE_PCLK2        ((uint32_t)0x00000000U) 
#define RCC_USART1CLKSOURCE_SYSCLK       RCC_CCIPR_USART1SEL_0
#define RCC_USART1CLKSOURCE_HSI          RCC_CCIPR_USART1SEL_1
#define RCC_USART1CLKSOURCE_LSE          (RCC_CCIPR_USART1SEL_0 | RCC_CCIPR_USART1SEL_1)

/**
  * @}
  */

/** @defgroup RCCEx_USART2_Clock_Source RCC USART2 Clock Source
  * @{
  */
#define RCC_USART2CLKSOURCE_PCLK1        ((uint32_t)0x00000000U) 
#define RCC_USART2CLKSOURCE_SYSCLK       RCC_CCIPR_USART2SEL_0
#define RCC_USART2CLKSOURCE_HSI          RCC_CCIPR_USART2SEL_1
#define RCC_USART2CLKSOURCE_LSE          (RCC_CCIPR_USART2SEL_0 | RCC_CCIPR_USART2SEL_1)

/**
  * @}
  */

/** @defgroup RCCEx_LPUART1_Clock_Source RCC LPUART Clock Source
  * @{
  */
#define RCC_LPUART1CLKSOURCE_PCLK1        ((uint32_t)0x00000000U) 
#define RCC_LPUART1CLKSOURCE_SYSCLK       RCC_CCIPR_LPUART1SEL_0
#define RCC_LPUART1CLKSOURCE_HSI          RCC_CCIPR_LPUART1SEL_1
#define RCC_LPUART1CLKSOURCE_LSE          (RCC_CCIPR_LPUART1SEL_0 | RCC_CCIPR_LPUART1SEL_1)

/**
  * @}
  */

/** @defgroup RCCEx_I2C1_Clock_Source RCC I2C1 Clock Source
  * @{
  */
#define RCC_I2C1CLKSOURCE_PCLK1          ((uint32_t)0x00000000U) 
#define RCC_I2C1CLKSOURCE_SYSCLK         RCC_CCIPR_I2C1SEL_0
#define RCC_I2C1CLKSOURCE_HSI            RCC_CCIPR_I2C1SEL_1

/**
  * @}
  */

#if defined(STM32L071xx) || defined(STM32L072xx) || defined(STM32L073xx)|| defined(STM32L081xx) || defined(STM32L082xx) || defined(STM32L083xx)  

/** @defgroup RCCEx_I2C3_Clock_Source RCC I2C3 Clock Source
  * @{
  */
#define RCC_I2C3CLKSOURCE_PCLK1          ((uint32_t)0x00000000U) 
#define RCC_I2C3CLKSOURCE_SYSCLK         RCC_CCIPR_I2C3SEL_0
#define RCC_I2C3CLKSOURCE_HSI            RCC_CCIPR_I2C3SEL_1

/**
  * @}
  */
#endif /* defined(STM32L071xx) || defined(STM32L072xx) || defined(STM32L073xx) || defined(STM32L081xx)|| defined(STM32L082xx) || defined(STM32L083xx) */



/** @defgroup RCCEx_TIM_PRescaler_Selection RCC TIM Prescaler Selection
  * @{
  */
#define RCC_TIMPRES_DESACTIVATED        ((uint8_t)0x00U)
#define RCC_TIMPRES_ACTIVATED           ((uint8_t)0x01U)
/**
  * @}
  */

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) 
/** @defgroup RCCEx_USB_Clock_Source RCC USB Clock Source
  * @{
  */
#define RCC_USBCLKSOURCE_HSI48           RCC_CCIPR_HSI48SEL
#define RCC_USBCLKSOURCE_PLL          ((uint32_t)0x00000000U)

/**
  * @}
  */
  
/** @defgroup RCCEx_RNG_Clock_Source RCC RNG Clock Source
  * @{
  */
#define RCC_RNGCLKSOURCE_HSI48           RCC_CCIPR_HSI48SEL
#define RCC_RNGCLKSOURCE_PLLCLK          ((uint32_t)0x00000000U)

/**
  * @}
  */  

/** @defgroup RCCEx_HSI48M_Clock_Source RCC HSI48M Clock Source
  * @{
  */
#define RCC_FLAG_HSI48               SYSCFG_CFGR3_VREFINT_RDYF

#define RCC_HSI48M_PLL                 ((uint32_t)0x00000000U)
#define RCC_HSI48M_HSI48                 RCC_CCIPR_HSI48SEL


/**
  * @}
  */
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx ) && !(STM32L071xx ) && !(STM32L081xx ) */ 

/** @defgroup RCC_HSI_Config RCC HSI Configuration
  * @{
  */
#define RCC_HSI_OFF                      ((uint8_t)0x00U)
#define RCC_HSI_ON                       RCC_CR_HSION
#define RCC_HSI_DIV4                     (RCC_CR_HSIDIVEN | RCC_CR_HSION)
#if defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L071xx) || defined(STM32L081xx)
#define RCC_HSI_OUTEN                    RCC_CR_HSIOUTEN
#endif

/**
  * @}
  */ 

/** @defgroup RCCEx_LPTIM1_Clock_Source RCC LPTIM1 Clock Source
  * @{
  */
#define RCC_LPTIM1CLKSOURCE_PCLK        ((uint32_t)0x00000000U)
#define RCC_LPTIM1CLKSOURCE_LSI         RCC_CCIPR_LPTIM1SEL_0
#define RCC_LPTIM1CLKSOURCE_HSI         RCC_CCIPR_LPTIM1SEL_1
#define RCC_LPTIM1CLKSOURCE_LSE         RCC_CCIPR_LPTIM1SEL

/**
  * @}
  */

/** @defgroup RCCEx_StopWakeUp_Clock RCC StopWakeUp Clock
  * @{
  */

#define RCC_STOP_WAKEUPCLOCK_MSI                ((uint32_t)0x00U)
#define RCC_STOP_WAKEUPCLOCK_HSI                RCC_CFGR_STOPWUCK

/**
  * @}
  */ 

/** @defgroup RCCEx_LSEDrive_Configuration RCC LSE Drive Configuration
  * @{
  */

#define RCC_LSEDRIVE_LOW                 ((uint32_t)0x00000000U)
#define RCC_LSEDRIVE_MEDIUMLOW           RCC_CSR_LSEDRV_0
#define RCC_LSEDRIVE_MEDIUMHIGH          RCC_CSR_LSEDRV_1
#define RCC_LSEDRIVE_HIGH                RCC_CSR_LSEDRV

/**
  * @}
  */  

/** @defgroup RCCEx_EXTI_LINE_LSECSS  RCC LSE CSS external interrupt line
  * @{
  */
#define RCC_EXTI_LINE_LSECSS             (EXTI_IMR_IM19)         /*!< External interrupt line 19 connected to the LSE CSS EXTI Line */
/**
  * @}
  */

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) 
/** @defgroup RCCEx_CRS_SynchroSource RCC CRS Synchro Source
  * @{
  */
#define RCC_CRS_SYNC_SOURCE_GPIO       ((uint32_t)0x00U)        /*!< Synchro Signal source GPIO */
#define RCC_CRS_SYNC_SOURCE_LSE        CRS_CFGR_SYNCSRC_0      /*!< Synchro Signal source LSE */
#define RCC_CRS_SYNC_SOURCE_USB        CRS_CFGR_SYNCSRC_1      /*!< Synchro Signal source USB SOF (default)*/
  
/**
  * @}
  */

/** @defgroup RCCEx_CRS_SynchroDivider RCC CRS Synchro Divider
  * @{
  */
#define RCC_CRS_SYNC_DIV1        ((uint32_t)0x00U)                          /*!< Synchro Signal not divided (default) */
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

/** @defgroup RCCEx_CRS_SynchroPolarity RCC CRS Synchro Polarity
  * @{
  */
#define RCC_CRS_SYNC_POLARITY_RISING        ((uint32_t)0x00U)      /*!< Synchro Active on rising edge (default) */
#define RCC_CRS_SYNC_POLARITY_FALLING       CRS_CFGR_SYNCPOL      /*!< Synchro Active on falling edge */
  
/**
  * @}
  */
  
/** @defgroup RCCEx_CRS_ReloadValueDefault RCC CRS Reload Default Value
  * @{
  */
#define RCC_CRS_RELOADVALUE_DEFAULT         ((uint32_t)0xBB7FU)      /*!< The reset value of the RELOAD field corresponds 
                                                              to a target frequency of 48 MHz and a synchronization signal frequency of 1 kHz (SOF signal from USB). */
    
/**
  * @}
  */
  
/** @defgroup RCCEx_CRS_ErrorLimitDefault RCC CRS Error Limit Default
  * @{
  */
#define RCC_CRS_ERRORLIMIT_DEFAULT          ((uint32_t)0x22U)      /*!< Default Frequency error limit */
    
/**
  * @}
  */

/** @defgroup RCCEx_CRS_HSI48CalibrationDefault RCC CRS HSI48 Calibration Default
  * @{
  */
#define RCC_CRS_HSI48CALIBRATION_DEFAULT    ((uint32_t)0x20U)      /*!< The default value is 32, which corresponds to the middle of the trimming interval. 
                                                                The trimming step is around 67 kHz between two consecutive TRIM steps. A higher TRIM value
                                                                corresponds to a higher output frequency */
    
/**
  * @}
  */

/** @defgroup RCCEx_CRS_FreqErrorDirection RCC CRS Frequency Error Direction
  * @{
  */
#define RCC_CRS_FREQERRORDIR_UP             ((uint32_t)0x00U)          /*!< Upcounting direction, the actual frequency is above the target */
#define RCC_CRS_FREQERRORDIR_DOWN           ((uint32_t)CRS_ISR_FEDIR) /*!< Downcounting direction, the actual frequency is below the target */
    
/**
  * @}
  */

/** @defgroup RCCEx_CRS_Interrupt_Sources RCC CRS Interrupt Sources
  * @{
  */
#define RCC_CRS_IT_SYNCOK             CRS_CR_SYNCOKIE    /*!< SYNC event OK */
#define RCC_CRS_IT_SYNCWARN           CRS_CR_SYNCWARNIE  /*!< SYNC warning */
#define RCC_CRS_IT_ERR                CRS_CR_ERRIE       /*!< error */
#define RCC_CRS_IT_ESYNC              CRS_CR_ESYNCIE     /*!< Expected SYNC */
#define RCC_CRS_IT_TRIMOVF            CRS_CR_ERRIE       /*!< Trimming overflow or underflow */
#define RCC_CRS_IT_SYNCERR            CRS_CR_ERRIE       /*!< SYNC error */
#define RCC_CRS_IT_SYNCMISS           CRS_CR_ERRIE       /*!< SYNC missed*/

/**
  * @}
  */
  
/** @defgroup RCCEx_CRS_Flags RCC CRS Flags
  * @{
  */
#define RCC_CRS_FLAG_SYNCOK             CRS_ISR_SYNCOKF     /* SYNC event OK flag     */
#define RCC_CRS_FLAG_SYNCWARN           CRS_ISR_SYNCWARNF   /* SYNC warning flag      */
#define RCC_CRS_FLAG_ERR                CRS_ISR_ERRF        /* Error flag        */
#define RCC_CRS_FLAG_ESYNC              CRS_ISR_ESYNCF      /* Expected SYNC flag     */
#define RCC_CRS_FLAG_TRIMOVF            CRS_ISR_TRIMOVF     /*!< Trimming overflow or underflow */
#define RCC_CRS_FLAG_SYNCERR            CRS_ISR_SYNCERR     /*!< SYNC error */
#define RCC_CRS_FLAG_SYNCMISS           CRS_ISR_SYNCMISS    /*!< SYNC missed*/

/**
  * @}
  */

#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx ) && !(STM32L071xx ) && !(STM32L081xx ) */  
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
#define __HAL_RCC_AES_CLK_ENABLE()          SET_BIT(RCC->AHBENR, (RCC_AHBENR_CRYPEN))
#define __HAL_RCC_AES_CLK_DISABLE()         CLEAR_BIT(RCC->AHBENR, (RCC_AHBENR_CRYPEN))
#endif /* STM32L062xx || STM32L063xx || STM32L072xx  || STM32L073xx || STM32L082xx  || STM32L083xx || STM32L041xx || STM32L021xx */

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) 
#define __HAL_RCC_TSC_CLK_ENABLE()             SET_BIT(RCC->AHBENR, (RCC_AHBENR_TSCEN))
#define __HAL_RCC_TSC_CLK_DISABLE()            CLEAR_BIT(RCC->AHBENR, (RCC_AHBENR_TSCEN))

#define __HAL_RCC_RNG_CLK_ENABLE()            SET_BIT(RCC->AHBENR, (RCC_AHBENR_RNGEN))
#define __HAL_RCC_RNG_CLK_DISABLE()           CLEAR_BIT(RCC->AHBENR, (RCC_AHBENR_RNGEN))
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx ) && !(STM32L071xx ) && !(STM32L081xx ) */


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
#if defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L071xx) || defined(STM32L081xx)
#define __HAL_RCC_GPIOE_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIOEEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOEEN);\
                                        UNUSED(tmpreg); \
                                      } while(0)

#define __HAL_RCC_GPIOE_CLK_DISABLE()        CLEAR_BIT(RCC->IOPENR,(RCC_IOPENR_GPIOEEN))

#endif /* STM32L071xx  ||  STM32L081xx  || */
       /* STM32L072xx  ||  STM32L082xx  || */
       /* STM32L073xx  ||  STM32L083xx     */
#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx)
#define __HAL_RCC_GPIOD_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIODEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIODEN);\
                                        UNUSED(tmpreg); \
                                      } while(0)
#define __HAL_RCC_GPIOD_CLK_DISABLE()        CLEAR_BIT(RCC->IOPENR,(RCC_IOPENR_GPIODEN))
#endif  /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) */
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

#define __HAL_RCC_CRS_CLK_ENABLE()     SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_CRSEN))
#define __HAL_RCC_CRS_CLK_DISABLE()    CLEAR_BIT(RCC->APB1ENR,(RCC_APB1ENR_CRSEN))
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx ) && !(STM32L071xx ) && !(STM32L081xx ) */
       

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)
#define __HAL_RCC_LCD_CLK_ENABLE()          SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_LCDEN))
#define __HAL_RCC_LCD_CLK_DISABLE()         CLEAR_BIT(RCC->APB1ENR, (RCC_APB1ENR_LCDEN))
#endif /* STM32L053xx || STM32L063xx || STM32L073xx  || STM32L083xx */

#if defined(STM32L053xx) || defined(STM32L063xx) || \
    defined(STM32L052xx) || defined(STM32L062xx) || \
    defined(STM32L051xx) || defined(STM32L061xx)
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
#endif /* STM32L011xx  || STM32L021xx  || STM32L031xx  || STM32L041xx   */


#if defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L071xx) || defined(STM32L081xx)
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
#endif /* STM32L071xx  ||  STM32L081xx  || */
       /* STM32L072xx  ||  STM32L082xx  || */
       /* STM32L073xx  ||  STM32L083xx     */

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L052xx) || defined(STM32L062xx) || defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L051xx) || defined(STM32L061xx) || defined(STM32L071xx) || defined(STM32L081xx) || \
    defined(STM32L031xx) || defined(STM32L041xx) || defined(STM32L011xx) || defined(STM32L021xx) 
 /**
  * @}
  */

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
#if defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L071xx) || defined(STM32L081xx)
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

#if defined(STM32L053xx) || defined(STM32L063xx) || \
    defined(STM32L052xx) || defined(STM32L062xx) || \
    defined(STM32L051xx) || defined(STM32L061xx)  
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

#if defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L071xx) || defined(STM32L081xx) 
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

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L052xx) || defined(STM32L062xx) || defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L051xx) || defined(STM32L061xx) || defined(STM32L071xx) || defined(STM32L081xx)

/**
  * @}
  */

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
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) &&  !(STM32L041xx ) &&  !(STM32L051xx ) && !(STM32L061xx ) && !(STM32L071xx ) && !(STM32L081xx ) */
       
#if defined(STM32L062xx) || defined(STM32L063xx)|| defined(STM32L082xx) || defined(STM32L083xx) || defined(STM32L041xx)
#define __HAL_RCC_AES_CLK_SLEEP_ENABLE()          SET_BIT(RCC->AHBLPENR, (RCC_AHBSMENR_CRYPSMEN))
#define __HAL_RCC_AES_CLK_SLEEP_DISABLE()         CLEAR_BIT(RCC->AHBLPENR, (RCC_AHBSMENR_CRYPSMEN))
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
#if defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L071xx) || defined(STM32L081xx) 
#define __HAL_RCC_GPIOE_CLK_SLEEP_ENABLE()         SET_BIT(RCC->IOPSMENR, (RCC_IOPSMENR_GPIOESMEN))
#define __HAL_RCC_GPIOE_CLK_SLEEP_DISABLE()        CLEAR_BIT(RCC->IOPSMENR,(RCC_IOPSMENR_GPIOESMEN))

#endif /* STM32L071xx  ||  STM32L081xx  || */
       /* STM32L072xx  ||  STM32L082xx  || */
       /* STM32L073xx  ||  STM32L083xx  || */
#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx)
#define __HAL_RCC_GPIOD_CLK_SLEEP_ENABLE()    SET_BIT(RCC->IOPSMENR, (RCC_IOPSMENR_GPIODSMEN))
#define __HAL_RCC_GPIOD_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->IOPSMENR,(RCC_IOPSMENR_GPIODSMEN))
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

#if defined(STM32L053xx) || defined(STM32L063xx) || \
    defined(STM32L052xx) || defined(STM32L062xx) || \
    defined(STM32L051xx) || defined(STM32L061xx) 
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
#endif /* STM32L051xx  || STM32L061xx  || */
       /* STM32L052xx  || STM32L062xx  || */
       /* STM32L053xx  || STM32L063xx     */
       
#if defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L071xx) || defined(STM32L081xx)
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
#endif /*  STM32L031xx  ||  STM32L041xx || STM32L011xx  || STM32L021xx */

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) 
#define __HAL_RCC_USB_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USBSMEN))
#define __HAL_RCC_USB_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_USBSMEN))
#define __HAL_RCC_CRS_CLK_SLEEP_ENABLE()    SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_CRSSMEN))
#define __HAL_RCC_CRS_CLK_SLEEP_DISABLE()   CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_CRSSMEN))
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx )  && !(STM32L071xx ) && !(STM32L081xx ) */

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)
#define __HAL_RCC_LCD_CLK_SLEEP_ENABLE()      SET_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LCDSMEN))
#define __HAL_RCC_LCD_CLK_SLEEP_DISABLE()     CLEAR_BIT(RCC->APB1SMENR, (RCC_APB1SMENR_LCDSMEN))
#endif /* STM32L053xx || STM32L063xx || STM32L073xx  || STM32L083xx */

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L052xx) || defined(STM32L062xx) || defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L051xx) || defined(STM32L061xx) || defined(STM32L071xx) || defined(STM32L081xx) || \
	defined(STM32L031xx) || defined(STM32L041xx) || defined(STM32L011xx) || defined(STM32L021xx) 

/**
  * @}
  */

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
#endif /* STM32L051xx  || STM32L061xx  || STM32L071xx  ||  STM32L081xx  || */
       /* STM32L052xx  || STM32L062xx  || STM32L072xx  ||  STM32L082xx  || */
       /* STM32L053xx  || STM32L063xx  || STM32L073xx  ||  STM32L083xx  || */
	     /* STM32L031xx  || STM32L041xx  || STM32L011xx  ||  STM32L021xx   */

/** @brief Macro to configures LCD clock (LCDCLK).
  *  @note   LCD and RTC use the same configuration
  *  @note   LCD can however be used in the Stop low power mode if the LSE or LSI is used as the
  *   LCD clock source.
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

/** @brief macros to get the LCD clock source.
  */
#define __HAL_RCC_GET_LCD_SOURCE()              __HAL_RCC_GET_RTC_SOURCE()

/** @brief macros to get the LCD clock pre-scaler.
  */
#define  __HAL_RCC_GET_LCD_HSE_PRESCALER()      __HAL_RCC_GET_RTC_HSE_PRESCALER()
/**
  * @}
  */
          
/** @brief  Macro to configure the I2C1 clock (I2C1CLK).
  *
  * @param  __I2C1_CLKSOURCE__: specifies the I2C1 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_I2C1CLKSOURCE_PCLK1: PCLK1 selected as I2C1 clock  
  *            @arg RCC_I2C1CLKSOURCE_HSI: HSI selected as I2C1 clock
  *            @arg RCC_I2C1CLKSOURCE_SYSCLK: System Clock selected as I2C1 clock 
  * @retval None
  */
#define __HAL_RCC_I2C1_CONFIG(__I2C1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_I2C1SEL, (uint32_t)(__I2C1_CLKSOURCE__))

/** @brief  Macro to get the I2C1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_I2C1CLKSOURCE_PCLK1: PCLK1 selected as I2C1 clock  
  *            @arg RCC_I2C1CLKSOURCE_HSI: HSI selected as I2C1 clock
  *            @arg RCC_I2C1CLKSOURCE_SYSCLK: System Clock selected as I2C1 clock
  */
#define __HAL_RCC_GET_I2C1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_I2C1SEL)))

#if defined (STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L071xx) || defined(STM32L081xx)
/** @brief  Macro to configure the I2C3 clock (I2C3CLK).
  *
  * @param  __I2C3_CLKSOURCE__: specifies the I2C3 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_I2C3CLKSOURCE_PCLK1: PCLK1 selected as I2C3 clock  
  *            @arg RCC_I2C3CLKSOURCE_HSI: HSI selected as I2C3 clock
  *            @arg RCC_I2C3CLKSOURCE_SYSCLK: System Clock selected as I2C3 clock
  * @retval None
  */
#define __HAL_RCC_I2C3_CONFIG(__I2C3_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_I2C3SEL, (uint32_t)(__I2C3_CLKSOURCE__))

/** @brief  Macro to get the I2C3 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_I2C3CLKSOURCE_PCLK1: PCLK1 selected as I2C3 clock  
  *            @arg RCC_I2C3CLKSOURCE_HSI: HSI selected as I2C3 clock
  *            @arg RCC_I2C3CLKSOURCE_SYSCLK: System Clock selected as I2C3 clock
  */
#define __HAL_RCC_GET_I2C3_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_I2C3SEL)))

#endif /*  STM32L071xx  ||  STM32L081xx  || */
       /*  STM32L072xx  ||  STM32L082xx  || */
       /*  STM32L073xx  ||  STM32L083xx  || */

/** @brief  Macro to configure the USART1 clock (USART1CLK).
  *
  * @param  __USART1_CLKSOURCE__: specifies the USART1 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_USART1CLKSOURCE_PCLK2: PCLK2 selected as USART1 clock
  *            @arg RCC_USART1CLKSOURCE_HSI: HSI selected as USART1 clock
  *            @arg RCC_USART1CLKSOURCE_SYSCLK: System Clock selected as USART1 clock
  *            @arg RCC_USART1CLKSOURCE_LSE: LSE selected as USART1 clock
  * @retval None
  */
#define __HAL_RCC_USART1_CONFIG(__USART1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_USART1SEL, (uint32_t)(__USART1_CLKSOURCE__))

/** @brief  Macro to get the USART1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_USART1CLKSOURCE_PCLK2: PCLK2 selected as USART1 clock
  *            @arg RCC_USART1CLKSOURCE_HSI: HSI selected as USART1 clock
  *            @arg RCC_USART1CLKSOURCE_SYSCLK: System Clock selected as USART1 clock
  *            @arg RCC_USART1CLKSOURCE_LSE: LSE selected as USART1 clock
  */
#define __HAL_RCC_GET_USART1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_USART1SEL)))

/** @brief  Macro to configure the USART2 clock (USART2CLK).
  *
  * @param  __USART2_CLKSOURCE__: specifies the USART2 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_USART2CLKSOURCE_PCLK1: PCLK1 selected as USART2 clock
  *            @arg RCC_USART2CLKSOURCE_HSI: HSI selected as USART2 clock
  *            @arg RCC_USART2CLKSOURCE_SYSCLK: System Clock selected as USART2 clock
  *            @arg RCC_USART2CLKSOURCE_LSE: LSE selected as USART2 clock
  * @retval None
  */
#define __HAL_RCC_USART2_CONFIG(__USART2_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_USART2SEL, (uint32_t)(__USART2_CLKSOURCE__))

/** @brief  Macro to get the USART2 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_USART2CLKSOURCE_PCLK1: PCLK1 selected as USART2 clock
  *            @arg RCC_USART2CLKSOURCE_HSI: HSI selected as USART2 clock
  *            @arg RCC_USART2CLKSOURCE_SYSCLK: System Clock selected as USART2 clock
  *            @arg RCC_USART2CLKSOURCE_LSE: LSE selected as USART2 clock
  */
#define __HAL_RCC_GET_USART2_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_USART2SEL)))

/** @brief  Macro to configure the LPUART1 clock (LPUART1CLK).
  *
  * @param  __LPUART1_CLKSOURCE__: specifies the LPUART1 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_LPUART1CLKSOURCE_PCLK1: PCLK1 selected as LPUART1 clock
  *            @arg RCC_LPUART1CLKSOURCE_HSI: HSI selected as LPUART1 clock
  *            @arg RCC_LPUART1CLKSOURCE_SYSCLK: System Clock selected as LPUART1 clock
  *            @arg RCC_LPUART1CLKSOURCE_LSE: LSE selected as LPUART1 clock
  * @retval None
  */
#define __HAL_RCC_LPUART1_CONFIG(__LPUART1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_LPUART1SEL, (uint32_t)(__LPUART1_CLKSOURCE__))

/** @brief  Macro to get the LPUART1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_LPUART1CLKSOURCE_PCLK1: PCLK1 selected as LPUART1 clock
  *            @arg RCC_LPUART1CLKSOURCE_HSI: HSI selected as LPUART1 clock
  *            @arg RCC_LPUART1CLKSOURCE_SYSCLK: System Clock selected as LPUART1 clock
  *            @arg RCC_LPUART1CLKSOURCE_LSE: LSE selected as LPUART1 clock
  */
#define __HAL_RCC_GET_LPUART1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_LPUART1SEL)))

/** @brief  Macro to configure the LPTIM1 clock (LPTIM1CLK).
  *
  * @param  __LPTIM1_CLKSOURCE__: specifies the LPTIM1 clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_LPTIM1CLKSOURCE_PCLK: PCLK selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_LSI : HSI  selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_HSI : LSI  selected as LPTIM1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_LSE : LSE  selected as LPTIM1 clock
  * @retval None
  */
#define __HAL_RCC_LPTIM1_CONFIG(__LPTIM1_CLKSOURCE__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_LPTIM1SEL, (uint32_t)(__LPTIM1_CLKSOURCE__))

/** @brief  Macro to get the LPTIM1 clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_LPTIM1CLKSOURCE_PCLK: PCLK selected as LPUART1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_LSI : HSI selected as LPUART1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_HSI : System Clock selected as LPUART1 clock
  *            @arg RCC_LPTIM1CLKSOURCE_LSE : LSE selected as LPUART1 clock
  */
#define __HAL_RCC_GET_LPTIM1_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_LPTIM1SEL)))

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx)
/** @brief  Macro to configure the USB clock (USBCLK).
  * @param  __USBCLKSource__: specifies the USB clock source.
  *         This parameter can be one of the following values:
  *            @arg RCC_USBCLKSOURCE_HSI48:  HSI48 selected as USB clock
  *            @arg RCC_USBCLKSOURCE_PLL: PLL Clock selected as USB clock
  */
#define __HAL_RCC_USB_CONFIG(__USBCLKSource__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_HSI48SEL, (uint32_t)(__USBCLKSource__))

/** @brief  Macro to get the USB clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_USBCLKSOURCE_HSI48:  HSI48 selected as USB clock
  *            @arg RCC_USBCLKSOURCE_PLL: PLL Clock selected as USB clock
  */
#define __HAL_RCC_GET_USB_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_HSI48SEL)))

/** @brief  Macro to configure the RNG clock (RNGCLK).
  * @param  __RNGCLKSource__: specifies the USB clock source.
  *         This parameter can be one of the following values:
  *            @arg RCC_RNGCLKSOURCE_HSI48:  HSI48 selected as RNG clock
  *            @arg RCC_RNGCLKSOURCE_PLLCLK: PLL Clock selected as RNG clock
  */
#define __HAL_RCC_RNG_CONFIG(__RNGCLKSource__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_HSI48SEL, (uint32_t)(__RNGCLKSource__))

/** @brief  Macro to get the RNG clock source.
  * @retval The clock source can be one of the following values:
  *            @arg RCC_RNGCLKSOURCE_HSI48:  HSI48 selected as RNG clock
  *            @arg RCC_RNGCLKSOURCE_PLLCLK: PLL Clock selected as RNG clock
  */
#define __HAL_RCC_GET_RNG_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_HSI48SEL)))

/** @brief macro to select the HSI48M clock source 
  * @note   This macro can be replaced by either __HAL_RCC_RNG_CONFIG or
  *         __HAL_RCC_USB_CONFIG to configure respectively RNG or UBS clock sources.
  *
  * @param  __HSI48MCLKSource__: specifies the HSI48M clock source dedicated for 
  *          USB an RNG peripherals.                 
  *          This parameter can be one of the following values:
  *            @arg RCC_HSI48M_PLL: A dedicated 48MHZ PLL output.
  *            @arg RCC_HSI48M_HSI48: 48MHZ issued from internal HSI48 oscillator. 
  */
#define __HAL_RCC_HSI48M_CONFIG(__HSI48MCLKSource__) \
                  MODIFY_REG(RCC->CCIPR, RCC_CCIPR_HSI48SEL, (uint32_t)(__HSI48MCLKSource__))                    

/** @brief  macro to get the HSI48M clock source.
  * @note   This macro can be replaced by either __HAL_RCC_GET_RNG_SOURCE or
  *         __HAL_RCC_GET_USB_SOURCE to get respectively RNG or UBS clock sources.
  * @retval The clock source can be one of the following values:
  *           @arg RCC_HSI48M_PLL: A dedicated 48MHZ PLL output.
  *            @arg RCC_HSI48M_HSI48: 48MHZ issued from internal HSI48 oscillator. 
  */
#define __HAL_RCC_GET_HSI48M_SOURCE() ((uint32_t)(READ_BIT(RCC->CCIPR, RCC_CCIPR_HSI48SEL)))       
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx ) && !(STM32L041xx ) && !(STM32L051xx ) && !(STM32L061xx )  && !(STM32L071xx )  && !(STM32L081xx ) */

/**
  * @brief    Macros to enable or disable the force of the Internal High Speed oscillator (HSI)
  *           in STOP mode to be quickly available as kernel clock for USART and I2C.
  * @note     The Enable of this function has not effect on the HSION bit.
  * @retval None
  */
#define __HAL_RCC_HSISTOP_ENABLE()  SET_BIT(RCC->CR, RCC_CR_HSIKERON)
#define __HAL_RCC_HSISTOP_DISABLE() CLEAR_BIT(RCC->CR, RCC_CR_HSIKERON)                   

/**
  * @brief  Macro to configures the External Low Speed oscillator (LSE) drive capability.
  * @param  __RCC_LSEDrive__: specifies the new state of the LSE drive capability.
  *          This parameter can be one of the following values:
  *            @arg RCC_LSEDRIVE_LOW: LSE oscillator low drive capability.
  *            @arg RCC_LSEDRIVE_MEDIUMLOW: LSE oscillator medium low drive capability.
  *            @arg RCC_LSEDRIVE_MEDIUMHIGH: LSE oscillator medium high drive capability.
  *            @arg RCC_LSEDRIVE_HIGH: LSE oscillator high drive capability.
  * @retval None
  */ 
#define __HAL_RCC_LSEDRIVE_CONFIG(__RCC_LSEDrive__) (MODIFY_REG(RCC->CSR,\
        RCC_CSR_LSEDRV, (uint32_t)(__RCC_LSEDrive__) ))

/**
  * @brief  Macro to configures the wake up from stop clock.
  * @param  __RCC_STOPWUCLK__: specifies the clock source used after wake up from stop
  *   This parameter can be one of the following values:
  *     @arg RCC_STOP_WAKEUPCLOCK_MSI:    MSI selected as system clock source
  *     @arg RCC_STOP_WAKEUPCLOCK_HSI:    HSI selected as system clock source
  * @retval None
  */
#define __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(__RCC_STOPWUCLK__) (MODIFY_REG(RCC->CFGR,\
        RCC_CFGR_STOPWUCK, (uint32_t)(__RCC_STOPWUCLK__) ))
 
#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx)
/**
  * @brief  Enables the specified CRS interrupts.
  * @param  __INTERRUPT__: specifies the CRS interrupt sources to be enabled.
  *          This parameter can be any combination of the following values:
  *              @arg RCC_CRS_IT_SYNCOK
  *              @arg RCC_CRS_IT_SYNCWARN
  *              @arg RCC_CRS_IT_ERR
  *              @arg RCC_CRS_IT_ESYNC
  * @retval None
  */
#define __HAL_RCC_CRS_ENABLE_IT(__INTERRUPT__)   SET_BIT(CRS->CR, (__INTERRUPT__))

/**
  * @brief  Disables the specified CRS interrupts.
  * @param  __INTERRUPT__: specifies the CRS interrupt sources to be disabled.
  *          This parameter can be any combination of the following values:
  *              @arg RCC_CRS_IT_SYNCOK
  *              @arg RCC_CRS_IT_SYNCWARN
  *              @arg RCC_CRS_IT_ERR
  *              @arg RCC_CRS_IT_ESYNC
  * @retval None
  */
#define __HAL_RCC_CRS_DISABLE_IT(__INTERRUPT__)  CLEAR_BIT(CRS->CR,(__INTERRUPT__))

/** @brief  Check the CRS interrupt has occurred or not.
  * @param  __INTERRUPT__: specifies the CRS interrupt source to check.
  *         This parameter can be one of the following values:
  *              @arg RCC_CRS_IT_SYNCOK
  *              @arg RCC_CRS_IT_SYNCWARN
  *              @arg RCC_CRS_IT_ERR
  *              @arg RCC_CRS_IT_ESYNC
  * @retval The new state of __INTERRUPT__ (SET or RESET).
  */
#define __HAL_RCC_CRS_GET_IT_SOURCE(__INTERRUPT__)     ((CRS->CR & (__INTERRUPT__))? SET : RESET)

/** @brief  Clear the CRS interrupt pending bits
  *         bits to clear the selected interrupt pending bits.
  * @param  __INTERRUPT__: specifies the interrupt pending bit to clear.
  *         This parameter can be any combination of the following values:
  *              @arg RCC_CRS_IT_SYNCOK
  *              @arg RCC_CRS_IT_SYNCWARN
  *              @arg RCC_CRS_IT_ERR
  *              @arg RCC_CRS_IT_ESYNC
  *              @arg RCC_CRS_IT_TRIMOVF
  *              @arg RCC_CRS_IT_SYNCERR
  *              @arg RCC_CRS_IT_SYNCMISS
  */
/* CRS IT Error Mask */
#define  RCC_CRS_IT_ERROR_MASK  ((uint32_t)(RCC_CRS_IT_TRIMOVF | RCC_CRS_IT_SYNCERR | RCC_CRS_IT_SYNCMISS))

#define __HAL_RCC_CRS_CLEAR_IT(__INTERRUPT__)   ((((__INTERRUPT__) &  RCC_CRS_IT_ERROR_MASK)!= 0) ? (CRS->ICR |= CRS_ICR_ERRC) : \
                                            (CRS->ICR = (__INTERRUPT__)))

/**
  * @brief  Checks whether the specified CRS flag is set or not.
  * @param  __FLAG__: specifies the flag to check.
  *          This parameter can be one of the following values:
  *              @arg RCC_CRS_FLAG_SYNCOK
  *              @arg RCC_CRS_FLAG_SYNCWARN
  *              @arg RCC_CRS_FLAG_ERR
  *              @arg RCC_CRS_FLAG_ESYNC
  *              @arg RCC_CRS_FLAG_TRIMOVF
  *              @arg RCC_CRS_FLAG_SYNCERR
  *              @arg RCC_CRS_FLAG_SYNCMISS
  * @retval The new state of _FLAG_ (TRUE or FALSE).
  */
#define __HAL_RCC_CRS_GET_FLAG(__FLAG__)  ((CRS->ISR & (__FLAG__)) == (__FLAG__))

/**
  * @brief  Clears the CRS specified FLAG.
  * @param _FLAG_: specifies the flag to clear.
  *          This parameter can be one of the following values:
  *              @arg RCC_CRS_FLAG_SYNCOK
  *              @arg RCC_CRS_FLAG_SYNCWARN
  *              @arg RCC_CRS_FLAG_ERR
  *              @arg RCC_CRS_FLAG_ESYNC
  *              @arg RCC_CRS_FLAG_TRIMOVF
  *              @arg RCC_CRS_FLAG_SYNCERR
  *              @arg RCC_CRS_FLAG_SYNCMISS
  * @retval None
  */

/* CRS Flag Error Mask */
#define RCC_CRS_FLAG_ERROR_MASK      ((uint32_t)(RCC_CRS_FLAG_TRIMOVF | RCC_CRS_FLAG_SYNCERR | RCC_CRS_FLAG_SYNCMISS))

#define __HAL_RCC_CRS_CLEAR_FLAG(__FLAG__)   ((((__FLAG__) & RCC_CRS_FLAG_ERROR_MASK)!= 0) ? (CRS->ICR |= CRS_ICR_ERRC) : \
                                            (CRS->ICR = (__FLAG__)))


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
#define __HAL_RCC_CRS_FREQ_ERROR_COUNTER_DISABLE()  CLEAR_BIT(CRS->CR,CRS_CR_CEN)

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
#define __HAL_RCC_CRS_AUTOMATIC_CALIB_DISABLE()  CLEAR_BIT(CRS->CR,CRS_CR_AUTOTRIMEN)

/**
  * @brief  Macro to calculate reload value to be set in CRS register according to target and sync frequencies
  * @note   The RELOAD value should be selected according to the ratio between the target frequency and the frequency 
  *             of the synchronization source after prescaling. It is then decreased by one in order to 
  *             reach the expected synchronization on the zero value. The formula is the following:
  *             RELOAD = (fTARGET / fSYNC) -1
  * @param  __FTARGET__ Target frequency (value in Hz)
  * @param  __FSYNC__ Synchronization signal frequency (value in Hz)
  * @retval None
  */
#define __HAL_RCC_CRS_RELOADVALUE_CALCULATE(__FTARGET__, __FSYNC__)  (((__FTARGET__) / (__FSYNC__)) - 1)

#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx) && !(STM32L041xx) && !(STM32L051xx) && !(STM32L061xx) && !(STM32L071xx) && !(STM32L081xx) */

#if defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L071xx) || defined(STM32L081xx)
/** @brief  Enable or disable the HSI OUT .
  * @note   After reset, the HSI output is not available
  */

#define __HAL_RCC_HSI_OUT_ENABLE()   SET_BIT(RCC->CR, RCC_CR_HSIOUTEN)
#define __HAL_RCC_HSI_OUT_DISABLE()  CLEAR_BIT(RCC->CR, RCC_CR_HSIOUTEN)

#endif /* STM32L071xx  ||  STM32L081xx  || */
       /* STM32L072xx  ||  STM32L082xx  || */
       /* STM32L073xx  ||  STM32L083xx     */

#if defined(STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx) ||\
    defined(STM32L052xx) || defined(STM32L062xx) || defined(STM32L072xx) || defined(STM32L082xx)

/**
  * @brief  Macro to enable or disable the Internal High Speed oscillator for USB (HSI48).
  * @note   After enabling the HSI48, the application software should wait on 
  *         HSI48RDY flag to be set indicating that HSI48 clock is stable and can
  *         be used to clock the USB.
  * @note   The HSI48 is stopped by hardware when entering STOP and STANDBY modes.
  */
#define __HAL_RCC_HSI48_ENABLE()  do { SET_BIT(RCC->CRRCR, RCC_CRRCR_HSI48ON);   \
                                                    RCC->APB2ENR |=  RCC_APB2ENR_SYSCFGEN; \
                                                    SYSCFG->CFGR3 |= (SYSCFG_CFGR3_ENREF_HSI48);  \
                                                   } while (0)
#define __HAL_RCC_HSI48_DISABLE()  do { CLEAR_BIT(RCC->CRRCR, RCC_CRRCR_HSI48ON);   \
                                                    SYSCFG->CFGR3 &= (uint32_t)~((uint32_t)(SYSCFG_CFGR3_ENREF_HSI48));  \
                                                   } while (0)
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

/** @defgroup RCCEx_Exported_Functions RCCEx Exported Functions
  * @{
  */
  
/** @defgroup RCCEx_Exported_Functions_Group1 Extended Peripheral Control functions 
 
  * @{
  */
HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);
void                  HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);
uint32_t              HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk);
void                  HAL_RCCEx_EnableLSECSS(void);
void                  HAL_RCCEx_DisableLSECSS(void);
void                  HAL_RCCEx_EnableLSECSS_IT(void);
void                  HAL_RCCEx_LSECSS_IRQHandler(void);
void                  HAL_RCCEx_LSECSS_Callback(void);

#if !defined(STM32L011xx) && !defined(STM32L021xx) && !defined(STM32L031xx) && !defined(STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx)
void                  HAL_RCCEx_CRSConfig(RCC_CRSInitTypeDef *pInit);
void                  HAL_RCCEx_CRSSoftwareSynchronizationGenerate(void);
void                  HAL_RCCEx_CRSGetSynchronizationInfo(RCC_CRSSynchroInfoTypeDef *pSynchroInfo);
uint32_t              HAL_RCCEx_CRSWaitSynchronization(uint32_t Timeout);
void HAL_RCCEx_EnableHSI48_VREFINT(void);
void HAL_RCCEx_DisableHSI48_VREFINT(void);
#endif /* !(STM32L011xx) && !(STM32L021xx) && !(STM32L031xx) && !(STM32L041xx) && !(STM32L051xx) && !(STM32L061xx) && !(STM32L071xx) && !(STM32L081xx) */

/**
  * @}
  */ 
/**
  * @}
  */ 


/* Private macros ------------------------------------------------------------*/
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

#define IS_RCC_USART1CLKSOURCE(__SOURCE__)  (((__SOURCE__) == RCC_USART1CLKSOURCE_PCLK2)  || \
                                             ((__SOURCE__) == RCC_USART1CLKSOURCE_SYSCLK) || \
                                             ((__SOURCE__) == RCC_USART1CLKSOURCE_LSE)    || \
                                             ((__SOURCE__) == RCC_USART1CLKSOURCE_HSI))
                                             
                                             
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
                                          
#define IS_RCC_I2C3CLKSOURCE(__SOURCE__)  (((__SOURCE__) == RCC_I2C3CLKSOURCE_PCLK1) || \
                                           ((__SOURCE__) == RCC_I2C3CLKSOURCE_SYSCLK)|| \
                                           ((__SOURCE__) == RCC_I2C3CLKSOURCE_HSI))
                                           
#define IS_RCC_USBCLKSOURCE(__SOURCE__)  (((__SOURCE__) == RCC_USBCLKSOURCE_HSI48) || \
                                          ((__SOURCE__) == RCC_USBCLKSOURCE_PLL))

#define IS_RCC_RNGCLKSOURCE(_SOURCE_)  (((_SOURCE_) == RCC_RNGCLKSOURCE_HSI48) || \
                                      ((_SOURCE_) == RCC_RNGCLKSOURCE_PLLCLK))
                                      
#define IS_RCC_HSI48MCLKSOURCE(__HSI48MCLK__) (((__HSI48MCLK__) == RCC_HSI48M_PLL) || ((__HSI48MCLK__) == RCC_HSI48M_HSI48))
                                          
#if defined(STM32L073xx) || defined(STM32L083xx) || \
    defined(STM32L072xx) || defined(STM32L082xx) || \
    defined(STM32L071xx) || defined(STM32L081xx)

#define IS_RCC_HSI(__HSI__) (((__HSI__) == RCC_HSI_OFF) || ((__HSI__) == RCC_HSI_ON) || \
                             ((__HSI__) == RCC_HSI_DIV4) || ((__HSI__) == RCC_HSI_OUTEN ))      
#else
#define IS_RCC_HSI(__HSI__) (((__HSI__) == RCC_HSI_OFF) || ((__HSI__) == RCC_HSI_ON) || \
                             ((__HSI__) == RCC_HSI_DIV4))
#endif

#define IS_RCC_LPTIMCLK(__LPTIMCLK_)     (((__LPTIMCLK_) == RCC_LPTIM1CLKSOURCE_PCLK) || \
                                          ((__LPTIMCLK_) == RCC_LPTIM1CLKSOURCE_LSI)  || \
                                          ((__LPTIMCLK_) == RCC_LPTIM1CLKSOURCE_HSI)  || \
                                          ((__LPTIMCLK_) == RCC_LPTIM1CLKSOURCE_LSE))
                                          
#define IS_RCC_STOPWAKEUP_CLOCK(__SOURCE__) (((__SOURCE__) == RCC_StopWakeUpClock_MSI) || \
                                             ((__SOURCE__) == RCC_StopWakeUpClock_HSI))

#define IS_RCC_LSE_DRIVE(__DRIVE__) (((__DRIVE__) == RCC_LSEDRIVE_LOW)        || ((__SOURCE__) == RCC_LSEDRIVE_MEDIUMLOW) || \
                                     ((__DRIVE__) == RCC_LSEDRIVE_MEDIUMHIGH) || ((__SOURCE__) == RCC_LSEDRIVE_HIGH))
                                     
#define IS_RCC_CRS_SYNC_SOURCE(__SOURCE__) (((__SOURCE__) == RCC_CRS_SYNC_SOURCE_GPIO) || \
                                            ((__SOURCE__) == RCC_CRS_SYNC_SOURCE_LSE) ||\
                                            ((__SOURCE__) == RCC_CRS_SYNC_SOURCE_USB))

#define IS_RCC_CRS_SYNC_DIV(__DIV__) (((__DIV__) == RCC_CRS_SYNC_DIV1) || ((__DIV__) == RCC_CRS_SYNC_DIV2)   ||\
                                      ((__DIV__) == RCC_CRS_SYNC_DIV4) || ((__DIV__) == RCC_CRS_SYNC_DIV8)   || \
                                      ((__DIV__) == RCC_CRS_SYNC_DIV16) || ((__DIV__) == RCC_CRS_SYNC_DIV32) || \
                                      ((__DIV__) == RCC_CRS_SYNC_DIV64) || ((__DIV__) == RCC_CRS_SYNC_DIV128))
                                      
#define IS_RCC_CRS_SYNC_POLARITY(__POLARITY__) (((__POLARITY__) == RCC_CRS_SYNC_POLARITY_RISING) || \
                                                ((__POLARITY__) == RCC_CRS_SYNC_POLARITY_FALLING))
                                                
#define IS_RCC_CRS_RELOADVALUE(__VALUE__) (((__VALUE__) <= 0xFFFFU))

#define IS_RCC_CRS_ERRORLIMIT(__VALUE__) (((__VALUE__) <= 0xFFU))

#define IS_RCC_CRS_HSI48CALIBRATION(__VALUE__) (((__VALUE__) <= 0x3FU))

#define IS_RCC_CRS_FREQERRORDIR(__DIR__) (((__DIR__) == RCC_CRS_FREQERRORDIR_UP) || \
                                          ((__DIR__) == RCC_CRS_FREQERRORDIR_DOWN))
                                          
/**
  * @}
  */
                                          
                                            
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

