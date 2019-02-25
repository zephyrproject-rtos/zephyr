/**
  ******************************************************************************
  * @file    stm32wbxx_hal_flash.h
  * @author  MCD Application Team
  * @brief   Header file of FLASH HAL module.
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
#ifndef STM32WBxx_HAL_FLASH_H
#define STM32WBxx_HAL_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_def.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @addtogroup FLASH
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup FLASH_Exported_Types FLASH Exported Types
  * @{
  */

/**
  * @brief  FLASH Erase structure definition
  */
typedef struct
{
  uint32_t TypeErase;   /*!< Mass erase or page erase.
                             This parameter can be a value of @ref FLASH_TYPE_ERASE */
  uint32_t Page;        /*!< Initial Flash page to erase when page erase is enabled
                             This parameter must be a value between 0 and (max number of pages - 1) */
  uint32_t NbPages;     /*!< Number of pages to be erased.
                             This parameter must be a value between 1 and (max number of pages - value of initial page)*/
} FLASH_EraseInitTypeDef;

/**
  * @brief  FLASH Option Bytes Program structure definition
  */
typedef struct
{
  uint32_t OptionType;             /*!< Option byte to be configured.
                                        This parameter can be a combination of the values of @ref FLASH_OB_TYPE */
  uint32_t WRPArea;                 /*!< Write protection area to be programmed (used for OPTIONBYTE_WRP).
                                        Only one WRP area could be programmed at the same time.
                                        This parameter can be value of @ref FLASH_OB_WRP_AREA */
  uint32_t WRPStartOffset;         /*!< Write protection start offset (used for OPTIONBYTE_WRP).
                                        This parameter must be a value between 0 and (max number of pages - 1) */
  uint32_t WRPEndOffset;           /*!< Write protection end offset (used for OPTIONBYTE_WRP).
                                        This parameter must be a value between WRPStartOffset and (max number of pages - 1) */
  uint32_t RDPLevel;               /*!< Set the read protection level (used for OPTIONBYTE_RDP).
                                        This parameter can be a value of @ref FLASH_OB_READ_PROTECTION */
  uint32_t UserType;               /*!< User option byte(s) to be configured (used for OPTIONBYTE_USER).
                                        This parameter can be a combination of @ref FLASH_OB_USER_TYPE */
  uint32_t UserConfig;             /*!< Value of the user option byte (used for OPTIONBYTE_USER).
                                        This parameter can be a combination of the values of
                                            @ref FLASH_OB_USER_AGC_TRIM, @ref FLASH_OB_USER_BOR_LEVEL
                                            @ref FLASH_OB_USER_nRST_STOP, @ref FLASH_OB_USER_nRST_STANDBY,
                                            @ref FLASH_OB_USER_nRST_SHUTDOWN, @ref FLASH_OB_USER_IWDG_SW,
                                            @ref FLASH_OB_USER_IWDG_STOP, @ref FLASH_OB_USER_IWDG_STANDBY,
                                            @ref FLASH_OB_USER_WWDG_SW, @ref FLASH_OB_USER_nBOOT1,
                                            @ref FLASH_OB_USER_SRAM2PE, @ref FLASH_OB_USER_SRAM2RST,
                                            @ref FLASH_OB_USER_nSWBOOT0, @ref FLASH_OB_USER_nBOOT0 */
  uint32_t PCROPConfig;            /*!< Configuration of the PCROP (used for OPTIONBYTE_PCROP).
                                        This parameter must be a combination of values of @ref FLASH_OB_PCROP_ZONE
                                        and @ref FLASH_OB_PCROP_RDP */
  uint32_t PCROP1AStartAddr;       /*!< PCROP Zone A Start address (used for OPTIONBYTE_PCROP). It represents first address of start block
                                        to protect. Make sure this parameter is multiple of PCROP granularity */
  uint32_t PCROP1AEndAddr;         /*!< PCROP Zone A End address (used for OPTIONBYTE_PCROP). It represents first address of end block
                                        to protect. Make sure this parameter is multiple of PCROP granularity */
  uint32_t PCROP1BStartAddr;       /*!< PCROP Zone B Start address (used for OPTIONBYTE_PCROP). It represents first address of start block
                                        to protect. Make sure this parameter is multiple of PCROP granularity */
  uint32_t PCROP1BEndAddr;         /*!< PCROP Zone B End address (used for OPTIONBYTE_PCROP). It represents first address of end block
                                        to protect. Make sure this parameter is multiple of PCROP granularity */
  uint32_t SecureFlashStartAddr;   /*!< Secure Flash start address (used for OPTIONBYTE_SFSA).
                                        This parameter must be a value between begin and end of bank
                                        => Contains the start address of the first 4K page of the secure Flash area */
  uint32_t SecureRAM2aStartAddr;   /*!< Secure Backup RAM2a start address (used for OPTIONBYTE_SBRSA).
                                        This parameter can be a value of @ref FLASH_SRAM2A_ADDRESS_RANGE */
  uint32_t SecureRAM2bStartAddr;   /*!< Secure non-Backup RAM2b start address (used for OPTIONBYTE_SNBRSB)
                                        This parameter can be a value of @ref FLASH_SRAM2B_ADDRESS_RANGE */
  uint32_t SecureMode;             /*!< Secure mode activated or desactivated.
                                        This parameter can be a value of @ref FLASH_OB_SECURITY_MODE */
  uint32_t C2BootRegion;           /*!< CPU2 Secure Boot memory region(used for OPTIONBYTE_C2_BOOT_VECT).
                                        This parameter can be a value of @ref FLASH_OB_C2_BOOT_REGION */
  uint32_t C2SecureBootVectAddr;   /*!< CPU2 Secure Boot reset vector (used for OPTIONBYTE_C2_BOOT_VECT).
                                        This parameter contains the CPU2 boot reset start address within
                                        the selected memory region. Make sure this parameter is word aligned. */
  uint32_t IPCCdataBufAddr;        /*!< IPCC mailbox data buffer base address (used for OPTIONBYTE_IPCC_BUF_ADDR).
                                        This parameter contains the IPCC mailbox data buffer start address area in SRAM2 */
} FLASH_OBProgramInitTypeDef;

/**
* @brief  FLASH handle Structure definition
*/
typedef struct
{
  HAL_LockTypeDef   Lock;              /* FLASH locking object */
  uint32_t          ErrorCode;         /* FLASH error code */
  uint32_t          ProcedureOnGoing;  /* Internal variable to indicate which procedure is ongoing or not in IT context */
  uint32_t          Address;           /* Internal variable to save address selected for program in IT context */
  uint32_t          Page;              /* Internal variable to define the current page which is erasing in IT context */
  uint32_t          NbPagesToErase;    /* Internal variable to save the remaining pages to erase in IT context */
} FLASH_ProcessTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup FLASH_Exported_Constants FLASH Exported Constants
  * @{
  */

/** @defgroup FLASH_KEYS FLASH Keys
  * @{
  */
#define FLASH_KEY1                      0x45670123U  /*!< Flash key1 */
#define FLASH_KEY2                      0xCDEF89ABU  /*!< Flash key2: used with FLASH_KEY1
                                                          to unlock the FLASH registers access */

#define FLASH_OPTKEY1                   0x08192A3BU  /*!< Flash option byte key1 */
#define FLASH_OPTKEY2                   0x4C5D6E7FU  /*!< Flash option byte key2: used with FLASH_OPTKEY1
                                                        to allow option bytes operations */
/**
  * @}
  */

/** @defgroup FLASH_LATENCY FLASH Latency
  * @{
  */
#define FLASH_LATENCY_0                 (FLASH_ACR_LATENCY_0WS)  /*!< FLASH Zero wait state   */
#define FLASH_LATENCY_1                 (FLASH_ACR_LATENCY_1WS)  /*!< FLASH One wait state    */
#define FLASH_LATENCY_2                 (FLASH_ACR_LATENCY_2WS)  /*!< FLASH Two wait states   */
#define FLASH_LATENCY_3                 (FLASH_ACR_LATENCY_3WS)  /*!< FLASH Three wait states */
/**
  * @}
  */

/** @defgroup FLASH_FLAGS FLASH Flags Definition
  * @{
  */
#define FLASH_FLAG_EOP                  FLASH_SR_EOP      /*!< FLASH End of operation flag */
#define FLASH_FLAG_OPERR                FLASH_SR_OPERR    /*!< FLASH Operation error flag */
#define FLASH_FLAG_PROGERR              FLASH_SR_PROGERR  /*!< FLASH Programming error flag */
#define FLASH_FLAG_WRPERR               FLASH_SR_WRPERR   /*!< FLASH Write protection error flag */
#define FLASH_FLAG_PGAERR               FLASH_SR_PGAERR   /*!< FLASH Programming alignment error flag */
#define FLASH_FLAG_SIZERR               FLASH_SR_SIZERR   /*!< FLASH Size error flag  */
#define FLASH_FLAG_PGSERR               FLASH_SR_PGSERR   /*!< FLASH Programming sequence error flag */
#define FLASH_FLAG_MISERR               FLASH_SR_MISERR   /*!< FLASH Fast programming data miss error flag */
#define FLASH_FLAG_FASTERR              FLASH_SR_FASTERR  /*!< FLASH Fast programming error flag */
#define FLASH_FLAG_OPTNV                FLASH_SR_OPTNV    /*!< FLASH User Option OPTVAL indication */
#define FLASH_FLAG_RDERR                FLASH_SR_RDERR    /*!< FLASH PCROP read error flag */
#define FLASH_FLAG_OPTVERR              FLASH_SR_OPTVERR  /*!< FLASH Option validity error flag  */
#define FLASH_FLAG_BSY                  FLASH_SR_BSY      /*!< FLASH Busy flag */
#define FLASH_FLAG_CFGBSY               FLASH_SR_CFGBSY   /*!< FLASH Programming/erase configuration busy */
#define FLASH_FLAG_PESD                 FLASH_SR_PESD     /*!< FLASH Programming/erase operation suspended */
#define FLASH_FLAG_ECCC                 FLASH_ECCR_ECCC   /*!< FLASH ECC correction */
#define FLASH_FLAG_ECCD                 FLASH_ECCR_ECCD   /*!< FLASH ECC detection */

#define FLASH_FLAG_SR_ERROR             (FLASH_FLAG_OPERR   | FLASH_FLAG_PROGERR | FLASH_FLAG_WRPERR | \
                                         FLASH_FLAG_PGAERR  | FLASH_FLAG_SIZERR  | FLASH_FLAG_PGSERR | \
                                         FLASH_FLAG_MISERR  | FLASH_FLAG_FASTERR | FLASH_FLAG_RDERR  | \
                                         FLASH_FLAG_OPTVERR)     /*!< All SR error flags */

#define FLASH_FLAG_ALL_ERRORS           (FLASH_FLAG_SR_ERROR | FLASH_FLAG_ECCC | FLASH_FLAG_ECCD)

/** @defgroup FLASH_INTERRUPT_DEFINITION FLASH Interrupts Definition
  * @brief FLASH Interrupt definition
  * @{
  */
#define FLASH_IT_EOP                    FLASH_CR_EOPIE     /*!< End of FLASH Operation Interrupt source */
#define FLASH_IT_OPERR                  FLASH_CR_ERRIE     /*!< Error Interrupt source */
#define FLASH_IT_RDERR                  FLASH_CR_RDERRIE   /*!< PCROP Read Error Interrupt source */
#define FLASH_IT_ECCC                   (FLASH_ECCR_ECCCIE >> FLASH_ECCR_ECCCIE_Pos)  /*!< ECC Correction Interrupt source */
/**
  * @}
  */

/** @defgroup FLASH_ERROR FLASH Error
  * @{
  */
#define HAL_FLASH_ERROR_NONE            0x00000000U
#define HAL_FLASH_ERROR_OP              FLASH_FLAG_OPERR
#define HAL_FLASH_ERROR_PROG            FLASH_FLAG_PROGERR
#define HAL_FLASH_ERROR_WRP             FLASH_FLAG_WRPERR
#define HAL_FLASH_ERROR_PGA             FLASH_FLAG_PGAERR
#define HAL_FLASH_ERROR_SIZ             FLASH_FLAG_SIZERR
#define HAL_FLASH_ERROR_PGS             FLASH_FLAG_PGSERR
#define HAL_FLASH_ERROR_MIS             FLASH_FLAG_MISERR
#define HAL_FLASH_ERROR_FAST            FLASH_FLAG_FASTERR
#define HAL_FLASH_ERROR_RD              FLASH_FLAG_RDERR
#define HAL_FLASH_ERROR_OPTV            FLASH_FLAG_OPTVERR
#define HAL_FLASH_ERROR_ECCD            FLASH_FLAG_ECCD
/**
  * @}
  */

/** @defgroup FLASH_TYPE_ERASE FLASH Erase Type
  * @{
  */
#define FLASH_TYPEERASE_PAGES           FLASH_CR_PER  /*!< Pages erase only*/
#define FLASH_TYPEERASE_MASSERASE       FLASH_CR_MER  /*!< Flash mass erase activation*/
/**
  * @}
  */

/** @defgroup FLASH_TYPE_PROGRAM FLASH Program Type
  * @{
  */
#define FLASH_TYPEPROGRAM_DOUBLEWORD    FLASH_CR_PG     /*!< Program a double-word (64-bit) at a specified address.*/
#define FLASH_TYPEPROGRAM_FAST          FLASH_CR_FSTPG  /*!< Fast program a 64 row double-word (64-bit) at a specified address.
                                                             And another 64 row double-word (64-bit) will be programmed */
/**
  * @}
  */

/** @defgroup FLASH_OB_TYPE FLASH Option Bytes Type
  * @{
  */
#define OPTIONBYTE_WRP                  0x00000001U  /*!< WRP option byte configuration             */
#define OPTIONBYTE_RDP                  0x00000002U  /*!< RDP option byte configuration             */
#define OPTIONBYTE_USER                 0x00000004U  /*!< User option byte configuration            */
#define OPTIONBYTE_PCROP                0x00000008U  /*!< PCROP option byte configuration           */
#define OPTIONBYTE_IPCC_BUF_ADDR        0x00000010U  /*!< IPCC mailbox buffer address configuration */
#define OPTIONBYTE_C2_BOOT_VECT         0x00000100U  /*!< CPU2 Secure Boot reset vector             */
#define OPTIONBYTE_SECURE_MODE          0x00000200U  /*!< Secure mode on activated or not           */
#define OPTIONBYTE_ALL                  (OPTIONBYTE_WRP   | OPTIONBYTE_RDP           | OPTIONBYTE_USER         | \
                                         OPTIONBYTE_PCROP | OPTIONBYTE_IPCC_BUF_ADDR | OPTIONBYTE_C2_BOOT_VECT | \
                                         OPTIONBYTE_SECURE_MODE) /*!< All option byte configuration */
/**
  * @}
  */

/** @defgroup FLASH_OB_WRP_AREA FLASH WRP Area
  * @{
  */
#define OB_WRPAREA_BANK1_AREAA          0x00000000U  /*!< Flash Area A */
#define OB_WRPAREA_BANK1_AREAB          0x00000001U  /*!< Flash Area B */
/**
  * @}
  */

/** @defgroup FLASH_OB_READ_PROTECTION FLASH Option Bytes Read Protection
  * @{
  */
#define OB_RDP_LEVEL_0                  0x000000AAU
#define OB_RDP_LEVEL_1                  0x000000BBU
#define OB_RDP_LEVEL_2                  0x000000CCU  /*!< Warning: When enabling read protection level 2
                                                          it's no more possible to go back to level 1 or 0 */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_TYPE FLASH Option Bytes User Type
  * @{
  */
#define OB_USER_BOR_LEV                 (FLASH_OPTR_BOR_LEV)    /*!< BOR reset Level */
#define OB_USER_nRST_STOP               (FLASH_OPTR_nRST_STOP)  /*!< Reset generated when entering the stop mode */
#define OB_USER_nRST_STDBY              (FLASH_OPTR_nRST_STDBY) /*!< Reset generated when entering the standby mode */
#define OB_USER_nRST_SHDW               (FLASH_OPTR_nRST_SHDW)  /*!< Reset generated when entering the shutdown mode */
#define OB_USER_IWDG_SW                 (FLASH_OPTR_IWDG_SW)    /*!< Independent watchdog selection */
#define OB_USER_IWDG_STOP               (FLASH_OPTR_IWDG_STOP)  /*!< Independent watchdog counter freeze in stop mode */
#define OB_USER_IWDG_STDBY              (FLASH_OPTR_IWDG_STDBY) /*!< Independent watchdog counter freeze in standby mode */
#define OB_USER_WWDG_SW                 (FLASH_OPTR_WWDG_SW)    /*!< Window watchdog selection */
#define OB_USER_nBOOT1                  (FLASH_OPTR_nBOOT1)     /*!< Boot configuration */
#define OB_USER_SRAM2PE                 (FLASH_OPTR_SRAM2PE)    /*!< SRAM2 parity check enable     */
#define OB_USER_SRAM2RST                (FLASH_OPTR_SRAM2RST)   /*!< SRAM2 erase when system reset */
#define OB_USER_nSWBOOT0                (FLASH_OPTR_nSWBOOT0)   /*!< Software BOOT0 */
#define OB_USER_nBOOT0                  (FLASH_OPTR_nBOOT0)     /*!< nBOOT0 option bit */
#define OB_USER_AGC_TRIM                (FLASH_OPTR_AGC_TRIM)   /*!< Automatic Gain Control Trimming */
#define OB_USER_ALL                     (OB_USER_BOR_LEV    | OB_USER_nRST_STOP | OB_USER_nRST_STDBY | \
                                         OB_USER_nRST_SHDW  | OB_USER_IWDG_SW   | OB_USER_IWDG_STOP  | \
                                         OB_USER_IWDG_STDBY | OB_USER_WWDG_SW   | OB_USER_nBOOT1     | \
                                         OB_USER_SRAM2PE    | OB_USER_SRAM2RST  | OB_USER_nSWBOOT0   | \
                                         OB_USER_nBOOT0     | OB_USER_AGC_TRIM)   /*!< all option bits */

/**
  * @}
  */

/** @defgroup FLASH_OB_USER_AGC_TRIM FLASH Option Bytes Automatic Gain Control Trimming
  * @{
  */
#define OB_AGC_TRIM_0                   0x00000000U                                                              /*!< Automatic Gain Control Trimming Value 0 */
#define OB_AGC_TRIM_1                   (FLASH_OPTR_AGC_TRIM_0)                                                  /*!< Automatic Gain Control Trimming Value 1 */
#define OB_AGC_TRIM_2                   (FLASH_OPTR_AGC_TRIM_1)                                                  /*!< Automatic Gain Control Trimming Value 2 */
#define OB_AGC_TRIM_3                   (FLASH_OPTR_AGC_TRIM_1 | FLASH_OPTR_AGC_TRIM_0)                          /*!< Automatic Gain Control Trimming Value 3 */
#define OB_AGC_TRIM_4                   (FLASH_OPTR_AGC_TRIM_2)                                                  /*!< Automatic Gain Control Trimming Value 4 */
#define OB_AGC_TRIM_5                   (FLASH_OPTR_AGC_TRIM_2 | FLASH_OPTR_AGC_TRIM_0)                          /*!< Automatic Gain Control Trimming Value 5 */
#define OB_AGC_TRIM_6                   (FLASH_OPTR_AGC_TRIM_2 | FLASH_OPTR_AGC_TRIM_1)                          /*!< Automatic Gain Control Trimming Value 6 */
#define OB_AGC_TRIM_7                   (FLASH_OPTR_AGC_TRIM_2 | FLASH_OPTR_AGC_TRIM_1 | FLASH_OPTR_AGC_TRIM_0)  /*!< Automatic Gain Control Trimming Value 7 */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_BOR_LEVEL FLASH Option Bytes User BOR Level
  * @{
  */
#define OB_BOR_LEVEL_0                  0x00000000U                                    /*!< Reset level threshold is around 1.7V */
#define OB_BOR_LEVEL_1                  (FLASH_OPTR_BOR_LEV_0)                         /*!< Reset level threshold is around 2.0V */
#define OB_BOR_LEVEL_2                  (FLASH_OPTR_BOR_LEV_1)                         /*!< Reset level threshold is around 2.2V */
#define OB_BOR_LEVEL_3                  (FLASH_OPTR_BOR_LEV_0 | FLASH_OPTR_BOR_LEV_1)  /*!< Reset level threshold is around 2.5V */
#define OB_BOR_LEVEL_4                  (FLASH_OPTR_BOR_LEV_2)                         /*!< Reset level threshold is around 2.8V */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_nRST_STOP FLASH Option Bytes User Reset On Stop
  * @{
  */
#define OB_STOP_RST                     0x00000000U             /*!< Reset generated when entering the stop mode    */
#define OB_STOP_NORST                   (FLASH_OPTR_nRST_STOP)  /*!< No reset generated when entering the stop mode */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_nRST_STANDBY FLASH Option Bytes User Reset On Standby
  * @{
  */
#define OB_STANDBY_RST                  0x00000000U             /*!< Reset generated when entering the standby mode    */
#define OB_STANDBY_NORST                (FLASH_OPTR_nRST_STDBY) /*!< No reset generated when entering the standby mode */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_nRST_SHUTDOWN FLASH Option Bytes User Reset On Shutdown
  * @{
  */
#define OB_SHUTDOWN_RST                 0x00000000U             /*!< Reset generated when entering the shutdown mode    */
#define OB_SHUTDOWN_NORST               (FLASH_OPTR_nRST_SHDW)  /*!< No reset generated when entering the shutdown mode */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_IWDG_SW FLASH Option Bytes User IWDG Type
  * @{
  */
#define OB_IWDG_HW                      0x00000000U           /*!< Hardware independent watchdog */
#define OB_IWDG_SW                      (FLASH_OPTR_IWDG_SW)  /*!< Software independent watchdog */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_IWDG_STOP FLASH Option Bytes User IWDG Mode On Stop
  * @{
  */
#define OB_IWDG_STOP_FREEZE             0x00000000U             /*!< Independent watchdog counter is frozen in Stop mode  */
#define OB_IWDG_STOP_RUN                (FLASH_OPTR_IWDG_STOP)  /*!< Independent watchdog counter is running in Stop mode */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_IWDG_STANDBY FLASH Option Bytes User IWDG Mode On Standby
  * @{
  */
#define OB_IWDG_STDBY_FREEZE            0x00000000U              /*!< Independent watchdog counter is frozen in Standby mode  */
#define OB_IWDG_STDBY_RUN               (FLASH_OPTR_IWDG_STDBY)  /*!< Independent watchdog counter is running in Standby mode */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_WWDG_SW FLASH Option Bytes User WWDG Type
  * @{
  */
#define OB_WWDG_HW                      0x00000000U           /*!< Hardware window watchdog */
#define OB_WWDG_SW                      (FLASH_OPTR_WWDG_SW)  /*!< Software window watchdog */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_SRAM2PE FLASH Option Bytes SRAM2 parity check
  * @{
  */
#define OB_SRAM2_PARITY_ENABLE          0x00000000U           /*!< SRAM2 parity check enable  */
#define OB_SRAM2_PARITY_DISABLE         (FLASH_OPTR_SRAM2PE)  /*!< SRAM2 parity check disable */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_SRAM2RST FLASH Option Bytes SRAM2 erase when system reset
  * @{
  */
#define OB_SRAM2_RST_ERASE              0x00000000U            /*!< SRAM2 erased when a system reset        */
#define OB_SRAM2_RST_NOT_ERASE          (FLASH_OPTR_SRAM2RST)  /*!< SRAM2 is not erased when a system reset */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_nBOOT1 FLASH Option Bytes User BOOT1 Type
  * @{
  */
#define OB_BOOT1_SRAM                   0x00000000U          /*!< Embedded SRAM is selected as boot space (if BOOT0=1) */
#define OB_BOOT1_SYSTEM                 (FLASH_OPTR_nBOOT1)  /*!< System memory is selected as boot space (if BOOT0=1) */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_nSWBOOT0 FLASH Option Bytes User Software BOOT0
  * @{
  */
#define OB_BOOT0_FROM_OB                0x00000000U            /*!< BOOT0 taken from the option bit nBOOT0 */
#define OB_BOOT0_FROM_PIN               (FLASH_OPTR_nSWBOOT0)  /*!< BOOT0 taken from PH3/BOOT0 pin         */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_nBOOT0 FLASH Option Bytes User nBOOT0 option bit
  * @{
  */
#define OB_BOOT0_RESET                  0x0000000U           /*!< nBOOT0 = 0 */
#define OB_BOOT0_SET                    (FLASH_OPTR_nBOOT0)  /*!< nBOOT0 = 1 */
/**
  * @}
  */

/** @defgroup FLASH_OB_PCROP_ZONE FLASH PCROP ZONE
  * @{
  */
#define OB_PCROP_ZONE_A                 0x00000001U  /*!< PCROP Zone A */
#define OB_PCROP_ZONE_B                 0x00000002U  /*!< PCROP Zone B */
/**
  * @}
  */

/** @defgroup FLASH_OB_PCROP_RDP FLASH Option Bytes PCROP On RDP Level Type
  * @{
  */
#define OB_PCROP_RDP_NOT_ERASE          0x00000000U                  /*!< PCROP area is not erased when the RDP level
                                                                          is decreased from Level 1 to Level 0 */
#define OB_PCROP_RDP_ERASE              (FLASH_PCROP1AER_PCROP_RDP)  /*!< PCROP area is erased when the RDP level is
                                                                          decreased from Level 1 to Level 0 (full mass erase) */
/**
  * @}
  */

/** @defgroup FLASH_OB_SECURITY_MODE Option Bytes FLASH Secure mode
  * @{
  */
#define SYSTEM_NOT_IN_SECURE_MODE       0x00000000U       /*!< Unsecure mode: Security disabled  */
#define SYSTEM_IN_SECURE_MODE           (FLASH_OPTR_ESE)  /*!< Secure mode  : Security enabled   */
/**
  * @}
  */

/** @defgroup FLASH_OB_C2_BOOT_REGION CPU2 Option Bytes Reset Boot Vector
  * @{
  */
#define OB_C2_BOOT_FROM_SRAM            0x00000000U          /*!< CPU2 boot from Sram  */
#define OB_C2_BOOT_FROM_FLASH           (FLASH_SRRVR_C2OPT)  /*!< CPU2 boot from Flash */
/**
  * @}
  */
/**
  * @}
  */

/** @defgroup FLASH_SRAM2A_ADDRESS_RANGE RAM2A address range in secure mode
  * @{
  */

#define SRAM2A_START_SECURE_ADDR_0       0x20030000U  /*  When in secure mode 0x20030000 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_1       0x20030400U  /*  When in secure mode 0x20030400 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_2       0x20030800U  /*  When in secure mode 0x20030800 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_3       0x20030C00U  /*  When in secure mode 0x20030C00 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_4       0x20031000U  /*  When in secure mode 0x20031000 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_5       0x20031400U  /*  When in secure mode 0x20031400 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_6       0x20031800U  /*  When in secure mode 0x20031800 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_7       0x20031C00U  /*  When in secure mode 0x20031C00 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_8       0x20032000U  /*  When in secure mode 0x20032000 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_9       0x20032400U  /*  When in secure mode 0x20032400 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_10      0x20032800U  /*  When in secure mode 0x20032800 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_11      0x20032C00U  /*  When in secure mode 0x20032C00 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_12      0x20033000U  /*  When in secure mode 0x20033000 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_13      0x20033400U  /*  When in secure mode 0x20033400 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_14      0x20033800U  /*  When in secure mode 0x20033800 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_15      0x20033C00U  /*  When in secure mode 0x20033C00 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_16      0x20034000U  /*  When in secure mode 0x20034000 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_17      0x20034400U  /*  When in secure mode 0x20034400 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_18      0x20034800U  /*  When in secure mode 0x20034800 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_19      0x20034C00U  /*  When in secure mode 0x20034C00 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_20      0x20035000U  /*  When in secure mode 0x20035000 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_21      0x20035400U  /*  When in secure mode 0x20035400 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_22      0x20035800U  /*  When in secure mode 0x20035800 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_23      0x20035C00U  /*  When in secure mode 0x20035C00 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_24      0x20036000U  /*  When in secure mode 0x20036000 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_25      0x20036400U  /*  When in secure mode 0x20036400 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_26      0x20036800U  /*  When in secure mode 0x20036800 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_27      0x20036C00U  /*  When in secure mode 0x20036C00 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_28      0x20037000U  /*  When in secure mode 0x20037000 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_29      0x20037400U  /*  When in secure mode 0x20037400 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_30      0x20037800U  /*  When in secure mode 0x20037800 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_START_SECURE_ADDR_31      0x20037C00U  /*  When in secure mode 0x20037C00 - 0x20037FFF is accessible only by M0 Plus  */
#define SRAM2A_FULL_UNSECURE             0x20040000U  /*  The RAM2A is accessible to M0 Plus and M4                                  */

/**
  * @}
  */

/** @defgroup FLASH_SRAM2B_ADDRESS_RANGE RAM2B address range in secure mode
  * @{
  */

#define SRAM2B_START_SECURE_ADDR_0       0x20038000U  /*  When in secure mode 0x20038000 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_1       0x20038400U  /*  When in secure mode 0x20038400 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_2       0x20038800U  /*  When in secure mode 0x20038800 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_3       0x20038C00U  /*  When in secure mode 0x20038C00 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_4       0x20039000U  /*  When in secure mode 0x20039000 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_5       0x20039400U  /*  When in secure mode 0x20039400 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_6       0x20039800U  /*  When in secure mode 0x20039800 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_7       0x20039C00U  /*  When in secure mode 0x20039C00 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_8       0x2003A000U  /*  When in secure mode 0x2003A000 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_9       0x2003A400U  /*  When in secure mode 0x2003A400 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_10      0x2003A800U  /*  When in secure mode 0x2003A800 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_11      0x2003AC00U  /*  When in secure mode 0x2003AC00 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_12      0x2003B000U  /*  When in secure mode 0x2003B000 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_13      0x2003B400U  /*  When in secure mode 0x2003B400 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_14      0x2003B800U  /*  When in secure mode 0x2003B800 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_15      0x2003BC00U  /*  When in secure mode 0x2003BC00 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_16      0x2003C000U  /*  When in secure mode 0x2003C000 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_17      0x2003C400U  /*  When in secure mode 0x2003C400 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_18      0x2003C800U  /*  When in secure mode 0x2003C800 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_19      0x2003CC00U  /*  When in secure mode 0x2003CC00 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_20      0x2003D000U  /*  When in secure mode 0x2003D000 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_21      0x2003D400U  /*  When in secure mode 0x2003D400 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_22      0x2003D800U  /*  When in secure mode 0x2003D800 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_23      0x2003DC00U  /*  When in secure mode 0x2003DC00 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_24      0x2003E000U  /*  When in secure mode 0x2003E000 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_25      0x2003E400U  /*  When in secure mode 0x2003E400 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_26      0x2003E800U  /*  When in secure mode 0x2003E800 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_27      0x2003EC00U  /*  When in secure mode 0x2003EC00 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_28      0x2003F000U  /*  When in secure mode 0x2003F000 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_29      0x2003F400U  /*  When in secure mode 0x2003F400 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_30      0x2003F800U  /*  When in secure mode 0x2003F800 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_START_SECURE_ADDR_31      0x2003FC00U  /*  When in secure mode 0x2003FC00 - 0x2003FFFF is accessible only by M0 Plus  */
#define SRAM2B_FULL_UNSECURE             0x2003FF00U  /*  The RAM2B is accessible to M0 Plus and M4                                  */

/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup FLASH_Exported_Macros FLASH Exported Macros
  *  @brief macros to control FLASH features
  *  @{
  */

/**
  * @brief  Set the FLASH Latency.
  * @param __LATENCY__ FLASH Latency
  *         This parameter can be one of the following values :
  *     @arg @ref FLASH_LATENCY_0 FLASH Zero wait state
  *     @arg @ref FLASH_LATENCY_1 FLASH One wait state
  *     @arg @ref FLASH_LATENCY_2 FLASH Two wait states
  *     @arg @ref FLASH_LATENCY_3 FLASH Three wait states
  * @retval None
  */
#define __HAL_FLASH_SET_LATENCY(__LATENCY__)    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, (__LATENCY__))

/**
  * @brief  Get the FLASH Latency.
  * @retval FLASH Latency
  *         Returned value can be one of the following values :
  *     @arg @ref FLASH_LATENCY_0 FLASH Zero wait state
  *     @arg @ref FLASH_LATENCY_1 FLASH One wait state
  *     @arg @ref FLASH_LATENCY_2 FLASH Two wait states
  *     @arg @ref FLASH_LATENCY_3 FLASH Three wait states
  */
#define __HAL_FLASH_GET_LATENCY()               READ_BIT(FLASH->ACR, FLASH_ACR_LATENCY)

/**
  * @brief  Enable the FLASH prefetch buffer.
  * @retval None
  */
#define __HAL_FLASH_PREFETCH_BUFFER_ENABLE()    SET_BIT(FLASH->ACR, FLASH_ACR_PRFTEN)

/**
  * @brief  Disable the FLASH prefetch buffer.
  * @retval None
  */
#define __HAL_FLASH_PREFETCH_BUFFER_DISABLE()   CLEAR_BIT(FLASH->ACR, FLASH_ACR_PRFTEN)

/**
  * @brief  Enable the FLASH instruction cache.
  * @retval none
  */
#define __HAL_FLASH_INSTRUCTION_CACHE_ENABLE()  SET_BIT(FLASH->ACR, FLASH_ACR_ICEN)

/**
  * @brief  Disable the FLASH instruction cache.
  * @retval none
  */
#define __HAL_FLASH_INSTRUCTION_CACHE_DISABLE() CLEAR_BIT(FLASH->ACR, FLASH_ACR_ICEN)

/**
  * @brief  Enable the FLASH data cache.
  * @retval none
  */
#define __HAL_FLASH_DATA_CACHE_ENABLE()         SET_BIT(FLASH->ACR, FLASH_ACR_DCEN)

/**
  * @brief  Disable the FLASH data cache.
  * @retval none
  */
#define __HAL_FLASH_DATA_CACHE_DISABLE()        CLEAR_BIT(FLASH->ACR, FLASH_ACR_DCEN)

/**
  * @brief  Reset the FLASH instruction Cache.
  * @note   This function must be used only when the Instruction Cache is disabled.
  * @retval None
  */
#define __HAL_FLASH_INSTRUCTION_CACHE_RESET()   do { SET_BIT(FLASH->ACR, FLASH_ACR_ICRST);   \
                                                     CLEAR_BIT(FLASH->ACR, FLASH_ACR_ICRST); \
                                                   } while (0)

/**
  * @brief  Reset the FLASH data Cache.
  * @note   This function must be used only when the data Cache is disabled.
  * @retval None
  */
#define __HAL_FLASH_DATA_CACHE_RESET()          do { SET_BIT(FLASH->ACR, FLASH_ACR_DCRST);   \
                                                     CLEAR_BIT(FLASH->ACR, FLASH_ACR_DCRST); \
                                                   } while (0)

/**
  * @}
  */

/** @defgroup FLASH_Interrupt FLASH Interrupts Macros
 *  @brief macros to handle FLASH interrupts
 * @{
 */

/**
  * @brief  Enable the specified FLASH interrupt.
  * @param __INTERRUPT__ FLASH interrupt
  *         This parameter can be any combination of the following values:
  *     @arg @ref FLASH_IT_EOP End of FLASH Operation Interrupt
  *     @arg @ref FLASH_IT_OPERR Error Interrupt
  *     @arg @ref FLASH_IT_RDERR PCROP Read Error Interrupt
  *     @arg @ref FLASH_IT_ECCC ECC Correction Interrupt
  * @retval none
  */
#define __HAL_FLASH_ENABLE_IT(__INTERRUPT__)    do { if(((__INTERRUPT__) & FLASH_IT_ECCC) != 0U) { SET_BIT(FLASH->ECCR, FLASH_ECCR_ECCCIE); }\
                                                     if(((__INTERRUPT__) & (~FLASH_IT_ECCC)) != 0U) { SET_BIT(FLASH->CR, ((__INTERRUPT__) & (~FLASH_IT_ECCC))); }\
                                                   } while(0)

/**
  * @brief  Disable the specified FLASH interrupt.
  * @param __INTERRUPT__ FLASH interrupt
  *         This parameter can be any combination of the following values:
  *     @arg @ref FLASH_IT_EOP End of FLASH Operation Interrupt
  *     @arg @ref FLASH_IT_OPERR Error Interrupt
  *     @arg @ref FLASH_IT_RDERR PCROP Read Error Interrupt
  *     @arg @ref FLASH_IT_ECCC ECC Correction Interrupt
  * @retval none
  */
#define __HAL_FLASH_DISABLE_IT(__INTERRUPT__)   do { if(((__INTERRUPT__) & FLASH_IT_ECCC) != 0U) { CLEAR_BIT(FLASH->ECCR, FLASH_ECCR_ECCCIE); }\
                                                     if(((__INTERRUPT__) & (~FLASH_IT_ECCC)) != 0U) { CLEAR_BIT(FLASH->CR, ((__INTERRUPT__) & (~FLASH_IT_ECCC))); }\
                                                   } while(0)

/**
  * @brief  Check whether the specified FLASH flag is set or not.
  * @param __FLAG__ specifies the FLASH flag to check.
  *   This parameter can be one of the following values:
  *     @arg @ref FLASH_FLAG_EOP FLASH End of Operation flag
  *     @arg @ref FLASH_FLAG_OPERR FLASH Operation error flag
  *     @arg @ref FLASH_FLAG_PROGERR FLASH Programming error flag
  *     @arg @ref FLASH_FLAG_WRPERR FLASH Write protection error flag
  *     @arg @ref FLASH_FLAG_PGAERR FLASH Programming alignment error flag
  *     @arg @ref FLASH_FLAG_SIZERR FLASH Size error flag
  *     @arg @ref FLASH_FLAG_PGSERR FLASH Programming sequence error flag
  *     @arg @ref FLASH_FLAG_MISERR FLASH Fast programming data miss error flag
  *     @arg @ref FLASH_FLAG_FASTERR FLASH Fast programming error flag
  *     @arg @ref FLASH_FLAG_OPTNV FLASH User Option OPTVAL indication
  *     @arg @ref FLASH_FLAG_RDERR FLASH PCROP read  error flag
  *     @arg @ref FLASH_FLAG_OPTVERR FLASH Option validity error flag
  *     @arg @ref FLASH_FLAG_BSY FLASH write/erase operations in progress flag
  *     @arg @ref FLASH_FLAG_CFGBSY Programming/erase configuration busy
  *     @arg @ref FLASH_FLAG_PESD FLASH Programming/erase operation suspended
  *     @arg @ref FLASH_FLAG_ECCC FLASH one ECC error has been detected and corrected
  *     @arg @ref FLASH_FLAG_ECCD FLASH two ECC errors have been detected
  * @retval The new state of FLASH_FLAG (SET or RESET).
  */
#define __HAL_FLASH_GET_FLAG(__FLAG__)          ((((__FLAG__) & (FLASH_FLAG_ECCC | FLASH_FLAG_ECCD)) != 0U) ? \
                                                 (READ_BIT(FLASH->ECCR, (__FLAG__)) == (__FLAG__))  : \
                                                 (READ_BIT(FLASH->SR,   (__FLAG__)) == (__FLAG__)))
/**
  * @brief  Clear the FLASH's pending flags.
  * @param __FLAG__ specifies the FLASH flags to clear.
  *   This parameter can be any combination of the following values:
  *     @arg @ref FLASH_FLAG_EOP FLASH End of Operation flag
  *     @arg @ref FLASH_FLAG_OPERR FLASH Operation error flag
  *     @arg @ref FLASH_FLAG_PROGERR FLASH Programming error flag
  *     @arg @ref FLASH_FLAG_WRPERR FLASH Write protection error flag
  *     @arg @ref FLASH_FLAG_PGAERR FLASH Programming alignment error flag
  *     @arg @ref FLASH_FLAG_SIZERR FLASH Size error flag
  *     @arg @ref FLASH_FLAG_PGSERR FLASH Programming sequence error flag
  *     @arg @ref FLASH_FLAG_MISERR FLASH Fast programming data miss error flag
  *     @arg @ref FLASH_FLAG_FASTERR FLASH Fast programming error flag
  *     @arg @ref FLASH_FLAG_RDERR FLASH PCROP read  error flag
  *     @arg @ref FLASH_FLAG_OPTVERR FLASH Option validity error flag
  *     @arg @ref FLASH_FLAG_ECCC FLASH one ECC error has been detected and corrected
  *     @arg @ref FLASH_FLAG_ECCD FLASH two ECC errors have been detected
  *     @arg @ref FLASH_FLAG_ALL_ERRORS FLASH All errors flags
  * @retval None
  */
#define __HAL_FLASH_CLEAR_FLAG(__FLAG__)        do { if(((__FLAG__) & (FLASH_FLAG_ECCC | FLASH_FLAG_ECCD)) != 0U) { SET_BIT(FLASH->ECCR, ((__FLAG__) & (FLASH_FLAG_ECCC | FLASH_FLAG_ECCD))); }\
                                                     if(((__FLAG__) & ~(FLASH_FLAG_ECCC | FLASH_FLAG_ECCD)) != 0U) { WRITE_REG(FLASH->SR, ((__FLAG__) & ~(FLASH_FLAG_ECCC | FLASH_FLAG_ECCD))); }\
                                                   } while(0)
/**
  * @}
  */

/* Include FLASH HAL Extended module */
#include "stm32wbxx_hal_flash_ex.h"
/* Exported variables --------------------------------------------------------*/
/** @defgroup FLASH_Exported_Variables FLASH Exported Variables
  * @{
  */
extern FLASH_ProcessTypeDef pFlash;
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup FLASH_Exported_Functions
  * @{
  */

/* Program operation functions  ***********************************************/
/** @addtogroup FLASH_Exported_Functions_Group1
  * @{
  */
HAL_StatusTypeDef  HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data);
HAL_StatusTypeDef  HAL_FLASH_Program_IT(uint32_t TypeProgram, uint32_t Address, uint64_t Data);
/* FLASH IRQ handler method */
void               HAL_FLASH_IRQHandler(void);
/* Callbacks in non blocking modes */
void               HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue);
void               HAL_FLASH_OperationErrorCallback(uint32_t ReturnValue);
/**
  * @}
  */

/* Peripheral Control functions  **********************************************/
/** @addtogroup FLASH_Exported_Functions_Group2
  * @{
  */
HAL_StatusTypeDef  HAL_FLASH_Unlock(void);
HAL_StatusTypeDef  HAL_FLASH_Lock(void);
/* Option bytes control */
HAL_StatusTypeDef  HAL_FLASH_OB_Unlock(void);
HAL_StatusTypeDef  HAL_FLASH_OB_Lock(void);
HAL_StatusTypeDef  HAL_FLASH_OB_Launch(void);
/**
  * @}
  */

/* Peripheral State functions  ************************************************/
/** @addtogroup FLASH_Exported_Functions_Group3
  * @{
  */
uint32_t HAL_FLASH_GetError(void);
/**
  * @}
  */

/**
  * @}
  */

/* Private types --------------------------------------------------------*/
/** @defgroup FLASH_Private_types FLASH Private Types
  * @{
  */
HAL_StatusTypeDef  FLASH_WaitForLastOperation(uint32_t Timeout);
/**
  * @}
  */

/* Private constants --------------------------------------------------------*/
/** @defgroup FLASH_Private_Constants FLASH Private Constants
  * @{
  */
#define FLASH_SIZE                      (((uint32_t)(*((uint16_t *)FLASHSIZE_BASE)) & (0x07FFUL)) << 10U)
#define FLASH_END_ADDR                  (FLASH_BASE + FLASH_SIZE - 1U)

#define FLASH_BANK_SIZE                 FLASH_SIZE   /*!< FLASH Bank Size */
#define FLASH_PAGE_SIZE                 0x00001000U  /*!< FLASH Page Size, 4KBytes */
#define FLASH_TIMEOUT_VALUE             1000U        /*!< FLASH Execution Timeout, 1 s */

#define FLASH_WRP_GRANULARITY           0x00001000U  /*!< FLASH Write Protection Granularity, 4KBytes */
#define FLASH_PCROP_GRANULARITY         0x00000800U  /*!< FLASH Code Readout Protection Granularity, 2KBytes */
#define FLASH_SECURE_PAGE_GRANULARITY   0x00001000U  /*!< FLASH Code Readout Protection Granularity, 4KBytes */

#define FLASH_TYPENONE                  0x00000000u  /*!< No Programming Procedure On Going */
/**
  * @}
  */


/** @defgroup SRAM_MEMORY_SIZE  SRAM memory size
  * @{
  */
#define SRAM_SECURE_PAGE_GRANULARITY    0x00000400U  /*!< Secure SRAM2A and SRAM2B Protection Granularity, 1KBytes */
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup FLASH_Private_Macros FLASH Private Macros
 *  @{
 */
#define IS_FLASH_MAIN_MEM_ADDRESS(__VALUE__)        (((__VALUE__) >= FLASH_BASE) && ((__VALUE__) <= (FLASH_BASE + FLASH_SIZE - 1UL)))

#define IS_FLASH_FAST_PROGRAM_ADDRESS(__VALUE__)    (((__VALUE__) >= FLASH_BASE) && ((__VALUE__) <= (FLASH_BASE + FLASH_SIZE - 256UL)) && (((__VALUE__) % 256UL) == 0UL))

#define IS_FLASH_PROGRAM_MAIN_MEM_ADDRESS(__VALUE__)   (((__VALUE__) >= FLASH_BASE) && ((__VALUE__) <= (FLASH_BASE + FLASH_SIZE - 8UL)) && (((__VALUE__) % 8UL) == 0UL))

#define IS_FLASH_PROGRAM_OTP_ADDRESS(__VALUE__)     (((__VALUE__) >= OTP_AREA_BASE) && ((__VALUE__) <= (OTP_AREA_END_ADDR + 1UL - 8UL)) && (((__VALUE__) % 8UL) == 0UL))

#define IS_FLASH_PROGRAM_ADDRESS(__VALUE__)         ((IS_FLASH_PROGRAM_MAIN_MEM_ADDRESS(__VALUE__)) || (IS_FLASH_PROGRAM_OTP_ADDRESS(__VALUE__)))

#define IS_FLASH_PAGE(__VALUE__)                    ((__VALUE__) <= 0xFFU)

#define IS_ADDR_ALIGNED_64BITS(__VALUE__)           (((__VALUE__) & ~0x7U) == (__VALUE__))

#define IS_FLASH_TYPEERASE(__VALUE__)               (((__VALUE__) == FLASH_TYPEERASE_PAGES) || \
                                                     ((__VALUE__) == FLASH_TYPEERASE_MASSERASE))

#define IS_FLASH_TYPEPROGRAM(__VALUE__)             (((__VALUE__) == FLASH_TYPEPROGRAM_DOUBLEWORD) || \
                                                     ((__VALUE__) == FLASH_TYPEPROGRAM_FAST))

#define IS_OB_BOOT_VECTOR_ADDR(__VALUE__)           ((((__VALUE__) >= FLASH_BASE) && ((__VALUE__) <= (FLASH_BASE + FLASH_SIZE - 1U)))    || \
                                                     (((__VALUE__) >= SRAM1_BASE) && ((__VALUE__) <= (SRAM1_BASE + SRAM1_SIZE - 1U)))    || \
                                                     (((__VALUE__) >= SRAM2A_BASE) && ((__VALUE__) <= (SRAM2A_BASE + SRAM2A_SIZE - 1U))) || \
                                                     (((__VALUE__) >= SRAM2B_BASE) && ((__VALUE__) <= (SRAM2B_BASE + SRAM2B_SIZE - 1U))))

#define IS_OB_BOOT_REGION(__VALUE__)                (((__VALUE__) == OB_C2_BOOT_FROM_FLASH) || ((__VALUE__) == OB_C2_BOOT_FROM_SRAM))

#define IS_OB_SFSA_START_ADDR(__VALUE__)            (((__VALUE__) >= FLASH_BASE) && ((__VALUE__) <= FLASH_END_ADDR) && (((__VALUE__) & ~(uint32_t)0xFFFU) == (__VALUE__)))
#define IS_OB_SBRSA_START_ADDR(__VALUE__)           (((__VALUE__) >= SRAM2A_BASE) && ((__VALUE__) <= (SRAM2A_BASE + SRAM2A_SIZE - 1U)) && (((__VALUE__) & ~0x3FFU) == (__VALUE__)))
#define IS_OB_SNBRSA_START_ADDR(__VALUE__)          (((__VALUE__) >= SRAM2B_BASE) && ((__VALUE__) <= (SRAM2B_BASE + SRAM2B_SIZE - 1U)) && (((__VALUE__) & ~0x3FFU) == (__VALUE__)))
#define IS_OB_SECURE_MODE(__VALUE__)                (((__VALUE__) == SYSTEM_IN_SECURE_MODE) || ((__VALUE__) == SYSTEM_NOT_IN_SECURE_MODE))

#define IS_OPTIONBYTE(__VALUE__)                    (((__VALUE__) <= (OPTIONBYTE_WRP | OPTIONBYTE_RDP | OPTIONBYTE_USER | OPTIONBYTE_PCROP | \
                                                              OPTIONBYTE_IPCC_BUF_ADDR | OPTIONBYTE_C2_BOOT_VECT | OPTIONBYTE_SECURE_MODE)))

#define IS_OB_WRPAREA(__VALUE__)                    (((__VALUE__) == OB_WRPAREA_BANK1_AREAA) || ((__VALUE__) == OB_WRPAREA_BANK1_AREAB))

#define IS_OB_RDP_LEVEL(__VALUE__)                  (((__VALUE__) == OB_RDP_LEVEL_0)   ||\
                                                     ((__VALUE__) == OB_RDP_LEVEL_1)   ||\
                                                     ((__VALUE__) == OB_RDP_LEVEL_2))

#define IS_OB_USER_TYPE(__VALUE__)                  ((((__VALUE__) & OB_USER_ALL) != 0U) && \
                                                     (((__VALUE__) & ~OB_USER_ALL) == 0U))

#define IS_OB_USER_CONFIG(__TYPE__, __VALUE__)      ((((__TYPE__) & OB_USER_BOR_LEV) == OB_USER_BOR_LEV) \
                                                      ? ((((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_BOR_LEV)) == OB_BOR_LEVEL_0) || \
                                                         (((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_BOR_LEV)) == OB_BOR_LEVEL_1) || \
                                                         (((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_BOR_LEV)) == OB_BOR_LEVEL_2) || \
                                                         (((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_BOR_LEV)) == OB_BOR_LEVEL_3) || \
                                                         (((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_BOR_LEV)) == OB_BOR_LEVEL_4)) \
                                                      : ((((__TYPE__) & OB_USER_AGC_TRIM) == OB_USER_AGC_TRIM) \
                                                       ? ((((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_AGC_TRIM)) == OB_AGC_TRIM_0) || \
                                                          (((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_AGC_TRIM)) == OB_AGC_TRIM_1) || \
                                                          (((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_AGC_TRIM)) == OB_AGC_TRIM_2) || \
                                                          (((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_AGC_TRIM)) == OB_AGC_TRIM_3) || \
                                                          (((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_AGC_TRIM)) == OB_AGC_TRIM_4) || \
                                                          (((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_AGC_TRIM)) == OB_AGC_TRIM_5) || \
                                                          (((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_AGC_TRIM)) == OB_AGC_TRIM_6) || \
                                                          (((__VALUE__) & ~(OB_USER_ALL & ~OB_USER_AGC_TRIM)) == OB_AGC_TRIM_7)) \
                                                       : ((~(__TYPE__) & (__VALUE__)) == 0U)))

#define IS_OB_USER_AGC_TRIMMING(__VALUE__)          (((__VALUE__) == OB_AGC_TRIM_0) || ((__VALUE__) == OB_AGC_TRIM_1) || \
                                                     ((__VALUE__) == OB_AGC_TRIM_2) || ((__VALUE__) == OB_AGC_TRIM_3) || \
                                                     ((__VALUE__) == OB_AGC_TRIM_4) || ((__VALUE__) == OB_AGC_TRIM_5) || \
                                                     ((__VALUE__) == OB_AGC_TRIM_6) || ((__VALUE__) == OB_AGC_TRIM_7))

#define IS_OB_USER_BOR_LEVEL(__VALUE__)             (((__VALUE__) == OB_BOR_LEVEL_0) || ((__VALUE__) == OB_BOR_LEVEL_1) || \
                                                     ((__VALUE__) == OB_BOR_LEVEL_2) || ((__VALUE__) == OB_BOR_LEVEL_3) || \
                                                     ((__VALUE__) == OB_BOR_LEVEL_4))

#define IS_OB_PCROP_CONFIG(__VALUE__)               (((__VALUE__) & ~(OB_PCROP_ZONE_A | OB_PCROP_ZONE_B | OB_PCROP_RDP_ERASE)) == 0U)

#define IS_OB_SECURE_CONFIG(__VALUE__)              (((__VALUE__) & ~(OB_SECURE_CONFIG_MEMORY | OB_SECURE_CONFIG_BOOT_RESET)) == 0U)

#define IS_OB_IPCC_BUF_ADDR(__VALUE__)              (IS_OB_SBRSA_START_ADDR(__VALUE__) || IS_OB_SNBRSA_START_ADDR(__VALUE__))

#define IS_FLASH_LATENCY(__VALUE__)                 (((__VALUE__) == FLASH_LATENCY_0) || \
                                                     ((__VALUE__) == FLASH_LATENCY_1) || \
                                                     ((__VALUE__) == FLASH_LATENCY_2) || \
                                                     ((__VALUE__) == FLASH_LATENCY_3))
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

#endif /* STM32WBxx_HAL_FLASH_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
