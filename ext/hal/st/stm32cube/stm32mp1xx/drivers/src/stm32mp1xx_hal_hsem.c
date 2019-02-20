/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_hsem.c
  * @author  MCD Application Team
  * @brief   HSEM HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the semaphore peripheral:
  *           + Semaphore Take function (2-Step Procedure) , non blocking
  *           + Semaphore FastTake function (1-Step Procedure) , non blocking
  *           + Semaphore Status check
  *           + Semaphore Clear Key Set and Get
  *           + Release and release all functions
  *           + Semaphore notification enabling and disabling and callnack functions
  *           + IRQ handler management
  *
  *
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
  [..]
      (#)Take a semaphore In 2-Step mode Using function HAL_HSEM_Take. This function takes as parameters :
           (++) the semaphore ID from 0 to 31
           (++) the process ID from 0 to 255
      (#) Fast Take semaphore In 1-Step mode Using function HAL_HSEM_FastTake. This function takes as parameter :
           (++) the semaphore ID from 0_ID to 31. Note that the process ID value is implicitly assumed as zero
      (#) Check if a semaphore is Taken using function HAL_HSEM_IsSemTaken. This function takes as parameter :
          (++) the semaphore ID from 0_ID to 31
          (++) It returns 1 if the given semaphore is taken otherwise (Free) zero
      (#)Release a semaphore using function with HAL_HSEM_Release. This function takes as parameters :
           (++) the semaphore ID from 0 to 31
           (++) the process ID from 0 to 255:
           (++) Note: If ProcessID and MasterID match, semaphore is freed, and an interrupt
         may be generated when enabled (notification activated). If ProcessID or MasterID does not match,
         semaphore remains taken (locked)

      (#)Release all semaphores at once taken by a given Master using function HAL_HSEM_Release_All
          This function takes as parameters :
           (++) the Release Key (value from 0 to 0xFFFF) can be Set or Get respectively by
              HAL_HSEM_SetClearKey() or HAL_HSEM_GetClearKey functions
           (++) the Master ID:
           (++) Note: If the Key and MasterID match, all semaphores taken by the given CPU that corresponds
           to MasterID  will be freed, and an interrupt may be generated when enabled (notification activated). If the
           Key or the MasterID doesn't match, semaphores remains taken (locked)

      (#)Semaphores Release all key functions:
         (++)  HAL_HSEM_SetClearKey() to set semaphore release all Key
         (++)  HAL_HSEM_GetClearKey() to get release all Key
      (#)Semaphores notification functions :
         (++)  HAL_HSEM_ActivateNotification to activate a notification callback on
               a given semaphores Mask (bitfield). When one or more semaphores defined by the mask are released
               the callback HAL_HSEM_FreeCallback will be asserted giving as parameters a mask of the released
               semaphores (bitfield).

         (++)  HAL_HSEM_DeactivateNotification to deactivate the notification of a given semaphores Mask (bitfield).
         (++) See the description of the macro __HAL_HSEM_SEMID_TO_MASK to check how to calculate a semaphore mask
                Used by the notification functions
     *** HSEM HAL driver macros list ***
     =============================================
     [..] Below the list of most used macros in HSEM HAL driver.

      (+) __HAL_HSEM_SEMID_TO_MASK: Helper macro to convert a Semaphore ID to a Mask.
      [..] Example of use :
      [..] mask = __HAL_HSEM_SEMID_TO_MASK(8)  |  __HAL_HSEM_SEMID_TO_MASK(21) | __HAL_HSEM_SEMID_TO_MASK(25).
      [..] All next macros take as parameter a semaphore Mask (bitfiled) that can be constructed using  __HAL_HSEM_SEMID_TO_MASK as the above example.
      (+) __HAL_HSEM_ENABLE_IT: Enable the specified semaphores Mask interrupts.
      (+) __HAL_HSEM_DISABLE_IT: Disable the specified semaphores Mask interrupts.
      (+) __HAL_HSEM_GET_IT: Checks whether the specified semaphore interrupt has occurred or not.
      (+) __HAL_HSEM_GET_FLAG: Get the semaphores status release flags.
      (+) __HAL_HSEM_CLEAR_FLAG: Clear the semaphores status release flags.

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

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @defgroup HSEM HSEM
  * @brief HSEM HAL module driver
  * @{
  */

#ifdef HAL_HSEM_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#if defined(DUAL_CORE)
#ifndef HSEM_R_MASTERID
#define HSEM_R_MASTERID HSEM_R_COREID
#endif

#ifndef HSEM_RLR_MASTERID
#define HSEM_RLR_MASTERID HSEM_RLR_COREID
#endif

#ifndef HSEM_CR_MASTERID
#define HSEM_CR_MASTERID HSEM_CR_COREID
#endif
#endif /* DUAL_CORE */
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/** @defgroup HSEM_Exported_Functions  HSEM Exported Functions
  * @{
  */

/** @defgroup HSEM_Exported_Functions_Group1 Take and Release functions
  *  @brief    HSEM Take and Release functions
  *
@verbatim
 ==============================================================================
              ##### HSEM Take and Release functions #####
 ==============================================================================
[..] This section provides functions allowing to:
      (+) Take a semaphore with 2 Step method
      (+) Fast Take a semaphore with 1 Step method
      (+) Check semaphore state Taken or not
      (+) Release a semaphore
      (+) Release all semaphore at once

@endverbatim
  * @{
  */


/**
  * @brief  Take a semaphore in 2 Step mode.
  * @param  SemID: semaphore ID from 0 to 31
  * @param  ProcessID: Process ID from 0 to 255
  * @retval HAL status
  */
HAL_StatusTypeDef  HAL_HSEM_Take(uint32_t SemID, uint32_t ProcessID)
{
  /* Check the parameters */
  assert_param(IS_HSEM_SEMID(SemID));
  assert_param(IS_HSEM_PROCESSID(ProcessID));

#if  USE_MULTI_CORE_SHARED_CODE != 0U
  /* First step  write R register with MasterID, processID and take bit=1*/
  HSEM->R[SemID] = ((ProcessID & HSEM_R_PROCID) | ((HAL_GetCurrentCPUID() << POSITION_VAL(HSEM_R_MASTERID)) & HSEM_R_MASTERID) | HSEM_R_LOCK);

  /* second step : read the R register . Take achieved if MasterID and processID match and take bit set to 1 */
  if (HSEM->R[SemID] == ((ProcessID & HSEM_R_PROCID) | ((HAL_GetCurrentCPUID() << POSITION_VAL(HSEM_R_MASTERID)) & HSEM_R_MASTERID) | HSEM_R_LOCK))
  {
    /*take success when MasterID and ProcessID match and take bit set*/
    return HAL_OK;
  }
#else
  /* First step  write R register with MasterID, processID and take bit=1*/
  HSEM->R[SemID] = (ProcessID | HSEM_CR_COREID_CURRENT | HSEM_R_LOCK);

  /* second step : read the R register . Take achieved if MasterID and processID match and take bit set to 1 */
  if (HSEM->R[SemID] == (ProcessID | HSEM_CR_COREID_CURRENT | HSEM_R_LOCK))
  {
    /*take success when MasterID and ProcessID match and take bit set*/
    return HAL_OK;
  }
#endif

  /* Semaphore take fails*/
  return HAL_ERROR;
}

/**
  * @brief  Fast Take a semaphore with 1 Step mode.
  * @param  SemID: semaphore ID from 0 to 31
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_HSEM_FastTake(uint32_t SemID)
{
  /* Check the parameters */
  assert_param(IS_HSEM_SEMID(SemID));

#if  USE_MULTI_CORE_SHARED_CODE != 0U
  /* Read the RLR register to take the semaphore */
  if (HSEM->RLR[SemID] == (((HAL_GetCurrentCPUID() << POSITION_VAL(HSEM_R_MASTERID)) & HSEM_RLR_MASTERID) | HSEM_RLR_LOCK))
  {
    /*take success when MasterID match and take bit set*/
    return HAL_OK;
  }
#else
  /* Read the RLR register to take the semaphore */
  if (HSEM->RLR[SemID] == (HSEM_CR_COREID_CURRENT | HSEM_RLR_LOCK))
  {
    /*take success when MasterID match and take bit set*/
    return HAL_OK;
  }
#endif

  /* Semaphore take fails */
  return HAL_ERROR;
}
/**
  * @brief  Check semaphore state Taken or not.
  * @param  SemID: semaphore ID
  * @retval HAL HSEM state
  */
uint32_t HAL_HSEM_IsSemTaken(uint32_t SemID)
{
  return (((HSEM->R[SemID] & HSEM_R_LOCK) != 0U) ? 1UL : 0UL);
}


/**
  * @brief  Release a semaphore.
  * @param  SemID: semaphore ID from 0 to 31
  * @param  ProcessID: Process ID from 0 to 255
  * @retval None
  */
void  HAL_HSEM_Release(uint32_t SemID, uint32_t ProcessID)
{
  /* Check the parameters */
  assert_param(IS_HSEM_SEMID(SemID));
  assert_param(IS_HSEM_PROCESSID(ProcessID));

  /* Clear the semaphore by writing to the R register : the MasterID , the processID and take bit = 0  */
  HSEM->R[SemID] = (ProcessID | HSEM_CR_COREID_CURRENT);

}

/**
  * @brief  Release All semaphore used by a given Master .
  * @param  Key: Semaphore Key , value from 0 to 0xFFFF
  * @param  CoreID: CoreID of the CPU that is using semaphores to be released
  * @retval None
  */
void HAL_HSEM_ReleaseAll(uint32_t Key, uint32_t CoreID)
{
  assert_param(IS_HSEM_KEY(Key));
  assert_param(IS_HSEM_COREID(CoreID));

  HSEM->CR = ((Key << HSEM_CR_KEY_Pos) | (CoreID << HSEM_CR_COREID_Pos));
}

/**
  * @}
  */

/** @defgroup HSEM_Exported_Functions_Group2 HSEM Set and Get Key functions
  *  @brief    HSEM Set and Get Key functions.
  *
@verbatim
  ==============================================================================
              ##### HSEM Set and Get Key functions #####
  ==============================================================================
    [..]  This section provides functions allowing to:
      (+) Set semaphore Key
      (+) Get semaphore Key
@endverbatim

  * @{
  */

/**
  * @brief  Set semaphore Key .
  * @param  Key: Semaphore Key , value from 0 to 0xFFFF
  * @retval None
  */
void  HAL_HSEM_SetClearKey(uint32_t Key)
{
  assert_param(IS_HSEM_KEY(Key));

  MODIFY_REG(HSEM->KEYR, HSEM_KEYR_KEY, (Key << HSEM_KEYR_KEY_Pos));

}

/**
  * @brief  Get semaphore Key .
  * @retval Semaphore Key , value from 0 to 0xFFFF
  */
uint32_t HAL_HSEM_GetClearKey(void)
{
  return (HSEM->KEYR >> HSEM_KEYR_KEY_Pos);
}

/**
  * @}
  */

/** @defgroup HSEM_Exported_Functions_Group3 HSEM IRQ handler management
  *  @brief    HSEM Notification functions.
  *
@verbatim
  ==============================================================================
      ##### HSEM IRQ handler management and Notification functions #####
  ==============================================================================
[..]  This section provides HSEM IRQ handler and Notification function.

@endverbatim
  * @{
  */

/**
  * @brief  Activate Semaphore release Notification for a given Semaphores Mask .
  * @param  SemMask: Mask of Released semaphores
  * @retval Semaphore Key
  */
void HAL_HSEM_ActivateNotification(uint32_t SemMask)
{
#if  USE_MULTI_CORE_SHARED_CODE != 0U
  /*enable the semaphore mask interrupts */
  if (HAL_GetCurrentCPUID() == HSEM_CPU1_COREID)
  {
    /*Use interrupt line 0 for CPU1 Master */
    HSEM->C1IER |= SemMask;
  }
  else /* HSEM_CPU2_COREID */
  {
    /*Use interrupt line 1 for CPU2 Master*/
    HSEM->C2IER |= SemMask;
  }
#else
  HSEM_COMMON->IER |= SemMask;
#endif
}

/**
  * @brief  Deactivate Semaphore release Notification for a given Semaphores Mask .
  * @param  SemMask: Mask of Released semaphores
  * @retval Semaphore Key
  */
void HAL_HSEM_DeactivateNotification(uint32_t SemMask)
{
#if  USE_MULTI_CORE_SHARED_CODE != 0U
  /*enable the semaphore mask interrupts */
  if (HAL_GetCurrentCPUID() == HSEM_CPU1_COREID)
  {
    /*Use interrupt line 0 for CPU1 Master */
    HSEM->C1IER &= ~SemMask;
  }
  else /* HSEM_CPU2_COREID */
  {
    /*Use interrupt line 1 for CPU2 Master*/
    HSEM->C2IER &= ~SemMask;
  }
#else
  HSEM_COMMON->IER &= ~SemMask;
#endif
}

/**
  * @brief  This function handles HSEM interrupt request
  * @retval None
  */
void HAL_HSEM_IRQHandler(void)
{
  uint32_t statusreg;
#if  USE_MULTI_CORE_SHARED_CODE != 0U
  if (HAL_GetCurrentCPUID() == HSEM_CPU1_COREID)
  {
    /* Get the list of masked freed semaphores*/
    statusreg = HSEM->C1MISR; /*Use interrupt line 0 for CPU1 Master*/

    /*Disable Interrupts*/
    HSEM->C1IER &= ~((uint32_t)statusreg);

    /*Clear Flags*/
    HSEM->C1ICR = ((uint32_t)statusreg);
  }
  else /* HSEM_CPU2_COREID */
  {
    /* Get the list of masked freed semaphores*/
    statusreg = HSEM->C2MISR;/*Use interrupt line 1 for CPU2 Master*/

    /*Disable Interrupts*/
    HSEM->C2IER &= ~((uint32_t)statusreg);

    /*Clear Flags*/
    HSEM->C2ICR = ((uint32_t)statusreg);
  }
#else
  /* Get the list of masked freed semaphores*/
  statusreg = HSEM_COMMON->MISR;

  /*Disable Interrupts*/
  HSEM_COMMON->IER &= ~((uint32_t)statusreg);

  /*Clear Flags*/
  HSEM_COMMON->ICR = ((uint32_t)statusreg);

#endif
  /* Call FreeCallback */
  HAL_HSEM_FreeCallback(statusreg);
}

/**
  * @brief Semaphore Released Callback.
  * @param SemMask: Mask of Released semaphores
  * @retval None
  */
__weak void HAL_HSEM_FreeCallback(uint32_t SemMask)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(SemMask);

  /* NOTE : This function should not be modified, when the callback is needed,
  the HAL_HSEM_FreeCallback can be implemented in the user file
    */
}

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_HSEM_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
