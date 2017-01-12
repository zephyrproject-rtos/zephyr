/*
 * Copyright (c) 2013-2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FSL_FLASH_H_
#define _FSL_FLASH_H_

#if (defined(BL_TARGET_FLASH) || defined(BL_TARGET_ROM) || defined(BL_TARGET_RAM))
#include <assert.h>
#include <string.h>
#include "fsl_device_registers.h"
#include "bootloader_common.h"
#else
#include "fsl_common.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @addtogroup flash_driver
 * @{
 */

/*!
 * @name Flash version
 * @{
 */
/*! @brief Construct the version number for drivers. */
#if !defined(MAKE_VERSION)
#define MAKE_VERSION(major, minor, bugfix) (((major) << 16) | ((minor) << 8) | (bugfix))
#endif

/*! @brief FLASH driver version for SDK*/
#define FSL_FLASH_DRIVER_VERSION (MAKE_VERSION(2, 1, 0)) /*!< Version 2.1.0. */

/*! @brief FLASH driver version for ROM*/
enum _flash_driver_version_constants
{
    kFLASH_DriverVersionName = 'F', /*!< Flash driver version name.*/
    kFLASH_DriverVersionMajor = 2,  /*!< Major flash driver version.*/
    kFLASH_DriverVersionMinor = 1,  /*!< Minor flash driver version.*/
    kFLASH_DriverVersionBugfix = 0  /*!< Bugfix for flash driver version.*/
};
/*@}*/

/*!
 * @name Flash configuration
 * @{
 */
/*! @brief Whether to support FlexNVM in flash driver */
#if !defined(FLASH_SSD_CONFIG_ENABLE_FLEXNVM_SUPPORT)
#define FLASH_SSD_CONFIG_ENABLE_FLEXNVM_SUPPORT 1 /*!< Enable FlexNVM support by default. */
#endif

/*! @brief Whether the FlexNVM is enabled in flash driver */
#define FLASH_SSD_IS_FLEXNVM_ENABLED (FLASH_SSD_CONFIG_ENABLE_FLEXNVM_SUPPORT && FSL_FEATURE_FLASH_HAS_FLEX_NVM)

/*! @brief Flash driver location. */
#if !defined(FLASH_DRIVER_IS_FLASH_RESIDENT)
#if (!defined(BL_TARGET_ROM) && !defined(BL_TARGET_RAM))
#define FLASH_DRIVER_IS_FLASH_RESIDENT 1 /*!< Used for flash resident application. */
#else
#define FLASH_DRIVER_IS_FLASH_RESIDENT 0 /*!< Used for non-flash resident application. */
#endif
#endif

/*! @brief Flash Driver Export option */
#if !defined(FLASH_DRIVER_IS_EXPORTED)
#if (defined(BL_TARGET_ROM) || defined(BL_TARGET_FLASH))
#define FLASH_DRIVER_IS_EXPORTED 1 /*!< Used for ROM bootloader. */
#else
#define FLASH_DRIVER_IS_EXPORTED 0 /*!< Used for SDK application. */
#endif
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
#elif defined(kStatusGroup_FLASH)
#define kStatusGroupGeneric kStatusGroup_Generic
#define kStatusGroupFlashDriver kStatusGroup_FLASH
#else
#define kStatusGroupGeneric 0
#define kStatusGroupFlashDriver 1
#endif

/*! @brief Construct a status code value from a group and code number. */
#if !defined(MAKE_STATUS)
#define MAKE_STATUS(group, code) ((((group)*100) + (code)))
#endif

/*!
 * @brief Flash driver status codes.
 */
enum _flash_status
{
    kStatus_FLASH_Success = MAKE_STATUS(kStatusGroupGeneric, 0),         /*!< Api is executed successfully*/
    kStatus_FLASH_InvalidArgument = MAKE_STATUS(kStatusGroupGeneric, 4), /*!< Invalid argument*/
    kStatus_FLASH_SizeError = MAKE_STATUS(kStatusGroupFlashDriver, 0),   /*!< Error size*/
    kStatus_FLASH_AlignmentError =
        MAKE_STATUS(kStatusGroupFlashDriver, 1), /*!< Parameter is not aligned with specified baseline*/
    kStatus_FLASH_AddressError = MAKE_STATUS(kStatusGroupFlashDriver, 2), /*!< Address is out of range */
    kStatus_FLASH_AccessError =
        MAKE_STATUS(kStatusGroupFlashDriver, 3), /*!< Invalid instruction codes and out-of bounds addresses */
    kStatus_FLASH_ProtectionViolation = MAKE_STATUS(
        kStatusGroupFlashDriver, 4), /*!< The program/erase operation is requested to execute on protected areas */
    kStatus_FLASH_CommandFailure =
        MAKE_STATUS(kStatusGroupFlashDriver, 5), /*!< Run-time error during command execution. */
    kStatus_FLASH_UnknownProperty = MAKE_STATUS(kStatusGroupFlashDriver, 6),   /*!< Unknown property.*/
    kStatus_FLASH_EraseKeyError = MAKE_STATUS(kStatusGroupFlashDriver, 7),     /*!< Api erase key is invalid.*/
    kStatus_FLASH_RegionExecuteOnly = MAKE_STATUS(kStatusGroupFlashDriver, 8), /*!< Current region is execute only.*/
    kStatus_FLASH_ExecuteInRamFunctionNotReady =
        MAKE_STATUS(kStatusGroupFlashDriver, 9), /*!< Execute-in-ram function is not available.*/
    kStatus_FLASH_PartitionStatusUpdateFailure =
        MAKE_STATUS(kStatusGroupFlashDriver, 10), /*!< Failed to update partition status.*/
    kStatus_FLASH_SetFlexramAsEepromError =
        MAKE_STATUS(kStatusGroupFlashDriver, 11), /*!< Failed to set flexram as eeprom.*/
    kStatus_FLASH_RecoverFlexramAsRamError =
        MAKE_STATUS(kStatusGroupFlashDriver, 12), /*!< Failed to recover flexram as ram.*/
    kStatus_FLASH_SetFlexramAsRamError = MAKE_STATUS(kStatusGroupFlashDriver, 13), /*!< Failed to set flexram as ram.*/
    kStatus_FLASH_RecoverFlexramAsEepromError =
        MAKE_STATUS(kStatusGroupFlashDriver, 14), /*!< Failed to recover flexram as eeprom.*/
    kStatus_FLASH_CommandNotSupported = MAKE_STATUS(kStatusGroupFlashDriver, 15), /*!< Flash api is not supported.*/
    kStatus_FLASH_SwapSystemNotInUninitialized =
        MAKE_STATUS(kStatusGroupFlashDriver, 16), /*!< Swap system is not in uninitialzed state.*/
    kStatus_FLASH_SwapIndicatorAddressError =
        MAKE_STATUS(kStatusGroupFlashDriver, 17), /*!< Swap indicator address is invalid.*/
};
/*@}*/

/*!
 * @name Flash API key
 * @{
 */
/*! @brief Construct the four char code for flash driver API key. */
#if !defined(FOUR_CHAR_CODE)
#define FOUR_CHAR_CODE(a, b, c, d) (((d) << 24) | ((c) << 16) | ((b) << 8) | ((a)))
#endif

/*!
 * @brief Enumeration for flash driver API keys.
 *
 * @note The resulting value is built with a byte order such that the string
 * being readable in expected order when viewed in a hex editor, if the value
 * is treated as a 32-bit little endian value.
 */
enum _flash_driver_api_keys
{
    kFLASH_ApiEraseKey = FOUR_CHAR_CODE('k', 'f', 'e', 'k') /*!< Key value used to validate all flash erase APIs.*/
};
/*@}*/

/*!
 * @brief Enumeration for supported flash margin levels.
 */
typedef enum _flash_margin_value
{
    kFLASH_MarginValueNormal,  /*!< Use the 'normal' read level for 1s.*/
    kFLASH_MarginValueUser,    /*!< Apply the 'User' margin to the normal read-1 level.*/
    kFLASH_MarginValueFactory, /*!< Apply the 'Factory' margin to the normal read-1 level.*/
    kFLASH_MarginValueInvalid  /*!< Not real margin level, Used to determine the range of valid margin level. */
} flash_margin_value_t;

/*!
 * @brief Enumeration for the three possible flash security states.
 */
typedef enum _flash_security_state
{
    kFLASH_SecurityStateNotSecure,       /*!< Flash is not secure.*/
    kFLASH_SecurityStateBackdoorEnabled, /*!< Flash backdoor is enabled.*/
    kFLASH_SecurityStateBackdoorDisabled /*!< Flash backdoor is disabled.*/
} flash_security_state_t;

/*!
 * @brief Enumeration for the three possible flash protection levels.
 */
typedef enum _flash_protection_state
{
    kFLASH_ProtectionStateUnprotected, /*!< Flash region is not protected.*/
    kFLASH_ProtectionStateProtected,   /*!< Flash region is protected.*/
    kFLASH_ProtectionStateMixed        /*!< Flash is mixed with protected and unprotected region.*/
} flash_protection_state_t;

/*!
 * @brief Enumeration for the three possible flash execute access levels.
 */
typedef enum _flash_execute_only_access_state
{
    kFLASH_AccessStateUnLimited,   /*!< Flash region is unLimited.*/
    kFLASH_AccessStateExecuteOnly, /*!< Flash region is execute only.*/
    kFLASH_AccessStateMixed        /*!< Flash is mixed with unLimited and execute only region.*/
} flash_execute_only_access_state_t;

/*!
 * @brief Enumeration for various flash properties.
 */
typedef enum _flash_property_tag
{
    kFLASH_PropertyPflashSectorSize = 0x00U,         /*!< Pflash sector size property.*/
    kFLASH_PropertyPflashTotalSize = 0x01U,          /*!< Pflash total size property.*/
    kFLASH_PropertyPflashBlockSize = 0x02U,          /*!< Pflash block size property.*/
    kFLASH_PropertyPflashBlockCount = 0x03U,         /*!< Pflash block count property.*/
    kFLASH_PropertyPflashBlockBaseAddr = 0x04U,      /*!< Pflash block base address property.*/
    kFLASH_PropertyPflashFacSupport = 0x05U,         /*!< Pflash fac support property.*/
    kFLASH_PropertyPflashAccessSegmentSize = 0x06U,  /*!< Pflash access segment size property.*/
    kFLASH_PropertyPflashAccessSegmentCount = 0x07U, /*!< Pflash access segment count property.*/
    kFLASH_PropertyFlexRamBlockBaseAddr = 0x08U,     /*!< FlexRam block base address property.*/
    kFLASH_PropertyFlexRamTotalSize = 0x09U,         /*!< FlexRam total size property.*/
    kFLASH_PropertyDflashSectorSize = 0x10U,         /*!< Dflash sector size property.*/
    kFLASH_PropertyDflashTotalSize = 0x11U,          /*!< Dflash total size property.*/
    kFLASH_PropertyDflashBlockSize = 0x12U,          /*!< Dflash block count property.*/
    kFLASH_PropertyDflashBlockCount = 0x13U,         /*!< Dflash block base address property.*/
    kFLASH_PropertyDflashBlockBaseAddr = 0x14U,      /*!< Eeprom total size property.*/
    kFLASH_PropertyEepromTotalSize = 0x15U
} flash_property_tag_t;

/*!
 * @brief Constants for execute-in-ram flash function.
 */
enum _flash_execute_in_ram_function_constants
{
    kFLASH_ExecuteInRamFunctionMaxSize = 64U, /*!< Max size of execute-in-ram function.*/
    kFLASH_ExecuteInRamFunctionTotalNum = 2U  /*!< Total number of execute-in-ram functions.*/
};

/*!
 * @brief Flash execute-in-ram function information.
 */
typedef struct _flash_execute_in_ram_function_config
{
    uint32_t activeFunctionCount;    /*!< Number of available execute-in-ram functions.*/
    uint8_t *flashRunCommand;        /*!< execute-in-ram function: flash_run_command.*/
    uint8_t *flashCacheClearCommand; /*!< execute-in-ram function: flash_cache_clear_command.*/
} flash_execute_in_ram_function_config_t;

/*!
 * @brief Enumeration for the two possible options of flash read resource command.
 */
typedef enum _flash_read_resource_option
{
    kFLASH_ResourceOptionFlashIfr =
        0x00U, /*!< Select code for Program flash 0 IFR, Program flash swap 0 IFR, Data flash 0 IFR */
    kFLASH_ResourceOptionVersionId = 0x01U /*!< Select code for Version ID*/
} flash_read_resource_option_t;

/*!
 * @brief Enumeration for the range of special-purpose flash resource
 */
enum _flash_read_resource_range
{
#if (FSL_FEATURE_FLASH_IS_FTFE == 1)
    kFLASH_ResourceRangePflashIfrSizeInBytes = 1024U, /*!< Pflash IFR size in byte.*/
    kFLASH_ResourceRangeVersionIdSizeInBytes = 8U,    /*!< Version ID IFR size in byte.*/
    kFLASH_ResourceRangeVersionIdStart = 0x08U,       /*!< Version ID IFR start address.*/
    kFLASH_ResourceRangeVersionIdEnd = 0x0FU,         /*!< Version ID IFR end address.*/
#else                                                 /* FSL_FEATURE_FLASH_IS_FTFL == 1 or FSL_FEATURE_FLASH_IS_FTFA = =1 */
    kFLASH_ResourceRangePflashIfrSizeInBytes = 256U, /*!< Pflash IFR size in byte.*/
    kFLASH_ResourceRangeVersionIdSizeInBytes = 8U,   /*!< Version ID IFR size in byte.*/
    kFLASH_ResourceRangeVersionIdStart = 0x00U,      /*!< Version ID IFR start address.*/
    kFLASH_ResourceRangeVersionIdEnd = 0x07U,        /*!< Version ID IFR end address.*/
#endif
    kFLASH_ResourceRangePflashSwapIfrStart = 0x40000U, /*!< Pflash swap IFR start address.*/
    kFLASH_ResourceRangePflashSwapIfrEnd = 0x403FFU,   /*!< Pflash swap IFR end address.*/
    kFLASH_ResourceRangeDflashIfrStart = 0x800000U,    /*!< Dflash IFR start address.*/
    kFLASH_ResourceRangeDflashIfrEnd = 0x8003FFU,      /*!< Dflash IFR end address.*/
};

/*!
 * @brief Enumeration for the two possilbe options of set flexram function command.
 */
typedef enum _flash_flexram_function_option
{
    kFLASH_FlexramFunctionOptionAvailableAsRam = 0xFFU,    /*!< Option used to make FlexRAM available as RAM */
    kFLASH_FlexramFunctionOptionAvailableForEeprom = 0x00U /*!< Option used to make FlexRAM available for EEPROM */
} flash_flexram_function_option_t;

/*!
 * @brief Enumeration for the possible options of Swap function
 */
typedef enum _flash_swap_function_option
{
    kFLASH_SwapFunctionOptionEnable = 0x00U, /*!< Option used to enable Swap function */
    kFLASH_SwapFunctionOptionDisable = 0x01U /*!< Option used to Disable Swap function */
} flash_swap_function_option_t;

/*!
 * @brief Enumeration for the possible options of Swap Control commands
 */
typedef enum _flash_swap_control_option
{
    kFLASH_SwapControlOptionIntializeSystem = 0x01U,    /*!< Option used to Intialize Swap System */
    kFLASH_SwapControlOptionSetInUpdateState = 0x02U,   /*!< Option used to Set Swap in Update State */
    kFLASH_SwapControlOptionSetInCompleteState = 0x04U, /*!< Option used to Set Swap in Complete State */
    kFLASH_SwapControlOptionReportStatus = 0x08U,       /*!< Option used to Report Swap Status */
    kFLASH_SwapControlOptionDisableSystem = 0x10U       /*!< Option used to Disable Swap Status */
} flash_swap_control_option_t;

/*!
 * @brief Enumeration for the possible flash swap status.
 */
typedef enum _flash_swap_state
{
    kFLASH_SwapStateUninitialized = 0x00U, /*!< Flash swap system is in uninitialized state.*/
    kFLASH_SwapStateReady = 0x01U,         /*!< Flash swap system is in ready state.*/
    kFLASH_SwapStateUpdate = 0x02U,        /*!< Flash swap system is in update state.*/
    kFLASH_SwapStateUpdateErased = 0x03U,  /*!< Flash swap system is in updateErased state.*/
    kFLASH_SwapStateComplete = 0x04U,      /*!< Flash swap system is in complete state.*/
    kFLASH_SwapStateDisabled = 0x05U       /*!< Flash swap system is in disabled state.*/
} flash_swap_state_t;

/*!
 * @breif Enumeration for the possible flash swap block status
 */
typedef enum _flash_swap_block_status
{
    kFLASH_SwapBlockStatusLowerHalfProgramBlocksAtZero =
        0x00U, /*!< Swap block status is that lower half program block at zero.*/
    kFLASH_SwapBlockStatusUpperHalfProgramBlocksAtZero =
        0x01U, /*!< Swap block status is that upper half program block at zero.*/
} flash_swap_block_status_t;

/*!
 * @brief Flash Swap information.
 */
typedef struct _flash_swap_state_config
{
    flash_swap_state_t flashSwapState;                /*!< Current swap system status.*/
    flash_swap_block_status_t currentSwapBlockStatus; /*!< Current swap block status.*/
    flash_swap_block_status_t nextSwapBlockStatus;    /*!< Next swap block status.*/
} flash_swap_state_config_t;

/*!
 * @brief Flash Swap IFR fileds.
 */
typedef struct _flash_swap_ifr_field_config
{
    uint16_t swapIndicatorAddress; /*!< Swap indicator address field.*/
    uint16_t swapEnableWord;       /*!< Swap enable word field.*/
    uint8_t reserved0[6];          /*!< Reserved field.*/
    uint16_t swapDisableWord;      /*!< Swap disable word field.*/
    uint8_t reserved1[4];          /*!< Reserved field.*/
} flash_swap_ifr_field_config_t;

/*!
 * @brief Enumeration for FlexRAM load during reset option.
 */
typedef enum _flash_partition_flexram_load_option
{
    kFLASH_PartitionFlexramLoadOptionLoadedWithValidEepromData =
        0x00U, /*!< FlexRAM is loaded with valid EEPROM data during reset sequence.*/
    kFLASH_PartitionFlexramLoadOptionNotLoaded = 0x01U /*!< FlexRAM is not loaded during reset sequence.*/
} flash_partition_flexram_load_option_t;

/*! @brief callback type used for pflash block*/
typedef void (*flash_callback_t)(void);

/*!
 * @brief Active flash information for current operation.
 */
typedef struct _flash_operation_config
{
    uint32_t convertedAddress;           /*!< Converted address for current flash type.*/
    uint32_t activeSectorSize;           /*!< Sector size of current flash type.*/
    uint32_t activeBlockSize;            /*!< Block size of current flash type.*/
    uint32_t blockWriteUnitSize;         /*!< write unit size.*/
    uint32_t sectorCmdAddressAligment;   /*!< Erase sector command address alignment.*/
    uint32_t sectionCmdAddressAligment;  /*!< Program/Verify section command address alignment.*/
    uint32_t resourceCmdAddressAligment; /*!< Read resource command address alignment.*/
    uint32_t checkCmdAddressAligment;    /*!< Program check command address alignment.*/
} flash_operation_config_t;

/*! @brief Flash driver state information.
 *
 * An instance of this structure is allocated by the user of the flash driver and
 * passed into each of the driver APIs.
 */
typedef struct _flash_config
{
    uint32_t PFlashBlockBase;                /*!< Base address of the first PFlash block */
    uint32_t PFlashTotalSize;                /*!< Size of all combined PFlash block. */
    uint32_t PFlashBlockCount;               /*!< Number of PFlash blocks. */
    uint32_t PFlashSectorSize;               /*!< Size in bytes of a sector of PFlash. */
    flash_callback_t PFlashCallback;         /*!< Callback function for flash API. */
    uint32_t PFlashAccessSegmentSize;        /*!< Size in bytes of a access segment of PFlash. */
    uint32_t PFlashAccessSegmentCount;       /*!< Number of PFlash access segments. */
    uint32_t *flashExecuteInRamFunctionInfo; /*!< Info struct of flash execute-in-ram function. */
    uint32_t FlexRAMBlockBase;               /*!< For FlexNVM device, this is the base address of FlexRAM
                                                  For non-FlexNVM device, this is the base address of acceleration RAM memory */
    uint32_t FlexRAMTotalSize;               /*!< For FlexNVM device, this is the size of FlexRAM
                                                  For non-FlexNVM device, this is the size of acceleration RAM memory */
    uint32_t DFlashBlockBase; /*!< For FlexNVM device, this is the base address of D-Flash memory (FlexNVM memory);
                                   For non-FlexNVM device, this field is unused */
    uint32_t DFlashTotalSize; /*!< For FlexNVM device, this is total size of the FlexNVM memory;
                                   For non-FlexNVM device, this field is unused */
    uint32_t EEpromTotalSize; /*!< For FlexNVM device, this is the size in byte of EEPROM area which was partitioned
                                 from FlexRAM;
                                   For non-FlexNVM device, this field is unused */
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
 * @brief Initializes global flash properties structure members
 *
 * This function checks and initializes Flash module for the other Flash APIs.
 *
 * @param config Pointer to storage for the driver runtime state.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_PartitionStatusUpdateFailure Failed to update partition status.
 */
status_t FLASH_Init(flash_config_t *config);

/*!
 * @brief Set the desired flash callback function
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param callback callback function to be stored in driver
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 */
status_t FLASH_SetCallback(flash_config_t *config, flash_callback_t callback);

/*!
 * @brief Prepare flash execute-in-ram functions
 *
 * @param config Pointer to storage for the driver runtime state.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 */
#if FLASH_DRIVER_IS_FLASH_RESIDENT
status_t FLASH_PrepareExecuteInRamFunctions(flash_config_t *config);
#endif

/*@}*/

/*!
 * @name Erasing
 * @{
 */

/*!
 * @brief Erases entire flash
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param key value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_EraseKeyError Api erase key is invalid.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 * @retval #kStatus_FLASH_PartitionStatusUpdateFailure Failed to update partition status
 */
status_t FLASH_EraseAll(flash_config_t *config, uint32_t key);

/*!
 * @brief Erases flash sectors encompassed by parameters passed into function
 *
 * This function erases the appropriate number of flash sectors based on the
 * desired start address and length.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be erased.
 *              The start address does not need to be sector aligned but must be word-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *                      to be erased. Must be word aligned.
 * @param key value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 * @retval #kStatus_FLASH_EraseKeyError Api erase key is invalid.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_Erase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint32_t key);

/*!
 * @brief Erases entire flash, including protected sectors.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param key value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_EraseKeyError Api erase key is invalid.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 * @retval #kStatus_FLASH_PartitionStatusUpdateFailure Failed to update partition status
 */
#if defined(FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD) && FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD
status_t FLASH_EraseAllUnsecure(flash_config_t *config, uint32_t key);
#endif

/*!
 * @brief Erases all program flash execute-only segments defined by the FXACC registers.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param key value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_EraseKeyError Api erase key is invalid.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_EraseAllExecuteOnlySegments(flash_config_t *config, uint32_t key);

/*@}*/

/*!
 * @name Programming
 * @{
 */

/*!
 * @brief Programs flash with data at locations passed in through parameters
 *
 * This function programs the flash memory with desired data for a given
 * flash area as determined by the start address and length.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be programmed. Must be
 *              word-aligned.
 * @param src Pointer to the source buffer of data that is to be programmed
 *            into the flash.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *                      to be programmed. Must be word-aligned.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_Program(flash_config_t *config, uint32_t start, uint32_t *src, uint32_t lengthInBytes);

/*!
 * @brief Programs Program Once Field through parameters
 *
 * This function programs the Program Once Field with desired data for a given
 * flash area as determined by the index and length.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param index The index indicating which area of Program Once Field to be programmed.
 * @param src Pointer to the source buffer of data that is to be programmed
 *            into the Program Once Field.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *                      to be programmed. Must be word-aligned.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_ProgramOnce(flash_config_t *config, uint32_t index, uint32_t *src, uint32_t lengthInBytes);

/*!
 * @brief Programs flash with data at locations passed in through parameters via Program Section command
 *
 * This function programs the flash memory with desired data for a given
 * flash area as determined by the start address and length.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be programmed. Must be
 *              word-aligned.
 * @param src Pointer to the source buffer of data that is to be programmed
 *            into the flash.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *                      to be programmed. Must be word-aligned.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 * @retval #kStatus_FLASH_SetFlexramAsRamError Failed to set flexram as ram
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 * @retval #kStatus_FLASH_RecoverFlexramAsEepromError Failed to recover flexram as eeprom
 */
#if defined(FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD) && FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD
status_t FLASH_ProgramSection(flash_config_t *config, uint32_t start, uint32_t *src, uint32_t lengthInBytes);
#endif

/*!
 * @brief Programs EEPROM with data at locations passed in through parameters
 *
 * This function programs the Emulated EEPROM with desired data for a given
 * flash area as determined by the start address and length.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be programmed. Must be
 *              word-aligned.
 * @param src Pointer to the source buffer of data that is to be programmed
 *            into the flash.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *                      to be programmed. Must be word-aligned.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 * @retval #kStatus_FLASH_SetFlexramAsEepromError Failed to set flexram as eeprom.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_RecoverFlexramAsRamError Failed to recover flexram as ram
 */
#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_EepromWrite(flash_config_t *config, uint32_t start, uint8_t *src, uint32_t lengthInBytes);
#endif

/*@}*/

/*!
 * @name Reading
 * @{
 */

/*!
 * @brief Read resource with data at locations passed in through parameters
 *
 * This function reads the flash memory with desired location for a given
 * flash area as determined by the start address and length.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be programmed. Must be
 *              word-aligned.
 * @param dst Pointer to the destination buffer of data that is used to store
 *        data to be read.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *        to be read. Must be word-aligned.
 * @param option The resource option which indicates which area should be read back.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
status_t FLASH_ReadResource(
    flash_config_t *config, uint32_t start, uint32_t *dst, uint32_t lengthInBytes, flash_read_resource_option_t option);
#endif

/*!
 * @brief Read Program Once Field through parameters
 *
 * This function reads the read once feild with given index and length
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param index The index indicating the area of program once field to be read.
 * @param dst Pointer to the destination buffer of data that is used to store
 *        data to be read.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *        to be programmed. Must be word-aligned.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_ReadOnce(flash_config_t *config, uint32_t index, uint32_t *dst, uint32_t lengthInBytes);

/*@}*/

/*!
 * @name Security
 * @{
 */

/*!
 * @brief Returns the security state via the pointer passed into the function
 *
 * This function retrieves the current Flash security status, including the
 * security enabling state and the backdoor key enabling state.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param state Pointer to the value returned for the current security status code:
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 */
status_t FLASH_GetSecurityState(flash_config_t *config, flash_security_state_t *state);

/*!
 * @brief Allows user to bypass security with a backdoor key
 *
 * If the MCU is in secured state, this function will unsecure the MCU by
 * comparing the provided backdoor key with ones in the Flash Configuration
 * Field.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param backdoorKey Pointer to the user buffer containing the backdoor key.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_SecurityBypass(flash_config_t *config, const uint8_t *backdoorKey);

/*@}*/

/*!
 * @name Verification
 * @{
 */

/*!
 * @brief Verifies erasure of entire flash at specified margin level
 *
 * This function will check to see if the flash have been erased to the
 * specified read margin level.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param margin Read margin choice
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_VerifyEraseAll(flash_config_t *config, flash_margin_value_t margin);

/*!
 * @brief Verifies erasure of desired flash area at specified margin level
 *
 * This function will check the appropriate number of flash sectors based on
 * the desired start address and length to see if the flash have been erased
 * to the specified read margin level.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be verified.
 *        The start address does not need to be sector aligned but must be word-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *        to be verified. Must be word-aligned.
 * @param margin Read margin choice
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_VerifyErase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, flash_margin_value_t margin);

/*!
 * @brief Verifies programming of desired flash area at specified margin level
 *
 * This function verifies the data programed in the flash memory using the
 * Flash Program Check Command and compares it with expected data for a given
 * flash area as determined by the start address and length.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be verified. Must be word-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *        to be verified. Must be word-aligned.
 * @param expectedData Pointer to the expected data that is to be
 *        verified against.
 * @param margin Read margin choice
 * @param failedAddress Pointer to returned failing address.
 * @param failedData Pointer to returned failing data.  Some derivitives do
 *        not included failed data as part of the FCCOBx registers.  In this
 *        case, zeros are returned upon failure.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_VerifyProgram(flash_config_t *config,
                             uint32_t start,
                             uint32_t lengthInBytes,
                             const uint32_t *expectedData,
                             flash_margin_value_t margin,
                             uint32_t *failedAddress,
                             uint32_t *failedData);

/*!
 * @brief Verifies if the program flash executeonly segments have been erased to
 *  the specified read margin level
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param margin Read margin choice
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_VerifyEraseAllExecuteOnlySegments(flash_config_t *config, flash_margin_value_t margin);

/*@}*/

/*!
 * @name Protection
 * @{
 */

/*!
 * @brief Returns the protection state of desired flash area via the pointer passed into the function
 *
 * This function retrieves the current Flash protect status for a given
 * flash area as determined by the start address and length.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be checked. Must be word-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *        to be checked.  Must be word-aligned.
 * @param protection_state Pointer to the value returned for the current
 *        protection status code for the desired flash area.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 */
status_t FLASH_IsProtected(flash_config_t *config,
                           uint32_t start,
                           uint32_t lengthInBytes,
                           flash_protection_state_t *protection_state);

/*!
 * @brief Returns the access state of desired flash area via the pointer passed into the function
 *
 * This function retrieves the current Flash access status for a given
 * flash area as determined by the start address and length.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be checked. Must be word-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *        to be checked.  Must be word-aligned.
 * @param access_state Pointer to the value returned for the current
 *        access status code for the desired flash area.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 */
status_t FLASH_IsExecuteOnly(flash_config_t *config,
                             uint32_t start,
                             uint32_t lengthInBytes,
                             flash_execute_only_access_state_t *access_state);

/*@}*/

/*!
 * @name Properties
 * @{
 */

/*!
 * @brief Returns the desired flash property.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param whichProperty The desired property from the list of properties in
 *        enum flash_property_tag_t
 * @param value Pointer to the value returned for the desired flash property
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_UnknownProperty unknown property tag
 */
status_t FLASH_GetProperty(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t *value);

/*@}*/

/*!
 * @name FlexRAM
 * @{
 */

/*!
 * @brief Set FlexRAM Function command
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param option The option used to set work mode of FlexRAM
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
status_t FLASH_SetFlexramFunction(flash_config_t *config, flash_flexram_function_option_t option);
#endif

/*@}*/

/*!
 * @name Swap
 * @{
 */

/*!
 * @brief Configure Swap function or Check the swap state of Flash Module
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param address Address used to configure the flash swap function
 * @param option The possible option used to configure Flash Swap function or check the flash swap status
 * @param returnInfo Pointer to the data which is used to return the information of flash swap.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_SwapIndicatorAddressError Swap indicator address is invalid
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
#if defined(FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD) && FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD
status_t FLASH_SwapControl(flash_config_t *config,
                           uint32_t address,
                           flash_swap_control_option_t option,
                           flash_swap_state_config_t *returnInfo);
#endif

/*!
 * @brief Swap the lower half flash with the higher half flaock
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param address Address used to configure the flash swap function
 * @param option The possible option used to configure Flash Swap function or check the flash swap status
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_SwapIndicatorAddressError Swap indicator address is invalid
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 * @retval #kStatus_FLASH_SwapSystemNotInUninitialized Swap system is not in uninitialzed state
 */
#if defined(FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP) && FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP
status_t FLASH_Swap(flash_config_t *config, uint32_t address, flash_swap_function_option_t option);
#endif

/*!
 * @name FlexNVM
 * @{
 */

/*!
 * @brief Prepares the FlexNVM block for use as data flash, EEPROM backup, or a combination of both and initializes the
 * FlexRAM.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param option The option used to set FlexRAM load behavior during reset.
 * @param eepromDataSizeCode Determines the amount of FlexRAM used in each of the available EEPROM subsystems.
 * @param flexnvmPartitionCode Specifies how to split the FlexNVM block between data flash memory and EEPROM backup
 *        memory supporting EEPROM functions.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-ram function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
#if defined(FSL_FEATURE_FLASH_HAS_PROGRAM_PARTITION_CMD) && FSL_FEATURE_FLASH_HAS_PROGRAM_PARTITION_CMD
status_t FLASH_ProgramPartition(flash_config_t *config,
                                flash_partition_flexram_load_option_t option,
                                uint32_t eepromDataSizeCode,
                                uint32_t flexnvmPartitionCode);
#endif

/*@}*/

/*!
* @name Flash Protection Utilities
* @{
*/

/*!
 * @brief Set PFLASH Protection to the intended protection status.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param protectStatus The expected protect status user wants to set to PFlash protection register. Each bit is
 * corresponding to protection of 1/32 of the total PFlash. The least significant bit is corresponding to the lowest
 * address area of P-Flash. The most significant bit is corresponding to the highest address area of PFlash. There are
 * two possible cases as shown below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_PflashSetProtection(flash_config_t *config, uint32_t protectStatus);

/*!
 * @brief Get PFLASH Protection Status.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param protectStatus  Protect status returned by PFlash IP. Each bit is corresponding to protection of 1/32 of the
 * total PFlash. The least significant bit is corresponding to the lowest address area of PFlash. The most significant
 * bit is corresponding to the highest address area of PFlash. Thee are two possible cases as below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 */
status_t FLASH_PflashGetProtection(flash_config_t *config, uint32_t *protectStatus);

/*!
 * @brief Set DFLASH Protection to the intended protection status.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param protectStatus The expected protect status user wants to set to DFlash protection register. Each bit is
 * corresponding to protection of 1/8 of the total DFlash. The least significant bit is corresponding to the lowest
 * address area of DFlash. The most significant bit is corresponding to the highest address area of  DFlash. There are
 * two possible cases as shown below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_CommandNotSupported Flash api is not supported
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_DflashSetProtection(flash_config_t *config, uint8_t protectStatus);
#endif

/*!
 * @brief Get DFLASH Protection Status.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param protectStatus  DFlash Protect status returned by PFlash IP. Each bit is corresponding to protection of 1/8 of
 * the total DFlash. The least significant bit is corresponding to the lowest address area of DFlash. The most
 * significant bit is corresponding to the highest address area of DFlash and so on. There are two possible cases as
 * below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_CommandNotSupported Flash api is not supported
 */
#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_DflashGetProtection(flash_config_t *config, uint8_t *protectStatus);
#endif

/*!
 * @brief Set EEPROM Protection to the intended protection status.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param protectStatus The expected protect status user wants to set to EEPROM protection register. Each bit is
 * corresponding to protection of 1/8 of the total EEPROM. The least significant bit is corresponding to the lowest
 * address area of EEPROM. The most significant bit is corresponding to the highest address area of EEPROM, and so on.
 * There are two possible cases as shown below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_CommandNotSupported Flash api is not supported
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_EepromSetProtection(flash_config_t *config, uint8_t protectStatus);
#endif

/*!
 * @brief Get DFLASH Protection Status.
 *
 * @param config Pointer to storage for the driver runtime state.
 * @param protectStatus  DFlash Protect status returned by PFlash IP. Each bit is corresponding to protection of 1/8 of
 * the total EEPROM. The least significant bit is corresponding to the lowest address area of EEPROM. The most
 * significant bit is corresponding to the highest address area of EEPROM. There are two possible cases as below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success Api was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_CommandNotSupported Flash api is not supported.
 */
#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_EepromGetProtection(flash_config_t *config, uint8_t *protectStatus);
#endif

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_FLASH_H_ */
