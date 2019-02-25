/**
  ******************************************************************************
  * @file    stm32wbxx_hal_hsem.h
  * @author  MCD Application Team
  * @brief   Header file of HSEM HAL module.
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
#ifndef STM32WBxx_HAL_HSEM_H
#define STM32WBxx_HAL_HSEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_def.h"

/** @addtogroup STM32WBxx_HAL_Driver
   * @{
   */

/** @addtogroup HSEM
   * @{
   */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup HSEM_Exported_Macros HSEM Exported Macros
  * @{
  */

/**
  * @brief  SemID to mask helper Macro.
  * @param  __SEMID__: semaphore ID from 0 to 31
  * @retval Semaphore Mask.
  */
#define __HAL_HSEM_SEMID_TO_MASK(__SEMID__) (1 << (__SEMID__))

/**
  * @brief  Enables the specified HSEM interrupts.
  * @param  __SEM_MASK__: semaphores Mask
  * @retval None.
  */
#if defined(DUAL_CORE)
#define __HAL_HSEM_ENABLE_IT(__SEM_MASK__) ((((SCB->CPUID & 0x000000F0) >> 4 )== 0x7) ? \
                                            (HSEM->C1IER |= (__SEM_MASK__)) : \
                                            (HSEM->C2IER |= (__SEM_MASK__)))
#else
#define __HAL_HSEM_ENABLE_IT(__SEM_MASK__) (HSEM->IER |= (__SEM_MASK__))
#endif /* DUAL_CORE */
/**
  * @brief  Disables the specified HSEM interrupts.
  * @param  __SEM_MASK__: semaphores Mask
  * @retval None.
  */
#if defined(DUAL_CORE)
#define __HAL_HSEM_DISABLE_IT(__SEM_MASK__) ((((SCB->CPUID & 0x000000F0) >> 4 )== 0x7) ? \
                                             (HSEM->C1IER &= ~(__SEM_MASK__)) :       \
                                             (HSEM->C2IER &= ~(__SEM_MASK__)))
#else
#define __HAL_HSEM_DISABLE_IT(__SEM_MASK__) (HSEM->IER &= ~(__SEM_MASK__))
#endif /* DUAL_CORE */

/**
  * @brief  Checks whether interrupt has occurred or not for semaphores specified by a mask.
  * @param  __SEM_MASK__: semaphores Mask
  * @retval semaphores Mask : Semaphores where an interrupt occurred.
  */
#if defined(DUAL_CORE)
#define __HAL_HSEM_GET_IT(__SEM_MASK__) ((((SCB->CPUID & 0x000000F0) >> 4 )== 0x7) ? \
                                         ((__SEM_MASK__) & HSEM->C1MISR) :        \
                                         ((__SEM_MASK__) & HSEM->C2MISR1))
#else
#define __HAL_HSEM_GET_IT(__SEM_MASK__) ((__SEM_MASK__) & HSEM->MISR)
#endif /* DUAL_CORE */

/**
  * @brief  Get the semaphores release status flags.
  * @param  __SEM_MASK__: semaphores Mask
  * @retval semaphores Mask : Semaphores where Release flags rise.
  */
#if defined(DUAL_CORE)
#define __HAL_HSEM_GET_FLAG(__SEM_MASK__) ((((SCB->CPUID & 0x000000F0) >> 4 )== 0x7) ? \
                                           (__SEM_MASK__) & HSEM->C1ISR :           \
                                           (__SEM_MASK__) & HSEM->C2ISR)
#else
#define __HAL_HSEM_GET_FLAG(__SEM_MASK__) ((__SEM_MASK__) & HSEM->ISR)
#endif /* DUAL_CORE */

/**
  * @brief  Clears the HSEM Interrupt flags.
  * @param  __SEM_MASK__: semaphores Mask
  * @retval None.
  */
#if defined(DUAL_CORE)
#define __HAL_HSEM_CLEAR_FLAG(__SEM_MASK__) ((((SCB->CPUID & 0x000000F0) >> 4 )== 0x7) ? \
                                             (HSEM->C1ICR |= (__SEM_MASK__)) :        \
                                             (HSEM->C2ICR |= (__SEM_MASK__)))
#else
#define __HAL_HSEM_CLEAR_FLAG(__SEM_MASK__) (HSEM->ICR |= (__SEM_MASK__))
#endif /* DUAL_CORE */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup HSEM_Exported_Functions HSEM Exported Functions
  * @{
  */

/** @addtogroup HSEM_Exported_Functions_Group1 Take and Release functions
  * @brief    HSEM Take and Release functions
  * @{
  */

/* HSEM semaphore take (lock) using 2-Step  method ****************************/
HAL_StatusTypeDef  HAL_HSEM_Take(uint32_t SemID, uint32_t ProcessID);
/* HSEM semaphore fast take (lock) using 1-Step  method ***********************/
HAL_StatusTypeDef  HAL_HSEM_FastTake(uint32_t SemID);
/* HSEM Check semaphore state Taken or not   **********************************/
uint32_t HAL_HSEM_IsSemTaken(uint32_t SemID);
/* HSEM Release  **************************************************************/
void  HAL_HSEM_Release(uint32_t SemID, uint32_t ProcessID);
/* HSEM Release All************************************************************/
void HAL_HSEM_ReleaseAll(uint32_t Key, uint32_t CoreID);

/**
  * @}
  */

/** @addtogroup HSEM_Exported_Functions_Group2 HSEM Set and Get Key functions
  * @brief    HSEM Set and Get Key functions.
  * @{
  */
/* HSEM Set Clear Key *********************************************************/
void  HAL_HSEM_SetClearKey(uint32_t Key);
/* HSEM Get Clear Key *********************************************************/
uint32_t HAL_HSEM_GetClearKey(void);
/**
  * @}
  */

/** @addtogroup HSEM_Exported_Functions_Group3
  * @brief   HSEM Notification functions
  * @{
  */
/* HSEM Activate HSEM Notification (When a semaphore is released) ) *****************/
void HAL_HSEM_ActivateNotification(uint32_t SemMask);
/* HSEM Deactivate HSEM Notification (When a semaphore is released)  ****************/
void HAL_HSEM_DeactivateNotification(uint32_t SemMask);
/* HSEM Free Callback (When a semaphore is released)  *******************************/
void HAL_HSEM_FreeCallback(uint32_t SemMask);
/* HSEM IRQ Handler  **********************************************************/
void HAL_HSEM_IRQHandler(void);

/**
  * @}
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup HSEM_Private_Macros HSEM Private Macros
  * @{
  */

#define IS_HSEM_SEMID(__SEMID__) ((__SEMID__) <= HSEM_SEMID_MAX )

#define IS_HSEM_PROCESSID(__PROCESSID__) ((__PROCESSID__) <= HSEM_PROCESSID_MAX )

#define IS_HSEM_KEY(__KEY__) ((__KEY__) <= HSEM_CLEAR_KEY_MAX )

#define IS_HSEM_COREID(__COREID__) (((__COREID__) == HSEM_CPU1_COREID) || \
                                    ((__COREID__) == HSEM_CPU2_COREID))


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

#endif /* STM32WBxx_HAL_HSEM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
