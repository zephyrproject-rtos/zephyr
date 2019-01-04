/**
  ******************************************************************************
  * @file    stm32l1xx_hal_flash_ex.c
  * @author  MCD Application Team
  * @brief   Extended FLASH HAL module driver.
  *    
  *          This file provides firmware functions to manage the following 
  *          functionalities of the internal FLASH memory:
  *            + FLASH Interface configuration
  *            + FLASH Memory Erasing
  *            + DATA EEPROM Programming/Erasing
  *            + Option Bytes Programming
  *            + Interrupts management
  *    
  @verbatim
  ==============================================================================
               ##### Flash peripheral Extended features  #####
  ==============================================================================
           
  [..] Comparing to other products, the FLASH interface for STM32L1xx
       devices contains the following additional features        
       (+) Erase functions
       (+) DATA_EEPROM memory management
       (+) BOOT option bit configuration       
       (+) PCROP protection for all sectors
   
                      ##### How to use this driver #####
  ==============================================================================
  [..] This driver provides functions to configure and program the FLASH memory 
       of all STM32L1xx. It includes:
       (+) Full DATA_EEPROM erase and program management
       (+) Boot activation
       (+) PCROP protection configuration and control for all pages
  
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

/** @addtogroup FLASH
  * @{
  */
/** @addtogroup FLASH_Private_Variables
 * @{
 */
/* Variables used for Erase pages under interruption*/
extern FLASH_ProcessTypeDef pFlash;
/**
  * @}
  */

/**
  * @}
  */
  
/** @defgroup FLASHEx FLASHEx
  * @brief FLASH HAL Extension module driver
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup FLASHEx_Private_Constants FLASHEx Private Constants
 * @{
 */
/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/** @defgroup FLASHEx_Private_Macros FLASHEx Private Macros
  * @{
  */
/**
  * @}
  */ 

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @defgroup FLASHEx_Private_Functions FLASHEx Private Functions
 * @{
 */
void                      FLASH_PageErase(uint32_t PageAddress);
static HAL_StatusTypeDef  FLASH_OB_WRPConfig(FLASH_OBProgramInitTypeDef *pOBInit, FunctionalState NewState);
static void               FLASH_OB_WRPConfigWRP1OrPCROP1(uint32_t WRP1OrPCROP1, FunctionalState NewState);
#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)    \
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xCA) \
 || defined(STM32L152xD) || defined(STM32L152xDX) || defined(STM32L162xCA) || defined(STM32L162xD)  \
 || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE)
static void               FLASH_OB_WRPConfigWRP2OrPCROP2(uint32_t WRP2OrPCROP2, FunctionalState NewState);
#endif /* STM32L100xC || STM32L151xC || STM32L152xC || (...) || STM32L151xE || STM32L152xE || STM32L162xE */
#if defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xD) || defined(STM32L152xDX) \
 || defined(STM32L162xD) || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE)   \
 || defined(STM32L162xE)
static void               FLASH_OB_WRPConfigWRP3(uint32_t WRP3, FunctionalState NewState);
#endif /* STM32L151xD || STM32L152xD || STM32L162xD || STM32L151xE || STM32L152xE || STM32L162xE */
#if defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE) || defined(STM32L151xDX) \
 || defined(STM32L152xDX) || defined(STM32L162xDX)
static void               FLASH_OB_WRPConfigWRP4(uint32_t WRP4, FunctionalState NewState);
#endif /* STM32L151xE || STM32L152xE || STM32L151xDX || ... */
#if defined(FLASH_OBR_SPRMOD)
static HAL_StatusTypeDef  FLASH_OB_PCROPConfig(FLASH_AdvOBProgramInitTypeDef *pAdvOBInit, FunctionalState NewState);
#endif /* FLASH_OBR_SPRMOD */
#if defined(FLASH_OBR_nRST_BFB2)
static HAL_StatusTypeDef  FLASH_OB_BootConfig(uint8_t OB_BOOT);
#endif /* FLASH_OBR_nRST_BFB2 */
static HAL_StatusTypeDef  FLASH_OB_RDPConfig(uint8_t OB_RDP);
static HAL_StatusTypeDef  FLASH_OB_UserConfig(uint8_t OB_IWDG, uint8_t OB_STOP, uint8_t OB_STDBY);
static HAL_StatusTypeDef  FLASH_OB_BORConfig(uint8_t OB_BOR);
static uint8_t            FLASH_OB_GetRDP(void);
static uint8_t            FLASH_OB_GetUser(void);
static uint8_t            FLASH_OB_GetBOR(void);
static HAL_StatusTypeDef  FLASH_DATAEEPROM_FastProgramByte(uint32_t Address, uint8_t Data);
static HAL_StatusTypeDef  FLASH_DATAEEPROM_FastProgramHalfWord(uint32_t Address, uint16_t Data);
static HAL_StatusTypeDef  FLASH_DATAEEPROM_FastProgramWord(uint32_t Address, uint32_t Data);
static HAL_StatusTypeDef  FLASH_DATAEEPROM_ProgramWord(uint32_t Address, uint32_t Data);
static HAL_StatusTypeDef  FLASH_DATAEEPROM_ProgramHalfWord(uint32_t Address, uint16_t Data);
static HAL_StatusTypeDef  FLASH_DATAEEPROM_ProgramByte(uint32_t Address, uint8_t Data);
/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/
/** @defgroup FLASHEx_Exported_Functions FLASHEx Exported Functions
  * @{
  */

/** @defgroup FLASHEx_Exported_Functions_Group1 FLASHEx Memory Erasing functions
 *  @brief   FLASH Memory Erasing functions
 *
@verbatim   
  ==============================================================================
                ##### FLASH Erasing Programming functions ##### 
  ==============================================================================

    [..] The FLASH Memory Erasing functions, includes the following functions:
    (+) @ref HAL_FLASHEx_Erase: return only when erase has been done
    (+) @ref HAL_FLASHEx_Erase_IT: end of erase is done when @ref HAL_FLASH_EndOfOperationCallback 
        is called with parameter 0xFFFFFFFF

    [..] Any operation of erase should follow these steps:
    (#) Call the @ref HAL_FLASH_Unlock() function to enable the flash control register and 
        program memory access.
    (#) Call the desired function to erase page.
    (#) Call the @ref HAL_FLASH_Lock() to disable the flash program memory access 
       (recommended to protect the FLASH memory against possible unwanted operation).

@endverbatim
  * @{
  */
  
/**
  * @brief  Erase the specified FLASH memory Pages 
  * @note   To correctly run this function, the @ref HAL_FLASH_Unlock() function
  *         must be called before.
  *         Call the @ref HAL_FLASH_Lock() to disable the flash memory access 
  *         (recommended to protect the FLASH memory against possible unwanted operation)
  * @note   For STM32L151xDX/STM32L152xDX/STM32L162xDX, as memory is not continuous between
  *         2 banks, user should perform pages erase by bank only.
  * @param[in]  pEraseInit pointer to an FLASH_EraseInitTypeDef structure that
  *         contains the configuration information for the erasing.
  * 
  * @param[out]  PageError pointer to variable  that
  *         contains the configuration information on faulty page in case of error
  *         (0xFFFFFFFF means that all the pages have been correctly erased)
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *PageError)
{
  HAL_StatusTypeDef status = HAL_ERROR;
  uint32_t address = 0U;
  
  /* Process Locked */
  __HAL_LOCK(&pFlash);

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

  if (status == HAL_OK)
  {
    /*Initialization of PageError variable*/
    *PageError = 0xFFFFFFFFU;

    /* Check the parameters */
    assert_param(IS_NBPAGES(pEraseInit->NbPages));
    assert_param(IS_FLASH_TYPEERASE(pEraseInit->TypeErase));
    assert_param(IS_FLASH_PROGRAM_ADDRESS(pEraseInit->PageAddress));
    assert_param(IS_FLASH_PROGRAM_ADDRESS((pEraseInit->PageAddress & ~(FLASH_PAGE_SIZE - 1U)) + pEraseInit->NbPages * FLASH_PAGE_SIZE - 1U));

#if defined(STM32L151xDX) || defined(STM32L152xDX) || defined(STM32L162xDX)
    /* Check on which bank belongs the 1st address to erase */
    if (pEraseInit->PageAddress < FLASH_BANK2_BASE)
    {
      /* BANK1 */
      /* Check that last page to erase still belongs to BANK1 */
      if (((pEraseInit->PageAddress & ~(FLASH_PAGE_SIZE - 1U)) + pEraseInit->NbPages * FLASH_PAGE_SIZE - 1U) > FLASH_BANK1_END)
      {
        /*  Last page does not belong to BANK1, erase procedure cannot be performed because memory is not
            continuous */
        /* Process Unlocked */
        __HAL_UNLOCK(&pFlash);
        return HAL_ERROR;
      }
    }
    else
    {
      /* BANK2 */
      /* Check that last page to erase still belongs to BANK2 */
      if (((pEraseInit->PageAddress & ~(FLASH_PAGE_SIZE - 1U)) + pEraseInit->NbPages * FLASH_PAGE_SIZE - 1U) > FLASH_BANK2_END)
      {
        /*  Last page does not belong to BANK2, erase procedure cannot be performed because memory is not
            continuous */
        /* Process Unlocked */
        __HAL_UNLOCK(&pFlash);
        return HAL_ERROR;
      }
    }
#endif /* STM32L151xDX || STM32L152xDX || STM32L162xDX */

    /* Erase page by page to be done*/
    for(address = pEraseInit->PageAddress; 
        address < ((pEraseInit->NbPages * FLASH_PAGE_SIZE) + pEraseInit->PageAddress);
        address += FLASH_PAGE_SIZE)
    {
      FLASH_PageErase(address);

      /* Wait for last operation to be completed */
      status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

      /* If the erase operation is completed, disable the ERASE Bit */
      CLEAR_BIT(FLASH->PECR, FLASH_PECR_PROG);
      CLEAR_BIT(FLASH->PECR, FLASH_PECR_ERASE);

      if (status != HAL_OK) 
      {
        /* In case of error, stop erase procedure and return the faulty address */
        *PageError = address;
        break;
      }
    }
  }

  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  return status;
}

/**
  * @brief  Perform a page erase of the specified FLASH memory pages  with interrupt enabled
  * @note   To correctly run this function, the @ref HAL_FLASH_Unlock() function
  *         must be called before.
  *         Call the @ref HAL_FLASH_Lock() to disable the flash memory access 
  *         (recommended to protect the FLASH memory against possible unwanted operation)
  *          End of erase is done when @ref HAL_FLASH_EndOfOperationCallback is called with parameter
  *          0xFFFFFFFF
  * @note   For STM32L151xDX/STM32L152xDX/STM32L162xDX, as memory is not continuous between
  *         2 banks, user should perform pages erase by bank only.
  * @param  pEraseInit pointer to an FLASH_EraseInitTypeDef structure that
  *         contains the configuration information for the erasing.
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASHEx_Erase_IT(FLASH_EraseInitTypeDef *pEraseInit)
{
  HAL_StatusTypeDef status = HAL_ERROR;

  /* If procedure already ongoing, reject the next one */
  if (pFlash.ProcedureOnGoing != FLASH_PROC_NONE)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_NBPAGES(pEraseInit->NbPages));
  assert_param(IS_FLASH_TYPEERASE(pEraseInit->TypeErase));
  assert_param(IS_FLASH_PROGRAM_ADDRESS(pEraseInit->PageAddress));
  assert_param(IS_FLASH_PROGRAM_ADDRESS((pEraseInit->PageAddress & ~(FLASH_PAGE_SIZE - 1U)) + pEraseInit->NbPages * FLASH_PAGE_SIZE - 1U));

  /* Process Locked */
  __HAL_LOCK(&pFlash);

#if defined(STM32L151xDX) || defined(STM32L152xDX) || defined(STM32L162xDX)
    /* Check on which bank belongs the 1st address to erase */
    if (pEraseInit->PageAddress < FLASH_BANK2_BASE)
    {
      /* BANK1 */
      /* Check that last page to erase still belongs to BANK1 */
      if (((pEraseInit->PageAddress & ~(FLASH_PAGE_SIZE - 1U)) + pEraseInit->NbPages * FLASH_PAGE_SIZE - 1U) > FLASH_BANK1_END)
      {
        /*  Last page does not belong to BANK1, erase procedure cannot be performed because memory is not
            continuous */
        /* Process Unlocked */
        __HAL_UNLOCK(&pFlash);
        return HAL_ERROR;
      }
    }
    else
    {
      /* BANK2 */
      /* Check that last page to erase still belongs to BANK2 */
      if (((pEraseInit->PageAddress & ~(FLASH_PAGE_SIZE - 1U)) + pEraseInit->NbPages * FLASH_PAGE_SIZE - 1U) > FLASH_BANK2_END)
      {
        /*  Last page does not belong to BANK2, erase procedure cannot be performed because memory is not
            continuous */
        /* Process Unlocked */
        __HAL_UNLOCK(&pFlash);
        return HAL_ERROR;
      }
    }
#endif /* STM32L151xDX || STM32L152xDX || STM32L162xDX */

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if (status == HAL_OK)
  {
    /* Enable End of FLASH Operation and Error source interrupts */
    __HAL_FLASH_ENABLE_IT(FLASH_IT_EOP | FLASH_IT_ERR);
    
    pFlash.ProcedureOnGoing = FLASH_PROC_PAGEERASE;
    pFlash.NbPagesToErase = pEraseInit->NbPages;
    pFlash.Page = pEraseInit->PageAddress;

    /*Erase 1st page and wait for IT*/
    FLASH_PageErase(pEraseInit->PageAddress);
  }
  else
  {
    /* Process Unlocked */
    __HAL_UNLOCK(&pFlash);
  }

  return status;
}

/**
  * @}
  */

/** @defgroup FLASHEx_Exported_Functions_Group2 Option Bytes Programming functions
 *  @brief   Option Bytes Programming functions
 *
@verbatim   
  ==============================================================================
                ##### Option Bytes Programming functions ##### 
  ==============================================================================  

    [..] Any operation of erase or program should follow these steps:
    (#) Call the @ref HAL_FLASH_OB_Unlock() function to enable the Flash option control 
        register access.
    (#) Call following function to program the desired option bytes.
        (++) @ref HAL_FLASHEx_OBProgram:
         - To Enable/Disable the desired sector write protection.
         - To set the desired read Protection Level.
         - To configure the user option Bytes: IWDG, STOP and the Standby.
         - To Set the BOR level.
    (#) Once all needed option bytes to be programmed are correctly written, call the
        @ref HAL_FLASH_OB_Launch(void) function to launch the Option Bytes programming process.
    (#) Call the @ref HAL_FLASH_OB_Lock() to disable the Flash option control register access (recommended
        to protect the option Bytes against possible unwanted operations).

    [..] Proprietary code Read Out Protection (PcROP):
    (#) The PcROP sector is selected by using the same option bytes as the Write
        protection (nWRPi bits). As a result, these 2 options are exclusive each other.
    (#) In order to activate the PcROP (change the function of the nWRPi option bits), 
        the SPRMOD option bit must be activated.
    (#) The active value of nWRPi bits is inverted when PCROP mode is active, this
        means: if SPRMOD = 1 and nWRPi = 1 (default value), then the user sector "i"
        is read/write protected.
    (#) To activate PCROP mode for Flash sector(s), you need to call the following function:
        (++) @ref HAL_FLASHEx_AdvOBProgram in selecting sectors to be read/write protected
        (++) @ref HAL_FLASHEx_OB_SelectPCROP to enable the read/write protection
    (#) PcROP is available only in STM32L151xBA, STM32L152xBA, STM32L151xC, STM32L152xC & STM32L162xC devices.

@endverbatim
  * @{
  */

/**
  * @brief  Program option bytes
  * @param  pOBInit pointer to an FLASH_OBInitStruct structure that
  *         contains the configuration information for the programming.
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASHEx_OBProgram(FLASH_OBProgramInitTypeDef *pOBInit)
{
  HAL_StatusTypeDef status = HAL_ERROR;
  
  /* Process Locked */
  __HAL_LOCK(&pFlash);

  /* Check the parameters */
  assert_param(IS_OPTIONBYTE(pOBInit->OptionType));

  /*Write protection configuration*/
  if((pOBInit->OptionType & OPTIONBYTE_WRP) == OPTIONBYTE_WRP)
  {
    assert_param(IS_WRPSTATE(pOBInit->WRPState));
    if (pOBInit->WRPState == OB_WRPSTATE_ENABLE)
    {
      /* Enable of Write protection on the selected Sector*/
      status = FLASH_OB_WRPConfig(pOBInit, ENABLE);
    }
    else
    {
      /* Disable of Write protection on the selected Sector*/
      status = FLASH_OB_WRPConfig(pOBInit, DISABLE);
    }
    if (status != HAL_OK)
    {
      /* Process Unlocked */
      __HAL_UNLOCK(&pFlash);
      return status;
    }
  }
  
  /* Read protection configuration*/
  if((pOBInit->OptionType & OPTIONBYTE_RDP) == OPTIONBYTE_RDP)
  {
    status = FLASH_OB_RDPConfig(pOBInit->RDPLevel);
    if (status != HAL_OK)
    {
      /* Process Unlocked */
      __HAL_UNLOCK(&pFlash);
      return status;
    }
  }
  
  /* USER  configuration*/
  if((pOBInit->OptionType & OPTIONBYTE_USER) == OPTIONBYTE_USER)
  {
    status = FLASH_OB_UserConfig(pOBInit->USERConfig & OB_IWDG_SW, 
                                 pOBInit->USERConfig & OB_STOP_NORST,
                                 pOBInit->USERConfig & OB_STDBY_NORST);
    if (status != HAL_OK)
    {
      /* Process Unlocked */
      __HAL_UNLOCK(&pFlash);
      return status;
    }
  }

  /* BOR Level  configuration*/
  if((pOBInit->OptionType & OPTIONBYTE_BOR) == OPTIONBYTE_BOR)
  {
    status = FLASH_OB_BORConfig(pOBInit->BORLevel);
    if (status != HAL_OK)
    {
      /* Process Unlocked */
      __HAL_UNLOCK(&pFlash);
      return status;
    }
  }
  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  return status;
}

/**
  * @brief   Get the Option byte configuration
  * @param  pOBInit pointer to an FLASH_OBInitStruct structure that
  *         contains the configuration information for the programming.
  * 
  * @retval None
  */
void HAL_FLASHEx_OBGetConfig(FLASH_OBProgramInitTypeDef *pOBInit)
{
  pOBInit->OptionType = OPTIONBYTE_WRP | OPTIONBYTE_RDP | OPTIONBYTE_USER | OPTIONBYTE_BOR;

  /*Get WRP1*/
  pOBInit->WRPSector0To31 = (uint32_t)(FLASH->WRPR1);

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)    \
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xCA) \
 || defined(STM32L152xD) || defined(STM32L152xDX) || defined(STM32L162xCA) || defined(STM32L162xD)  \
 || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE)
    
  /*Get WRP2*/
  pOBInit->WRPSector32To63 = (uint32_t)(FLASH->WRPR2);

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || (...) || STM32L151xE || STM32L152xE || STM32L162xE */
  
#if defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xD) || defined(STM32L152xDX) \
 || defined(STM32L162xD) || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE)  \
 || defined(STM32L162xE)
    
  /*Get WRP3*/
  pOBInit->WRPSector64To95 = (uint32_t)(FLASH->WRPR3);

#endif /* STM32L151xD || STM32L152xD || STM32L162xD || STM32L151xE || STM32L152xE || STM32L162xE */
  
#if defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE) || defined(STM32L151xDX) \
 || defined(STM32L152xDX) || defined(STM32L162xDX)

  /*Get WRP4*/
  pOBInit->WRPSector96To127 = (uint32_t)(FLASH->WRPR4);

#endif /* STM32L151xE || STM32L152xE || STM32L162xE || STM32L151xDX || ... */

  /*Get RDP Level*/
  pOBInit->RDPLevel   = FLASH_OB_GetRDP();

  /*Get USER*/
  pOBInit->USERConfig = FLASH_OB_GetUser();

  /*Get BOR Level*/
  pOBInit->BORLevel   = FLASH_OB_GetBOR();
}

#if defined(FLASH_OBR_SPRMOD) || defined(FLASH_OBR_nRST_BFB2)
    
/**
  * @brief  Program option bytes
  * @note   This function can be used only for Cat2 & Cat3 devices for PCROP and Cat4 & Cat5 for BFB2.
  * @param  pAdvOBInit pointer to an FLASH_AdvOBProgramInitTypeDef structure that
  *         contains the configuration information for the programming.
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASHEx_AdvOBProgram (FLASH_AdvOBProgramInitTypeDef *pAdvOBInit)
{
  HAL_StatusTypeDef status = HAL_ERROR;
  
  /* Check the parameters */
  assert_param(IS_OBEX(pAdvOBInit->OptionType));

#if defined(FLASH_OBR_SPRMOD)
    
  /* Program PCROP option byte*/
  if ((pAdvOBInit->OptionType & OPTIONBYTE_PCROP) == OPTIONBYTE_PCROP)
  {
    /* Check the parameters */
    assert_param(IS_PCROPSTATE(pAdvOBInit->PCROPState));
    if (pAdvOBInit->PCROPState == OB_PCROP_STATE_ENABLE)
    {
      /*Enable of Write protection on the selected Sector*/
      status = FLASH_OB_PCROPConfig(pAdvOBInit, ENABLE);
      if (status != HAL_OK)
      {
        return status;
      }
    }
    else
    {
      /* Disable of Write protection on the selected Sector*/ 
      status = FLASH_OB_PCROPConfig(pAdvOBInit, DISABLE);
      if (status != HAL_OK)
      {
        return status;
      }
    }
  }
  
#endif /* FLASH_OBR_SPRMOD */

#if defined(FLASH_OBR_nRST_BFB2)
    
  /* Program BOOT config option byte */
  if ((pAdvOBInit->OptionType & OPTIONBYTE_BOOTCONFIG) == OPTIONBYTE_BOOTCONFIG)
  {
    status = FLASH_OB_BootConfig(pAdvOBInit->BootConfig);
  }
  
#endif /* FLASH_OBR_nRST_BFB2 */

  return status;
}

/**
  * @brief  Get the OBEX byte configuration
  * @note   This function can be used only for Cat2  & Cat3 devices for PCROP and Cat4 & Cat5 for BFB2.
  * @param  pAdvOBInit pointer to an FLASH_AdvOBProgramInitTypeDef structure that
  *         contains the configuration information for the programming.
  * 
  * @retval None
  */
void HAL_FLASHEx_AdvOBGetConfig(FLASH_AdvOBProgramInitTypeDef *pAdvOBInit)
{
  pAdvOBInit->OptionType = 0U;
  
#if defined(FLASH_OBR_SPRMOD)
      
  pAdvOBInit->OptionType |= OPTIONBYTE_PCROP;

  /*Get PCROP state */
  pAdvOBInit->PCROPState = (FLASH->OBR & FLASH_OBR_SPRMOD) >> POSITION_VAL(FLASH_OBR_SPRMOD);
  
  /*Get PCROP protected sector from 0 to 31 */
  pAdvOBInit->PCROPSector0To31 = FLASH->WRPR1;
  
#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)

  /*Get PCROP protected sector from 32 to 63 */
  pAdvOBInit->PCROPSector32To63 = FLASH->WRPR2;

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC */
#endif /* FLASH_OBR_SPRMOD */

#if defined(FLASH_OBR_nRST_BFB2)
      
  pAdvOBInit->OptionType |= OPTIONBYTE_BOOTCONFIG;

  /* Get Boot config OB */
  pAdvOBInit->BootConfig = (FLASH->OBR & FLASH_OBR_nRST_BFB2) >> 16U;

#endif /* FLASH_OBR_nRST_BFB2 */
}

#endif /* FLASH_OBR_SPRMOD || FLASH_OBR_nRST_BFB2 */

#if defined(FLASH_OBR_SPRMOD)

/**
  * @brief  Select the Protection Mode (SPRMOD).
  * @note   This function can be used only for STM32L151xBA, STM32L152xBA, STM32L151xC, STM32L152xC & STM32L162xC devices
  * @note   Once SPRMOD bit is active, unprotection of a protected sector is not possible 
  * @note   Read a protected sector will set RDERR Flag and write a protected sector will set WRPERR Flag
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_FLASHEx_OB_SelectPCROP(void)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint16_t tmp1 = 0U;
  uint32_t tmp2 = 0U;
  uint8_t optiontmp = 0U;
  uint16_t optiontmp2 = 0U;
  
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  /* Mask RDP Byte */
  optiontmp =  (uint8_t)(*(__IO uint8_t *)(OB_BASE)); 
  
  /* Update Option Byte */
  optiontmp2 = (uint16_t)(OB_PCROP_SELECTED | optiontmp); 
  
  /* calculate the option byte to write */
  tmp1 = (uint16_t)(~(optiontmp2 ));
  tmp2 = (uint32_t)(((uint32_t)((uint32_t)(tmp1) << 16U)) | ((uint32_t)optiontmp2));
  
  if(status == HAL_OK)
  {         
    /* Clean the error context */
    pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    /* program PCRop */
    OB->RDP = tmp2;
    
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  }
  
  /* Return the Read protection operation Status */
  return status;            
}

/**
  * @brief  Deselect the Protection Mode (SPRMOD).
  * @note   This function can be used only for STM32L151xBA, STM32L152xBA, STM32L151xC, STM32L152xC & STM32L162xC devices
  * @note   Once SPRMOD bit is active, unprotection of a protected sector is not possible 
  * @note   Read a protected sector will set RDERR Flag and write a protected sector will set WRPERR Flag
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_FLASHEx_OB_DeSelectPCROP(void)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint16_t tmp1 = 0U;
  uint32_t tmp2 = 0U;
  uint8_t optiontmp = 0U;
  uint16_t optiontmp2 = 0U;
  
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  /* Mask RDP Byte */
  optiontmp =  (uint8_t)(*(__IO uint8_t *)(OB_BASE)); 
  
  /* Update Option Byte */
  optiontmp2 = (uint16_t)(OB_PCROP_DESELECTED | optiontmp); 
  
  /* calculate the option byte to write */
  tmp1 = (uint16_t)(~(optiontmp2 ));
  tmp2 = (uint32_t)(((uint32_t)((uint32_t)(tmp1) << 16U)) | ((uint32_t)optiontmp2));
  
  if(status == HAL_OK)
  {         
    /* Clean the error context */
    pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    /* program PCRop */
    OB->RDP = tmp2;
    
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  }
  
  /* Return the Read protection operation Status */
  return status;            
}

#endif /* FLASH_OBR_SPRMOD */

/**
  * @}
  */

/** @defgroup FLASHEx_Exported_Functions_Group3 DATA EEPROM Programming functions
 *  @brief   DATA EEPROM Programming functions
 *
@verbatim   
 ===============================================================================
                     ##### DATA EEPROM Programming functions ##### 
 ===============================================================================  
 
    [..] Any operation of erase or program should follow these steps:
    (#) Call the @ref HAL_FLASHEx_DATAEEPROM_Unlock() function to enable the data EEPROM access
        and Flash program erase control register access.
    (#) Call the desired function to erase or program data.
    (#) Call the @ref HAL_FLASHEx_DATAEEPROM_Lock() to disable the data EEPROM access
        and Flash program erase control register access(recommended
        to protect the DATA_EEPROM against possible unwanted operation).

@endverbatim
  * @{
  */

/**
  * @brief  Unlocks the data memory and FLASH_PECR register access.
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASHEx_DATAEEPROM_Unlock(void)
{
  if((FLASH->PECR & FLASH_PECR_PELOCK) != RESET)
  {  
    /* Unlocking the Data memory and FLASH_PECR register access*/
    FLASH->PEKEYR = FLASH_PEKEY1;
    FLASH->PEKEYR = FLASH_PEKEY2;
  }
  else
  {
    return HAL_ERROR;
  }
  return HAL_OK;  
}

/**
  * @brief  Locks the Data memory and FLASH_PECR register access.
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASHEx_DATAEEPROM_Lock(void)
{
  /* Set the PELOCK Bit to lock the data memory and FLASH_PECR register access */
  SET_BIT(FLASH->PECR, FLASH_PECR_PELOCK);
  
  return HAL_OK;
}

/**
  * @brief  Erase a word in data memory.
  * @param  Address specifies the address to be erased.
  * @param  TypeErase  Indicate the way to erase at a specified address.
  *         This parameter can be a value of @ref FLASH_Type_Program
  * @note   To correctly run this function, the @ref HAL_FLASHEx_DATAEEPROM_Unlock() function
  *         must be called before.
  *         Call the @ref HAL_FLASHEx_DATAEEPROM_Lock() to the data EEPROM access
  *         and Flash program erase control register access(recommended to protect 
  *         the DATA_EEPROM against possible unwanted operation).
  * @retval HAL_StatusTypeDef HAL Status
  */
HAL_StatusTypeDef HAL_FLASHEx_DATAEEPROM_Erase(uint32_t TypeErase, uint32_t Address)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_TYPEPROGRAMDATA(TypeErase));
  assert_param(IS_FLASH_DATA_ADDRESS(Address));
  
  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {
    /* Clean the error context */
    pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    if(TypeErase == FLASH_TYPEERASEDATA_WORD)
    {
      /* Write 00000000h to valid address in the data memory */
      *(__IO uint32_t *) Address = 0x00000000U;
    }

    if(TypeErase == FLASH_TYPEERASEDATA_HALFWORD)
    {
      /* Write 0000h to valid address in the data memory */
      *(__IO uint16_t *) Address = (uint16_t)0x0000;
    }

    if(TypeErase == FLASH_TYPEERASEDATA_BYTE)
    {
      /* Write 00h to valid address in the data memory */
      *(__IO uint8_t *) Address = (uint8_t)0x00;
    }

    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  }
   
  /* Return the erase status */
  return status;
}  

/**
  * @brief  Program word at a specified address
  * @note   To correctly run this function, the @ref HAL_FLASHEx_DATAEEPROM_Unlock() function
  *         must be called before.
  *         Call the @ref HAL_FLASHEx_DATAEEPROM_Unlock() to he data EEPROM access
  *         and Flash program erase control register access(recommended to protect 
  *         the DATA_EEPROM against possible unwanted operation).
  * @note   The function @ref HAL_FLASHEx_DATAEEPROM_EnableFixedTimeProgram() can be called before 
  *         this function to configure the Fixed Time Programming.
  * @param  TypeProgram  Indicate the way to program at a specified address.
  *         This parameter can be a value of @ref FLASHEx_Type_Program_Data
  * @param  Address  specifie the address to be programmed.
  * @param  Data     specifie the data to be programmed
  * 
  * @retval HAL_StatusTypeDef HAL Status
  */

HAL_StatusTypeDef   HAL_FLASHEx_DATAEEPROM_Program(uint32_t TypeProgram, uint32_t Address, uint32_t Data)
{
  HAL_StatusTypeDef status = HAL_ERROR;
  
  /* Process Locked */
  __HAL_LOCK(&pFlash);

  /* Check the parameters */
  assert_param(IS_TYPEPROGRAMDATA(TypeProgram));

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {
    /* Clean the error context */
    pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    if(TypeProgram == FLASH_TYPEPROGRAMDATA_WORD)
    {
      /* Program word (32-bit) at a specified address.*/
      status = FLASH_DATAEEPROM_ProgramWord(Address, (uint32_t) Data);
    }
    else if(TypeProgram == FLASH_TYPEPROGRAMDATA_HALFWORD)
    {
      /* Program halfword (16-bit) at a specified address.*/
      status = FLASH_DATAEEPROM_ProgramHalfWord(Address, (uint16_t) Data);
    }
    else if(TypeProgram == FLASH_TYPEPROGRAMDATA_BYTE)
    {
      /* Program byte (8-bit) at a specified address.*/
      status = FLASH_DATAEEPROM_ProgramByte(Address, (uint8_t) Data);
    }
    else if(TypeProgram == FLASH_TYPEPROGRAMDATA_FASTBYTE)
    {
      /*Program word (8-bit) at a specified address.*/
      status = FLASH_DATAEEPROM_FastProgramByte(Address, (uint8_t) Data);
    }
    else if(TypeProgram == FLASH_TYPEPROGRAMDATA_FASTHALFWORD)
    {
      /* Program halfword (16-bit) at a specified address.*/
      status = FLASH_DATAEEPROM_FastProgramHalfWord(Address, (uint16_t) Data);
    }    
    else if(TypeProgram == FLASH_TYPEPROGRAMDATA_FASTWORD)
    {
      /* Program word (32-bit) at a specified address.*/
      status = FLASH_DATAEEPROM_FastProgramWord(Address, (uint32_t) Data);
    }
    else
    {
      status = HAL_ERROR;
    }

  }

  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  return status;
}

/**
  * @brief  Enable DATA EEPROM fixed Time programming (2*Tprog).
  * @retval None
  */
void HAL_FLASHEx_DATAEEPROM_EnableFixedTimeProgram(void)
{
  SET_BIT(FLASH->PECR, FLASH_PECR_FTDW);
}

/**
  * @brief  Disables DATA EEPROM fixed Time programming (2*Tprog).
  * @retval None
  */
void HAL_FLASHEx_DATAEEPROM_DisableFixedTimeProgram(void)
{
  CLEAR_BIT(FLASH->PECR, FLASH_PECR_FTDW);
}

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup FLASHEx_Private_Functions
 * @{
 */

/*
==============================================================================
              OPTIONS BYTES
==============================================================================
*/
/**
  * @brief  Enables or disables the read out protection.
  * @note   To correctly run this function, the @ref HAL_FLASH_OB_Unlock() function
  *         must be called before.
  * @param  OB_RDP specifies the read protection level. 
  *   This parameter can be:
  *     @arg @ref OB_RDP_LEVEL_0 No protection
  *     @arg @ref OB_RDP_LEVEL_1 Read protection of the memory
  *     @arg @ref OB_RDP_LEVEL_2 Chip protection
  * 
  *  !!!Warning!!! When enabling OB_RDP_LEVEL_2 it's no more possible to go back to level 1 or 0
  *   
  * @retval HAL status
  */
static HAL_StatusTypeDef FLASH_OB_RDPConfig(uint8_t OB_RDP)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint32_t tmp1 = 0U, tmp2 = 0U, tmp3 = 0U;
  
  /* Check the parameters */
  assert_param(IS_OB_RDP(OB_RDP));
  
  tmp1 = (uint32_t)(OB->RDP & FLASH_OBR_RDPRT);
  
  /* According to errata sheet, DocID022054 Rev 5, par2.1.5
  Before setting Level0 in the RDP register, check that the current level is not equal to Level0.
  If the current level is not equal to Level0, Level0 can be activated.
  If the current level is Level0 then the RDP register must not be written again with Level0. */
  
  if ((tmp1 == OB_RDP_LEVEL_0) && (OB_RDP == OB_RDP_LEVEL_0))
  {
    /*current level is Level0 then the RDP register must not be written again with Level0. */
    status = HAL_ERROR;
  }
  else 
  {
#if defined(FLASH_OBR_SPRMOD)
    /* Mask SPRMOD bit */
    tmp3 = (uint32_t)(OB->RDP & FLASH_OBR_SPRMOD);
#endif

    /* calculate the option byte to write */
    tmp1 = (~((uint32_t)(OB_RDP | tmp3)));
    tmp2 = (uint32_t)(((uint32_t)((uint32_t)(tmp1) << 16U)) | ((uint32_t)(OB_RDP | tmp3)));

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

    if(status == HAL_OK)
    {
      /* Clean the error context */
      pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

      /* program read protection level */
      OB->RDP = tmp2;

      /* Wait for last operation to be completed */
      status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    }
  }

  /* Return the Read protection operation Status */
  return status;
}

/**
  * @brief  Programs the FLASH brownout reset threshold level Option Byte.
  * @param  OB_BOR Selects the brownout reset threshold level.
  *   This parameter can be one of the following values:
  *     @arg @ref OB_BOR_OFF BOR is disabled at power down, the reset is asserted when the VDD 
  *                      power supply reaches the PDR(Power Down Reset) threshold (1.5V)
  *     @arg @ref OB_BOR_LEVEL1 BOR Reset threshold levels for 1.7V - 1.8V VDD power supply
  *     @arg @ref OB_BOR_LEVEL2 BOR Reset threshold levels for 1.9V - 2.0V VDD power supply
  *     @arg @ref OB_BOR_LEVEL3 BOR Reset threshold levels for 2.3V - 2.4V VDD power supply
  *     @arg @ref OB_BOR_LEVEL4 BOR Reset threshold levels for 2.55V - 2.65V VDD power supply
  *     @arg @ref OB_BOR_LEVEL5 BOR Reset threshold levels for 2.8V - 2.9V VDD power supply
  * @retval HAL status
  */
static HAL_StatusTypeDef FLASH_OB_BORConfig(uint8_t OB_BOR)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint32_t tmp = 0U, tmp1 = 0U;

  /* Check the parameters */
  assert_param(IS_OB_BOR_LEVEL(OB_BOR));

  /* Get the User Option byte register */
  tmp1 = OB->USER & ((~FLASH_OBR_BOR_LEV) >> 16U);

  /* Calculate the option byte to write - [0xFFU | nUSER | 0x00U | USER]*/
  tmp = (uint32_t)~((OB_BOR | tmp1)) << 16U;
  tmp |= (OB_BOR | tmp1);
    
  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {  
    /* Clean the error context */
    pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    /* Write the BOR Option Byte */            
    OB->USER = tmp;

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  }
  
  /* Return the Option Byte BOR programmation Status */
  return status;
}

/**
  * @brief  Returns the FLASH User Option Bytes values.
  * @retval The FLASH User Option Bytes.
  */
static uint8_t FLASH_OB_GetUser(void)
{
  /* Return the User Option Byte */
  return (uint8_t)((FLASH->OBR & FLASH_OBR_USER) >> 16U);
}

/**
  * @brief  Returns the FLASH Read Protection level.
  * @retval FLASH RDP level
  *         This parameter can be one of the following values:
  *            @arg @ref OB_RDP_LEVEL_0 No protection
  *            @arg @ref OB_RDP_LEVEL_1 Read protection of the memory
  *            @arg @ref OB_RDP_LEVEL_2 Full chip protection
  */
static uint8_t FLASH_OB_GetRDP(void)
{
  return (uint8_t)(FLASH->OBR & FLASH_OBR_RDPRT);
}

/**
  * @brief  Returns the FLASH BOR level.
  * @retval The BOR level Option Bytes.
  */
static uint8_t FLASH_OB_GetBOR(void)
{
  /* Return the BOR level */
  return (uint8_t)((FLASH->OBR & (uint32_t)FLASH_OBR_BOR_LEV) >> 16U);
}

/**
  * @brief  Write protects the desired pages of the first 64KB of the Flash.
  * @param  pOBInit pointer to an FLASH_OBInitStruct structure that
  *         contains WRP parameters.
  * @param  NewState new state of the specified FLASH Pages Wtite protection.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval HAL_StatusTypeDef
  */
static HAL_StatusTypeDef FLASH_OB_WRPConfig(FLASH_OBProgramInitTypeDef *pOBInit, FunctionalState NewState)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
 
  if(status == HAL_OK)
  {
    /* Clean the error context */
    pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    /* WRP for sector between 0 to 31 */
    if (pOBInit->WRPSector0To31 != 0U)
    {
      FLASH_OB_WRPConfigWRP1OrPCROP1(pOBInit->WRPSector0To31, NewState);
    }
    
#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)    \
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xCA) \
 || defined(STM32L152xD) || defined(STM32L152xDX) || defined(STM32L162xCA) || defined(STM32L162xD)  \
 || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE)
      
    /* Pages for Cat3, Cat4 & Cat5 devices*/
    /* WRP for sector between 32 to 63 */
    if (pOBInit->WRPSector32To63 != 0U)
    {
      FLASH_OB_WRPConfigWRP2OrPCROP2(pOBInit->WRPSector32To63, NewState);
    }
    
#endif /* STM32L100xC || STM32L151xC || STM32L152xC || (...) || STM32L151xE || STM32L152xE || STM32L162xE */

#if defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xD) || defined(STM32L152xDX) \
 || defined(STM32L162xD) || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE)  \
 || defined(STM32L162xE)
      
    /* Pages for devices with FLASH >= 256KB*/
    /* WRP for sector between 64 to 95 */
    if (pOBInit->WRPSector64To95 != 0U)
    {
      FLASH_OB_WRPConfigWRP3(pOBInit->WRPSector64To95, NewState);
    }
    
#endif /* STM32L151xD || STM32L152xD || STM32L162xD || STM32L151xE || STM32L152xE || STM32L162xE */

#if defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE) || defined(STM32L151xDX) \
 || defined(STM32L152xDX) || defined(STM32L162xDX)

    /* Pages for Cat5 devices*/
    /* WRP for sector between 96 to 127 */
    if (pOBInit->WRPSector96To127 != 0U)
    {
      FLASH_OB_WRPConfigWRP4(pOBInit->WRPSector96To127, NewState);
    }
    
#endif /* STM32L151xE || STM32L152xE || STM32L162xE || STM32L151xDX || ... */

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  }

  /* Return the write protection operation Status */
  return status;      
}

#if defined(STM32L151xBA) || defined(STM32L152xBA) || defined(STM32L151xC) || defined(STM32L152xC) \
 || defined(STM32L162xC)
/**
  * @brief  Enables the read/write protection (PCROP) of the desired 
  *         sectors.
  * @note   This function can be used only for Cat2 & Cat3 devices
  * @param  pAdvOBInit pointer to an FLASH_AdvOBProgramInitTypeDef structure that
  *         contains PCROP parameters.
  * @param  NewState new state of the specified FLASH Pages read/Write protection.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval HAL status
  */
static HAL_StatusTypeDef FLASH_OB_PCROPConfig(FLASH_AdvOBProgramInitTypeDef *pAdvOBInit, FunctionalState NewState)
{
  HAL_StatusTypeDef status = HAL_OK;
  FunctionalState pcropstate = DISABLE;
  
  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  /* Invert state to use same function of WRP */
  if (NewState == DISABLE)
  {
    pcropstate = ENABLE;
  }
        
  if(status == HAL_OK)
  {
    /* Clean the error context */
    pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    /* Pages for Cat2 devices*/
    /* PCROP for sector between 0 to 31 */
    if (pAdvOBInit->PCROPSector0To31 != 0U)
    {
      FLASH_OB_WRPConfigWRP1OrPCROP1(pAdvOBInit->PCROPSector0To31, pcropstate);
    }
    
#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)

    /* Pages for Cat3 devices*/
    /* WRP for sector between 32 to 63 */
    if (pAdvOBInit->PCROPSector32To63 != 0U)
    {
      FLASH_OB_WRPConfigWRP2OrPCROP2(pAdvOBInit->PCROPSector32To63, pcropstate);
    }
    
#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC  */
    
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  }

  /* Return the write protection operation Status */
  return status;      
}
#endif /* STM32L151xBA || STM32L152xBA || STM32L151xC || STM32L152xC || STM32L162xC */

/**
  * @brief  Write protects the desired pages of the first 128KB of the Flash.
  * @param  WRP1OrPCROP1 specifies the address of the pages to be write protected.
  *   This parameter can be a combination of @ref FLASHEx_Option_Bytes_Write_Protection1
  * @param  NewState new state of the specified FLASH Pages Write protection.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
static void FLASH_OB_WRPConfigWRP1OrPCROP1(uint32_t WRP1OrPCROP1, FunctionalState NewState)
{
  uint32_t wrp01data = 0U, wrp23data = 0U;
  
  uint32_t tmp1 = 0U, tmp2 = 0U;
  
  /* Check the parameters */
  assert_param(IS_OB_WRP(WRP1OrPCROP1));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    wrp01data = (uint16_t)(((WRP1OrPCROP1 & WRP_MASK_LOW) | OB->WRP01));
    wrp23data = (uint16_t)((((WRP1OrPCROP1 & WRP_MASK_HIGH)>>16U | OB->WRP23))); 
    tmp1 = (uint32_t)(~(wrp01data) << 16U)|(wrp01data);
    OB->WRP01 = tmp1;

    tmp2 = (uint32_t)(~(wrp23data) << 16U)|(wrp23data);
    OB->WRP23 = tmp2;      
  }
  else
  {
    wrp01data = (uint16_t)(~WRP1OrPCROP1 & (WRP_MASK_LOW & OB->WRP01));
    wrp23data = (uint16_t)((((~WRP1OrPCROP1 & WRP_MASK_HIGH)>>16U & OB->WRP23))); 

    tmp1 = (uint32_t)((~wrp01data) << 16U)|(wrp01data);
    OB->WRP01 = tmp1;
    
    tmp2 = (uint32_t)((~wrp23data) << 16U)|(wrp23data);
    OB->WRP23 = tmp2;
  }
}

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)    \
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xCA) \
 || defined(STM32L152xD) || defined(STM32L152xDX) || defined(STM32L162xCA) || defined(STM32L162xD)  \
 || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE)
/**
  * @brief  Enable Write protects the desired pages of the second 128KB of the Flash.
  * @note   This function can be used only for Cat3, Cat4  & Cat5 devices.
  * @param  WRP2OrPCROP2 specifies the address of the pages to be write protected.
  *   This parameter can be a combination of @ref FLASHEx_Option_Bytes_Write_Protection2
  * @param  NewState new state of the specified FLASH Pages Wtite protection.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
static void FLASH_OB_WRPConfigWRP2OrPCROP2(uint32_t WRP2OrPCROP2, FunctionalState NewState)
{
  uint32_t wrp45data = 0U, wrp67data = 0U;
  
  uint32_t tmp1 = 0U, tmp2 = 0U;
  
  /* Check the parameters */
  assert_param(IS_OB_WRP(WRP2OrPCROP2));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    wrp45data = (uint16_t)(((WRP2OrPCROP2 & WRP_MASK_LOW) | OB->WRP45));
    wrp67data = (uint16_t)((((WRP2OrPCROP2 & WRP_MASK_HIGH)>>16U | OB->WRP67))); 
    tmp1 = (uint32_t)(~(wrp45data) << 16U)|(wrp45data);
    OB->WRP45 = tmp1;
    
    tmp2 = (uint32_t)(~(wrp67data) << 16U)|(wrp67data);
    OB->WRP67 = tmp2;
  }
  else
  {
    wrp45data = (uint16_t)(~WRP2OrPCROP2 & (WRP_MASK_LOW & OB->WRP45));
    wrp67data = (uint16_t)((((~WRP2OrPCROP2 & WRP_MASK_HIGH)>>16U & OB->WRP67))); 
    
    tmp1 = (uint32_t)((~wrp45data) << 16U)|(wrp45data);
    OB->WRP45 = tmp1;
    
    tmp2 = (uint32_t)((~wrp67data) << 16U)|(wrp67data);
    OB->WRP67 = tmp2;
  }
}
#endif /* STM32L100xC || STM32L151xC || STM32L152xC || (...) || STM32L151xE || STM32L152xE || STM32L162xE */

#if defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xD) || defined(STM32L152xDX) \
 || defined(STM32L162xD) || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE)  \
 || defined(STM32L162xE)
/**
  * @brief  Enable Write protects the desired pages of the third 128KB of the Flash.
  * @note   This function can be used only for STM32L151xD, STM32L152xD, STM32L162xD  & Cat5 devices.
  * @param  WRP3 specifies the address of the pages to be write protected.
  *   This parameter can be a combination of @ref FLASHEx_Option_Bytes_Write_Protection3
  * @param  NewState new state of the specified FLASH Pages Wtite protection.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
static void FLASH_OB_WRPConfigWRP3(uint32_t WRP3, FunctionalState NewState)
{
  uint32_t wrp89data = 0U, wrp1011data = 0U;
  
  uint32_t tmp1 = 0U, tmp2 = 0U;
  
  /* Check the parameters */
  assert_param(IS_OB_WRP(WRP3));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    wrp89data = (uint16_t)(((WRP3 & WRP_MASK_LOW) | OB->WRP89));
    wrp1011data = (uint16_t)((((WRP3 & WRP_MASK_HIGH)>>16U | OB->WRP1011))); 
    tmp1 = (uint32_t)(~(wrp89data) << 16U)|(wrp89data);
    OB->WRP89 = tmp1;

    tmp2 = (uint32_t)(~(wrp1011data) << 16U)|(wrp1011data);
    OB->WRP1011 = tmp2;      
  }
  else
  {
    wrp89data = (uint16_t)(~WRP3 & (WRP_MASK_LOW & OB->WRP89));
    wrp1011data = (uint16_t)((((~WRP3 & WRP_MASK_HIGH)>>16U & OB->WRP1011))); 

    tmp1 = (uint32_t)((~wrp89data) << 16U)|(wrp89data);
    OB->WRP89 = tmp1;

    tmp2 = (uint32_t)((~wrp1011data) << 16U)|(wrp1011data);
    OB->WRP1011 = tmp2;
  }
}
#endif /* STM32L151xD || STM32L152xD || STM32L162xD || STM32L151xE || STM32L152xE || STM32L162xE */

#if defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE) || defined(STM32L151xDX) \
 || defined(STM32L152xDX) || defined(STM32L162xDX)
/**
  * @brief  Enable Write protects the desired pages of the Fourth 128KB of the Flash.
  * @note   This function can be used only for Cat5 & STM32L1xxDX devices.
  * @param  WRP4 specifies the address of the pages to be write protected.
  *   This parameter can be a combination of @ref FLASHEx_Option_Bytes_Write_Protection4
  * @param  NewState new state of the specified FLASH Pages Wtite protection.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
static void FLASH_OB_WRPConfigWRP4(uint32_t WRP4, FunctionalState NewState)
{
  uint32_t wrp1213data = 0U, wrp1415data = 0U;
  
  uint32_t tmp1 = 0U, tmp2 = 0U;
  
  /* Check the parameters */
  assert_param(IS_OB_WRP(WRP4));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    wrp1213data = (uint16_t)(((WRP4 & WRP_MASK_LOW) | OB->WRP1213));
    wrp1415data = (uint16_t)((((WRP4 & WRP_MASK_HIGH)>>16U | OB->WRP1415))); 
    tmp1 = (uint32_t)(~(wrp1213data) << 16U)|(wrp1213data);
    OB->WRP1213 = tmp1;

    tmp2 = (uint32_t)(~(wrp1415data) << 16U)|(wrp1415data);
    OB->WRP1415 = tmp2;      
  }
  else
  {
    wrp1213data = (uint16_t)(~WRP4 & (WRP_MASK_LOW & OB->WRP1213));
    wrp1415data = (uint16_t)((((~WRP4 & WRP_MASK_HIGH)>>16U & OB->WRP1415))); 

    tmp1 = (uint32_t)((~wrp1213data) << 16U)|(wrp1213data);
    OB->WRP1213 = tmp1;

    tmp2 = (uint32_t)((~wrp1415data) << 16U)|(wrp1415data);
    OB->WRP1415 = tmp2;
  }
}
#endif /* STM32L151xE || STM32L152xE || STM32L162xE || STM32L151xDX || ... */

/**
  * @brief  Programs the FLASH User Option Byte: IWDG_SW / RST_STOP / RST_STDBY.
  * @param  OB_IWDG Selects the WDG mode.
  *   This parameter can be one of the following values:
  *     @arg @ref OB_IWDG_SW Software WDG selected
  *     @arg @ref OB_IWDG_HW Hardware WDG selected
  * @param  OB_STOP Reset event when entering STOP mode.
  *   This parameter can be one of the following values:
  *     @arg @ref OB_STOP_NORST No reset generated when entering in STOP
  *     @arg @ref OB_STOP_RST Reset generated when entering in STOP
  * @param  OB_STDBY Reset event when entering Standby mode.
  *   This parameter can be one of the following values:
  *     @arg @ref OB_STDBY_NORST No reset generated when entering in STANDBY
  *     @arg @ref OB_STDBY_RST Reset generated when entering in STANDBY
  * @retval HAL status
  */
static HAL_StatusTypeDef FLASH_OB_UserConfig(uint8_t OB_IWDG, uint8_t OB_STOP, uint8_t OB_STDBY)
{
  HAL_StatusTypeDef status = HAL_OK; 
  uint32_t tmp = 0U, tmp1 = 0U;

  /* Check the parameters */
  assert_param(IS_OB_IWDG_SOURCE(OB_IWDG));
  assert_param(IS_OB_STOP_SOURCE(OB_STOP));
  assert_param(IS_OB_STDBY_SOURCE(OB_STDBY));

  /* Get the User Option byte register */
  tmp1 = OB->USER & ((~FLASH_OBR_USER) >> 16U);

  /* Calculate the user option byte to write */ 
  tmp = (uint32_t)(((uint32_t)~((uint32_t)((uint32_t)(OB_IWDG) | (uint32_t)(OB_STOP) | (uint32_t)(OB_STDBY) | tmp1))) << 16U);
  tmp |= ((uint32_t)(OB_IWDG) | ((uint32_t)OB_STOP) | (uint32_t)(OB_STDBY) | tmp1);
  
  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {  
    /* Clean the error context */
    pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    /* Write the User Option Byte */
    OB->USER = tmp;
    
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  }

  /* Return the Option Byte program Status */
  return status;
}

#if defined(FLASH_OBR_nRST_BFB2)
/**
  * @brief  Configures to boot from Bank1 or Bank2.
  * @param  OB_BOOT select the FLASH Bank to boot from.
  *   This parameter can be one of the following values:
  *     @arg @ref OB_BOOT_BANK2 At startup, if boot pins are set in boot from user Flash
  *        position and this parameter is selected the device will boot from Bank2 or Bank1,
  *        depending on the activation of the bank. The active banks are checked in
  *        the following order: Bank2, followed by Bank1.
  *        The active bank is recognized by the value programmed at the base address
  *        of the respective bank (corresponding to the initial stack pointer value
  *        in the interrupt vector table).
  *     @arg @ref OB_BOOT_BANK1 At startup, if boot pins are set in boot from user Flash
  *        position and this parameter is selected the device will boot from Bank1(Default).
  *        For more information, please refer to AN2606 from www.st.com. 
  * @retval HAL status
  */
static HAL_StatusTypeDef FLASH_OB_BootConfig(uint8_t OB_BOOT)
{
  HAL_StatusTypeDef status = HAL_OK; 
  uint32_t tmp = 0U, tmp1 = 0U;

  /* Check the parameters */
  assert_param(IS_OB_BOOT_BANK(OB_BOOT));

  /* Get the User Option byte register  and BOR Level*/
  tmp1 = OB->USER & ((~FLASH_OBR_nRST_BFB2) >> 16U);

  /* Calculate the option byte to write */
  tmp = (uint32_t)~(OB_BOOT | tmp1) << 16U;
  tmp |= (OB_BOOT | tmp1);

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

  if(status == HAL_OK)
  {  
    /* Clean the error context */
    pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

    /* Write the BOOT Option Byte */
    OB->USER = tmp;
    
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  }

  /* Return the Option Byte program Status */
  return status;
}

#endif /* FLASH_OBR_nRST_BFB2 */

/*
==============================================================================
              DATA
==============================================================================
*/

/**
  * @brief  Write a Byte at a specified address in data memory.
  * @param  Address specifies the address to be written.
  * @param  Data specifies the data to be written.
  * @note   This function assumes that the is data word is already erased.
  * @retval HAL status
  */
static HAL_StatusTypeDef FLASH_DATAEEPROM_FastProgramByte(uint32_t Address, uint8_t Data)
{
  HAL_StatusTypeDef status = HAL_OK;
#if defined(STM32L100xB) || defined(STM32L151xB) || defined(STM32L152xB)
  uint32_t tmp = 0U, tmpaddr = 0U;
#endif /* STM32L100xB || STM32L151xB || STM32L152xB  */
  
  /* Check the parameters */
  assert_param(IS_FLASH_DATA_ADDRESS(Address)); 

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    
  if(status == HAL_OK)
  {
    /* Clear the FTDW bit */
    CLEAR_BIT(FLASH->PECR, FLASH_PECR_FTDW);

#if defined(STM32L100xB) || defined(STM32L151xB) || defined(STM32L152xB)
    /* Possible only on Cat1 devices */
    if(Data != (uint8_t)0x00U) 
    {
      /* If the previous operation is completed, proceed to write the new Data */
      *(__IO uint8_t *)Address = Data;
            
      /* Wait for last operation to be completed */
      status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    }
    else
    {
      tmpaddr = Address & 0xFFFFFFFCU;
      tmp = * (__IO uint32_t *) tmpaddr;
      tmpaddr = 0xFFU << ((uint32_t) (0x8U * (Address & 0x3U)));
      tmp &= ~tmpaddr;
      status = HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEERASEDATA_WORD, Address & 0xFFFFFFFCU);
      /* Process Unlocked */
      __HAL_UNLOCK(&pFlash);
      status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTWORD, (Address & 0xFFFFFFFCU), tmp);
      /* Process Locked */
      __HAL_LOCK(&pFlash);
    }
#else /*!Cat1*/ 
    /* If the previous operation is completed, proceed to write the new Data */
    *(__IO uint8_t *)Address = Data;
            
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
#endif /* STM32L100xB || STM32L151xB || STM32L152xB  */
  }
  /* Return the Write Status */
  return status;
}

/**
  * @brief  Writes a half word at a specified address in data memory.
  * @param  Address specifies the address to be written.
  * @param  Data specifies the data to be written.
  * @note   This function assumes that the is data word is already erased.
  * @retval HAL status
  */
static HAL_StatusTypeDef FLASH_DATAEEPROM_FastProgramHalfWord(uint32_t Address, uint16_t Data)
{
  HAL_StatusTypeDef status = HAL_OK;
#if defined(STM32L100xB) || defined(STM32L151xB) || defined(STM32L152xB)
  uint32_t tmp = 0U, tmpaddr = 0U;
#endif /* STM32L100xB || STM32L151xB || STM32L152xB  */
  
  /* Check the parameters */
  assert_param(IS_FLASH_DATA_ADDRESS(Address));

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    
  if(status == HAL_OK)
  {
    /* Clear the FTDW bit */
    CLEAR_BIT(FLASH->PECR, FLASH_PECR_FTDW);

#if defined(STM32L100xB) || defined(STM32L151xB) || defined(STM32L152xB)
    /* Possible only on Cat1 devices */
    if(Data != (uint16_t)0x0000U) 
    {
      /* If the previous operation is completed, proceed to write the new data */
      *(__IO uint16_t *)Address = Data;
  
      /* Wait for last operation to be completed */
      status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    }
    else
    {
      /* Process Unlocked */
      __HAL_UNLOCK(&pFlash);
      if((Address & 0x3U) != 0x3U)
      {
        tmpaddr = Address & 0xFFFFFFFCU;
        tmp = * (__IO uint32_t *) tmpaddr;
        tmpaddr = 0xFFFFU << ((uint32_t) (0x8U * (Address & 0x3U)));
        tmp &= ~tmpaddr;        
        status = HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEERASEDATA_WORD, Address & 0xFFFFFFFCU);
        status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTWORD, (Address & 0xFFFFFFFCU), tmp);
      }
      else
      {
        HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE, Address, 0x00U);
        HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE, Address + 1U, 0x00U);
      }
      /* Process Locked */
      __HAL_LOCK(&pFlash);
    }
#else /* !Cat1 */
    /* If the previous operation is completed, proceed to write the new data */
    *(__IO uint16_t *)Address = Data;
  
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
#endif /* STM32L100xB || STM32L151xB || STM32L152xB  */
  }
  /* Return the Write Status */
  return status;
}

/**
  * @brief  Programs a word at a specified address in data memory.
  * @param  Address specifies the address to be written.
  * @param  Data specifies the data to be written.
  * @note   This function assumes that the is data word is already erased.
  * @retval HAL status
  */
static HAL_StatusTypeDef FLASH_DATAEEPROM_FastProgramWord(uint32_t Address, uint32_t Data)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_FLASH_DATA_ADDRESS(Address));
  
  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {
    /* Clear the FTDW bit */
    CLEAR_BIT(FLASH->PECR, FLASH_PECR_FTDW);
  
    /* If the previous operation is completed, proceed to program the new data */    
    *(__IO uint32_t *)Address = Data;
    
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);       
  }
  /* Return the Write Status */
  return status;
}

/**
  * @brief  Write a Byte at a specified address in data memory without erase.
  * @param  Address specifies the address to be written.
  * @param  Data specifies the data to be written.
  * @retval HAL status
  */
static HAL_StatusTypeDef FLASH_DATAEEPROM_ProgramByte(uint32_t Address, uint8_t Data)
{
  HAL_StatusTypeDef status = HAL_OK;
#if defined(STM32L100xB) || defined(STM32L151xB) || defined(STM32L152xB)
  uint32_t tmp = 0U, tmpaddr = 0U;
#endif /* STM32L100xB || STM32L151xB || STM32L152xB  */
  
  /* Check the parameters */
  assert_param(IS_FLASH_DATA_ADDRESS(Address)); 

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {
#if defined(STM32L100xB) || defined(STM32L151xB) || defined(STM32L152xB)
    if(Data != (uint8_t) 0x00U)
    {  
      *(__IO uint8_t *)Address = Data;
    
      /* Wait for last operation to be completed */
      status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

    }
    else
    {
      tmpaddr = Address & 0xFFFFFFFCU;
      tmp = * (__IO uint32_t *) tmpaddr;
      tmpaddr = 0xFFU << ((uint32_t) (0x8U * (Address & 0x3U)));
      tmp &= ~tmpaddr;        
      status = HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEERASEDATA_WORD, Address & 0xFFFFFFFCU);
      /* Process Unlocked */
      __HAL_UNLOCK(&pFlash);
      status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTWORD, (Address & 0xFFFFFFFCU), tmp);
      /* Process Locked */
      __HAL_LOCK(&pFlash);
    }
#else /* Not Cat1*/
    *(__IO uint8_t *)Address = Data;
    
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
#endif /* STM32L100xB || STM32L151xB || STM32L152xB  */
  }
  /* Return the Write Status */
  return status;
}

/**
  * @brief  Writes a half word at a specified address in data memory without erase.
  * @param  Address specifies the address to be written.
  * @param  Data specifies the data to be written.
  * @retval HAL status
  */
static HAL_StatusTypeDef FLASH_DATAEEPROM_ProgramHalfWord(uint32_t Address, uint16_t Data)
{
  HAL_StatusTypeDef status = HAL_OK;
#if defined(STM32L100xB) || defined(STM32L151xB) || defined(STM32L152xB)
  uint32_t tmp = 0U, tmpaddr = 0U;
#endif /* STM32L100xB || STM32L151xB || STM32L152xB  */
  
  /* Check the parameters */
  assert_param(IS_FLASH_DATA_ADDRESS(Address));

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {
#if defined(STM32L100xB) || defined(STM32L151xB) || defined(STM32L152xB)
    if(Data != (uint16_t)0x0000U)
    {
      *(__IO uint16_t *)Address = Data;
   
      /* Wait for last operation to be completed */
      status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    }
    else
    {
      /* Process Unlocked */
      __HAL_UNLOCK(&pFlash);
      if((Address & 0x3U) != 0x3U)
      {
        tmpaddr = Address & 0xFFFFFFFCU;
        tmp = * (__IO uint32_t *) tmpaddr;
        tmpaddr = 0xFFFFU << ((uint32_t) (0x8U * (Address & 0x3U)));
        tmp &= ~tmpaddr;          
        status = HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEERASEDATA_WORD, Address & 0xFFFFFFFCU);
        status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTWORD, (Address & 0xFFFFFFFCU), tmp);
      }
      else
      {
        HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE, Address, 0x00U);
        HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE, Address + 1U, 0x00U);
      }
      /* Process Locked */
      __HAL_LOCK(&pFlash);
    }
#else /* Not Cat1*/
    *(__IO uint16_t *)Address = Data;
   
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
#endif /* STM32L100xB || STM32L151xB || STM32L152xB  */
  }
  /* Return the Write Status */
  return status;
}

/**
  * @brief  Programs a word at a specified address in data memory without erase.
  * @param  Address specifies the address to be written.
  * @param  Data specifies the data to be written.
  * @retval HAL status
  */
static HAL_StatusTypeDef FLASH_DATAEEPROM_ProgramWord(uint32_t Address, uint32_t Data)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_FLASH_DATA_ADDRESS(Address));
  
  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  
  if(status == HAL_OK)
  {
    *(__IO uint32_t *)Address = Data;

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  }
  /* Return the Write Status */
  return status;
}

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup FLASH
  * @{
  */


/** @addtogroup FLASH_Private_Functions
 * @{
 */

/**
  * @brief  Erases a specified page in program memory.
  * @param  PageAddress The page address in program memory to be erased.
  * @note   A Page is erased in the Program memory only if the address to load 
  *         is the start address of a page (multiple of @ref FLASH_PAGE_SIZE bytes).
  * @retval None
  */
void FLASH_PageErase(uint32_t PageAddress)
{
  /* Clean the error context */
  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

  /* Set the ERASE bit */
  SET_BIT(FLASH->PECR, FLASH_PECR_ERASE);

  /* Set PROG bit */
  SET_BIT(FLASH->PECR, FLASH_PECR_PROG);

  /* Write 00000000h to the first word of the program page to erase */
  *(__IO uint32_t *)(uint32_t)(PageAddress & ~(FLASH_PAGE_SIZE - 1)) = 0x00000000;
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
