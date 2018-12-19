/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_IAP_H_
#define _FSL_IAP_H_

#include "fsl_common.h"

/*!
 * @addtogroup IAP_driver
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_IAP_DRIVER_VERSION (MAKE_VERSION(2, 0, 2)) /*!< Version 2.0.2. */
                                                       /*@}*/

/*!
 * @brief iap status codes.
 */
enum _iap_status
{
    kStatus_IAP_Success = kStatus_Success,                          /*!< Api is executed successfully */
    kStatus_IAP_InvalidCommand = MAKE_STATUS(kStatusGroup_IAP, 1U), /*!< Invalid command */
    kStatus_IAP_SrcAddrError = MAKE_STATUS(kStatusGroup_IAP, 2U),   /*!< Source address is not on word boundary */
    kStatus_IAP_DstAddrError =
        MAKE_STATUS(kStatusGroup_IAP, 3U), /*!< Destination address is not on a correct boundary */
    kStatus_IAP_SrcAddrNotMapped =
        MAKE_STATUS(kStatusGroup_IAP, 4U), /*!< Source address is not mapped in the memory map */
    kStatus_IAP_DstAddrNotMapped =
        MAKE_STATUS(kStatusGroup_IAP, 5U), /*!< Destination address is not mapped in the memory map */
    kStatus_IAP_CountError =
        MAKE_STATUS(kStatusGroup_IAP, 6U), /*!< Byte count is not multiple of 4 or is not a permitted value */
    kStatus_IAP_InvalidSector = MAKE_STATUS(
        kStatusGroup_IAP, 7), /*!< Sector number is invalid or end sector number is greater than start sector number */
    kStatus_IAP_SectorNotblank = MAKE_STATUS(kStatusGroup_IAP, 8U), /*!< One or more sectors are not blank */
    kStatus_IAP_NotPrepared =
        MAKE_STATUS(kStatusGroup_IAP, 9U), /*!< Command to prepare sector for write operation was not executed */
    kStatus_IAP_CompareError =
        MAKE_STATUS(kStatusGroup_IAP, 10U),                /*!< Destination and source memory contents do not match */
    kStatus_IAP_Busy = MAKE_STATUS(kStatusGroup_IAP, 11U), /*!< Flash programming hardware interface is busy */
    kStatus_IAP_ParamError =
        MAKE_STATUS(kStatusGroup_IAP, 12U), /*!< Insufficient number of parameters or invalid parameter */
    kStatus_IAP_AddrError = MAKE_STATUS(kStatusGroup_IAP, 13U),     /*!< Address is not on word boundary */
    kStatus_IAP_AddrNotMapped = MAKE_STATUS(kStatusGroup_IAP, 14U), /*!< Address is not mapped in the memory map */
    kStatus_IAP_NoPower = MAKE_STATUS(kStatusGroup_IAP, 24U),       /*!< Flash memory block is powered down */
    kStatus_IAP_NoClock = MAKE_STATUS(kStatusGroup_IAP, 27U), /*!< Flash memory block or controller is not clocked */
    kStatus_IAP_ReinvokeISPConfig = MAKE_STATUS(kStatusGroup_IAP, 0x1CU), /*!< Reinvoke configuration error */
};

/*!
 * @brief iap command codes.
 */
enum _iap_commands
{
    kIapCmd_IAP_ReadFactorySettings = 40U,   /*!< Read the factory settings */
    kIapCmd_IAP_PrepareSectorforWrite = 50U, /*!< Prepare Sector for write */
    kIapCmd_IAP_CopyRamToFlash = 51U,        /*!< Copy RAM to flash */
    kIapCmd_IAP_EraseSector = 52U,           /*!< Erase Sector */
    kIapCmd_IAP_BlankCheckSector = 53U,      /*!< Blank check sector */
    kIapCmd_IAP_ReadPartId = 54U,            /*!< Read part id */
    kIapCmd_IAP_Read_BootromVersion = 55U,   /*!< Read bootrom version */
    kIapCmd_IAP_Compare = 56U,               /*!< Compare */
    kIapCmd_IAP_ReinvokeISP = 57U,           /*!< Reinvoke ISP */
    kIapCmd_IAP_ReadUid = 58U,               /*!< Read Uid */
    kIapCmd_IAP_ErasePage = 59U,             /*!< Erase Page */
    kIapCmd_IAP_ReadSignature = 70U,         /*!< Read Signature */
    kIapCmd_IAP_ExtendedReadSignature = 73U, /*!< Extended Read Signature */
    kIapCmd_IAP_ReadEEPROMPage = 80U,        /*!< Read EEPROM page */
    kIapCmd_IAP_WriteEEPROMPage = 81U        /*!< Write EEPROM page */
};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*! @brief IAP_ENTRY API function type */
typedef void (*IAP_ENTRY_T)(uint32_t cmd[], uint32_t stat[]);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief IAP_ENTRY API function type
 *
 * Wrapper for rom iap call
 *
 * @param cmd_param IAP command and relevant parameter array.
 * @param status_result IAP status result array.
 *
 * @retval None. Status/Result is returned via status_result array.
 */
static inline void iap_entry(uint32_t *cmd_param, uint32_t *status_result)
{
    __disable_irq();
    ((IAP_ENTRY_T)FSL_FEATURE_SYSCON_IAP_ENTRY_LOCATION)(cmd_param, status_result);
    __enable_irq();
}

/*!
 * @brief Read part identification number.

 * This function is used to read the part identification number.
 *
 * @param partID Address to store the part identification number.
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.
 */
status_t IAP_ReadPartID(uint32_t *partID);

/*!
 * @brief Read boot code version number.

 * This function is used to read the boot code version number.
 *
 * @param bootCodeVersion Address to store the boot code version.
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.

 * @note Boot code version is two 32-bit words. Word 0 is the major version, word 1 is the minor version.
 */
status_t IAP_ReadBootCodeVersion(uint32_t *bootCodeVersion);

/*!
 * @brief Reinvoke ISP

 * This function is used to invoke the boot loader in ISP mode. It maps boot vectors and configures the
 * peripherals for ISP.
 *
 * @param ispTyoe ISP type selection.
 * @param status store the possible status
 *
 * @retval #kStatus_IAP_ReinvokeISPConfig reinvoke configuration error.

 * @note The error response is returned if IAP is disabled, or if there is an invalid ISP type selection. When
 * there is no error the call does not return, so there can be no status code.
 */
void IAP_ReinvokeISP(uint8_t ispType, uint32_t *status);

/*!
 * @brief Read unique identification.

 * This function is used to read the unique id.
 *
 * @param uniqueID store the uniqueID.
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.
 */
status_t IAP_ReadUniqueID(uint32_t *uniqueID);

#if (defined(FSL_FEATURE_IAP_HAS_READ_FACTORY_SETTINGS_FUNCTION) && \
     (FSL_FEATURE_IAP_HAS_READ_FACTORY_SETTINGS_FUNCTION == 1))
/*!
 * @brief Read factory settings.

 * This function reads the factory settings for calibration registers.
 *
 * @param dstRegAddr Address of the targeted calibration register.
 * @param factoryValue Store the factory value
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.
 * @retval #kStatus_IAP_ParamError Param0 is not one of the supported calibration registers
 */
status_t IAP_ReadFactorySettings(uint32_t dstRegAddr, uint32_t *factoryValue);
#endif

#if (defined(FSL_FEATURE_IAP_HAS_FLASH_FUNCTION) && (FSL_FEATURE_IAP_HAS_FLASH_FUNCTION == 1))
/*!
 * @brief	Prepare sector for write operation

 * This function prepares sector(s) for write/erase operation. This function must be
 * called before calling the IAP_CopyRamToFlash() or IAP_EraseSector() or
 * IAP_ErasePage() function. The end sector must be greater than or equal to
 * start sector number.
 *
 * @param startSector Start sector number.
 * @param endSector End sector number.
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.
 * @retval #kStatus_IAP_NoPower Flash memory block is powered down.
 * @retval #kStatus_IAP_NoClock Flash memory block or controller is not clocked.
 * @retval #kStatus_IAP_InvalidSector Sector number is invalid or end sector number
 *         is greater than start sector number.
 * @retval #kStatus_IAP_Busy Flash programming hardware interface is busy.
 */
status_t IAP_PrepareSectorForWrite(uint32_t startSector, uint32_t endSector);

/*!
 * @brief	Copy RAM to flash.

 * This function programs the flash memory. Corresponding sectors must be prepared
 * via IAP_PrepareSectorForWrite before calling calling this function. The addresses
 * should be a 256 byte boundary and the number of bytes should be 256 | 512 | 1024 | 4096.
 *
 * @param dstAddr Destination flash address where data bytes are to be written.
 * @param srcAddr Source ram address from where data bytes are to be read.
 * @param numOfBytes Number of bytes to be written.
 * @param systemCoreClock SystemCoreClock in Hz. It is converted to KHz before calling the
 *                        rom IAP function.
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.
 * @retval #kStatus_IAP_NoPower Flash memory block is powered down.
 * @retval #kStatus_IAP_NoClock Flash memory block or controller is not clocked.
 * @retval #kStatus_IAP_SrcAddrError Source address is not on word boundary.
 * @retval #kStatus_IAP_DstAddrError Destination address is not on a correct boundary.
 * @retval #kStatus_IAP_SrcAddrNotMapped Source address is not mapped in the memory map.
 * @retval #kStatus_IAP_DstAddrNotMapped Destination address is not mapped in the memory map.
 * @retval #kStatus_IAP_CountError Byte count is not multiple of 4 or is not a permitted value.
 * @retval #kStatus_IAP_NotPrepared Command to prepare sector for write operation was not executed.
 * @retval #kStatus_IAP_Busy Flash programming hardware interface is busy.
 */
status_t IAP_CopyRamToFlash(uint32_t dstAddr, uint32_t *srcAddr, uint32_t numOfBytes, uint32_t systemCoreClock);

/*!
 * @brief	Erase sector

 * This function erases sector(s). The end sector must be greater than or equal to
 * start sector number. IAP_PrepareSectorForWrite must be called before
 * calling this function.
 *
 * @param startSector Start sector number.
 * @param endSector End sector number.
 * @param systemCoreClock SystemCoreClock in Hz. It is converted to KHz before calling the
 *                        rom IAP function.
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.
 * @retval #kStatus_IAP_NoPower Flash memory block is powered down.
 * @retval #kStatus_IAP_NoClock Flash memory block or controller is not clocked.
 * @retval #kStatus_IAP_InvalidSector Sector number is invalid or end sector number
 *         is greater than start sector number.
 * @retval #kStatus_IAP_NotPrepared Command to prepare sector for write operation was not executed.
 * @retval #kStatus_IAP_Busy Flash programming hardware interface is busy.
 */
status_t IAP_EraseSector(uint32_t startSector, uint32_t endSector, uint32_t systemCoreClock);

/*!

 * This function erases page(s). The end page must be greater than or equal to
 * start page number. Corresponding sectors must be prepared via IAP_PrepareSectorForWrite
 * before calling calling this function.
 *
 * @param startPage Start page number
 * @param endPage End page number
 * @param systemCoreClock SystemCoreClock in Hz. It is converted to KHz before calling the
 *                        rom IAP function.
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.
 * @retval #kStatus_IAP_NoPower Flash memory block is powered down.
 * @retval #kStatus_IAP_NoClock Flash memory block or controller is not clocked.
 * @retval #kStatus_IAP_InvalidSector Page number is invalid or end page number
 *         is greater than start page number
 * @retval #kStatus_IAP_NotPrepared Command to prepare sector for write operation was not executed.
 * @retval #kStatus_IAP_Busy Flash programming hardware interface is busy.
 */
status_t IAP_ErasePage(uint32_t startPage, uint32_t endPage, uint32_t systemCoreClock);

/*!
 * @brief Blank check sector(s)
 *
 * Blank check single or multiples sectors of flash memory. The end sector must be greater than or equal to
 * start sector number. It can be used to verify the sector eraseure after IAP_EraseSector call.
 *
 * @param	startSector	: Start sector number. Must be greater than or equal to start sector number
 * @param	endSector	: End sector number
 * @retval #kStatus_IAP_Success One or more sectors are in erased state.
 * @retval #kStatus_IAP_NoPower Flash memory block is powered down.
 * @retval #kStatus_IAP_NoClock Flash memory block or controller is not clocked.
 * @retval #kStatus_IAP_SectorNotblank One or more sectors are not blank.
 */
status_t IAP_BlankCheckSector(uint32_t startSector, uint32_t endSector);

/*!
 * @brief Compare memory contents of flash with ram.

 * This function compares the contents of flash and ram. It can be used to verify the flash
 * memory contents after IAP_CopyRamToFlash call.
 *
 * @param dstAddr Destination flash address.
 * @param srcAddr Source ram address.
 * @param numOfBytes Number of bytes to be compared.
 *
 * @retval #kStatus_IAP_Success Contents of flash and ram match.
 * @retval #kStatus_IAP_NoPower Flash memory block is powered down.
 * @retval #kStatus_IAP_NoClock Flash memory block or controller is not clocked.
 * @retval #kStatus_IAP_AddrError Address is not on word boundary.
 * @retval #kStatus_IAP_AddrNotMapped Address is not mapped in the memory map.
 * @retval #kStatus_IAP_CountError Byte count is not multiple of 4 or is not a permitted value.
 * @retval #kStatus_IAP_CompareError Destination and source memory contents do not match.
 */
status_t IAP_Compare(uint32_t dstAddr, uint32_t *srcAddr, uint32_t numOfBytes);

#if defined(FSL_FEATURE_IAP_HAS_FLASH_EXTENDED_SIGNATURE_READ) && FSL_FEATURE_IAP_HAS_FLASH_EXTENDED_SIGNATURE_READ
/*!
 * @brief Extended Read signature.

 * This function calculates the signature value for one or more pages of on-chip flash memory.
 *
 * @param startPage Start page number.
 * @param endPage End page number.
 * @param numOfStates Number of wait status.
 * @param signature Address to store the signature value.
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.
 */
status_t IAP_ExtendedFlashSignatureRead(uint32_t startPage, uint32_t endPage, uint32_t numOfState, uint32_t *signature);
#endif /* FSL_FEATURE_IAP_HAS_FLASH_EXTENDED_SIGNATURE_READ */

#if defined(FSL_FEATURE_IAP_HAS_FLASH_SIGNATURE_READ) && FSL_FEATURE_IAP_HAS_FLASH_SIGNATURE_READ
/*!
 * @brief Read flash signature
 *
 * This funtion is used to obtain a 32-bit signature value of the entire flash memory.
 *
 * @param signature Address to store the 32-bit generated signature value.
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.
 */
status_t IAP_ReadFlashSignature(uint32_t *signature);
#endif /* FSL_FEATURE_IAP_HAS_FLASH_SIGNATURE_READ */
#endif /* FSL_FEATURE_IAP_HAS_FLASH_FUNCTION */

#if (defined(FSL_FEATURE_IAP_HAS_EEPROM_FUNCTION) && (FSL_FEATURE_IAP_HAS_EEPROM_FUNCTION == 1))
/*!
 * @brief Read EEPROM page.

 * This function is used to read given page of EEPROM into the memory provided.
 *
 * @param pageNumber EEPROM page number.
 * @param dstAddr Memory address to store the value read from EEPROM.
 * @param systemCoreClock Current core clock frequency in kHz.
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.
 * @retval #kStatus_IAP_InvalidSector Sector number is invalid.
 * @retval #kStatus_IAP_SrcAddrNotMapped Source address is not mapped in the memory map.
 *
 * @note Value 0xFFFFFFFF of systemCoreClock will retain the timing and clock settings for
 * EEPROM]
 */
status_t IAP_ReadEEPROMPage(uint32_t pageNumber, uint32_t *dstAddr, uint32_t systemCoreClock);

/*!
 * @brief Write EEPROM page.

 * This function is used to write given data in the provided memory to a page of EEPROM.
 *
 * @param pageNumber EEPROM page number.
 * @param srcAddr Memory address holding data to be stored on to EEPROM page.
 * @param systemCoreClock Current core clock frequency in kHz.
 *
 * @retval #kStatus_IAP_Success Api was executed successfully.
 * @retval #kStatus_IAP_InvalidSector Sector number is invalid.
 * @retval #kStatus_IAP_SrcAddrNotMapped Source address is not mapped in the memory map.
 *
 * @note Value 0xFFFFFFFF of systemCoreClock will retain the timing and clock settings for
 * EEPROM]
 */
status_t IAP_WriteEEPROMPage(uint32_t pageNumber, uint32_t *srcAddr, uint32_t systemCoreClock);
#endif /* FSL_FEATURE_IAP_HAS_EEPROM_FUNCTION */

#ifdef __cplusplus
}
#endif

/*@}*/

#endif /* _FSL_IAP_H_ */
