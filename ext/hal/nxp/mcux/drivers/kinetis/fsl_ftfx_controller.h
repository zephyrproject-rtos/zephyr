/*
* Copyright 2013-2016 Freescale Semiconductor, Inc.
* Copyright 2016-2018 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*
*/

#ifndef _FSL_FTFX_CONTROLLER_H_
#define _FSL_FTFX_CONTROLLER_H_

#include "fsl_ftfx_features.h"
#include "fsl_ftfx_utilities.h"

/*!
 * @addtogroup ftfx_controller
 * @{
 */
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.flash"
#endif

/*!
 * @name FTFx status
 * @{
 */
/*! @brief FTFx driver status group. */
#if defined(kStatusGroup_FlashDriver)
#define kStatusGroupGeneric kStatusGroup_Generic
#define kStatusGroupFtfxDriver kStatusGroup_FlashDriver
#elif defined(kStatusGroup_FLASH)
#define kStatusGroupGeneric kStatusGroup_Generic
#define kStatusGroupFtfxDriver kStatusGroup_FLASH
#else
#define kStatusGroupGeneric 0
#define kStatusGroupFtfxDriver 1
#endif

/*!
 * @brief FTFx driver status codes.
 */
enum _ftfx_status
{
    kStatus_FTFx_Success = MAKE_STATUS(kStatusGroupGeneric, 0),         /*!< API is executed successfully*/
    kStatus_FTFx_InvalidArgument = MAKE_STATUS(kStatusGroupGeneric, 4), /*!< Invalid argument*/
    kStatus_FTFx_SizeError = MAKE_STATUS(kStatusGroupFtfxDriver, 0),   /*!< Error size*/
    kStatus_FTFx_AlignmentError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 1), /*!< Parameter is not aligned with the specified baseline*/
    kStatus_FTFx_AddressError = MAKE_STATUS(kStatusGroupFtfxDriver, 2), /*!< Address is out of range */
    kStatus_FTFx_AccessError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 3), /*!< Invalid instruction codes and out-of bound addresses */
    kStatus_FTFx_ProtectionViolation = MAKE_STATUS(
        kStatusGroupFtfxDriver, 4), /*!< The program/erase operation is requested to execute on protected areas */
    kStatus_FTFx_CommandFailure =
        MAKE_STATUS(kStatusGroupFtfxDriver, 5), /*!< Run-time error during command execution. */
    kStatus_FTFx_UnknownProperty = MAKE_STATUS(kStatusGroupFtfxDriver, 6), /*!< Unknown property.*/
    kStatus_FTFx_EraseKeyError = MAKE_STATUS(kStatusGroupFtfxDriver, 7),   /*!< API erase key is invalid.*/
    kStatus_FTFx_RegionExecuteOnly =
        MAKE_STATUS(kStatusGroupFtfxDriver, 8), /*!< The current region is execute-only.*/
    kStatus_FTFx_ExecuteInRamFunctionNotReady =
        MAKE_STATUS(kStatusGroupFtfxDriver, 9), /*!< Execute-in-RAM function is not available.*/
    kStatus_FTFx_PartitionStatusUpdateFailure =
        MAKE_STATUS(kStatusGroupFtfxDriver, 10), /*!< Failed to update partition status.*/
    kStatus_FTFx_SetFlexramAsEepromError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 11), /*!< Failed to set FlexRAM as EEPROM.*/
    kStatus_FTFx_RecoverFlexramAsRamError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 12), /*!< Failed to recover FlexRAM as RAM.*/
    kStatus_FTFx_SetFlexramAsRamError = MAKE_STATUS(kStatusGroupFtfxDriver, 13), /*!< Failed to set FlexRAM as RAM.*/
    kStatus_FTFx_RecoverFlexramAsEepromError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 14), /*!< Failed to recover FlexRAM as EEPROM.*/
    kStatus_FTFx_CommandNotSupported = MAKE_STATUS(kStatusGroupFtfxDriver, 15), /*!< Flash API is not supported.*/
    kStatus_FTFx_SwapSystemNotInUninitialized =
        MAKE_STATUS(kStatusGroupFtfxDriver, 16), /*!< Swap system is not in an uninitialzed state.*/
    kStatus_FTFx_SwapIndicatorAddressError =
        MAKE_STATUS(kStatusGroupFtfxDriver, 17), /*!< The swap indicator address is invalid.*/
    kStatus_FTFx_ReadOnlyProperty = MAKE_STATUS(kStatusGroupFtfxDriver, 18), /*!< The flash property is read-only.*/
    kStatus_FTFx_InvalidPropertyValue =
        MAKE_STATUS(kStatusGroupFtfxDriver, 19), /*!< The flash property value is out of range.*/
    kStatus_FTFx_InvalidSpeculationOption =
        MAKE_STATUS(kStatusGroupFtfxDriver, 20), /*!< The option of flash prefetch speculation is invalid.*/
};
/*@}*/

/*!
 * @name FTFx API key
 * @{
 */
/*!
 * @brief Enumeration for FTFx driver API keys.
 *
 * @note The resulting value is built with a byte order such that the string
 * being readable in expected order when viewed in a hex editor, if the value
 * is treated as a 32-bit little endian value.
 */
enum _ftfx_driver_api_keys
{
    kFTFx_ApiEraseKey = FOUR_CHAR_CODE('k', 'f', 'e', 'k') /*!< Key value used to validate all FTFx erase APIs.*/
};
/*@}*/

/*!
 * @brief Enumeration for the FlexRAM load during reset option.
 */
typedef enum _ftfx_partition_flexram_load_option
{
    kFTFx_PartitionFlexramLoadOptLoadedWithValidEepromData =
        0x00U, /*!< FlexRAM is loaded with valid EEPROM data during reset sequence.*/
    kFTFx_PartitionFlexramLoadOptNotLoaded = 0x01U /*!< FlexRAM is not loaded during reset sequence.*/
} ftfx_partition_flexram_load_opt_t;

/*!
 * @brief Enumeration for the two possible options of flash read resource command.
 */
typedef enum _ftfx_read_resource_opt
{
    kFTFx_ResourceOptionFlashIfr =
        0x00U, /*!< Select code for Program flash 0 IFR, Program flash swap 0 IFR, Data flash 0 IFR */
    kFTFx_ResourceOptionVersionId = 0x01U /*!< Select code for the version ID*/
} ftfx_read_resource_opt_t;

/*!
 * @brief Enumeration for supported FTFx margin levels.
 */
typedef enum _ftfx_margin_value
{
    kFTFx_MarginValueNormal,  /*!< Use the 'normal' read level for 1s.*/
    kFTFx_MarginValueUser,    /*!< Apply the 'User' margin to the normal read-1 level.*/
    kFTFx_MarginValueFactory, /*!< Apply the 'Factory' margin to the normal read-1 level.*/
    kFTFx_MarginValueInvalid  /*!< Not real margin level, Used to determine the range of valid margin level. */
} ftfx_margin_value_t;

/*!
 * @brief Enumeration for the three possible FTFx security states.
 */
typedef enum _ftfx_security_state
{
    kFTFx_SecurityStateNotSecure = (int)0xc33cc33c,       /*!< Flash is not secure.*/
    kFTFx_SecurityStateBackdoorEnabled = (int)0x5aa55aa5, /*!< Flash backdoor is enabled.*/
    kFTFx_SecurityStateBackdoorDisabled = (int)0x5ac33ca5 /*!< Flash backdoor is disabled.*/
} ftfx_security_state_t;

/*!
 * @brief Enumeration for the two possilbe options of set FlexRAM function command.
 */
typedef enum _ftfx_flexram_function_option
{
    kFTFx_FlexramFuncOptAvailableAsRam = 0xFFU,    /*!< An option used to make FlexRAM available as RAM */
    kFTFx_FlexramFuncOptAvailableForEeprom = 0x00U /*!< An option used to make FlexRAM available for EEPROM */
} ftfx_flexram_func_opt_t;

/*!
 * @brief Enumeration for the possible options of Swap control commands
 */
typedef enum _ftfx_swap_control_option
{
    kFTFx_SwapControlOptionIntializeSystem = 0x01U,    /*!< An option used to initialize the Swap system */
    kFTFx_SwapControlOptionSetInUpdateState = 0x02U,   /*!< An option used to set the Swap in an update state */
    kFTFx_SwapControlOptionSetInCompleteState = 0x04U, /*!< An option used to set the Swap in a complete state */
    kFTFx_SwapControlOptionReportStatus = 0x08U,       /*!< An option used to report the Swap status */
    kFTFx_SwapControlOptionDisableSystem = 0x10U       /*!< An option used to disable the Swap status */
} ftfx_swap_control_opt_t;

/*!
 * @brief Enumeration for the possible flash Swap status.
 */
typedef enum _ftfx_swap_state
{
    kFTFx_SwapStateUninitialized = 0x00U, /*!< Flash Swap system is in an uninitialized state.*/
    kFTFx_SwapStateReady = 0x01U,         /*!< Flash Swap system is in a ready state.*/
    kFTFx_SwapStateUpdate = 0x02U,        /*!< Flash Swap system is in an update state.*/
    kFTFx_SwapStateUpdateErased = 0x03U,  /*!< Flash Swap system is in an updateErased state.*/
    kFTFx_SwapStateComplete = 0x04U,      /*!< Flash Swap system is in a complete state.*/
    kFTFx_SwapStateDisabled = 0x05U       /*!< Flash Swap system is in a disabled state.*/
} ftfx_swap_state_t;

/*!
 * @breif Enumeration for the possible flash Swap block status
 */
typedef enum _ftfx_swap_block_status
{
    kFTFx_SwapBlockStatusLowerHalfProgramBlocksAtZero =
        0x00U, /*!< Swap block status is that lower half program block at zero.*/
    kFTFx_SwapBlockStatusUpperHalfProgramBlocksAtZero =
        0x01U, /*!< Swap block status is that upper half program block at zero.*/
} ftfx_swap_block_status_t;

/*!
 * @brief Flash Swap information
 */
typedef struct _ftfx_swap_state_config
{
    ftfx_swap_state_t flashSwapState;                /*!<The current Swap system status.*/
    ftfx_swap_block_status_t currentSwapBlockStatus; /*!< The current Swap block status.*/
    ftfx_swap_block_status_t nextSwapBlockStatus;    /*!< The next Swap block status.*/
} ftfx_swap_state_config_t;

/*!
 * @brief Enumeration for FTFx memory type.
 */
enum _ftfx_memory_type
{
    kFTFx_MemTypePflash = 0x00U,
    kFTFx_MemTypeFlexnvm = 0x01U
} ;

/*!
 * @brief ftfx special memory access information.
 */
typedef struct _ftfx_special_mem
{
    uint32_t base;  /*!< Base address of flash special memory.*/
    uint32_t size;  /*!< size of flash special memory.*/
    uint32_t count; /*!< flash special memory count.*/
} ftfx_spec_mem_t;

/*!
 * @brief Flash memory descriptor.
 */
typedef struct _ftfx_mem_descriptor
{
    uint8_t type;                     /*!< Type of flash block.*/
    uint8_t index;                    /*!< Index of flash block.*/
    uint8_t reserved[2];
    struct {
        uint32_t isIndBlock:1;
        uint32_t hasIndPfsizeReg:1;
        uint32_t hasProtControl:1;
        uint32_t hasIndProtReg:1;
        uint32_t hasXaccControl:1;
        uint32_t hasIndXaccReg:1;
        uint32_t :18;
        uint32_t ProtRegBits:8;
    } feature;
    uint32_t blockBase;                /*!< A base address of the flash block */
    uint32_t totalSize;                /*!< The size of the flash block. */
    uint32_t sectorSize;               /*!< The size in bytes of a sector of flash. */
    uint32_t blockCount;               /*!< A number of flash blocks. */
    ftfx_spec_mem_t accessSegmentMem;
    ftfx_spec_mem_t protectRegionMem;
} ftfx_mem_desc_t;

/*!
 * @brief Active FTFx information for the current operation.
 */
typedef struct _ftfx_ops_config
{
    uint32_t convertedAddress;          /*!< A converted address for the current flash type.*/
    struct {
        uint8_t sectorCmd;
        uint8_t sectionCmd;
        uint8_t resourceCmd;
        uint8_t checkCmd;
        uint8_t swapCtrlCmd;
        uint8_t blockWriteUnitSize;
        uint8_t reserved[2];
    } addrAligment;
} ftfx_ops_config_t;

/*!
 * @brief Flash IFR memory descriptor.
 */
typedef struct _ftfx_ifr_descriptor
{
    struct {
        uint32_t has4ByteIdxSupport:1;
        uint32_t has8ByteIdxSupport:1;
        uint32_t :30;
    } feature;
    struct {
        uint8_t versionIdStart;
        uint8_t versionIdSize;
        uint16_t ifrMemSize;
        uint32_t pflashIfrStart;
        uint32_t dflashIfrStart;
        uint32_t pflashSwapIfrStart;
    } resRange;
    struct {
        uint16_t mix8byteIdxStart;
        uint16_t mix8byteIdxEnd;
    } idxInfo;
} ftfx_ifr_desc_t;

/*! @brief Flash driver state information.
 *
 * An instance of this structure is allocated by the user of the flash driver and
 * passed into each of the driver APIs.
 */
typedef struct _ftfx_config
{
    ftfx_mem_desc_t flashDesc;
    ftfx_ops_config_t opsConfig;
    uint32_t flexramBlockBase;              /*!< The base address of the FlexRAM/acceleration RAM */
    uint32_t flexramTotalSize;              /*!< The size of the FlexRAM/acceleration RAM */
    uint16_t eepromTotalSize;               /*!< The size of EEPROM area which was partitioned from FlexRAM */
    uint16_t reserved;
    uint32_t *runCmdFuncAddr;               /*!< An buffer point to the flash execute-in-RAM function. */
    ftfx_ifr_desc_t ifrDesc;
} ftfx_config_t;

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 */
status_t FTFx_API_Init(ftfx_config_t *config);

/*!
 * @brief Updates FlexNVM memory partition status according to data flash 0 IFR.
 *
 * This function updates FlexNVM memory partition status.
 *
 * @param config Pointer to the storage for the driver runtime state.
 *
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_PartitionStatusUpdateFailure Failed to update the partition status.
 */
status_t FTFx_API_UpdateFlexnvmPartitionStatus(ftfx_config_t *config);

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
 *              The start address does not need to be sector-aligned but must be word-aligned.
 * @param lengthInBytes The length, given in bytes (not words or long-words)
 *                      to be erased. Must be word-aligned.
 * @param key The value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_AlignmentError The parameter is not aligned with the specified baseline.
 * @retval #kStatus_FTFx_AddressError The address is out of range.
 * @retval #kStatus_FTFx_EraseKeyError The API erase key is invalid.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
status_t FTFx_CMD_Erase(ftfx_config_t *config,
                        uint32_t start,
                        uint32_t lengthInBytes,
                        uint32_t key);

/*!
 * @brief Erases entire flash
 *
 * @param config Pointer to the storage for the driver runtime state.
 * @param key A value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_EraseKeyError API erase key is invalid.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during command execution.
 * @retval #kStatus_FTFx_PartitionStatusUpdateFailure Failed to update the partition status.
 */
status_t FTFx_CMD_EraseAll(ftfx_config_t *config, uint32_t key);

/*!
 * @brief Erases the entire flash, including protected sectors.
 *
 * @param config Pointer to the storage for the driver runtime state.
 * @param key A value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_EraseKeyError API erase key is invalid.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during command execution.
 * @retval #kStatus_FTFx_PartitionStatusUpdateFailure Failed to update the partition status.
 */
#if defined(FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD) && FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD
status_t FTFx_CMD_EraseAllUnsecure(ftfx_config_t *config, uint32_t key);
#endif

/*!
 * @brief Erases all program flash execute-only segments defined by the FXACC registers.
 *
 * @param config Pointer to the storage for the driver runtime state.
 * @param key A value used to validate all flash erase APIs.
 *
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_EraseKeyError API erase key is invalid.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
status_t FTFx_CMD_EraseAllExecuteOnlySegments(ftfx_config_t *config, uint32_t key);

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_AlignmentError Parameter is not aligned with the specified baseline.
 * @retval #kStatus_FTFx_AddressError Address is out of range.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
status_t FTFx_CMD_Program(ftfx_config_t *config,
                          uint32_t start,
                          uint8_t *src,
                          uint32_t lengthInBytes);

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
status_t FTFx_CMD_ProgramOnce(ftfx_config_t *config, uint32_t index, uint8_t *src, uint32_t lengthInBytes);

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FTFx_AddressError Address is out of range.
 * @retval #kStatus_FTFx_SetFlexramAsRamError Failed to set flexram as RAM.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during command execution.
 * @retval #kStatus_FTFx_RecoverFlexramAsEepromError Failed to recover FlexRAM as EEPROM.
 */
#if defined(FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD) && FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD
status_t FTFx_CMD_ProgramSection(ftfx_config_t *config,
                                 uint32_t start,
                                 uint8_t *src,
                                 uint32_t lengthInBytes);
#endif

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument Invalid argument is provided.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during command execution.
 */
#if defined(FSL_FEATURE_FLASH_HAS_PROGRAM_PARTITION_CMD) && FSL_FEATURE_FLASH_HAS_PROGRAM_PARTITION_CMD
status_t FTFx_CMD_ProgramPartition(ftfx_config_t *config,
                                   ftfx_partition_flexram_load_opt_t option,
                                   uint32_t eepromDataSizeCode,
                                   uint32_t flexnvmPartitionCode);
#endif

/*@}*/

/*!
 * @name Reading
 * @{
 */

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
status_t FTFx_CMD_ReadOnce(ftfx_config_t *config, uint32_t index, uint8_t *dst, uint32_t lengthInBytes);

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_AlignmentError Parameter is not aligned with the specified baseline.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
status_t FTFx_CMD_ReadResource(ftfx_config_t *config,
                               uint32_t start,
                               uint8_t *dst,
                               uint32_t lengthInBytes,
                               ftfx_read_resource_opt_t option);
#endif

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FTFx_AddressError Address is out of range.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
status_t FTFx_CMD_VerifyErase(ftfx_config_t *config,
                              uint32_t start,
                              uint32_t lengthInBytes,
                              ftfx_margin_value_t margin);

/*!
 * @brief Verifies erasure of the entire flash at a specified margin level.
 *
 * This function checks whether the flash is erased to the
 * specified read margin level.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param margin Read margin choice.
 *
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
status_t FTFx_CMD_VerifyEraseAll(ftfx_config_t *config, ftfx_margin_value_t margin);

/*!
 * @brief Verifies whether the program flash execute-only segments have been erased to
 *  the specified read margin level.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param margin Read margin choice.
 *
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
status_t FTFx_CMD_VerifyEraseAllExecuteOnlySegments(ftfx_config_t *config, ftfx_margin_value_t margin);

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FTFx_AddressError Address is out of range.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
status_t FTFx_CMD_VerifyProgram(ftfx_config_t *config,
                                uint32_t start,
                                uint32_t lengthInBytes,
                                const uint8_t *expectedData,
                                ftfx_margin_value_t margin,
                                uint32_t *failedAddress,
                                uint32_t *failedData);

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 */
status_t FTFx_REG_GetSecurityState(ftfx_config_t *config, ftfx_security_state_t *state);

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
status_t FTFx_CMD_SecurityBypass(ftfx_config_t *config, const uint8_t *backdoorKey);

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
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
status_t FTFx_CMD_SetFlexramFunction(ftfx_config_t *config, ftfx_flexram_func_opt_t option);
#endif

/*@}*/

/*!
 * @name Swap
 * @{
 */

/*!
 * @brief Configures the Swap function or checks the swap state of the Flash module.
 *
 * @param config A pointer to the storage for the driver runtime state.
 * @param address Address used to configure the flash Swap function.
 * @param option The possible option used to configure Flash Swap function or check the flash Swap status
 * @param returnInfo A pointer to the data which is used to return the information of flash Swap.
 *
 * @retval #kStatus_FTFx_Success API was executed successfully.
 * @retval #kStatus_FTFx_InvalidArgument An invalid argument is provided.
 * @retval #kStatus_FTFx_AlignmentError Parameter is not aligned with specified baseline.
 * @retval #kStatus_FTFx_SwapIndicatorAddressError Swap indicator address is invalid.
 * @retval #kStatus_FTFx_ExecuteInRamFunctionNotReady Execute-in-RAM function is not available.
 * @retval #kStatus_FTFx_AccessError Invalid instruction codes and out-of bounds addresses.
 * @retval #kStatus_FTFx_ProtectionViolation The program/erase operation is requested to execute on protected areas.
 * @retval #kStatus_FTFx_CommandFailure Run-time error during the command execution.
 */
#if defined(FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD) && FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD
status_t FTFx_CMD_SwapControl(ftfx_config_t *config,
                              uint32_t address,
                              ftfx_swap_control_opt_t option,
                              ftfx_swap_state_config_t *returnInfo);
#endif

/*@}*/

#if defined(__cplusplus)
}
#endif


#endif /* _FSL_FTFX_CONTROLLER_H_ */

