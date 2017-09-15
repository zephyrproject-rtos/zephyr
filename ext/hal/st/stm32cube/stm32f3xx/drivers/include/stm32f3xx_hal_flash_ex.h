/**
  ******************************************************************************
  * @file    stm32f3xx_hal_flash_ex.h
  * @author  MCD Application Team
  * @brief   Header file of Flash HAL Extended module.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F3xx_HAL_FLASH_EX_H
#define __STM32F3xx_HAL_FLASH_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal_def.h"

/** @addtogroup STM32F3xx_HAL_Driver
  * @{
  */

/** @addtogroup FLASHEx
  * @{
  */

/** @addtogroup FLASHEx_Private_Constants
  * @{
  */

#define FLASH_SIZE_DATA_REGISTER (0x1FFFF7CCU)

/**
  * @}
  */

/** @addtogroup FLASHEx_Private_Macros
  * @{
  */
#define IS_FLASH_TYPEERASE(VALUE) (((VALUE) == FLASH_TYPEERASE_PAGES) || \
                             ((VALUE) == FLASH_TYPEERASE_MASSERASE))

#define IS_OPTIONBYTE(VALUE) ((VALUE) <= (OPTIONBYTE_WRP | OPTIONBYTE_RDP | OPTIONBYTE_USER | OPTIONBYTE_DATA))

#define IS_WRPSTATE(VALUE) (((VALUE) == OB_WRPSTATE_DISABLE) || \
                            ((VALUE) == OB_WRPSTATE_ENABLE))

#define IS_OB_DATA_ADDRESS(ADDRESS) (((ADDRESS) == OB_DATA_ADDRESS_DATA0) || ((ADDRESS) == OB_DATA_ADDRESS_DATA1))

#define IS_OB_RDP_LEVEL(LEVEL)     (((LEVEL) == OB_RDP_LEVEL_0)   ||\
                                    ((LEVEL) == OB_RDP_LEVEL_1))/*||\
                                    ((LEVEL) == OB_RDP_LEVEL_2))*/

#define IS_OB_IWDG_SOURCE(SOURCE)  (((SOURCE) == OB_IWDG_SW) || ((SOURCE) == OB_IWDG_HW))

#define IS_OB_STOP_SOURCE(SOURCE)  (((SOURCE) == OB_STOP_NO_RST) || ((SOURCE) == OB_STOP_RST))

#define IS_OB_STDBY_SOURCE(SOURCE) (((SOURCE) == OB_STDBY_NO_RST) || ((SOURCE) == OB_STDBY_RST))

#define IS_OB_BOOT1(BOOT1)         (((BOOT1) == OB_BOOT1_RESET) || ((BOOT1) == OB_BOOT1_SET))

#define IS_OB_VDDA_ANALOG(ANALOG)  (((ANALOG) == OB_VDDA_ANALOG_ON) || ((ANALOG) == OB_VDDA_ANALOG_OFF))

#define IS_OB_SRAM_PARITY(PARITY)  (((PARITY) == OB_SRAM_PARITY_SET) || ((PARITY) == OB_SRAM_PARITY_RESET))


#if defined(FLASH_OBR_SDADC12_VDD_MONITOR)
#define IS_OB_SDACD_VDD_MONITOR(VDD_MONITOR) (((VDD_MONITOR) == OB_SDACD_VDD_MONITOR_SET) || \
                                              ((VDD_MONITOR) == OB_SDACD_VDD_MONITOR_RESET))
#endif /* FLASH_OBR_SDADC12_VDD_MONITOR */

#define IS_OB_WRP(PAGE) (((PAGE) != 0x0000000U))

#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) \
 || defined(STM32F373xC) || defined(STM32F378xx)
#define IS_FLASH_PROGRAM_ADDRESS(ADDRESS) (((ADDRESS) >= FLASH_BASE) && (((*((uint16_t *)FLASH_SIZE_DATA_REGISTER)) == 0x100U) ? \
                                           ((ADDRESS) <= 0x0803FFFFU) :  (((*((uint16_t *)FLASH_SIZE_DATA_REGISTER)) == 0x80U) ? \
                                           ((ADDRESS) <= 0x0801FFFFU) :  ((ADDRESS) <= 0x0800FFFFU))))
#endif /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)
#define IS_FLASH_PROGRAM_ADDRESS(ADDRESS) (((ADDRESS) >= FLASH_BASE) && ((ADDRESS) <= 0x0807FFFFU))
#endif /* STM32F302xE || STM32F303xE || STM32F398xx */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx) \
 || defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
#define IS_FLASH_PROGRAM_ADDRESS(ADDRESS) (((ADDRESS) >= FLASH_BASE) && (((*((uint16_t *)FLASH_SIZE_DATA_REGISTER)) == 0x40U) ? \
                                           ((ADDRESS) <= 0x0800FFFFU) :  (((*((uint16_t *)FLASH_SIZE_DATA_REGISTER)) == 0x20U) ? \
                                           ((ADDRESS) <= 0x08007FFFU) :  ((ADDRESS) <= 0x08003FFFU))))
#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx */

#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) \
 || defined(STM32F373xC) || defined(STM32F378xx)
#define IS_FLASH_NB_PAGES(ADDRESS,NBPAGES) (((*((uint16_t *)FLASH_SIZE_DATA_REGISTER)) == 0x100U) ? ((ADDRESS)+((NBPAGES)*FLASH_PAGE_SIZE)-1U <= 0x0803FFFFU) : \
                                           (((*((uint16_t *)FLASH_SIZE_DATA_REGISTER)) == 0x80U)  ? ((ADDRESS)+((NBPAGES)*FLASH_PAGE_SIZE)-1U <= 0x0801FFFFU) : \
                                            ((ADDRESS)+((NBPAGES)*FLASH_PAGE_SIZE)-1U <= 0x0800FFFFU)))
#endif /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)
#define IS_FLASH_NB_PAGES(ADDRESS,NBPAGES) ((ADDRESS)+((NBPAGES)*FLASH_PAGE_SIZE)-1U <= 0x0807FFFFU)
#endif /* STM32F302xE || STM32F303xE || STM32F398xx */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx) \
 || defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
#define IS_FLASH_NB_PAGES(ADDRESS,NBPAGES) (((*((uint16_t *)FLASH_SIZE_DATA_REGISTER)) == 0x40U) ? ((ADDRESS)+((NBPAGES)*FLASH_PAGE_SIZE)-1U <= 0x0800FFFFU) : \
                                           (((*((uint16_t *)FLASH_SIZE_DATA_REGISTER)) == 0x20U) ? ((ADDRESS)+((NBPAGES)*FLASH_PAGE_SIZE)-1U <= 0x08007FFFU) : \
                                            ((ADDRESS)+((NBPAGES)*FLASH_PAGE_SIZE)-1U <= 0x08003FFFU)))
#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx */

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
  uint32_t TypeErase;   /*!< TypeErase: Mass erase or page erase.
                             This parameter can be a value of @ref FLASHEx_Type_Erase */

  uint32_t PageAddress; /*!< PageAdress: Initial FLASH page address to erase when mass erase is disabled
                             This parameter must be a number between Min_Data = FLASH_BASE and Max_Data = FLASH_BANK1_END */

  uint32_t NbPages;     /*!< NbPages: Number of pagess to be erased.
                             This parameter must be a value between Min_Data = 1 and Max_Data = (max number of pages - value of initial page)*/

} FLASH_EraseInitTypeDef;

/**
  * @brief  FLASH Options bytes program structure definition
  */
typedef struct
{
  uint32_t OptionType;  /*!< OptionType: Option byte to be configured.
                             This parameter can be a value of @ref FLASHEx_OB_Type */

  uint32_t WRPState;    /*!< WRPState: Write protection activation or deactivation.
                             This parameter can be a value of @ref FLASHEx_OB_WRP_State */

  uint32_t WRPPage;     /*!< WRPPage: specifies the page(s) to be write protected
                             This parameter can be a value of @ref FLASHEx_OB_Write_Protection */

  uint8_t RDPLevel;     /*!< RDPLevel: Set the read protection level..
                             This parameter can be a value of @ref FLASHEx_OB_Read_Protection */

  uint8_t USERConfig;   /*!< USERConfig: Program the FLASH User Option Byte:
                             IWDG / STOP / STDBY / BOOT1 / VDDA_ANALOG / SRAM_PARITY / SDADC12_VDD_MONITOR
                             This parameter can be a combination of @ref FLASHEx_OB_IWatchdog, @ref FLASHEx_OB_nRST_STOP,
                             @ref FLASHEx_OB_nRST_STDBY, @ref FLASHEx_OB_BOOT1, @ref FLASHEx_OB_VDDA_Analog_Monitoring,
                             @ref FLASHEx_OB_RAM_Parity_Check_Enable.
                             @if STM32F373xC
                             And @ref FLASHEx_OB_SDADC12_VDD_MONITOR (only for STM32F373xC & STM32F378xx devices)
                             @endif
                             @if STM32F378xx
                             And @ref FLASHEx_OB_SDADC12_VDD_MONITOR (only for STM32F373xC & STM32F378xx devices)
                             @endif
                             */

  uint32_t DATAAddress; /*!< DATAAddress: Address of the option byte DATA to be programmed
                             This parameter can be a value of @ref FLASHEx_OB_Data_Address */

  uint8_t DATAData;     /*!< DATAData: Data to be stored in the option byte DATA
                             This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFFU */
} FLASH_OBProgramInitTypeDef;
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup FLASHEx_Exported_Constants FLASHEx Exported Constants
  * @{
  */

/** @defgroup FLASHEx_Page_Size FLASHEx Page Size
  * @{
  */
#define FLASH_PAGE_SIZE          0x800
/**
  * @}
  */

/** @defgroup FLASHEx_Type_Erase FLASH Type Erase
  * @{
  */
#define FLASH_TYPEERASE_PAGES     (0x00U)  /*!<Pages erase only*/
#define FLASH_TYPEERASE_MASSERASE (0x01U)  /*!<Flash mass erase activation*/

/**
  * @}
  */

/** @defgroup FLASHEx_OptionByte_Constants Option Byte Constants
  * @{
  */

/** @defgroup FLASHEx_OB_Type Option Bytes Type
  * @{
  */
#define OPTIONBYTE_WRP       (0x01U)  /*!<WRP option byte configuration*/
#define OPTIONBYTE_RDP       (0x02U)  /*!<RDP option byte configuration*/
#define OPTIONBYTE_USER      (0x04U)  /*!<USER option byte configuration*/
#define OPTIONBYTE_DATA      (0x08U)  /*!<DATA option byte configuration*/

/**
  * @}
  */

/** @defgroup FLASHEx_OB_WRP_State Option Byte WRP State
  * @{
  */
#define OB_WRPSTATE_DISABLE   (0x00U)  /*!<Disable the write protection of the desired pages*/
#define OB_WRPSTATE_ENABLE    (0x01U)  /*!<Enable the write protection of the desired pagess*/

/**
  * @}
  */

/** @defgroup FLASHEx_OB_Write_Protection FLASHEx OB Write Protection
  * @{
  */
#define OB_WRP_PAGES0TO1               (0x00000001U) /* Write protection of page 0 to 1 */
#define OB_WRP_PAGES2TO3               (0x00000002U) /* Write protection of page 2 to 3 */
#define OB_WRP_PAGES4TO5               (0x00000004U) /* Write protection of page 4 to 5 */
#define OB_WRP_PAGES6TO7               (0x00000008U) /* Write protection of page 6 to 7 */
#define OB_WRP_PAGES8TO9               (0x00000010U) /* Write protection of page 8 to 9 */
#define OB_WRP_PAGES10TO11             (0x00000020U) /* Write protection of page 10 to 11 */
#define OB_WRP_PAGES12TO13             (0x00000040U) /* Write protection of page 12 to 13 */
#define OB_WRP_PAGES14TO15             (0x00000080U) /* Write protection of page 14 to 15 */
#define OB_WRP_PAGES16TO17             (0x00000100U) /* Write protection of page 16 to 17 */
#define OB_WRP_PAGES18TO19             (0x00000200U) /* Write protection of page 18 to 19 */
#define OB_WRP_PAGES20TO21             (0x00000400U) /* Write protection of page 20 to 21 */
#define OB_WRP_PAGES22TO23             (0x00000800U) /* Write protection of page 22 to 23 */
#define OB_WRP_PAGES24TO25             (0x00001000U) /* Write protection of page 24 to 25 */
#define OB_WRP_PAGES26TO27             (0x00002000U) /* Write protection of page 26 to 27 */
#define OB_WRP_PAGES28TO29             (0x00004000U) /* Write protection of page 28 to 29 */
#define OB_WRP_PAGES30TO31             (0x00008000U) /* Write protection of page 30 to 31 */

#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) \
 || defined(STM32F373xC) || defined(STM32F378xx)
#define OB_WRP_PAGES32TO33             (0x00010000U) /* Write protection of page 32 to 33 */
#define OB_WRP_PAGES34TO35             (0x00020000U) /* Write protection of page 34 to 35 */
#define OB_WRP_PAGES36TO37             (0x00040000U) /* Write protection of page 36 to 37 */
#define OB_WRP_PAGES38TO39             (0x00080000U) /* Write protection of page 38 to 39 */
#define OB_WRP_PAGES40TO41             (0x00100000U) /* Write protection of page 40 to 41 */
#define OB_WRP_PAGES42TO43             (0x00200000U) /* Write protection of page 42 to 43 */
#define OB_WRP_PAGES44TO45             (0x00400000U) /* Write protection of page 44 to 45 */
#define OB_WRP_PAGES46TO47             (0x00800000U) /* Write protection of page 46 to 47 */
#define OB_WRP_PAGES48TO49             (0x01000000U) /* Write protection of page 48 to 49 */
#define OB_WRP_PAGES50TO51             (0x02000000U) /* Write protection of page 50 to 51 */
#define OB_WRP_PAGES52TO53             (0x04000000U) /* Write protection of page 52 to 53 */
#define OB_WRP_PAGES54TO55             (0x08000000U) /* Write protection of page 54 to 55 */
#define OB_WRP_PAGES56TO57             (0x10000000U) /* Write protection of page 56 to 57 */
#define OB_WRP_PAGES58TO59             (0x20000000U) /* Write protection of page 58 to 59 */
#define OB_WRP_PAGES60TO61             (0x40000000U) /* Write protection of page 60 to 61 */
#define OB_WRP_PAGES62TO127            (0x80000000U) /* Write protection of page 62 to 127 */
#endif /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F373xC || STM32F378xx                   */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)
#define OB_WRP_PAGES32TO33             (0x00010000U) /* Write protection of page 32 to 33 */
#define OB_WRP_PAGES34TO35             (0x00020000U) /* Write protection of page 34 to 35 */
#define OB_WRP_PAGES36TO37             (0x00040000U) /* Write protection of page 36 to 37 */
#define OB_WRP_PAGES38TO39             (0x00080000U) /* Write protection of page 38 to 39 */
#define OB_WRP_PAGES40TO41             (0x00100000U) /* Write protection of page 40 to 41 */
#define OB_WRP_PAGES42TO43             (0x00200000U) /* Write protection of page 42 to 43 */
#define OB_WRP_PAGES44TO45             (0x00400000U) /* Write protection of page 44 to 45 */
#define OB_WRP_PAGES46TO47             (0x00800000U) /* Write protection of page 46 to 47 */
#define OB_WRP_PAGES48TO49             (0x01000000U) /* Write protection of page 48 to 49 */
#define OB_WRP_PAGES50TO51             (0x02000000U) /* Write protection of page 50 to 51 */
#define OB_WRP_PAGES52TO53             (0x04000000U) /* Write protection of page 52 to 53 */
#define OB_WRP_PAGES54TO55             (0x08000000U) /* Write protection of page 54 to 55 */
#define OB_WRP_PAGES56TO57             (0x10000000U) /* Write protection of page 56 to 57 */
#define OB_WRP_PAGES58TO59             (0x20000000U) /* Write protection of page 58 to 59 */
#define OB_WRP_PAGES60TO61             (0x40000000U) /* Write protection of page 60 to 61 */
#define OB_WRP_PAGES62TO255            (0x80000000U) /* Write protection of page 62 to 255 */
#endif /* STM32F302xE || STM32F303xE || STM32F398xx */

#define OB_WRP_PAGES0TO15MASK          (0x000000FFU)
#define OB_WRP_PAGES16TO31MASK         (0x0000FF00U)

#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) \
 || defined(STM32F373xC) || defined(STM32F378xx)
#define OB_WRP_PAGES32TO47MASK         (0x00FF0000U)
#define OB_WRP_PAGES48TO127MASK        (0xFF000000U)
#endif /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F373xC || STM32F378xx                   */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)
#define OB_WRP_PAGES32TO47MASK         (0x00FF0000U)
#define OB_WRP_PAGES48TO255MASK        (0xFF000000U)
#endif /* STM32F302xE || STM32F303xE || STM32F398xx */

#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) \
 || defined(STM32F373xC) || defined(STM32F378xx)
#define OB_WRP_PAGES32TO47MASK         (0x00FF0000U)
#define OB_WRP_PAGES48TO127MASK        (0xFF000000U)
#endif /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F373xC || STM32F378xx                   */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) \
 || defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) \
 || defined(STM32F373xC) || defined(STM32F378xx)
#define OB_WRP_ALLPAGES                (0xFFFFFFFFU) /*!< Write protection of all pages */
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F373xC || STM32F378xx                   */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx) \
 || defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
#define OB_WRP_ALLPAGES                (0x0000FFFFU) /*!< Write protection of all pages */
#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx    */

/**
  * @}
  */

/** @defgroup FLASHEx_OB_Read_Protection Option Byte Read Protection
  * @{
  */
#define OB_RDP_LEVEL_0             ((uint8_t)0xAAU)
#define OB_RDP_LEVEL_1             ((uint8_t)0xBBU)
#define OB_RDP_LEVEL_2             ((uint8_t)0xCCU) /*!< Warning: When enabling read protection level 2
                                                      it's no more possible to go back to level 1 or 0U */
/**
  * @}
  */

/** @defgroup FLASHEx_OB_IWatchdog Option Byte IWatchdog
  * @{
  */
#define OB_IWDG_SW                 ((uint8_t)0x01U)  /*!< Software IWDG selected */
#define OB_IWDG_HW                 ((uint8_t)0x00U)  /*!< Hardware IWDG selected */
/**
  * @}
  */

/** @defgroup FLASHEx_OB_nRST_STOP Option Byte nRST STOP
  * @{
  */
#define OB_STOP_NO_RST             ((uint8_t)0x02U) /*!< No reset generated when entering in STOP */
#define OB_STOP_RST                ((uint8_t)0x00U) /*!< Reset generated when entering in STOP */
/**
  * @}
  */

/** @defgroup FLASHEx_OB_nRST_STDBY Option Byte nRST STDBY
  * @{
  */
#define OB_STDBY_NO_RST            ((uint8_t)0x04U) /*!< No reset generated when entering in STANDBY */
#define OB_STDBY_RST               ((uint8_t)0x00U) /*!< Reset generated when entering in STANDBY */
/**
  * @}
  */

/** @defgroup FLASHEx_OB_BOOT1 Option Byte BOOT1
  * @{
  */
#define OB_BOOT1_RESET             ((uint8_t)0x00U) /*!< BOOT1 Reset */
#define OB_BOOT1_SET               ((uint8_t)0x10U) /*!< BOOT1 Set */
/**
  * @}
  */

/** @defgroup FLASHEx_OB_VDDA_Analog_Monitoring Option Byte VDDA Analog Monitoring
  * @{
  */
#define OB_VDDA_ANALOG_ON          ((uint8_t)0x20U) /*!< Analog monitoring on VDDA Power source ON */
#define OB_VDDA_ANALOG_OFF         ((uint8_t)0x00U) /*!< Analog monitoring on VDDA Power source OFF */
/**
  * @}
  */

/** @defgroup FLASHEx_OB_RAM_Parity_Check_Enable Option Byte SRAM Parity Check Enable
  * @{
  */
#define OB_SRAM_PARITY_SET         ((uint8_t)0x00U) /*!< SRAM parity check enable set */
#define OB_SRAM_PARITY_RESET       ((uint8_t)0x40U) /*!< SRAM parity check enable reset */
/**
  * @}
  */


#if defined(FLASH_OBR_SDADC12_VDD_MONITOR)
/** @defgroup FLASHEx_OB_SDADC12_VDD_MONITOR OB SDADC12 VDD MONITOR
  * @{
  */
#define OB_SDACD_VDD_MONITOR_RESET           ((uint8_t)0x00U) /*!< SDADC VDD Monitor reset */
#define OB_SDACD_VDD_MONITOR_SET             ((uint8_t)0x80U) /*!< SDADC VDD Monitor set */

/**
  * @}
  */
#endif /* FLASH_OBR_SDADC12_VDD_MONITOR */

/** @defgroup FLASHEx_OB_Data_Address  Option Byte Data Address
  * @{
  */
#define OB_DATA_ADDRESS_DATA0     (0x1FFFF804U)
#define OB_DATA_ADDRESS_DATA1     (0x1FFFF806U)
/**
  * @}
  */

/**
  * @}
  */

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
/* IO operation functions *****************************************************/
HAL_StatusTypeDef  HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *PageError);
HAL_StatusTypeDef  HAL_FLASHEx_Erase_IT(FLASH_EraseInitTypeDef *pEraseInit);

/**
  * @}
  */

/** @addtogroup FLASHEx_Exported_Functions_Group2
  * @{
  */
/* Peripheral Control functions ***********************************************/
HAL_StatusTypeDef  HAL_FLASHEx_OBErase(void);
HAL_StatusTypeDef  HAL_FLASHEx_OBProgram(FLASH_OBProgramInitTypeDef *pOBInit);
void               HAL_FLASHEx_OBGetConfig(FLASH_OBProgramInitTypeDef *pOBInit);
uint32_t           HAL_FLASHEx_OBGetUserData(uint32_t DATAAdress);

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

#endif /* __STM32F3xx_HAL_FLASH_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

