/**
  ******************************************************************************
  * @file    stm32wbxx_hal_flash_ex.c
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

  [..] Comparing to other previous devices, the FLASH interface for STM32WBxx
       devices contains the following additional features

       (+) Capacity up to 1 Mbyte with single bank architecture supporting read-while-write
           capability (RWW)
       (+) Single bank memory organization
       (+) PCROP protection
       (+) WRP protection
       (+) CPU2 Security area
       (+) Program Erase Suspend feature

                        ##### How to use this driver #####
 ==============================================================================
  [..] This driver provides functions to configure and program the FLASH memory
       of all STM32WBxx devices. It includes
      (#) Flash Memory Erase functions:
           (++) Lock and Unlock the FLASH interface using HAL_FLASH_Unlock() and
                HAL_FLASH_Lock() functions
           (++) Erase function: Erase page, erase all sectors
           (++) There are two modes of erase :
             (+++) Polling Mode using HAL_FLASHEx_Erase()
             (+++) Interrupt Mode using HAL_FLASHEx_Erase_IT()

      (#) Option Bytes Programming function: Use HAL_FLASHEx_OBProgram() to :
        (++) Set/Reset the write protection (per 4 KByte)
        (++) Set the Read protection Level
        (++) Program the user Option Bytes
        (++) Configure the PCROP protection (per 2 KByte)
        (++) Configure the IPCC Buffer start Address
        (++) Configure the CPU2 boot region and reset vector start Address
        (++) Configure the Flash and SRAM2 secure area

      (#) Get Option Bytes Configuration function: Use HAL_FLASHEx_OBGetConfig() to :
        (++) Get the value of a write protection area
        (++) Know if the read protection is activated
        (++) Get the value of the user Option Bytes
        (++) Get the value of a PCROP area
        (++) Get the IPCC Buffer start Address
        (++) Get the CPU2 boot region and reset vector start Address
        (++) Get the Flash and SRAM2 secure area

      (#) Flash Suspend, Allow functions:
           (++) Suspend or Allow new program or erase operation request using HAL_FLASHEx_SuspendOperation() and
                HAL_FLASHEx_AllowOperation() functions

      (#) Check is flash content is empty or not using HAL_FLASHEx_FlashEmptyCheck.
          and modify this setting (for flash loader purpose e.g.) using
          HAL_FLASHEx_ForceFlashEmpty.

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
#include "stm32wbxx_hal.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @defgroup FLASHEx FLASHEx
  * @brief FLASH Extended HAL module driver
  * @{
  */

#ifdef HAL_FLASH_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @defgroup FLASHEx_Private_Functions FLASHEx Private Functions
 * @{
 */
static void              FLASH_MassErase(void);
static void              FLASH_AcknowledgePageErase(void);
static void              FLASH_FlushCaches(void);
static void              FLASH_OB_WRPConfig(uint32_t WRPArea, uint32_t WRPStartOffset, uint32_t WRDPEndOffset);
static void              FLASH_OB_OptrConfig(uint32_t UserType, uint32_t UserConfig, uint32_t RDPLevel);
static void              FLASH_OB_PCROPConfig(uint32_t PCROPConfig, uint32_t PCROP1AStartAddr, uint32_t PCROP1AEndAddr, uint32_t PCROP1BStartAddr, uint32_t PCROP1BEndAddr);
static void              FLASH_OB_IPCCBufferAddrConfig(uint32_t IPCCDataBufAddr);
static void              FLASH_OB_SecureConfig(FLASH_OBProgramInitTypeDef *pOBParam);
static void              FLASH_OB_GetWRP(uint32_t WRPArea, uint32_t *WRPStartOffset, uint32_t *WRDPEndOffset);
static uint32_t          FLASH_OB_GetRDP(void);
static uint32_t          FLASH_OB_GetUser(void);
static void              FLASH_OB_GetPCROP(uint32_t *PCROPConfig, uint32_t *PCROP1AStartAddr, uint32_t *PCROP1AEndAddr, uint32_t *PCROP1BStartAddr, uint32_t *PCROP1BEndAddr);
static uint32_t          FLASH_OB_GetIPCCBufferAddr(void);
static void              FLASH_OB_GetSecureMemoryConfig(uint32_t *SecureFlashStartAddr, uint32_t *SecureRAM2aStartAddr, uint32_t *SecureRAM2bStartAddr, uint32_t *SecureMode);
static void              FLASH_OB_GetC2BootResetConfig(uint32_t *C2BootResetVectAddr, uint32_t *C2BootResetRegion);
static HAL_StatusTypeDef FLASH_OB_ProceedWriteOperation(void);
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
  * @note   Before any operation, it is possible to check there is no operation suspended
  *         by call HAL_FLASHEx_IsOperationSuspended()
  * @param[in]  pEraseInit Pointer to an @ref FLASH_EraseInitTypeDef structure that
  *         contains the configuration information for the erasing.
  * @param[out]  PageError Pointer to variable that contains the configuration
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

  /* Verify that next operation can be proceed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

  if (status == HAL_OK)
  {
    if (pEraseInit->TypeErase == FLASH_TYPEERASE_MASSERASE)
    {
      /* Mass erase to be done */
      FLASH_MassErase();

      /* Wait for last operation to be completed */
      status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    }
    else
    {
      /*Initialization of PageError variable*/
      *PageError = 0xFFFFFFFFU;

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
      FLASH_AcknowledgePageErase();
    }

    /* Flush the caches to be sure of the data consistency */
    FLASH_FlushCaches();
  }

  /* Process Unlocked */
  __HAL_UNLOCK(&pFlash);

  return status;
}

/**
  * @brief  Perform a mass erase or erase the specified FLASH memory pages with interrupt enabled.
  * @note   Before any operation, it is possible to check there is no operation suspended
  *         by call HAL_FLASHEx_IsOperationSuspended()
  * @param  pEraseInit Pointer to an @ref FLASH_EraseInitTypeDef structure that
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

  /* Verify that next operation can be proceed */
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

    if (pEraseInit->TypeErase == FLASH_TYPEERASE_MASSERASE)
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
  * @param  pOBInit Pointer to an @ref FLASH_OBProgramInitTypeDef structure that
  *         contains the configuration information for the programming.
  * @note   To configure any option bytes, the option lock bit OPTLOCK must be
  *         cleared with the call of @ref HAL_FLASH_OB_Unlock() function.
  * @note   New option bytes configuration will be taken into account only
  *         - after an option bytes launch through the call of @ref HAL_FLASH_OB_Launch()
  *         - a Power On Reset
  *         - an exit from Standby or Shutdown mode.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_FLASHEx_OBProgram(FLASH_OBProgramInitTypeDef *pOBInit)
{
  uint32_t optrRDPLevel;
  uint32_t optrUserType;
  uint32_t optrUserConfig;
  HAL_StatusTypeDef status;

  /* Check the parameters */
  assert_param(IS_OPTIONBYTE(pOBInit->OptionType));

  /* Process Locked */
  __HAL_LOCK(&pFlash);

  pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

  /* Write protection configuration */
  if ((pOBInit->OptionType & OPTIONBYTE_WRP) != 0U)
  {
    /* Configure of Write protection on the selected area */
    FLASH_OB_WRPConfig(pOBInit->WRPArea, pOBInit->WRPStartOffset, pOBInit->WRPEndOffset);
  }

  /* Option register (either RDP or USER)*/
  if ((pOBInit->OptionType & (OPTIONBYTE_RDP | OPTIONBYTE_USER)) != 0U)
  {
    if ((pOBInit->OptionType & OPTIONBYTE_RDP) != 0U)
    {
      /* Modify RDP */
      optrRDPLevel = pOBInit->RDPLevel;
    }
    else
    {
      /* Do not modify RDP */
      optrRDPLevel = FLASH_OB_GetRDP();
    }

    if ((pOBInit->OptionType & OPTIONBYTE_USER) != 0U)
    {
      /* Modify User Data */
      optrUserType = pOBInit->UserType;
      optrUserConfig = pOBInit->UserConfig;
    }
    else
    {
      /* Do not modifiy User Data */
      optrUserType = FLASH_OB_GetUser();
      optrUserConfig = FLASH_OB_GetUser();
    }

    /* Fully modify OPTR register with RDP & user datas */
    FLASH_OB_OptrConfig(optrUserType, optrUserConfig, optrRDPLevel);
  }

  /* PCROP Configuration */
  if ((pOBInit->OptionType & OPTIONBYTE_PCROP) != 0U)
  {
    /* Check the parameters */
    assert_param(IS_OB_PCROP_CONFIG(pOBInit->PCROPConfig));

    if ((pOBInit->PCROPConfig & (OB_PCROP_ZONE_A | OB_PCROP_ZONE_B | OB_PCROP_RDP_ERASE)) != 0U)
    {
      /* Configure the Zone 1A, 1B Proprietary code readout protection */
      FLASH_OB_PCROPConfig(pOBInit->PCROPConfig, pOBInit->PCROP1AStartAddr, pOBInit->PCROP1AEndAddr, pOBInit->PCROP1BStartAddr, pOBInit->PCROP1BEndAddr);
    }
  }

  /*  Secure mode and CPU2 Boot Vector */
  if ((pOBInit->OptionType & (OPTIONBYTE_SECURE_MODE | OPTIONBYTE_C2_BOOT_VECT)) != 0U)
  {
    /* Set the secure flash and SRAM memory start address */
    FLASH_OB_SecureConfig(pOBInit);
  }

  /* IPCC mailbox data buffer address */
  if ((pOBInit->OptionType & OPTIONBYTE_IPCC_BUF_ADDR) != 0U)
  {
    /* Configure the IPCC data buffer address */
    FLASH_OB_IPCCBufferAddrConfig(pOBInit->IPCCdataBufAddr);
  }

  /* Proceed the OB Write Operation */
  status = FLASH_OB_ProceedWriteOperation();

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
  * @param  pOBInit Pointer to an @ref FLASH_OBProgramInitTypeDef structure that contains the
  *                  configuration information. The fields pOBInit->WRPArea and
  *                  pOBInit->PCROPConfig should indicate which area is requested
  *                  for the WRP and PCROP.
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
  pOBInit->UserConfig = FLASH_OB_GetUser();
  pOBInit->UserType = OB_USER_ALL;

  /* Get the Zone 1A and 1B Proprietary code readout protection */
  FLASH_OB_GetPCROP(&(pOBInit->PCROPConfig), &(pOBInit->PCROP1AStartAddr), &(pOBInit->PCROP1AEndAddr), &(pOBInit->PCROP1BStartAddr), &(pOBInit->PCROP1BEndAddr));
  pOBInit->PCROPConfig |= (OB_PCROP_ZONE_A | OB_PCROP_ZONE_B);

  /* Get the IPCC start Address */
  pOBInit->IPCCdataBufAddr = FLASH_OB_GetIPCCBufferAddr();

  /* Get the Secure Flash start address, Secure Backup RAM2a start address, Secure non-Backup RAM2b start address and the Security Mode, */
  FLASH_OB_GetSecureMemoryConfig(&(pOBInit->SecureFlashStartAddr), &(pOBInit->SecureRAM2aStartAddr), &(pOBInit->SecureRAM2bStartAddr), &(pOBInit->SecureMode));

  /* Get the M0+ Secure Boot reset vector and Secure Boot memory selection */
  FLASH_OB_GetC2BootResetConfig(&(pOBInit->C2SecureBootVectAddr), &(pOBInit->C2BootRegion));
}

/**
  * @brief  Flash Empty check
  * @note   This API checks if first location in Flash is programmed or not.
  *         This check is done once by Option Byte Loader.
  * @retval Returned value can be one of the following values:
  *         @arg @ref FLASH_PROG_NOT_EMPTY 1st location in Flash is programmed
  *         @arg @ref FLASH_PROG_EMPTY 1st location in Flash is empty
  */
uint32_t HAL_FLASHEx_FlashEmptyCheck(void)
{
  return (READ_BIT(FLASH->ACR, FLASH_ACR_EMPTY));
}


/**
  * @brief  Force Empty check value.
  * @note   Allows to modify program empty check value in order to force this
  *         information in Flash Interface, for all next reset that do not launch
  *         Option Byte Loader.
  * @param  FlashEmpty Specifies the empty check value
  *          This parameter can be one of the following values:
  *            @arg @ref FLASH_PROG_NOT_EMPTY 1st location in Flash is programmed
  *            @arg @ref FLASH_PROG_EMPTY 1st location in Flash is empty
  * @retval None
  */
void HAL_FLASHEx_ForceFlashEmpty(uint32_t FlashEmpty)
{
  assert_param(IS_FLASH_EMPTY_CHECK(FlashEmpty));

  MODIFY_REG(FLASH->ACR, FLASH_ACR_EMPTY, FlashEmpty);
}

/**
  * @brief  Suspend new program or erase operation request.
  * @note   Any new Flash program and erase operation on both CPU side will be suspended
  *         until this bit and the same bit in Flash CPU2 access control register (FLASH_C2ACR) are
  *         cleared. The PESD bit in both the Flash status register (FLASH_SR) and Flash
  *         CPU2 status register (FLASH_C2SR) register will be set when at least one PES
  *         bit in FLASH_ACR or FLASH_C2ACR is set.
  * @retval None
  */
void HAL_FLASHEx_SuspendOperation(void)
{
  SET_BIT(FLASH->ACR, FLASH_ACR_PES);
}

/**
  * @brief  Allow new program or erase operation request.
  * @note   Any new Flash program and erase operation on both CPU side will be allowed
  *         until one of this bit or the same bit in Flash CPU2 access control register (FLASH_C2ACR) is
  *         set. The PESD bit in both the Flash status register (FLASH_SR) and Flash
  *         CPU2 status register (FLASH_C2SR) register will be clear when both PES
  *         bit in FLASH_ACR or FLASH_C2ACR is cleared.
  * @retval None
  */
void HAL_FLASHEx_AllowOperation(void)
{
  CLEAR_BIT(FLASH->ACR, FLASH_ACR_PES);
}

/**
  * @brief  Check if new program or erase operation request from CPU1 or CPU2 is suspended
  * @note   Any new Flash program and erase operation on both CPU side will be allowed
  *         until one of this bit or the same bit in Flash CPU2 access control register (FLASH_C2ACR) is
  *         set. The PESD bit in both the Flash status register (FLASH_SR) and Flash
  *         CPU2 status register (FLASH_C2SR) register will be cleared when both PES
  *         bit in FLASH_ACR and FLASH_C2ACR are cleared.
  * @retval Status
  *          - 0 : No suspended flash operation
  *          - 1 : Flash operation is suspended
  */
uint32_t HAL_FLASHEx_IsOperationSuspended(void)
{
  uint32_t status = 0U;

  if (READ_BIT(FLASH->SR, FLASH_SR_PESD) == FLASH_SR_PESD)
  {
    status = 1U;
  }

  return status;
}

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
  /* Set the Mass Erase Bit */
  SET_BIT(FLASH->CR, FLASH_CR_MER);
  /* Proceed to erase all sectors */
  SET_BIT(FLASH->CR, FLASH_CR_STRT);
}

/**
  * @brief  Erase the specified FLASH memory page.
  * @param  Page FLASH page to erase
  *         This parameter must be a value between 0 and (max number of pages in Flash - 1)
  * @retval None
  */
void FLASH_PageErase(uint32_t Page)
{
  /* Check the parameters */
  assert_param(IS_FLASH_PAGE(Page));

  /* Proceed to erase the page */
  MODIFY_REG(FLASH->CR, (FLASH_CR_PNB | FLASH_CR_PER), ((Page << FLASH_CR_PNB_Pos) | FLASH_CR_PER));
  SET_BIT(FLASH->CR, FLASH_CR_STRT);
}

/**
  * @brief  Flush the instruction and data caches.
  * @retval None
  */
static void FLASH_FlushCaches(void)
{
  /* Flush instruction cache  */
  if (READ_BIT(FLASH->ACR, FLASH_ACR_ICEN) == FLASH_ACR_ICEN)
  {
    /* Disable instruction cache  */
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
    /* Reset instruction cache */
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();
    /* Enable instruction cache */
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
  }

  /* Flush data cache */
  if (READ_BIT(FLASH->ACR, FLASH_ACR_DCEN) == FLASH_ACR_DCEN)
  {
    /* Disable data cache  */
    __HAL_FLASH_DATA_CACHE_DISABLE();
    /* Reset data cache */
    __HAL_FLASH_DATA_CACHE_RESET();
    /* Enable data cache */
    __HAL_FLASH_DATA_CACHE_ENABLE();
  }
}

/**
  * @brief  Acknlowldge the page erase operation.
  * @retval None
  */
static void FLASH_AcknowledgePageErase(void)
{
  CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));
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
  * @note   To configure the WRP options, the option lock bit OPTLOCK must be
  *         cleared with the call of the @ref HAL_FLASH_OB_Unlock() function.
  * @note   To validate the WRP options, the option bytes must be reloaded
  *         through the call of the @ref HAL_FLASH_OB_Launch() function.
  * @param  WRPArea Specifies the area to be configured.
  *          This parameter can be one of the following values:
  *            @arg @ref OB_WRPAREA_BANK1_AREAA Flash Bank 1 Area A
  *            @arg @ref OB_WRPAREA_BANK1_AREAB Flash Bank 1 Area B
  * @param  WRPStartOffset Specifies the start page of the write protected area
  *          This parameter can be page number between 0 and (max number of pages in the Flash - 1)
  * @param  WRDPEndOffset Specifies the end page of the write protected area
  *          This parameter can be page number between WRPStartOffset and (max number of pages in the Flash - 1)
  * @retval None
  */
static void FLASH_OB_WRPConfig(uint32_t WRPArea, uint32_t WRPStartOffset, uint32_t WRDPEndOffset)
{
  /* Check the parameters */
  assert_param(IS_OB_WRPAREA(WRPArea));
  assert_param(IS_FLASH_PAGE(WRPStartOffset));
  assert_param(IS_FLASH_PAGE(WRDPEndOffset));

  /* Configure the write protected area */
  if (WRPArea == OB_WRPAREA_BANK1_AREAA)
  {
    MODIFY_REG(FLASH->WRP1AR, (FLASH_WRP1AR_WRP1A_STRT | FLASH_WRP1AR_WRP1A_END),
               (WRPStartOffset | (WRDPEndOffset << FLASH_WRP1AR_WRP1A_END_Pos)));
  }
  else /* OB_WRPAREA_BANK1_AREAB */
  {
    MODIFY_REG(FLASH->WRP1BR, (FLASH_WRP1BR_WRP1B_STRT | FLASH_WRP1BR_WRP1B_END),
               (WRPStartOffset | (WRDPEndOffset << FLASH_WRP1AR_WRP1A_END_Pos)));
  }
}

/**
  * @brief  Set user & RDP configiuration
  * @note   !!! Warning : When enabling OB_RDP level 2 it's no more possible
  *         to go back to level 1 or 0 !!!
  * @param  UserType The FLASH User Option Bytes to be modified
  *         This parameter can be a combination of all the following values:
  *         @arg @ref OB_USER_BOR_LEV or @ref OB_USER_nRST_STOP or @ref OB_USER_nRST_STDBY or
  *         @arg @ref OB_USER_nRST_SHDW or @ref OB_USER_IWDG_SW or @ref OB_USER_IWDG_STOP or
  *         @arg @ref OB_USER_IWDG_STDBY or @ref OB_USER_WWDG_SW or @ref OB_USER_nBOOT1 or
  *         @arg @ref OB_USER_SRAM2PE or @ref OB_USER_SRAM2RST or @ref OB_USER_nSWBOOT0 or
  *         @arg @ref OB_USER_nBOOT0 or @ref OB_USER_AGC_TRIM or @ref OB_USER_ALL
  * @param  UserConfig The FLASH User Option Bytes values.
  *         This parameter can be a combination of all the following values:
  *         @arg @ref OB_BOR_LEVEL_0 or @ref OB_BOR_LEVEL_1 or ... or @ref OB_BOR_LEVEL_4
  *         @arg @ref OB_STOP_RST or @ref OB_STOP_RST
  *         @arg @ref OB_STANDBY_RST or @ref OB_STANDBY_NORST
  *         @arg @ref OB_SHUTDOWN_RST or @ref OB_SHUTDOWN_NORST
  *         @arg @ref OB_IWDG_SW or @ref OB_IWDG_HW
  *         @arg @ref OB_IWDG_STOP_FREEZE or @ref OB_IWDG_STOP_RUN
  *         @arg @ref OB_IWDG_STDBY_FREEZE or @ref OB_IWDG_STDBY_RUN
  *         @arg @ref OB_WWDG_SW or @ref OB_WWDG_HW
  *         @arg @ref OB_BOOT1_SRAM or @ref OB_BOOT1_SYSTEM
  *         @arg @ref OB_SRAM2_PARITY_ENABLE or @ref OB_SRAM2_PARITY_DISABLE
  *         @arg @ref OB_SRAM2_RST_ERASE or @ref OB_SRAM2_RST_NOT_ERASE
  *         @arg @ref OB_BOOT0_FROM_OB or @ref OB_BOOT0_FROM_PIN
  *         @arg @ref OB_BOOT0_RESET or @ref OB_BOOT0_SET
  *         @arg @ref OB_AGC_TRIM_0 or @ref OB_AGC_TRIM_1 or ... or @ref OB_AGC_TRIM_7
  * @param  RDPLevel: specifies the read protection level.
  *         This parameter can be one of the following values:
  *            @arg @ref OB_RDP_LEVEL_0 No protection
  *            @arg @ref OB_RDP_LEVEL_1 Read protection of the memory
  *            @arg @ref OB_RDP_LEVEL_2 Full chip protection
  * @retval None
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

/**
  * @brief  Configure the Zone 1A, 1B Proprietary code readout protection of the desired addresses,
  *         and erase configuration on RDP regression.
  * @note   To configure the PCROP options, the option lock bit OPTLOCK must be
  *         cleared with the call of the @ref HAL_FLASH_OB_Unlock() function.
  * @note   To validate the PCROP options, the option bytes must be reloaded
  *         through the call of the @ref HAL_FLASH_OB_Launch() function.
  * @param  PCROPConfig: specifies the erase configuration (OB_PCROP_RDP_NOT_ERASE or OB_PCROP_RDP_ERASE)
  *         on RDP level 1 regression.
  * @param  PCROP1AStartAddr Specifies the Zone 1A Start address of the Proprietary code readout protection
  *         This parameter can be an address between begin and end of the flash
  * @param  PCROP1AEndAddr Specifies the Zone 1A end address of the Proprietary code readout protection
  *         This parameter can be an address between PCROP1AStartAddr and end of the flash
  * @param  PCROP1BStartAddr Specifies the Zone 1B Start address of the Proprietary code readout protection
  *         This parameter can be an address between begin and end of the flash
  * @param  PCROP1BEndAddr Specifies the Zone 1B end address of the Proprietary code readout protection
  *         This parameter can be an address between PCROP1BStartAddr and end of the flash
  * @retval None
  */
static void FLASH_OB_PCROPConfig(uint32_t PCROPConfig, uint32_t PCROP1AStartAddr, uint32_t PCROP1AEndAddr, uint32_t PCROP1BStartAddr, uint32_t PCROP1BEndAddr)
{
  uint32_t startoffset;
  uint32_t endoffset;
  uint32_t pcrop1aend;

  /* Check the parameters */
  assert_param(IS_OB_PCROP_CONFIG(PCROPConfig));

  if ((PCROPConfig & OB_PCROP_ZONE_B) == OB_PCROP_ZONE_B)
  {
    assert_param(IS_FLASH_MAIN_MEM_ADDRESS(PCROP1BStartAddr));
    assert_param(IS_FLASH_MAIN_MEM_ADDRESS(PCROP1BEndAddr));

    /* Compute offset depending on pcrop granularity */
    startoffset = ((PCROP1BStartAddr - FLASH_BASE) / FLASH_PCROP_GRANULARITY); /* 2K pages */
    endoffset = ((PCROP1BEndAddr - FLASH_BASE) / FLASH_PCROP_GRANULARITY); /* 2K pages */

    /* Configure the Proprietary code readout protection start address */
    MODIFY_REG(FLASH->PCROP1BSR, FLASH_PCROP1BSR_PCROP1B_STRT, startoffset);

    /* Configure the Proprietary code readout protection end address */
    MODIFY_REG(FLASH->PCROP1BER, FLASH_PCROP1BER_PCROP1B_END, endoffset);
  }

  if ((PCROPConfig & (OB_PCROP_ZONE_A | OB_PCROP_RDP_ERASE)) != 0U)
  {
    assert_param(IS_FLASH_MAIN_MEM_ADDRESS(PCROP1AStartAddr));
    assert_param(IS_FLASH_MAIN_MEM_ADDRESS(PCROP1AEndAddr));

    /* get pcrop 1A end register */
    pcrop1aend = FLASH->PCROP1AER;

    /* Configure the Proprietary code readout protection offset */
    if ((PCROPConfig & OB_PCROP_ZONE_A) != 0U)
    {
      /* Compute offset depending on pcrop granularity */
      startoffset = ((PCROP1AStartAddr - FLASH_BASE) / FLASH_PCROP_GRANULARITY); /* 2K pages */
      endoffset = ((PCROP1AEndAddr - FLASH_BASE) / FLASH_PCROP_GRANULARITY); /* 2K pages */

      /* Set Zone A start offset */
      MODIFY_REG(FLASH->PCROP1ASR, FLASH_PCROP1ASR_PCROP1A_STRT, startoffset);

      /* Set Zone A end offset */
      pcrop1aend &= ~FLASH_PCROP1AER_PCROP1A_END;
      pcrop1aend |= endoffset;
    }

    /* Set RDP erase protection if needed. This bit is only set & will be reset by mass erase */
    if ((PCROPConfig & OB_PCROP_RDP_ERASE) != 0U)
    {
      pcrop1aend |= FLASH_PCROP1AER_PCROP_RDP;
    }

    /* set 1A End register */
    MODIFY_REG(FLASH->PCROP1AER, FLASH_PCROP1AER_PCROP1A_END, pcrop1aend);
  }
}

/**
  * @brief  Program the FLASH IPCC data buffer address.
  * @note   To configure the extra user option bytes, the option lock bit OPTLOCK must
  *         be cleared with the call of the @ref HAL_FLASH_OB_Unlock() function.
  * @note   To validate the extra user option bytes, the option bytes must be reloaded
  *         through the call of the @ref HAL_FLASH_OB_Launch() function.
  * @param  IPCCDataBufAddr IPCC data buffer start address area in SRAM2
  *         This parameter must be the double-word aligned
  * @retval None
  */
static void FLASH_OB_IPCCBufferAddrConfig(uint32_t IPCCDataBufAddr)
{
  assert_param(IS_OB_IPCC_BUF_ADDR(IPCCDataBufAddr));

  /* Configure the option bytes register */
  MODIFY_REG(FLASH->IPCCBR, FLASH_IPCCBR_IPCCDBA, (uint32_t)((IPCCDataBufAddr - SRAM2A_BASE) >> 4));
}

/**
  * @brief  Configure the secure start address of the different memories (FLASH and SRAM2)
  *         , the secure mode and the CPU2 Secure Boot reset vector
  * @note   To configure the PCROP options, the option lock bit OPTLOCK must be
  *         cleared with the call of the @ref HAL_FLASH_OB_Unlock() function.
  * @param  pOBParam Pointer to an @ref FLASH_OBProgramInitTypeDef structure that
  *         contains the configuration information for the programming
  * @retval void
  */
static void FLASH_OB_SecureConfig(FLASH_OBProgramInitTypeDef *pOBParam)
{
  uint32_t sfr_reg_val = READ_REG(FLASH->SFR);
  uint32_t srrvr_reg_val = READ_REG(FLASH->SRRVR);

  if ((pOBParam->OptionType & OPTIONBYTE_SECURE_MODE) != 0U)
  {
    assert_param(IS_OB_SFSA_START_ADDR(pOBParam->SecureFlashStartAddr));
    assert_param(IS_OB_SBRSA_START_ADDR(pOBParam->SecureRAM2aStartAddr));
    assert_param(IS_OB_SNBRSA_START_ADDR(pOBParam->SecureRAM2bStartAddr));
    assert_param(IS_OB_SECURE_MODE(pOBParam->SecureMode));

    /* Configure SFR register content with start PAGE index to secure */
    MODIFY_REG(sfr_reg_val, FLASH_SFR_SFSA, (uint32_t)((pOBParam->SecureFlashStartAddr - FLASH_BASE) / FLASH_PAGE_SIZE));

    /* Configure SRRVR register */
    MODIFY_REG(srrvr_reg_val, (FLASH_SRRVR_SBRSA | FLASH_SRRVR_SNBRSA), \
               (((uint32_t)(((pOBParam->SecureRAM2aStartAddr - SRAM2A_BASE) / SRAM_SECURE_PAGE_GRANULARITY) << FLASH_SRRVR_SBRSA_Pos)) | \
                ((uint32_t)(((pOBParam->SecureRAM2bStartAddr - SRAM2B_BASE) / SRAM_SECURE_PAGE_GRANULARITY) << FLASH_SRRVR_SNBRSA_Pos))));

    /* If Secure mode is requested, clear the corresponding bit */
    /* Else set the corresponding bit */
    if (pOBParam->SecureMode == SYSTEM_IN_SECURE_MODE)
    {
      CLEAR_BIT(sfr_reg_val, FLASH_SFR_FSD);
      CLEAR_BIT(srrvr_reg_val, (FLASH_SRRVR_BRSD | FLASH_SRRVR_NBRSD));
    }
    else
    {
      SET_BIT(sfr_reg_val, FLASH_SFR_FSD);
      SET_BIT(srrvr_reg_val, (FLASH_SRRVR_BRSD | FLASH_SRRVR_NBRSD));
    }

    /* Update Flash registers */
    WRITE_REG(FLASH->SFR, sfr_reg_val);
  }

  /* Boot vector */
  if ((pOBParam->OptionType & OPTIONBYTE_C2_BOOT_VECT) != 0U)
  {
    /* Check the parameters */
    assert_param(IS_OB_BOOT_VECTOR_ADDR(pOBParam->C2SecureBootVectAddr));
    assert_param(IS_OB_BOOT_REGION(pOBParam->C2BootRegion));

    /* Set the boot vector */
    if (pOBParam->C2BootRegion == OB_C2_BOOT_FROM_FLASH)
    {
      MODIFY_REG(srrvr_reg_val, (FLASH_SRRVR_SBRV | FLASH_SRRVR_C2OPT), (uint32_t)((uint32_t)((pOBParam->C2SecureBootVectAddr - FLASH_BASE) >> 2) | pOBParam->C2BootRegion));
    }
    else
    {
      MODIFY_REG(srrvr_reg_val, (FLASH_SRRVR_SBRV | FLASH_SRRVR_C2OPT), (uint32_t)((uint32_t)((pOBParam->C2SecureBootVectAddr - SRAM1_BASE) >> 2) | pOBParam->C2BootRegion));
    }
  }

  /* Update Flash registers */
  WRITE_REG(FLASH->SRRVR, srrvr_reg_val);
}

/**
  * @brief  Return the FLASH Write Protection Option Bytes value.
  * @param[in]   WRPArea Specifies the area to be returned.
  *              This parameter can be one of the following values:
  *              @arg @ref OB_WRPAREA_BANK1_AREAA Flash Bank 1 Area A
  *              @arg @ref OB_WRPAREA_BANK1_AREAB Flash Bank 1 Area B
  * @param[out]  WRPStartOffset Specifies the address where to copied the start page
  *                             of the write protected area
  * @param[out]  WRDPEndOffset Specifies the address where to copied the end page of
  *                            the write protected area
  * @retval None
  */
static void FLASH_OB_GetWRP(uint32_t WRPArea, uint32_t *WRPStartOffset, uint32_t *WRDPEndOffset)
{
  /* Check the parameters */
  assert_param(IS_OB_WRPAREA(WRPArea));

  /* Get the configuration of the write protected area */
  if (WRPArea == OB_WRPAREA_BANK1_AREAA)
  {
    *WRPStartOffset = READ_BIT(FLASH->WRP1AR, FLASH_WRP1AR_WRP1A_STRT);
    *WRDPEndOffset = (READ_BIT(FLASH->WRP1AR, FLASH_WRP1AR_WRP1A_END) >> FLASH_WRP1AR_WRP1A_END_Pos);
  }
  else /* OB_WRPAREA_BANK1_AREAB */
  {
    *WRPStartOffset = READ_BIT(FLASH->WRP1BR, FLASH_WRP1BR_WRP1B_STRT);
    *WRDPEndOffset = (READ_BIT(FLASH->WRP1BR, FLASH_WRP1BR_WRP1B_END) >> FLASH_WRP1BR_WRP1B_END_Pos);
  }
}

/**
  * @brief  Return the FLASH Read Protection level.
  * @retval FLASH ReadOut Protection Status:
  *         This return value can be one of the following values:
  *            @arg @ref OB_RDP_LEVEL_0 No protection
  *            @arg @ref OB_RDP_LEVEL_1 Read protection of the memory
  *            @arg @ref OB_RDP_LEVEL_2 Full chip protection
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
  * @retval This return value can be a combination of all the following values:
  *         @arg @ref OB_BOR_LEVEL_0 or @ref OB_BOR_LEVEL_1 or ... or @ref OB_BOR_LEVEL_4
  *         @arg @ref OB_STOP_RST or @ref OB_STOP_RST
  *         @arg @ref OB_STANDBY_RST or @ref OB_STANDBY_NORST
  *         @arg @ref OB_SHUTDOWN_RST or @ref OB_SHUTDOWN_NORST
  *         @arg @ref OB_IWDG_SW or @ref OB_IWDG_HW
  *         @arg @ref OB_IWDG_STOP_FREEZE or @ref OB_IWDG_STOP_RUN
  *         @arg @ref OB_IWDG_STDBY_FREEZE or @ref OB_IWDG_STDBY_RUN
  *         @arg @ref OB_WWDG_SW or @ref OB_WWDG_HW
  *         @arg @ref OB_BOOT1_SRAM or @ref OB_BOOT1_SYSTEM
  *         @arg @ref OB_SRAM2_PARITY_ENABLE or @ref OB_SRAM2_PARITY_DISABLE
  *         @arg @ref OB_SRAM2_RST_ERASE or @ref OB_SRAM2_RST_NOT_ERASE
  *         @arg @ref OB_BOOT0_FROM_OB or @ref OB_BOOT0_FROM_PIN
  *         @arg @ref OB_BOOT0_RESET or @ref OB_BOOT0_SET
  *         @arg @ref OB_AGC_TRIM_0 or @ref OB_AGC_TRIM_1 or ... or @ref OB_AGC_TRIM_7
  */
static uint32_t FLASH_OB_GetUser(void)
{
  uint32_t user_config = (READ_REG(FLASH->OPTR) & OB_USER_ALL);
  CLEAR_BIT(user_config, FLASH_OPTR_RDP);
  CLEAR_BIT(user_config, FLASH_OPTR_ESE);

  return user_config;
}

/**
  * @brief  Return the FLASH Write Protection Option Bytes value.
  * @param PCROPConfig [out] Specifies the address where to copied the configuration of PCROP_RDP option
  * @param PCROP1AStartAddr [out] Specifies the address where to copied the start address
  *                         of the Zone 1A Proprietary code readout protection
  * @param PCROP1AEndAddr [out] Specifies the address where to copied the end address of
  *                       the Zone 1A Proprietary code readout protection
  * @param PCROP1BStartAddr [out] Specifies the address where to copied the start address
  *                         of the Zone 1B Proprietary code readout protection
  * @param PCROP1BEndAddr [out] Specifies the address where to copied the end address of
  *                       the Zone 1B Proprietary code readout protection
  * @retval None
  */
static void FLASH_OB_GetPCROP(uint32_t *PCROPConfig, uint32_t *PCROP1AStartAddr, uint32_t *PCROP1AEndAddr, uint32_t *PCROP1BStartAddr, uint32_t *PCROP1BEndAddr)
{
  uint32_t pcrop;

  pcrop             = (READ_BIT(FLASH->PCROP1BSR, FLASH_PCROP1BSR_PCROP1B_STRT));
  *PCROP1BStartAddr = ((pcrop * FLASH_PCROP_GRANULARITY) + FLASH_BASE);

  pcrop             = (READ_BIT(FLASH->PCROP1BER, FLASH_PCROP1BER_PCROP1B_END));
  *PCROP1BEndAddr    = ((pcrop * FLASH_PCROP_GRANULARITY) + FLASH_BASE);

  pcrop             = (READ_BIT(FLASH->PCROP1ASR, FLASH_PCROP1ASR_PCROP1A_STRT));
  *PCROP1AStartAddr = ((pcrop * FLASH_PCROP_GRANULARITY) + FLASH_BASE);

  pcrop             = (READ_BIT(FLASH->PCROP1AER, FLASH_PCROP1AER_PCROP1A_END));
  *PCROP1AEndAddr   = ((pcrop * FLASH_PCROP_GRANULARITY) + FLASH_BASE);

  *PCROPConfig      = (READ_REG(FLASH->PCROP1AER) & FLASH_PCROP1AER_PCROP_RDP);
}

/**
  * @brief  Return the FLASH IPCC data buffer base address Option Byte value.
  * @retval Returned value is the IPCC data buffer start address area in SRAM2.
  */
static uint32_t FLASH_OB_GetIPCCBufferAddr(void)
{
  return (uint32_t)((READ_BIT(FLASH->IPCCBR, FLASH_IPCCBR_IPCCDBA) << 4) + SRAM2A_BASE);
}

/**
  * @brief  Return the Secure Flash start address, Secure Backup RAM2a start address, Secure non-Backup RAM2b start address and the SecureMode
  * @param  SecureFlashStartAddr Specifies the address where to copied the Secure Flash start address
  * @param  SecureRAM2aStartAddr Specifies the address where to copied the Secure Backup RAM2a start address
  * @param  SecureRAM2bStartAddr Specifies the address where to copied the Secure non-Backup RAM2b start address
  * @param  SecureMode           Specifies the address where to copied the Secure Mode.
  *                              This return value can be one of the following values:
  *                              @arg @ref SYSTEM_IN_SECURE_MODE : Security enabled
  *                              @arg @ref SYSTEM_NOT_IN_SECURE_MODE : Security disabled
  * @retval None
  */
static void FLASH_OB_GetSecureMemoryConfig(uint32_t *SecureFlashStartAddr, uint32_t *SecureRAM2aStartAddr, uint32_t *SecureRAM2bStartAddr, uint32_t *SecureMode)
{
  uint32_t user_config = (READ_BIT(FLASH->SFR, FLASH_SFR_SFSA) >> FLASH_SFR_SFSA_Pos);

  *SecureFlashStartAddr = ((user_config * FLASH_PAGE_SIZE) + FLASH_BASE);

  user_config = (READ_BIT(FLASH->SRRVR, FLASH_SRRVR_SBRSA) >> FLASH_SRRVR_SBRSA_Pos);

  *SecureRAM2aStartAddr = ((user_config * SRAM_SECURE_PAGE_GRANULARITY) + SRAM2A_BASE);

  user_config = (READ_BIT(FLASH->SRRVR, FLASH_SRRVR_SNBRSA) >> FLASH_SRRVR_SNBRSA_Pos);

  *SecureRAM2bStartAddr = ((user_config * SRAM_SECURE_PAGE_GRANULARITY) + SRAM2B_BASE);

  *SecureMode = (READ_BIT(FLASH->OPTR, FLASH_OPTR_ESE));
}

/**
  * @brief  Return the CPU2 Secure Boot reset vector address and the CPU2 Secure Boot Region
  * @param  C2BootResetVectAddr Specifies the address where to copied the CPU2 Secure Boot reset vector address
  * @param  C2BootResetRegion   Specifies the Secure Boot reset memory region
  * @retval None
  */
static void FLASH_OB_GetC2BootResetConfig(uint32_t *C2BootResetVectAddr, uint32_t *C2BootResetRegion)
{
  *C2BootResetRegion = (READ_BIT(FLASH->SRRVR, FLASH_SRRVR_C2OPT));

  if (*C2BootResetRegion == OB_C2_BOOT_FROM_FLASH)
  {
    *C2BootResetVectAddr = (uint32_t)((READ_BIT(FLASH->SRRVR, FLASH_SRRVR_SBRV) << 2) + FLASH_BASE);
  }
  else
  {
    *C2BootResetVectAddr = (uint32_t)((READ_BIT(FLASH->SRRVR, FLASH_SRRVR_SBRV) << 2) + SRAM1_BASE);
  }
}

/**
  * @brief  Proceed the OB Write Operation.
  * @retval HAL Status
  */
static HAL_StatusTypeDef FLASH_OB_ProceedWriteOperation(void)
{
  HAL_StatusTypeDef status;

  /* Verify that next operation can be proceed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

  if (status == HAL_OK)
  {
    /* Set OPTSTRT Bit */
    SET_BIT(FLASH->CR, FLASH_CR_OPTSTRT);

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
  }

  return status;
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

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
