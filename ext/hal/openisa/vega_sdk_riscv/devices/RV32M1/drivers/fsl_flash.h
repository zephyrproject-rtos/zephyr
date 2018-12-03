/*
 * Copyright (c) 2013-2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
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
/*! @brief Constructs the version number for drivers. */
#if !defined(MAKE_VERSION)
#define MAKE_VERSION(major, minor, bugfix) (((major) << 16) | ((minor) << 8) | (bugfix))
#endif

/*! @brief Flash driver version for SDK*/
#define FSL_FLASH_DRIVER_VERSION (MAKE_VERSION(2, 4, 1)) /*!< Version 2.4.1. */

/*! @brief Flash driver version for ROM*/
enum _flash_driver_version_constants
{
    kFLASH_DriverVersionName = 'F', /*!< Flash driver version name.*/
    kFLASH_DriverVersionMajor = 2,  /*!< Major flash driver version.*/
    kFLASH_DriverVersionMinor = 4,  /*!< Minor flash driver version.*/
    kFLASH_DriverVersionBugfix = 1  /*!< Bugfix for flash driver version.*/
};
/*@}*/

/*!
 * @name Flash configuration
 * @{
 */
/*! @brief Indicates whether to support FlexNVM in the Flash driver */
#if !defined(FLASH_SSD_CONFIG_ENABLE_FLEXNVM_SUPPORT)
#define FLASH_SSD_CONFIG_ENABLE_FLEXNVM_SUPPORT 1 /*!< Enables the FlexNVM support by default. */
#endif

/*! @brief Indicates whether the FlexNVM is enabled in the Flash driver */
#define FLASH_SSD_IS_FLEXNVM_ENABLED (FLASH_SSD_CONFIG_ENABLE_FLEXNVM_SUPPORT && FSL_FEATURE_FLASH_HAS_FLEX_NVM)

/*! @brief Indicates whether to support Secondary flash in the Flash driver */
#if !defined(FLASH_SSD_CONFIG_ENABLE_SECONDARY_FLASH_SUPPORT)
#define FLASH_SSD_CONFIG_ENABLE_SECONDARY_FLASH_SUPPORT 1 /*!< Enables the secondary flash support by default. */
#endif

/*! @brief Indicates whether the secondary flash is supported in the Flash driver */
#if defined(FSL_FEATURE_FLASH_HAS_MULTIPLE_FLASH) || defined(FSL_FEATURE_FLASH_PFLASH_1_START_ADDRESS)
#define FLASH_SSD_IS_SECONDARY_FLASH_ENABLED (FLASH_SSD_CONFIG_ENABLE_SECONDARY_FLASH_SUPPORT)
#else
#define FLASH_SSD_IS_SECONDARY_FLASH_ENABLED (0)
#endif

/*! @brief Flash driver location. */
#if !defined(FLASH_DRIVER_IS_FLASH_RESIDENT)
#if (!defined(BL_TARGET_ROM) && !defined(BL_TARGET_RAM))
#define FLASH_DRIVER_IS_FLASH_RESIDENT 1 /*!< Used for the flash resident application. */
#else
#define FLASH_DRIVER_IS_FLASH_RESIDENT 0 /*!< Used for the non-flash resident application. */
#endif
#endif

/*! @brief Flash Driver Export option */
#if !defined(FLASH_DRIVER_IS_EXPORTED)
#if (defined(BL_TARGET_ROM) || defined(BL_TARGET_FLASH))
#define FLASH_DRIVER_IS_EXPORTED 1 /*!< Used for the ROM bootloader. */
#else
#define FLASH_DRIVER_IS_EXPORTED 0 /*!< Used for the MCUXpresso SDK application. */
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
    kStatus_FLASH_PartitionStatusUpdateFailure =
        MAKE_STATUS(kStatusGroupFlashDriver, 10), /*!< Failed to update partition status.*/
    kStatus_FLASH_SetFlexramAsEepromError =
        MAKE_STATUS(kStatusGroupFlashDriver, 11), /*!< Failed to set FlexRAM as EEPROM.*/
    kStatus_FLASH_RecoverFlexramAsRamError =
        MAKE_STATUS(kStatusGroupFlashDriver, 12), /*!< Failed to recover FlexRAM as RAM.*/
    kStatus_FLASH_SetFlexramAsRamError = MAKE_STATUS(kStatusGroupFlashDriver, 13), /*!< Failed to set FlexRAM as RAM.*/
    kStatus_FLASH_RecoverFlexramAsEepromError =
        MAKE_STATUS(kStatusGroupFlashDriver, 14), /*!< Failed to recover FlexRAM as EEPROM.*/
    kStatus_FLASH_CommandNotSupported = MAKE_STATUS(kStatusGroupFlashDriver, 15), /*!< Flash API is not supported.*/
    kStatus_FLASH_SwapSystemNotInUninitialized =
        MAKE_STATUS(kStatusGroupFlashDriver, 16), /*!< Swap system is not in an uninitialzed state.*/
    kStatus_FLASH_SwapIndicatorAddressError =
        MAKE_STATUS(kStatusGroupFlashDriver, 17), /*!< The swap indicator address is invalid.*/
    kStatus_FLASH_ReadOnlyProperty = MAKE_STATUS(kStatusGroupFlashDriver, 18), /*!< The flash property is read-only.*/
    kStatus_FLASH_InvalidPropertyValue =
        MAKE_STATUS(kStatusGroupFlashDriver, 19), /*!< The flash property value is out of range.*/
    kStatus_FLASH_InvalidSpeculationOption =
        MAKE_STATUS(kStatusGroupFlashDriver, 20), /*!< The option of flash prefetch speculation is invalid.*/
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
    kFLASH_SecurityStateNotSecure = 0xc33cc33cU,       /*!< Flash is not secure.*/
    kFLASH_SecurityStateBackdoorEnabled = 0x5aa55aa5U, /*!< Flash backdoor is enabled.*/
    kFLASH_SecurityStateBackdoorDisabled = 0x5ac33ca5U /*!< Flash backdoor is disabled.*/
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
    kFLASH_AccessStateUnLimited,   /*!< Flash region is unlimited.*/
    kFLASH_AccessStateExecuteOnly, /*!< Flash region is execute only.*/
    kFLASH_AccessStateMixed        /*!< Flash is mixed with unlimited and execute only region.*/
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
    kFLASH_PropertyDflashBlockSize = 0x12U,          /*!< Dflash block size property.*/
    kFLASH_PropertyDflashBlockCount = 0x13U,         /*!< Dflash block count property.*/
    kFLASH_PropertyDflashBlockBaseAddr = 0x14U,      /*!< Dflash block base address property.*/
    kFLASH_PropertyEepromTotalSize = 0x15U,          /*!< EEPROM total size property.*/
    kFLASH_PropertyFlashMemoryIndex = 0x20U          /*!< Flash memory index property.*/
} flash_property_tag_t;

/*!
 * @brief Constants for execute-in-RAM flash function.
 */
enum _flash_execute_in_ram_function_constants
{
    kFLASH_ExecuteInRamFunctionMaxSizeInWords = 16U, /*!< The maximum size of execute-in-RAM function.*/
    kFLASH_ExecuteInRamFunctionTotalNum = 2U         /*!< Total number of execute-in-RAM functions.*/
};

/*!
 * @brief Flash execute-in-RAM function information.
 */
typedef struct _flash_execute_in_ram_function_config
{
    uint32_t activeFunctionCount;      /*!< Number of available execute-in-RAM functions.*/
    uint32_t *flashRunCommand;         /*!< Execute-in-RAM function: flash_run_command.*/
    uint32_t *flashCommonBitOperation; /*!< Execute-in-RAM function: flash_common_bit_operation.*/
} flash_execute_in_ram_function_config_t;

/*!
 * @brief Enumeration for the two possible options of flash read resource command.
 */
typedef enum _flash_read_resource_option
{
    kFLASH_ResourceOptionFlashIfr =
        0x00U, /*!< Select code for Program flash 0 IFR, Program flash swap 0 IFR, Data flash 0 IFR */
    kFLASH_ResourceOptionVersionId = 0x01U /*!< Select code for the version ID*/
} flash_read_resource_option_t;

/*!
 * @brief Enumeration for the range of special-purpose flash resource
 */
enum _flash_read_resource_range
{
#if (FSL_FEATURE_FLASH_IS_FTFE == 1)
    kFLASH_ResourceRangePflashIfrSizeInBytes = 1024U,  /*!< Pflash IFR size in byte.*/
    kFLASH_ResourceRangeVersionIdSizeInBytes = 8U,     /*!< Version ID IFR size in byte.*/
    kFLASH_ResourceRangeVersionIdStart = 0x08U,        /*!< Version ID IFR start address.*/
    kFLASH_ResourceRangeVersionIdEnd = 0x0FU,          /*!< Version ID IFR end address.*/
    kFLASH_ResourceRangePflashSwapIfrStart = 0x40000U, /*!< Pflash swap IFR start address.*/
    kFLASH_ResourceRangePflashSwapIfrEnd =
        (kFLASH_ResourceRangePflashSwapIfrStart + 0x3FFU), /*!< Pflash swap IFR end address.*/
#else                                                      /* FSL_FEATURE_FLASH_IS_FTFL == 1 or FSL_FEATURE_FLASH_IS_FTFA = =1 */
    kFLASH_ResourceRangePflashIfrSizeInBytes = 256U,  /*!< Pflash IFR size in byte.*/
    kFLASH_ResourceRangeVersionIdSizeInBytes = 8U,    /*!< Version ID IFR size in byte.*/
    kFLASH_ResourceRangeVersionIdStart = 0x00U,       /*!< Version ID IFR start address.*/
    kFLASH_ResourceRangeVersionIdEnd = 0x07U,         /*!< Version ID IFR end address.*/
#if 0x20000U == (FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT * FSL_FEATURE_FLASH_PFLASH_BLOCK_SIZE)
    kFLASH_ResourceRangePflashSwapIfrStart = 0x8000U, /*!< Pflash swap IFR start address.*/
#elif 0x40000U == (FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT * FSL_FEATURE_FLASH_PFLASH_BLOCK_SIZE)
    kFLASH_ResourceRangePflashSwapIfrStart = 0x10000U, /*!< Pflash swap IFR start address.*/
#elif 0x80000U == (FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT * FSL_FEATURE_FLASH_PFLASH_BLOCK_SIZE)
    kFLASH_ResourceRangePflashSwapIfrStart = 0x20000U, /*!< Pflash swap IFR start address.*/
#else
    kFLASH_ResourceRangePflashSwapIfrStart = 0,
#endif
    kFLASH_ResourceRangePflashSwapIfrEnd =
        (kFLASH_ResourceRangePflashSwapIfrStart + 0xFFU), /*!< Pflash swap IFR end address.*/
#endif
    kFLASH_ResourceRangeDflashIfrStart = 0x800000U, /*!< Dflash IFR start address.*/
    kFLASH_ResourceRangeDflashIfrEnd = 0x8003FFU,   /*!< Dflash IFR end address.*/
};

/*!
 * @brief Enumeration for the index of read/program once record
 */
enum _k3_flash_read_once_index
{
    kFLASH_RecordIndexSwapAddr = 0xA1U,    /*!< Index of Swap indicator address.*/
    kFLASH_RecordIndexSwapEnable = 0xA2U,  /*!< Index of Swap system enable.*/
    kFLASH_RecordIndexSwapDisable = 0xA3U, /*!< Index of Swap system disable.*/
};

/*!
 * @brief Enumeration for the two possilbe options of set FlexRAM function command.
 */
typedef enum _flash_flexram_function_option
{
    kFLASH_FlexramFunctionOptionAvailableAsRam = 0xFFU,    /*!< An option used to make FlexRAM available as RAM */
    kFLASH_FlexramFunctionOptionAvailableForEeprom = 0x00U /*!< An option used to make FlexRAM available for EEPROM */
} flash_flexram_function_option_t;

/*!
 * @brief Enumeration for acceleration RAM property.
 */
enum _flash_acceleration_ram_property
{
    kFLASH_AccelerationRamSize = 0x400U
};

/*!
 * @brief Enumeration for the possible options of Swap function
 */
typedef enum _flash_swap_function_option
{
    kFLASH_SwapFunctionOptionEnable = 0x00U, /*!< An option used to enable the Swap function */
    kFLASH_SwapFunctionOptionDisable = 0x01U /*!< An option used to disable the Swap function */
} flash_swap_function_option_t;

/*!
 * @brief Enumeration for the possible options of Swap control commands
 */
typedef enum _flash_swap_control_option
{
    kFLASH_SwapControlOptionIntializeSystem = 0x01U,    /*!< An option used to initialize the Swap system */
    kFLASH_SwapControlOptionSetInUpdateState = 0x02U,   /*!< An option used to set the Swap in an update state */
    kFLASH_SwapControlOptionSetInCompleteState = 0x04U, /*!< An option used to set the Swap in a complete state */
    kFLASH_SwapControlOptionReportStatus = 0x08U,       /*!< An option used to report the Swap status */
    kFLASH_SwapControlOptionDisableSystem = 0x10U       /*!< An option used to disable the Swap status */
} flash_swap_control_option_t;

/*!
 * @brief Enumeration for the possible flash Swap status.
 */
typedef enum _flash_swap_state
{
    kFLASH_SwapStateUninitialized = 0x00U, /*!< Flash Swap system is in an uninitialized state.*/
    kFLASH_SwapStateReady = 0x01U,         /*!< Flash Swap system is in a ready state.*/
    kFLASH_SwapStateUpdate = 0x02U,        /*!< Flash Swap system is in an update state.*/
    kFLASH_SwapStateUpdateErased = 0x03U,  /*!< Flash Swap system is in an updateErased state.*/
    kFLASH_SwapStateComplete = 0x04U,      /*!< Flash Swap system is in a complete state.*/
    kFLASH_SwapStateDisabled = 0x05U       /*!< Flash Swap system is in a disabled state.*/
} flash_swap_state_t;

/*!
 * @breif Enumeration for the possible flash Swap block status
 */
typedef enum _flash_swap_block_status
{
    kFLASH_SwapBlockStatusLowerHalfProgramBlocksAtZero =
        0x00U, /*!< Swap block status is that lower half program block at zero.*/
    kFLASH_SwapBlockStatusUpperHalfProgramBlocksAtZero =
        0x01U, /*!< Swap block status is that upper half program block at zero.*/
} flash_swap_block_status_t;

/*!
 * @brief Flash Swap information
 */
typedef struct _flash_swap_state_config
{
    flash_swap_state_t flashSwapState;                /*!<The current Swap system status.*/
    flash_swap_block_status_t currentSwapBlockStatus; /*!< The current Swap block status.*/
    flash_swap_block_status_t nextSwapBlockStatus;    /*!< The next Swap block status.*/
} flash_swap_state_config_t;

/*!
 * @brief Flash Swap IFR fields
 */
typedef struct _flash_swap_ifr_field_config
{
    uint16_t swapIndicatorAddress; /*!< A Swap indicator address field.*/
    uint16_t swapEnableWord;       /*!< A Swap enable word field.*/
    uint8_t reserved0[4];          /*!< A reserved field.*/
#if (FSL_FEATURE_FLASH_IS_FTFE == 1)
    uint8_t reserved1[2];     /*!< A reserved field.*/
    uint16_t swapDisableWord; /*!< A Swap disable word field.*/
    uint8_t reserved2[4];     /*!< A reserved field.*/
#endif
} flash_swap_ifr_field_config_t;

/*!
 * @brief Flash Swap IFR field data
 */
typedef union _flash_swap_ifr_field_data
{
    uint32_t flashSwapIfrData[2];                    /*!< A flash Swap IFR field data .*/
    flash_swap_ifr_field_config_t flashSwapIfrField; /*!< A flash Swap IFR field structure.*/
} flash_swap_ifr_field_data_t;

/*!
 * @brief PFlash protection status - low 32bit
 */
typedef union _pflash_protection_status_low
{
    uint32_t protl32b; /*!< PROT[31:0] .*/
    struct
    {
        uint8_t protsl; /*!< PROTS[7:0] .*/
        uint8_t protsh; /*!< PROTS[15:8] .*/
        uint8_t reserved[2];
    } prots16b;
} pflash_protection_status_low_t;

/*!
 * @brief PFlash protection status - full
 */
typedef struct _pflash_protection_status
{
    pflash_protection_status_low_t valueLow32b; /*!< PROT[31:0] or PROTS[15:0].*/
#if ((FSL_FEATURE_FLASH_IS_FTFA == 1) && (defined(FTFA_FPROTH0_PROT_MASK))) || \
    ((FSL_FEATURE_FLASH_IS_FTFE == 1) && (defined(FTFE_FPROTH0_PROT_MASK))) || \
    ((FSL_FEATURE_FLASH_IS_FTFL == 1) && (defined(FTFL_FPROTH0_PROT_MASK)))
    struct
    {
        uint32_t proth32b;
    } valueHigh32b; /*!< PROT[63:32].*/
#endif
} pflash_protection_status_t;

/*!
 * @brief Enumeration for the FlexRAM load during reset option.
 */
typedef enum _flash_partition_flexram_load_option
{
    kFLASH_PartitionFlexramLoadOptionLoadedWithValidEepromData =
        0x00U, /*!< FlexRAM is loaded with valid EEPROM data during reset sequence.*/
    kFLASH_PartitionFlexramLoadOptionNotLoaded = 0x01U /*!< FlexRAM is not loaded during reset sequence.*/
} flash_partition_flexram_load_option_t;

/*!
 * @brief Enumeration for the flash memory index.
 */
typedef enum _flash_memory_index
{
    kFLASH_MemoryIndexPrimaryFlash = 0x00U,   /*!< Current flash memory is primary flash.*/
    kFLASH_MemoryIndexSecondaryFlash = 0x01U, /*!< Current flash memory is secondary flash.*/
} flash_memory_index_t;

/*!
 * @brief Enumeration for the flash cache controller index.
 */
typedef enum _flash_cache_controller_index
{
    kFLASH_CacheControllerIndexForCore0 = 0x00U, /*!< Current flash cache controller is for core 0.*/
    kFLASH_CacheControllerIndexForCore1 = 0x01U, /*!< Current flash cache controller is for core 1.*/
} flash_cache_controller_index_t;

/*!
 * @brief Enumeration for the two possible options of flash prefetch speculation.
 */
typedef enum _flash_prefetch_speculation_option
{
    kFLASH_prefetchSpeculationOptionEnable = 0x00U,
    kFLASH_prefetchSpeculationOptionDisable = 0x01U
} flash_prefetch_speculation_option_t;

/*!
 * @brief Flash prefetch speculation status.
 */
typedef struct _flash_prefetch_speculation_status
{
    flash_prefetch_speculation_option_t instructionOption; /*!< Instruction speculation.*/
    flash_prefetch_speculation_option_t dataOption;        /*!< Data speculation.*/
} flash_prefetch_speculation_status_t;

/*!
 * @brief Flash cache clear process code.
 */
typedef enum _flash_cache_clear_process
{
    kFLASH_CacheClearProcessPre = 0x00U,  /*!< Pre flash cache clear process.*/
    kFLASH_CacheClearProcessPost = 0x01U, /*!< Post flash cache clear process.*/
} flash_cache_clear_process_t;

/*!
 * @brief Active flash protection information for the current operation.
 */
typedef struct _flash_protection_config
{
    uint32_t regionBase;  /*!< Base address of flash protection region.*/
    uint32_t regionSize;  /*!< size of flash protection region.*/
    uint32_t regionCount; /*!< flash protection region count.*/
} flash_protection_config_t;

/*!
 * @brief Active flash Execute-Only access information for the current operation.
 */
typedef struct _flash_access_config
{
    uint32_t SegmentBase;  /*!< Base address of flash Execute-Only segment.*/
    uint32_t SegmentSize;  /*!< size of flash Execute-Only segment.*/
    uint32_t SegmentCount; /*!< flash Execute-Only segment count.*/
} flash_access_config_t;

/*!
 * @brief Active flash information for the current operation.
 */
typedef struct _flash_operation_config
{
    uint32_t convertedAddress;           /*!< A converted address for the current flash type.*/
    uint32_t activeSectorSize;           /*!< A sector size of the current flash type.*/
    uint32_t activeBlockSize;            /*!< A block size of the current flash type.*/
    uint32_t blockWriteUnitSize;         /*!< The write unit size.*/
    uint32_t sectorCmdAddressAligment;   /*!< An erase sector command address alignment.*/
    uint32_t sectionCmdAddressAligment;  /*!< A program/verify section command address alignment.*/
    uint32_t resourceCmdAddressAligment; /*!< A read resource command address alignment.*/
    uint32_t checkCmdAddressAligment;    /*!< A program check command address alignment.*/
} flash_operation_config_t;

/*! @brief Flash driver state information.
 *
 * An instance of this structure is allocated by the user of the flash driver and
 * passed into each of the driver APIs.
 */
typedef struct _flash_config
{
    uint32_t PFlashBlockBase;                /*!< A base address of the first PFlash block */
    uint32_t PFlashTotalSize;                /*!< The size of the combined PFlash block. */
    uint8_t PFlashBlockCount;                /*!< A number of PFlash blocks. */
    uint8_t FlashMemoryIndex;                /*!< 0 - primary flash; 1 - secondary flash*/
    uint8_t Reserved0[2];                    /*!< Reserved field 0 */
    uint32_t PFlashSectorSize;               /*!< The size in bytes of a sector of PFlash. */
    uint32_t Reserved1;                      /*!< Reserved field 1 */
    uint32_t PFlashAccessSegmentSize;        /*!< A size in bytes of an access segment of PFlash. */
    uint32_t PFlashAccessSegmentCount;       /*!< A number of PFlash access segments. */
    uint32_t *flashExecuteInRamFunctionInfo; /*!< An information structure of the flash execute-in-RAM function. */
    uint32_t FlexRAMBlockBase;               /*!< For the FlexNVM device, this is the base address of the FlexRAM */
    /*!< For the non-FlexNVM device, this is the base address of the acceleration RAM memory */
    uint32_t FlexRAMTotalSize; /*!< For the FlexNVM device, this is the size of the FlexRAM */
                               /*!< For the non-FlexNVM device, this is the size of the acceleration RAM memory */
    uint32_t
        DFlashBlockBase; /*!< For the FlexNVM device, this is the base address of the D-Flash memory (FlexNVM memory) */
                         /*!< For the non-FlexNVM device, this field is unused */
    uint32_t DFlashTotalSize; /*!< For the FlexNVM device, this is the total size of the FlexNVM memory; */
                              /*!< For the non-FlexNVM device, this field is unused */
    uint32_t EEpromTotalSize; /*!< For the FlexNVM device, this is the size in bytes of the EEPROM area which was
                                 partitioned from FlexRAM */
    /*!< For the non-FlexNVM device, this field is unused */
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
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_PartitionStatusUpdateFailure Failed to update the partition status.
 */
status_t FLASH_Init(flash_config_t *config);

/*!
 * @brief Prepares flash execute-in-RAM functions.
 *
 * @param config Pointer to the storage for the driver runtime state.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
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
 * @param config Pointer to the storage for the driver runtime state.
 * @param key A value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_EraseKeyError API erase key is invalid.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 * @retval #kStatus_FLASH_PartitionStatusUpdateFailure Failed to update the partition status.
 */
status_t FLASH_EraseAll(flash_config_t *config, uint32_t key);

/*!
 * @brief Erases the flash sectors encompassed by parameters passed into function.
 *
 * This function erases the appropriate number of flash sectors based on the
 * desired start address and length.
 *
 * @param config The pointer to the storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be erased.
 *              The start address does not need to be sector-aligned but must be word-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *                      to be erased. Must be word-aligned.
 * @param key The value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError The parameter is not aligned with the specified baseline.
 * @retval #kStatus_FLASH_AddressError The address is out of range.
 * @retval #kStatus_FLASH_EraseKeyError The API erase key is invalid.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
status_t FLASH_Erase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint32_t key);

/*!
 * @brief Erases the entire flash, including protected sectors.
 *
 * @param config Pointer to the storage for the driver runtime state.
 * @param key A value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_EraseKeyError API erase key is invalid.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 * @retval #kStatus_FLASH_PartitionStatusUpdateFailure Failed to update the partition status.
 */
#if defined(FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD) && FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD
status_t FLASH_EraseAllUnsecure(flash_config_t *config, uint32_t key);
#endif

/*!
 * @brief Erases all program flash execute-only segments defined by the FXACC registers.
 *
 * @param config Pointer to the storage for the driver runtime state.
 * @param key A value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_EraseKeyError API erase key is invalid.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
status_t FLASH_EraseAllExecuteOnlySegments(flash_config_t *config, uint32_t key);

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
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
status_t FLASH_Program(flash_config_t *config, uint32_t start, uint32_t *src, uint32_t lengthInBytes);

/*!
 * @brief Programs Program Once Field through parameters.
 *
 * This function programs the Program Once Field with the desired data for a given
 * flash area as determined by the index and length.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param index The index indicating which area of the Program Once Field to be programmed.
 * @param src A pointer to the source buffer of data that is to be programmed
 *            into the Program Once Field.
 * @param lengthInBytes The length, given in bytes (not words or long-words),
 *                      to be programmed. Must be word-aligned.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
status_t FLASH_ProgramOnce(flash_config_t *config, uint32_t index, uint32_t *src, uint32_t lengthInBytes);

/*!
 * @brief Programs flash with data at locations passed in through parameters via the Program Section command.
 *
 * This function programs the flash memory with the desired data for a given
 * flash area as determined by the start address and length.
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
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 * @retval #kStatus_FLASH_SetFlexramAsRamError Failed to set flexram as RAM.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 * @retval #kStatus_FLASH_RecoverFlexramAsEepromError Failed to recover FlexRAM as EEPROM.
 */
#if defined(FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD) && FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD
status_t FLASH_ProgramSection(flash_config_t *config, uint32_t start, uint32_t *src, uint32_t lengthInBytes);
#endif

/*!
 * @brief Programs the EEPROM with data at locations passed in through parameters.
 *
 * This function programs the emulated EEPROM with the desired data for a given
 * flash area as determined by the start address and length.
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
 * @retval #kStatus_FLASH_AddressError Address is out of range.
 * @retval #kStatus_FLASH_SetFlexramAsEepromError Failed to set flexram as eeprom.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_RecoverFlexramAsRamError Failed to recover the FlexRAM as RAM.
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
 * @brief Reads the resource with data at locations passed in through parameters.
 *
 * This function reads the flash memory with the desired location for a given
 * flash area as determined by the start address and length.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be programmed. Must be
 *              word-aligned.
 * @param dst A pointer to the destination buffer of data that is used to store
 *        data to be read.
 * @param lengthInBytes The length, given in bytes (not words or long-words),
 *        to be read. Must be word-aligned.
 * @param option The resource option which indicates which area should be read back.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with the specified baseline.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
status_t FLASH_ReadResource(
    flash_config_t *config, uint32_t start, uint32_t *dst, uint32_t lengthInBytes, flash_read_resource_option_t option);
#endif

/*!
 * @brief Reads the Program Once Field through parameters.
 *
 * This function reads the read once feild with given index and length.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param index The index indicating the area of program once field to be read.
 * @param dst A pointer to the destination buffer of data that is used to store
 *        data to be read.
 * @param lengthInBytes The length, given in bytes (not words or long-words),
 *        to be programmed. Must be word-aligned.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
status_t FLASH_ReadOnce(flash_config_t *config, uint32_t index, uint32_t *dst, uint32_t lengthInBytes);

/*@}*/

/*!
 * @name Security
 * @{
 */

/*!
 * @brief Returns the security state via the pointer passed into the function.
 *
 * This function retrieves the current flash security status, including the
 * security enabling state and the backdoor key enabling state.
 *
 * @param config A pointer to storage for the driver runtime state.
 * @param state A pointer to the value returned for the current security status code:
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 */
status_t FLASH_GetSecurityState(flash_config_t *config, flash_security_state_t *state);

/*!
 * @brief Allows users to bypass security with a backdoor key.
 *
 * If the MCU is in secured state, this function unsecures the MCU by
 * comparing the provided backdoor key with ones in the flash configuration
 * field.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param backdoorKey A pointer to the user buffer containing the backdoor key.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
status_t FLASH_SecurityBypass(flash_config_t *config, const uint8_t *backdoorKey);

/*@}*/

/*!
 * @name Verification
 * @{
 */

/*!
 * @brief Verifies erasure of the entire flash at a specified margin level.
 *
 * This function checks whether the flash is erased to the
 * specified read margin level.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param margin Read margin choice.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
status_t FLASH_VerifyEraseAll(flash_config_t *config, flash_margin_value_t margin);

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
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
status_t FLASH_VerifyErase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, flash_margin_value_t margin);

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
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
status_t FLASH_VerifyProgram(flash_config_t *config,
                             uint32_t start,
                             uint32_t lengthInBytes,
                             const uint32_t *expectedData,
                             flash_margin_value_t margin,
                             uint32_t *failedAddress,
                             uint32_t *failedData);

/*!
 * @brief Verifies whether the program flash execute-only segments have been erased to
 *  the specified read margin level.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param margin Read margin choice.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
status_t FLASH_VerifyEraseAllExecuteOnlySegments(flash_config_t *config, flash_margin_value_t margin);

/*@}*/

/*!
 * @name Protection
 * @{
 */

/*!
 * @brief Returns the protection state of the desired flash area via the pointer passed into the function.
 *
 * This function retrieves the current flash protect status for a given
 * flash area as determined by the start address and length.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be checked. Must be word-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *        to be checked.  Must be word-aligned.
 * @param protection_state A pointer to the value returned for the current
 *        protection status code for the desired flash area.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_AddressError The address is out of range.
 */
status_t FLASH_IsProtected(flash_config_t *config,
                           uint32_t start,
                           uint32_t lengthInBytes,
                           flash_protection_state_t *protection_state);

/*!
 * @brief Returns the access state of the desired flash area via the pointer passed into the function.
 *
 * This function retrieves the current flash access status for a given
 * flash area as determined by the start address and length.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param start The start address of the desired flash memory to be checked. Must be word-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words),
 *        to be checked.  Must be word-aligned.
 * @param access_state A pointer to the value returned for the current
 *        access status code for the desired flash area.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError The parameter is not aligned to the specified baseline.
 * @retval #kStatus_FLASH_AddressError The address is out of range.
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
 * @retval #kStatus_FLASH_InvalidPropertyValue An invalid property value.
 * @retval #kStatus_FLASH_ReadOnlyProperty An read-only property tag.
 */
status_t FLASH_SetProperty(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t value);

/*@}*/

/*!
 * @name FlexRAM
 * @{
 */

/*!
 * @brief Sets the FlexRAM function command.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param option The option used to set the work mode of FlexRAM.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
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
 * @brief Configures the Swap function or checks the the swap state of the Flash module.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param address Address used to configure the flash Swap function.
 * @param option The possible option used to configure Flash Swap function or check the flash Swap status
 * @param returnInfo A pointer to the data which is used to return the information of flash Swap.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_SwapIndicatorAddressError Swap indicator address is invalid.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during the command execution.
 */
#if defined(FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD) && FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD
status_t FLASH_SwapControl(flash_config_t *config,
                           uint32_t address,
                           flash_swap_control_option_t option,
                           flash_swap_state_config_t *returnInfo);
#endif

/*!
 * @brief Swaps the lower half flash with the higher half flash.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param address Address used to configure the flash swap function
 * @param option The possible option used to configure the Flash Swap function or check the flash Swap status.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FLASH_SwapIndicatorAddressError Swap indicator address is invalid.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FLASH_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FLASH_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 * @retval #kStatus_FLASH_SwapSystemNotInUninitialized Swap system is not in an uninitialzed state.
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
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FLASH_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
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
 * @brief Sets the PFlash Protection to the intended protection status.
 *
 * @param config A pointer to storage for the driver runtime state.
 * @param protectStatus The expected protect status to set to the PFlash protection register. Each bit is
 * corresponding to protection of 1/32(64) of the total PFlash. The least significant bit is corresponding to the lowest
 * address area of PFlash. The most significant bit is corresponding to the highest address area of PFlash. There are
 * two possible cases as shown below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
status_t FLASH_PflashSetProtection(flash_config_t *config, pflash_protection_status_t *protectStatus);

/*!
 * @brief Gets the PFlash protection status.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param protectStatus  Protect status returned by the PFlash IP. Each bit is corresponding to the protection of
 * 1/32(64)
 * of the
 * total PFlash. The least significant bit corresponds to the lowest address area of the PFlash. The most significant
 * bit corresponds to the highest address area of PFlash. There are two possible cases as shown below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 */
status_t FLASH_PflashGetProtection(flash_config_t *config, pflash_protection_status_t *protectStatus);

/*!
 * @brief Sets the DFlash protection to the intended protection status.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param protectStatus The expected protect status to set to the DFlash protection register. Each bit
 * corresponds to the protection of the 1/8 of the total DFlash. The least significant bit corresponds to the lowest
 * address area of the DFlash. The most significant bit corresponds to the highest address area of  the DFlash. There
 * are
 * two possible cases as shown below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_CommandNotSupported Flash API is not supported.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_DflashSetProtection(flash_config_t *config, uint8_t protectStatus);
#endif

/*!
 * @brief Gets the DFlash protection status.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param protectStatus  DFlash Protect status returned by the PFlash IP. Each bit corresponds to the protection of the
 * 1/8 of
 * the total DFlash. The least significant bit corresponds to the lowest address area of the DFlash. The most
 * significant bit corresponds to the highest address area of the DFlash, and so on. There are two possible cases as
 * below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_CommandNotSupported Flash API is not supported.
 */
#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_DflashGetProtection(flash_config_t *config, uint8_t *protectStatus);
#endif

/*!
 * @brief Sets the EEPROM protection to the intended protection status.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param protectStatus The expected protect status to set to the EEPROM protection register. Each bit
 * corresponds to the protection of the 1/8 of the total EEPROM. The least significant bit corresponds to the lowest
 * address area of the EEPROM. The most significant bit corresponds to the highest address area of EEPROM, and so on.
 * There are two possible cases as shown below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_CommandNotSupported Flash API is not supported.
 * @retval #kStatus_FLASH_CommandFailure Run-time error during command execution.
 */
#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_EepromSetProtection(flash_config_t *config, uint8_t protectStatus);
#endif

/*!
 * @brief Gets the DFlash protection status.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param protectStatus  DFlash Protect status returned by the PFlash IP. Each bit corresponds to the protection of the
 * 1/8 of
 * the total EEPROM. The least significant bit corresponds to the lowest address area of the EEPROM. The most
 * significant bit corresponds to the highest address area of the EEPROM. There are two possible cases as below:
 *       0: this area is protected.
 *       1: this area is unprotected.
 *
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FLASH_CommandNotSupported Flash API is not supported.
 */
#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_EepromGetProtection(flash_config_t *config, uint8_t *protectStatus);
#endif

/*@}*/

/*@}*/

/*!
* @name Flash Speculation Utilities
* @{
*/

/*!
 * @brief Sets the PFlash prefetch speculation to the intended speculation status.
 *
 * @param speculationStatus The expected protect status to set to the PFlash protection register. Each bit is
 * @retval #kStatus_FLASH_Success API was executed successfully.
 * @retval #kStatus_FLASH_InvalidSpeculationOption An invalid speculation option argument is provided.
 */
status_t FLASH_PflashSetPrefetchSpeculation(flash_prefetch_speculation_status_t *speculationStatus);

/*!
 * @brief Gets the PFlash prefetch speculation status.
 *
 * @param speculationStatus  Speculation status returned by the PFlash IP.
 * @retval #kStatus_FLASH_Success API was executed successfully.
 */
status_t FLASH_PflashGetPrefetchSpeculation(flash_prefetch_speculation_status_t *speculationStatus);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_FLASH_H_ */
