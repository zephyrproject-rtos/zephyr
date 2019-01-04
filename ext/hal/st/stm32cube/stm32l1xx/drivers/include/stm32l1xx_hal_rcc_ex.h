/**
  ******************************************************************************
  * @file    stm32l1xx_hal_rcc_ex.h
  * @author  MCD Application Team
  * @brief   Header file of RCC HAL Extension module.
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
#ifndef __STM32L1xx_HAL_RCC_EX_H
#define __STM32L1xx_HAL_RCC_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal_def.h"

/** @addtogroup STM32L1xx_HAL_Driver
  * @{
  */

/** @addtogroup RCCEx
  * @{
  */ 

/** @addtogroup RCCEx_Private_Constants
 * @{
 */

#define LSI_VALUE                  (37000U)  /* ~37kHz */

#if defined(STM32L100xBA) || defined(STM32L151xBA) || defined(STM32L152xBA)\
 || defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX)\
 || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

/* Alias word address of LSECSSON bit */
#define LSECSSON_BITNUMBER      POSITION_VAL(RCC_CSR_LSECSSON)
#define CSR_LSECSSON_BB         ((uint32_t)(PERIPH_BB_BASE + (RCC_CSR_OFFSET_BB * 32U) + (LSECSSON_BITNUMBER * 4U)))

#endif /* STM32L100xBA || STM32L151xBA || STM32L152xBA || STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX*/

/**
  * @}
  */

/** @addtogroup RCCEx_Private_Macros
  * @{
  */
#if defined(LCD)

#define IS_RCC_PERIPHCLOCK(__CLK__) ((RCC_PERIPHCLK_RTC <= (__CLK__)) && ((__CLK__) <= RCC_PERIPHCLK_LCD))

#else /* Not LCD LINE */

#define IS_RCC_PERIPHCLOCK(__CLK__) ((__CLK__) == RCC_PERIPHCLK_RTC)

#endif /* LCD */

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
} RCC_PeriphCLKInitTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup RCCEx_Exported_Constants RCCEx Exported Constants
  * @{
  */

/** @defgroup RCCEx_Periph_Clock_Selection RCCEx Periph Clock Selection
  * @{
  */
#define RCC_PERIPHCLK_RTC           (0x00000001U)

#if defined(LCD)

#define RCC_PERIPHCLK_LCD           (0x00000002U)

#endif /* LCD */

/**
  * @}
  */

#if defined(RCC_LSECSS_SUPPORT)
/** @defgroup RCCEx_EXTI_LINE_LSECSS  RCC LSE CSS external interrupt line
  * @{
  */
#define RCC_EXTI_LINE_LSECSS             (EXTI_IMR_IM19)         /*!< External interrupt line 19 connected to the LSE CSS EXTI Line */
/**
  * @}
  */
#endif /* RCC_LSECSS_SUPPORT */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Macros RCCEx Exported Macros
 * @{
 */

/** @defgroup RCCEx_Peripheral_Clock_Enable_Disable RCCEx_Peripheral_Clock_Enable_Disable
  * @brief  Enables or disables the AHB1 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before 
  *         using it.   
  * @{
  */
#if defined(STM32L151xB) || defined(STM32L152xB) || defined(STM32L151xBA)\
 || defined(STM32L152xBA) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_GPIOE_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOEEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_GPIOEEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_GPIOE_CLK_DISABLE()   (RCC->AHBENR &= ~(RCC_AHBENR_GPIOEEN))

#endif /* STM32L151xB || STM32L152xB || ... || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_GPIOF_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOFEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_GPIOFEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_GPIOG_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOGEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_GPIOGEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_GPIOF_CLK_DISABLE()   (RCC->AHBENR &= ~(RCC_AHBENR_GPIOFEN))
#define __HAL_RCC_GPIOG_CLK_DISABLE()   (RCC->AHBENR &= ~(RCC_AHBENR_GPIOGEN))

#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_DMA2_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_DMA2EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_DMA2EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_DMA2_CLK_DISABLE()    (RCC->AHBENR &= ~(RCC_AHBENR_DMA2EN))

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L162xC) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_CRYP_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_AESEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_AESEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_CRYP_CLK_DISABLE()    (RCC->AHBENR &= ~(RCC_AHBENR_AESEN))

#endif /* STM32L162xC || STM32L162xCA || STM32L162xD || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)
  
#define __HAL_RCC_FSMC_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->AHBENR, RCC_AHBENR_FSMCEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_FSMCEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_FSMC_CLK_DISABLE()    (RCC->AHBENR &= ~(RCC_AHBENR_FSMCEN))

#endif /* STM32L151xD || STM32L152xD || STM32L162xD */

#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_LCD_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_LCDEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_LCDEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_LCD_CLK_DISABLE()       (RCC->APB1ENR &= ~(RCC_APB1ENR_LCDEN))

#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

/** @brief  Enables or disables the Low Speed APB (APB1) peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before 
  *         using it. 
  */
#if defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)\
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_TIM5_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM5EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM5EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_TIM5_CLK_DISABLE()    (RCC->APB1ENR &= ~(RCC_APB1ENR_TIM5EN))

#endif /* STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_SPI3_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI3EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI3EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_SPI3_CLK_DISABLE()    (RCC->APB1ENR &= ~(RCC_APB1ENR_SPI3EN))

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_UART4_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_UART4EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_UART4EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_UART5_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB1ENR, RCC_APB1ENR_UART5EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB1ENR, RCC_APB1ENR_UART5EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_UART4_CLK_DISABLE()   (RCC->APB1ENR &= ~(RCC_APB1ENR_UART4EN))
#define __HAL_RCC_UART5_CLK_DISABLE()   (RCC->APB1ENR &= ~(RCC_APB1ENR_UART5EN))

#endif /* STM32L151xD || STM32L152xD || STM32L162xD || (...) || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE)\
 || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)\
 || defined(STM32L162xC) || defined(STM32L152xC) || defined(STM32L151xC)

#define __HAL_RCC_OPAMP_CLK_ENABLE()      __HAL_RCC_COMP_CLK_ENABLE()   /* Peripherals COMP and OPAMP share the same clock domain */
#define __HAL_RCC_OPAMP_CLK_DISABLE()     __HAL_RCC_COMP_CLK_DISABLE()  /* Peripherals COMP and OPAMP share the same clock domain */
      
#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || (...) || STM32L162xC || STM32L152xC || STM32L151xC */
      
/** @brief  Enables or disables the High Speed APB (APB2) peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before 
  *         using it.
  */
#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)

#define __HAL_RCC_SDIO_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SDIOEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_SDIOEN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)
#define __HAL_RCC_SDIO_CLK_DISABLE()    (RCC->APB2ENR &= ~(RCC_APB2ENR_SDIOEN))

#endif /* STM32L151xD || STM32L152xD || STM32L162xD */

/**
    * @}
    */
  

/** @defgroup RCCEx_Force_Release_Peripheral_Reset RCCEx Force Release Peripheral Reset
  * @brief  Forces or releases AHB peripheral reset.
  * @{
  */  
#if defined(STM32L151xB) || defined(STM32L152xB) || defined(STM32L151xBA)\
 || defined(STM32L152xBA) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_GPIOE_FORCE_RESET()   (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOERST))
#define __HAL_RCC_GPIOE_RELEASE_RESET() (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOERST))

#endif /* STM32L151xB || STM32L152xB || ... || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_GPIOF_FORCE_RESET()   (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOFRST))
#define __HAL_RCC_GPIOG_FORCE_RESET()   (RCC->AHBRSTR |= (RCC_AHBRSTR_GPIOGRST))

#define __HAL_RCC_GPIOF_RELEASE_RESET() (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOFRST))
#define __HAL_RCC_GPIOG_RELEASE_RESET() (RCC->AHBRSTR &= ~(RCC_AHBRSTR_GPIOGRST))

#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_DMA2_FORCE_RESET()    (RCC->AHBRSTR |= (RCC_AHBRSTR_DMA2RST))
#define __HAL_RCC_DMA2_RELEASE_RESET()  (RCC->AHBRSTR &= ~(RCC_AHBRSTR_DMA2RST))

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L162xC) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_CRYP_FORCE_RESET()     (RCC->AHBRSTR |= (RCC_AHBRSTR_AESRST))
#define __HAL_RCC_CRYP_RELEASE_RESET()   (RCC->AHBRSTR &= ~(RCC_AHBRSTR_AESRST))

#endif /* STM32L162xC || STM32L162xCA || STM32L162xD || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)
  
#define __HAL_RCC_FSMC_FORCE_RESET()    (RCC->AHBRSTR |= (RCC_AHBRSTR_FSMCRST))
#define __HAL_RCC_FSMC_RELEASE_RESET()  (RCC->AHBRSTR &= ~(RCC_AHBRSTR_FSMCRST))

#endif /* STM32L151xD || STM32L152xD || STM32L162xD */

#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_LCD_FORCE_RESET()     (RCC->APB1RSTR |= (RCC_APB1RSTR_LCDRST))
#define __HAL_RCC_LCD_RELEASE_RESET()   (RCC->APB1RSTR &= ~(RCC_APB1RSTR_LCDRST))

#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

/** @brief  Forces or releases APB1 peripheral reset.
  */
#if defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)\
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_TIM5_FORCE_RESET()    (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM5RST))
#define __HAL_RCC_TIM5_RELEASE_RESET()  (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM5RST))

#endif /* STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_SPI3_FORCE_RESET()    (RCC->APB1RSTR |= (RCC_APB1RSTR_SPI3RST))
#define __HAL_RCC_SPI3_RELEASE_RESET()  (RCC->APB1RSTR &= ~(RCC_APB1RSTR_SPI3RST))

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_UART4_FORCE_RESET()   (RCC->APB1RSTR |= (RCC_APB1RSTR_UART4RST))
#define __HAL_RCC_UART5_FORCE_RESET()   (RCC->APB1RSTR |= (RCC_APB1RSTR_UART5RST))

#define __HAL_RCC_UART4_RELEASE_RESET() (RCC->APB1RSTR &= ~(RCC_APB1RSTR_UART4RST))
#define __HAL_RCC_UART5_RELEASE_RESET() (RCC->APB1RSTR &= ~(RCC_APB1RSTR_UART5RST))

#endif /* STM32L151xD || STM32L152xD || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)\
 || defined(STM32L162xC) || defined(STM32L152xC) || defined(STM32L151xC)

#define __HAL_RCC_OPAMP_FORCE_RESET()     __HAL_RCC_COMP_FORCE_RESET()   /* Peripherals COMP and OPAMP share the same clock domain */
#define __HAL_RCC_OPAMP_RELEASE_RESET()   __HAL_RCC_COMP_RELEASE_RESET() /* Peripherals COMP and OPAMP share the same clock domain */
      
#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xC || STM32L152xC || STM32L151xC */
      
/** @brief  Forces or releases APB2 peripheral reset.
  */
#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)

#define __HAL_RCC_SDIO_FORCE_RESET()    (RCC->APB2RSTR |= (RCC_APB2RSTR_SDIORST))
#define __HAL_RCC_SDIO_RELEASE_RESET()  (RCC->APB2RSTR &= ~(RCC_APB2RSTR_SDIORST))

#endif /* STM32L151xD || STM32L152xD || STM32L162xD */

/**
  * @}
  */

/** @defgroup RCCEx_Peripheral_Clock_Sleep_Enable_Disable RCCEx Peripheral Clock Sleep Enable Disable
  * @brief  Enables or disables the AHB1 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */
#if defined(STM32L151xB) || defined(STM32L152xB) || defined(STM32L151xBA)\
 || defined(STM32L152xBA) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_GPIOE_CLK_SLEEP_ENABLE()  (RCC->AHBLPENR |= (RCC_AHBLPENR_GPIOELPEN))
#define __HAL_RCC_GPIOE_CLK_SLEEP_DISABLE() (RCC->AHBLPENR &= ~(RCC_AHBLPENR_GPIOELPEN))

#endif /* STM32L151xB || STM32L152xB || ... || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_GPIOF_CLK_SLEEP_ENABLE()  (RCC->AHBLPENR |= (RCC_AHBLPENR_GPIOFLPEN))
#define __HAL_RCC_GPIOG_CLK_SLEEP_ENABLE()  (RCC->AHBLPENR |= (RCC_AHBLPENR_GPIOGLPEN))

#define __HAL_RCC_GPIOF_CLK_SLEEP_DISABLE() (RCC->AHBLPENR &= ~(RCC_AHBLPENR_GPIOFLPEN))
#define __HAL_RCC_GPIOG_CLK_SLEEP_DISABLE() (RCC->AHBLPENR &= ~(RCC_AHBLPENR_GPIOGLPEN))

#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_DMA2_CLK_SLEEP_ENABLE()   (RCC->AHBLPENR |= (RCC_AHBLPENR_DMA2LPEN))
#define __HAL_RCC_DMA2_CLK_SLEEP_DISABLE()  (RCC->AHBLPENR &= ~(RCC_AHBLPENR_DMA2LPEN))

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L162xC) || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_CRYP_CLK_SLEEP_ENABLE()    (RCC->AHBLPENR |= (RCC_AHBLPENR_AESLPEN))
#define __HAL_RCC_CRYP_CLK_SLEEP_DISABLE()   (RCC->AHBLPENR &= ~(RCC_AHBLPENR_AESLPEN))

#endif /* STM32L162xC || STM32L162xCA || STM32L162xD || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)
  
#define __HAL_RCC_FSMC_CLK_SLEEP_ENABLE()   (RCC->AHBLPENR |= (RCC_AHBLPENR_FSMCLPEN))
#define __HAL_RCC_FSMC_CLK_SLEEP_DISABLE()  (RCC->AHBLPENR &= ~(RCC_AHBLPENR_FSMCLPEN))

#endif /* STM32L151xD || STM32L152xD || STM32L162xD */

#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_LCD_CLK_SLEEP_ENABLE()    (RCC->APB1LPENR |= (RCC_APB1LPENR_LCDLPEN))
#define __HAL_RCC_LCD_CLK_SLEEP_DISABLE()   (RCC->APB1LPENR &= ~(RCC_APB1LPENR_LCDLPEN))

#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

/** @brief  Enables or disables the APB1 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *           power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  */
#if defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)\
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_TIM5_CLK_SLEEP_ENABLE()   (RCC->APB1LPENR |= (RCC_APB1LPENR_TIM5LPEN))
#define __HAL_RCC_TIM5_CLK_SLEEP_DISABLE()  (RCC->APB1LPENR &= ~(RCC_APB1LPENR_TIM5LPEN))

#endif /* STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_SPI3_CLK_SLEEP_ENABLE()   (RCC->APB1LPENR |= (RCC_APB1LPENR_SPI3LPEN))
#define __HAL_RCC_SPI3_CLK_SLEEP_DISABLE()  (RCC->APB1LPENR &= ~(RCC_APB1LPENR_SPI3LPEN))

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_UART4_CLK_SLEEP_ENABLE()  (RCC->APB1LPENR |= (RCC_APB1LPENR_UART4LPEN))
#define __HAL_RCC_UART5_CLK_SLEEP_ENABLE()  (RCC->APB1LPENR |= (RCC_APB1LPENR_UART5LPEN))

#define __HAL_RCC_UART4_CLK_SLEEP_DISABLE() (RCC->APB1LPENR &= ~(RCC_APB1LPENR_UART4LPEN))
#define __HAL_RCC_UART5_CLK_SLEEP_DISABLE() (RCC->APB1LPENR &= ~(RCC_APB1LPENR_UART5LPEN))

#endif /* STM32L151xD || STM32L152xD || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)\
 || defined(STM32L162xC) || defined(STM32L152xC) || defined(STM32L151xC)

#define __HAL_RCC_OPAMP_CLK_SLEEP_ENABLE()      __HAL_RCC_COMP_CLK_SLEEP_ENABLE()   /* Peripherals COMP and OPAMP share the same clock domain */
#define __HAL_RCC_OPAMP_CLK_SLEEP_DISABLE()     __HAL_RCC_COMP_CLK_SLEEP_DISABLE()  /* Peripherals COMP and OPAMP share the same clock domain */
      
#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xC || STM32L152xC || STM32L151xC */

/** @brief  Enables or disables the APB2 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *           power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  */
#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)

#define __HAL_RCC_SDIO_CLK_SLEEP_ENABLE()   (RCC->APB2LPENR |= (RCC_APB2LPENR_SDIOLPEN))
#define __HAL_RCC_SDIO_CLK_SLEEP_DISABLE()  (RCC->APB2LPENR &= ~(RCC_APB2LPENR_SDIOLPEN))

#endif /* STM32L151xD || STM32L152xD || STM32L162xD */

/**
  * @}
  */

/** @defgroup RCCEx_Peripheral_Clock_Enable_Disable_Status Peripheral Clock Enable Disable Status
  * @brief  Get the enable or disable status of peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */

#if defined(STM32L151xB) || defined(STM32L152xB) || defined(STM32L151xBA)\
 || defined(STM32L152xBA) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_GPIOE_IS_CLK_ENABLED()       ((RCC->AHBENR & (RCC_AHBENR_GPIOEEN)) != RESET)
#define __HAL_RCC_GPIOE_IS_CLK_DISABLED()      ((RCC->AHBENR & (RCC_AHBENR_GPIOEEN)) == RESET)

#endif /* STM32L151xB || STM32L152xB || ... || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_GPIOF_IS_CLK_ENABLED()       ((RCC->AHBENR & (RCC_AHBENR_GPIOFEN)) != RESET)
#define __HAL_RCC_GPIOG_IS_CLK_ENABLED()       ((RCC->AHBENR & (RCC_AHBENR_GPIOGEN)) != RESET)
#define __HAL_RCC_GPIOF_IS_CLK_DISABLED()      ((RCC->AHBENR & (RCC_AHBENR_GPIOFEN)) == RESET)
#define __HAL_RCC_GPIOG_IS_CLK_DISABLED()      ((RCC->AHBENR & (RCC_AHBENR_GPIOGEN)) == RESET)

#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_DMA2_IS_CLK_ENABLED()        ((RCC->AHBENR & (RCC_AHBENR_DMA2EN)) != RESET)
#define __HAL_RCC_DMA2_IS_CLK_DISABLED()       ((RCC->AHBENR & (RCC_AHBENR_DMA2EN)) == RESET)

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L162xC) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_CRYP_IS_CLK_ENABLED()        ((RCC->AHBENR & (RCC_AHBENR_AESEN)) != RESET)
#define __HAL_RCC_CRYP_IS_CLK_DISABLED()       ((RCC->AHBENR & (RCC_AHBENR_AESEN)) == RESET)

#endif /* STM32L162xC || STM32L162xCA || STM32L162xD || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)

#define __HAL_RCC_FSMC_IS_CLK_ENABLED()        ((RCC->AHBENR & (RCC_AHBENR_FSMCEN)) != RESET)
#define __HAL_RCC_FSMC_IS_CLK_DISABLED()       ((RCC->AHBENR & (RCC_AHBENR_FSMCEN)) == RESET)

#endif /* STM32L151xD || STM32L152xD || STM32L162xD */

#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_LCD_IS_CLK_ENABLED()         ((RCC->APB1ENR & (RCC_APB1ENR_LCDEN)) != RESET)
#define __HAL_RCC_LCD_IS_CLK_DISABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_LCDEN)) == RESET)

#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)\
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_TIM5_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_TIM5EN)) != RESET)
#define __HAL_RCC_TIM5_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_TIM5EN)) == RESET)

#endif /* STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_SPI3_IS_CLK_ENABLED()        ((RCC->APB1ENR & (RCC_APB1ENR_SPI3EN)) != RESET)
#define __HAL_RCC_SPI3_IS_CLK_DISABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_SPI3EN)) == RESET)

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_UART4_IS_CLK_ENABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_UART4EN)) != RESET)
#define __HAL_RCC_UART5_IS_CLK_ENABLED()       ((RCC->APB1ENR & (RCC_APB1ENR_UART5EN)) != RESET)
#define __HAL_RCC_UART4_IS_CLK_DISABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_UART4EN)) == RESET)
#define __HAL_RCC_UART5_IS_CLK_DISABLED()      ((RCC->APB1ENR & (RCC_APB1ENR_UART5EN)) == RESET)

#endif /* STM32L151xD || STM32L152xD || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)\
 || defined(STM32L162xC) || defined(STM32L152xC) || defined(STM32L151xC)

#define __HAL_RCC_OPAMP_IS_CLK_ENABLED()       __HAL_RCC_COMP_IS_CLK_ENABLED()
#define __HAL_RCC_OPAMP_IS_CLK_DISABLED()      __HAL_RCC_COMP_IS_CLK_DISABLED()

#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xC || STM32L152xC || STM32L151xC */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)

#define __HAL_RCC_SDIO_IS_CLK_ENABLED()        ((RCC->APB2ENR & (RCC_APB2ENR_SDIOEN)) != RESET)
#define __HAL_RCC_SDIO_IS_CLK_DISABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_SDIOEN)) == RESET)

#endif /* STM32L151xD || STM32L152xD || STM32L162xD */

/**
  * @}
  */

/** @defgroup RCCEx_Peripheral_Clock_Sleep_Enable_Disable_Status Peripheral Clock Sleep Enable Disable Status
  * @brief  Get the enable or disable status of peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */

#if defined(STM32L151xB) || defined(STM32L152xB) || defined(STM32L151xBA)\
 || defined(STM32L152xBA) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_GPIOE_IS_CLK_SLEEP_ENABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOELPEN)) != RESET)
#define __HAL_RCC_GPIOE_IS_CLK_SLEEP_DISABLED()      ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOELPEN)) == RESET)

#endif /* STM32L151xB || STM32L152xB || ... || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_GPIOF_IS_CLK_SLEEP_ENABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOFLPEN)) != RESET)
#define __HAL_RCC_GPIOG_IS_CLK_SLEEP_ENABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOGLPEN)) != RESET)
#define __HAL_RCC_GPIOF_IS_CLK_SLEEP_DISABLED()      ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOFLPEN)) == RESET)
#define __HAL_RCC_GPIOG_IS_CLK_SLEEP_DISABLED()      ((RCC->AHBLPENR & (RCC_AHBLPENR_GPIOGLPEN)) == RESET)

#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_DMA2_IS_CLK_SLEEP_ENABLED()        ((RCC->AHBLPENR & (RCC_AHBLPENR_DMA2LPEN)) != RESET)
#define __HAL_RCC_DMA2_IS_CLK_SLEEP_DISABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_DMA2LPEN)) == RESET)

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L162xC) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_CRYP_IS_CLK_SLEEP_ENABLED()        ((RCC->AHBLPENR & (RCC_AHBLPENR_AESLPEN)) != RESET)
#define __HAL_RCC_CRYP_IS_CLK_SLEEP_DISABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_AESLPEN)) == RESET)

#endif /* STM32L162xC || STM32L162xCA || STM32L162xD || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)

#define __HAL_RCC_FSMC_IS_CLK_SLEEP_ENABLED()        ((RCC->AHBLPENR & (RCC_AHBLPENR_FSMCLPEN)) != RESET)
#define __HAL_RCC_FSMC_IS_CLK_SLEEP_DISABLED()       ((RCC->AHBLPENR & (RCC_AHBLPENR_FSMCLPEN)) == RESET)

#endif /* STM32L151xD || STM32L152xD || STM32L162xD */

#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    
#define __HAL_RCC_LCD_IS_CLK_SLEEP_ENABLED()         ((RCC->APB1LPENR & (RCC_APB1LPENR_LCDLPEN)) != RESET)
#define __HAL_RCC_LCD_IS_CLK_SLEEP_DISABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_LCDLPEN)) == RESET)

#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)\
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_TIM5_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM5LPEN)) != RESET)
#define __HAL_RCC_TIM5_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_TIM5LPEN)) == RESET)

#endif /* STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD)\
 || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA)\
 || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_SPI3_IS_CLK_SLEEP_ENABLED()        ((RCC->APB1LPENR & (RCC_APB1LPENR_SPI3LPEN)) != RESET)
#define __HAL_RCC_SPI3_IS_CLK_SLEEP_DISABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_SPI3LPEN)) == RESET)

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)

#define __HAL_RCC_UART4_IS_CLK_SLEEP_ENABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_UART4LPEN)) != RESET)
#define __HAL_RCC_UART5_IS_CLK_SLEEP_ENABLED()       ((RCC->APB1LPENR & (RCC_APB1LPENR_UART5LPEN)) != RESET)
#define __HAL_RCC_UART4_IS_CLK_SLEEP_DISABLED()      ((RCC->APB1LPENR & (RCC_APB1LPENR_UART4LPEN)) == RESET)
#define __HAL_RCC_UART5_IS_CLK_SLEEP_DISABLED()      ((RCC->APB1LPENR & (RCC_APB1LPENR_UART5LPEN)) == RESET)

#endif /* STM32L151xD || STM32L152xD || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA)\
 || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD)\
 || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX)\
 || defined(STM32L162xC) || defined(STM32L152xC) || defined(STM32L151xC)

#define __HAL_RCC_OPAMP_IS_CLK_SLEEP_ENABLED()       __HAL_RCC_COMP_IS_CLK_SLEEP_ENABLED()
#define __HAL_RCC_OPAMP_IS_CLK_SLEEP_DISABLED()      __HAL_RCC_COMP_IS_CLK_SLEEP_DISABLED()

#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xC || STM32L152xC || STM32L151xC */

#if defined(STM32L151xD) || defined(STM32L152xD) || defined(STM32L162xD)

#define __HAL_RCC_SDIO_IS_CLK_SLEEP_ENABLED()        ((RCC->APB2LPENR & (RCC_APB2LPENR_SDIOLPEN)) != RESET)
#define __HAL_RCC_SDIO_IS_CLK_SLEEP_DISABLED()       ((RCC->APB2LPENR & (RCC_APB2LPENR_SDIOLPEN)) == RESET)

#endif /* STM32L151xD || STM32L152xD || STM32L162xD */

/**
  * @}
  */
  
  
#if defined(RCC_LSECSS_SUPPORT)

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
  } while(0U)  
  
/**
  * @brief Disable the RCC LSE CSS Extended Interrupt Rising & Falling Trigger.
  * @retval None.
  */
#define __HAL_RCC_LSECSS_EXTI_DISABLE_RISING_FALLING_EDGE()  \
  do {                                                       \
    __HAL_RCC_LSECSS_EXTI_DISABLE_RISING_EDGE();             \
    __HAL_RCC_LSECSS_EXTI_DISABLE_FALLING_EDGE();            \
  } while(0U)  

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

#endif /* RCC_LSECSS_SUPPORT */

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

#if defined(RCC_LSECSS_SUPPORT)

void              HAL_RCCEx_EnableLSECSS(void);
void              HAL_RCCEx_DisableLSECSS(void);
void              HAL_RCCEx_EnableLSECSS_IT(void);
void              HAL_RCCEx_LSECSS_IRQHandler(void);
void              HAL_RCCEx_LSECSS_Callback(void);

#endif /* RCC_LSECSS_SUPPORT */

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

#endif /* __STM32L1xx_HAL_RCC_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

