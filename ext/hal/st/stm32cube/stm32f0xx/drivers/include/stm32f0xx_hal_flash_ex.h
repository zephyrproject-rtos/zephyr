/**
  ******************************************************************************
  * @file    stm32f0xx_hal_flash_ex.h
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
#ifndef __STM32F0xx_HAL_FLASH_EX_H
#define __STM32F0xx_HAL_FLASH_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal_def.h"

/** @addtogroup STM32F0xx_HAL_Driver
  * @{
  */

/** @addtogroup FLASHEx
  * @{
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

#if defined(FLASH_OBR_BOOT_SEL)
#define IS_OB_BOOT_SEL(BOOT_SEL)   (((BOOT_SEL) == OB_BOOT_SEL_RESET) || ((BOOT_SEL) == OB_BOOT_SEL_SET))
#define IS_OB_BOOT0(BOOT0)         (((BOOT0) == OB_BOOT0_RESET) || ((BOOT0) == OB_BOOT0_SET))
#endif /* FLASH_OBR_BOOT_SEL */


#define IS_OB_WRP(PAGE) (((PAGE) != 0x0000000U))

#define IS_FLASH_NB_PAGES(ADDRESS,NBPAGES) ((ADDRESS)+((NBPAGES)*FLASH_PAGE_SIZE)-1 <= FLASH_BANK1_END)

#define IS_FLASH_PROGRAM_ADDRESS(ADDRESS) (((ADDRESS) >= FLASH_BASE) && ((ADDRESS) <= FLASH_BANK1_END))

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
                             IWDG / STOP / STDBY / BOOT1 / VDDA_ANALOG / SRAM_PARITY
                             This parameter can be a combination of @ref FLASHEx_OB_IWatchdog, @ref FLASHEx_OB_nRST_STOP,
                             @ref FLASHEx_OB_nRST_STDBY, @ref FLASHEx_OB_BOOT1, @ref FLASHEx_OB_VDDA_Analog_Monitoring and
                             @ref FLASHEx_OB_RAM_Parity_Check_Enable */

  uint32_t DATAAddress; /*!< DATAAddress: Address of the option byte DATA to be programmed
                             This parameter can be a value of @ref FLASHEx_OB_Data_Address */

  uint8_t DATAData;     /*!< DATAData: Data to be stored in the option byte DATA
                             This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF */
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
#if defined(STM32F030x6) || defined(STM32F030x8) || defined(STM32F031x6) || defined(STM32F038xx) \
 || defined(STM32F051x8) || defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F058xx) || defined(STM32F070x6)
#define FLASH_PAGE_SIZE          0x400U
#endif /* STM32F030x6 || STM32F030x8 || STM32F031x6 || STM32F051x8 || STM32F042x6 || STM32F048xx || STM32F058xx || STM32F070x6 */

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB) \
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)
#define FLASH_PAGE_SIZE          0x800U
#endif /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F091xC || STM32F098xx || STM32F030xC */
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
#if defined(STM32F030x6) || defined(STM32F030x8) || defined(STM32F031x6) || defined(STM32F038xx) \
 || defined(STM32F051x8) || defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F058xx) || defined(STM32F070x6)
#define OB_WRP_PAGES0TO3               (0x00000001U) /* Write protection of page 0 to 3 */
#define OB_WRP_PAGES4TO7               (0x00000002U) /* Write protection of page 4 to 7 */
#define OB_WRP_PAGES8TO11              (0x00000004U) /* Write protection of page 8 to 11 */
#define OB_WRP_PAGES12TO15             (0x00000008U) /* Write protection of page 12 to 15 */
#define OB_WRP_PAGES16TO19             (0x00000010U) /* Write protection of page 16 to 19 */
#define OB_WRP_PAGES20TO23             (0x00000020U) /* Write protection of page 20 to 23 */
#define OB_WRP_PAGES24TO27             (0x00000040U) /* Write protection of page 24 to 27 */
#define OB_WRP_PAGES28TO31             (0x00000080U) /* Write protection of page 28 to 31 */
#if defined(STM32F030x8) || defined(STM32F051x8) || defined(STM32F058xx)
#define OB_WRP_PAGES32TO35             (0x00000100U) /* Write protection of page 32 to 35 */
#define OB_WRP_PAGES36TO39             (0x00000200U) /* Write protection of page 36 to 39 */
#define OB_WRP_PAGES40TO43             (0x00000400U) /* Write protection of page 40 to 43 */
#define OB_WRP_PAGES44TO47             (0x00000800U) /* Write protection of page 44 to 47 */
#define OB_WRP_PAGES48TO51             (0x00001000U) /* Write protection of page 48 to 51 */
#define OB_WRP_PAGES52TO57             (0x00002000U) /* Write protection of page 52 to 57 */
#define OB_WRP_PAGES56TO59             (0x00004000U) /* Write protection of page 56 to 59 */
#define OB_WRP_PAGES60TO63             (0x00008000U) /* Write protection of page 60 to 63 */
#endif /* STM32F030x8 || STM32F051x8 || STM32F058xx */

#if defined(STM32F030x6) || defined(STM32F030x8) || defined(STM32F031x6) || defined(STM32F038xx) \
 || defined(STM32F051x8) || defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F058xx) || defined(STM32F070x6)
#define OB_WRP_PAGES0TO31MASK          (0x000000FFU)
#endif /* STM32F030x6 || STM32F030x8 || STM32F031x6 || STM32F051x8 || STM32F042x6 || STM32F048xx || STM32F038xx || STM32F058xx || STM32F070x6 */

#if defined(STM32F030x8) || defined(STM32F051x8) || defined(STM32F058xx)
#define OB_WRP_PAGES32TO63MASK         (0x0000FF00U)
#endif /* STM32F030x8 || STM32F051x8 || STM32F058xx */

#if defined(STM32F030x6) || defined(STM32F031x6) || defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F038xx)|| defined(STM32F070x6)
#define OB_WRP_ALLPAGES                (0x000000FFU) /*!< Write protection of all pages */
#endif /* STM32F030x6 || STM32F031x6 || STM32F042x6 || STM32F048xx || STM32F038xx || STM32F070x6 */

#if defined(STM32F030x8) || defined(STM32F051x8) || defined(STM32F058xx)
#define OB_WRP_ALLPAGES                (0x0000FFFFU) /*!< Write protection of all pages */
#endif /* STM32F030x8 || STM32F051x8 || STM32F058xx */
#endif /* STM32F030x6 || STM32F030x8 || STM32F031x6 || STM32F051x8 || STM32F042x6 || STM32F048xx || STM32F038xx || STM32F058xx || STM32F070x6 */

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB) \
 || defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)
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
#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)
#define OB_WRP_PAGES62TO63             (0x80000000U) /* Write protection of page 62 to 63 */
#endif /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB */
#if defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)
#define OB_WRP_PAGES62TO127            (0x80000000U) /* Write protection of page 62 to 127 */
#endif /* STM32F091xC || STM32F098xx || STM32F030xC */

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB) \
 || defined(STM32F091xC) || defined(STM32F098xx)|| defined(STM32F030xC)
#define OB_WRP_PAGES0TO15MASK          (0x000000FFU)
#define OB_WRP_PAGES16TO31MASK         (0x0000FF00U)
#define OB_WRP_PAGES32TO47MASK         (0x00FF0000U)
#endif /* STM32F071xB || STM32F072xB || STM32F078xx  || STM32F091xC || STM32F098xx || STM32F070xB || STM32F030xC */

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB)
#define OB_WRP_PAGES48TO63MASK         (0xFF000000U)
#endif /* STM32F071xB || STM32F072xB || STM32F078xx || STM32F070xB */
#if defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F030xC)
#define OB_WRP_PAGES48TO127MASK        (0xFF000000U)
#endif /* STM32F091xC || STM32F098xx || STM32F030xC */

#define OB_WRP_ALLPAGES                (0xFFFFFFFFU) /*!< Write protection of all pages */
#endif /* STM32F071xB || STM32F072xB || STM32F078xx  || STM32F091xC || STM32F098xx || STM32F030xC || STM32F070xB */

/**
  * @}
  */

/** @defgroup FLASHEx_OB_Read_Protection Option Byte Read Protection
  * @{
  */
#define OB_RDP_LEVEL_0             ((uint8_t)0xAAU)
#define OB_RDP_LEVEL_1             ((uint8_t)0xBBU)
#define OB_RDP_LEVEL_2             ((uint8_t)0xCCU) /*!< Warning: When enabling read protection level 2
                                                      it's no more possible to go back to level 1 or 0 */
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

#if defined(FLASH_OBR_BOOT_SEL)
/** @defgroup FLASHEx_OB_BOOT_SEL FLASHEx Option Byte BOOT SEL
  * @{
  */
#define OB_BOOT_SEL_RESET          ((uint8_t)0x00U) /*!< BOOT_SEL Reset */
#define OB_BOOT_SEL_SET            ((uint8_t)0x80U) /*!< BOOT_SEL Set */
/**
  * @}
  */

/** @defgroup FLASHEx_OB_BOOT0 FLASHEx Option Byte BOOT0
  * @{
  */
#define OB_BOOT0_RESET             ((uint8_t)0x00U) /*!< BOOT0 Reset */
#define OB_BOOT0_SET               ((uint8_t)0x08U) /*!< BOOT0 Set */
/**
  * @}
  */
#endif /* FLASH_OBR_BOOT_SEL */


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

#endif /* __STM32F0xx_HAL_FLASH_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

