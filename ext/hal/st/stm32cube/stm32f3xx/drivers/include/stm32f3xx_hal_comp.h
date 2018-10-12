/**
  ******************************************************************************
  * @file    stm32f3xx_hal_comp.h
  * @author  MCD Application Team
  * @brief   Header file of COMP HAL module.
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
#ifndef __STM32F3xx_HAL_COMP_H
#define __STM32F3xx_HAL_COMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal_def.h"

/** @addtogroup STM32F3xx_HAL_Driver
  * @{
  */

/** @addtogroup COMP
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup COMP_Exported_Types COMP Exported Types
  * @{
  */
/**
  * @brief  COMP Init structure definition
  */
typedef struct
{

  uint32_t InvertingInput;     /*!< Selects the inverting input of the comparator.
                                    This parameter can be a value of @ref COMPEx_InvertingInput */

  uint32_t NonInvertingInput;  /*!< Selects the non inverting input of the comparator.
                                    This parameter can be a value of @ref COMPEx_NonInvertingInput
                                    Note: Only available on STM32F302xB/xC, STM32F303xB/xC and STM32F358xx devices */

  uint32_t Output;             /*!< Selects the output redirection of the comparator.
                                    This parameter can be a value of @ref COMPEx_Output */

  uint32_t OutputPol;          /*!< Selects the output polarity of the comparator.
                                    This parameter can be a value of @ref COMP_OutputPolarity */

  uint32_t Hysteresis;         /*!< Selects the hysteresis voltage of the comparator.
                                    This parameter can be a value of @ref COMPEx_Hysteresis
                                    Note: Only available on STM32F302xB/xC, STM32F303xB/xC, STM32F373xB/xC, STM32F358xx and STM32F378xx devices */

  uint32_t BlankingSrce;       /*!< Selects the output blanking source of the comparator.
                                    This parameter can be a value of @ref COMPEx_BlankingSrce
                                    Note: Not available on STM32F373xB/C and STM32F378xx devices */

  uint32_t Mode;               /*!< Selects the operating consumption mode of the comparator
                                    to adjust the speed/consumption.
                                    This parameter can be a value of @ref COMPEx_Mode
                                    Note: Not available on STM32F301x6/x8, STM32F302x6/x8, STM32F334x6/x8, STM32F318xx and STM32F328xx devices */

  uint32_t WindowMode;         /*!< Selects the window mode of the comparator X (X=2U, 4 or 6 if available).
                                    This parameter can be a value of @ref COMPEx_WindowMode */

  uint32_t TriggerMode;        /*!< Selects the trigger mode of the comparator (interrupt mode).
                                    This parameter can be a value of @ref COMP_TriggerMode */

} COMP_InitTypeDef;

/**
  * @brief  HAL State structures definition
  */
typedef enum
{
  HAL_COMP_STATE_RESET             = 0x00U,    /*!< COMP not yet initialized or disabled             */
  HAL_COMP_STATE_READY             = 0x01U,    /*!< COMP initialized and ready for use               */
  HAL_COMP_STATE_READY_LOCKED      = 0x11U,    /*!< COMP initialized but the configuration is locked */
  HAL_COMP_STATE_BUSY              = 0x02U,    /*!< COMP is running                                  */
  HAL_COMP_STATE_BUSY_LOCKED       = 0x12     /*!< COMP is running and the configuration is locked  */
} HAL_COMP_StateTypeDef;

/**
  * @brief  COMP Handle Structure definition
  */
typedef struct
{
  COMP_TypeDef                *Instance; /*!< Register base address    */
  COMP_InitTypeDef            Init;      /*!< COMP required parameters */
  HAL_LockTypeDef             Lock;      /*!< Locking object           */
  __IO HAL_COMP_StateTypeDef  State;     /*!< COMP communication state */
} COMP_HandleTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup COMP_Exported_Constants COMP Exported Constants
  * @{
  */

/** @defgroup COMP_OutputPolarity COMP Output Polarity
  * @{
  */
#define COMP_OUTPUTPOL_NONINVERTED             (0x00000000U)  /*!< COMP output on GPIO isn't inverted */
#define COMP_OUTPUTPOL_INVERTED                COMP_CSR_COMPxPOL       /*!< COMP output on GPIO is inverted  */
/**
  * @}
  */

/** @defgroup COMP_OutputLevel COMP Output Level
  * @{
  */
/* When output polarity is not inverted, comparator output is low when
   the non-inverting input is at a lower voltage than the inverting input*/
#define COMP_OUTPUTLEVEL_LOW                   (0x00000000U)
/* When output polarity is not inverted, comparator output is high when
   the non-inverting input is at a higher voltage than the inverting input */
#define COMP_OUTPUTLEVEL_HIGH                  COMP_CSR_COMPxOUT
/**
  * @}
  */

/** @defgroup COMP_TriggerMode COMP Trigger Mode
  * @{
  */
#define COMP_TRIGGERMODE_NONE                 (0x00000000U)   /*!< No External Interrupt trigger detection */
#define COMP_TRIGGERMODE_IT_RISING            (0x00000001U)   /*!< External Interrupt Mode with Rising edge trigger detection */
#define COMP_TRIGGERMODE_IT_FALLING           (0x00000002U)   /*!< External Interrupt Mode with Falling edge trigger detection */
#define COMP_TRIGGERMODE_IT_RISING_FALLING    (0x00000003U)   /*!< External Interrupt Mode with Rising/Falling edge trigger detection */
#define COMP_TRIGGERMODE_EVENT_RISING         (0x00000010U)   /*!< Event Mode with Rising edge trigger detection */
#define COMP_TRIGGERMODE_EVENT_FALLING        (0x00000020U)   /*!< Event Mode with Falling edge trigger detection */
#define COMP_TRIGGERMODE_EVENT_RISING_FALLING (0x00000030U)   /*!< Event Mode with Rising/Falling edge trigger detection */
/**
  * @}
  */

/** @defgroup COMP_State_Lock COMP State Lock
  * @{
  */
#define COMP_STATE_BIT_LOCK                   (0x00000010U)   /* Lock bit in COMP handle state */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup COMP_Exported_Macros COMP Exported Macros
  * @{
  */

/** @brief  Reset COMP handle state.
  * @param  __HANDLE__  COMP handle.
  * @retval None
  */
#define __HAL_COMP_RESET_HANDLE_STATE(__HANDLE__)  ((__HANDLE__)->State = HAL_COMP_STATE_RESET)
/**
  * @}
  */

/* Include COMP HAL Extended module */
#include "stm32f3xx_hal_comp_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @addtogroup COMP_Exported_Functions
  * @{
  */
/** @addtogroup COMP_Exported_Functions_Group1
 * @{
 */
/* Initialization and de-initialization functions  ****************************/
HAL_StatusTypeDef HAL_COMP_Init(COMP_HandleTypeDef *hcomp);
HAL_StatusTypeDef HAL_COMP_DeInit(COMP_HandleTypeDef *hcomp);
void              HAL_COMP_MspInit(COMP_HandleTypeDef *hcomp);
void              HAL_COMP_MspDeInit(COMP_HandleTypeDef *hcomp);
/**
 * @}
 */

/* IO operation functions  *****************************************************/
/** @addtogroup COMP_Exported_Functions_Group2
 * @{
 */
HAL_StatusTypeDef HAL_COMP_Start(COMP_HandleTypeDef *hcomp);
HAL_StatusTypeDef HAL_COMP_Stop(COMP_HandleTypeDef *hcomp);
HAL_StatusTypeDef HAL_COMP_Start_IT(COMP_HandleTypeDef *hcomp);
HAL_StatusTypeDef HAL_COMP_Stop_IT(COMP_HandleTypeDef *hcomp);
void              HAL_COMP_IRQHandler(COMP_HandleTypeDef *hcomp);
/* Callback in Interrupt mode */
void              HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp);
/**
  * @}
  */

/* Peripheral Control functions  ************************************************/
/** @addtogroup COMP_Exported_Functions_Group3
 * @{
 */
HAL_StatusTypeDef HAL_COMP_Lock(COMP_HandleTypeDef *hcomp);
uint32_t          HAL_COMP_GetOutputLevel(COMP_HandleTypeDef *hcomp);
/**
  * @}
  */

/* Peripheral State functions  **************************************************/
/** @addtogroup COMP_Exported_Functions_Group4
  * @{
  */
HAL_COMP_StateTypeDef HAL_COMP_GetState(COMP_HandleTypeDef *hcomp);
/**
  * @}
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup COMP_Private_Macros COMP Private Macros
  * @{
  */

/** @defgroup COMP_IS_COMP_Definitions COMP Private macros to check input parameters
  * @{
  */

#define IS_COMP_OUTPUTPOL(__POL__)  (((__POL__) == COMP_OUTPUTPOL_NONINVERTED)  || \
                                     ((__POL__) == COMP_OUTPUTPOL_INVERTED))

#define IS_COMP_TRIGGERMODE(__MODE__)  (((__MODE__) == COMP_TRIGGERMODE_NONE)                 || \
                                        ((__MODE__) == COMP_TRIGGERMODE_IT_RISING)            || \
                                        ((__MODE__) == COMP_TRIGGERMODE_IT_FALLING)           || \
                                        ((__MODE__) == COMP_TRIGGERMODE_IT_RISING_FALLING)    || \
                                        ((__MODE__) == COMP_TRIGGERMODE_EVENT_RISING)         || \
                                        ((__MODE__) == COMP_TRIGGERMODE_EVENT_FALLING)        || \
                                        ((__MODE__) == COMP_TRIGGERMODE_EVENT_RISING_FALLING))

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

#endif /* __STM32F3xx_HAL_COMP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
