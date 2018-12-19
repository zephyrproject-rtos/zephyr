/*
* Copyright 2013-2016 Freescale Semiconductor, Inc.
* Copyright 2016-2018 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*
*/

#include "fsl_ftfx_flash.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief Enumeration for special memory property.
 */
enum _ftfx_special_mem_property
{
    kFTFx_AccessSegmentUnitSize = 256UL,
    kFTFx_MinProtectBlockSize = 1024UL,
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


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void flash_init_features(ftfx_config_t *config);

static uint32_t flash_calculate_mem_size(uint32_t pflashBlockCount,
                                         uint32_t pflashBlockSize,
                                         uint32_t pfsizeMask,
                                         uint32_t pfsizeShift);

static uint32_t flash_calculate_prot_segment_size(uint32_t flashSize, uint32_t segmentCount);

static status_t flash_check_range_to_get_index(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint8_t *flashIndex);

/*! @brief Convert address for flash.*/
static status_t flash_convert_start_address(ftfx_config_t *config, uint32_t start);

#if defined(FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP) && FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP
/*! @brief Validates the gived address to see if it is equal to swap indicator address in pflash swap IFR.*/
static status_t flash_validate_swap_indicator_address(ftfx_config_t *config, uint32_t address);
#endif /* FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP */

/*******************************************************************************
 * Variables
 ******************************************************************************/

static volatile uint32_t *const kFPROTL = (volatile uint32_t *)&FTFx_FPROT_LOW_REG;
static volatile uint32_t *const kFPROTH = (volatile uint32_t *)&FTFx_FPROT_HIGH_REG;
#if FTFx_DRIVER_HAS_FLASH1_SUPPORT
volatile uint8_t *const kFPROTSL = (volatile uint8_t *)&FTFx_FPROTSL_REG;
volatile uint8_t *const kFPROTSH = (volatile uint8_t *)&FTFx_FPROTSH_REG;
#endif

/*!
 * @brief Table of pflash sizes.
 *
 *  The index into this table is the value of the SIM_FCFG1.PFSIZE bitfield.
 *
 *  The values in this table have been right shifted 10 bits so that they will all fit within
 *  an 16-bit integer. To get the actual flash density, you must left shift the looked up value
 *  by 10 bits.
 *
 *  Elements of this table have a value of 0 in cases where the PFSIZE bitfield value is
 *  reserved.
 *
 *  Code to use the table:
 *  @code
 *      uint8_t pfsize = (SIM->FCFG1 & SIM_FCFG1_PFSIZE_MASK) >> SIM_FCFG1_PFSIZE_SHIFT;
 *      flashDensity = ((uint32_t)kPFlashDensities[pfsize]) << 10;
 *  @endcode
 */
#if defined(FSL_FEATURE_FLASH_SIZE_ENCODING_RULE_VERSION) && (FSL_FEATURE_FLASH_SIZE_ENCODING_RULE_VERSION == 1)
static const uint16_t kPFlashDensities[] = {
    0,    /* 0x0 - undefined */
    0,    /* 0x1 - undefined */
    0,    /* 0x2 - undefined */
    0,    /* 0x3 - undefined */
    0,    /* 0x4 - undefined */
    0,    /* 0x5 - undefined */
    0,    /* 0x6 - undefined */
    0,    /* 0x7 - undefined */
    0,    /* 0x8 - undefined */
    0,    /* 0x9 - undefined */
    256,  /* 0xa - 262144, 256KB */
    0,    /* 0xb - undefined */
    1024, /* 0xc - 1048576, 1MB */
    0,    /* 0xd - undefined */
    0,    /* 0xe - undefined */
    0,    /* 0xf - undefined */
};
#else
static const uint16_t kPFlashDensities[] = {
    8,    /* 0x0 - 8192, 8KB */
    16,   /* 0x1 - 16384, 16KB */
    24,   /* 0x2 - 24576, 24KB */
    32,   /* 0x3 - 32768, 32KB */
    48,   /* 0x4 - 49152, 48KB */
    64,   /* 0x5 - 65536, 64KB */
    96,   /* 0x6 - 98304, 96KB */
    128,  /* 0x7 - 131072, 128KB */
    192,  /* 0x8 - 196608, 192KB */
    256,  /* 0x9 - 262144, 256KB */
    384,  /* 0xa - 393216, 384KB */
    512,  /* 0xb - 524288, 512KB */
    768,  /* 0xc - 786432, 768KB */
    1024, /* 0xd - 1048576, 1MB */
    1536, /* 0xe - 1572864, 1.5MB */
    /* 2048,  0xf - 2097152, 2MB */
};
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/

status_t FLASH_Init(flash_config_t *config)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    for (uint8_t flashIndex = 0; flashIndex < FTFx_FLASH_COUNT; flashIndex++)
    {
        uint32_t pflashStartAddress;
        uint32_t pflashBlockSize;
        uint32_t pflashBlockCount;
        uint32_t pflashBlockSectorSize;
        uint32_t pflashProtectionRegionCount;
        uint32_t pflashBlockWriteUnitSize;
        uint32_t pflashSectorCmdAlignment;
        uint32_t pflashSectionCmdAlignment;
        uint32_t pfsizeMask;
        uint32_t pfsizeShift;
        uint32_t facssValue;
        uint32_t facsnValue;

        config->ftfxConfig[flashIndex].flashDesc.type = kFTFx_MemTypePflash;
        config->ftfxConfig[flashIndex].flashDesc.index = flashIndex;
        flash_init_features(&config->ftfxConfig[flashIndex]);

#if FTFx_DRIVER_HAS_FLASH1_SUPPORT
        if(flashIndex == 1)
        {
            pflashStartAddress = FLASH1_FEATURE_PFLASH_START_ADDRESS;
            pflashBlockSize = FLASH1_FEATURE_PFLASH_BLOCK_SIZE;
            pflashBlockCount = FLASH1_FEATURE_PFLASH_BLOCK_COUNT;
            pflashBlockSectorSize = FLASH1_FEATURE_PFLASH_BLOCK_SECTOR_SIZE;
            pflashProtectionRegionCount = FLASH1_FEATURE_PFLASH_PROTECTION_REGION_COUNT;
            pflashBlockWriteUnitSize = FLASH1_FEATURE_PFLASH_BLOCK_WRITE_UNIT_SIZE;
            pflashSectorCmdAlignment = FLASH1_FEATURE_PFLASH_SECTOR_CMD_ADDRESS_ALIGMENT;
            pflashSectionCmdAlignment = FLASH1_FEATURE_PFLASH_SECTION_CMD_ADDRESS_ALIGMENT;
            pfsizeMask = SIM_FLASH1_PFSIZE_MASK;
            pfsizeShift = SIM_FLASH1_PFSIZE_SHIFT;
            facssValue = FTFx_FACSSS_REG;
            facsnValue = FTFx_FACSNS_REG;
        }
        else
#endif
        {
            pflashStartAddress = FLASH0_FEATURE_PFLASH_START_ADDRESS;
            pflashBlockSize = FLASH0_FEATURE_PFLASH_BLOCK_SIZE;
            pflashBlockCount = FLASH0_FEATURE_PFLASH_BLOCK_COUNT;
            pflashBlockSectorSize = FLASH0_FEATURE_PFLASH_BLOCK_SECTOR_SIZE;
            pflashProtectionRegionCount = FLASH0_FEATURE_PFLASH_PROTECTION_REGION_COUNT;
            pflashBlockWriteUnitSize = FLASH0_FEATURE_PFLASH_BLOCK_WRITE_UNIT_SIZE;
            pflashSectorCmdAlignment = FLASH0_FEATURE_PFLASH_SECTOR_CMD_ADDRESS_ALIGMENT;
            pflashSectionCmdAlignment = FLASH0_FEATURE_PFLASH_SECTION_CMD_ADDRESS_ALIGMENT;
            pfsizeMask = SIM_FLASH0_PFSIZE_MASK;
            pfsizeShift = SIM_FLASH0_PFSIZE_SHIFT;
            facssValue = FTFx_FACSS_REG;
            facsnValue = FTFx_FACSN_REG;
        }

        config->ftfxConfig[flashIndex].flashDesc.blockBase = pflashStartAddress;
        config->ftfxConfig[flashIndex].flashDesc.blockCount = pflashBlockCount;
        config->ftfxConfig[flashIndex].flashDesc.sectorSize = pflashBlockSectorSize;

        if (config->ftfxConfig[flashIndex].flashDesc.feature.isIndBlock &&
            config->ftfxConfig[flashIndex].flashDesc.feature.hasIndPfsizeReg)
        {
            config->ftfxConfig[flashIndex].flashDesc.totalSize = flash_calculate_mem_size(pflashBlockCount, pflashBlockSize, pfsizeMask, pfsizeShift);
        }
        else
        {
            config->ftfxConfig[flashIndex].flashDesc.totalSize = pflashBlockCount * pflashBlockSize;
        }

        if (config->ftfxConfig[flashIndex].flashDesc.feature.hasXaccControl)
        {
            ftfx_spec_mem_t *specMem;
            specMem = &config->ftfxConfig[flashIndex].flashDesc.accessSegmentMem;
            if (config->ftfxConfig[flashIndex].flashDesc.feature.hasIndXaccReg)
            {
                specMem->base = config->ftfxConfig[flashIndex].flashDesc.blockBase;
                specMem->size = kFTFx_AccessSegmentUnitSize << facssValue;
                specMem->count = facsnValue;
            }
            else
            {
                specMem->base = config->ftfxConfig[0].flashDesc.blockBase;
                specMem->size = kFTFx_AccessSegmentUnitSize << FTFx_FACSS_REG;
                specMem->count = FTFx_FACSN_REG;
            }
        }

        if (config->ftfxConfig[flashIndex].flashDesc.feature.hasProtControl)
        {
            ftfx_spec_mem_t *specMem;
            specMem = &config->ftfxConfig[flashIndex].flashDesc.protectRegionMem;
            if (config->ftfxConfig[flashIndex].flashDesc.feature.hasIndProtReg)
            {
                specMem->base = config->ftfxConfig[flashIndex].flashDesc.blockBase;
                specMem->count = pflashProtectionRegionCount;
                specMem->size = flash_calculate_prot_segment_size(config->ftfxConfig[flashIndex].flashDesc.totalSize, specMem->count);
            }
            else
            {
                uint32_t pflashTotalSize = 0;
                specMem->base = config->ftfxConfig[0].flashDesc.blockBase;
                specMem->count = FLASH0_FEATURE_PFLASH_PROTECTION_REGION_COUNT;
#if (FTFx_FLASH_COUNT != 1)
                if (flashIndex == FTFx_FLASH_COUNT - 1)
#endif
                {
                    uint32_t segmentSize;
                    for (uint32_t i = 0; i < FTFx_FLASH_COUNT; i++)
                    {
                         pflashTotalSize += config->ftfxConfig[flashIndex].flashDesc.totalSize;
                    }
                    segmentSize = flash_calculate_prot_segment_size(pflashTotalSize, specMem->count);
                    for (uint32_t i = 0; i < FTFx_FLASH_COUNT; i++)
                    {
                         config->ftfxConfig[i].flashDesc.protectRegionMem.size = segmentSize;
                    }
                }
            }
        }

        config->ftfxConfig[flashIndex].opsConfig.addrAligment.blockWriteUnitSize = pflashBlockWriteUnitSize;
        config->ftfxConfig[flashIndex].opsConfig.addrAligment.sectorCmd = pflashSectorCmdAlignment;
        config->ftfxConfig[flashIndex].opsConfig.addrAligment.sectionCmd = pflashSectionCmdAlignment;
        config->ftfxConfig[flashIndex].opsConfig.addrAligment.resourceCmd = FSL_FEATURE_FLASH_PFLASH_RESOURCE_CMD_ADDRESS_ALIGMENT;
        config->ftfxConfig[flashIndex].opsConfig.addrAligment.checkCmd = FSL_FEATURE_FLASH_PFLASH_CHECK_CMD_ADDRESS_ALIGMENT;
        config->ftfxConfig[flashIndex].opsConfig.addrAligment.swapCtrlCmd = FSL_FEATURE_FLASH_PFLASH_SWAP_CONTROL_CMD_ADDRESS_ALIGMENT;

        /* Init FTFx Kernel */
        returnCode = FTFx_API_Init(&config->ftfxConfig[flashIndex]);
        if (returnCode != kStatus_FTFx_Success)
        {
            return returnCode;
        }
    }

    return kStatus_FTFx_Success;
}

status_t FLASH_Erase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint32_t key)
{
    status_t returnCode;
    uint8_t flashIndex;

    returnCode = flash_check_range_to_get_index(config, start, lengthInBytes, &flashIndex);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    returnCode = flash_convert_start_address(&config->ftfxConfig[flashIndex], start);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    return FTFx_CMD_Erase(&config->ftfxConfig[flashIndex], start, lengthInBytes, key);
}

status_t FLASH_EraseAll(flash_config_t *config, uint32_t key)
{
    return FTFx_CMD_EraseAll(&config->ftfxConfig[0], key);
}

#if defined(FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD) && FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD
status_t FLASH_EraseAllUnsecure(flash_config_t *config, uint32_t key)
{
    return FTFx_CMD_EraseAllUnsecure(&config->ftfxConfig[0], key);
}
#endif

status_t FLASH_Program(flash_config_t *config, uint32_t start, uint8_t *src, uint32_t lengthInBytes)
{
    status_t returnCode;
    uint8_t flashIndex;

    returnCode = flash_check_range_to_get_index(config, start, lengthInBytes, &flashIndex);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    returnCode = flash_convert_start_address(&config->ftfxConfig[flashIndex], start);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    return FTFx_CMD_Program(&config->ftfxConfig[flashIndex], start, src, lengthInBytes);
}

#if defined(FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD) && FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD
status_t FLASH_ProgramSection(flash_config_t *config, uint32_t start, uint8_t *src, uint32_t lengthInBytes)
{
    status_t returnCode;
    uint8_t flashIndex;

    returnCode = flash_check_range_to_get_index(config, start, lengthInBytes, &flashIndex);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    returnCode = flash_convert_start_address(&config->ftfxConfig[flashIndex], start);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    return FTFx_CMD_ProgramSection(&config->ftfxConfig[flashIndex], start, src, lengthInBytes);
}
#endif

#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
status_t FLASH_ReadResource(flash_config_t *config,
                            uint32_t start,
                            uint8_t *dst,
                            uint32_t lengthInBytes,
                            ftfx_read_resource_opt_t option)
{
    return FTFx_CMD_ReadResource(&config->ftfxConfig[0], start, dst, lengthInBytes, option);
}
#endif

status_t FLASH_VerifyErase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, ftfx_margin_value_t margin)
{
    status_t returnCode;
    uint8_t flashIndex;

    returnCode = flash_check_range_to_get_index(config, start, lengthInBytes, &flashIndex);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    returnCode = flash_convert_start_address(&config->ftfxConfig[flashIndex], start);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    return FTFx_CMD_VerifyErase(&config->ftfxConfig[flashIndex], start, lengthInBytes, margin);
}

status_t FLASH_VerifyEraseAll(flash_config_t *config, ftfx_margin_value_t margin)
{
    return FTFx_CMD_VerifyEraseAll(&config->ftfxConfig[0], margin);
}

status_t FLASH_VerifyProgram(flash_config_t *config,
                             uint32_t start,
                             uint32_t lengthInBytes,
                             const uint8_t *expectedData,
                             ftfx_margin_value_t margin,
                             uint32_t *failedAddress,
                             uint32_t *failedData)
{
    status_t returnCode;
    uint8_t flashIndex;

    returnCode = flash_check_range_to_get_index(config, start, lengthInBytes, &flashIndex);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    returnCode = flash_convert_start_address(&config->ftfxConfig[flashIndex], start);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    return FTFx_CMD_VerifyProgram(&config->ftfxConfig[flashIndex], start, lengthInBytes, expectedData, margin, failedAddress, failedData);
}

status_t FLASH_GetSecurityState(flash_config_t *config, ftfx_security_state_t *state)
{
    return FTFx_REG_GetSecurityState(&config->ftfxConfig[0], state);
}

status_t FLASH_SecurityBypass(flash_config_t *config, const uint8_t *backdoorKey)
{
    return FTFx_CMD_SecurityBypass(&config->ftfxConfig[0], backdoorKey);
}

#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
status_t FLASH_SetFlexramFunction(flash_config_t *config, ftfx_flexram_func_opt_t option)
{
    return FTFx_CMD_SetFlexramFunction(&config->ftfxConfig[0], option);
}
#endif

#if defined(FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP) && FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP
status_t FLASH_Swap(flash_config_t *config, uint32_t address, bool isSetEnable)
{
    status_t returnCode;
    ftfx_swap_state_config_t returnInfo;
    ftfx_config_t *ftfxConfig;
    uint8_t flashIndex;

    returnCode = flash_check_range_to_get_index(config, address, 1, &flashIndex);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    ftfxConfig = &config->ftfxConfig[flashIndex];

    memset(&returnInfo, 0xFFU, sizeof(returnInfo));

    do
    {
        returnCode = FTFx_CMD_SwapControl(ftfxConfig, address, kFTFx_SwapControlOptionReportStatus, &returnInfo);
        if (returnCode != kStatus_FTFx_Success)
        {
            return returnCode;
        }

        if (!isSetEnable)
        {
            if (returnInfo.flashSwapState == kFTFx_SwapStateDisabled)
            {
                return kStatus_FTFx_Success;
            }
            else if (returnInfo.flashSwapState == kFTFx_SwapStateUninitialized)
            {
                /* The swap system changed to the DISABLED state with Program flash block 0
                 * located at relative flash address 0x0_0000 */
                returnCode = FTFx_CMD_SwapControl(ftfxConfig, address, kFTFx_SwapControlOptionDisableSystem, &returnInfo);
            }
            else
            {
                /* Swap disable should be requested only when swap system is in the uninitialized state */
                return kStatus_FTFx_SwapSystemNotInUninitialized;
            }
        }
        else
        {
            /* When first swap: the initial swap state is Uninitialized, flash swap indicator address is unset,
             *    the swap procedure should be Uninitialized -> Update-Erased -> Complete.
             * After the first swap has been completed, the flash swap inidicator address cannot be modified
             *    unless EraseAllBlocks command is issued, the swap procedure is changed to Update -> Update-Erased ->
             *    Complete. */
            switch (returnInfo.flashSwapState)
            {
                case kFTFx_SwapStateUninitialized:
                    /* If current swap mode is Uninitialized, Initialize Swap to Initialized/READY state. */
                    returnCode =
                        FTFx_CMD_SwapControl(ftfxConfig, address, kFTFx_SwapControlOptionIntializeSystem, &returnInfo);
                    break;
                case kFTFx_SwapStateReady:
                    /* Validate whether the address provided to the swap system is matched to
                     * swap indicator address in the IFR */
                    returnCode = flash_validate_swap_indicator_address(ftfxConfig, address);
                    if (returnCode == kStatus_FTFx_Success)
                    {
                        /* If current swap mode is Initialized/Ready, Initialize Swap to UPDATE state. */
                        returnCode =
                            FTFx_CMD_SwapControl(ftfxConfig, address, kFTFx_SwapControlOptionSetInUpdateState, &returnInfo);
                    }
                    break;
                case kFTFx_SwapStateUpdate:
                    /* If current swap mode is Update, Erase indicator sector in non active block
                     * to proceed swap system to update-erased state */
                    returnCode = FLASH_Erase(config, address + (ftfxConfig->flashDesc.totalSize >> 1),
                                              ftfxConfig->opsConfig.addrAligment.sectorCmd, kFTFx_ApiEraseKey);
                    break;
                case kFTFx_SwapStateUpdateErased:
                    /* If current swap mode is Update or Update-Erased, progress Swap to COMPLETE State */
                    returnCode =
                        FTFx_CMD_SwapControl(ftfxConfig, address, kFTFx_SwapControlOptionSetInCompleteState, &returnInfo);
                    break;
                case kFTFx_SwapStateComplete:
                    break;
                case kFTFx_SwapStateDisabled:
                    /* When swap system is in disabled state, We need to clear swap system back to uninitialized
                     * by issuing EraseAllBlocks command */
                    returnCode = kStatus_FTFx_SwapSystemNotInUninitialized;
                    break;
                default:
                    returnCode = kStatus_FTFx_InvalidArgument;
                    break;
            }
        }
        if (returnCode != kStatus_FTFx_Success)
        {
            break;
        }
    } while (!((kFTFx_SwapStateComplete == returnInfo.flashSwapState) && isSetEnable));

    return returnCode;
}
#endif /* FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP */

status_t FLASH_IsProtected(flash_config_t *config,
                           uint32_t start,
                           uint32_t lengthInBytes,
                           flash_prot_state_t *protection_state)
{
    status_t returnCode;
    ftfx_config_t *ftfxConfig;
    uint8_t flashIndex;

    if (protection_state == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    returnCode = flash_check_range_to_get_index(config, start, lengthInBytes, &flashIndex);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    ftfxConfig = &config->ftfxConfig[flashIndex];

    if (ftfxConfig->flashDesc.feature.hasProtControl)
    {
        uint32_t endAddress;           /* end address for protection check */
        uint32_t regionCheckedCounter; /* increments each time the flash address was checked for
                                        * protection status */
        uint32_t regionCounter;        /* incrementing variable used to increment through the flash
                                        * protection regions */
        uint32_t protectStatusCounter; /* increments each time a flash region was detected as protected */
        uint8_t flashRegionProtectStatus[MAX_FLASH_PROT_REGION_COUNT]; /* array of the protection
                                                                          * status for each
                                                                          * protection region */
        uint32_t flashRegionAddress[MAX_FLASH_PROT_REGION_COUNT + 1];  /* array of the start addresses for each flash
                                                                        * protection region. Note this is REGION_COUNT+1
                                                                        * due to requiring the next start address after
                                                                        * the end of flash for loop-check purposes below */
        bool isBreakNeeded = false;
        /* calculating Flash end address */
        endAddress = start + lengthInBytes;

        /* populate the flashRegionAddress array with the start address of each flash region */
        regionCounter = 0; /* make sure regionCounter is initialized to 0 first */
        /* populate up to 33rd element of array, this is the next address after end of flash array */
        while (regionCounter <= ftfxConfig->flashDesc.protectRegionMem.count)
        {
            flashRegionAddress[regionCounter] =
                ftfxConfig->flashDesc.protectRegionMem.base + ftfxConfig->flashDesc.protectRegionMem.size * regionCounter;
            regionCounter++;
        }

        /* populate flashRegionProtectStatus array with status information
         * Protection status for each region is stored in the FPROT[3:0] registers
         * Each bit represents one region of flash
         * 4 registers * 8-bits-per-register = 32-bits (32-regions)
         * The convention is:
         * FPROT3[bit 0] is the first protection region (start of flash memory)
         * FPROT0[bit 7] is the last protection region (end of flash memory)
         * regionCounter is used to determine which FPROT[3:0] register to check for protection status
         * Note: FPROT=1 means NOT protected, FPROT=0 means protected */
        regionCounter = 0; /* make sure regionCounter is initialized to 0 first */
        while (regionCounter < ftfxConfig->flashDesc.protectRegionMem.count)
        {
            if ((ftfxConfig->flashDesc.index == 0) || (!ftfxConfig->flashDesc.feature.hasIndProtReg))
            {
                /* Note: So far protection region count may be 16/20/24/32/64 */
                if (regionCounter < 8)
                {
                    flashRegionProtectStatus[regionCounter] = (FTFx_FPROTL3_REG >> regionCounter) & (0x01u);
                }
                else if (regionCounter < 16)
                {
                    flashRegionProtectStatus[regionCounter] = (FTFx_FPROTL2_REG >> (regionCounter - 8)) & (0x01u);
                }
#if defined(MAX_FLASH_PROT_REGION_COUNT) && (MAX_FLASH_PROT_REGION_COUNT > 16)
#if (MAX_FLASH_PROT_REGION_COUNT == 20)
                else if (regionCounter < 20)
                {
                    flashRegionProtectStatus[regionCounter] = (FTFx_FPROTL1_REG >> (regionCounter - 16)) & (0x01u);
                }
#else 
                else if (regionCounter < 24)
                {
                    flashRegionProtectStatus[regionCounter] = (FTFx_FPROTL1_REG >> (regionCounter - 16)) & (0x01u);
                }
#endif /*(MAX_FLASH_PROT_REGION_COUNT == 20)*/
#endif
#if defined(MAX_FLASH_PROT_REGION_COUNT) && (MAX_FLASH_PROT_REGION_COUNT > 24)
                else if (regionCounter < 32)
                {
                    flashRegionProtectStatus[regionCounter] = (FTFx_FPROTL0_REG >> (regionCounter - 24)) & (0x01u);
                }
#endif
#if defined(MAX_FLASH_PROT_REGION_COUNT) && (MAX_FLASH_PROT_REGION_COUNT == 64)
                else if (regionCounter < 40)
                {
                    flashRegionProtectStatus[regionCounter] = (FTFx_FPROTH3_REG >> (regionCounter - 32)) & (0x01u);
                }
                else if (regionCounter < 48)
                {
                    flashRegionProtectStatus[regionCounter] = (FTFx_FPROTH2_REG >> (regionCounter - 40)) & (0x01u);
                }
                else if (regionCounter < 56)
                {
                    flashRegionProtectStatus[regionCounter] = (FTFx_FPROTH1_REG >> (regionCounter - 48)) & (0x01u);
                }
                else if (regionCounter < 64)
                {
                    flashRegionProtectStatus[regionCounter] = (FTFx_FPROTH0_REG >> (regionCounter - 56)) & (0x01u);
                }
#endif
                else
                {
                    isBreakNeeded = true;
                }
                regionCounter++;
            }
            else if ((ftfxConfig->flashDesc.index == 1) && ftfxConfig->flashDesc.feature.hasIndProtReg)
            {
                /* Note: So far protection region count may be 8/16 */
                if (regionCounter < 8)
                {
                    flashRegionProtectStatus[regionCounter] = (FTFx_FPROTSL_REG >> regionCounter) & (0x01u);
                }
                else if (regionCounter < 16)
                {
                    flashRegionProtectStatus[regionCounter] = (FTFx_FPROTSH_REG >> (regionCounter - 8)) & (0x01u);
                }
                else
                {
                    isBreakNeeded = true;
                }
                regionCounter++;
            }
            else
            {}
            
            if (isBreakNeeded)
            {
                break;
            }
        }

        /* loop through the flash regions and check
         * desired flash address range for protection status
         * loop stops when it is detected that start has exceeded the endAddress */
        regionCounter = 0; /* make sure regionCounter is initialized to 0 first */
        regionCheckedCounter = 0;
        protectStatusCounter = 0; /* make sure protectStatusCounter is initialized to 0 first */
        while (start < endAddress)
        {
            /* check to see if the address falls within this protection region
             * Note that if the entire flash is to be checked, the last protection
             * region checked would consist of the last protection start address and
             * the start address following the end of flash */
            if ((start >= flashRegionAddress[regionCounter]) && (start < flashRegionAddress[regionCounter + 1]))
            {
                /* increment regionCheckedCounter to indicate this region was checked */
                regionCheckedCounter++;

                /* check the protection status of this region
                 * Note: FPROT=1 means NOT protected, FPROT=0 means protected */
                if (!flashRegionProtectStatus[regionCounter])
                {
                    /* increment protectStatusCounter to indicate this region is protected */
                    protectStatusCounter++;
                }
                start += ftfxConfig->flashDesc.protectRegionMem.size; /* increment to an address within the next region */
            }
            regionCounter++; /* increment regionCounter to check for the next flash protection region */
        }

        /* if protectStatusCounter == 0, then no region of the desired flash region is protected */
        if (protectStatusCounter == 0)
        {
            *protection_state = kFLASH_ProtectionStateUnprotected;
        }
        /* if protectStatusCounter == regionCheckedCounter, then each region checked was protected */
        else if (protectStatusCounter == regionCheckedCounter)
        {
            *protection_state = kFLASH_ProtectionStateProtected;
        }
        /* if protectStatusCounter != regionCheckedCounter, then protection status is mixed
         * In other words, some regions are protected while others are unprotected */
        else
        {
            *protection_state = kFLASH_ProtectionStateMixed;
        }
    }
    else
    {
        *protection_state = kFLASH_ProtectionStateUnprotected;
    }

    return kStatus_FTFx_Success;
}

status_t FLASH_IsExecuteOnly(flash_config_t *config,
                             uint32_t start,
                             uint32_t lengthInBytes,
                             flash_xacc_state_t *access_state)
{
    status_t returnCode;
    ftfx_config_t *ftfxConfig;
    uint8_t flashIndex;

    if (access_state == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    returnCode = flash_check_range_to_get_index(config, start, lengthInBytes, &flashIndex);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    ftfxConfig = &config->ftfxConfig[flashIndex];

    if (ftfxConfig->flashDesc.feature.hasXaccControl)
    {
#if defined(FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL) && FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL
        uint32_t executeOnlySegmentCounter = 0;

        /* calculating end address */
        uint32_t endAddress = start + lengthInBytes;

        /* Aligning start address and end address */
        uint32_t alignedStartAddress = ALIGN_DOWN(start, ftfxConfig->flashDesc.accessSegmentMem.size);
        uint32_t alignedEndAddress = ALIGN_UP(endAddress, ftfxConfig->flashDesc.accessSegmentMem.size);

        uint32_t segmentIndex = 0;
        uint32_t maxSupportedExecuteOnlySegmentCount =
            (alignedEndAddress - alignedStartAddress) / ftfxConfig->flashDesc.accessSegmentMem.size;

        while (start < endAddress)
        {
            uint32_t xacc = 0;
            bool isInvalidSegmentIndex = false;

            segmentIndex = (start - ftfxConfig->flashDesc.accessSegmentMem.base) / ftfxConfig->flashDesc.accessSegmentMem.size;

            if ((ftfxConfig->flashDesc.index == 0) || (!ftfxConfig->flashDesc.feature.hasIndXaccReg))
            {
                /* For primary flash, The eight XACC registers allow up to 64 restricted segments of equal memory size.
                 */
                if (segmentIndex < 32)
                {
                    xacc = *(const volatile uint32_t *)&FTFx_XACCL3_REG;
                }
                else if (segmentIndex < ftfxConfig->flashDesc.accessSegmentMem.count)
                {
                    xacc = *(const volatile uint32_t *)&FTFx_XACCH3_REG;
                    segmentIndex -= 32;
                }
                else
                {
                    isInvalidSegmentIndex = true;
                }
            }
#if FTFx_DRIVER_HAS_FLASH1_SUPPORT
            else  if ((ftfxConfig->flashDesc.index == 1) && ftfxConfig->flashDesc.feature.hasIndXaccReg)
            {
                /* For secondary flash, The two XACCS registers allow up to 16 restricted segments of equal memory size.
                 */
                if (segmentIndex < 8)
                {
                    xacc = *(const volatile uint8_t *)&FTFx_XACCSL_REG;
                }
                else if (segmentIndex < ftfxConfig->flashDesc.accessSegmentMem.count)
                {
                    xacc = *(const volatile uint8_t *)&FTFx_XACCSH_REG;
                    segmentIndex -= 8;
                }
                else
                {
                    isInvalidSegmentIndex = true;
                }
            }
#endif
            else
            {}

            if (isInvalidSegmentIndex)
            {
                break;
            }

            /* Determine if this address range is in a execute-only protection flash segment. */
            if ((~xacc) & (1u << segmentIndex))
            {
                executeOnlySegmentCounter++;
            }

            start += ftfxConfig->flashDesc.accessSegmentMem.size;
        }

        if (executeOnlySegmentCounter < 1u)
        {
            *access_state = kFLASH_AccessStateUnLimited;
        }
        else if (executeOnlySegmentCounter < maxSupportedExecuteOnlySegmentCount)
        {
            *access_state = kFLASH_AccessStateMixed;
        }
        else
        {
            *access_state = kFLASH_AccessStateExecuteOnly;
        }
#endif /* FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL */
    }
    else
    {
        *access_state = kFLASH_AccessStateUnLimited;
    }

    return kStatus_FTFx_Success;
}

status_t FLASH_PflashSetProtection(flash_config_t *config, pflash_prot_status_t *protectStatus)
{
    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    if (config->ftfxConfig[0].flashDesc.feature.hasProtControl)
    {
        if (config->ftfxConfig[0].flashDesc.feature.ProtRegBits >= 32)
        {
            *kFPROTL = protectStatus->protl;
            if (protectStatus->protl != *kFPROTL)
            {
                return kStatus_FTFx_CommandFailure;
            }
        }
        if (config->ftfxConfig[0].flashDesc.feature.ProtRegBits == 64)
        {
            *kFPROTH = protectStatus->proth;
            if (protectStatus->proth != *kFPROTH)
            {
                return kStatus_FTFx_CommandFailure;
            }
        }
    }
#if FTFx_DRIVER_HAS_FLASH1_SUPPORT
    else if (config->ftfxConfig[1].flashDesc.feature.hasProtControl && \
             config->ftfxConfig[1].flashDesc.feature.hasIndProtReg)
    {
        if (config->ftfxConfig[1].flashDesc.feature.ProtRegBits == 16)
        {
            *kFPROTSL = protectStatus->protsl;
            if (protectStatus->protsl != *kFPROTSL)
            {
                return kStatus_FTFx_CommandFailure;
            }
            *kFPROTSH = protectStatus->protsh;
            if (protectStatus->protsh != *kFPROTSH)
            {
                return kStatus_FTFx_CommandFailure;
            }
        }
    }
#endif

    return kStatus_FTFx_Success;
}

status_t FLASH_PflashGetProtection(flash_config_t *config, pflash_prot_status_t *protectStatus)
{
    if ((config == NULL) || (protectStatus == NULL))
    {
        return kStatus_FTFx_InvalidArgument;
    }

    if (config->ftfxConfig[0].flashDesc.feature.hasProtControl)
    {
        if (config->ftfxConfig[0].flashDesc.feature.ProtRegBits >= 32)
        {
            protectStatus->protl = *kFPROTL;
        }
        if (config->ftfxConfig[0].flashDesc.feature.ProtRegBits == 64)
        {
            protectStatus->proth = *kFPROTH;
        }
    }
#if FTFx_DRIVER_HAS_FLASH1_SUPPORT
    else if (config->ftfxConfig[1].flashDesc.feature.hasProtControl && \
             config->ftfxConfig[1].flashDesc.feature.hasIndProtReg)
    {
        if (config->ftfxConfig[0].flashDesc.feature.ProtRegBits == 16)
        {
            protectStatus->protsl = *kFPROTSL;
            protectStatus->protsh = *kFPROTSH;
        }
    }
#endif

    return kStatus_FTFx_Success;
}

status_t FLASH_GetProperty(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t *value)
{
    if ((config == NULL) || (value == NULL))
    {
        return kStatus_FTFx_InvalidArgument;
    }

    switch (whichProperty)
    {
        case kFLASH_PropertyPflash0SectorSize:
            *value = config->ftfxConfig[0].flashDesc.sectorSize;
            break;
        case kFLASH_PropertyPflash0TotalSize:
            *value = config->ftfxConfig[0].flashDesc.totalSize;
            break;
        case kFLASH_PropertyPflash0BlockSize:
            *value = config->ftfxConfig[0].flashDesc.totalSize / config->ftfxConfig[0].flashDesc.blockCount;
            break;
        case kFLASH_PropertyPflash0BlockCount:
            *value = config->ftfxConfig[0].flashDesc.blockCount;
            break;
        case kFLASH_PropertyPflash0BlockBaseAddr:
            *value = config->ftfxConfig[0].flashDesc.blockBase;
            break;
        case kFLASH_PropertyPflash0FacSupport:
            *value = (uint32_t)config->ftfxConfig[0].flashDesc.feature.hasXaccControl;
            break;
        case kFLASH_PropertyPflash0AccessSegmentSize:
            *value = config->ftfxConfig[0].flashDesc.accessSegmentMem.size;
            break;
        case kFLASH_PropertyPflash0AccessSegmentCount:
            *value = config->ftfxConfig[0].flashDesc.accessSegmentMem.count;
            break;

#if FTFx_DRIVER_HAS_FLASH1_SUPPORT
        case kFLASH_PropertyPflash1SectorSize:
            *value = config->ftfxConfig[1].flashDesc.sectorSize;
            break;
        case kFLASH_PropertyPflash1TotalSize:
            *value = config->ftfxConfig[1].flashDesc.totalSize;
            break;
        case kFLASH_PropertyPflash1BlockSize:
            *value = config->ftfxConfig[1].flashDesc.totalSize / config->ftfxConfig[1].flashDesc.blockCount;
            break;
        case kFLASH_PropertyPflash1BlockCount:
            *value = config->ftfxConfig[1].flashDesc.blockCount;
            break;
        case kFLASH_PropertyPflash1BlockBaseAddr:
            *value = config->ftfxConfig[1].flashDesc.blockBase;
            break;
        case kFLASH_PropertyPflash1FacSupport:
            *value = (uint32_t)config->ftfxConfig[1].flashDesc.feature.hasXaccControl;
            break;
        case kFLASH_PropertyPflash1AccessSegmentSize:
            *value = config->ftfxConfig[1].flashDesc.accessSegmentMem.size;
            break;
        case kFLASH_PropertyPflash1AccessSegmentCount:
            *value = config->ftfxConfig[1].flashDesc.accessSegmentMem.count;
            break;
#endif

        case kFLASH_PropertyFlexRamBlockBaseAddr:
            *value = config->ftfxConfig[0].flexramBlockBase;
            break;
        case kFLASH_PropertyFlexRamTotalSize:
            *value = config->ftfxConfig[0].flexramTotalSize;
            break;

        default: /* catch inputs that are not recognized */
            return kStatus_FTFx_UnknownProperty;
    }

    return kStatus_FTFx_Success;
}

static void flash_init_features(ftfx_config_t *config)
{
    if (config->flashDesc.index == 0)
    {
        config->flashDesc.feature.isIndBlock = 1;
        config->flashDesc.feature.hasIndPfsizeReg = 1;
        config->flashDesc.feature.hasIndProtReg = 1;
        config->flashDesc.feature.hasIndXaccReg = 1;
    }
#if FTFx_DRIVER_HAS_FLASH1_SUPPORT
    else if (config->flashDesc.index == 1)
    {
        config->flashDesc.feature.isIndBlock = FTFx_FLASH1_IS_INDEPENDENT_BLOCK;
        config->flashDesc.feature.hasIndPfsizeReg = config->flashDesc.feature.isIndBlock;
        config->flashDesc.feature.hasIndProtReg = FTFx_FLASH1_HAS_INT_PROT_REG;
        config->flashDesc.feature.hasIndXaccReg = FTFx_FLASH1_HAS_INT_XACC_REG;
    }
#endif

    config->flashDesc.feature.hasProtControl = 1;
    config->flashDesc.feature.hasXaccControl = FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL;
}

static uint32_t flash_calculate_mem_size(uint32_t pflashBlockCount,
                                         uint32_t pflashBlockSize,
                                         uint32_t pfsizeMask,
                                         uint32_t pfsizeShift)
{
    uint8_t pfsize;
    uint32_t flashDensity;

    /* PFSIZE=0xf means that on customer parts the IFR was not correctly programmed.
     * We just use the pre-defined flash size in feature file here to support pre-production parts */
    pfsize = (SIM_FCFG1_REG & pfsizeMask) >> pfsizeShift;
    if (pfsize == 0xf)
    {
        flashDensity = pflashBlockCount * pflashBlockSize;
    }
    else
    {
        flashDensity = ((uint32_t)kPFlashDensities[pfsize]) << 10;
    }

    return flashDensity;
}

static uint32_t flash_calculate_prot_segment_size(uint32_t flashSize, uint32_t segmentCount)
{
    uint32_t segmentSize;

    /* Calculate the size of the flash protection region
     * If the flash density is > 32KB, then protection region is 1/32 of total flash density
     * Else if flash density is < 32KB, then flash protection region is set to 1KB */
    if (flashSize > segmentCount * kFTFx_MinProtectBlockSize)
    {
        segmentSize = flashSize / segmentCount;
    }
    else
    {
        segmentSize = kFTFx_MinProtectBlockSize;
    }

    return segmentSize;
}

static status_t flash_check_range_to_get_index(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint8_t *flashIndex)
{
    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* Validates the range of the given address */
    for (uint8_t index = 0; index < FTFx_FLASH_COUNT; index++)
    {
        if ((start >= config->ftfxConfig[index].flashDesc.blockBase) &&
            ((start + lengthInBytes) <= (config->ftfxConfig[index].flashDesc.blockBase + config->ftfxConfig[index].flashDesc.totalSize)))
        {
            *flashIndex = config->ftfxConfig[index].flashDesc.index;
            return kStatus_FTFx_Success;
        }
    }

    return kStatus_FTFx_AddressError;
}

static status_t flash_convert_start_address(ftfx_config_t *config, uint32_t start)
{
    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    if (config->flashDesc.index && config->flashDesc.feature.isIndBlock)
    {
        /* When required by the command, address bit 23 selects between main flash memory
         * (=0) and secondary flash memory (=1).*/
        config->opsConfig.convertedAddress = start - config->flashDesc.blockBase + 0x800000U;
    }
    else
    {
        config->opsConfig.convertedAddress = start;
    }

    return kStatus_FTFx_Success;
}

#if defined(FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP) && FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP
/*! @brief Validates the gived address to see if it is equal to swap indicator address in pflash swap IFR.*/
static status_t flash_validate_swap_indicator_address(ftfx_config_t *config, uint32_t address)
{
    status_t returnCode;
    struct _flash_swap_ifr_field_config
    {
        uint16_t swapIndicatorAddress; /*!< A Swap indicator address field.*/
        uint16_t swapEnableWord;       /*!< A Swap enable word field.*/
        uint8_t reserved0[4];          /*!< A reserved field.*/
        uint8_t reserved1[2];          /*!< A reserved field.*/
        uint16_t swapDisableWord;      /*!< A Swap disable word field.*/
        uint8_t reserved2[4];          /*!< A reserved field.*/
    } flashSwapIfrFieldData;
    uint32_t swapIndicatorAddress;

#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
    returnCode =
        FTFx_CMD_ReadResource(config, config->ifrDesc.resRange.pflashSwapIfrStart, (uint8_t *)&flashSwapIfrFieldData,
                               sizeof(flashSwapIfrFieldData), kFTFx_ResourceOptionFlashIfr);

    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }
#else
    {
        /* From RM, the actual info are stored in FCCOB6,7 */
        uint32_t returnValue[2];
        returnCode = FTFx_CMD_ReadOnce(config, kFLASH_RecordIndexSwapAddr, (uint8_t *)returnValue, 4);
        if (returnCode != kStatus_FTFx_Success)
        {
            return returnCode;
        }
        flashSwapIfrFieldData.swapIndicatorAddress = (uint16_t)returnValue[0];
        returnCode = FTFx_CMD_ReadOnce(config, kFLASH_RecordIndexSwapEnable, (uint8_t *)returnValue, 4);
        if (returnCode != kStatus_FTFx_Success)
        {
            return returnCode;
        }
        flashSwapIfrFieldData.swapEnableWord = (uint16_t)returnValue[0];
        returnCode = FTFx_CMD_ReadOnce(config, kFLASH_RecordIndexSwapDisable, (uint8_t *)returnValue, 4);
        if (returnCode != kStatus_FTFx_Success)
        {
            return returnCode;
        }
        flashSwapIfrFieldData.swapDisableWord = (uint16_t)returnValue[0];
    }
#endif

    /* The high bits value of Swap Indicator Address is stored in Program Flash Swap IFR Field,
     * the low severval bit value of Swap Indicator Address is always 1'b0 */
    swapIndicatorAddress = (uint32_t)flashSwapIfrFieldData.swapIndicatorAddress *
                           config->opsConfig.addrAligment.swapCtrlCmd;
    if (address != swapIndicatorAddress)
    {
        return kStatus_FTFx_SwapIndicatorAddressError;
    }

    return returnCode;
}
#endif /* FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP */

