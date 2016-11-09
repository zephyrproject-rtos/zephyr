/**
  ******************************************************************************
  * @file    stm32f2xx_hal_rcc_ex.h
  * @author  MCD Application Team
  * @version V1.1.3
  * @date    29-June-2016
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
#ifndef __STM32F2xx_HAL_RCC_EX_H
#define __STM32F2xx_HAL_RCC_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx_hal_def.h"

/** @addtogroup STM32F2xx_HAL_Driver
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
  * @brief  PLLI2S Clock structure definition  
  */
typedef struct
{
  uint32_t PLLI2SN;    /*!< Specifies the multiplication factor for PLLI2S VCO output clock.
                            This parameter must be a number between Min_Data = 192 and Max_Data = 432.
                            This parameter will be used only when PLLI2S is selected as Clock Source I2S */

  uint32_t PLLI2SR;    /*!< Specifies the division factor for I2S clock.
                            This parameter must be a number between Min_Data = 2 and Max_Data = 7. 
                            This parameter will be used only when PLLI2S is selected as Clock Source I2S */

}RCC_PLLI2SInitTypeDef;

/** 
  * @brief  RCC extended clocks structure definition  
  */
typedef struct
{
  uint32_t PeriphClockSelection; /*!< The Extended Clock to be configured.
                                      This parameter can be a value of @ref RCCEx_Periph_Clock_Selection */

  RCC_PLLI2SInitTypeDef PLLI2S;  /*!< PLL I2S structure parameters. 
                                      This parameter will be used only when PLLI2S is selected as Clock Source I2S */

  uint32_t RTCClockSelection;      /*!< Specifies RTC Clock Prescalers Selection. 
                                      This parameter can be a value of @ref RCC_RTC_Clock_Source */

  uint8_t TIMPresSelection;      /*!< Specifies TIM Clock Prescalers Selection. 
                                      This parameter can be a value of @ref RCCEx_TIM_PRescaler_Selection */

}RCC_PeriphCLKInitTypeDef;
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Constants RCCEx Exported Constants
  * @{
  */

/** @defgroup RCCEx_Periph_Clock_Selection RCC Periph Clock Selection
  * @{
  */
#define RCC_PERIPHCLK_I2S             ((uint32_t)0x00000001)
#define RCC_PERIPHCLK_TIM             ((uint32_t)0x00000002)
#define RCC_PERIPHCLK_RTC             ((uint32_t)0x00000004)
#define RCC_PERIPHCLK_PLLI2S          ((uint32_t)0x00000008)

/**
  * @}
  */

/** @defgroup RCCEx_TIM_PRescaler_Selection  RCC TIM PRescaler Selection
  * @{
  */
#define RCC_TIMPRES_DESACTIVATED        ((uint8_t)0x00)
#define RCC_TIMPRES_ACTIVATED           ((uint8_t)0x01)
/**
  * @}
  */

/**
  * @}
  */
     
/* Exported macro ------------------------------------------------------------*/
/** @defgroup RCCEx_Exported_Macros RCC Exported Macros
  * @{
  */

/** @defgroup RCCEx_AHB1_Clock_Enable_Disable AHB1 Peripheral Clock Enable Disable
  * @brief  Enables or disables the AHB1 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before 
  *         using it.
  * @{
  */
#if defined(STM32F207xx) || defined(STM32F217xx)
#define __HAL_RCC_ETHMAC_CLK_ENABLE()   do { \
                                       __IO uint32_t tmpreg = 0x00; \
                                       SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_ETHMACEN);\
                                       /* Delay after an RCC peripheral clock enabling */ \
                                       tmpreg = READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_ETHMACEN);\
                                       UNUSED(tmpreg); \
                                       } while(0)
#define __HAL_RCC_ETHMACTX_CLK_ENABLE()   do { \
                                       __IO uint32_t tmpreg = 0x00; \
                                       SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_ETHMACTXEN);\
                                       /* Delay after an RCC peripheral clock enabling */ \
                                       tmpreg = READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_ETHMACTXEN);\
                                       UNUSED(tmpreg); \
                                       } while(0)
#define __HAL_RCC_ETHMACRX_CLK_ENABLE()   do { \
                                       __IO uint32_t tmpreg = 0x00; \
                                       SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_ETHMACRXEN);\
                                       /* Delay after an RCC peripheral clock enabling */ \
                                       tmpreg = READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_ETHMACRXEN);\
                                       UNUSED(tmpreg); \
                                       } while(0)
#define __HAL_RCC_ETHMACPTP_CLK_ENABLE()   do { \
                                       __IO uint32_t tmpreg = 0x00; \
                                       SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_ETHMACPTPEN);\
                                       /* Delay after an RCC peripheral clock enabling */ \
                                       tmpreg = READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_ETHMACPTPEN);\
                                       UNUSED(tmpreg); \
                                       } while(0)

#define __HAL_RCC_ETHMAC_CLK_DISABLE()    (RCC->AHB1ENR &= ~(RCC_AHB1ENR_ETHMACEN))
#define __HAL_RCC_ETHMACTX_CLK_DISABLE()  (RCC->AHB1ENR &= ~(RCC_AHB1ENR_ETHMACTXEN))
#define __HAL_RCC_ETHMACRX_CLK_DISABLE()  (RCC->AHB1ENR &= ~(RCC_AHB1ENR_ETHMACRXEN))
#define __HAL_RCC_ETHMACPTP_CLK_DISABLE() (RCC->AHB1ENR &= ~(RCC_AHB1ENR_ETHMACPTPEN))

/** @defgroup RCC_AHB1_Peripheral_Clock_Enable_Disable_Status AHB1 Peripheral Clock Enable Disable Status
  * @brief  Get the enable or disable status of the AHB1 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
#define __HAL_RCC_ETHMAC_IS_CLK_ENABLED()    ((RCC->AHB1ENR &(RCC_AHB1ENR_ETHMACEN))!= RESET)
#define __HAL_RCC_ETHMACTX_IS_CLK_ENABLED()  ((RCC->AHB1ENR &(RCC_AHB1ENR_ETHMACTXEN))!= RESET)
#define __HAL_RCC_ETHMACRX_IS_CLK_ENABLED()  ((RCC->AHB1ENR &(RCC_AHB1ENR_ETHMACRXEN))!= RESET)
#define __HAL_RCC_ETHMACPTP_IS_CLK_ENABLED() ((RCC->AHB1ENR &(RCC_AHB1ENR_ETHMACPTPEN))!= RESET) 
#define __HAL_RCC_ETH_IS_CLK_ENABLED()        (__HAL_RCC_ETHMAC_IS_CLK_ENABLED()   && \
                                               __HAL_RCC_ETHMACTX_IS_CLK_ENABLED() && \
	                                       __HAL_RCC_ETHMACRX_IS_CLK_ENABLED())
#define __HAL_RCC_ETHMAC_IS_CLK_DISABLED()    ((RCC->AHB1ENR &(RCC_AHB1ENR_ETHMACEN))== RESET)
#define __HAL_RCC_ETHMACTX_IS_CLK_DISABLED()  ((RCC->AHB1ENR &(RCC_AHB1ENR_ETHMACTXEN))== RESET)
#define __HAL_RCC_ETHMACRX_IS_CLK_DISABLED()  ((RCC->AHB1ENR &(RCC_AHB1ENR_ETHMACRXEN))== RESET)
#define __HAL_RCC_ETHMACPTP_IS_CLK_DISABLED() ((RCC->AHB1ENR &(RCC_AHB1ENR_ETHMACPTPEN))== RESET)
#define __HAL_RCC_ETH_IS_CLK_DISABLED()        (__HAL_RCC_ETHMAC_IS_CLK_DISABLED()   && \
                                                __HAL_RCC_ETHMACTX_IS_CLK_DISABLED() && \
	                                        __HAL_RCC_ETHMACRX_IS_CLK_DISABLED())
/**
  * @}
  */ 

/**
  * @brief  Enable ETHERNET clock.
  */
#define __HAL_RCC_ETH_CLK_ENABLE() do {                                     \
                                        __HAL_RCC_ETHMAC_CLK_ENABLE();      \
                                        __HAL_RCC_ETHMACTX_CLK_ENABLE();    \
                                        __HAL_RCC_ETHMACRX_CLK_ENABLE();    \
                                      } while(0)
/**
  * @brief  Disable ETHERNET clock.
  */
#define __HAL_RCC_ETH_CLK_DISABLE()  do {                                      \
                                          __HAL_RCC_ETHMACTX_CLK_DISABLE();    \
                                          __HAL_RCC_ETHMACRX_CLK_DISABLE();    \
                                          __HAL_RCC_ETHMAC_CLK_DISABLE();      \
                                        } while(0)
#endif /* STM32F207xx || STM32F217xx */
/**
  * @}
  */

/** @defgroup RCCEx_AHB2_Clock_Enable_Disable AHB2 Peripheral Clock Enable Disable
  * @brief  Enable or disable the AHB2 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before 
  *         using it.
  * @{
  */
#if defined(STM32F207xx) || defined(STM32F217xx)
#define __HAL_RCC_DCMI_CLK_ENABLE()   do { \
                                       __IO uint32_t tmpreg = 0x00; \
                                       SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_DCMIEN);\
                                       /* Delay after an RCC peripheral clock enabling */ \
                                       tmpreg = READ_BIT(RCC->AHB2ENR, RCC_AHB2ENR_DCMIEN);\
                                       UNUSED(tmpreg); \
                                       } while(0)

#define __HAL_RCC_DCMI_CLK_DISABLE()  (RCC->AHB2ENR &= ~(RCC_AHB2ENR_DCMIEN))
#endif /* STM32F207xx || STM32F217xx */

#if defined(STM32F215xx) || defined(STM32F217xx)
#define __HAL_RCC_CRYP_CLK_ENABLE()   do { \
                                       __IO uint32_t tmpreg = 0x00; \
                                       SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_CRYPEN);\
                                       /* Delay after an RCC peripheral clock enabling */ \
                                       tmpreg = READ_BIT(RCC->AHB2ENR, RCC_AHB2ENR_CRYPEN);\
                                       UNUSED(tmpreg); \
                                       } while(0)
#define __HAL_RCC_HASH_CLK_ENABLE()   do { \
                                       __IO uint32_t tmpreg = 0x00; \
                                       SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_HASHEN);\
                                       /* Delay after an RCC peripheral clock enabling */ \
                                       tmpreg = READ_BIT(RCC->AHB2ENR, RCC_AHB2ENR_HASHEN);\
                                       UNUSED(tmpreg); \
                                       } while(0)

#define __HAL_RCC_CRYP_CLK_DISABLE()  (RCC->AHB2ENR &= ~(RCC_AHB2ENR_CRYPEN))
#define __HAL_RCC_HASH_CLK_DISABLE()  (RCC->AHB2ENR &= ~(RCC_AHB2ENR_HASHEN))
#endif /* STM32F215xx || STM32F217xx */
/**
  * @}
  */

/** @defgroup RCC_AHB2_Peripheral_Clock_Enable_Disable_Status AHB2 Peripheral Clock Enable Disable Status
  * @brief  Get the enable or disable status of the AHB2 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */
#if defined(STM32F207xx) || defined(STM32F217xx)
#define __HAL_RCC_DCMI_IS_CLK_ENABLED()  ((RCC->AHB2ENR &(RCC_AHB2ENR_DCMIEN))!= RESET)
#define __HAL_RCC_DCMI_IS_CLK_DISABLED() ((RCC->AHB2ENR &(RCC_AHB2ENR_DCMIEN))== RESET)
#endif /* defined(STM32F207xx) || defined(STM32F217xx) */
#if defined(STM32F215xx) || defined(STM32F217xx)
#define __HAL_RCC_CRYP_IS_CLK_ENABLED()  ((RCC->AHB2ENR &(RCC_AHB2ENR_CRYPEN))!= RESET)
#define __HAL_RCC_HASH_IS_CLK_ENABLED()  ((RCC->AHB2ENR &(RCC_AHB2ENR_HASHEN))!= RESET)

#define __HAL_RCC_CRYP_IS_CLK_DISABLED()  ((RCC->AHB2ENR &(RCC_AHB2ENR_CRYPEN))== RESET)
#define __HAL_RCC_HASH_IS_CLK_DISABLED()  ((RCC->AHB2ENR &(RCC_AHB2ENR_HASHEN))== RESET)
#endif /* defined(STM32F215xx) || defined(STM32F217xx) */
/**
  * @}
  */ 
                                         
/** @defgroup RCCEx_AHB1_Force_Release_Reset AHB1 Force Release Reset 
  * @brief  Force or release AHB1 peripheral reset.
  * @{
  */
#if defined(STM32F207xx) || defined(STM32F217xx)
#define __HAL_RCC_ETHMAC_FORCE_RESET()   (RCC->AHB1RSTR |= (RCC_AHB1RSTR_ETHMACRST))
#define __HAL_RCC_ETHMAC_RELEASE_RESET() (RCC->AHB1RSTR &= ~(RCC_AHB1RSTR_ETHMACRST))
#endif /* STM32F207xx || STM32F217xx */                                        
/**
  * @}
  */

/** @defgroup RCCEx_AHB2_Force_Release_Reset AHB2 Force Release Reset 
  * @brief  Force or release AHB2 peripheral reset.
  * @{
  */
#if defined(STM32F207xx) || defined(STM32F217xx)
#define __HAL_RCC_DCMI_FORCE_RESET()   (RCC->AHB2RSTR |= (RCC_AHB2RSTR_DCMIRST))
#define __HAL_RCC_DCMI_RELEASE_RESET() (RCC->AHB2RSTR &= ~(RCC_AHB2RSTR_DCMIRST))
#endif /* STM32F207xx || STM32F217xx */

#if defined(STM32F215xx) || defined(STM32F217xx)
#define __HAL_RCC_CRYP_FORCE_RESET()   (RCC->AHB2RSTR |= (RCC_AHB2RSTR_CRYPRST))
#define __HAL_RCC_HASH_FORCE_RESET()   (RCC->AHB2RSTR |= (RCC_AHB2RSTR_HASHRST))

#define __HAL_RCC_CRYP_RELEASE_RESET() (RCC->AHB2RSTR &= ~(RCC_AHB2RSTR_CRYPRST))
#define __HAL_RCC_HASH_RELEASE_RESET() (RCC->AHB2RSTR &= ~(RCC_AHB2RSTR_HASHRST))
#endif /* STM32F215xx || STM32F217xx */

/**
  * @}
  */

/** @defgroup RCCEx_AHB1_LowPower_Enable_Disable AHB1 Peripheral Low Power Enable Disable
  * @brief  Enable or disable the AHB1 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */
#if defined(STM32F207xx) || defined(STM32F217xx)
#define __HAL_RCC_ETHMAC_CLK_SLEEP_ENABLE()     (RCC->AHB1LPENR |= (RCC_AHB1LPENR_ETHMACLPEN))
#define __HAL_RCC_ETHMACTX_CLK_SLEEP_ENABLE()   (RCC->AHB1LPENR |= (RCC_AHB1LPENR_ETHMACTXLPEN))
#define __HAL_RCC_ETHMACRX_CLK_SLEEP_ENABLE()   (RCC->AHB1LPENR |= (RCC_AHB1LPENR_ETHMACRXLPEN))
#define __HAL_RCC_ETHMACPTP_CLK_SLEEP_ENABLE()  (RCC->AHB1LPENR |= (RCC_AHB1LPENR_ETHMACPTPLPEN))

#define __HAL_RCC_ETHMAC_CLK_SLEEP_DISABLE()    (RCC->AHB1LPENR &= ~(RCC_AHB1LPENR_ETHMACLPEN))
#define __HAL_RCC_ETHMACTX_CLK_SLEEP_DISABLE()  (RCC->AHB1LPENR &= ~(RCC_AHB1LPENR_ETHMACTXLPEN))
#define __HAL_RCC_ETHMACRX_CLK_SLEEP_DISABLE()  (RCC->AHB1LPENR &= ~(RCC_AHB1LPENR_ETHMACRXLPEN))
#define __HAL_RCC_ETHMACPTP_CLK_SLEEP_DISABLE() (RCC->AHB1LPENR &= ~(RCC_AHB1LPENR_ETHMACPTPLPEN))
#endif /* STM32F207xx || STM32F217xx */
/**
  * @}
  */

/** @defgroup RCCEx_AHB2_LowPower_Enable_Disable AHB2 Peripheral Low Power Enable Disable
  * @brief  Enable or disable the AHB2 peripheral clock during Low Power (Sleep) mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wake-up from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during SLEEP mode.
  * @{
  */
#if defined(STM32F207xx) || defined(STM32F217xx)
#define __HAL_RCC_DCMI_CLK_SLEEP_ENABLE()  (RCC->AHB2LPENR |= (RCC_AHB2LPENR_DCMILPEN))
#define __HAL_RCC_DCMI_CLK_SLEEP_DISABLE() (RCC->AHB2LPENR &= ~(RCC_AHB2LPENR_DCMILPEN))
#endif /* STM32F207xx || STM32F217xx */

#if defined(STM32F215xx) || defined(STM32F217xx)
#define __HAL_RCC_CRYP_CLK_SLEEP_ENABLE()  (RCC->AHB2LPENR |= (RCC_AHB2LPENR_CRYPLPEN))
#define __HAL_RCC_HASH_CLK_SLEEP_ENABLE()  (RCC->AHB2LPENR |= (RCC_AHB2LPENR_HASHLPEN))

#define __HAL_RCC_CRYP_CLK_SLEEP_DISABLE() (RCC->AHB2LPENR &= ~(RCC_AHB2LPENR_CRYPLPEN))
#define __HAL_RCC_HASH_CLK_SLEEP_DISABLE() (RCC->AHB2LPENR &= ~(RCC_AHB2LPENR_HASHLPEN))
#endif /* STM32F215xx || STM32F217xx */
/**
  * @}
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup RCCEx_Exported_Functions
  *  @{
  */

/** @addtogroup RCCEx_Exported_Functions_Group1
  *  @{
  */
HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);
void HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit);

/**
  * @}
  */ 

/**
  * @}
  */
/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup RCCEx_Private_Constants RCC Private Constants
  * @{
  */

/** @defgroup RCCEx_BitAddress_AliasRegion RCC BitAddress AliasRegion
  * @brief RCC registers bit address in the alias region
  * @{
  */
#define PLL_TIMEOUT_VALUE          ((uint32_t)100)  /* 100 ms */
/**
  * @}
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup RCCEx_Private_Macros RCC Private Macros
  * @{
  */
/** @defgroup RCCEx_IS_RCC_Definitions RCC Private macros to check input parameters
  * @{
  */
#define IS_RCC_PERIPHCLOCK(SELECTION) ((1 <= (SELECTION)) && ((SELECTION) <= 0x0000000F))
#define IS_RCC_PLLI2SN_VALUE(VALUE) ((192 <= (VALUE)) && ((VALUE) <= 432))
#define IS_RCC_PLLI2SR_VALUE(VALUE) ((2 <= (VALUE)) && ((VALUE) <= 7))  
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

#endif /* __STM32F2xx_HAL_RCC_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
