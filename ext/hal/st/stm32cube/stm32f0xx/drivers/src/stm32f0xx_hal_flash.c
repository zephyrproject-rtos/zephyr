/**
  ******************************************************************************
  * @file    stm32f0xx_hal_flash.c
  * @author  MCD Application Team
  * @brief   FLASH HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the internal FLASH memory:
  *           + Program operations functions
  *           + Memory Control functions
  *           + Peripheral State functions
  *
  @verbatim
  ==============================================================================
                        ##### FLASH peripheral features #####
  ==============================================================================
  [..] The Flash memory interface manages CPU AHB I-Code and D-Code accesses
       to the Flash memory. It implements the erase and program Flash memory operations
       and the read and write protection mechanisms.

  [..] The Flash memory interface accelerates code execution with a system of instruction
      prefetch.

  [..] The FLASH main features are:
      (+) Flash memory read operations
      (+) Flash memory program/erase operations
      (+) Read / write protections
      (+) Prefetch on I-Code
      (+) Option Bytes programming


                     ##### How to use this driver #####
  ==============================================================================
  [..]
      This driver provides functions and macros to configure and program the FLASH
      memory of all STM32F0xx devices.

      (#) FLASH Memory I/O Programming functions: this group includes all needed
          functions to erase and program the main memory:
        (++) Lock and Unlock the FLASH interface
        (++) Erase function: Erase page, erase all pages
        (++) Program functions: half word, word and doubleword
      (#) FLASH Option Bytes Programming functions: this group includes all needed
          functions to manage the Option Bytes:
        (++) Lock and Unlock the Option Bytes
        (++) Set/Reset the write protection
        (++) Set the Read protection Level
        (++) Program the user Option Bytes
        (++) Launch the Option Bytes loader
        (++) Erase Option Bytes
        (++) Program the data Option Bytes
        (++) Get the Write protection.
        (++) Get the user option bytes.

      (#) Interrupts and flags management functions : this group
          includes all needed functions to:
        (++) Handle FLASH interrupts
        (++) Wait for last FLASH operation according to its status
        (++) Get error flag status

  [..] In addition to these function, this driver includes a set of macros allowing
       to handle the following operations:

      (+) Set/Get the latency
      (+) Enable/Disable the prefetch buffer
      (+) Enable/Disable the FLASH interrupts
      (+) Monitor the FLASH flags status

  @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/** @addtogroup STM32F0xx_HAL_Driver
  * @{
  */

#ifdef HAL_FLASH_MODULE_ENABLED

/** @defgroup FLASH FLASH
  * @brief FLASH HAL module driver
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup FLASH_Private_Constants FLASH Private Constants
  * @{
  */
/**
  * @}
  */

/* Private macro ---------------------------- ---------------------------------*/
/** @defgroup FLASH_Private_Macros FLASH Private Macros
  * @{
  */

/**
  * @}
  */

/* Private variables ---------------------------------------------------------*/
/** @defgroup FLASH_Private_Variables FLASH Private Variables
  * @{
  */
/* Variables used for Erase pages under interruption*/
FLASH_ProcessTypeDef pFlash;
/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup FLASH_Private_Functions FLASH Private Functions
  * @{
  */
static  void   FLASH_Program_HalfWord(uint32_t Address, uint16_t Data);
static  void   FLASH_SetErrorCode(void);
extern void    FLASH_PageErase(uint32_t PageAddress);
/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/
/** @defgroup FLASH_Exported_Functions FLASH Exported Functions
  * @{
  */

/** @defgroup FLASH_Exported_Functions_Group1 Programming operation functions
  *  @brief   Programming operation functions
  *
@verbatim
@endverbatim
  * @{
  */

/**
  * @brief  Program halfword, word or double word at a specified address
  * @note   The function HAL_FLASH_Unlock() should be called before to unlock the FLASH interface
  *         The function HAL_FLASH_Lock() should be called after to lock the FLASH interface
  *
  * @note   If an erase and a program operations are requested simultaneously,
  *         the erase operation is performed before the program one.
  *
  * @note   FLASH should be previously erased before new programmation (only exception to this
  *         is when 0x0000 is programmed)
  *
  * @param  TypeProgram   Indicate the way to program at a specified address.
  *                       This parameter can be a value of @ref FLASH_Type_Program
  * @param  Address       Specifie the address to be programmed.
  * @param  Data          Specifie the data to be programmed
  *
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data)
{
  HAL_StatusTypeDef status = HAL_ERROR;
  uint8_t index = 0U;
  uint8_t nbiterations = 0U;

  /* Process Locked */
  __HAL_LOCK(&pFlash);

  /* Check the parameters */
  assert_param(IS_FLASH_TYPEPROGRAM(TypeProgram));
  assert_param(IS_FLASH_PROGRAM_ADDRESS(Address));

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

  if(status == HAL_OK)
  {
    if(TypeProgram == FLASH_TYPEPROGRAM_HALFWORD)
    {
      /* Program halfword (16-bit) at a specified address. */
      nbiterations = 1U;
    }
    else if(TypeProgram == FLASH_TYPEPROGRAM_WORD)
    {
      /* Program word (32-bit = 2*16-bit) at a specified address. */
      nbiterations = 2U;
    }
    else
    {
      /* Program double word (64-bit = 4*16-bit) at a specified address. */
      nbiterations = 4U;
    }

    for (index = 0U; index < nbiterations; index++)
    {
      FLASH_Program_HalfWord((Address + (2U*index)), (uint16_t)(Data >> (16U*index)));

        /* Wait for last operation to be completed */
        status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

        /* If the program operation is completed, disable the PG Bit */
        CLEAR_BIT(FLASH->CR, FLASH_CR_PG);
      /* In case of error, stop programation procedure */
      if (status != HAL_OK)
      {
        break;
      }
    }
  }

  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  return status;
}

/**
  * @brief  Program halfword, word or double word at a specified address  with interrupt enabled.
  * @note   The function HAL_FLASH_Unlock() should be called before to unlock the FLASH interface
  *         The function HAL_FLASH_Lock() should be called after to lock the FLASH interface
  *
  * @note   If an erase and a program operations are requested simultaneously,
  *         the erase operation is performed before the program one.
  *
  * @param  TypeProgram  Indicate the way to program at a specified address.
  *                      This parameter can be a value of @ref FLASH_Type_Program
  * @param  Address      Specifie the address to be programmed.
  * @param  Data         Specifie the data to be programmed
  *
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_Program_IT(uint32_t TypeProgram, uint32_t Address, uint64_t Data)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Process Locked */
  __HAL_LOCK(&pFlash);

  /* Check the parameters */
  assert_param(IS_FLASH_TYPEPROGRAM(TypeProgram));
  assert_param(IS_FLASH_PROGRAM_ADDRESS(Address));

  /* Enable End of FLASH Operation and Error source interrupts */
  __HAL_FLASH_ENABLE_IT(FLASH_IT_EOP | FLASH_IT_ERR);

  pFlash.Address = Address;
  pFlash.Data = Data;

  if(TypeProgram == FLASH_TYPEPROGRAM_HALFWORD)
  {
    pFlash.ProcedureOnGoing = FLASH_PROC_PROGRAMHALFWORD;
    /* Program halfword (16-bit) at a specified address. */
    pFlash.DataRemaining = 1U;
  }
  else if(TypeProgram == FLASH_TYPEPROGRAM_WORD)
  {
    pFlash.ProcedureOnGoing = FLASH_PROC_PROGRAMWORD;
    /* Program word (32-bit : 2*16-bit) at a specified address. */
    pFlash.DataRemaining = 2U;
  }
  else
  {
    pFlash.ProcedureOnGoing = FLASH_PROC_PROGRAMDOUBLEWORD;
    /* Program double word (64-bit : 4*16-bit) at a specified address. */
    pFlash.DataRemaining = 4U;
  }

  /* Program halfword (16-bit) at a specified address. */
  FLASH_Program_HalfWord(Address, (uint16_t)Data);

  return status;
}

/**
  * @brief This function handles FLASH interrupt request.
  * @retval None
  */
void HAL_FLASH_IRQHandler(void)
{
  uint32_t addresstmp = 0U;

  /* Check FLASH operation error flags */
  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR) ||__HAL_FLASH_GET_FLAG(FLASH_FLAG_PGERR))
  {
    /* Return the faulty address */
    addresstmp = pFlash.Address;
    /* Reset address */
    pFlash.Address = 0xFFFFFFFFU;

    /* Save the Error code */
    FLASH_SetErrorCode();

    /* FLASH error interrupt user callback */
    HAL_FLASH_OperationErrorCallback(addresstmp);

    /* Stop the procedure ongoing */
    pFlash.ProcedureOnGoing = FLASH_PROC_NONE;
  }

  /* Check FLASH End of Operation flag  */
  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP))
  {
    /* Clear FLASH End of Operation pending bit */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);

    /* Process can continue only if no error detected */
    if(pFlash.ProcedureOnGoing != FLASH_PROC_NONE)
    {
      if(pFlash.ProcedureOnGoing == FLASH_PROC_PAGEERASE)
      {
        /* Nb of pages to erased can be decreased */
        pFlash.DataRemaining--;

        /* Check if there are still pages to erase */
        if(pFlash.DataRemaining != 0U)
        {
          addresstmp = pFlash.Address;
          /*Indicate user which sector has been erased */
          HAL_FLASH_EndOfOperationCallback(addresstmp);

          /*Increment sector number*/
          addresstmp = pFlash.Address + FLASH_PAGE_SIZE;
          pFlash.Address = addresstmp;

          /* If the erase operation is completed, disable the PER Bit */
          CLEAR_BIT(FLASH->CR, FLASH_CR_PER);

          FLASH_PageErase(addresstmp);
        }
        else
        {
          /* No more pages to Erase, user callback can be called. */
          /* Reset Sector and stop Erase pages procedure */
          pFlash.Address = addresstmp = 0xFFFFFFFFU;
          pFlash.ProcedureOnGoing = FLASH_PROC_NONE;
          /* FLASH EOP interrupt user callback */
          HAL_FLASH_EndOfOperationCallback(addresstmp);
        }
      }
      else if(pFlash.ProcedureOnGoing == FLASH_PROC_MASSERASE)
      {
        /* Operation is completed, disable the MER Bit */
        CLEAR_BIT(FLASH->CR, FLASH_CR_MER);

          /* MassErase ended. Return the selected bank */
          /* FLASH EOP interrupt user callback */
          HAL_FLASH_EndOfOperationCallback(0);

          /* Stop Mass Erase procedure*/
          pFlash.ProcedureOnGoing = FLASH_PROC_NONE;
        }
      else
      {
        /* Nb of 16-bit data to program can be decreased */
        pFlash.DataRemaining--;

        /* Check if there are still 16-bit data to program */
        if(pFlash.DataRemaining != 0U)
        {
          /* Increment address to 16-bit */
          pFlash.Address += 2;
          addresstmp = pFlash.Address;

          /* Shift to have next 16-bit data */
          pFlash.Data = (pFlash.Data >> 16U);

          /* Operation is completed, disable the PG Bit */
          CLEAR_BIT(FLASH->CR, FLASH_CR_PG);

          /*Program halfword (16-bit) at a specified address.*/
          FLASH_Program_HalfWord(addresstmp, (uint16_t)pFlash.Data);
        }
        else
        {
          /* Program ended. Return the selected address */
          /* FLASH EOP interrupt user callback */
          if (pFlash.ProcedureOnGoing == FLASH_PROC_PROGRAMHALFWORD)
          {
            HAL_FLASH_EndOfOperationCallback(pFlash.Address);
          }
          else if (pFlash.ProcedureOnGoing == FLASH_PROC_PROGRAMWORD)
          {
            HAL_FLASH_EndOfOperationCallback(pFlash.Address - 2U);
          }
          else
          {
            HAL_FLASH_EndOfOperationCallback(pFlash.Address - 6U);
          }

          /* Reset Address and stop Program procedure */
          pFlash.Address = 0xFFFFFFFFU;
          pFlash.ProcedureOnGoing = FLASH_PROC_NONE;
        }
      }
    }
  }


  if(pFlash.ProcedureOnGoing == FLASH_PROC_NONE)
  {
    /* Operation is completed, disable the PG, PER and MER Bits */
    CLEAR_BIT(FLASH->CR, (FLASH_CR_PG | FLASH_CR_PER | FLASH_CR_MER));

    /* Disable End of FLASH Operation and Error source interrupts */
    __HAL_FLASH_DISABLE_IT(FLASH_IT_EOP | FLASH_IT_ERR);

    /* Process Unlocked */
    __HAL_UNLOCK(&pFlash);
  }
}

/**
  * @brief  FLASH end of operation interrupt callback
  * @param  ReturnValue The value saved in this parameter depends on the ongoing procedure
  *                 - Mass Erase: No return value expected
  *                 - Pages Erase: Address of the page which has been erased
  *                    (if 0xFFFFFFFF, it means that all the selected pages have been erased)
  *                 - Program: Address which was selected for data program
  * @retval none
  */
__weak void HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(ReturnValue);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_FLASH_EndOfOperationCallback could be implemented in the user file
   */
}

/**
  * @brief  FLASH operation error interrupt callback
  * @param  ReturnValue The value saved in this parameter depends on the ongoing procedure
  *                 - Mass Erase: No return value expected
  *                 - Pages Erase: Address of the page which returned an error
  *                 - Program: Address which was selected for data program
  * @retval none
  */
__weak void HAL_FLASH_OperationErrorCallback(uint32_t ReturnValue)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(ReturnValue);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_FLASH_OperationErrorCallback could be implemented in the user file
   */
}

/**
  * @}
  */

/** @defgroup FLASH_Exported_Functions_Group2 Peripheral Control functions
 *  @brief   management functions
 *
@verbatim
 ===============================================================================
                      ##### Peripheral Control functions #####
 ===============================================================================
    [..]
    This subsection provides a set of functions allowing to control the FLASH
    memory operations.

@endverbatim
  * @{
  */

/**
  * @brief  Unlock the FLASH control register access
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_Unlock(void)
{
  if (HAL_IS_BIT_SET(FLASH->CR, FLASH_CR_LOCK))
  {
    /* Authorize the FLASH Registers access */
    WRITE_REG(FLASH->KEYR, FLASH_KEY1);
    WRITE_REG(FLASH->KEYR, FLASH_KEY2);
  }
  else
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  Locks the FLASH control register access
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_Lock(void)
{
  /* Set the LOCK Bit to lock the FLASH Registers access */
  SET_BIT(FLASH->CR, FLASH_CR_LOCK);

  return HAL_OK;
}

/**
  * @brief  Unlock the FLASH Option Control Registers access.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_OB_Unlock(void)
{
  if (HAL_IS_BIT_CLR(FLASH->CR, FLASH_CR_OPTWRE))
  {
    /* Authorizes the Option Byte register programming */
    WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY1);
    WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY2);
  }
  else
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  Lock the FLASH Option Control Registers access.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_OB_Lock(void)
{
  /* Clear the OPTWRE Bit to lock the FLASH Option Byte Registers access */
  CLEAR_BIT(FLASH->CR, FLASH_CR_OPTWRE);

  return HAL_OK;
}

/**
  * @brief  Launch the option byte loading.
  * @note   This function will reset automatically the MCU.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_OB_Launch(void)
{
  /* Set the OBL_Launch bit to launch the option byte loading */
  SET_BIT(FLASH->CR, FLASH_CR_OBL_LAUNCH);

  /* Wait for last operation to be completed */
  return(FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE));
}

/**
  * @}
  */

/** @defgroup FLASH_Exported_Functions_Group3 Peripheral errors functions
 *  @brief    Peripheral errors functions
 *
@verbatim
 ===============================================================================
                      ##### Peripheral Errors functions #####
 ===============================================================================
    [..]
    This subsection permit to get in run-time errors of  the FLASH peripheral.

@endverbatim
  * @{
  */

/**
  * @brief  Get the specific FLASH error flag.
  * @retval FLASH_ErrorCode The returned value can be:
  *            @ref FLASH_Error_Codes
  */
uint32_t HAL_FLASH_GetError(void)
{
   return pFlash.ErrorCode;
}

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup FLASH_Private_Functions
 * @{
 */

/**
  * @brief  Program a half-word (16-bit) at a specified address.
  * @param  Address specify the address to be programmed.
  * @param  Data    specify the data to be programmed.
  * @retval None
  */
static void FLASH_Program_HalfWord(uint32_t Address, uint16_t Data)
{
  /* Clean the error context */
  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    /* Proceed to program the new data */
    SET_BIT(FLASH->CR, FLASH_CR_PG);

  /* Write data in the address */
  *(__IO uint16_t*)Address = Data;
}

/**
  * @brief  Wait for a FLASH operation to complete.
  * @param  Timeout  maximum flash operation timeout
  * @retval HAL Status
  */
HAL_StatusTypeDef FLASH_WaitForLastOperation(uint32_t Timeout)
{
  /* Wait for the FLASH operation to complete by polling on BUSY flag to be reset.
     Even if the FLASH operation fails, the BUSY flag will be reset and an error
     flag will be set */

  uint32_t tickstart = HAL_GetTick();

  while(__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY))
  {
    if (Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0U) || ((HAL_GetTick()-tickstart) > Timeout))
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /* Check FLASH End of Operation flag  */
  if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP))
  {
    /* Clear FLASH End of Operation pending bit */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);
  }

  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR)  ||
     __HAL_FLASH_GET_FLAG(FLASH_FLAG_PGERR))
  {
    /*Save the error code*/
    FLASH_SetErrorCode();
    return HAL_ERROR;
  }

  /* There is no error flag set */
  return HAL_OK;
}


/**
  * @brief  Set the specific FLASH error flag.
  * @retval None
  */
static void FLASH_SetErrorCode(void)
{
  uint32_t flags = 0U;

  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR))
  {
    pFlash.ErrorCode |= HAL_FLASH_ERROR_WRP;
    flags |= FLASH_FLAG_WRPERR;
  }
  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_PGERR))
  {
    pFlash.ErrorCode |= HAL_FLASH_ERROR_PROG;
    flags |= FLASH_FLAG_PGERR;
  }
  /* Clear FLASH error pending bits */
  __HAL_FLASH_CLEAR_FLAG(flags);
}
/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_FLASH_MODULE_ENABLED */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
