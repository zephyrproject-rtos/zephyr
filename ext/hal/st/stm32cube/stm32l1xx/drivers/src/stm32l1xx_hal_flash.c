/**
  ******************************************************************************
  * @file    stm32l1xx_hal_flash.c
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
      memory of all STM32L1xx devices.
    
      (#) FLASH Memory I/O Programming functions: this group includes all needed
          functions to erase and program the main memory:
        (++) Lock and Unlock the FLASH interface
        (++) Erase function: Erase page
        (++) Program functions: Fast Word and Half Page(should be 
        executed from internal SRAM).
  
      (#) DATA EEPROM Programming functions: this group includes all 
          needed functions to erase and program the DATA EEPROM memory:
        (++) Lock and Unlock the DATA EEPROM interface.
        (++) Erase function: Erase Byte, erase HalfWord, erase Word, erase 
             Double Word (should be executed from internal SRAM).
        (++) Program functions: Fast Program Byte, Fast Program Half-Word, 
             FastProgramWord, Program Byte, Program Half-Word, 
             Program Word and Program Double-Word (should be executed 
             from internal SRAM).

      (#) FLASH Option Bytes Programming functions: this group includes all needed
          functions to manage the Option Bytes:
        (++) Lock and Unlock the Option Bytes
        (++) Set/Reset the write protection
        (++) Set the Read protection Level
        (++) Program the user Option Bytes
        (++) Launch the Option Bytes loader
        (++) Set/Get the Read protection Level.
        (++) Set/Get the BOR level.
        (++) Get the Write protection.
        (++) Get the user option bytes.
    
      (#) Interrupts and flags management functions : this group 
          includes all needed functions to:
        (++) Handle FLASH interrupts
        (++) Wait for last FLASH operation according to its status
        (++) Get error flag status

    (#) FLASH Interface configuration functions: this group includes 
      the management of following features:
      (++) Enable/Disable the RUN PowerDown mode.
      (++) Enable/Disable the SLEEP PowerDown mode.  
  
    (#) FLASH Peripheral State methods: this group includes 
      the management of following features:
      (++) Wait for the FLASH operation
      (++)  Get the specific FLASH error flag
    
  [..] In addition to these function, this driver includes a set of macros allowing
       to handle the following operations:
      
      (+) Set/Get the latency
      (+) Enable/Disable the prefetch buffer
      (+) Enable/Disable the 64 bit Read Access.
      (+) Enable/Disable the Flash power-down
      (+) Enable/Disable the FLASH interrupts
      (+) Monitor the FLASH flags status
          
                 ##### Programming operation functions #####
  ===============================================================================  
     [..]
     This subsection provides a set of functions allowing to manage the FLASH 
     program operations.
  
    [..] The FLASH Memory Programming functions, includes the following functions:
     (+) HAL_FLASH_Unlock(void);
     (+) HAL_FLASH_Lock(void);
     (+) HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t Data)
     (+) HAL_FLASH_Program_IT(uint32_t TypeProgram, uint32_t Address, uint32_t Data)
    
     [..] Any operation of erase or program should follow these steps:
     (#) Call the HAL_FLASH_Unlock() function to enable the flash control register and 
         program memory access.
     (#) Call the desired function to erase page or program data.
     (#) Call the HAL_FLASH_Lock() to disable the flash program memory access 
        (recommended to protect the FLASH memory against possible unwanted operation).
  
               ##### Option Bytes Programming functions ##### 
   ==============================================================================  
  
     [..] The FLASH_Option Bytes Programming_functions, includes the following functions:
     (+) HAL_FLASH_OB_Unlock(void);
     (+) HAL_FLASH_OB_Lock(void);
     (+) HAL_FLASH_OB_Launch(void);
     (+) HAL_FLASHEx_OBProgram(FLASH_OBProgramInitTypeDef *pOBInit);
     (+) HAL_FLASHEx_OBGetConfig(FLASH_OBProgramInitTypeDef *pOBInit);
    
     [..] Any operation of erase or program should follow these steps:
     (#) Call the HAL_FLASH_OB_Unlock() function to enable the Flash option control 
         register access.
     (#) Call the following functions to program the desired option bytes.
         (++) HAL_FLASHEx_OBProgram(FLASH_OBProgramInitTypeDef *pOBInit);      
     (#) Once all needed option bytes to be programmed are correctly written, call the
         HAL_FLASH_OB_Launch(void) function to launch the Option Bytes programming process.
     (#) Call the HAL_FLASH_OB_Lock() to disable the Flash option control register access (recommended
         to protect the option Bytes against possible unwanted operations).
  
    [..] Proprietary code Read Out Protection (PcROP):    
    (#) The PcROP sector is selected by using the same option bytes as the Write
        protection. As a result, these 2 options are exclusive each other.
    (#) To activate PCROP mode for Flash sectors(s), you need to follow the sequence below:
        (++) Use this function HAL_FLASHEx_AdvOBProgram with PCROPState = OB_PCROP_STATE_ENABLE.

  @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"

/** @addtogroup STM32L1xx_HAL_Driver
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
  * @brief  Program word at a specified address
  * @note   To correctly run this function, the HAL_FLASH_Unlock() function
  *         must be called before.
  *         Call the HAL_FLASH_Lock() to disable the flash memory access
  *         (recommended to protect the FLASH memory against possible unwanted operation).
  *
  * @param  TypeProgram   Indicate the way to program at a specified address.
  *                       This parameter can be a value of @ref FLASH_Type_Program
  * @param  Address       Specifie the address to be programmed.
  * @param  Data          Specifie the data to be programmed
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t Data)
{
  HAL_StatusTypeDef status = HAL_ERROR;
  
  /* Process Locked */
  __HAL_LOCK(&pFlash);

  /* Check the parameters */
  assert_param(IS_FLASH_TYPEPROGRAM(TypeProgram));
  assert_param(IS_FLASH_PROGRAM_ADDRESS(Address));

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {
    /* Clean the error context */
    pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    /*Program word (32-bit) at a specified address.*/
    *(__IO uint32_t *)Address = Data;

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  }

  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  return status;
}

/**
  * @brief   Program word at a specified address  with interrupt enabled.
  *
  * @param  TypeProgram  Indicate the way to program at a specified address.
  *                      This parameter can be a value of @ref FLASH_Type_Program
  * @param  Address      Specifie the address to be programmed.
  * @param  Data         Specifie the data to be programmed
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_Program_IT(uint32_t TypeProgram, uint32_t Address, uint32_t Data)
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
  pFlash.ProcedureOnGoing = FLASH_PROC_PROGRAM;
  /* Clean the error context */
  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

  if(TypeProgram == FLASH_TYPEPROGRAM_WORD)
  {
    /* Program word (32-bit) at a specified address. */
    *(__IO uint32_t *)Address = Data;
  }
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
  if( __HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR)     || 
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_PGAERR)     || 
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_SIZERR)     || 
#if defined(FLASH_SR_RDERR)
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_RDERR)      || 
#endif /* FLASH_SR_RDERR */
#if defined(FLASH_SR_OPTVERRUSR)
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERRUSR) || 
#endif /* FLASH_SR_OPTVERRUSR */
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERR) )
  {
    if(pFlash.ProcedureOnGoing == FLASH_PROC_PAGEERASE)
    {
      /* Return the faulty sector */
      addresstmp = pFlash.Page;
      pFlash.Page = 0xFFFFFFFFU;
    }
    else
    {
      /* Return the faulty address */
      addresstmp = pFlash.Address;
    }
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
        pFlash.NbPagesToErase--;

        /* Check if there are still pages to erase */
        if(pFlash.NbPagesToErase != 0U)
        {
          addresstmp = pFlash.Page;
          /*Indicate user which sector has been erased */
          HAL_FLASH_EndOfOperationCallback(addresstmp);

          /*Increment sector number*/
          addresstmp = pFlash.Page + FLASH_PAGE_SIZE;
          pFlash.Page = addresstmp;

          /* If the erase operation is completed, disable the ERASE Bit */
          CLEAR_BIT(FLASH->PECR, FLASH_PECR_ERASE);

          FLASH_PageErase(addresstmp);
        }
        else
        {
          /* No more pages to Erase, user callback can be called. */
          /* Reset Sector and stop Erase pages procedure */
          pFlash.Page = addresstmp = 0xFFFFFFFFU;
          pFlash.ProcedureOnGoing = FLASH_PROC_NONE;
          /* FLASH EOP interrupt user callback */
          HAL_FLASH_EndOfOperationCallback(addresstmp);
        }
      }
      else
      {
          /* If the program operation is completed, disable the PROG Bit */
          CLEAR_BIT(FLASH->PECR, FLASH_PECR_PROG);

          /* Program ended. Return the selected address */
          /* FLASH EOP interrupt user callback */
          HAL_FLASH_EndOfOperationCallback(pFlash.Address);
        
          /* Reset Address and stop Program procedure */
          pFlash.Address = 0xFFFFFFFFU;
          pFlash.ProcedureOnGoing = FLASH_PROC_NONE;
      }
    }
  }
  

  if(pFlash.ProcedureOnGoing == FLASH_PROC_NONE)
  {
    /* Operation is completed, disable the PROG and ERASE */
    CLEAR_BIT(FLASH->PECR, (FLASH_PECR_ERASE | FLASH_PECR_PROG));

    /* Disable End of FLASH Operation and Error source interrupts */
    __HAL_FLASH_DISABLE_IT(FLASH_IT_EOP | FLASH_IT_ERR);

    /* Process Unlocked */
    __HAL_UNLOCK(&pFlash);
  }
}

/**
  * @brief  FLASH end of operation interrupt callback
  * @param  ReturnValue: The value saved in this parameter depends on the ongoing procedure
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
  * @param  ReturnValue: The value saved in this parameter depends on the ongoing procedure
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
  if (HAL_IS_BIT_SET(FLASH->PECR, FLASH_PECR_PRGLOCK))
  {
    /* Unlocking FLASH_PECR register access*/
    if(HAL_IS_BIT_SET(FLASH->PECR, FLASH_PECR_PELOCK))
    {  
      WRITE_REG(FLASH->PEKEYR, FLASH_PEKEY1);
      WRITE_REG(FLASH->PEKEYR, FLASH_PEKEY2);
    }
    
    /* Unlocking the program memory access */
    WRITE_REG(FLASH->PRGKEYR, FLASH_PRGKEY1);
    WRITE_REG(FLASH->PRGKEYR, FLASH_PRGKEY2);  
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
  /* Set the PRGLOCK Bit to lock the FLASH Registers access */
  SET_BIT(FLASH->PECR, FLASH_PECR_PRGLOCK);
  
  return HAL_OK;  
}

/**
  * @brief  Unlock the FLASH Option Control Registers access.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASH_OB_Unlock(void)
{
  if(HAL_IS_BIT_SET(FLASH->PECR, FLASH_PECR_OPTLOCK))
  {
    /* Unlocking FLASH_PECR register access*/
    if(HAL_IS_BIT_SET(FLASH->PECR, FLASH_PECR_PELOCK))
    {  
      /* Unlocking FLASH_PECR register access*/
      WRITE_REG(FLASH->PEKEYR, FLASH_PEKEY1);
      WRITE_REG(FLASH->PEKEYR, FLASH_PEKEY2);
    }

    /* Unlocking the option bytes block access */
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
  /* Set the OPTLOCK Bit to lock the option bytes block access */
  SET_BIT(FLASH->PECR, FLASH_PECR_OPTLOCK);
  
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
  SET_BIT(FLASH->PECR, FLASH_PECR_OBL_LAUNCH);
  
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
     __HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERR) || 
#if defined(FLASH_SR_RDERR)
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_RDERR) || 
#endif /* FLASH_SR_RDERR */
#if defined(FLASH_SR_OPTVERRUSR)
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERRUSR) || 
#endif /* FLASH_SR_OPTVERRUSR */
     __HAL_FLASH_GET_FLAG(FLASH_FLAG_PGAERR))
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
  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_PGAERR))
  {
    pFlash.ErrorCode |= HAL_FLASH_ERROR_PGA;
    flags |= FLASH_FLAG_PGAERR;
  }
  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERR))
  {
    pFlash.ErrorCode |= HAL_FLASH_ERROR_OPTV;
    flags |= FLASH_FLAG_OPTVERR;
  }

#if defined(FLASH_SR_RDERR)
  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_RDERR))
  {
    pFlash.ErrorCode |= HAL_FLASH_ERROR_RD;
    flags |= FLASH_FLAG_RDERR;
  }
#endif /* FLASH_SR_RDERR */
#if defined(FLASH_SR_OPTVERRUSR)
  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERRUSR))
  {
    pFlash.ErrorCode |= HAL_FLASH_ERROR_OPTVUSR;
    flags |= FLASH_FLAG_OPTVERRUSR;
  }
#endif /* FLASH_SR_OPTVERRUSR */

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
