/*
* Copyright 2013-2016 Freescale Semiconductor, Inc.
* Copyright 2016-2018 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*
*/

#include "fsl_ftfx_controller.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @name Flash controller command numbers
 * @{
 */
#define FTFx_VERIFY_BLOCK 0x00U                    /*!< RD1BLK*/
#define FTFx_VERIFY_SECTION 0x01U                  /*!< RD1SEC*/
#define FTFx_PROGRAM_CHECK 0x02U                   /*!< PGMCHK*/
#define FTFx_READ_RESOURCE 0x03U                   /*!< RDRSRC*/
#define FTFx_PROGRAM_LONGWORD 0x06U                /*!< PGM4*/
#define FTFx_PROGRAM_PHRASE 0x07U                  /*!< PGM8*/
#define FTFx_ERASE_BLOCK 0x08U                     /*!< ERSBLK*/
#define FTFx_ERASE_SECTOR 0x09U                    /*!< ERSSCR*/
#define FTFx_PROGRAM_SECTION 0x0BU                 /*!< PGMSEC*/
#define FTFx_GENERATE_CRC 0x0CU                    /*!< CRCGEN*/
#define FTFx_VERIFY_ALL_BLOCK 0x40U                /*!< RD1ALL*/
#define FTFx_READ_ONCE 0x41U                       /*!< RDONCE or RDINDEX*/
#define FTFx_PROGRAM_ONCE 0x43U                    /*!< PGMONCE or PGMINDEX*/
#define FTFx_ERASE_ALL_BLOCK 0x44U                 /*!< ERSALL*/
#define FTFx_SECURITY_BY_PASS 0x45U                /*!< VFYKEY*/
#define FTFx_SWAP_CONTROL 0x46U                    /*!< SWAP*/
#define FTFx_ERASE_ALL_BLOCK_UNSECURE 0x49U        /*!< ERSALLU*/
#define FTFx_VERIFY_ALL_EXECUTE_ONLY_SEGMENT 0x4AU /*!< RD1XA*/
#define FTFx_ERASE_ALL_EXECUTE_ONLY_SEGMENT 0x4BU  /*!< ERSXA*/
#define FTFx_PROGRAM_PARTITION 0x80U               /*!< PGMPART*/
#define FTFx_SET_FLEXRAM_FUNCTION 0x81U            /*!< SETRAM*/
/*@}*/

/*!
 * @brief Constants for execute-in-RAM flash function.
 */
enum _ftfx_ram_func_constants
{
    kFTFx_RamFuncMaxSizeInWords = 16U, /*!< The maximum size of execute-in-RAM function.*/
};

/*! @brief A function pointer used to point to relocated flash_run_command() */
typedef void (*callFtfxRunCommand_t)(FTFx_REG8_ACCESS_TYPE ftfx_fstat);

/*!
 * @name Enumeration for Flash security register code
 * @{
 */
enum _ftfx_fsec_register_code
{
    kFTFx_FsecRegCode_KEYEN_Enabled = 0x80U,
    kFTFx_FsecRegCode_SEC_Unsecured = 0x02U
};
/*@}*/

/*!
 * @brief Enumeration for flash config area.
 */
enum _ftfx_pflash_config_area_range
{
    kFTFx_PflashConfigAreaStart = 0x400U,
    kFTFx_PflashConfigAreaEnd = 0x40FU
};


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*! @brief Init IFR memory related info */
static status_t ftfx_init_ifr(ftfx_config_t *config);

#if FTFx_DRIVER_IS_FLASH_RESIDENT
/*! @brief Copy flash_run_command() to RAM*/
static void ftfx_copy_run_command_to_ram(uint32_t *ftfxRunCommand);
#endif /* FTFx_DRIVER_IS_FLASH_RESIDENT */

/*! @brief Internal function Flash command sequence. Called by driver APIs only*/
static status_t ftfx_command_sequence(ftfx_config_t *config);

/*! @brief Validates the range and alignment of the given address range.*/
static status_t ftfx_check_mem_range(ftfx_config_t *config,
                                     uint32_t startAddress,
                                     uint32_t lengthInBytes,
                                     uint8_t alignmentBaseline);

/*! @brief Validates the given user key for flash erase APIs.*/
static status_t ftfx_check_user_key(uint32_t key);

/*! @brief Reads word from byte address.*/
static uint32_t ftfx_read_word_from_byte_address(const uint8_t *src);

/*! @brief Writes word to byte address.*/
static void ftfx_write_word_to_byte_address(uint8_t *dst, uint32_t word);

#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
/*! @brief Validates the range of the given resource address.*/
static status_t ftfx_check_resource_range(ftfx_config_t *config,
                                          uint32_t start,
                                          uint32_t lengthInBytes,
                                          uint32_t alignmentBaseline,
                                          ftfx_read_resource_opt_t option);
#endif /* FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
/*! @brief Validates the given flexram function option.*/
static inline status_t ftfx_check_flexram_function_option(ftfx_flexram_func_opt_t option);
#endif /* FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD) && FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD
/*! @brief Validates the given swap control option.*/
static status_t ftfx_check_swap_control_option(ftfx_swap_control_opt_t option);
#endif /* FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD */

/*******************************************************************************
 * Variables
 ******************************************************************************/

#if FTFx_DRIVER_IS_FLASH_RESIDENT
/*!
 * @brief Position independent code of flash_run_command()
 *
 * Note1: The prototype of C function is shown as below:
 * @code
 *   void flash_run_command(FTFx_REG8_ACCESS_TYPE ftfx_fstat)
 *   {
 *       // clear CCIF bit
 *       *ftfx_fstat = FTFx_FSTAT_CCIF_MASK;
 *
 *       // Check CCIF bit of the flash status register, wait till it is set.
 *       // IP team indicates that this loop will always complete.
 *       while (!((*ftfx_fstat) & FTFx_FSTAT_CCIF_MASK))
 *       {
 *       }
 *   }
 * @endcode
 * Note2: The binary code is generated by IAR 7.70.1
 */
static const uint16_t s_ftfxRunCommandFunctionCode[] = {
    0x2180, /* MOVS  R1, #128 ; 0x80 */
    0x7001, /* STRB  R1, [R0] */
    /* @4: */
    0x7802, /* LDRB  R2, [R0] */
    0x420a, /* TST   R2, R1 */
    0xd0fc, /* BEQ.N @4 */
    0x4770  /* BX    LR */
};
#if (!FTFx_DRIVER_IS_EXPORTED)
/*! @brief A static buffer used to hold flash_run_command() */
static uint32_t s_ftfxRunCommand[kFTFx_RamFuncMaxSizeInWords];
#endif /* (!FTFx_DRIVER_IS_EXPORTED) */
#endif /* FTFx_DRIVER_IS_FLASH_RESIDENT */

/*! @brief Access to FTFx Registers */
static volatile uint32_t *const kFCCOBx = (volatile uint32_t *)&FTFx_FCCOB3_REG;

#if FSL_FEATURE_FLASH_HAS_FLEX_NVM
/*! @brief Table of eeprom sizes. */
static const uint16_t kEepromDensities[16] = {
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0000,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0001,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0010,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0011,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0100,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0101,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0110,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0111,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1000,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1001,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1010,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1011,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1100,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1101,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1110,
    FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1111
};
/*! @brief Table of dflash sizes. */
static const uint32_t kDflashDensities[16] = {
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0000,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0001,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0010,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0011,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0100,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0101,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0110,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0111,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1000,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1001,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1010,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1011,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1100,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1101,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1110,
    FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1111
};
#endif /* FSL_FEATURE_FLASH_HAS_FLEX_NVM */

/*******************************************************************************
 * Code
 ******************************************************************************/

status_t FTFx_API_Init(ftfx_config_t *config)
{
    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    config->flexramBlockBase = FSL_FEATURE_FLASH_FLEX_RAM_START_ADDRESS;
    config->flexramTotalSize = FSL_FEATURE_FLASH_FLEX_RAM_SIZE;

    /* copy required flash command to RAM */
#if FTFx_DRIVER_IS_FLASH_RESIDENT
    if (NULL == config->runCmdFuncAddr)
    {
#if FTFx_DRIVER_IS_EXPORTED
        return kStatus_FTFx_ExecuteInRamFunctionNotReady;
#else
        config->runCmdFuncAddr = s_ftfxRunCommand;
#endif /* FTFx_DRIVER_IS_EXPORTED */
    }
    ftfx_copy_run_command_to_ram(config->runCmdFuncAddr);
#endif /* FTFx_DRIVER_IS_FLASH_RESIDENT */

    ftfx_init_ifr(config);

    return kStatus_FTFx_Success;
}

#if FSL_FEATURE_FLASH_HAS_FLEX_NVM
status_t FTFx_API_UpdateFlexnvmPartitionStatus(ftfx_config_t *config)
{
    struct _dflash_ifr_field_config
    {
        uint32_t reserved0;
        uint8_t FlexNVMPartitionCode;
        uint8_t EEPROMDataSetSize;
        uint16_t reserved1;
    } dataIFRReadOut;
    uint32_t flexnvmInfoIfrAddr;
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    flexnvmInfoIfrAddr = config->ifrDesc.resRange.dflashIfrStart + config->ifrDesc.resRange.ifrMemSize - sizeof(dataIFRReadOut);

#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
    /* Get FlexNVM memory partition info from data flash IFR */
    returnCode = FTFx_CMD_ReadResource(config, flexnvmInfoIfrAddr, (uint8_t *)&dataIFRReadOut,
                                       sizeof(dataIFRReadOut), kFTFx_ResourceOptionFlashIfr);
    if (returnCode != kStatus_FTFx_Success)
    {
        return kStatus_FTFx_PartitionStatusUpdateFailure;
    }
#else
#error "Cannot get FlexNVM memory partition info"
#endif

    /* Fill out partitioned EEPROM size */
    dataIFRReadOut.EEPROMDataSetSize &= 0x0FU;
    config->eepromTotalSize = kEepromDensities[dataIFRReadOut.EEPROMDataSetSize];

    /* Fill out partitioned DFlash size */
    dataIFRReadOut.FlexNVMPartitionCode &= 0x0FU;
    config->flashDesc.totalSize = kDflashDensities[dataIFRReadOut.FlexNVMPartitionCode];

    return kStatus_FTFx_Success;
}
#endif /* FSL_FEATURE_FLASH_HAS_FLEX_NVM */

status_t FTFx_CMD_Erase(ftfx_config_t *config,
                        uint32_t start,
                        uint32_t lengthInBytes,
                        uint32_t key)
{
    uint32_t sectorSize;
    uint32_t endAddress;      /* storing end address */
    uint32_t numberOfSectors; /* number of sectors calculated by endAddress */
    status_t returnCode;

    /* Check the supplied address range. */
    returnCode = ftfx_check_mem_range(config, start, lengthInBytes, config->opsConfig.addrAligment.sectorCmd);
    if (returnCode)
    {
        return returnCode;
    }

    /* Validate the user key */
    returnCode = ftfx_check_user_key(key);
    if (returnCode)
    {
        return returnCode;
    }

    start = config->opsConfig.convertedAddress;
    sectorSize = config->flashDesc.sectorSize;

    /* calculating Flash end address */
    endAddress = start + lengthInBytes - 1;

    /* re-calculate the endAddress and align it to the start of the next sector
     * which will be used in the comparison below */
    if (endAddress % sectorSize)
    {
        numberOfSectors = endAddress / sectorSize + 1;
        endAddress = numberOfSectors * sectorSize - 1;
    }

    /* the start address will increment to the next sector address
     * until it reaches the endAdddress */
    while (start <= endAddress)
    {
        /* preparing passing parameter to erase a flash block */
        kFCCOBx[0] = BYTE2WORD_1_3(FTFx_ERASE_SECTOR, start);

        /* calling flash command sequence function to execute the command */
        returnCode = ftfx_command_sequence(config);

        /* checking the success of command execution */
        if (kStatus_FTFx_Success != returnCode)
        {
            break;
        }
        else
        {
            /* Increment to the next sector */
            start += sectorSize;
        }
    }

    return (returnCode);
}

status_t FTFx_CMD_EraseAll(ftfx_config_t *config, uint32_t key)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* preparing passing parameter to erase all flash blocks */
    kFCCOBx[0] = BYTE2WORD_1_3(FTFx_ERASE_ALL_BLOCK, 0xFFFFFFU);

    /* Validate the user key */
    returnCode = ftfx_check_user_key(key);
    if (returnCode)
    {
        return returnCode;
    }

    /* calling flash command sequence function to execute the command */
    returnCode = ftfx_command_sequence(config);

#if FSL_FEATURE_FLASH_HAS_FLEX_NVM
    /* Data flash IFR will be erased by erase all command, so we need to
     *  update FlexNVM memory partition status synchronously */
    if (returnCode == kStatus_FTFx_Success)
    {
        returnCode = FTFx_API_UpdateFlexnvmPartitionStatus(config);
    }
#endif

    return returnCode;
}

#if defined(FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD) && FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD
status_t FTFx_CMD_EraseAllUnsecure(ftfx_config_t *config, uint32_t key)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* Prepare passing parameter to erase all flash blocks (unsecure). */
    kFCCOBx[0] = BYTE2WORD_1_3(FTFx_ERASE_ALL_BLOCK_UNSECURE, 0xFFFFFFU);

    /* Validate the user key */
    returnCode = ftfx_check_user_key(key);
    if (returnCode)
    {
        return returnCode;
    }

    /* calling flash command sequence function to execute the command */
    returnCode = ftfx_command_sequence(config);

#if FSL_FEATURE_FLASH_HAS_FLEX_NVM
    /* Data flash IFR will be erased by erase all unsecure command, so we need to
     *  update FlexNVM memory partition status synchronously */
    if (returnCode == kStatus_FTFx_Success)
    {
        returnCode = FTFx_API_UpdateFlexnvmPartitionStatus(config);
    }
#endif

    return returnCode;
}
#endif /* FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD */

status_t FTFx_CMD_EraseAllExecuteOnlySegments(ftfx_config_t *config, uint32_t key)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* preparing passing parameter to erase all execute-only segments
     * 1st element for the FCCOB register */
    kFCCOBx[0] = BYTE2WORD_1_3(FTFx_ERASE_ALL_EXECUTE_ONLY_SEGMENT, 0xFFFFFFU);

    /* Validate the user key */
    returnCode = ftfx_check_user_key(key);
    if (returnCode)
    {
        return returnCode;
    }

    /* calling flash command sequence function to execute the command */
    returnCode = ftfx_command_sequence(config);

    return returnCode;
}

status_t FTFx_CMD_Program(ftfx_config_t *config,
                          uint32_t start,
                          uint8_t *src,
                          uint32_t lengthInBytes)
{
    status_t returnCode;
    uint8_t blockWriteUnitSize = config->opsConfig.addrAligment.blockWriteUnitSize;

    if (src == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* Check the supplied address range. */
    returnCode = ftfx_check_mem_range(config, start, lengthInBytes, blockWriteUnitSize);
    if (returnCode)
    {
        return returnCode;
    }

    start = config->opsConfig.convertedAddress;

    while (lengthInBytes > 0)
    {
        /* preparing passing parameter to program the flash block */
        kFCCOBx[1] = ftfx_read_word_from_byte_address((const uint8_t*)src);
        src += 4;

        if (4 == blockWriteUnitSize)
        {
            kFCCOBx[0] = BYTE2WORD_1_3(FTFx_PROGRAM_LONGWORD, start);
        }
        else if (8 == blockWriteUnitSize)
        {
            kFCCOBx[2] = ftfx_read_word_from_byte_address((const uint8_t*)src);
            src += 4;
            kFCCOBx[0] = BYTE2WORD_1_3(FTFx_PROGRAM_PHRASE, start);
        }
        else
        {
        }

        /* calling flash command sequence function to execute the command */
        returnCode = ftfx_command_sequence(config);

        /* checking for the success of command execution */
        if (kStatus_FTFx_Success != returnCode)
        {
            break;
        }
        else
        {
            /* update start address for next iteration */
            start += blockWriteUnitSize;

            /* update lengthInBytes for next iteration */
            lengthInBytes -= blockWriteUnitSize;
        }
    }

    return (returnCode);
}

status_t FTFx_CMD_ProgramOnce(ftfx_config_t *config, uint32_t index, uint8_t *src, uint32_t lengthInBytes)
{
    status_t returnCode;

    if ((config == NULL) || (src == NULL))
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* pass parameters to FTFx */
    kFCCOBx[0] = BYTE2WORD_1_1_2(FTFx_PROGRAM_ONCE, index, 0xFFFFU);

    kFCCOBx[1] = ftfx_read_word_from_byte_address((const uint8_t*)src);

    /* Note: Have to separate the first index from the rest if it equals 0
     * to avoid a pointless comparison of unsigned int to 0 compiler warning */
    if (config->ifrDesc.feature.has8ByteIdxSupport)
    {
        if (config->ifrDesc.feature.has4ByteIdxSupport)
        {
            if (((index == config->ifrDesc.idxInfo.mix8byteIdxStart) ||
                 ((index >= config->ifrDesc.idxInfo.mix8byteIdxStart + 1) && (index <= config->ifrDesc.idxInfo.mix8byteIdxStart))) &&
                  (lengthInBytes == 8))
            {
                kFCCOBx[2] = ftfx_read_word_from_byte_address((const uint8_t*)src + 4);
            }
        }
        else
        {
            kFCCOBx[2] = ftfx_read_word_from_byte_address((const uint8_t*)src + 4);
        }
    }

    /* calling flash command sequence function to execute the command */
    returnCode = ftfx_command_sequence(config);

    return returnCode;
}

#if defined(FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD) && FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD
status_t FTFx_CMD_ProgramSection(ftfx_config_t *config,
                                 uint32_t start,
                                 uint8_t *src,
                                 uint32_t lengthInBytes)
{
    status_t returnCode;
    uint32_t sectorSize;
    uint8_t aligmentInBytes = config->opsConfig.addrAligment.sectionCmd;
#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
    bool needSwitchFlexRamMode = false;
#endif /* FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD */

    if (src == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* Check the supplied address range. */
    returnCode = ftfx_check_mem_range(config, start, lengthInBytes, aligmentInBytes);
    if (returnCode)
    {
        return returnCode;
    }

    start = config->opsConfig.convertedAddress;
    sectorSize = config->flashDesc.sectorSize;

#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
    /* Switch function of FlexRAM if needed */
    if (!(FTFx->FCNFG & FTFx_FCNFG_RAMRDY_MASK))
    {
        needSwitchFlexRamMode = true;

        returnCode = FTFx_CMD_SetFlexramFunction(config, kFTFx_FlexramFuncOptAvailableAsRam);
        if (returnCode != kStatus_FTFx_Success)
        {
            return kStatus_FTFx_SetFlexramAsRamError;
        }
    }
#endif /* FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD */

    while (lengthInBytes > 0)
    {
        /* Make sure the write operation doesn't span two sectors */
        uint32_t endAddressOfCurrentSector = ALIGN_UP(start, sectorSize);
        uint32_t lengthTobeProgrammedOfCurrentSector;
        uint32_t currentOffset = 0;

        if (endAddressOfCurrentSector == start)
        {
            endAddressOfCurrentSector += sectorSize;
        }

        if (lengthInBytes + start > endAddressOfCurrentSector)
        {
            lengthTobeProgrammedOfCurrentSector = endAddressOfCurrentSector - start;
        }
        else
        {
            lengthTobeProgrammedOfCurrentSector = lengthInBytes;
        }

        /* Program Current Sector */
        while (lengthTobeProgrammedOfCurrentSector > 0)
        {
            /* Make sure the program size doesn't exceeds Acceleration RAM size */
            uint32_t programSizeOfCurrentPass;
            uint32_t numberOfPhases;

            if (lengthTobeProgrammedOfCurrentSector > config->flexramTotalSize)
            {
                programSizeOfCurrentPass = config->flexramTotalSize;
            }
            else
            {
                programSizeOfCurrentPass = lengthTobeProgrammedOfCurrentSector;
            }

            /* Copy data to FlexRAM */
            memcpy((void *)config->flexramBlockBase, src + currentOffset, programSizeOfCurrentPass);
            /* Set start address of the data to be programmed */
            kFCCOBx[0] = BYTE2WORD_1_3(FTFx_PROGRAM_SECTION, start + currentOffset);
            /* Set program size in terms of FEATURE_FLASH_SECTION_CMD_ADDRESS_ALIGMENT */
            numberOfPhases = programSizeOfCurrentPass / aligmentInBytes;

            kFCCOBx[1] = BYTE2WORD_2_2(numberOfPhases, 0xFFFFU);

            /* Peform command sequence */
            returnCode = ftfx_command_sequence(config);

            if (returnCode != kStatus_FTFx_Success)
            {
                return returnCode;
            }

            lengthTobeProgrammedOfCurrentSector -= programSizeOfCurrentPass;
            currentOffset += programSizeOfCurrentPass;
        }

        src += currentOffset;
        start += currentOffset;
        lengthInBytes -= currentOffset;
    }

#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
    /* Restore function of FlexRAM if needed. */
    if (needSwitchFlexRamMode)
    {
        returnCode = FTFx_CMD_SetFlexramFunction(config, kFTFx_FlexramFuncOptAvailableForEeprom);
        if (returnCode != kStatus_FTFx_Success)
        {
            return kStatus_FTFx_RecoverFlexramAsEepromError;
        }
    }
#endif /* FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD */

    return returnCode;
}
#endif /* FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_PROGRAM_PARTITION_CMD) && FSL_FEATURE_FLASH_HAS_PROGRAM_PARTITION_CMD
status_t FTFx_CMD_ProgramPartition(ftfx_config_t *config,
                                   ftfx_partition_flexram_load_opt_t option,
                                   uint32_t eepromDataSizeCode,
                                   uint32_t flexnvmPartitionCode)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* eepromDataSizeCode[7:6], flexnvmPartitionCode[7:4] should be all 1'b0
     *  or it will cause access error. */
    /* eepromDataSizeCode &= 0x3FU;  */
    /* flexnvmPartitionCode &= 0x0FU; */

    /* preparing passing parameter to program the flash block */
    kFCCOBx[0] = BYTE2WORD_1_2_1(FTFx_PROGRAM_PARTITION, 0xFFFFU, option);
    kFCCOBx[1] = BYTE2WORD_1_1_2(eepromDataSizeCode, flexnvmPartitionCode, 0xFFFFU);

    /* calling flash command sequence function to execute the command */
    returnCode = ftfx_command_sequence(config);

#if FSL_FEATURE_FLASH_HAS_FLEX_NVM
    /* Data flash IFR will be updated by program partition command during reset sequence,
     * so we just set reserved values for partitioned FlexNVM size here */
    config->eepromTotalSize = 0xFFFFU;
    config->flashDesc.totalSize = 0xFFFFFFFFU;
#endif

    return (returnCode);
}
#endif /* FSL_FEATURE_FLASH_HAS_PROGRAM_PARTITION_CMD */

status_t FTFx_CMD_ReadOnce(ftfx_config_t *config, uint32_t index, uint8_t *dst, uint32_t lengthInBytes)
{
    status_t returnCode;

    if ((config == NULL) || (dst == NULL))
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* pass parameters to FTFx */
    kFCCOBx[0] = BYTE2WORD_1_1_2(FTFx_READ_ONCE, index, 0xFFFFU);

    /* calling flash command sequence function to execute the command */
    returnCode = ftfx_command_sequence(config);

    if (kStatus_FTFx_Success == returnCode)
    {
        ftfx_write_word_to_byte_address(dst, kFCCOBx[1]);
        /* Note: Have to separate the first index from the rest if it equals 0
         *       to avoid a pointless comparison of unsigned int to 0 compiler warning */
        if (config->ifrDesc.feature.has8ByteIdxSupport)
        {
            if (config->ifrDesc.feature.has4ByteIdxSupport)
            {
                if (((index == config->ifrDesc.idxInfo.mix8byteIdxStart) ||
                     ((index >= config->ifrDesc.idxInfo.mix8byteIdxStart + 1) && (index <= config->ifrDesc.idxInfo.mix8byteIdxStart))) &&
                      (lengthInBytes == 8))
                {
                    ftfx_write_word_to_byte_address(dst + 4, kFCCOBx[2]);
                }
            }
            else
            {
                ftfx_write_word_to_byte_address(dst + 4, kFCCOBx[2]);
            }
        }
    }

    return returnCode;
}

#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
status_t FTFx_CMD_ReadResource(ftfx_config_t *config,
                               uint32_t start,
                               uint8_t *dst,
                               uint32_t lengthInBytes,
                               ftfx_read_resource_opt_t option)
{
    status_t returnCode;

    if ((config == NULL) || (dst == NULL))
    {
        return kStatus_FTFx_InvalidArgument;
    }

    uint8_t aligmentInBytes = config->opsConfig.addrAligment.resourceCmd;

    /* Check the supplied address range. */
    returnCode = ftfx_check_resource_range(config, start, lengthInBytes, aligmentInBytes, option);
    if (returnCode != kStatus_FTFx_Success)
    {
        return returnCode;
    }

    while (lengthInBytes > 0)
    {
        /* preparing passing parameter */
        kFCCOBx[0] = BYTE2WORD_1_3(FTFx_READ_RESOURCE, start);
        if (aligmentInBytes == 4)
        {
            kFCCOBx[2] = BYTE2WORD_1_3(option, 0xFFFFFFU);
        }
        else if (aligmentInBytes == 8)
        {
            kFCCOBx[1] = BYTE2WORD_1_3(option, 0xFFFFFFU);
        }
        else
        {
        }

        /* calling flash command sequence function to execute the command */
        returnCode = ftfx_command_sequence(config);

        if (kStatus_FTFx_Success != returnCode)
        {
            break;
        }

        /* fetch data */
        ftfx_write_word_to_byte_address(dst, kFCCOBx[1]);
        dst += 4;
        if (aligmentInBytes == 8)
        {
            ftfx_write_word_to_byte_address(dst, kFCCOBx[2]);
            dst += 4;
        }
        /* update start address for next iteration */
        start += aligmentInBytes;
        /* update lengthInBytes for next iteration */
        lengthInBytes -= aligmentInBytes;
    }

    return (returnCode);
}
#endif /* FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD */

status_t FTFx_CMD_VerifyErase(ftfx_config_t *config,
                              uint32_t start,
                              uint32_t lengthInBytes,
                              ftfx_margin_value_t margin)
{
    /* Check arguments. */
    uint32_t blockSize;
    uint32_t nextBlockStartAddress;
    uint32_t remainingBytes;
    uint8_t aligmentInBytes = config->opsConfig.addrAligment.sectionCmd;
    status_t returnCode;

    returnCode = ftfx_check_mem_range(config, start, lengthInBytes, aligmentInBytes);
    if (returnCode)
    {
        return returnCode;
    }

    start = config->opsConfig.convertedAddress;
    blockSize = config->flashDesc.totalSize / config->flashDesc.blockCount;

    nextBlockStartAddress = ALIGN_UP(start, blockSize);
    if (nextBlockStartAddress == start)
    {
        nextBlockStartAddress += blockSize;
    }

    remainingBytes = lengthInBytes;

    while (remainingBytes)
    {
        uint32_t numberOfPhrases;
        uint32_t verifyLength = nextBlockStartAddress - start;
        if (verifyLength > remainingBytes)
        {
            verifyLength = remainingBytes;
        }

        numberOfPhrases = verifyLength / aligmentInBytes;

        /* Fill in verify section command parameters. */
        kFCCOBx[0] = BYTE2WORD_1_3(FTFx_VERIFY_SECTION, start);
        kFCCOBx[1] = BYTE2WORD_2_1_1(numberOfPhrases, margin, 0xFFU);

        /* calling flash command sequence function to execute the command */
        returnCode = ftfx_command_sequence(config);
        if (returnCode)
        {
            return returnCode;
        }

        remainingBytes -= verifyLength;
        start += verifyLength;
        nextBlockStartAddress += blockSize;
    }

    return kStatus_FTFx_Success;
}

status_t FTFx_CMD_VerifyEraseAll(ftfx_config_t *config, ftfx_margin_value_t margin)
{
    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* preparing passing parameter to verify all block command */
    kFCCOBx[0] = BYTE2WORD_1_1_2(FTFx_VERIFY_ALL_BLOCK, margin, 0xFFFFU);

    /* calling flash command sequence function to execute the command */
    return ftfx_command_sequence(config);
}

status_t FTFx_CMD_VerifyEraseAllExecuteOnlySegments(ftfx_config_t *config, ftfx_margin_value_t margin)
{
    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* preparing passing parameter to verify erase all execute-only segments command */
    kFCCOBx[0] = BYTE2WORD_1_1_2(FTFx_VERIFY_ALL_EXECUTE_ONLY_SEGMENT, margin, 0xFFFFU);

    /* calling flash command sequence function to execute the command */
    return ftfx_command_sequence(config);
}

status_t FTFx_CMD_VerifyProgram(ftfx_config_t *config,
                                uint32_t start,
                                uint32_t lengthInBytes,
                                const uint8_t *expectedData,
                                ftfx_margin_value_t margin,
                                uint32_t *failedAddress,
                                uint32_t *failedData)
{
    status_t returnCode;
    uint8_t aligmentInBytes = config->opsConfig.addrAligment.checkCmd;
    if (expectedData == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    returnCode = ftfx_check_mem_range(config, start, lengthInBytes, aligmentInBytes);
    if (returnCode)
    {
        return returnCode;
    }

    start = config->opsConfig.convertedAddress;

    while (lengthInBytes)
    {
        /* preparing passing parameter to program check the flash block */
        kFCCOBx[0] = BYTE2WORD_1_3(FTFx_PROGRAM_CHECK, start);
        kFCCOBx[1] = BYTE2WORD_1_3(margin, 0xFFFFFFU);
        kFCCOBx[2] = ftfx_read_word_from_byte_address((const uint8_t*)expectedData);

        /* calling flash command sequence function to execute the command */
        returnCode = ftfx_command_sequence(config);

        /* checking for the success of command execution */
        if (kStatus_FTFx_Success != returnCode)
        {
            if (failedAddress)
            {
                *failedAddress = start;
            }
            if (failedData)
            {
                *failedData = 0;
            }
            break;
        }

        lengthInBytes -= aligmentInBytes;
        expectedData += aligmentInBytes;
        start += aligmentInBytes;
    }

    return (returnCode);
}

status_t FTFx_CMD_SecurityBypass(ftfx_config_t *config, const uint8_t *backdoorKey)
{
    uint8_t registerValue; /* registerValue */
    status_t returnCode;   /* return code variable */

    if ((config == NULL) || (backdoorKey == NULL))
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* set the default return code as kStatus_Success */
    returnCode = kStatus_FTFx_Success;

    /* Get flash security register value */
    registerValue = FTFx->FSEC;

    /* Check to see if flash is in secure state (any state other than 0x2)
     * If not, then skip this since flash is not secure */
    if (0x02 != (registerValue & 0x03))
    {
        /* preparing passing parameter to erase a flash block */
        kFCCOBx[0] = BYTE2WORD_1_3(FTFx_SECURITY_BY_PASS, 0xFFFFFFU);
        kFCCOBx[1] = BYTE2WORD_1_1_1_1(backdoorKey[0], backdoorKey[1], backdoorKey[2], backdoorKey[3]);
        kFCCOBx[2] = BYTE2WORD_1_1_1_1(backdoorKey[4], backdoorKey[5], backdoorKey[6], backdoorKey[7]);

        /* calling flash command sequence function to execute the command */
        returnCode = ftfx_command_sequence(config);
    }

    return (returnCode);
}

status_t FTFx_REG_GetSecurityState(ftfx_config_t *config, ftfx_security_state_t *state)
{
    /* store data read from flash register */
    uint8_t registerValue;

    if ((config == NULL) || (state == NULL))
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* Get flash security register value */
    registerValue = FTFx->FSEC;

    /* check the status of the flash security bits in the security register */
    if (kFTFx_FsecRegCode_SEC_Unsecured == (registerValue & FTFx_FSEC_SEC_MASK))
    {
        /* Flash in unsecured state */
        *state = kFTFx_SecurityStateNotSecure;
    }
    else
    {
        /* Flash in secured state
         * check for backdoor key security enable bit */
        if (kFTFx_FsecRegCode_KEYEN_Enabled == (registerValue & FTFx_FSEC_KEYEN_MASK))
        {
            /* Backdoor key security enabled */
            *state = kFTFx_SecurityStateBackdoorEnabled;
        }
        else
        {
            /* Backdoor key security disabled */
            *state = kFTFx_SecurityStateBackdoorDisabled;
        }
    }

    return (kStatus_FTFx_Success);
}

#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
 status_t FTFx_CMD_SetFlexramFunction(ftfx_config_t *config, ftfx_flexram_func_opt_t option)
 {
     status_t status;

     if (config == NULL)
     {
         return kStatus_FTFx_InvalidArgument;
     }

     status = ftfx_check_flexram_function_option(option);
     if (status != kStatus_FTFx_Success)
     {
         return status;
     }

     /* preparing passing parameter to verify all block command */
     kFCCOBx[0] = BYTE2WORD_1_1_2(FTFx_SET_FLEXRAM_FUNCTION, option, 0xFFFFU);

     /* calling flash command sequence function to execute the command */
     return ftfx_command_sequence(config);
 }
#endif /* FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD) && FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD
status_t FTFx_CMD_SwapControl(ftfx_config_t *config,
                              uint32_t address,
                              ftfx_swap_control_opt_t option,
                              ftfx_swap_state_config_t *returnInfo)
{
    status_t returnCode;

    if ((config == NULL) || (returnInfo == NULL))
    {
        return kStatus_FTFx_InvalidArgument;
    }

    if (address & (FSL_FEATURE_FLASH_PFLASH_SWAP_CONTROL_CMD_ADDRESS_ALIGMENT - 1))
    {
        return kStatus_FTFx_AlignmentError;
    }

    /* Make sure address provided is in the lower half of Program flash but not in the Flash Configuration Field */
    if ((address >= (config->flashDesc.totalSize / 2)) ||
        ((address >= kFTFx_PflashConfigAreaStart) && (address <= kFTFx_PflashConfigAreaEnd)))
    {
        return kStatus_FTFx_SwapIndicatorAddressError;
    }

    /* Check the option. */
    returnCode = ftfx_check_swap_control_option(option);
    if (returnCode)
    {
        return returnCode;
    }

    kFCCOBx[0] = BYTE2WORD_1_3(FTFx_SWAP_CONTROL, address);
    kFCCOBx[1] = BYTE2WORD_1_3(option, 0xFFFFFFU);

    returnCode = ftfx_command_sequence(config);

    returnInfo->flashSwapState = (ftfx_swap_state_t)FTFx_FCCOB5_REG;
    returnInfo->currentSwapBlockStatus = (ftfx_swap_block_status_t)FTFx_FCCOB6_REG;
    returnInfo->nextSwapBlockStatus = (ftfx_swap_block_status_t)FTFx_FCCOB7_REG;

    return returnCode;
}
#endif /* FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD */

static status_t ftfx_init_ifr(ftfx_config_t *config)
{
    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

#if FSL_FEATURE_FLASH_IS_FTFA
    /* FTFA parts(eg. K80, KL80, L5K) support both 4-bytes and 8-bytes unit size */
    config->ifrDesc.feature.has4ByteIdxSupport = 1;
    config->ifrDesc.feature.has8ByteIdxSupport = 1;
    config->ifrDesc.idxInfo.mix8byteIdxStart = 0x10U;
    config->ifrDesc.idxInfo.mix8byteIdxEnd = 0x13U;
#elif FSL_FEATURE_FLASH_IS_FTFE
    /* FTFE parts(eg. K65, KE18) only support 8-bytes unit size */
    config->ifrDesc.feature.has4ByteIdxSupport = 0;
    config->ifrDesc.feature.has8ByteIdxSupport = 1;
#elif FSL_FEATURE_FLASH_IS_FTFL
    /* FTFL parts(eg. K20) only support 4-bytes unit size */
    config->ifrDesc.feature.has4ByteIdxSupport = 1;
    config->ifrDesc.feature.has8ByteIdxSupport = 0;
#endif

    config->ifrDesc.resRange.pflashIfrStart = 0x0000U;
    config->ifrDesc.resRange.versionIdSize = 0x08U;
#if FSL_FEATURE_FLASH_IS_FTFE
    config->ifrDesc.resRange.versionIdStart = 0x08U;
    config->ifrDesc.resRange.ifrMemSize = 0x0400U;
#else /* FSL_FEATURE_FLASH_IS_FTFL == 1 or FSL_FEATURE_FLASH_IS_FTFA = =1 */
    config->ifrDesc.resRange.versionIdStart = 0x00U;
    config->ifrDesc.resRange.ifrMemSize = 0x0100U;
#endif

#if FSL_FEATURE_FLASH_HAS_FLEX_NVM
    config->ifrDesc.resRange.dflashIfrStart = 0x800000U;
#endif

#if FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD
#if FSL_FEATURE_FLASH_IS_FTFE
    config->ifrDesc.resRange.pflashSwapIfrStart = 0x40000U;
#else /* FSL_FEATURE_FLASH_IS_FTFL == 1 or FSL_FEATURE_FLASH_IS_FTFA == 1 */
    config->ifrDesc.resRange.pflashSwapIfrStart = config->flashDesc.totalSize / 4;
#endif
#endif /* FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD */

    return kStatus_FTFx_Success;
}

#if FTFx_DRIVER_IS_FLASH_RESIDENT
/*!
 * @brief Copy PIC of flash_run_command() to RAM
 */
static void ftfx_copy_run_command_to_ram(uint32_t *ftfxRunCommand)
{
    assert(sizeof(s_ftfxRunCommandFunctionCode) <= (kFTFx_RamFuncMaxSizeInWords * 4));

    /* Since the value of ARM function pointer is always odd, but the real start address
     * of function memory should be even, that's why +1 operation exist. */
    memcpy((uint8_t *)ftfxRunCommand, (const uint8_t *)s_ftfxRunCommandFunctionCode, sizeof(s_ftfxRunCommandFunctionCode));
}
#endif /* FTFx_DRIVER_IS_FLASH_RESIDENT */

/*!
 * @brief FTFx Command Sequence
 *
 * This function is used to perform the command write sequence to the flash.
 *
 * @param driver Pointer to storage for the driver runtime state.
 * @return An error code or kStatus_FTFx_Success
 */
static status_t ftfx_command_sequence(ftfx_config_t *config)
{
    uint8_t registerValue;

#if FTFx_DRIVER_IS_FLASH_RESIDENT
    /* clear RDCOLERR & ACCERR & FPVIOL flag in flash status register */
    FTFx->FSTAT = FTFx_FSTAT_RDCOLERR_MASK | FTFx_FSTAT_ACCERR_MASK | FTFx_FSTAT_FPVIOL_MASK;

    /* Since the value of ARM function pointer is always odd, but the real start address
     * of function memory should be even, that's why +1 operation exist. */
    callFtfxRunCommand_t callFtfxRunCommand = (callFtfxRunCommand_t)((uint32_t)config->runCmdFuncAddr + 1);

    /* We pass the ftfx_fstat address as a parameter to flash_run_comamnd() instead of using
     * pre-processed MICRO sentences or operating global variable in flash_run_comamnd()
     * to make sure that flash_run_command() will be compiled into position-independent code (PIC). */
    callFtfxRunCommand((FTFx_REG8_ACCESS_TYPE)(&FTFx->FSTAT));
#else
    /* clear RDCOLERR & ACCERR & FPVIOL flag in flash status register */
    FTFx->FSTAT = FTFx_FSTAT_RDCOLERR_MASK | FTFx_FSTAT_ACCERR_MASK | FTFx_FSTAT_FPVIOL_MASK;

    /* clear CCIF bit */
    FTFx->FSTAT = FTFx_FSTAT_CCIF_MASK;

    /* Check CCIF bit of the flash status register, wait till it is set.
     * IP team indicates that this loop will always complete. */
    while (!(FTFx->FSTAT & FTFx_FSTAT_CCIF_MASK))
    {
    }
#endif /* FTFx_DRIVER_IS_FLASH_RESIDENT */

    /* Check error bits */
    /* Get flash status register value */
    registerValue = FTFx->FSTAT;

    /* checking access error */
    if (registerValue & FTFx_FSTAT_ACCERR_MASK)
    {
        return kStatus_FTFx_AccessError;
    }
    /* checking protection error */
    else if (registerValue & FTFx_FSTAT_FPVIOL_MASK)
    {
        return kStatus_FTFx_ProtectionViolation;
    }
    /* checking MGSTAT0 non-correctable error */
    else if (registerValue & FTFx_FSTAT_MGSTAT0_MASK)
    {
        return kStatus_FTFx_CommandFailure;
    }
    else
    {
        return kStatus_FTFx_Success;
    }
}

/*! @brief Validates the range and alignment of the given address range.*/
static status_t ftfx_check_mem_range(ftfx_config_t *config,
                                     uint32_t startAddress,
                                     uint32_t lengthInBytes,
                                     uint8_t alignmentBaseline)
{
    if (config == NULL)
    {
        return kStatus_FTFx_InvalidArgument;
    }

    /* Verify the start and length are alignmentBaseline aligned. */
    if ((startAddress & (alignmentBaseline - 1)) || (lengthInBytes & (alignmentBaseline - 1)))
    {
        return kStatus_FTFx_AlignmentError;
    }

    /* check for valid range of the target addresses */
    if ((startAddress >= config->flashDesc.blockBase) &&
        ((startAddress + lengthInBytes) <= (config->flashDesc.blockBase + config->flashDesc.totalSize)))
    {
        return kStatus_FTFx_Success;
    }

    return kStatus_FTFx_AddressError;
}

/*! @brief Validates the given user key for flash erase APIs.*/
static status_t ftfx_check_user_key(uint32_t key)
{
    /* Validate the user key */
    if (key != kFTFx_ApiEraseKey)
    {
        return kStatus_FTFx_EraseKeyError;
    }

    return kStatus_FTFx_Success;
}

/*! @brief Reads word from byte address.*/
static uint32_t ftfx_read_word_from_byte_address(const uint8_t *src)
{
    uint32_t word = 0;

    if (!((uint32_t)src % 4))
    {
        word = *(const uint32_t *)src;
    }
    else
    {
        for (uint32_t i = 0; i < 4; i++)
        {
            word |= (uint32_t)(*src) << (i * 8);
            src++;
        }
    }

   return word;
}

/*! @brief Writes word to byte address.*/
static void ftfx_write_word_to_byte_address(uint8_t *dst, uint32_t word)
{
    if (!((uint32_t)dst % 4))
    {
        *(uint32_t *)dst = word;
    }
    else
    {
        for (uint32_t i = 0; i < 4; i++)
        {
            *dst = (uint8_t)((word >> (i * 8)) & 0xFFU);
            dst++;
        }
    }
}

#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
/*! @brief Validates the range of the given resource address.*/
static status_t ftfx_check_resource_range(ftfx_config_t *config,
                                          uint32_t start,
                                          uint32_t lengthInBytes,
                                          uint32_t alignmentBaseline,
                                          ftfx_read_resource_opt_t option)
{
    status_t status;
    uint32_t maxReadbleAddress;

    if ((start & (alignmentBaseline - 1)) || (lengthInBytes & (alignmentBaseline - 1)))
    {
        return kStatus_FTFx_AlignmentError;
    }

    status = kStatus_FTFx_Success;

    maxReadbleAddress = start + lengthInBytes - 1;
    if (option == kFTFx_ResourceOptionVersionId)
    {
        if ((start != config->ifrDesc.resRange.versionIdStart) ||
            (lengthInBytes != config->ifrDesc.resRange.versionIdSize))
        {
            status = kStatus_FTFx_InvalidArgument;
        }
    }
    else if (option == kFTFx_ResourceOptionFlashIfr)
    {
        if ((start >= config->ifrDesc.resRange.pflashIfrStart) &&
            (maxReadbleAddress < (config->ifrDesc.resRange.pflashIfrStart + config->ifrDesc.resRange.ifrMemSize)))
        {
        }
#if FSL_FEATURE_FLASH_HAS_FLEX_NVM
        else if ((start >= config->ifrDesc.resRange.dflashIfrStart) &&
                 (maxReadbleAddress < (config->ifrDesc.resRange.dflashIfrStart + config->ifrDesc.resRange.ifrMemSize)))
        {
        }
#endif
#if FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD
        else if ((start >= config->ifrDesc.resRange.pflashSwapIfrStart) &&
                 (maxReadbleAddress < (config->ifrDesc.resRange.pflashSwapIfrStart + config->ifrDesc.resRange.ifrMemSize)))
        {
        }
#endif
        else
        {
            status = kStatus_FTFx_InvalidArgument;
        }
    }
    else
    {
        status = kStatus_FTFx_InvalidArgument;
    }

    return status;
}
#endif /* FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
/*! @brief Validates the given flexram function option.*/
static inline status_t ftfx_check_flexram_function_option(ftfx_flexram_func_opt_t option)
{
    if ((option != kFTFx_FlexramFuncOptAvailableAsRam) &&
        (option != kFTFx_FlexramFuncOptAvailableForEeprom))
    {
        return kStatus_FTFx_InvalidArgument;
    }

    return kStatus_FTFx_Success;
}
#endif /* FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD) && FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD
/*! @brief Validates the given swap control option.*/
static status_t ftfx_check_swap_control_option(ftfx_swap_control_opt_t option)
{
    if ((option == kFTFx_SwapControlOptionIntializeSystem) || (option == kFTFx_SwapControlOptionSetInUpdateState) ||
        (option == kFTFx_SwapControlOptionSetInCompleteState) || (option == kFTFx_SwapControlOptionReportStatus) ||
        (option == kFTFx_SwapControlOptionDisableSystem))
    {
        return kStatus_FTFx_Success;
    }

    return kStatus_FTFx_InvalidArgument;
}
#endif /* FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD */
