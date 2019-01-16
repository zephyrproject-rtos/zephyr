/**
  ******************************************************************************
  * @file    stm32g0xx_hal_flash.c
  * @author  MCD Application Team
  * @brief   FLASH HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the internal FLASH memory:
  *           + Program operations functions
  *           + Memory Control functions
  *           + Peripheral Errors functions
  *
 @verbatim
  ==============================================================================
                        ##### FLASH peripheral features #####
  ==============================================================================

  [..] The Flash memory interface manages CPU AHB I-Code and D-Code accesses
       to the Flash memory. It implements the erase and program Flash memory operations
       and the read and write protection mechanisms.

  [..] The Flash memory interface accelerates code execution with a system of instruction
       prefetch and cache lines.

  [..] The FLASH main features are:
      (+) Flash memory read operations
      (+) Flash memory program/erase operations
      (+) Read / write protections
      (+) Option bytes programming
      (+) Prefetch on I-Code
      (+) 32 cache lines of 4*64 bits on I-Code
      (+) Error code correction (ECC) : Data in flash are 72-bits word
          (8 bits added per double word)

                        ##### How to use this driver #####
 ==============================================================================
    [..]
      This driver provides functions and macros to configure and program the FLASH
      memory of all STM32G0xx devices.

      (#) Flash Memory IO Programming functions:
           (++) Lock and Unlock the FLASH interface using HAL_FLASH_Unlock() and
                HAL_FLASH_Lock() functions
           (++) Program functions: double word and fast program (full row programming)
           (++) There are two modes of programming:
            (+++) Polling mode using HAL_FLASH_Program() function
            (+++) Interrupt mode using HAL_FLASH_Program_IT() function

      (#) Interrupts and flags management functions:
           (++) Handle FLASH interrupts by calling HAL_FLASH_IRQHandler()
           (++) Callback functions are called when the flash operations are finished :
                HAL_FLASH_EndOfOperationCallback() when everything is ok, otherwise
                HAL_FLASH_OperationErrorCallback()
           (++) Get error flag status by calling HAL_GetError()

      (#) Option bytes management functions :
           (++) Lock and Unlock the option bytes using HAL_FLASH_OB_Unlock() and
                HAL_FLASH_OB_Lock() functions
           (++) Launch the reload of the option bytes using HAL_FLASH_Launch() function.
                In this case, a reset is generated

    [..]
      In addition to these functions, this driver includes a set of macros allowing
      to handle the following operations:
       (+) Set the latency
       (+) Enable/Disable the prefetch buffer
       (+) Enable/Disable the Instruction cache
       (+) Reset the Instruction cache
       (+) Enable/Disable the Flash power-down during low-power run and sleep modes
       (+) Enable/Disable the Flash interrupts
       (+) Monitor the Flash flags status

 @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @defgroup FLASH FLASH
  * @brief FLASH HAL module driver
  * @{
  */

#ifdef HAL_FLASH_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup FLASH_Private_Variables FLASH Private Variables
 * @{
 */
/**
  * @brief  Variable used for Program/Erase sectors under interruption
  */
extern FLASH_ProcessTypeDef pFlash;
FLASH_ProcessTypeDef pFlash  = {.Lock = HAL_UNLOCKED, \
                                .ErrorCode = HAL_FLASH_ERROR_NONE, \
                                .ProcedureOnGoing = FLASH_TYPENONE, \
                                .Address = 0U, \
                                .Page = 0U, \
                                .NbPagesToErase = 0U};
/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup FLASH_Private_Functions FLASH Private Functions
 * @{
 */
HAL_StatusTypeDef    FLASH_WaitForLastOperation(uint32_t Timeout);
extern void          FLASH_PageErase(uint32_t Page);
static void          FLASH_Program_DoubleWord(uint32_t Address, uint64_t Data);
static void          FLASH_Program_Fast(uint32_t Address, uint32_t DataAddress);
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup FLASH_Exported_Functions FLASH Exported Functions
  * @{
  */

/** @defgroup FLASH_Exported_Functions_Group1 Programming operation functions
 *  @brief   Programming operation functions
 *
@verbatim
 ===============================================================================
                  ##### Programming operation functions #####
 ===============================================================================
    [..]
    This subsection provides a set of functions allowing to manage the FLASH
    program operations.

@endverbatim
  * @{
  */

/**
  * @brief  Program double word or fast program of a row at a specified address.
  * @param  TypeProgram Indicate the way to program at a specified address.
  *                      This parameter can be a value of @ref FLASH_Type_Program
  * @param  Address specifies the address to be programmed.
  * @param  Data specifies the data to be programmed
  *               This parameter is the data for the double word program and the address where
  *               are stored the data for the row fast program
  *
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data)
{
  HAL_StatusTypeDef status;

  /* Check the parameters */
  assert_param(IS_FLASH_TYPEPROGRAM(TypeProgram));

  /* Process Locked */
  __HAL_LOCK(&pFlash);

  /* Reset error code */
  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

  if (status == HAL_OK)
  {
    if (TypeProgram == FLASH_TYPEPROGRAM_DOUBLEWORD)
    {
      /* Check the parameters */
      assert_param(IS_FLASH_PROGRAM_ADDRESS(Address));

      /* Program double-word (64-bit) at a specified address */
      FLASH_Program_DoubleWord(Address, Data);
    }
    else
    {
      /* Check the parameters */
      assert_param(IS_FLASH_FAST_PROGRAM_ADDRESS(Address));

      /* Fast program a 32 row double-word (64-bit) at a specified address */
      FLASH_Program_Fast(Address, (uint32_t)Data);
    }

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

    /* If the program operation is completed, disable the PG or FSTPG Bit */
      CLEAR_BIT(FLASH->CR, TypeProgram);
    }

  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  /* return status */
  return status;
}


/**
  * @brief  Program double word or fast program of a row at a specified address with interrupt enabled.
  * @param  TypeProgram Indicate the way to program at a specified address.
  *                      This parameter can be a value of @ref FLASH_Type_Program
  * @param  Address specifies the address to be programmed.
  * @param  Data specifies the data to be programmed
  *               This parameter is the data for the double word program and the address where
  *               are stored the data for the row fast program
  *
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_Program_IT(uint32_t TypeProgram, uint32_t Address, uint64_t Data)
{
  HAL_StatusTypeDef status;

  /* Check the parameters */
  assert_param(IS_FLASH_TYPEPROGRAM(TypeProgram));

  /* Process Locked */
  __HAL_LOCK(&pFlash);

  /* Reset error code */
  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

  if (status != HAL_OK)
  {
    /* Process Unlocked */
    __HAL_UNLOCK(&pFlash);
  }
  else
  {
    /* Set internal variables used by the IRQ handler */
    pFlash.ProcedureOnGoing = TypeProgram;
    pFlash.Address = Address;

    /* Enable End of Operation and Error interrupts */
    __HAL_FLASH_ENABLE_IT(FLASH_IT_EOP | FLASH_IT_OPERR | FLASH_IT_ECCC);

    if (TypeProgram == FLASH_TYPEPROGRAM_DOUBLEWORD)
    {
      /* Check the parameters */
      assert_param(IS_FLASH_PROGRAM_ADDRESS(Address));

      /* Program double-word (64-bit) at a specified address */
      FLASH_Program_DoubleWord(Address, Data);
    }
    else
    {
      /* Check the parameters */
      assert_param(IS_FLASH_FAST_PROGRAM_ADDRESS(Address));

      /* Fast program a 32 row double-word (64-bit) at a specified address */
      FLASH_Program_Fast(Address, (uint32_t)Data);
    }
  }

  /* return status */
  return status;
}


/**
  * @brief Handle FLASH interrupt request.
  * @retval None
  */
void HAL_FLASH_IRQHandler(void)
{
  uint32_t param = 0xFFFFFFFFu;
  uint32_t error;

  /* save flash errors. Only ECC detection can be checked here as ECCC
     generates NMI */
  error = (FLASH->SR & FLASH_FLAG_SR_ERROR);
  error |= (FLASH->ECCR & FLASH_FLAG_ECCC);

  CLEAR_BIT(FLASH->CR, pFlash.ProcedureOnGoing);
  
  /* A] Set parameter for user or error callbacks */
  /* check operation was a program or erase */
  if ((pFlash.ProcedureOnGoing & (FLASH_TYPEPROGRAM_DOUBLEWORD | FLASH_TYPEPROGRAM_FAST)) != 0x00u)
  {
    /* return adress being programmed */
    param = pFlash.Address;
  }
  else if ((pFlash.ProcedureOnGoing & (FLASH_TYPEERASE_MASS | FLASH_TYPEERASE_PAGES)) != 0x00u)
  {
    /* return page number being erased (0 for mass erase) */
    param = pFlash.Page;
  }
  else
  {
    /* Nothing to do */
  }

  /* B] Check errors */
  if (error != 0x00u)
  {
    /*Save the error code*/
    pFlash.ErrorCode |= error;

    /* clear error flags */
    FLASH->SR = FLASH_FLAG_SR_ERROR;
    FLASH->ECCR |= FLASH_FLAG_ECCC;

    /*Stop the procedure ongoing*/
    pFlash.ProcedureOnGoing = FLASH_TYPENONE;

    /* Error callback */
    HAL_FLASH_OperationErrorCallback(param);
  }

  /* C] Check FLASH End of Operation flag */
  if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP) != 0x00u)
  {
    /* Clear FLASH End of Operation pending bit */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);

    if (pFlash.ProcedureOnGoing == FLASH_TYPEERASE_PAGES)
    {
      /* Nb of pages to erased can be decreased */
      pFlash.NbPagesToErase--;

      /* Check if there are still pages to erase*/
      if (pFlash.NbPagesToErase != 0x00u)
      {
        /* Increment page number */
        pFlash.Page++;
        FLASH_PageErase(pFlash.Page);
      }
      else
      {
        /* No more pages to erase: stop erase pages procedure */
        pFlash.ProcedureOnGoing = FLASH_TYPENONE;
      }
    }
    else
    {
      /*Stop the ongoing procedure */
      pFlash.ProcedureOnGoing = FLASH_TYPENONE;
    }

    /* User callback */
    HAL_FLASH_EndOfOperationCallback(param);
  }

  if (pFlash.ProcedureOnGoing == FLASH_TYPENONE)
  {
    /* Disable End of Operation and Error interrupts */
    __HAL_FLASH_DISABLE_IT(FLASH_IT_EOP | FLASH_IT_OPERR);

    /* Process Unlocked */
    __HAL_UNLOCK(&pFlash);
  }
}


/**
  * @brief  FLASH end of operation interrupt callback.
  * @param  ReturnValue The value saved in this parameter depends on the ongoing procedure
  *                  Mass Erase: 0
  *                  Page Erase: Page which has been erased
  *                  Program: Address which was selected for data program
  * @retval None
  */
__weak void HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(ReturnValue);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_FLASH_EndOfOperationCallback could be implemented in the user file
   */
}


/**
  * @brief  FLASH operation error interrupt callback.
  * @param  ReturnValue The value saved in this parameter depends on the ongoing procedure
  *                 Mass Erase: 0
  *                 Page Erase: Page number which returned an error
  *                 Program: Address which was selected for data program
  * @retval None
  */
__weak void HAL_FLASH_OperationErrorCallback(uint32_t ReturnValue)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(ReturnValue);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_FLASH_OperationErrorCallback could be implemented in the user file
   */
}

/**
  * @}
  */

/** @defgroup FLASH_Exported_Functions_Group2 Peripheral Control functions
 *  @brief   Management functions
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
  * @brief  Unlock the FLASH control register access.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_Unlock(void)
{
  HAL_StatusTypeDef status = HAL_OK;

  if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0x00u)
  {
    /* Authorize the FLASH Registers access */
    WRITE_REG(FLASH->KEYR, FLASH_KEY1);
    WRITE_REG(FLASH->KEYR, FLASH_KEY2);

    /* verify Flash is unlock */
    if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0x00u)
    {
      status = HAL_ERROR;
    }
  }

  return status;
}


/**
  * @brief  Lock the FLASH control register access.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_Lock(void)
{
  HAL_StatusTypeDef status = HAL_ERROR;

  /* Set the LOCK Bit to lock the FLASH Registers access */
  SET_BIT(FLASH->CR, FLASH_CR_LOCK);

  /* verify Flash is lock */
  if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0x00u)
  {
    status = HAL_OK;
  }

  return status;
}


/**
  * @brief  Unlock the FLASH Option Bytes Registers access.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_OB_Unlock(void)
{
  HAL_StatusTypeDef status = HAL_ERROR;

  if (READ_BIT(FLASH->CR, FLASH_CR_OPTLOCK) != 0x00u)
  {
    /* Authorizes the Option Byte register programming */
    WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY1);
    WRITE_REG(FLASH->OPTKEYR, FLASH_OPTKEY2);

    /* verify option bytes are unlock */
    if (READ_BIT(FLASH->CR, FLASH_CR_OPTLOCK) == 0x00u)
    {
      status = HAL_OK;
    }
  }

  return status;
}


/**
  * @brief  Lock the FLASH Option Bytes Registers access.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_OB_Lock(void)
{
  HAL_StatusTypeDef status = HAL_ERROR;

  /* Set the OPTLOCK Bit to lock the FLASH Option Byte Registers access */
  SET_BIT(FLASH->CR, FLASH_CR_OPTLOCK);

  /* verify option bytes are lock */
  if (READ_BIT(FLASH->CR, FLASH_CR_OPTLOCK) != 0x00u)
  {
    status = HAL_OK;
  }

  return status;
}


/**
  * @brief  Launch the option byte loading.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_OB_Launch(void)
{
  /* Set the bit to force the option byte reloading */
  SET_BIT(FLASH->CR, FLASH_CR_OBL_LAUNCH);

  /* We should not reach here : Option byte launch generates Option byte reset
     so return error */
  return HAL_ERROR;
}

/**
  * @}
  */

/** @defgroup FLASH_Exported_Functions_Group3 Peripheral State and Errors functions
 *  @brief   Peripheral Errors functions
 *
@verbatim
 ===============================================================================
                ##### Peripheral Errors functions #####
 ===============================================================================
    [..]
    This subsection permits to get in run-time Errors of the FLASH peripheral.

@endverbatim
  * @{
  */

/**
  * @brief  Get the specific FLASH error flag.
  * @retval FLASH_ErrorCode: The returned value can be:
  *            @arg HAL_FLASH_ERROR_RD: FLASH Read Protection error flag (PCROP)(*)
  *            @arg HAL_FLASH_ERROR_PGS: FLASH Programming Sequence error flag
  *            @arg HAL_FLASH_ERROR_PGP: FLASH Programming Parallelism error flag
  *            @arg HAL_FLASH_ERROR_PGA: FLASH Programming Alignment error flag
  *            @arg HAL_FLASH_ERROR_WRP: FLASH Write protected error flag
  *            @arg HAL_FLASH_ERROR_OPERATION: FLASH operation Error flag
  *            @arg HAL_FLASH_ERROR_NONE: No error set
  *            @arg HAL_FLASH_ERROR_OP: FLASH Operation error
  *            @arg HAL_FLASH_ERROR_PROG: FLASH Programming error
  *            @arg HAL_FLASH_ERROR_WRP: FLASH Write protection error
  *            @arg HAL_FLASH_ERROR_PGA: FLASH Programming alignment error
  *            @arg HAL_FLASH_ERROR_SIZ: FLASH Size error
  *            @arg HAL_FLASH_ERROR_PGS: FLASH Programming sequence error
  *            @arg HAL_FLASH_ERROR_MIS: FLASH Fast programming data miss error
  *            @arg HAL_FLASH_ERROR_FAST: FLASH Fast programming error
  *            @arg HAL_FLASH_ERROR_OPTV: FLASH Option validity error
  *            @arg HAL_FLASH_ERROR_ECCC: FLASH on ECC error have been detected and corrected
  * @note (*) availability depends on devices
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

/* Private functions ---------------------------------------------------------*/

/** @addtogroup FLASH_Private_Functions
  * @{
  */

/**
  * @brief  Wait for a FLASH operation to complete.
  * @param  Timeout maximum flash operation timeout
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef FLASH_WaitForLastOperation(uint32_t Timeout)
{
  uint32_t error;
  /* Wait for the FLASH operation to complete by polling on BUSY flag to be reset.
     Even if the FLASH operation fails, the BUSY flag will be reset and an error
     flag will be set */
  uint32_t timeout = HAL_GetTick() + Timeout;

  /* Wait if any operation is ongoing */
  while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != 0x00u)
  {
    if (Timeout != HAL_MAX_DELAY)
    {
      if (HAL_GetTick() >= timeout)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /* check flash errors. Only ECC correction can be checked here as ECCD
      generates NMI */
  error = (FLASH->SR & FLASH_FLAG_SR_ERROR);
  error |= (FLASH->ECCR & FLASH_FLAG_ECCC);
  if (error != 0x00u)
  {
    /*Save the error code*/
    pFlash.ErrorCode |= error;

    /* clear error flags */
    FLASH->SR = FLASH_FLAG_SR_ERROR;
    FLASH->ECCR |= FLASH_FLAG_ECCC;

    return HAL_ERROR;
  }

  /* Wait for control register to be written */
  timeout = HAL_GetTick() + Timeout;

  while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_CFGBSY) != 0x00u)
  {
    if (Timeout != HAL_MAX_DELAY)
    {
      if (HAL_GetTick() >= timeout)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  return HAL_OK;
}


/**
  * @brief  Program double-word (64-bit) at a specified address.
  * @param  Address specifies the address to be programmed.
  * @param  Data specifies the data to be programmed.
  * @retval None
  */
static void FLASH_Program_DoubleWord(uint32_t Address, uint64_t Data)
{
  /* Set PG bit */
  SET_BIT(FLASH->CR, FLASH_CR_PG);

  /* Program the double word */
  *(uint32_t *)Address = (uint32_t)Data;
  *(uint32_t *)(Address + 4u) = (uint32_t)(Data >> 32u);
}


/**
  * @brief  Fast program a 32 row double-word (64-bit) at a specified address.
  * @param  Address specifies the address to be programmed.
  * @param  DataAddress specifies the address where the data are stored.
  * @retval None
  */
static __RAM_FUNC void FLASH_Program_Fast(uint32_t Address, uint32_t DataAddress)
{
  uint8_t index = 0;
  uint32_t dest = Address;
  uint32_t src = DataAddress;
  uint32_t primask_bit;

  /* Set FSTPG bit */
  SET_BIT(FLASH->CR, FLASH_CR_FSTPG);

  /* Enter critical section: row programming should not be longer than 7 ms */
  primask_bit = __get_PRIMASK();
  __disable_irq();

  /* Fast Program : 64 words */
  while (index < 64u)
  {
    *(uint32_t *)dest = *(uint32_t *)src;
    src += 4u;
    dest += 4u;
    index++;
  }

  /* wait for BSY1 in order to be sure that flash operation is ended befoire
     allowing prefetch in flash. Timeout does not return status, as it will
     be anyway done later */
  while((FLASH->SR & FLASH_SR_BSY1) != 0x00u)
  {
  }

  /* Exit critical section: restore previous priority mask */
  __set_PRIMASK(primask_bit);
}

/**
  * @}
  */

#endif /* HAL_FLASH_MODULE_ENABLED */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
