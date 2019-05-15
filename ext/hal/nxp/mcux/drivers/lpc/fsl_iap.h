/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __FSL_IAP_H_
#define __FSL_IAP_H_

#include "fsl_common.h"
/*!
 * @addtogroup iap_driver
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*!
 * @name Flash version
 * @{
 */
/*! @brief Constructs the version number for drivers. */
#if !defined(MAKE_VERSION)
#define MAKE_VERSION(major, minor, bugfix) (((major) << 16) | ((minor) << 8) | (bugfix))
#endif

/*! @brief Flash driver version for SDK*/
#define FSL_FLASH_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0. */

/*! @brief Flash driver version for ROM*/
enum _flash_driver_version_constants
{
    kFLASH_DriverVersionName = 'F', /*!< Flash driver version name.*/
    kFLASH_DriverVersionMajor = 2,  /*!< Major flash driver version.*/
    kFLASH_DriverVersionMinor = 0,  /*!< Minor flash driver version.*/
    kFLASH_DriverVersionBugfix = 0  /*!< Bugfix for flash driver version.*/
};

/*@}*/

/*!
 * @name Flash configuration
 * @{
 */
/*! @brief Flash IP Type. */
#if !defined(FSL_FEATURE_FLASH_IP_IS_C040HD_ATFC)
#define FSL_FEATURE_FLASH_IP_IS_C040HD_ATFC (1)
#endif
#if !defined(FSL_FEATURE_FLASH_IP_IS_C040HD_FC)
#define FSL_FEATURE_FLASH_IP_IS_C040HD_FC (0)
#endif

/*@}*/

/*!
 * @name Flash status
 * @{
 */
/*! @brief Flash driver status group. */
#if defined(kStatusGroup_FlashDriver)
#define kStatusGroupGeneric kStatusGroup_Generic
#define kStatusGroupFlashDriver kStatusGroup_FlashDriver
#elif defined(kStatusGroup_FLASHIAP)
#define kStatusGroupGeneric kStatusGroup_Generic
#define kStatusGroupFlashDriver kStatusGroup_FLASH
#else
#define kStatusGroupGeneric 0
#define kStatusGroupFlashDriver 1
#endif

/*! @brief Constructs a status code value from a group and a code number. */
#if !defined(MAKE_STATUS)
#define MAKE_STATUS(group, code) ((((group)*100) + (code)))
#endif

/*!
 * @brief Flash driver status codes.
 */
enum _flash_status
{
    kStatus_FLASH_Success = MAKE_STATUS(kStatusGroupGeneric, 0),         /*!< API is executed successfully*/
    kStatus_FLASH_InvalidArgument = MAKE_STATUS(kStatusGroupGeneric, 4), /*!< Invalid argument*/
    kStatus_FLASH_SizeError = MAKE_STATUS(kStatusGroupFlashDriver, 0),   /*!< Error size*/
    kStatus_FLASH_AlignmentError =
        MAKE_STATUS(kStatusGroupFlashDriver, 1), /*!< Parameter is not aligned with the specified baseline*/
    kStatus_FLASH_AddressError = MAKE_STATUS(kStatusGroupFlashDriver, 2), /*!< Address is out of range */
    kStatus_FLASH_AccessError =
        MAKE_STATUS(kStatusGroupFlashDriver, 3), /*!< Invalid instruction codes and out-of bound addresses */
    kStatus_FLASH_ProtectionViolation = MAKE_STATUS(
        kStatusGroupFlashDriver, 4), /*!< The program/erase operation is requested to execute on protected areas */
    kStatus_FLASH_CommandFailure =
        MAKE_STATUS(kStatusGroupFlashDriver, 5), /*!< Run-time error during command execution. */
    kStatus_FLASH_UnknownProperty = MAKE_STATUS(kStatusGroupFlashDriver, 6), /*!< Unknown property.*/
    kStatus_FLASH_EraseKeyError = MAKE_STATUS(kStatusGroupFlashDriver, 7),   /*!< API erase key is invalid.*/
    kStatus_FLASH_RegionExecuteOnly =
        MAKE_STATUS(kStatusGroupFlashDriver, 8), /*!< The current region is execute-only.*/
    kStatus_FLASH_ExecuteInRamFunctionNotReady =
        MAKE_STATUS(kStatusGroupFlashDriver, 9), /*!< Execute-in-RAM function is not available.*/

    kStatus_FLASH_CommandNotSupported = MAKE_STATUS(kStatusGroupFlashDriver, 11), /*!< Flash API is not supported.*/
    kStatus_FLASH_ReadOnlyProperty = MAKE_STATUS(kStatusGroupFlashDriver, 12), /*!< The flash property is read-only.*/
    kStatus_FLASH_InvalidPropertyValue =
        MAKE_STATUS(kStatusGroupFlashDriver, 13), /*!< The flash property value is out of range.*/
    kStatus_FLASH_InvalidSpeculationOption =
        MAKE_STATUS(kStatusGroupFlashDriver, 14), /*!< The option of flash prefetch speculation is invalid.*/
    kStatus_FLASH_EccError = MAKE_STATUS(kStatusGroupFlashDriver,
                                         0x10), /*!< A correctable or uncorrectable error during command execution. */
    kStatus_FLASH_CompareError =
        MAKE_STATUS(kStatusGroupFlashDriver, 0x11), /*!< Destination and source memory contents do not match. */
    kStatus_FLASH_RegulationLoss = MAKE_STATUS(kStatusGroupFlashDriver, 0x12), /*!< A loss of regulation during read. */
    kStatus_FLASH_InvalidWaitStateCycles =
        MAKE_STATUS(kStatusGroupFlashDriver, 0x13), /*!< The wait state cycle set to r/w mode is invalid. */

    kStatus_FLASH_OutOfDateCfpaPage =
        MAKE_STATUS(kStatusGroupFlashDriver, 0x20), /*!< CFPA page version is out of date. */
    kStatus_FLASH_BlankIfrPageData = MAKE_STATUS(kStatusGroupFlashDriver, 0x21), /*!< Blank page cannnot be read. */
    kStatus_FLASH_EncryptedRegionsEraseNotDoneAtOnce =
        MAKE_STATUS(kStatusGroupFlashDriver, 0x22), /*!< Encrypted flash subregions are not erased at once. */
    kStatus_FLASH_ProgramVerificationNotAllowed = MAKE_STATUS(
        kStatusGroupFlashDriver, 0x23), /*!< Program verification is not allowed when the encryption is enabled. */
    kStatus_FLASH_HashCheckError =
        MAKE_STATUS(kStatusGroupFlashDriver, 0x24), /*!< Hash check of page data is failed. */
    kStatus_FLASH_SealedFfrRegion = MAKE_STATUS(kStatusGroupFlashDriver, 0x25), /*!< The FFR region is sealed. */
    kStatus_FLASH_FfrRegionWriteBroken = MAKE_STATUS(
        kStatusGroupFlashDriver, 0x26), /*!< The FFR Spec region is not allowed to be written discontinuously. */
    kStatus_FLASH_NmpaAccessNotAllowed =
        MAKE_STATUS(kStatusGroupFlashDriver, 0x27), /*!< The NMPA region is not allowed to be read/written/erased. */
    kStatus_FLASH_CmpaCfgDirectEraseNotAllowed =
        MAKE_STATUS(kStatusGroupFlashDriver, 0x28), /*!< The CMPA Cfg region is not allowed to be erased directly. */
    kStatus_FLASH_FfrBankIsLocked = MAKE_STATUS(kStatusGroupFlashDriver, 0x29), /*!< The FFR bank region is locked. */
};
/*@}*/

/*!
 * @name Flash API key
 * @{
 */
/*! @brief Constructs the four character code for the Flash driver API key. */
#if !defined(FOUR_CHAR_CODE)
#define FOUR_CHAR_CODE(a, b, c, d) (((d) << 24) | ((c) << 16) | ((b) << 8) | ((a)))
#endif

/*!
 * @brief Enumeration for Flash driver API keys.
 *
 * @note The resulting value is built with a byte order such that the string
 * being readable in expected order when viewed in a hex editor, if the value
 * is treated as a 32-bit little endian value.
 */
enum _flash_driver_api_keys
{
    kFLASH_ApiEraseKey = FOUR_CHAR_CODE('l', 'f', 'e', 'k') /*!< Key value used to validate all flash erase APIs.*/
};
/*@}*/

/*!
 * @brief Enumeration for various flash properties.
 */
typedef enum _flash_property_tag
{
    kFLASH_PropertyPflashSectorSize = 0x00U,    /*!< Pflash sector size property.*/
    kFLASH_PropertyPflashTotalSize = 0x01U,     /*!< Pflash total size property.*/
    kFLASH_PropertyPflashBlockSize = 0x02U,     /*!< Pflash block size property.*/
    kFLASH_PropertyPflashBlockCount = 0x03U,    /*!< Pflash block count property.*/
    kFLASH_PropertyPflashBlockBaseAddr = 0x04U, /*!< Pflash block base address property.*/

    kFLASH_PropertyPflashPageSize = 0x30U,   /*!< Pflash page size property.*/
    kFLASH_PropertyPflashSystemFreq = 0x31U, /*!< System Frequency System Frequency.*/

    kFLASH_PropertyFfrSectorSize = 0x40U,    /*!< FFR sector size property.*/
    kFLASH_PropertyFfrTotalSize = 0x41U,     /*!< FFR total size property.*/
    kFLASH_PropertyFfrBlockBaseAddr = 0x42U, /*!< FFR block base address property.*/
    kFLASH_PropertyFfrPageSize = 0x43U,      /*!< FFR page size property.*/
} flash_property_tag_t;

/*!
 * @brief Enumeration for flash max pages to erase.
 */
enum _flash_max_erase_page_value
{
    kFLASH_MaxPagesToErase = 100U /*!< The max value in pages to erase. */
};

enum _flash_freq_tag
{
    kSysToFlashFreq_lowInMHz = 12u,
    kSysToFlashFreq_defaultInMHz = 96u,
    kSysToFlashFreq_100MHz = 100u
};

/*!
 * @brief Enumeration for flash alignment property.
 */
enum _flash_alignment_property
{
    kFLASH_AlignementUnitVerifyErase = 4,       /*!< The alignment unit in bytes used for verify erase operation.*/
    kFLASH_AlignementUnitProgram = 512,         /*!< The alignment unit in bytes used for program operation.*/
    /*kFLASH_AlignementUnitVerifyProgram = 4,*/ /*!< The alignment unit in bytes used for verify program operation.*/
    kFLASH_AlignementUnitSingleWordRead = 16    /*!< The alignment unit in bytes used for SingleWordRead command.*/
};

/*!
 * @brief Enumeration for flash read ecc option
 */
enum _flash_read_ecc_option
{
    kFLASH_ReadWithEccOn = 0,  /*! ECC is on */
    kFLASH_ReadWithEccOff = 1, /*! ECC is off */
};

/*!
 * @brief Enumeration for flash read margin option
 */
enum _flash_read_margin_option
{
    kFLASH_ReadMarginNormal = 0,               /*!< Normal read */
    kFLASH_ReadMarginVsProgram = 1,            /*!< Margin vs. program */
    kFLASH_ReadMarginVsErase = 2,              /*!< Margin vs. erase */
    kFLASH_ReadMarginIllegalBitCombination = 3 /*!< Illegal bit combination */
};

/*!
 * @brief Enumeration for flash read dmacc option
 */
enum _flash_read_dmacc_option
{
    kFLASH_ReadDmaccDisabled = 0, /*!< Memory word */
    kFLASH_ReadDmaccEnabled = 1,  /*!< DMACC word */
};

/*!
 * @brief Enumeration for flash ramp control option
 */
enum _flash_ramp_control_option
{
    kFLASH_RampControlDivisionFactorReserved = 0, /*!< Reserved */
    kFLASH_RampControlDivisionFactor256 = 1,      /*!< clk48mhz / 256 = 187.5KHz */
    kFLASH_RampControlDivisionFactor128 = 2,      /*!< clk48mhz / 128 = 375KHz */
    kFLASH_RampControlDivisionFactor64 = 3        /*!< clk48mhz / 64 = 750KHz */
};

/*! @brief Flash ECC log info. */
typedef struct _flash_ecc_log
{
    uint32_t firstEccEventAddress;
    uint32_t eccErrorCount;
    uint32_t eccCorrectionCount;
    uint32_t reserved;
} flash_ecc_log_t;

/*! @brief Flash controller paramter config. */
typedef struct _flash_mode_config
{
    uint32_t sysFreqInMHz;
    /* ReadSingleWord parameter. */
    struct
    {
        uint8_t readWithEccOff : 1;
        uint8_t readMarginLevel : 2;
        uint8_t readDmaccWord : 1;
        uint8_t reserved0 : 4;
        uint8_t reserved1[3];
    } readSingleWord;
    /* SetWriteMode parameter. */
    struct
    {
        uint8_t programRampControl;
        uint8_t eraseRampControl;
        uint8_t reserved[2];
    } setWriteMode;
    /* SetReadMode parameter. */
    struct
    {
        uint16_t readInterfaceTimingTrim;
        uint16_t readControllerTimingTrim;
        uint8_t readWaitStates;
        uint8_t reserved[3];
    } setReadMode;
} flash_mode_config_t;

/*! @brief Flash controller paramter config. */
typedef struct _flash_ffr_config
{
    uint32_t ffrBlockBase;
    uint32_t ffrTotalSize;
    uint32_t ffrPageSize;
    uint32_t cfpaPageVersion;
    uint32_t cfpaPageOffset;
} flash_ffr_config_t;

/*! @brief Flash driver state information.
 *
 * An instance of this structure is allocated by the user of the flash driver and
 * passed into each of the driver APIs.
 */
typedef struct _flash_config
{
    uint32_t PFlashBlockBase;  /*!< A base address of the first PFlash block */
    uint32_t PFlashTotalSize;  /*!< The size of the combined PFlash block. */
    uint32_t PFlashBlockCount; /*!< A number of PFlash blocks. */
    uint32_t PFlashPageSize;   /*!< The size in bytes of a page of PFlash. */
    uint32_t PFlashSectorSize; /*!< The size in bytes of a sector of PFlash. */
    flash_ffr_config_t ffrConfig;
    flash_mode_config_t modeConfig;
} flash_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization
 * @{
 */

/*!
 * @brief Initializes the global flash properties structure members.
 *
 * This function checks and initializes the Flash module for the other Flash APIs.
 *
 * @param config Pointer to the storage for the driver runtime state.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 * @retval #kStatus_FLASH_CommandNotSupported Flash API is not supported.
 * @retval #kStatus_FLASH_EccError A correctable or uncorrectable error during command execution.
 */
status_t FLASH_Init(flash_config_t *config);

/*@}*/

/*!
 * @name Erasing
 * @{
 */

/*!
 * @brief Erases the flash sectors encompassed by parameters passed into function.
 *
 * This function erases the appropriate number of flash sectors based on the
 * desired start address and length.
 *
 * @param config The pointer to the storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be erased.
 *              The start address does not need to be sector-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *                      to be erased. Must be word-aligned.
 * @param key The value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError The parameter is not aligned with the specified baseline.
 * @retval #kStatus_FLASH_AddressError The address is out of range.
 * @retval #kStatus_FLASH_EraseKeyError The API erase key is invalid.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 * @retval #kStatus_FLASH_CommandNotSupported Flash API is not supported.
 * @retval #kStatus_FLASH_EccError A correctable or uncorrectable error during command execution.
 */
status_t FLASH_Erase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint32_t key);

/*@}*/

/*!
 * @name Programming
 * @{
 */

/*!
 * @brief Programs flash with data at locations passed in through parameters.
 *
 * This function programs the flash memory with the desired data for a given
 * flash area as determined by the start address and the length.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be programmed. Must be
 *              word-aligned.
 * @param src A pointer to the source buffer of data that is to be programmed
 *            into the flash.
 * @param lengthInBytes The length, given in bytes (not words or long-words),
 *                      to be programmed. Must be word-aligned.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with the specified baseline.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 * @retval #kStatus_FLASH_CommandNotSupported Flash API is not supported.
 * @retval #kStatus_FLASH_EccError A correctable or uncorrectable error during command execution.
 */
status_t FLASH_Program(flash_config_t *config, uint32_t start, uint8_t *src, uint32_t lengthInBytes);

/*@}*/

/*!
 * @name Verification
 * @{
 */

/*!
 * @brief Verifies an erasure of the desired flash area at a specified margin level.
 *
 * This function checks the appropriate number of flash sectors based on
 * the desired start address and length to check whether the flash is erased
 * to the specified read margin level.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be verified.
 *        The start address does not need to be sector-aligned but must be word-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words),
 *        to be verified. Must be word-aligned.
 * @param margin Read margin choice.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 * @retval #kStatus_FLASH_CommandNotSupported Flash API is not supported.
 * @retval #kStatus_FLASH_EccError A correctable or uncorrectable error during command execution.
 */
status_t FLASH_VerifyErase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes);

/*!
 * @brief Verifies programming of the desired flash area at a specified margin level.
 *
 * This function verifies the data programed in the flash memory using the
 * Flash Program Check Command and compares it to the expected data for a given
 * flash area as determined by the start address and length.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be verified. Must be word-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words),
 *        to be verified. Must be word-aligned.
 * @param expectedData A pointer to the expected data that is to be
 *        verified against.
 * @param margin Read margin choice.
 * @param failedAddress A pointer to the returned failing address.
 * @param failedData A pointer to the returned failing data.  Some derivatives do
 *        not include failed data as part of the FCCOBx registers.  In this
 *        case, zeros are returned upon failure.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 * @retval #kStatus_FLASH_CommandNotSupported Flash API is not supported.
 * @retval #kStatus_FLASH_EccError A correctable or uncorrectable error during command execution.
 */
status_t FLASH_VerifyProgram(flash_config_t *config,
                             uint32_t start,
                             uint32_t lengthInBytes,
                             const uint8_t *expectedData,
                             uint32_t *failedAddress,
                             uint32_t *failedData);

/*@}*/

/*!
 * @name Properties
 * @{
 */

/*!
 * @brief Returns the desired flash property.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param whichProperty The desired property from the list of properties in
 *        enum flash_property_tag_t
 * @param value A pointer to the value returned for the desired flash property.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_UnknownProperty An unknown property tag.
 */
status_t FLASH_GetProperty(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t *value);

/*!
 * @brief Sets the desired flash property.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param whichProperty The desired property from the list of properties in
 *        enum flash_property_tag_t
 * @param value A to set for the desired flash property.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_UnknownProperty An unknown property tag.
 * @retval #kStatus_FLASH_ReadOnlyProperty An read-only property tag.
 */
status_t FLASH_SetProperty(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t value);

/*@}*/

#ifdef __cplusplus
}
#endif
/*@}*/

#endif /* __FLASH_FLASH_H_ */
