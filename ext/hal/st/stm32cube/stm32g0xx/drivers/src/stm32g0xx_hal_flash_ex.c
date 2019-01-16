/**
  ******************************************************************************
  * @file    stm32g0xx_hal_flash_ex.c
  * @author  MCD Application Team
  * @brief   Extended FLASH HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the FLASH extended peripheral:
  *           + Extended programming operations functions
  *
 @verbatim
 ==============================================================================
                   ##### Flash Extended features #####
  ==============================================================================

  [..] Comparing to other previous devices, the FLASH interface for STM32G0xx
       devices contains the following additional features

       (+) Capacity up to 128 Kbytes with single bank architecture supporting read-while-write
           capability (RWW)
       (+) Single bank memory organization
       (+) PCROP protection

                        ##### How to use this driver #####
 ==============================================================================
  [..] This driver provides functions to configure and program the FLASH memory
       of all STM32G0xx devices. It includes
      (#) Flash Memory Erase functions:
           (++) Lock and Unlock the FLASH interface using HAL_FLASH_Unlock() and
                HAL_FLASH_Lock() functions
           (++) Erase function: Erase page, erase all sectors
           (++) There are two modes of erase :
             (+++) Polling Mode using HAL_FLASHEx_Erase()
             (+++) Interrupt Mode using HAL_FLASHEx_Erase_IT()

      (#) Option Bytes Programming function: Use HAL_FLASHEx_OBProgram() to :
        (++) Set/Reset the write protection
        (++) Set the Read protection Level
        (++) Program the user Option Bytes
        (++) Configure the PCROP protection
        (++) Set Securable memory area and boot entry point

      (#) Get Option Bytes Configuration function: Use HAL_FLASHEx_OBGetConfig() to :
        (++) Get the value of a write protection area
        (++) Know if the read protection is activated
        (++) Get the value of the user Option Bytes
        (++) Get Securable memory area and boot entry point informations

      (#) Enable or disable debugger usage using HAL_FLASHEx_EnableDebugger and
          HAL_FLASHEx_DisableDebugger.

      (#) Check is flash content is empty or not using HAL_FLASHEx_FlashEmptyCheck.
          and modify this setting (for flash loader purpose e.g.) using
          HAL_FLASHEx_ForceFlashEmpty.

      (#) Enable securable memory area protectionusing HAL_FLASHEx_EnableSecMemProtection

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

/** @defgroup FLASHEx FLASHEx
  * @brief FALSH Extended HAL module driver
  * @{
  */

#ifdef HAL_FLASH_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup FLASHEx_Private_Variables FLASHEx Private Variables
 * @{
 */
extern FLASH_ProcessTypeDef pFlash;
/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup FLASHEx_Private_Functions FLASHEx Private Functions
 * @{
 */
extern HAL_StatusTypeDef  FLASH_WaitForLastOperation(uint32_t Timeout);
void                      FLASH_PageErase(uint32_t Page);
static void               FLASH_MassErase(void);
void                      FLASH_FlushCaches(void);
static void               FLASH_OB_WRPConfig(uint32_t WRPArea, uint32_t WRPStartOffset, uint32_t WRDPEndOffset);
static void               FLASH_OB_OptrConfig(uint32_t UserType, uint32_t UserConfig, uint32_t RDPLevel);
#if defined(FLASH_PCROP_SUPPORT)
static void               FLASH_OB_PCROP1AConfig(uint32_t PCROPConfig, uint32_t PCROP1AStartAddr, uint32_t PCROP1AEndAddr);
static void               FLASH_OB_PCROP1BConfig(uint32_t PCROP1BStartAddr, uint32_t PCROP1BEndAddr);
#endif
#if defined(FLASH_SECURABLE_MEMORY_SUPPORT)
static void               FLASH_OB_SecMemConfig(uint32_t BootEntry, uint32_t SecSize);
#endif
static void               FLASH_OB_GetWRP(uint32_t WRPArea, uint32_t *WRPStartOffset, uint32_t *WRDPEndOffset);
static uint32_t           FLASH_OB_GetRDP(void);
static uint32_t           FLASH_OB_GetUser(void);
#if defined(FLASH_PCROP_SUPPORT)
static void               FLASH_OB_GetPCROP1A(uint32_t *PCROPConfig, uint32_t *PCROP1AStartAddr, uint32_t *PCROP1AEndAddr);
static void               FLASH_OB_GetPCROP1B(uint32_t *PCROP1BStartAddr, uint32_t *PCROP1BEndAddr);
#endif
#if defined(FLASH_SECURABLE_MEMORY_SUPPORT)
static void               FLASH_OB_GetSecMem(uint32_t *BootEntry, uint32_t *SecSize);
#endif
/**
  * @}
  */

/* Exported functions -------------------------------------------------------*/
/** @defgroup FLASHEx_Exported_Functions FLASH Extended Exported Functions
  * @{
  */

/** @defgroup FLASHEx_Exported_Functions_Group1 Extended IO operation functions
 *  @brief   Extended IO operation functions
 *
@verbatim
 ===============================================================================
                ##### Extended programming operation functions #####
 ===============================================================================
    [..]
    This subsection provides a set of functions allowing to manage the Extended FLASH
    programming operations Operations.

@endverbatim
  * @{
  */

/**
  * @brief  Perform a mass erase or erase the specified FLASH memory pages.
  * @param[in]  pEraseInit pointer to an FLASH_EraseInitTypeDef structure that
  *         contains the configuration information for the erasing.
  * @param[out]  PageError   pointer to variable that contains the configuration
  *         information on faulty page in case of error (0xFFFFFFFF means that all
  *         the pages have been correctly erased)
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *PageError)
{
  HAL_StatusTypeDef status;
  uint32_t index;

  /* Check the parameters */
  assert_param(IS_FLASH_TYPEERASE(pEraseInit->TypeErase));

  /* Process Locked */
  __HAL_LOCK(&pFlash);

  /* Reset error code */
  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

  if (status == HAL_OK)
  {
    if (pEraseInit->TypeErase == FLASH_TYPEERASE_MASS)
    {
      /* Mass erase to be done */
      FLASH_MassErase();

      /* Wait for last operation to be completed */
      status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    }
    else
    {
      /*Initialization of PageError variable*/
      *PageError = 0xFFFFFFFFu;

      for (index = pEraseInit->Page; index < (pEraseInit->Page + pEraseInit->NbPages); index++)
      {
        /* Start erase page */
        FLASH_PageErase(index);

        /* Wait for last operation to be completed */
        status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

        if (status != HAL_OK)
        {
          /* In case of error, stop erase procedure and return the faulty address */
          *PageError = index;
          break;
        }
      }

      /* If operation is completed or interrupted, disable the Page Erase Bit */
      CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
    }
  }

  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  /* return status */
  return status;
}


/**
  * @brief  Perform a mass erase or erase the specified FLASH memory pages with interrupt enabled.
  * @param  pEraseInit pointer to an FLASH_EraseInitTypeDef structure that
  *         contains the configuration information for the erasing.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASHEx_Erase_IT(FLASH_EraseInitTypeDef *pEraseInit)
{
  HAL_StatusTypeDef status;

  /* Check the parameters */
  assert_param(IS_FLASH_TYPEERASE(pEraseInit->TypeErase));

  /* Process Locked */
  __HAL_LOCK(&pFlash);

  /* Reset error code */
  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

  /* save procedure for interrupt treatment */
  pFlash.ProcedureOnGoing = pEraseInit->TypeErase;

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

  if (status != HAL_OK)
  {
    /* Process Unlocked */
    __HAL_UNLOCK(&pFlash);
  }
  else
  {
    /* Enable End of Operation and Error interrupts */
    __HAL_FLASH_ENABLE_IT(FLASH_IT_EOP | FLASH_IT_OPERR);

    if (pEraseInit->TypeErase == FLASH_TYPEERASE_MASS)
    {
      /* Set Page to 0 for Interrupt callback managment */
      pFlash.Page = 0;

      /* Proceed to Mass Erase */
      FLASH_MassErase();
    }
    else
    {
      /* Erase by page to be done */
      pFlash.NbPagesToErase = pEraseInit->NbPages;
      pFlash.Page = pEraseInit->Page;

      /*Erase 1st page and wait for IT */
      FLASH_PageErase(pEraseInit->Page);
    }
  }

  /* return status */
  return status;
}


/**
  * @brief  Program Option bytes.
  * @param  pOBInit pointer to an FLASH_OBInitStruct structure that
  *         contains the configuration information for the programming.
  * @note   To configure any option bytes, the option lock bit OPTLOCK must be
  *         cleared with the call of HAL_FLASH_OB_Unlock() function.
  * @note   New option bytes configuration will be taken into account only
  *         - after an option bytes launch through the call of HAL_FLASH_OB_Launch()
  *         - a Power On Reset
  *         - an exit from Standby or Shutdown mode.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASHEx_OBProgram(FLASH_OBProgramInitTypeDef *pOBInit)
{
  uint32_t optr;
  HAL_StatusTypeDef status;

  /* Check the parameters */
  assert_param(IS_OPTIONBYTE(pOBInit->OptionType));

  /* Process Locked */
  __HAL_LOCK(&pFlash);

  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

  /* Write protection configuration */
  if ((pOBInit->OptionType & OPTIONBYTE_WRP) != 0x00u)
  {
    /* Configure of Write protection on the selected area */
    FLASH_OB_WRPConfig(pOBInit->WRPArea, pOBInit->WRPStartOffset, pOBInit->WRPEndOffset);
  }

  /* Option register */
  if ((pOBInit->OptionType & (OPTIONBYTE_RDP | OPTIONBYTE_USER)) == (OPTIONBYTE_RDP | OPTIONBYTE_USER))
  {
    /* Fully modify OPTR register with RDP & user datas */
    FLASH_OB_OptrConfig(pOBInit->USERType, pOBInit->USERConfig, pOBInit->RDPLevel);
  }
  else if ((pOBInit->OptionType & OPTIONBYTE_RDP) != 0x00u)
  {
    /* Only modify RDP so get current user data */
    optr = FLASH_OB_GetUser();
    FLASH_OB_OptrConfig(optr, optr, pOBInit->RDPLevel);
  }
  else if ((pOBInit->OptionType & OPTIONBYTE_USER) != 0x00u)
  {
    /* Only modify user so get current RDP level */
    optr = FLASH_OB_GetRDP();
    FLASH_OB_OptrConfig(pOBInit->USERType, pOBInit->USERConfig, optr);
  }
  else
  {
    /* nothing to do */
  }

#if defined(FLASH_PCROP_SUPPORT)
  /* PCROP Configuration */
  if ((pOBInit->OptionType & OPTIONBYTE_PCROP) != 0x00u)
  {
    /* Check the parameters */
    assert_param(IS_OB_PCROP_CONFIG(pOBInit->PCROPConfig));

    if ((pOBInit->PCROPConfig & (OB_PCROP_ZONE_A | OB_PCROP_RDP_ERASE)) != 0x00u)
    {
      /* Configure the 1A Proprietary code readout protection */
      FLASH_OB_PCROP1AConfig(pOBInit->PCROPConfig, pOBInit->PCROP1AStartAddr, pOBInit->PCROP1AEndAddr);
    }

    if ((pOBInit->PCROPConfig & OB_PCROP_ZONE_B) != 0x00u)
    {
      /* Configure the 1B Proprietary code readout protection */
      FLASH_OB_PCROP1BConfig(pOBInit->PCROP1BStartAddr, pOBInit->PCROP1BEndAddr);
    }
  }
#endif
#if defined(FLASH_SECURABLE_MEMORY_SUPPORT)
  /* Securable Memory Area Configuration */
  if ((pOBInit->OptionType & OPTIONBYTE_SEC) != 0x00u)
  {
    /* Configure the securable memory area protection */
    FLASH_OB_SecMemConfig(pOBInit->BootEntryPoint, pOBInit->SecSize);
  }
#endif

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

  if (status == HAL_OK)
  {
    /* Set OPTSTRT Bit */
    SET_BIT(FLASH->CR, FLASH_CR_OPTSTRT);

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

    /* If the option byte program operation is completed, disable the OPTSTRT Bit */
    CLEAR_BIT(FLASH->CR, FLASH_CR_OPTSTRT);
  }

  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  /* return status */
  return status;
}


/**
  * @brief  Get the Option bytes configuration.
  * @note   warning: this API only read flash register, it does not reflect any
  *         change that would have been programmed between previous Option byte
  *         loading and current call.
  * @param  pOBInit  pointer to an FLASH_OBInitStruct structure that contains the
  *                  configuration information. The fields pOBInit->WRPArea and
  *                  pOBInit->PCROPConfig should indicate which area is requested
  *                  for the WRP and PCROP
  * @retval None
  */
void HAL_FLASHEx_OBGetConfig(FLASH_OBProgramInitTypeDef *pOBInit)
{
  pOBInit->OptionType = OPTIONBYTE_ALL;

  /* Get write protection on the selected area */
  FLASH_OB_GetWRP(pOBInit->WRPArea, &(pOBInit->WRPStartOffset), &(pOBInit->WRPEndOffset));

  /* Get Read protection level */
  pOBInit->RDPLevel = FLASH_OB_GetRDP();

  /* Get the user option bytes */
  pOBInit->USERConfig = FLASH_OB_GetUser();
  pOBInit->USERType = OB_USER_ALL;

#if defined(FLASH_PCROP_SUPPORT)
  /* Get the Proprietary code readout protection */
  FLASH_OB_GetPCROP1A(&(pOBInit->PCROPConfig), &(pOBInit->PCROP1AStartAddr), &(pOBInit->PCROP1AEndAddr));
  FLASH_OB_GetPCROP1B(&(pOBInit->PCROP1BStartAddr), &(pOBInit->PCROP1BEndAddr));
  pOBInit->PCROPConfig |= (OB_PCROP_ZONE_A | OB_PCROP_ZONE_B);
#endif

#if defined(FLASH_SECURABLE_MEMORY_SUPPORT)
  /* Get the Securable Memory Area protection */
  FLASH_OB_GetSecMem(&(pOBInit->BootEntryPoint), &(pOBInit->SecSize));
#endif
}

#if defined(FLASH_ACR_DBG_SWEN)
/**
  * @brief  Enable Debugger.
  * @note   After calling this API, flash interface allow debugger intrusion.
  * @retval None
  */
void HAL_FLASHEx_EnableDebugger(void)
{
  FLASH->ACR |= FLASH_ACR_DBG_SWEN;
}


/**
  * @brief  Disable Debugger.
  * @note   After calling this API, Debugger is disabled: it is no more possible to
  *         break, see CPU register, etc...
  * @retval None
  */
void HAL_FLASHEx_DisableDebugger(void)
{
  FLASH->ACR &= ~FLASH_ACR_DBG_SWEN;
}
#endif /* FLASH_ACR_DBG_SWEN */

/**
  * @brief  Flash Empy check
  * @note   This API checks if first location in Flash is programmed or not.
  *         This check is done once by Option Byte Loader.
  * @retval 0 if 1st location is not programmed else
  */
uint32_t HAL_FLASHEx_FlashEmptyCheck(void)
{
  return ((FLASH->ACR & FLASH_ACR_PROGEMPTY));
}


/**
  * @brief  Force Empty check value.
  * @note   Allows to modify program empty check value in order to force this
  *         infrmation in Flash Interface, for all next reset that do not launch
  *         Option Byte Loader.
  * @param  FlashEmpty this parameter can be a value of @ref FLASHEx_Empty_Check
  * @retval None
  */
void HAL_FLASHEx_ForceFlashEmpty(uint32_t FlashEmpty)
{
  uint32_t acr;
  assert_param(IS_FLASH_EMPTY_CHECK(FlashEmpty));

  acr = (FLASH->ACR & ~FLASH_ACR_PROGEMPTY);
  FLASH->ACR = (acr | FlashEmpty);
}


#if defined(FLASH_SECURABLE_MEMORY_SUPPORT)
/**
  * @brief  Securable memory area protection enable
  * @param  Bank Select Bank to be secured. On G0, there is only 1 bank so
  *         parameter has to be set to 0.
  * @note   This API locks Securable memory area which is defined in SEC_SIZE option byte
  *         (that can be retrieved calling HAL_FLASHEx_OBGetConfig API and checking
  *         Secsize).
  * @note   SEC_PROT bit can only be set, it will be reset by system reset.
  * @retval None
  */
void HAL_FLASHEx_EnableSecMemProtection(uint32_t Bank)
{
  assert_param(IS_FLASH_BANK(Bank));
  FLASH->CR |= FLASH_CR_SEC_PROT;
}
#endif
/**
  * @}
  */

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/
/** @addtogroup FLASHEx_Private_Functions
  * @{
  */

/**
  * @brief  Mass erase of FLASH memory.
  * @retval None
  */
static void FLASH_MassErase(void)
{
  /* Set the Mass Erase Bit, then Start bit */
  FLASH->CR |= (FLASH_CR_STRT | FLASH_CR_MER1);

}


/**
  * @brief  Erase the specified FLASH memory page.
  * @param  Page FLASH page to erase
  *         This parameter must be a value between 0 and (max number of pages in Flash - 1)
  * @retval None
  */
void FLASH_PageErase(uint32_t Page)
{
  uint32_t tmp;

  /* Get configuration register, then clear page number */
  tmp = (FLASH->CR & ~FLASH_CR_PNB);

  /* Set page number, Page Erase bit & Start bit */
  FLASH->CR = (tmp | (FLASH_CR_STRT | (Page <<  FLASH_CR_PNB_Pos) | FLASH_CR_PER));
}


/**
  * @brief  Flush the instruction cache.
  * @retval None
  */
void FLASH_FlushCaches(void)
{
  /* Flush instruction cache  */
  if (READ_BIT(FLASH->ACR, FLASH_ACR_ICEN) != 0U)
  {
    /* Disable instruction cache  */
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
    /* Reset instruction cache */
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();
    /* Enable instruction cache */
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
  }
}


/**
  * @brief  Configure the write protection of the desired pages.
  * @note   When WRP is active in a zone, it cannot be erased or programmed.
  *         Consequently, a software mass erase cannot be performed if one zone
  *         is write-protected.
  * @note   When the memory read protection level is selected (RDP level = 1),
  *         it is not possible to program or erase Flash memory if the CPU debug
  *         features are connected (JTAG or single wire) or boot code is being
  *         executed from RAM or System flash, even if WRP is not activated.
  * @param  WRPArea  specifies the area to be configured.
  *         This parameter can be one of the following values:
  *            @arg OB_WRPAREA_ZONE_A: Flash Zone A
  *            @arg OB_WRPAREA_ZONE_B: Flash Zone B
  * @param  WRPStartOffset  specifies the start page of the write protected area
  *         This parameter is a page number between 0 and (max number of pages in Flash - 1)
  * @param  WRDPEndOffset  specifies the end page of the write protected area
  *         This parameter is a be page number between WRPStartOffset and (max number of pages in Flash - 1)
  * @retval HAL Status
  */
static void FLASH_OB_WRPConfig(uint32_t WRPArea, uint32_t WRPStartOffset, uint32_t WRDPEndOffset)
{
  /* Check the parameters */
  assert_param(IS_OB_WRPAREA(WRPArea));
  assert_param(IS_FLASH_PAGE(WRPStartOffset));
  assert_param(IS_FLASH_PAGE(WRDPEndOffset));

  /* Configure the write protected area */
  if (WRPArea != OB_WRPAREA_ZONE_A)
  {
    FLASH->WRP1BR = ((WRDPEndOffset << FLASH_WRP1AR_WRP1A_END_Pos) | WRPStartOffset);
  }
  else
  {
    FLASH->WRP1AR = ((WRDPEndOffset << FLASH_WRP1BR_WRP1B_END_Pos) | WRPStartOffset);
  }
}


/**
  * @brief  Set user & RDP configiuration
  * @note   !!! Warning : When enabling OB_RDP level 2 it is no more possible
  *         to go back to level 1 or 0 !!!
  * @param  UserType  User Option Bytes to be modified.
  *         This parameter can be a combination of @ref FLASH_OB_USER_Type
  * @param  UserConfig  The selected user option Bytes values.
  *         This parameter can be a combination of:
  *         @arg @ref FLASH_OB_USER_BOR_ENABLE(*),
  *         @arg @ref FLASH_OB_USER_BOR_LEVEL(*),
  *         @arg @ref FLASH_OB_USER_RESET_CONFIG(*),
  *         @arg @ref FLASH_OB_USER_nRST_STOP,
  *         @arg @ref FLASH_OB_USER_nRST_STANDBY,
  *         @arg @ref FLASH_OB_USER_nRST_SHUTDOWN(*),
  *         @arg @ref FLASH_OB_USER_IWDG_SW,
  *         @arg @ref FLASH_OB_USER_IWDG_STOP,
  *         @arg @ref FLASH_OB_USER_IWDG_STANDBY,
  *         @arg @ref FLASH_OB_USER_WWDG_SW,
  *         @arg @ref FLASH_OB_USER_SRAM_PARITY,
  *         @arg @ref FLASH_OB_USER_nBOOT_SEL,
  *         @arg @ref FLASH_OB_USER_nBOOT1,
  *         @arg @ref FLASH_OB_USER_nBOOT0,
  *         @arg @ref FLASH_OB_USER_INPUT_RESET_HOLDER(*)
  * @param  RDPLevel  specifies the read protection level.
  *         This parameter can be one of the following values:
  *            @arg OB_RDP_LEVEL_0: No protection
  *            @arg OB_RDP_LEVEL_1: Memory Read protection
  *            @arg OB_RDP_LEVEL_2: Full chip protection
  * @note  (*) availability depends on devices
  * @retval HAL status
  */
static void FLASH_OB_OptrConfig(uint32_t UserType, uint32_t UserConfig, uint32_t RDPLevel)
{
  uint32_t optr;

  /* Check the parameters */
  assert_param(IS_OB_USER_TYPE(UserType));
  assert_param(IS_OB_USER_CONFIG(UserType, UserConfig));
  assert_param(IS_OB_RDP_LEVEL(RDPLevel));

  /* Configure the RDP level in the option bytes register */
  optr = FLASH->OPTR;
  optr &= ~(UserType | FLASH_OPTR_RDP);
  FLASH->OPTR = (optr | UserConfig | RDPLevel);
}

#if defined(FLASH_PCROP_SUPPORT)
/**
  * @brief  Configure the 1A Proprietary code readout protection & erase configuration on RDP regression.
  * @note   It is recommended to align PCROP zone with page granularity when using PCROP_RDP or avoid
  *         having some executable code in a page where PCROP zone starts or ends.
  * @note   Minimum PCROP area size is 2 times the chosen granularity: PCROPA_STRT and PCROPA_END.
  *         So if the requirement is to be able to read-protect 1KB areas, the ROP granularity
  *         has to be set to 512 Bytes
  * @param  PCROPConfig  specifies the erase configuration (OB_PCROP_RDP_NOT_ERASE or OB_PCROP_RDP_ERASE)
  *         on RDP level 1 regression.
  * @param  PCROP1AStartAddr  specifies the start address of the 1A Proprietary code readout protection
  *          This parameter can be an address between begin and end of the flash
  * @param  PCROP1AEndAddr  specifies the end address of the 1A Proprietary code readout protection
  *          This parameter can be an address between PCROP1AStartAddr and end of the flash
  * @retval HAL Status
  */
static void FLASH_OB_PCROP1AConfig(uint32_t PCROPConfig, uint32_t PCROP1AStartAddr, uint32_t PCROP1AEndAddr)
{
  uint32_t startoffset;
  uint32_t endoffset;
  uint32_t pcrop1aend;

  /* Check the parameters */
  assert_param(IS_OB_PCROP_CONFIG(PCROPConfig));
  assert_param(IS_FLASH_MAIN_MEM_ADDRESS(PCROP1AStartAddr));
  assert_param(IS_FLASH_MAIN_MEM_ADDRESS(PCROP1AEndAddr));

  /* get pcrop 1A end register */
  pcrop1aend = FLASH->PCROP1AER;

  /* Configure the Proprietary code readout protection offset */
  if ((PCROPConfig & OB_PCROP_ZONE_A) != 0x00u)
  {
    /* Compute offset depending on pcrop granularity */
    startoffset = ((PCROP1AStartAddr - FLASH_BASE) >> FLASH_PCROP_GRANULARITY_OFFSET);
    endoffset = ((PCROP1AEndAddr - FLASH_BASE) >> FLASH_PCROP_GRANULARITY_OFFSET);

    /* Set Zone A start offset */
    FLASH->PCROP1ASR = startoffset;

    /* Set Zone A end offset */
    pcrop1aend &= ~FLASH_PCROP1AER_PCROP1A_END;
    pcrop1aend |= endoffset;
  }

  /* Set RDP erase protection if needed. This bit is only set & will be reset by mass erase */
  if ((PCROPConfig & OB_PCROP_RDP_ERASE) != 0x00u)
  {
    pcrop1aend |= FLASH_PCROP1AER_PCROP_RDP;
  }

  /* set 1A End register */
  FLASH->PCROP1AER = pcrop1aend;
}


/**
  * @brief  Configure the 1B Proprietary code readout protection.
  * @note   It is recommended to align PCROP zone with page granularity when using PCROP_RDP or avoid
  *         having some executable code in a page where PCROP zone starts or ends.
  * @note   Minimum PCROP area size is 2 times the chosen granularity: PCROPA_STRT and PCROPA_END.
  *         So if the requirement is to be able to read-protect 1KB areas, the ROP granularity
  *         has to be set to 512 Bytes
  * @param  PCROP1BStartAddr  specifies the start address of the 1B Proprietary code readout protection
  *          This parameter can be an address between begin and end of the bank
  * @param  PCROP1BEndAddr  specifies the end address of the 1B Proprietary code readout protection
  *          This parameter can be an address between PCROP1BStartAddr and end of the bank
  * @retval HAL Status
  */
static void FLASH_OB_PCROP1BConfig(uint32_t PCROP1BStartAddr, uint32_t PCROP1BEndAddr)
{
  uint32_t startoffset;
  uint32_t endoffset;

  /* Check the parameters */
  assert_param(IS_FLASH_MAIN_MEM_ADDRESS(PCROP1BStartAddr));
  assert_param(IS_FLASH_MAIN_MEM_ADDRESS(PCROP1BEndAddr));

  /* Configure the Proprietary code readout protection offset */
  startoffset = ((PCROP1BStartAddr - FLASH_BASE) >> FLASH_PCROP_GRANULARITY_OFFSET);
  endoffset = ((PCROP1BEndAddr - FLASH_BASE) >> FLASH_PCROP_GRANULARITY_OFFSET);

  /* Set Zone B start offset */
  FLASH->PCROP1BSR = startoffset;
  /* Set Zone B end offset */
  FLASH->PCROP1BER = endoffset;
}
#endif

#if defined(FLASH_SECURABLE_MEMORY_SUPPORT)
/**
  * @brief  Configure Securable Memory area feature.
  * @param  BootEntry  specifies if boot scheme is forced to Flash (System or user) or not
  *         This parameter can be one of the following values:
  *           @arg @ref OB_BOOT_ENTRY_FORCED_NONE No boot entry forced
  *           @arg @ref OB_BOOT_ENTRY_FORCED_FLASH FLash selected as unique entry boot
  * @param  SecSize specifies number of pages to protect as securable memory area, starting from
  *         beginning of the Flash (page 0).
  * @retval HAL Status
  */
static void FLASH_OB_SecMemConfig(uint32_t BootEntry, uint32_t SecSize)
{
  uint32_t secmem;

  /* Check the parameters */
  assert_param(IS_OB_SEC_BOOT_LOCK(BootEntry));
  assert_param(IS_OB_SEC_SIZE(SecSize));

  /* Set securable memory area configuration */
  secmem = (FLASH->SECR & ~(FLASH_SECR_BOOT_LOCK | FLASH_SECR_SEC_SIZE));
  FLASH->SECR = (secmem | BootEntry | SecSize);
}
#endif


/**
  * @brief  Return the FLASH Write Protection Option Bytes value.
  * @param[in]  WRPArea specifies the area to be returned.
  *             This parameter can be one of the following values:
  *               @arg @ref OB_WRPAREA_ZONE_A Flash Zone A
  *               @arg @ref OB_WRPAREA_ZONE_B Flash Zone B
  * @param[out]  WRPStartOffset  specifies the address where to copied the start page
  *                         of the write protected area
  * @param[out]  WRDPEndOffset  specifies the address where to copied the end page of
  *                        the write protected area
  * @retval None
  */
static void FLASH_OB_GetWRP(uint32_t WRPArea, uint32_t *WRPStartOffset, uint32_t *WRDPEndOffset)
{
  /* Check the parameters */
  assert_param(IS_OB_WRPAREA(WRPArea));

  /* Get the configuration of the write protected area */
  if (WRPArea == OB_WRPAREA_ZONE_A)
  {
    *WRPStartOffset = READ_BIT(FLASH->WRP1AR, FLASH_WRP1AR_WRP1A_STRT);
    *WRDPEndOffset = (READ_BIT(FLASH->WRP1AR, FLASH_WRP1AR_WRP1A_END) >> FLASH_WRP1AR_WRP1A_END_Pos);
  }
  else
  {
    *WRPStartOffset = READ_BIT(FLASH->WRP1BR, FLASH_WRP1BR_WRP1B_STRT);
    *WRDPEndOffset = (READ_BIT(FLASH->WRP1BR, FLASH_WRP1BR_WRP1B_END) >> FLASH_WRP1BR_WRP1B_END_Pos);
  }
}

/**
  * @brief  Return the FLASH Read Protection level.
  * @retval FLASH ReadOut Protection Status:
  *         This return value can be one of the following values:
  *            @arg OB_RDP_LEVEL_0: No protection
  *            @arg OB_RDP_LEVEL_1: Read protection of the memory
  *            @arg OB_RDP_LEVEL_2: Full chip protection
  */
static uint32_t FLASH_OB_GetRDP(void)
{
  uint32_t rdplvl = READ_BIT(FLASH->OPTR, FLASH_OPTR_RDP);

  if ((rdplvl != OB_RDP_LEVEL_0) && (rdplvl != OB_RDP_LEVEL_2))
  {
    return (OB_RDP_LEVEL_1);
  }
  else
  {
    return rdplvl;
  }
}


/**
  * @brief  Return the FLASH User Option Byte value.
  * @retval The FLASH User Option Bytes values. It will be a combination of
  *         @ref FLASH_OB_USER_BOR_ENABLE(*),
  *         @ref FLASH_OB_USER_BOR_LEVEL(*),
  *         @ref FLASH_OB_USER_RESET_CONFIG(*),
  *         @ref FLASH_OB_USER_nRST_STOP,
  *         @ref FLASH_OB_USER_nRST_STANDBY,
  *         @ref FLASH_OB_USER_nRST_SHUTDOWN(*),
  *         @ref FLASH_OB_USER_IWDG_SW,
  *         @ref FLASH_OB_USER_IWDG_STOP,
  *         @ref FLASH_OB_USER_IWDG_STANDBY,
  *         @ref FLASH_OB_USER_WWDG_SW,
  *         @ref FLASH_OB_USER_SRAM_PARITY,
  *         @ref FLASH_OB_USER_nBOOT_SEL,
  *         @ref FLASH_OB_USER_nBOOT1,
  *         @ref FLASH_OB_USER_nBOOT0,
  *         @ref FLASH_OB_USER_INPUT_RESET_HOLDER(*)
  * @note  (*) availability depends on devices
  */
static uint32_t FLASH_OB_GetUser(void)
{
  uint32_t user = ((FLASH->OPTR & ~FLASH_OPTR_RDP) & OB_USER_ALL);
  return user;
}

#if defined(FLASH_PCROP_SUPPORT)
/**
  * @brief  Return the FLASH PCROP Protection Option Bytes value.
  * @param  PCROPConfig [out]  specifies the configuration of PCROP_RDP option.
  * @param  PCROP1AStartAddr [out]  specifies the address where to copied the start address
  *         of the 1A Proprietary code readout protection
  * @param  PCROP1AEndAddr [out]  specifies the address where to copied the end address of
  *         the 1A Proprietary code readout protection
  * @retval None
  */
static void FLASH_OB_GetPCROP1A(uint32_t *PCROPConfig, uint32_t *PCROP1AStartAddr, uint32_t *PCROP1AEndAddr)
{
  uint32_t pcrop;

  pcrop = (FLASH->PCROP1ASR & FLASH_PCROP1ASR_PCROP1A_STRT);
  *PCROP1AStartAddr = (pcrop << FLASH_PCROP_GRANULARITY_OFFSET);
  *PCROP1AStartAddr += FLASH_BASE;

  pcrop = FLASH->PCROP1AER;
  *PCROP1AEndAddr = ((pcrop & FLASH_PCROP1AER_PCROP1A_END) << FLASH_PCROP_GRANULARITY_OFFSET);
  *PCROP1AEndAddr += (FLASH_BASE + FLASH_PCROP_GRANULARITY - 1u);

  *PCROPConfig &= ~OB_PCROP_RDP_ERASE;
  *PCROPConfig |= (pcrop & FLASH_PCROP1AER_PCROP_RDP);
}


/**
  * @brief  Return the FLASH PCROP Protection Option Bytes value.
  * @param  PCROP1BStartAddr [out]  specifies the address where to copied the start address
  *         of the 1B Proprietary code readout protection
  * @param  PCROP1BEndAddr [out]  specifies the address where to copied the end address of
  *         the 1B Proprietary code readout protection
  * @retval None
  */
static void FLASH_OB_GetPCROP1B(uint32_t *PCROP1BStartAddr, uint32_t *PCROP1BEndAddr)
{
  uint32_t pcrop;

  pcrop = (FLASH->PCROP1BSR & FLASH_PCROP1BSR_PCROP1B_STRT);
  *PCROP1BStartAddr = (pcrop << FLASH_PCROP_GRANULARITY_OFFSET);
  *PCROP1BStartAddr += FLASH_BASE;

  pcrop = (FLASH->PCROP1BER & FLASH_PCROP1BER_PCROP1B_END);
  *PCROP1BEndAddr = (pcrop << FLASH_PCROP_GRANULARITY_OFFSET);
  *PCROP1BEndAddr += (FLASH_BASE + FLASH_PCROP_GRANULARITY - 1u);
}
#endif

#if defined(FLASH_SECURABLE_MEMORY_SUPPORT)
/**
  * @brief  Return the FLASH Securable memory area protection Option Bytes value.
  * @param  BootEntry  specifies boot scheme configuration
  * @param  SecSize specifies number of pages to protect as secure memory area, starting from
  *         beginning of the Flash (page 0).
  * @retval None
  */
static void FLASH_OB_GetSecMem(uint32_t *BootEntry, uint32_t *SecSize)
{
  uint32_t secmem = FLASH->SECR;

  *BootEntry = (secmem & FLASH_SECR_BOOT_LOCK);
  *SecSize = (secmem & FLASH_SECR_SEC_SIZE);
}
#endif

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

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
