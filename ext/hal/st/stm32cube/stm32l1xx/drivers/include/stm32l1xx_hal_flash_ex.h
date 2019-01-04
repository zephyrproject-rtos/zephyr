/**
  ******************************************************************************
  * @file    stm32l1xx_hal_flash_ex.h
  * @author  MCD Application Team
  * @brief   Header file of Flash HAL Extended module.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L1xx_HAL_FLASH_EX_H
#define __STM32L1xx_HAL_FLASH_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal_def.h"

/** @addtogroup STM32L1xx_HAL_Driver
  * @{
  */

/** @addtogroup FLASHEx
  * @{
  */ 

/** @addtogroup FLASHEx_Private_Constants
  * @{
  */
#if defined(FLASH_SR_RDERR) && defined(FLASH_SR_OPTVERRUSR)

#define FLASH_FLAG_MASK         ( FLASH_FLAG_EOP        | FLASH_FLAG_ENDHV  | FLASH_FLAG_WRPERR | \
                                  FLASH_FLAG_OPTVERR    | FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | \
                                  FLASH_FLAG_OPTVERRUSR | FLASH_FLAG_RDERR)

#elif defined(FLASH_SR_RDERR)

#define FLASH_FLAG_MASK         ( FLASH_FLAG_EOP        | FLASH_FLAG_ENDHV  | FLASH_FLAG_WRPERR | \
                                  FLASH_FLAG_OPTVERR    | FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | \
                                  FLASH_FLAG_RDERR)

#elif defined(FLASH_SR_OPTVERRUSR)

#define FLASH_FLAG_MASK         ( FLASH_FLAG_EOP        | FLASH_FLAG_ENDHV  | FLASH_FLAG_WRPERR | \
                                  FLASH_FLAG_OPTVERR    | FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | \
                                  FLASH_FLAG_OPTVERRUSR)

#else

#define FLASH_FLAG_MASK         ( FLASH_FLAG_EOP        | FLASH_FLAG_ENDHV  | FLASH_FLAG_WRPERR | \
                                  FLASH_FLAG_OPTVERR    | FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR)

#endif /* FLASH_SR_RDERR & FLASH_SR_OPTVERRUSR */

#if defined(STM32L100xB) || defined(STM32L151xB) || defined(STM32L152xB) || defined(STM32L100xBA) \
 || defined(STM32L151xBA) || defined(STM32L152xBA)
     
/******* Devices with FLASH 128K *******/
#define FLASH_NBPAGES_MAX       512U /* 512 pages from page 0 to page 511U */

#elif defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC) \
   || defined(STM32L151xCA) || defined(STM32L152xCA) || defined(STM32L162xCA)

/******* Devices with FLASH 256K *******/
#define FLASH_NBPAGES_MAX       1025U /* 1025 pages from page 0 to page 1024U */

#elif defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xD) || defined(STM32L152xDX) \
   || defined(STM32L162xD) || defined(STM32L162xDX)

/******* Devices with FLASH 384K *******/
#define FLASH_NBPAGES_MAX       1536U /* 1536 pages from page 0 to page 1535U */

#elif defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE)

/******* Devices with FLASH 512K *******/
#define FLASH_NBPAGES_MAX       2048U /* 2048 pages from page 0 to page 2047U */

#endif /* STM32L100xB || STM32L151xB || STM32L152xB || STM32L100xBA || STM32L151xBA || STM32L152xBA */

#define WRP_MASK_LOW                 (0x0000FFFFU)
#define WRP_MASK_HIGH                (0xFFFF0000U)

/**
  * @}
  */  

/** @addtogroup FLASHEx_Private_Macros
  * @{
  */

#define IS_FLASH_TYPEERASE(__VALUE__)   (((__VALUE__) == FLASH_TYPEERASE_PAGES))

#define IS_OPTIONBYTE(__VALUE__)        (((__VALUE__) <= (OPTIONBYTE_WRP|OPTIONBYTE_RDP|OPTIONBYTE_USER|OPTIONBYTE_BOR)))

#define IS_WRPSTATE(__VALUE__)          (((__VALUE__) == OB_WRPSTATE_DISABLE) || \
                                         ((__VALUE__) == OB_WRPSTATE_ENABLE))
                                         
#define IS_OB_WRP(__PAGE__)             (((__PAGE__) != 0x0000000U))

#define IS_OB_RDP(__LEVEL__)            (((__LEVEL__) == OB_RDP_LEVEL_0) ||\
                                         ((__LEVEL__) == OB_RDP_LEVEL_1) ||\
                                         ((__LEVEL__) == OB_RDP_LEVEL_2))
                                         
#define IS_OB_BOR_LEVEL(__LEVEL__)      (((__LEVEL__) == OB_BOR_OFF)     || \
                                         ((__LEVEL__) == OB_BOR_LEVEL1)  || \
                                         ((__LEVEL__) == OB_BOR_LEVEL2)  || \
                                         ((__LEVEL__) == OB_BOR_LEVEL3)  || \
                                         ((__LEVEL__) == OB_BOR_LEVEL4)  || \
                                         ((__LEVEL__) == OB_BOR_LEVEL5))

#define IS_OB_IWDG_SOURCE(__SOURCE__)   (((__SOURCE__) == OB_IWDG_SW) || ((__SOURCE__) == OB_IWDG_HW))

#define IS_OB_STOP_SOURCE(__SOURCE__)   (((__SOURCE__) == OB_STOP_NORST) || ((__SOURCE__) == OB_STOP_RST))

#define IS_OB_STDBY_SOURCE(__SOURCE__)  (((__SOURCE__) == OB_STDBY_NORST) || ((__SOURCE__) == OB_STDBY_RST))

#if defined(FLASH_OBR_SPRMOD) && defined(FLASH_OBR_nRST_BFB2)
    
#define IS_OBEX(__VALUE__)              (((__VALUE__) == OPTIONBYTE_PCROP) || ((__VALUE__) == OPTIONBYTE_BOOTCONFIG))

#elif defined(FLASH_OBR_SPRMOD) && !defined(FLASH_OBR_nRST_BFB2)

#define IS_OBEX(__VALUE__)              ((__VALUE__) == OPTIONBYTE_PCROP)

#elif !defined(FLASH_OBR_SPRMOD) && defined(FLASH_OBR_nRST_BFB2)

#define IS_OBEX(__VALUE__)              ((__VALUE__) == OPTIONBYTE_BOOTCONFIG)

#endif /* FLASH_OBR_SPRMOD && FLASH_OBR_nRST_BFB2 */

#if defined(FLASH_OBR_SPRMOD)

#define IS_PCROPSTATE(__VALUE__)        (((__VALUE__) == OB_PCROP_STATE_DISABLE) || \
                                         ((__VALUE__) == OB_PCROP_STATE_ENABLE))  

#define IS_OB_PCROP(__PAGE__)           (((__PAGE__) != 0x0000000U))
#endif /* FLASH_OBR_SPRMOD */

#if defined(FLASH_OBR_nRST_BFB2)
    
#define IS_OB_BOOT_BANK(__BANK__)     (((__BANK__) == OB_BOOT_BANK2) || ((__BANK__) == OB_BOOT_BANK1))

#endif /* FLASH_OBR_nRST_BFB2 */

#define IS_TYPEERASEDATA(__VALUE__)     (((__VALUE__) == FLASH_TYPEERASEDATA_BYTE) || \
                                         ((__VALUE__) == FLASH_TYPEERASEDATA_HALFWORD) || \
                                         ((__VALUE__) == FLASH_TYPEERASEDATA_WORD))
#define IS_TYPEPROGRAMDATA(__VALUE__)   (((__VALUE__) == FLASH_TYPEPROGRAMDATA_BYTE) || \
                                         ((__VALUE__) == FLASH_TYPEPROGRAMDATA_HALFWORD) || \
                                         ((__VALUE__) == FLASH_TYPEPROGRAMDATA_WORD) || \
                                         ((__VALUE__) == FLASH_TYPEPROGRAMDATA_FASTBYTE) || \
                                         ((__VALUE__) == FLASH_TYPEPROGRAMDATA_FASTHALFWORD) || \
                                         ((__VALUE__) == FLASH_TYPEPROGRAMDATA_FASTWORD))


/** @defgroup FLASHEx_Address FLASHEx Address
  * @{
  */

#define IS_FLASH_DATA_ADDRESS(__ADDRESS__)          (((__ADDRESS__) >= FLASH_EEPROM_BASE) && ((__ADDRESS__) <= FLASH_EEPROM_END))

#if defined(STM32L100xB) || defined(STM32L151xB) || defined(STM32L152xB) || defined(STM32L100xBA)  \
 || defined(STM32L151xBA) || defined(STM32L152xBA) || defined(STM32L100xC) || defined(STM32L151xC) \
 || defined(STM32L152xC) || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L152xCA) \
 || defined(STM32L162xCA)

#define IS_FLASH_PROGRAM_ADDRESS(__ADDRESS__)       (((__ADDRESS__) >= FLASH_BASE) && ((__ADDRESS__) <= FLASH_END))  

#else /*STM32L151xD || STM32L152xD || STM32L162xD || STM32L151xE || STM32L152xE || STM32L162xE */

#define IS_FLASH_PROGRAM_ADDRESS(__ADDRESS__)       (((__ADDRESS__) >= FLASH_BASE) && ((__ADDRESS__) <= FLASH_BANK2_END))  
#define IS_FLASH_PROGRAM_BANK1_ADDRESS(__ADDRESS__) (((__ADDRESS__) >= FLASH_BASE) && ((__ADDRESS__) <= FLASH_BANK1_END))  
#define IS_FLASH_PROGRAM_BANK2_ADDRESS(__ADDRESS__) (((__ADDRESS__) >= FLASH_BANK2_BASE) && ((__ADDRESS__) <= FLASH_BANK2_END))  

#endif /* STM32L100xB || STM32L151xB || STM32L152xB || (...) || STM32L151xCA || STM32L152xCA || STM32L162xCA */

#define IS_NBPAGES(__PAGES__) (((__PAGES__) >= 1U) && ((__PAGES__) <= FLASH_NBPAGES_MAX)) 

/**
  * @}
  */ 

/**
  * @}
  */  
/* Exported types ------------------------------------------------------------*/ 

/** @defgroup FLASHEx_Exported_Types FLASHEx Exported Types
  * @{
  */  

/**
  * @brief  FLASH Erase structure definition
  */
typedef struct
{
  uint32_t TypeErase;   /*!< TypeErase: Page Erase only.
                             This parameter can be a value of @ref FLASHEx_Type_Erase */

  uint32_t PageAddress; /*!< PageAddress: Initial FLASH address to be erased
                             This parameter must be a value belonging to FLASH Programm address (depending on the devices)  */
  
  uint32_t NbPages;     /*!< NbPages: Number of pages to be erased.
                             This parameter must be a value between 1 and (max number of pages - value of Initial page)*/
  
} FLASH_EraseInitTypeDef;

/**
  * @brief  FLASH Option Bytes PROGRAM structure definition
  */
typedef struct
{
  uint32_t  OptionType;       /*!< OptionType: Option byte to be configured.
                                   This parameter can be a value of @ref FLASHEx_Option_Type */

  uint32_t  WRPState;         /*!< WRPState: Write protection activation or deactivation.
                                   This parameter can be a value of @ref FLASHEx_WRP_State */

  uint32_t  WRPSector0To31;   /*!< WRPSector0To31: specifies the sector(s) which are write protected between Sector 0 to 31
                                   This parameter can be a combination of @ref FLASHEx_Option_Bytes_Write_Protection1 */  
  
#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)    \
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xCA) \
 || defined(STM32L152xD) || defined(STM32L152xDX) || defined(STM32L162xCA) || defined(STM32L162xD)  \
 || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE)
  uint32_t  WRPSector32To63;  /*!< WRPSector32To63: specifies the sector(s) which are write protected between Sector 32 to 63
                                   This parameter can be a combination of @ref FLASHEx_Option_Bytes_Write_Protection2 */  
#endif /* STM32L100xC || STM32L151xC || STM32L152xC || (...) || STM32L151xE || STM32L152xE || STM32L162xE */

#if defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xD) || defined(STM32L152xDX) \
 || defined(STM32L162xD) || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE)  \
 || defined(STM32L162xE)
  uint32_t  WRPSector64To95;  /*!< WRPSector64to95: specifies the sector(s) which are write protected between Sector 64 to 95
                                   This parameter can be a combination of @ref FLASHEx_Option_Bytes_Write_Protection3 */  
#endif /* STM32L151xD || STM32L152xD || STM32L162xD || STM32L151xE || STM32L152xE || STM32L162xE */

#if defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE) || defined(STM32L151xDX) \
 || defined(STM32L152xDX) || defined(STM32L162xDX)
  uint32_t  WRPSector96To127; /*!< WRPSector96To127: specifies the sector(s) which are write protected between Sector 96 to 127 or
                                   Sectors 96 to 111 for STM32L1xxxDX devices.
                                   This parameter can be a combination of @ref FLASHEx_Option_Bytes_Write_Protection4 */  
#endif /* STM32L151xE || STM32L152xE || STM32L162xE || STM32L151xDX || ... */
                              
  uint8_t   RDPLevel;         /*!< RDPLevel: Set the read protection level.
                                   This parameter can be a value of @ref FLASHEx_Option_Bytes_Read_Protection */

  uint8_t   BORLevel;         /*!< BORLevel: Set the BOR Level.
                                   This parameter can be a value of @ref FLASHEx_Option_Bytes_BOR_Level */
                                
  uint8_t   USERConfig;       /*!< USERConfig: Program the FLASH User Option Byte: IWDG_SW / RST_STOP / RST_STDBY.
                                   This parameter can be a combination of @ref FLASHEx_Option_Bytes_IWatchdog, 
                                   @ref FLASHEx_Option_Bytes_nRST_STOP and @ref FLASHEx_Option_Bytes_nRST_STDBY*/
} FLASH_OBProgramInitTypeDef;

#if defined(FLASH_OBR_SPRMOD) || defined(FLASH_OBR_nRST_BFB2)
/**
  * @brief  FLASH Advanced Option Bytes Program structure definition
  */
typedef struct
{
  uint32_t OptionType;          /*!< OptionType: Option byte to be configured for extension .
                                     This parameter can be a value of @ref FLASHEx_OptionAdv_Type */

#if defined(FLASH_OBR_SPRMOD)
  uint32_t PCROPState;          /*!< PCROPState: PCROP activation or deactivation.
                                     This parameter can be a value of @ref FLASHEx_PCROP_State */

  uint32_t  PCROPSector0To31;   /*!< PCROPSector0To31: specifies the sector(s) set for PCROP
                                     This parameter can be a value of @ref FLASHEx_Option_Bytes_PC_ReadWrite_Protection1 */
  
#if defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)
  uint32_t  PCROPSector32To63;  /*!< PCROPSector32To63: specifies the sector(s) set for PCROP
                                     This parameter can be a value of @ref FLASHEx_Option_Bytes_PC_ReadWrite_Protection2 */
#endif /* STM32L151xC || STM32L152xC || STM32L162xC */
#endif /* FLASH_OBR_SPRMOD */
 
#if defined(FLASH_OBR_nRST_BFB2)
  uint16_t BootConfig;          /*!< BootConfig: specifies Option bytes for boot config
                                     This parameter can be a value of @ref FLASHEx_Option_Bytes_BOOT */
#endif /* FLASH_OBR_nRST_BFB2*/
} FLASH_AdvOBProgramInitTypeDef;

/**
  * @}
  */
#endif /* FLASH_OBR_SPRMOD || FLASH_OBR_nRST_BFB2 */

/* Exported constants --------------------------------------------------------*/


/** @defgroup FLASHEx_Exported_Constants FLASHEx Exported Constants
  * @{
  */  

/** @defgroup FLASHEx_Type_Erase FLASHEx_Type_Erase
  * @{
  */
#define FLASH_TYPEERASE_PAGES           (0x00U)  /*!<Page erase only*/

/**
  * @}
  */

/** @defgroup FLASHEx_Option_Type FLASHEx Option Type
  * @{
  */
#define OPTIONBYTE_WRP            (0x01U)  /*!<WRP option byte configuration*/
#define OPTIONBYTE_RDP            (0x02U)  /*!<RDP option byte configuration*/
#define OPTIONBYTE_USER           (0x04U)  /*!<USER option byte configuration*/
#define OPTIONBYTE_BOR            (0x08U)  /*!<BOR option byte configuration*/

/**
  * @}
  */

/** @defgroup FLASHEx_WRP_State FLASHEx WRP State
  * @{
  */
#define OB_WRPSTATE_DISABLE        (0x00U)  /*!<Disable the write protection of the desired sectors*/
#define OB_WRPSTATE_ENABLE         (0x01U)  /*!<Enable the write protection of the desired sectors*/

/**
  * @}
  */

/** @defgroup FLASHEx_Option_Bytes_Write_Protection1 FLASHEx Option Bytes Write Protection1
  * @{
  */
  
/* Common pages for Cat1, Cat2, Cat3, Cat4 & Cat5 devices */
#define OB_WRP1_PAGES0TO15    (0x00000001U) /* Write protection of Sector0 */  
#define OB_WRP1_PAGES16TO31   (0x00000002U) /* Write protection of Sector1 */  
#define OB_WRP1_PAGES32TO47   (0x00000004U) /* Write protection of Sector2 */  
#define OB_WRP1_PAGES48TO63   (0x00000008U) /* Write protection of Sector3 */  
#define OB_WRP1_PAGES64TO79   (0x00000010U) /* Write protection of Sector4 */  
#define OB_WRP1_PAGES80TO95   (0x00000020U) /* Write protection of Sector5 */  
#define OB_WRP1_PAGES96TO111  (0x00000040U) /* Write protection of Sector6 */  
#define OB_WRP1_PAGES112TO127 (0x00000080U) /* Write protection of Sector7 */  
#define OB_WRP1_PAGES128TO143 (0x00000100U) /* Write protection of Sector8 */  
#define OB_WRP1_PAGES144TO159 (0x00000200U) /* Write protection of Sector9 */  
#define OB_WRP1_PAGES160TO175 (0x00000400U) /* Write protection of Sector10 */ 
#define OB_WRP1_PAGES176TO191 (0x00000800U) /* Write protection of Sector11 */ 
#define OB_WRP1_PAGES192TO207 (0x00001000U) /* Write protection of Sector12 */ 
#define OB_WRP1_PAGES208TO223 (0x00002000U) /* Write protection of Sector13 */ 
#define OB_WRP1_PAGES224TO239 (0x00004000U) /* Write protection of Sector14 */ 
#define OB_WRP1_PAGES240TO255 (0x00008000U) /* Write protection of Sector15 */ 
#define OB_WRP1_PAGES256TO271 (0x00010000U) /* Write protection of Sector16 */ 
#define OB_WRP1_PAGES272TO287 (0x00020000U) /* Write protection of Sector17 */ 
#define OB_WRP1_PAGES288TO303 (0x00040000U) /* Write protection of Sector18 */ 
#define OB_WRP1_PAGES304TO319 (0x00080000U) /* Write protection of Sector19 */ 
#define OB_WRP1_PAGES320TO335 (0x00100000U) /* Write protection of Sector20 */ 
#define OB_WRP1_PAGES336TO351 (0x00200000U) /* Write protection of Sector21 */ 
#define OB_WRP1_PAGES352TO367 (0x00400000U) /* Write protection of Sector22 */ 
#define OB_WRP1_PAGES368TO383 (0x00800000U) /* Write protection of Sector23 */ 
#define OB_WRP1_PAGES384TO399 (0x01000000U) /* Write protection of Sector24 */ 
#define OB_WRP1_PAGES400TO415 (0x02000000U) /* Write protection of Sector25 */ 
#define OB_WRP1_PAGES416TO431 (0x04000000U) /* Write protection of Sector26 */ 
#define OB_WRP1_PAGES432TO447 (0x08000000U) /* Write protection of Sector27 */ 
#define OB_WRP1_PAGES448TO463 (0x10000000U) /* Write protection of Sector28 */ 
#define OB_WRP1_PAGES464TO479 (0x20000000U) /* Write protection of Sector29 */ 
#define OB_WRP1_PAGES480TO495 (0x40000000U) /* Write protection of Sector30 */ 
#define OB_WRP1_PAGES496TO511 (0x80000000U) /* Write protection of Sector31 */ 
  
#define OB_WRP1_ALLPAGES      ((uint32_t)FLASH_WRPR1_WRP) /*!< Write protection of all Sectors */
  
/**
  * @}
  */ 

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)    \
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xCA) \
 || defined(STM32L152xD) || defined(STM32L152xDX) || defined(STM32L162xCA) || defined(STM32L162xD)  \
 || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE)

/** @defgroup FLASHEx_Option_Bytes_Write_Protection2 FLASHEx Option Bytes Write Protection2
  * @{
  */
  
/* Pages for Cat3, Cat4 & Cat5 devices*/
#define OB_WRP2_PAGES512TO527   (0x00000001U) /* Write protection of Sector32 */  
#define OB_WRP2_PAGES528TO543   (0x00000002U) /* Write protection of Sector33 */  
#define OB_WRP2_PAGES544TO559   (0x00000004U) /* Write protection of Sector34 */  
#define OB_WRP2_PAGES560TO575   (0x00000008U) /* Write protection of Sector35 */  
#define OB_WRP2_PAGES576TO591   (0x00000010U) /* Write protection of Sector36 */  
#define OB_WRP2_PAGES592TO607   (0x00000020U) /* Write protection of Sector37 */  
#define OB_WRP2_PAGES608TO623   (0x00000040U) /* Write protection of Sector38 */  
#define OB_WRP2_PAGES624TO639   (0x00000080U) /* Write protection of Sector39 */  
#define OB_WRP2_PAGES640TO655   (0x00000100U) /* Write protection of Sector40 */  
#define OB_WRP2_PAGES656TO671   (0x00000200U) /* Write protection of Sector41 */  
#define OB_WRP2_PAGES672TO687   (0x00000400U) /* Write protection of Sector42 */  
#define OB_WRP2_PAGES688TO703   (0x00000800U) /* Write protection of Sector43 */  
#define OB_WRP2_PAGES704TO719   (0x00001000U) /* Write protection of Sector44 */  
#define OB_WRP2_PAGES720TO735   (0x00002000U) /* Write protection of Sector45 */  
#define OB_WRP2_PAGES736TO751   (0x00004000U) /* Write protection of Sector46 */  
#define OB_WRP2_PAGES752TO767   (0x00008000U) /* Write protection of Sector47 */  

#if defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)   \
 || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA) || defined(STM32L152xD) \
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L152xE)  \
 || defined(STM32L162xE)

#define OB_WRP2_PAGES768TO783   (0x00010000U) /* Write protection of Sector48 */  
#define OB_WRP2_PAGES784TO799   (0x00020000U) /* Write protection of Sector49 */  
#define OB_WRP2_PAGES800TO815   (0x00040000U) /* Write protection of Sector50 */  
#define OB_WRP2_PAGES816TO831   (0x00080000U) /* Write protection of Sector51 */  
#define OB_WRP2_PAGES832TO847   (0x00100000U) /* Write protection of Sector52 */  
#define OB_WRP2_PAGES848TO863   (0x00200000U) /* Write protection of Sector53 */  
#define OB_WRP2_PAGES864TO879   (0x00400000U) /* Write protection of Sector54 */  
#define OB_WRP2_PAGES880TO895   (0x00800000U) /* Write protection of Sector55 */  
#define OB_WRP2_PAGES896TO911   (0x01000000U) /* Write protection of Sector56 */  
#define OB_WRP2_PAGES912TO927   (0x02000000U) /* Write protection of Sector57 */  
#define OB_WRP2_PAGES928TO943   (0x04000000U) /* Write protection of Sector58 */  
#define OB_WRP2_PAGES944TO959   (0x08000000U) /* Write protection of Sector59 */  
#define OB_WRP2_PAGES960TO975   (0x10000000U) /* Write protection of Sector60 */  
#define OB_WRP2_PAGES976TO991   (0x20000000U) /* Write protection of Sector61 */  
#define OB_WRP2_PAGES992TO1007  (0x40000000U) /* Write protection of Sector62 */
#define OB_WRP2_PAGES1008TO1023 (0x80000000U) /* Write protection of Sector63 */

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || (...) || STM32L162xD || STM32L151xE || STM32L152xE || STM32L162xE */
      
#define OB_WRP2_ALLPAGES        ((uint32_t)FLASH_WRPR2_WRP) /*!< Write protection of all Sectors */

/**
  * @}
  */ 

#endif /* STM32L100xC || STM32L151xC || STM32L152xC || (...) || STM32L162xD || STM32L151xDX || STM32L152xE || STM32L162xE */

#if defined(STM32L151xD) || defined(STM32L151xDX) || defined(STM32L152xD) || defined(STM32L152xDX) \
 || defined(STM32L162xD) || defined(STM32L162xDX) || defined(STM32L151xE) || defined(STM32L152xE)  \
 || defined(STM32L162xE)

/** @defgroup FLASHEx_Option_Bytes_Write_Protection3 FLASHEx Option Bytes Write Protection3
  * @{
  */
  
/* Pages for devices with FLASH >= 256KB*/
#define OB_WRP3_PAGES1024TO1039 (0x00000001U) /* Write protection of Sector64 */
#define OB_WRP3_PAGES1040TO1055 (0x00000002U) /* Write protection of Sector65 */
#define OB_WRP3_PAGES1056TO1071 (0x00000004U) /* Write protection of Sector66 */
#define OB_WRP3_PAGES1072TO1087 (0x00000008U) /* Write protection of Sector67 */
#define OB_WRP3_PAGES1088TO1103 (0x00000010U) /* Write protection of Sector68 */
#define OB_WRP3_PAGES1104TO1119 (0x00000020U) /* Write protection of Sector69 */
#define OB_WRP3_PAGES1120TO1135 (0x00000040U) /* Write protection of Sector70 */
#define OB_WRP3_PAGES1136TO1151 (0x00000080U) /* Write protection of Sector71 */
#define OB_WRP3_PAGES1152TO1167 (0x00000100U) /* Write protection of Sector72 */
#define OB_WRP3_PAGES1168TO1183 (0x00000200U) /* Write protection of Sector73 */
#define OB_WRP3_PAGES1184TO1199 (0x00000400U) /* Write protection of Sector74 */
#define OB_WRP3_PAGES1200TO1215 (0x00000800U) /* Write protection of Sector75 */
#define OB_WRP3_PAGES1216TO1231 (0x00001000U) /* Write protection of Sector76 */
#define OB_WRP3_PAGES1232TO1247 (0x00002000U) /* Write protection of Sector77 */
#define OB_WRP3_PAGES1248TO1263 (0x00004000U) /* Write protection of Sector78 */
#define OB_WRP3_PAGES1264TO1279 (0x00008000U) /* Write protection of Sector79 */
#define OB_WRP3_PAGES1280TO1295 (0x00010000U) /* Write protection of Sector80 */
#define OB_WRP3_PAGES1296TO1311 (0x00020000U) /* Write protection of Sector81 */
#define OB_WRP3_PAGES1312TO1327 (0x00040000U) /* Write protection of Sector82 */
#define OB_WRP3_PAGES1328TO1343 (0x00080000U) /* Write protection of Sector83 */
#define OB_WRP3_PAGES1344TO1359 (0x00100000U) /* Write protection of Sector84 */
#define OB_WRP3_PAGES1360TO1375 (0x00200000U) /* Write protection of Sector85 */
#define OB_WRP3_PAGES1376TO1391 (0x00400000U) /* Write protection of Sector86 */
#define OB_WRP3_PAGES1392TO1407 (0x00800000U) /* Write protection of Sector87 */
#define OB_WRP3_PAGES1408TO1423 (0x01000000U) /* Write protection of Sector88 */
#define OB_WRP3_PAGES1424TO1439 (0x02000000U) /* Write protection of Sector89 */
#define OB_WRP3_PAGES1440TO1455 (0x04000000U) /* Write protection of Sector90 */
#define OB_WRP3_PAGES1456TO1471 (0x08000000U) /* Write protection of Sector91 */
#define OB_WRP3_PAGES1472TO1487 (0x10000000U) /* Write protection of Sector92 */
#define OB_WRP3_PAGES1488TO1503 (0x20000000U) /* Write protection of Sector93 */
#define OB_WRP3_PAGES1504TO1519 (0x40000000U) /* Write protection of Sector94 */
#define OB_WRP3_PAGES1520TO1535 (0x80000000U) /* Write protection of Sector95 */

#define OB_WRP3_ALLPAGES        ((uint32_t)FLASH_WRPR3_WRP) /*!< Write protection of all Sectors */

/**
  * @}
  */ 

#endif /* STM32L151xD || STM32L152xD || STM32L162xD || STM32L151xE || STM32L152xE || STM32L162xE*/

#if defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE) || defined(STM32L151xDX) \
 || defined(STM32L152xDX) || defined(STM32L162xDX)

/** @defgroup FLASHEx_Option_Bytes_Write_Protection4 FLASHEx Option Bytes Write Protection4
  * @{
  */
  
/* Pages for Cat5 devices*/
#define OB_WRP4_PAGES1536TO1551 (0x00000001U)/* Write protection of Sector96*/   
#define OB_WRP4_PAGES1552TO1567 (0x00000002U)/* Write protection of Sector97*/   
#define OB_WRP4_PAGES1568TO1583 (0x00000004U)/* Write protection of Sector98*/   
#define OB_WRP4_PAGES1584TO1599 (0x00000008U)/* Write protection of Sector99*/   
#define OB_WRP4_PAGES1600TO1615 (0x00000010U) /* Write protection of Sector100*/ 
#define OB_WRP4_PAGES1616TO1631 (0x00000020U) /* Write protection of Sector101*/ 
#define OB_WRP4_PAGES1632TO1647 (0x00000040U) /* Write protection of Sector102*/ 
#define OB_WRP4_PAGES1648TO1663 (0x00000080U) /* Write protection of Sector103*/ 
#define OB_WRP4_PAGES1664TO1679 (0x00000100U) /* Write protection of Sector104*/ 
#define OB_WRP4_PAGES1680TO1695 (0x00000200U) /* Write protection of Sector105*/ 
#define OB_WRP4_PAGES1696TO1711 (0x00000400U) /* Write protection of Sector106*/ 
#define OB_WRP4_PAGES1712TO1727 (0x00000800U) /* Write protection of Sector107*/ 
#define OB_WRP4_PAGES1728TO1743 (0x00001000U) /* Write protection of Sector108*/ 
#define OB_WRP4_PAGES1744TO1759 (0x00002000U) /* Write protection of Sector109*/ 
#define OB_WRP4_PAGES1760TO1775 (0x00004000U) /* Write protection of Sector110*/ 
#define OB_WRP4_PAGES1776TO1791 (0x00008000U) /* Write protection of Sector111*/ 

#if defined(STM32L151xE) || defined(STM32L152xE) || defined(STM32L162xE)

#define OB_WRP4_PAGES1792TO1807 (0x00010000U) /* Write protection of Sector112*/ 
#define OB_WRP4_PAGES1808TO1823 (0x00020000U) /* Write protection of Sector113*/ 
#define OB_WRP4_PAGES1824TO1839 (0x00040000U) /* Write protection of Sector114*/ 
#define OB_WRP4_PAGES1840TO1855 (0x00080000U) /* Write protection of Sector115*/ 
#define OB_WRP4_PAGES1856TO1871 (0x00100000U) /* Write protection of Sector116*/ 
#define OB_WRP4_PAGES1872TO1887 (0x00200000U) /* Write protection of Sector117*/ 
#define OB_WRP4_PAGES1888TO1903 (0x00400000U) /* Write protection of Sector118*/ 
#define OB_WRP4_PAGES1904TO1919 (0x00800000U) /* Write protection of Sector119*/ 
#define OB_WRP4_PAGES1920TO1935 (0x01000000U) /* Write protection of Sector120*/ 
#define OB_WRP4_PAGES1936TO1951 (0x02000000U) /* Write protection of Sector121*/ 
#define OB_WRP4_PAGES1952TO1967 (0x04000000U) /* Write protection of Sector122*/ 
#define OB_WRP4_PAGES1968TO1983 (0x08000000U) /* Write protection of Sector123*/ 
#define OB_WRP4_PAGES1984TO1999 (0x10000000U) /* Write protection of Sector124*/ 
#define OB_WRP4_PAGES2000TO2015 (0x20000000U) /* Write protection of Sector125*/ 
#define OB_WRP4_PAGES2016TO2031 (0x40000000U) /* Write protection of Sector126*/ 
#define OB_WRP4_PAGES2032TO2047 (0x80000000U) /* Write protection of Sector127*/ 

#endif /* STM32L151xE || STM32L152xE || STM32L162xE */

#define OB_WRP4_ALLPAGES        ((uint32_t)FLASH_WRPR4_WRP) /*!< Write protection of all Sectors */

/**
  * @}
  */ 

#endif /* STM32L151xE || STM32L152xE || STM32L162xE || STM32L151xDX || ... */

/** @defgroup FLASHEx_Option_Bytes_Read_Protection FLASHEx Option Bytes Read Protection
  * @{
  */ 
#define OB_RDP_LEVEL_0         ((uint8_t)0xAAU)
#define OB_RDP_LEVEL_1         ((uint8_t)0xBBU)
#define OB_RDP_LEVEL_2         ((uint8_t)0xCCU) /* Warning: When enabling read protection level 2 
                                                it is no more possible to go back to level 1 or 0 */

/**
  * @}
  */ 

/** @defgroup FLASHEx_Option_Bytes_BOR_Level FLASHEx Option Bytes BOR Level
  * @{
  */

#define OB_BOR_OFF       ((uint8_t)0x00U) /*!< BOR is disabled at power down, the reset is asserted when the VDD 
                                              power supply reaches the PDR(Power Down Reset) threshold (1.5V) */
#define OB_BOR_LEVEL1    ((uint8_t)0x08U) /*!< BOR Reset threshold levels for 1.7V - 1.8V VDD power supply    */
#define OB_BOR_LEVEL2    ((uint8_t)0x09U) /*!< BOR Reset threshold levels for 1.9V - 2.0V VDD power supply    */
#define OB_BOR_LEVEL3    ((uint8_t)0x0AU) /*!< BOR Reset threshold levels for 2.3V - 2.4V VDD power supply    */
#define OB_BOR_LEVEL4    ((uint8_t)0x0BU) /*!< BOR Reset threshold levels for 2.55V - 2.65V VDD power supply  */
#define OB_BOR_LEVEL5    ((uint8_t)0x0CU) /*!< BOR Reset threshold levels for 2.8V - 2.9V VDD power supply    */

/**
  * @}
  */
  
/** @defgroup FLASHEx_Option_Bytes_IWatchdog FLASHEx Option Bytes IWatchdog
  * @{
  */

#define OB_IWDG_SW                     ((uint8_t)0x10U)  /*!< Software WDG selected */
#define OB_IWDG_HW                     ((uint8_t)0x00U)  /*!< Hardware WDG selected */

/**
  * @}
  */

/** @defgroup FLASHEx_Option_Bytes_nRST_STOP FLASHEx Option Bytes nRST_STOP
  * @{
  */

#define OB_STOP_NORST                  ((uint8_t)0x20U) /*!< No reset generated when entering in STOP */
#define OB_STOP_RST                    ((uint8_t)0x00U) /*!< Reset generated when entering in STOP */
/**
  * @}
  */

/** @defgroup FLASHEx_Option_Bytes_nRST_STDBY FLASHEx Option Bytes nRST_STDBY
  * @{
  */

#define OB_STDBY_NORST                 ((uint8_t)0x40U) /*!< No reset generated when entering in STANDBY */
#define OB_STDBY_RST                   ((uint8_t)0x00U) /*!< Reset generated when entering in STANDBY */

/**
  * @}
  */

#if defined(FLASH_OBR_SPRMOD)
    
/** @defgroup FLASHEx_OptionAdv_Type FLASHEx Option Advanced Type
  * @{
  */ 
  
#define OPTIONBYTE_PCROP        (0x01U)  /*!<PCROP option byte configuration*/

/**
  * @}
  */

#endif /* FLASH_OBR_SPRMOD */

#if defined(FLASH_OBR_nRST_BFB2)

/** @defgroup FLASHEx_OptionAdv_Type FLASHEx Option Advanced Type
  * @{
  */ 
  
#define OPTIONBYTE_BOOTCONFIG   (0x02U)  /*!<BOOTConfig option byte configuration*/

/**
  * @}
  */

#endif /* FLASH_OBR_nRST_BFB2 */

#if defined(FLASH_OBR_SPRMOD)

/** @defgroup  FLASHEx_PCROP_State FLASHEx PCROP State
  * @{
  */
#define OB_PCROP_STATE_DISABLE        (0x00U)  /*!<Disable PCROP for selected sectors */
#define OB_PCROP_STATE_ENABLE         (0x01U)  /*!<Enable PCROP for selected sectors */
    
/**
  * @}
  */

/** @defgroup  FLASHEx_Selection_Protection_Mode FLASHEx Selection Protection Mode
  * @{
  */
#define OB_PCROP_DESELECTED     ((uint16_t)0x0000U)            /*!< Disabled PCROP, nWPRi bits used for Write Protection on sector i */
#define OB_PCROP_SELECTED       ((uint16_t)FLASH_OBR_SPRMOD)  /*!< Enable PCROP, nWPRi bits used for PCRoP Protection on sector i   */

/**
  * @}
  */
#endif /* FLASH_OBR_SPRMOD */

#if defined(STM32L151xBA) || defined(STM32L152xBA) || defined(STM32L151xC) || defined(STM32L152xC) \
 || defined(STM32L162xC)
/** @defgroup FLASHEx_Option_Bytes_PC_ReadWrite_Protection1 FLASHEx Option Bytes PC ReadWrite Protection 1
  * @{
  */
  
/* Common pages for Cat1, Cat2, Cat3, Cat4 & Cat5 devices */
#define OB_PCROP1_PAGES0TO15    (0x00000001U) /* PC Read/Write  protection of Sector0 */  
#define OB_PCROP1_PAGES16TO31   (0x00000002U) /* PC Read/Write  protection of Sector1 */  
#define OB_PCROP1_PAGES32TO47   (0x00000004U) /* PC Read/Write  protection of Sector2 */  
#define OB_PCROP1_PAGES48TO63   (0x00000008U) /* PC Read/Write  protection of Sector3 */  
#define OB_PCROP1_PAGES64TO79   (0x00000010U) /* PC Read/Write  protection of Sector4 */  
#define OB_PCROP1_PAGES80TO95   (0x00000020U) /* PC Read/Write  protection of Sector5 */  
#define OB_PCROP1_PAGES96TO111  (0x00000040U) /* PC Read/Write  protection of Sector6 */  
#define OB_PCROP1_PAGES112TO127 (0x00000080U) /* PC Read/Write  protection of Sector7 */  
#define OB_PCROP1_PAGES128TO143 (0x00000100U) /* PC Read/Write  protection of Sector8 */  
#define OB_PCROP1_PAGES144TO159 (0x00000200U) /* PC Read/Write  protection of Sector9 */  
#define OB_PCROP1_PAGES160TO175 (0x00000400U) /* PC Read/Write  protection of Sector10 */ 
#define OB_PCROP1_PAGES176TO191 (0x00000800U) /* PC Read/Write  protection of Sector11 */ 
#define OB_PCROP1_PAGES192TO207 (0x00001000U) /* PC Read/Write  protection of Sector12 */ 
#define OB_PCROP1_PAGES208TO223 (0x00002000U) /* PC Read/Write  protection of Sector13 */ 
#define OB_PCROP1_PAGES224TO239 (0x00004000U) /* PC Read/Write  protection of Sector14 */ 
#define OB_PCROP1_PAGES240TO255 (0x00008000U) /* PC Read/Write  protection of Sector15 */ 
#define OB_PCROP1_PAGES256TO271 (0x00010000U) /* PC Read/Write  protection of Sector16 */ 
#define OB_PCROP1_PAGES272TO287 (0x00020000U) /* PC Read/Write  protection of Sector17 */ 
#define OB_PCROP1_PAGES288TO303 (0x00040000U) /* PC Read/Write  protection of Sector18 */ 
#define OB_PCROP1_PAGES304TO319 (0x00080000U) /* PC Read/Write  protection of Sector19 */ 
#define OB_PCROP1_PAGES320TO335 (0x00100000U) /* PC Read/Write  protection of Sector20 */ 
#define OB_PCROP1_PAGES336TO351 (0x00200000U) /* PC Read/Write  protection of Sector21 */ 
#define OB_PCROP1_PAGES352TO367 (0x00400000U) /* PC Read/Write  protection of Sector22 */ 
#define OB_PCROP1_PAGES368TO383 (0x00800000U) /* PC Read/Write  protection of Sector23 */ 
#define OB_PCROP1_PAGES384TO399 (0x01000000U) /* PC Read/Write  protection of Sector24 */ 
#define OB_PCROP1_PAGES400TO415 (0x02000000U) /* PC Read/Write  protection of Sector25 */ 
#define OB_PCROP1_PAGES416TO431 (0x04000000U) /* PC Read/Write  protection of Sector26 */ 
#define OB_PCROP1_PAGES432TO447 (0x08000000U) /* PC Read/Write  protection of Sector27 */ 
#define OB_PCROP1_PAGES448TO463 (0x10000000U) /* PC Read/Write  protection of Sector28 */ 
#define OB_PCROP1_PAGES464TO479 (0x20000000U) /* PC Read/Write  protection of Sector29 */ 
#define OB_PCROP1_PAGES480TO495 (0x40000000U) /* PC Read/Write  protection of Sector30 */ 
#define OB_PCROP1_PAGES496TO511 (0x80000000U) /* PC Read/Write  protection of Sector31 */ 
  
#define OB_PCROP1_ALLPAGES      (0xFFFFFFFFU) /*!< PC Read/Write  protection of all Sectors */
  
/**
  * @}
  */ 
#endif /* STM32L151xBA || STM32L152xBA || STM32L151xC || STM32L152xC || STM32L162xC */

#if defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC)

/** @defgroup FLASHEx_Option_Bytes_PC_ReadWrite_Protection2 FLASHEx Option Bytes PC ReadWrite Protection 2
  * @{
  */
  
/* Pages for Cat3, Cat4 & Cat5 devices*/
#define OB_PCROP2_PAGES512TO527   (0x00000001U) /* PC Read/Write  protection of Sector32 */  
#define OB_PCROP2_PAGES528TO543   (0x00000002U) /* PC Read/Write  protection of Sector33 */  
#define OB_PCROP2_PAGES544TO559   (0x00000004U) /* PC Read/Write  protection of Sector34 */  
#define OB_PCROP2_PAGES560TO575   (0x00000008U) /* PC Read/Write  protection of Sector35 */  
#define OB_PCROP2_PAGES576TO591   (0x00000010U) /* PC Read/Write  protection of Sector36 */  
#define OB_PCROP2_PAGES592TO607   (0x00000020U) /* PC Read/Write  protection of Sector37 */  
#define OB_PCROP2_PAGES608TO623   (0x00000040U) /* PC Read/Write  protection of Sector38 */  
#define OB_PCROP2_PAGES624TO639   (0x00000080U) /* PC Read/Write  protection of Sector39 */  
#define OB_PCROP2_PAGES640TO655   (0x00000100U) /* PC Read/Write  protection of Sector40 */  
#define OB_PCROP2_PAGES656TO671   (0x00000200U) /* PC Read/Write  protection of Sector41 */  
#define OB_PCROP2_PAGES672TO687   (0x00000400U) /* PC Read/Write  protection of Sector42 */  
#define OB_PCROP2_PAGES688TO703   (0x00000800U) /* PC Read/Write  protection of Sector43 */  
#define OB_PCROP2_PAGES704TO719   (0x00001000U) /* PC Read/Write  protection of Sector44 */  
#define OB_PCROP2_PAGES720TO735   (0x00002000U) /* PC Read/Write  protection of Sector45 */  
#define OB_PCROP2_PAGES736TO751   (0x00004000U) /* PC Read/Write  protection of Sector46 */  
#define OB_PCROP2_PAGES752TO767   (0x00008000U) /* PC Read/Write  protection of Sector47 */  
#define OB_PCROP2_PAGES768TO783   (0x00010000U) /* PC Read/Write  protection of Sector48 */  
#define OB_PCROP2_PAGES784TO799   (0x00020000U) /* PC Read/Write  protection of Sector49 */  
#define OB_PCROP2_PAGES800TO815   (0x00040000U) /* PC Read/Write  protection of Sector50 */  
#define OB_PCROP2_PAGES816TO831   (0x00080000U) /* PC Read/Write  protection of Sector51 */  
#define OB_PCROP2_PAGES832TO847   (0x00100000U) /* PC Read/Write  protection of Sector52 */  
#define OB_PCROP2_PAGES848TO863   (0x00200000U) /* PC Read/Write  protection of Sector53 */  
#define OB_PCROP2_PAGES864TO879   (0x00400000U) /* PC Read/Write  protection of Sector54 */  
#define OB_PCROP2_PAGES880TO895   (0x00800000U) /* PC Read/Write  protection of Sector55 */  
#define OB_PCROP2_PAGES896TO911   (0x01000000U) /* PC Read/Write  protection of Sector56 */  
#define OB_PCROP2_PAGES912TO927   (0x02000000U) /* PC Read/Write  protection of Sector57 */  
#define OB_PCROP2_PAGES928TO943   (0x04000000U) /* PC Read/Write  protection of Sector58 */  
#define OB_PCROP2_PAGES944TO959   (0x08000000U) /* PC Read/Write  protection of Sector59 */  
#define OB_PCROP2_PAGES960TO975   (0x10000000U) /* PC Read/Write  protection of Sector60 */  
#define OB_PCROP2_PAGES976TO991   (0x20000000U) /* PC Read/Write  protection of Sector61 */  
#define OB_PCROP2_PAGES992TO1007  (0x40000000U) /* PC Read/Write  protection of Sector62 */
#define OB_PCROP2_PAGES1008TO1023 (0x80000000U) /* PC Read/Write  protection of Sector63 */

#define OB_PCROP2_ALLPAGES        (0xFFFFFFFFU) /*!< PC Read/Write  protection of all Sectors */

/**
  * @}
  */ 
#endif /* STM32L151xC || STM32L152xC || STM32L162xC */

/** @defgroup FLASHEx_Type_Erase_Data FLASHEx Type Erase Data
  * @{
  */
#define FLASH_TYPEERASEDATA_BYTE            (0x00U)  /*!<Erase byte (8-bit) at a specified address.*/
#define FLASH_TYPEERASEDATA_HALFWORD        (0x01U)  /*!<Erase a half-word (16-bit) at a specified address.*/
#define FLASH_TYPEERASEDATA_WORD            (0x02U)  /*!<Erase a word (32-bit) at a specified address.*/

/**
  * @}
  */

/** @defgroup FLASHEx_Type_Program_Data FLASHEx Type Program Data
  * @{
  */
#define FLASH_TYPEPROGRAMDATA_BYTE            (0x00U)  /*!<Program byte (8-bit) at a specified address.*/
#define FLASH_TYPEPROGRAMDATA_HALFWORD        (0x01U)  /*!<Program a half-word (16-bit) at a specified address.*/
#define FLASH_TYPEPROGRAMDATA_WORD            (0x02U)  /*!<Program a word (32-bit) at a specified address.*/
#define FLASH_TYPEPROGRAMDATA_FASTBYTE        (0x04U)  /*!<Fast Program byte (8-bit) at a specified address.*/
#define FLASH_TYPEPROGRAMDATA_FASTHALFWORD    (0x08U)  /*!<Fast Program a half-word (16-bit) at a specified address.*/
#define FLASH_TYPEPROGRAMDATA_FASTWORD        (0x10U)  /*!<Fast Program a word (32-bit) at a specified address.*/

/**
  * @}
  */

#if defined(FLASH_OBR_nRST_BFB2)
    
/** @defgroup FLASHEx_Option_Bytes_BOOT FLASHEx Option Bytes BOOT
  * @{
  */

#define OB_BOOT_BANK2                 ((uint8_t)0x00U) /*!< At startup, if boot pins are set in boot from user Flash position
                                                            and this parameter is selected the device will boot from Bank 2 
                                                            or Bank 1, depending on the activation of the bank */
#define OB_BOOT_BANK1                 ((uint8_t)(FLASH_OBR_nRST_BFB2 >> 16U)) /*!< At startup, if boot pins are set in boot from user Flash position
                                                            and this parameter is selected the device will boot from Bank1(Default) */

/**
  * @}
  */
#endif /* FLASH_OBR_nRST_BFB2 */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/

/** @defgroup FLASHEx_Exported_Macros FLASHEx Exported Macros
 *  @{
 */
 
/**
  * @brief  Set the FLASH Latency.
  * @param  __LATENCY__ FLASH Latency
  *          This parameter can be one of the following values:
  *            @arg @ref FLASH_LATENCY_0  FLASH Zero Latency cycle
  *            @arg @ref FLASH_LATENCY_1  FLASH One Latency cycle
  * @retval none
  */ 
#define __HAL_FLASH_SET_LATENCY(__LATENCY__)  do  { \
                                                  if ((__LATENCY__) == FLASH_LATENCY_1) {__HAL_FLASH_ACC64_ENABLE();} \
                                                  MODIFY_REG((FLASH->ACR), FLASH_ACR_LATENCY, (__LATENCY__)); \
                                              } while(0U)

/**
  * @brief  Get the FLASH Latency.
  * @retval FLASH Latency                   
  *          This parameter can be one of the following values:
  *            @arg @ref FLASH_LATENCY_0  FLASH Zero Latency cycle
  *            @arg @ref FLASH_LATENCY_1  FLASH One Latency cycle
  */ 
#define __HAL_FLASH_GET_LATENCY()     (READ_BIT((FLASH->ACR), FLASH_ACR_LATENCY))

/**
  * @brief  Enable the FLASH 64-bit access.
  * @note    Read access 64 bit is used.
  * @note    This bit cannot be written at the same time as the LATENCY and 
  *          PRFTEN bits.
  * @retval none
  */ 
#define __HAL_FLASH_ACC64_ENABLE()    (SET_BIT((FLASH->ACR), FLASH_ACR_ACC64))

  /**
  * @brief  Disable the FLASH 64-bit access.
  * @note     Read access 32 bit is used
  * @note     To reset this bit, the LATENCY should be zero wait state and the 
  *               prefetch off.
  * @retval none
  */ 
#define __HAL_FLASH_ACC64_DISABLE()   (CLEAR_BIT((FLASH->ACR), FLASH_ACR_ACC64))

/**
  * @brief  Enable the FLASH prefetch buffer.
  * @retval none
  */ 
#define __HAL_FLASH_PREFETCH_BUFFER_ENABLE()    do  { __HAL_FLASH_ACC64_ENABLE(); \
                                                  SET_BIT((FLASH->ACR), FLASH_ACR_PRFTEN); \
                                                } while(0U)

/**
  * @brief  Disable the FLASH prefetch buffer.
  * @retval none
  */ 
#define __HAL_FLASH_PREFETCH_BUFFER_DISABLE()     CLEAR_BIT((FLASH->ACR), FLASH_ACR_PRFTEN)

/**
  * @brief  Enable the FLASH power down during Sleep mode
  * @retval none
  */ 
#define __HAL_FLASH_SLEEP_POWERDOWN_ENABLE()      SET_BIT(FLASH->ACR, FLASH_ACR_SLEEP_PD)

/**
  * @brief  Disable the FLASH power down during Sleep mode
  * @retval none
  */ 
#define __HAL_FLASH_SLEEP_POWERDOWN_DISABLE()     CLEAR_BIT(FLASH->ACR, FLASH_ACR_SLEEP_PD)

/**
  * @brief  Enable the Flash Run power down mode.
  * @note   Writing this bit  to 0 this bit, automatically the keys are
  *         loss and a new unlock sequence is necessary to re-write it to 1.
  */
#define __HAL_FLASH_POWER_DOWN_ENABLE() do { FLASH->PDKEYR = FLASH_PDKEY1;    \
                                             FLASH->PDKEYR = FLASH_PDKEY2;    \
                                             SET_BIT((FLASH->ACR), FLASH_ACR_RUN_PD);  \
                                           } while (0U)

/**
  * @brief  Disable the Flash Run power down mode.
  * @note   Writing this bit to 0 this bit, automatically the keys are
  *         loss and a new unlock sequence is necessary to re-write it to 1.
  */
#define __HAL_FLASH_POWER_DOWN_DISABLE() do { FLASH->PDKEYR = FLASH_PDKEY1;    \
                                              FLASH->PDKEYR = FLASH_PDKEY2;    \
                                             CLEAR_BIT((FLASH->ACR), FLASH_ACR_RUN_PD);  \
                                            } while (0U)
                                            
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @addtogroup FLASHEx_Exported_Functions
  * @{
  */

/** @addtogroup FLASHEx_Exported_Functions_Group1
  * @{
  */

HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *PageError);
HAL_StatusTypeDef HAL_FLASHEx_Erase_IT(FLASH_EraseInitTypeDef *pEraseInit);

/**
  * @}
  */

/** @addtogroup FLASHEx_Exported_Functions_Group2
  * @{
  */

HAL_StatusTypeDef HAL_FLASHEx_OBProgram(FLASH_OBProgramInitTypeDef *pOBInit);
void              HAL_FLASHEx_OBGetConfig(FLASH_OBProgramInitTypeDef *pOBInit);

#if defined(FLASH_OBR_SPRMOD) || defined(FLASH_OBR_nRST_BFB2)
    
HAL_StatusTypeDef HAL_FLASHEx_AdvOBProgram (FLASH_AdvOBProgramInitTypeDef *pAdvOBInit);
void              HAL_FLASHEx_AdvOBGetConfig(FLASH_AdvOBProgramInitTypeDef *pAdvOBInit);

#endif /* FLASH_OBR_SPRMOD || FLASH_OBR_nRST_BFB2 */

#if defined(FLASH_OBR_SPRMOD)

HAL_StatusTypeDef HAL_FLASHEx_OB_SelectPCROP(void);
HAL_StatusTypeDef HAL_FLASHEx_OB_DeSelectPCROP(void);

#endif /* FLASH_OBR_SPRMOD */

/**
  * @}
  */

/** @addtogroup FLASHEx_Exported_Functions_Group3
  * @{
  */

HAL_StatusTypeDef HAL_FLASHEx_DATAEEPROM_Unlock(void);
HAL_StatusTypeDef HAL_FLASHEx_DATAEEPROM_Lock(void);

HAL_StatusTypeDef HAL_FLASHEx_DATAEEPROM_Erase(uint32_t TypeErase, uint32_t Address);
HAL_StatusTypeDef HAL_FLASHEx_DATAEEPROM_Program(uint32_t TypeProgram, uint32_t Address, uint32_t Data);
void              HAL_FLASHEx_DATAEEPROM_EnableFixedTimeProgram(void);
void              HAL_FLASHEx_DATAEEPROM_DisableFixedTimeProgram(void);

/**
  * @}
  */

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

#endif /* __STM32L1xx_HAL_FLASH_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
