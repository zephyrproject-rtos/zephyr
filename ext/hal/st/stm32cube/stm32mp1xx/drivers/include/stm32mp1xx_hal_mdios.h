/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_mdios.h
  * @author  MCD Application Team
  * @version V0.0.1
  * @date    01-July-2016
  * @brief   Header file of MDIOS HAL module.
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
#ifndef __STM32MP1xx_HAL_MDIOS_H
#define __STM32MP1xx_HAL_MDIOS_H

#ifdef __cplusplus
 extern "C" {
#endif

#if defined (MDIOS)
   
/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @addtogroup MDIOS
  * @{
  */ 

/* Exported types ------------------------------------------------------------*/
/** @defgroup MDIOS_Exported_Types MDIOS Exported Types
  * @{
  */
   
/** @defgroup MDIOS_Exported_Types_Group1 MDIOS State structures definition
  * @{
  */

typedef enum
{
  HAL_MDIOS_STATE_RESET             = 0x00U,    /*!< Peripheral not yet Initialized or disabled         */
  HAL_MDIOS_STATE_READY             = 0x01U,    /*!< Peripheral Initialized and ready for use           */
  HAL_MDIOS_STATE_BUSY              = 0x02U,    /*!< an internal process is ongoing                     */
  HAL_MDIOS_STATE_ERROR             = 0x04U     /*!< Reception process is ongoing                       */
}HAL_MDIOS_StateTypeDef;

/** 
  * @}
  */

/** @defgroup MDIOS_Exported_Types_Group2 MDIOS Init Structure definition
  * @{
  */

typedef struct
{
  uint32_t PortAddress;           /*!< Specifies the MDIOS port address.   
                                       This parameter can be a value from 0 to 31 */
  uint32_t PreambleCheck;         /*!< Specifies whether the preamble check is enabled or disabled.   
                                       This parameter can be a value of @ref MDIOS_Preamble_Check */   
}MDIOS_InitTypeDef;

/** 
  * @}
  */

/** @defgroup MDIOS_Exported_Types_Group4 MDIOS handle Structure definition
  * @{
  */

typedef struct
{
  MDIOS_TypeDef                *Instance;     /*!< Register base address       */
  
  MDIOS_InitTypeDef            Init;          /*!< MDIOS Init Structure        */
  
  __IO HAL_MDIOS_StateTypeDef  State;         /*!< MDIOS communication state   */
  
  HAL_LockTypeDef              Lock;          /*!< MDIOS Lock                  */
}MDIOS_HandleTypeDef;

/** 
  * @}
  */

/** 
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup MDIOS_Exported_Constants MDIOS Exported Constants
  * @{
  */

/** @defgroup MDIOS_Preamble_Check  MDIOS Preamble Check
  * @{
  */
#define MDIOS_PREAMBLE_CHECK_ENABLE      ((uint32_t)0x00000000U)
#define MDIOS_PREAMBLE_CHECK_DISABLE     MDIOS_CR_DPC  
/**
  * @}
  */

/** @defgroup MDIOS_Input_Output_Registers_Definitions MDIOS Input Output Registers Definitions
  * @{
  */
#define MDIOS_REG0                      ((uint32_t)0x00000000U)
#define MDIOS_REG1                      ((uint32_t)0x00000001U)
#define MDIOS_REG2                      ((uint32_t)0x00000002U)
#define MDIOS_REG3                      ((uint32_t)0x00000003U)
#define MDIOS_REG4                      ((uint32_t)0x00000004U)
#define MDIOS_REG5                      ((uint32_t)0x00000005U)
#define MDIOS_REG6                      ((uint32_t)0x00000006U)
#define MDIOS_REG7                      ((uint32_t)0x00000007U)
#define MDIOS_REG8                      ((uint32_t)0x00000008U)
#define MDIOS_REG9                      ((uint32_t)0x00000009U)
#define MDIOS_REG10                     ((uint32_t)0x0000000AU)
#define MDIOS_REG11                     ((uint32_t)0x0000000BU)
#define MDIOS_REG12                     ((uint32_t)0x0000000CU)
#define MDIOS_REG13                     ((uint32_t)0x0000000DU)
#define MDIOS_REG14                     ((uint32_t)0x0000000EU)
#define MDIOS_REG15                     ((uint32_t)0x0000000FU)
#define MDIOS_REG16                     ((uint32_t)0x00000010U)
#define MDIOS_REG17                     ((uint32_t)0x00000011U)
#define MDIOS_REG18                     ((uint32_t)0x00000012U)
#define MDIOS_REG19                     ((uint32_t)0x00000013U)
#define MDIOS_REG20                     ((uint32_t)0x00000014U)
#define MDIOS_REG21                     ((uint32_t)0x00000015U)
#define MDIOS_REG22                     ((uint32_t)0x00000016U)
#define MDIOS_REG23                     ((uint32_t)0x00000017U)
#define MDIOS_REG24                     ((uint32_t)0x00000018U)
#define MDIOS_REG25                     ((uint32_t)0x00000019U)
#define MDIOS_REG26                     ((uint32_t)0x0000001AU)
#define MDIOS_REG27                     ((uint32_t)0x0000001BU)
#define MDIOS_REG28                     ((uint32_t)0x0000001CU)
#define MDIOS_REG29                     ((uint32_t)0x0000001DU)
#define MDIOS_REG30                     ((uint32_t)0x0000001EU)
#define MDIOS_REG31                     ((uint32_t)0x0000001FU)
/**
  * @}
  */ 

/** @defgroup MDIOS_Registers_Flags  MDIOS Registers Flags
  * @{
  */
#define MDIOS_REG0_FLAG			((uint32_t)0x00000001U)
#define	MDIOS_REG1_FLAG			((uint32_t)0x00000002U)
#define	MDIOS_REG2_FLAG			((uint32_t)0x00000004U)
#define	MDIOS_REG3_FLAG			((uint32_t)0x00000008U)
#define	MDIOS_REG4_FLAG			((uint32_t)0x00000010U)
#define	MDIOS_REG5_FLAG			((uint32_t)0x00000020U)
#define	MDIOS_REG6_FLAG			((uint32_t)0x00000040U)
#define	MDIOS_REG7_FLAG			((uint32_t)0x00000080U)
#define	MDIOS_REG8_FLAG			((uint32_t)0x00000100U)
#define	MDIOS_REG9_FLAG			((uint32_t)0x00000200U)
#define	MDIOS_REG10_FLAG		((uint32_t)0x00000400U)
#define	MDIOS_REG11_FLAG		((uint32_t)0x00000800U)
#define	MDIOS_REG12_FLAG		((uint32_t)0x00001000U)
#define	MDIOS_REG13_FLAG		((uint32_t)0x00002000U)
#define	MDIOS_REG14_FLAG		((uint32_t)0x00004000U)
#define	MDIOS_REG15_FLAG		((uint32_t)0x00008000U)
#define	MDIOS_REG16_FLAG		((uint32_t)0x00010000U)
#define	MDIOS_REG17_FLAG		((uint32_t)0x00020000U)
#define	MDIOS_REG18_FLAG		((uint32_t)0x00040000U)
#define	MDIOS_REG19_FLAG		((uint32_t)0x00080000U)
#define	MDIOS_REG20_FLAG		((uint32_t)0x00100000U)
#define	MDIOS_REG21_FLAG		((uint32_t)0x00200000U)
#define	MDIOS_REG22_FLAG		((uint32_t)0x00400000U)
#define	MDIOS_REG23_FLAG		((uint32_t)0x00800000U)
#define	MDIOS_REG24_FLAG		((uint32_t)0x01000000U)
#define	MDIOS_REG25_FLAG		((uint32_t)0x02000000U)
#define	MDIOS_REG26_FLAG		((uint32_t)0x04000000U)
#define	MDIOS_REG27_FLAG		((uint32_t)0x08000000U)
#define	MDIOS_REG28_FLAG		((uint32_t)0x10000000U)
#define	MDIOS_REG29_FLAG		((uint32_t)0x20000000U)
#define	MDIOS_REG30_FLAG		((uint32_t)0x40000000U)
#define	MDIOS_REG31_FLAG		((uint32_t)0x80000000U)
#define	MDIOS_ALLREG_FLAG		((uint32_t)0xFFFFFFFFU)
/**
  * @}
  */

/** @defgroup MDIOS_Interrupt_sources Interrupt Sources
  * @{
  */
#define MDIOS_IT_WRITE                   MDIOS_CR_WRIE
#define MDIOS_IT_READ                    MDIOS_CR_RDIE
#define MDIOS_IT_ERROR                   MDIOS_CR_EIE
/**
  * @}
  */

/** @defgroup MDIOS_Interrupt_Flags  MDIOS Interrupt Flags
  * @{
  */
#define	MDIOS_TURNAROUND_ERROR_FLAG       MDIOS_SR_TERF
#define	MDIOS_START_ERROR_FLAG            MDIOS_SR_SERF
#define	MDIOS_PREAMBLE_ERROR_FLAG         MDIOS_SR_PERF
/**
  * @}
  */

 /** @defgroup MDIOS_Wakeup_Line  MDIOS Wakeup Line
  * @{
  */
#define MDIOS_WAKEUP_EXTI_LINE  ((uint32_t)0x00000400)  /* !<  42 - 32 = 10 */
/**
  * @}
  */
  
/**
  * @}
  */
/* Exported macros -----------------------------------------------------------*/
/** @defgroup MDIOS_Exported_Macros MDIOS Exported Macros
  * @{
  */

/** @brief Reset MDIOS handle state
  * @param  __HANDLE__: MDIOS handle.
  * @retval None
  */
#define __HAL_MDIOS_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = HAL_MDIOS_STATE_RESET)

/**
  * @brief  Enable/Disable the MDIOS peripheral.
  * @param  __HANDLE__: specifies the MDIOS handle.
  * @retval None
  */
#define __HAL_MDIOS_ENABLE(__HANDLE__) ((__HANDLE__)->Instance->CR |= MDIOS_CR_EN)
#define __HAL_MDIOS_DISABLE(__HANDLE__) ((__HANDLE__)->Instance->CR &= ~MDIOS_CR_EN)


/**
  * @brief  Enable the MDIOS device interrupt.
  * @param  __HANDLE__: specifies the MDIOS handle.
  * @param  __INTERRUPT__ : specifies the MDIOS interrupt sources to be enabled.
  *         This parameter can be one or a combination of the following values:
  *            @arg MDIOS_IT_WRITE: Register write interrupt
  *            @arg MDIOS_IT_READ: Register read interrupt
  *            @arg MDIOS_IT_ERROR: Error interrupt 
  * @retval None
  */
#define __HAL_MDIOS_ENABLE_IT(__HANDLE__, __INTERRUPT__)  ((__HANDLE__)->Instance->CR |= (__INTERRUPT__))

/**
  * @brief  Disable the MDIOS device interrupt.
  * @param  __HANDLE__: specifies the MDIOS handle.
  * @param  __INTERRUPT__ : specifies the MDIOS interrupt sources to be disabled.
  *         This parameter can be one or a combination of the following values:
  *            @arg MDIOS_IT_WRITE: Register write interrupt
  *            @arg MDIOS_IT_READ: Register read interrupt
  *            @arg MDIOS_IT_ERROR: Error interrupt 
  * @retval None
  */
#define __HAL_MDIOS_DISABLE_IT(__HANDLE__, __INTERRUPT__)  ((__HANDLE__)->Instance->CR &= ~(__INTERRUPT__))

/** @brief Set MDIOS slave get write register flag
  * @param  __HANDLE__: specifies the MDIOS handle.
  * @param  __FLAG__: specifies the write register flag
  * @retval The state of write flag
  */
#define __HAL_MDIOS_GET_WRITE_FLAG(__HANDLE__, __FLAG__)      ((__HANDLE__)->Instance->WRFR &  (__FLAG__))

/** @brief MDIOS slave get read register flag
  * @param  __HANDLE__: specifies the MDIOS handle.
  * @param  __FLAG__: specifies the read register flag
  * @retval The state of read flag
  */
#define __HAL_MDIOS_GET_READ_FLAG(__HANDLE__, __FLAG__)        ((__HANDLE__)->Instance->RDFR &  (__FLAG__))

/** @brief MDIOS slave get interrupt
  * @param  __HANDLE__: specifies the MDIOS handle.
  * @param  __FLAG__ : specifies the Error flag.
  *         This parameter can be one or a combination of the following values:
  *            @arg MDIOS_TURNARROUND_ERROR_FLAG: Register write interrupt
  *            @arg MDIOS_START_ERROR_FLAG: Register read interrupt
  *            @arg MDIOS_PREAMBLE_ERROR_FLAG: Error interrupt 
  * @retval The state of the error flag
  */
#define __HAL_MDIOS_GET_ERROR_FLAG(__HANDLE__, __FLAG__)       ((__HANDLE__)->Instance->SR &  (__FLAG__))

/** @brief  MDIOS slave clear interrupt
  * @param  __HANDLE__: specifies the MDIOS handle.
  * @param  __FLAG__ : specifies the Error flag.
  *         This parameter can be one or a combination of the following values:
  *            @arg MDIOS_TURNARROUND_ERROR_FLAG: Register write interrupt
  *            @arg MDIOS_START_ERROR_FLAG: Register read interrupt
  *            @arg MDIOS_PREAMBLE_ERROR_FLAG: Error interrupt 
  * @retval none
  */
#define __HAL_MDIOS_CLEAR_ERROR_FLAG(__HANDLE__, __FLAG__)       ((__HANDLE__)->Instance->CLRFR) |= (__FLAG__)

/**
  * @brief  Checks whether the specified MDIOS interrupt is set or not.
  * @param  __HANDLE__: specifies the MDIOS handle.
  * @param  __INTERRUPT__ : specifies the MDIOS interrupt sources
  *            This parameter can be one or a combination of the following values:
  *            @arg MDIOS_IT_WRITE: Register write interrupt
  *            @arg MDIOS_IT_READ: Register read interrupt
  *            @arg MDIOS_IT_ERROR: Error interrupt 
  * @retval The state of the interrupt source
  */
#define __HAL_MDIOS_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__) ((__HANDLE__)->Instance->CR & (__INTERRUPT__))

/**
  * @brief Enable the MDIOS WAKEUP Exti Line.
  * @param  __EXTI_LINE__: specifies the MDIOS WAKEUP Exti sources to be enabled.
  * This parameter can be:
  *   @arg MDIOS_WAKEUP_EXTI_LINE     
  * @retval None.
  */
#define __HAL_MDIOS_WAKEUP_EXTI_ENABLE_IT(__EXTI_LINE__)   (EXTI_D1->IMR2 |= (__EXTI_LINE__))

/**
  * @brief Enable the MDIOS WAKEUP Exti Line by Domain2.
  * @param  __EXTILINE__: specifies the MDIOS WAKEUP Exti sources to be enabled.
  * This parameter can be:
  *   @arg MDIOS_WAKEUP_EXTI_LINE     
  * @retval None.
  */
#define __HAL_MDIOS_WAKEUP_EXTID2_ENABLE_IT(__EXTI_LINE__)   (EXTI_D2->IMR2 |= (__EXTI_LINE__)) 

/**
  * @brief checks whether the specified MDIOS WAKEUP Exti interrupt flag is set or not.
  * @param  __EXTILINE__: specifies the MDIOS WAKEUP Exti sources to be cleared.
  * This parameter can be:
  *   @arg MDIOS_WAKEUP_EXTI_LINE  
  * @retval EXTI MDIOS WAKEUP Line Status.
  */
#define __HAL_MDIOS_WAKEUP_EXTI_GET_FLAG(__EXTI_LINE__)  (EXTI_D1->PR2 & (__EXTI_LINE__))

/**
  * @brief checks whether the specified MDIOS WAKEUP Exti interrupt flag is set or not.
  * @param  __EXTILINE__: specifies the MDIOS WAKEUP Exti sources to be cleared.
  * This parameter can be:
  *   @arg MDIOS_WAKEUP_EXTI_LINE  
  * @retval EXTI MDIOS WAKEUP Line Status.
  */
#define __HAL_MDIOS_WAKEUP_EXTID2_GET_FLAG(__EXTI_LINE__)  (EXTI_D2->PR2 & (__EXTI_LINE__))

/**
  * @brief Clear the MDIOS WAKEUP Exti flag.
  * @param  __EXTILINE__: specifies the MDIOS WAKEUP Exti sources to be cleared.
  * This parameter can be:
  *   @arg MDIOS_WAKEUP_EXTI_LINE  
  * @retval None.
  */
#define __HAL_MDIOS_WAKEUP_EXTI_CLEAR_FLAG(__EXTI_LINE__) (EXTI_D1->PR2 = (__EXTI_LINE__))

/**
  * @brief Clear the MDIOS WAKEUP Exti flag.
  * @param  __EXTILINE__: specifies the MDIOS WAKEUP Exti sources to be cleared.
  * This parameter can be:
  *   @arg MDIOS_WAKEUP_EXTI_LINE  
  * @retval None.
  */
#define __HAL_MDIOS_WAKEUP_EXTID2_CLEAR_FLAG(__EXTI_LINE__) (EXTI_D2->PR2 = (__EXTI_LINE__))

/**
  * @brief  enable rising edge interrupt on selected EXTI line.
  * @param  __EXTILINE__: specifies the ETH WAKEUP EXTI sources to be disabled.
  * This parameter can be:
  *  @arg ETH_WAKEUP_EXTI_LINE
  * @retval None
  */
#define __HAL_MDIOS_WAKEUP_EXTI_ENABLE_RISING_EDGE(__EXTI_LINE__) (EXTI->FTSR2 &= ~(__EXTI_LINE__)); \
                                                                   (EXTI->RTSR2 |= (__EXTI_LINE__)) 

/**
  * @brief  enable falling edge interrupt on selected EXTI line.
  * @param  __EXTILINE__: specifies the ETH WAKEUP EXTI sources to be disabled.
  * This parameter can be:
  *  @arg ETH_WAKEUP_EXTI_LINE
  * @retval None
  */
#define __HAL_MDIOS_WAKEUP_EXTI_ENABLE_FALLING_EDGE(__EXTI_LINE__) (EXTI->RTSR2 &= ~(__EXTI_LINE__));\
                                                                 (EXTI->FTSR2 |= (__EXTI_LINE__))

/**
  * @brief  enable falling edge interrupt on selected EXTI line.
  * @param  __EXTILINE__: specifies the ETH WAKEUP EXTI sources to be disabled.
  * This parameter can be:
  *  @arg ETH_WAKEUP_EXTI_LINE
  * @retval None
  */
#define __HAL_MDIOS_WAKEUP_EXTI_ENABLE_RISING_FALLING_EDGE(__EXTI_LINE__) (EXTI->RTSR2 |= (__EXTI_LINE__));\
                                                                          (EXTI->FTSR2 |= (__EXTI_LINE__))

/**
  * @brief  Generates a Software interrupt on selected EXTI line.
  * @param  __EXTILINE__: specifies the MDIOS WAKEUP EXTI sources to be disabled.
  * This parameter can be:
  *  @arg MDIOS_WAKEUP_EXTI_LINE
  * @retval None
  */
#define __HAL_MDIOS_WAKEUP_EXTI_GENERATE_SWIT(__EXTI_LINE__) (EXTI->SWIER2 |= (__EXTI_LINE__))   

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup MDIOS_Exported_Functions MDIOS Exported Functions
  * @{
  */

/** @addtogroup MDIOS_Exported_Functions_Group1
  * @{
  */
HAL_StatusTypeDef HAL_MDIOS_Init(MDIOS_HandleTypeDef *hmdios);
HAL_StatusTypeDef HAL_MDIOS_DeInit(MDIOS_HandleTypeDef *hmdios);
void HAL_MDIOS_MspInit(MDIOS_HandleTypeDef *hmdios);
void  HAL_MDIOS_MspDeInit(MDIOS_HandleTypeDef *hmdios);
/**
  * @}
  */

/** @addtogroup MDIOS_Exported_Functions_Group2
  * @{
  */
HAL_StatusTypeDef HAL_MDIOS_WriteReg(MDIOS_HandleTypeDef *hmdios,  uint32_t RegNum, uint16_t Data);
HAL_StatusTypeDef HAL_MDIOS_ReadReg(MDIOS_HandleTypeDef *hmdios,  uint32_t RegNum, uint16_t *pData);

uint32_t HAL_MDIOS_GetWrittenRegAddress(MDIOS_HandleTypeDef *hmdios);
uint32_t HAL_MDIOS_GetReadRegAddress(MDIOS_HandleTypeDef *hmdios);
HAL_StatusTypeDef HAL_MDIOS_ClearWriteRegAddress(MDIOS_HandleTypeDef *hmdios, uint32_t RegNum);
HAL_StatusTypeDef HAL_MDIOS_ClearReadRegAddress(MDIOS_HandleTypeDef *hmdios, uint32_t RegNum);

HAL_StatusTypeDef HAL_MDIOS_EnableEvents(MDIOS_HandleTypeDef *hmdios);
void HAL_MDIOS_IRQHandler(MDIOS_HandleTypeDef *hmdios);
void HAL_MDIOS_WriteCpltCallback(MDIOS_HandleTypeDef *hmdios);
void HAL_MDIOS_ReadCpltCallback(MDIOS_HandleTypeDef *hmdios);
void HAL_MDIOS_ErrorCallback(MDIOS_HandleTypeDef *hmdios);
void HAL_MDIOS_WakeUpCallback(MDIOS_HandleTypeDef *hmdios);
/**
  * @}
  */

/** @addtogroup MDIOS_Exported_Functions_Group3
  * @{
  */
uint32_t HAL_MDIOS_GetError(MDIOS_HandleTypeDef *hmdios);
HAL_MDIOS_StateTypeDef HAL_MDIOS_GetState(MDIOS_HandleTypeDef *hmdios);
/**
  * @}
  */

/**
  * @}
  */

/* Private types -------------------------------------------------------------*/
/** @defgroup MDIOS_Private_Types MDIOS Private Types
  * @{
  */

/**
  * @}
  */ 

/* Private variables ---------------------------------------------------------*/
/** @defgroup MDIOS_Private_Variables MDIOS Private Variables
  * @{
  */

/**
  * @}
  */

/* Private constants ---------------------------------------------------------*/
/** @defgroup MDIOS_Private_Constants MDIOS Private Constants
  * @{
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup MDIOS_Private_Macros MDIOS Private Macros
  * @{
  */

#define IS_MDIOS_PORTADDRESS(__ADDR__) ((__ADDR__) < 32)

#define IS_MDIOS_REGISTER(__REGISTER__) ((__REGISTER__) < 32)

#define IS_MDIOS_PREAMBLECHECK(__PREAMBLECHECK__) (((__PREAMBLECHECK__) == MDIOS_PREAMBLE_CHECK_ENABLE) || \
                                                   ((__PREAMBLECHECK__) == MDIOS_PREAMBLE_CHECK_DISABLE))

 /**
  * @}
  */
  
/* Private functions ---------------------------------------------------------*/
/** @defgroup MDIOS_Private_Functions MDIOS Private Functions
  * @{
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

#endif /* MDIOS */

#ifdef __cplusplus
}
#endif

#endif /* __STM32MP1xx_HAL_MDIOS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
