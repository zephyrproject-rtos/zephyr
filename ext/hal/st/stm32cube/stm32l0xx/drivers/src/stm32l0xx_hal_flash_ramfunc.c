/**
  ******************************************************************************
  * @file    stm32l0xx_hal_flash_ramfunc.c
  * @author  MCD Application Team
  * @brief   FLASH RAMFUNC driver.
  *          This file provides a Flash firmware functions which should be 
  *          executed from internal SRAM
  *
  *  @verbatim

    *** ARM Compiler ***
    --------------------
    [..] RAM functions are defined using the toolchain options. 
         Functions that are be executed in RAM should reside in a separate
         source module. Using the 'Options for File' dialog you can simply change
         the 'Code / Const' area of a module to a memory space in physical RAM.
         Available memory areas are declared in the 'Target' tab of the 
         Options for Target' dialog.

    *** ICCARM Compiler ***
    -----------------------
    [..] RAM functions are defined using a specific toolchain keyword "__ramfunc".

    *** GNU Compiler ***
    --------------------
    [..] RAM functions are defined using a specific toolchain attribute
         "__attribute__((section(".RamFunc")))".

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
#include "stm32l0xx_hal.h"

/** @addtogroup STM32L0xx_HAL_Driver
  * @{
  */

#ifdef HAL_FLASH_MODULE_ENABLED

/** @addtogroup FLASH
  * @{
  */
/** @addtogroup FLASH_Private_Variables
 * @{
 */
extern FLASH_ProcessTypeDef pFlash;
/**
  * @}
  */

/**
  * @}
  */
  
/** @defgroup FLASH_RAMFUNC FLASH_RAMFUNC
  * @brief FLASH functions executed from RAM
  * @{
  */ 


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @defgroup FLASH_RAMFUNC_Private_Functions FLASH RAM Private Functions
 * @{
 */

static __RAM_FUNC FLASHRAM_WaitForLastOperation(uint32_t Timeout);
static __RAM_FUNC FLASHRAM_SetErrorCode(void);

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/
 
/** @defgroup FLASH_RAMFUNC_Exported_Functions FLASH RAM Exported Functions
 *
@verbatim  
 ===============================================================================
                      ##### ramfunc functions #####
 ===============================================================================  
    [..]
    This subsection provides a set of functions that should be executed from RAM 
    transfers.

@endverbatim
  * @{
  */ 

/** @defgroup FLASH_RAMFUNC_Exported_Functions_Group1 Peripheral features functions 
  * @{
  */  

/**
  * @brief  Enable  the power down mode during RUN mode.
  * @note  This function can be used only when the user code is running from Internal SRAM.
  * @retval HAL status
  */
__RAM_FUNC HAL_FLASHEx_EnableRunPowerDown(void)
{
  /* Enable the Power Down in Run mode*/
  __HAL_FLASH_POWER_DOWN_ENABLE();

  return HAL_OK;
}

/**
  * @brief  Disable the power down mode during RUN mode.
  * @note  This function can be used only when the user code is running from Internal SRAM.
  * @retval HAL status
  */
__RAM_FUNC HAL_FLASHEx_DisableRunPowerDown(void)
{
  /* Disable the Power Down in Run mode*/
  __HAL_FLASH_POWER_DOWN_DISABLE();

  return HAL_OK;  
}

/**
  * @}
  */

/** @defgroup FLASH_RAMFUNC_Exported_Functions_Group2 Programming and erasing operation functions 
 *
@verbatim  
@endverbatim
  * @{
  */

#if defined(FLASH_PECR_PARALLBANK)
/**
  * @brief  Erases a specified 2 pages in program memory in parallel.
  * @note   This function can be used only for STM32L07xxx/STM32L08xxx  devices.
  *         To correctly run this function, the @ref HAL_FLASH_Unlock() function
  *         must be called before.
  *         Call the @ref HAL_FLASH_Lock() to disable the flash memory access 
  *        (recommended to protect the FLASH memory against possible unwanted operation).
  * @param  Page_Address1: The page address in program memory to be erased in 
  *         the first Bank (BANK1). This parameter should be between FLASH_BASE
  *         and FLASH_BANK1_END.
  * @param  Page_Address2: The page address in program memory to be erased in 
  *         the second Bank (BANK2). This parameter should be between FLASH_BANK2_BASE
  *         and FLASH_BANK2_END.
  * @note   A Page is erased in the Program memory only if the address to load 
  *         is the start address of a page (multiple of @ref FLASH_PAGE_SIZE bytes).
  * @retval HAL status
  */
__RAM_FUNC HAL_FLASHEx_EraseParallelPage(uint32_t Page_Address1, uint32_t Page_Address2)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Wait for last operation to be completed */
  status = FLASHRAM_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {
    /* Proceed to erase the page */
    SET_BIT(FLASH->PECR, FLASH_PECR_PARALLBANK);
    SET_BIT(FLASH->PECR, FLASH_PECR_ERASE);
    SET_BIT(FLASH->PECR, FLASH_PECR_PROG);
  
    /* Write 00000000h to the first word of the first program page to erase */
    *(__IO uint32_t *)Page_Address1 = 0x00000000U;
    /* Write 00000000h to the first word of the second program page to erase */    
    *(__IO uint32_t *)Page_Address2 = 0x00000000U;
 
    /* Wait for last operation to be completed */
    status = FLASHRAM_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

    /* If the erase operation is completed, disable the ERASE, PROG and PARALLBANK bits */
    CLEAR_BIT(FLASH->PECR, FLASH_PECR_PROG);
    CLEAR_BIT(FLASH->PECR, FLASH_PECR_ERASE);
    CLEAR_BIT(FLASH->PECR, FLASH_PECR_PARALLBANK);
  }     
  /* Return the Erase Status */
  return status;
}

/**
  * @brief  Program 2 half pages in program memory in parallel (half page size is 16 Words).
  * @note   This function can be used only for STM32L07xxx/STM32L08xxx  devices.
  * @param  Address1: specifies the first address to be written in the first bank 
  *        (BANK1). This parameter should be between FLASH_BASE and (FLASH_BANK1_END - FLASH_PAGE_SIZE).
  * @param  pBuffer1: pointer to the buffer  containing the data to be  written 
  *         to the first half page in the first bank.
  * @param  Address2: specifies the second address to be written in the second bank
  *        (BANK2). This parameter should be between FLASH_BANK2_BASE and (FLASH_BANK2_END - FLASH_PAGE_SIZE).
  * @param  pBuffer2: pointer to the buffer containing the data to be  written 
  *         to the second half page in the second bank.
  * @note   To correctly run this function, the @ref HAL_FLASH_Unlock() function
  *         must be called before.
  *         Call the @ref HAL_FLASH_Lock() to disable the flash memory access  
  *         (recommended to protect the FLASH memory against possible unwanted operation).
  * @note   Half page write is possible only from SRAM.
  * @note   A half page is written to the program memory only if the first 
  *         address to load is the start address of a half page (multiple of 64 
  *         bytes) and the 15 remaining words to load are in the same half page.
  * @note   During the Program memory half page write all read operations are 
  *         forbidden (this includes DMA read operations and debugger read 
  *         operations such as breakpoints, periodic updates, etc.).
  * @note   If a PGAERR is set during a Program memory half page write, the 
  *         complete write operation is aborted. Software should then reset the 
  *         FPRG and PROG/DATA bits and restart the write operation from the 
  *         beginning.
  * @retval HAL status
  */
__RAM_FUNC HAL_FLASHEx_ProgramParallelHalfPage(uint32_t Address1, uint32_t* pBuffer1, uint32_t Address2, uint32_t* pBuffer2)
{
  uint32_t count = 0U; 
  HAL_StatusTypeDef status = HAL_OK;

  /* Wait for last operation to be completed */
  status = FLASHRAM_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {
    /* Proceed to program the new half page */
    SET_BIT(FLASH->PECR, FLASH_PECR_PARALLBANK);
    SET_BIT(FLASH->PECR, FLASH_PECR_FPRG);
    SET_BIT(FLASH->PECR, FLASH_PECR_PROG);

    /* Wait for last operation to be completed */
    status = FLASHRAM_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    if(status == HAL_OK)
    {
      /* Disable all IRQs */
      __disable_irq();

      /* Write the first half page directly with 16 different words */
      while(count < 16U)
      {
        /* Address1 doesn't need to be increased */ 
        *(__IO uint32_t*) Address1 = *pBuffer1;
        pBuffer1++;
        count ++;  
      }

      /* Write the second half page directly with 16 different words */
      count = 0U;
      while(count < 16U)
      {
        /* Address2 doesn't need to be increased */ 
        *(__IO uint32_t*) Address2 = *pBuffer2;
        pBuffer2++;
        count ++;  
      }

      /* Enable IRQs */
      __enable_irq();

      /* Wait for last operation to be completed */
      status = FLASHRAM_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    }

    /* if the write operation is completed, disable the PROG, FPRG and PARALLBANK bits */
    CLEAR_BIT(FLASH->PECR, FLASH_PECR_PROG);
    CLEAR_BIT(FLASH->PECR, FLASH_PECR_FPRG);
    CLEAR_BIT(FLASH->PECR, FLASH_PECR_PARALLBANK);
  }

  /* Return the Write Status */
  return status;
}
#endif /* FLASH_PECR_PARALLBANK */

/**
  * @brief  Program a half page in program memory.
  * @param  Address: specifies the address to be written.
  * @param  pBuffer: pointer to the buffer  containing the data to be  written to 
  *         the half page.
  * @note   To correctly run this function, the @ref HAL_FLASH_Unlock() function
  *         must be called before.
  *         Call the @ref HAL_FLASH_Lock() to disable the flash memory access  
  *         (recommended to protect the FLASH memory against possible unwanted operation)
  * @note   Half page write is possible only from SRAM.
  * @note   A half page is written to the program memory only if the first 
  *         address to load is the start address of a half page (multiple of 64 
  *         bytes) and the 15 remaining words to load are in the same half page.
  * @note   During the Program memory half page write all read operations are 
  *         forbidden (this includes DMA read operations and debugger read 
  *         operations such as breakpoints, periodic updates, etc.).
  * @note   If a PGAERR is set during a Program memory half page write, the 
  *         complete write operation is aborted. Software should then reset the 
  *         FPRG and PROG/DATA bits and restart the write operation from the 
  *         beginning.
  * @retval HAL status
  */
__RAM_FUNC HAL_FLASHEx_HalfPageProgram(uint32_t Address, uint32_t* pBuffer)
{
  uint32_t count = 0U; 
  HAL_StatusTypeDef status = HAL_OK;

  /* Wait for last operation to be completed */
  status = FLASHRAM_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {
    /* Proceed to program the new half page */
    SET_BIT(FLASH->PECR, FLASH_PECR_FPRG);
    SET_BIT(FLASH->PECR, FLASH_PECR_PROG);
    
    /* Disable all IRQs */
    __disable_irq();

    /* Write one half page directly with 16 different words */
    while(count < 16U)
    {
      /* Address doesn't need to be increased */ 
      *(__IO uint32_t*) Address = *pBuffer;
      pBuffer++;
      count ++;  
    }

    /* Enable IRQs */
    __enable_irq();

    /* Wait for last operation to be completed */
    status = FLASHRAM_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
 
    /* If the write operation is completed, disable the PROG and FPRG bits */
    CLEAR_BIT(FLASH->PECR, FLASH_PECR_PROG);
    CLEAR_BIT(FLASH->PECR, FLASH_PECR_FPRG);
  }

  /* Return the Write Status */
  return status;
}

/**
  * @}
  */

/** @defgroup FLASH_RAMFUNC_Exported_Functions_Group3 Peripheral errors functions 
 *  @brief    Peripheral errors functions 
 *
@verbatim   
 ===============================================================================
                      ##### Peripheral errors functions #####
 ===============================================================================  
    [..]
    This subsection permit to get in run-time errors of  the FLASH peripheral.

@endverbatim
  * @{
  */

/**
  * @brief  Get the specific FLASH errors flag.
  * @param  Error pointer is the error value. It can be a mixed of:
  *            @arg @ref HAL_FLASH_ERROR_RD      FLASH Read Protection error flag (PCROP)
  *            @arg @ref HAL_FLASH_ERROR_SIZE    FLASH Programming Parallelism error flag  
  *            @arg @ref HAL_FLASH_ERROR_PGA     FLASH Programming Alignment error flag
  *            @arg @ref HAL_FLASH_ERROR_WRP     FLASH Write protected error flag
  *            @arg @ref HAL_FLASH_ERROR_OPTV    FLASH Option valid error flag 
  *            @arg @ref HAL_FLASH_ERROR_FWWERR  FLASH Write or Erase operation aborted
  *            @arg @ref HAL_FLASH_ERROR_NOTZERO FLASH Write operation is done in a not-erased region
  * @retval HAL Status
  */
__RAM_FUNC HAL_FLASHEx_GetError(uint32_t * Error)
{ 
  *Error = pFlash.ErrorCode;
  return HAL_OK;  
}

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup FLASH_RAMFUNC_Private_Functions
  * @{
  */ 

/**
  * @brief  Set the specific FLASH error flag.
  * @retval HAL Status
  */
static __RAM_FUNC FLASHRAM_SetErrorCode(void)
{
  uint32_t flags = 0;
  
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
  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_SIZERR))
  { 
    pFlash.ErrorCode |= HAL_FLASH_ERROR_SIZE;
    flags |= FLASH_FLAG_SIZERR;
  }
  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERR))
  {
    /* WARNING : On the first cut of STM32L031xx and STM32L041xx devices,
     *           (RefID = 0x1000) the FLASH_FLAG_OPTVERR bit was not behaving
     *           as expected. If the user run an application using the first
     *           cut of the STM32L031xx device or the first cut of the STM32L041xx
     *           device, this error should be ignored. The revId of the device
     *           can be retrieved via the HAL_GetREVID() function.
     *
     */
    pFlash.ErrorCode |= HAL_FLASH_ERROR_OPTV;
    flags |= FLASH_FLAG_OPTVERR;
  }

  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_RDERR))
  {
    pFlash.ErrorCode |= HAL_FLASH_ERROR_RD;
    flags |= FLASH_FLAG_RDERR;
  }
  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_FWWERR))
  { 
    pFlash.ErrorCode |= HAL_FLASH_ERROR_FWWERR;
    flags |= HAL_FLASH_ERROR_FWWERR;
  }
  if(__HAL_FLASH_GET_FLAG(FLASH_FLAG_NOTZEROERR))
  { 
    pFlash.ErrorCode |= HAL_FLASH_ERROR_NOTZERO;
    flags |= FLASH_FLAG_NOTZEROERR;
  }

  /* Clear FLASH error pending bits */
  __HAL_FLASH_CLEAR_FLAG(flags);

  return HAL_OK;
}  

/**
  * @brief  Wait for a FLASH operation to complete.
  * @param  Timeout: maximum flash operationtimeout
  * @retval HAL status
  */
static __RAM_FUNC  FLASHRAM_WaitForLastOperation(uint32_t Timeout)
{ 
    /* Wait for the FLASH operation to complete by polling on BUSY flag to be reset.
       Even if the FLASH operation fails, the BUSY flag will be reset and an error
       flag will be set */
       
    while(__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) && (Timeout != 0x00U)) 
    { 
      Timeout--;
    }
    
    if(Timeout == 0x00U)
    {
      return HAL_TIMEOUT;
    }
    
  /* Check FLASH End of Operation flag  */
  if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP))
  {
    /* Clear FLASH End of Operation pending bit */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);
  }
  
  if( __HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR)     || 
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_PGAERR)     || 
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_SIZERR)     || 
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_OPTVERR)    || 
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_RDERR)      || 
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_FWWERR)     || 
      __HAL_FLASH_GET_FLAG(FLASH_FLAG_NOTZEROERR) )
  {
    /*Save the error code*/

    /* WARNING : On the first cut of STM32L031xx and STM32L041xx devices,
     *           (RefID = 0x1000) the FLASH_FLAG_OPTVERR bit was not behaving
     *           as expected. If the user run an application using the first
     *           cut of the STM32L031xx device or the first cut of the STM32L041xx
     *           device, this error should be ignored. The revId of the device
     *           can be retrieved via the HAL_GetREVID() function.
     *
     */
    FLASHRAM_SetErrorCode();
    return HAL_ERROR;
  }

  /* There is no error flag set */
  return HAL_OK;
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
