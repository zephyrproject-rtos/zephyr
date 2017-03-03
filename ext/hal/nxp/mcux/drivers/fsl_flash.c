/*
 * Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
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

#include "fsl_flash.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @name Misc utility defines
 * @{
 */
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a) ((x) & (uint32_t)(-((int32_t)(a))))
#endif
#ifndef ALIGN_UP
#define ALIGN_UP(x, a) (-((int32_t)((uint32_t)(-((int32_t)(x))) & (uint32_t)(-((int32_t)(a))))))
#endif

#define BYTES_JOIN_TO_WORD_1_3(x, y) ((((uint32_t)(x)&0xFFU) << 24) | ((uint32_t)(y)&0xFFFFFFU))
#define BYTES_JOIN_TO_WORD_2_2(x, y) ((((uint32_t)(x)&0xFFFFU) << 16) | ((uint32_t)(y)&0xFFFFU))
#define BYTES_JOIN_TO_WORD_3_1(x, y) ((((uint32_t)(x)&0xFFFFFFU) << 8) | ((uint32_t)(y)&0xFFU))
#define BYTES_JOIN_TO_WORD_1_1_2(x, y, z) \
    ((((uint32_t)(x)&0xFFU) << 24) | (((uint32_t)(y)&0xFFU) << 16) | ((uint32_t)(z)&0xFFFFU))
#define BYTES_JOIN_TO_WORD_1_2_1(x, y, z) \
    ((((uint32_t)(x)&0xFFU) << 24) | (((uint32_t)(y)&0xFFFFU) << 8) | ((uint32_t)(z)&0xFFU))
#define BYTES_JOIN_TO_WORD_2_1_1(x, y, z) \
    ((((uint32_t)(x)&0xFFFFU) << 16) | (((uint32_t)(y)&0xFFU) << 8) | ((uint32_t)(z)&0xFFU))
#define BYTES_JOIN_TO_WORD_1_1_1_1(x, y, z, w)                                                      \
    ((((uint32_t)(x)&0xFFU) << 24) | (((uint32_t)(y)&0xFFU) << 16) | (((uint32_t)(z)&0xFFU) << 8) | \
     ((uint32_t)(w)&0xFFU))
/*@}*/

/*! @brief Data flash IFR map Field*/
#if defined(FSL_FEATURE_FLASH_IS_FTFE) && FSL_FEATURE_FLASH_IS_FTFE
#define DFLASH_IFR_READRESOURCE_START_ADDRESS 0x8003F8U
#else /* FSL_FEATURE_FLASH_IS_FTFL == 1 or FSL_FEATURE_FLASH_IS_FTFA = =1 */
#define DFLASH_IFR_READRESOURCE_START_ADDRESS 0x8000F8U
#endif

/*!
 * @name Reserved FlexNVM size (For a variety of purposes) defines
 * @{
 */
#define FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED 0xFFFFFFFFU
#define FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_RESERVED 0xFFFFU
/*@}*/

/*!
 * @name Flash Program Once Field defines
 * @{
 */
#if defined(FSL_FEATURE_FLASH_IS_FTFA) && FSL_FEATURE_FLASH_IS_FTFA
/* FTFA parts(eg. K80, KL80, L5K) support both 4-bytes and 8-bytes unit size */
#define FLASH_PROGRAM_ONCE_MIN_ID_8BYTES \
    0x10U /* Minimum Index indcating one of Progam Once Fields which is accessed in 8-byte records */
#define FLASH_PROGRAM_ONCE_MAX_ID_8BYTES \
    0x13U /* Maximum Index indcating one of Progam Once Fields which is accessed in 8-byte records */
#define FLASH_PROGRAM_ONCE_IS_4BYTES_UNIT_SUPPORT 1
#define FLASH_PROGRAM_ONCE_IS_8BYTES_UNIT_SUPPORT 1
#elif defined(FSL_FEATURE_FLASH_IS_FTFE) && FSL_FEATURE_FLASH_IS_FTFE
/* FTFE parts(eg. K65, KE18) only support 8-bytes unit size */
#define FLASH_PROGRAM_ONCE_IS_4BYTES_UNIT_SUPPORT 0
#define FLASH_PROGRAM_ONCE_IS_8BYTES_UNIT_SUPPORT 1
#elif defined(FSL_FEATURE_FLASH_IS_FTFL) && FSL_FEATURE_FLASH_IS_FTFL
/* FTFL parts(eg. K20) only support 4-bytes unit size */
#define FLASH_PROGRAM_ONCE_IS_4BYTES_UNIT_SUPPORT 1
#define FLASH_PROGRAM_ONCE_IS_8BYTES_UNIT_SUPPORT 0
#endif
/*@}*/

/*!
 * @name Flash security status defines
 * @{
 */
#define FLASH_SECURITY_STATE_KEYEN 0x80U
#define FLASH_SECURITY_STATE_UNSECURED 0x02U
#define FLASH_NOT_SECURE 0x01U
#define FLASH_SECURE_BACKDOOR_ENABLED 0x02U
#define FLASH_SECURE_BACKDOOR_DISABLED 0x04U
/*@}*/

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
#define FTFx_PROGRAM_PARTITION 0x80U               /*!< PGMPART)*/
#define FTFx_SET_FLEXRAM_FUNCTION 0x81U            /*!< SETRAM*/
                                                   /*@}*/

/*!
 * @name Common flash register info defines
 * @{
 */
#if defined(FTFA)
#define FTFx FTFA
#define FTFx_BASE FTFA_BASE
#define FTFx_FSTAT_CCIF_MASK FTFA_FSTAT_CCIF_MASK
#define FTFx_FSTAT_RDCOLERR_MASK FTFA_FSTAT_RDCOLERR_MASK
#define FTFx_FSTAT_ACCERR_MASK FTFA_FSTAT_ACCERR_MASK
#define FTFx_FSTAT_FPVIOL_MASK FTFA_FSTAT_FPVIOL_MASK
#define FTFx_FSTAT_MGSTAT0_MASK FTFA_FSTAT_MGSTAT0_MASK
#define FTFx_FSEC_SEC_MASK FTFA_FSEC_SEC_MASK
#define FTFx_FSEC_KEYEN_MASK FTFA_FSEC_KEYEN_MASK
#if defined(FSL_FEATURE_FLASH_HAS_FLEX_RAM) && FSL_FEATURE_FLASH_HAS_FLEX_RAM
#define FTFx_FCNFG_RAMRDY_MASK FTFA_FCNFG_RAMRDY_MASK
#endif /* FSL_FEATURE_FLASH_HAS_FLEX_RAM */
#if defined(FSL_FEATURE_FLASH_HAS_FLEX_NVM) && FSL_FEATURE_FLASH_HAS_FLEX_NVM
#define FTFx_FCNFG_EEERDY_MASK FTFA_FCNFG_EEERDY_MASK
#endif /* FSL_FEATURE_FLASH_HAS_FLEX_NVM */
#elif defined(FTFE)
#define FTFx FTFE
#define FTFx_BASE FTFE_BASE
#define FTFx_FSTAT_CCIF_MASK FTFE_FSTAT_CCIF_MASK
#define FTFx_FSTAT_RDCOLERR_MASK FTFE_FSTAT_RDCOLERR_MASK
#define FTFx_FSTAT_ACCERR_MASK FTFE_FSTAT_ACCERR_MASK
#define FTFx_FSTAT_FPVIOL_MASK FTFE_FSTAT_FPVIOL_MASK
#define FTFx_FSTAT_MGSTAT0_MASK FTFE_FSTAT_MGSTAT0_MASK
#define FTFx_FSEC_SEC_MASK FTFE_FSEC_SEC_MASK
#define FTFx_FSEC_KEYEN_MASK FTFE_FSEC_KEYEN_MASK
#if defined(FSL_FEATURE_FLASH_HAS_FLEX_RAM) && FSL_FEATURE_FLASH_HAS_FLEX_RAM
#define FTFx_FCNFG_RAMRDY_MASK FTFE_FCNFG_RAMRDY_MASK
#endif /* FSL_FEATURE_FLASH_HAS_FLEX_RAM */
#if defined(FSL_FEATURE_FLASH_HAS_FLEX_NVM) && FSL_FEATURE_FLASH_HAS_FLEX_NVM
#define FTFx_FCNFG_EEERDY_MASK FTFE_FCNFG_EEERDY_MASK
#endif /* FSL_FEATURE_FLASH_HAS_FLEX_NVM */
#elif defined(FTFL)
#define FTFx FTFL
#define FTFx_BASE FTFL_BASE
#define FTFx_FSTAT_CCIF_MASK FTFL_FSTAT_CCIF_MASK
#define FTFx_FSTAT_RDCOLERR_MASK FTFL_FSTAT_RDCOLERR_MASK
#define FTFx_FSTAT_ACCERR_MASK FTFL_FSTAT_ACCERR_MASK
#define FTFx_FSTAT_FPVIOL_MASK FTFL_FSTAT_FPVIOL_MASK
#define FTFx_FSTAT_MGSTAT0_MASK FTFL_FSTAT_MGSTAT0_MASK
#define FTFx_FSEC_SEC_MASK FTFL_FSEC_SEC_MASK
#define FTFx_FSEC_KEYEN_MASK FTFL_FSEC_KEYEN_MASK
#if defined(FSL_FEATURE_FLASH_HAS_FLEX_RAM) && FSL_FEATURE_FLASH_HAS_FLEX_RAM
#define FTFx_FCNFG_RAMRDY_MASK FTFL_FCNFG_RAMRDY_MASK
#endif /* FSL_FEATURE_FLASH_HAS_FLEX_RAM */
#if defined(FSL_FEATURE_FLASH_HAS_FLEX_NVM) && FSL_FEATURE_FLASH_HAS_FLEX_NVM
#define FTFx_FCNFG_EEERDY_MASK FTFL_FCNFG_EEERDY_MASK
#endif /* FSL_FEATURE_FLASH_HAS_FLEX_NVM */
#else
#error "Unknown flash controller"
#endif
/*@}*/

/*!
 * @name Common flash register access info defines
 * @{
 */
#if defined(FTFA_FCCOB_CCOBn_MASK) || defined(FTFE_FCCOB_CCOBn_MASK) || defined(FTFL_FCCOB_CCOBn_MASK)
#define FTFx_FCCOB3_REG (FTFx->FCCOB[0])
#define FTFx_FCCOB5_REG (FTFx->FCCOB[6])
#define FTFx_FCCOB6_REG (FTFx->FCCOB[5])
#define FTFx_FCCOB7_REG (FTFx->FCCOB[4])
#else
#define FTFx_FCCOB3_REG (FTFx->FCCOB3)
#define FTFx_FCCOB5_REG (FTFx->FCCOB5)
#define FTFx_FCCOB6_REG (FTFx->FCCOB6)
#define FTFx_FCCOB7_REG (FTFx->FCCOB7)
#endif

#if defined(FTFA_FPROT_PROT_MASK) || defined(FTFE_FPROT_PROT_MASK) || defined(FTFL_FPROT_PROT_MASK)
#define FTFx_FPROT_LOW_REG (FTFx->FPROT[4])
#define FTFx_FPROTL3_REG (FTFx->FPROT[4])
#define FTFx_FPROTL2_REG (FTFx->FPROT[5])
#define FTFx_FPROTL1_REG (FTFx->FPROT[6])
#define FTFx_FPROTL0_REG (FTFx->FPROT[7])
#define FTFx_FPROT_HIGH_REG (FTFx->FPROT[0])
#define FTFx_FPROTH3_REG (FTFx->FPROT[0])
#define FTFx_FPROTH2_REG (FTFx->FPROT[1])
#define FTFx_FPROTH1_REG (FTFx->FPROT[2])
#define FTFx_FPROTH0_REG (FTFx->FPROT[3])
#else
#define FTFx_FPROT_LOW_REG (FTFx->FPROT3)
#define FTFx_FPROTL3_REG (FTFx->FPROT3)
#define FTFx_FPROTL2_REG (FTFx->FPROT2)
#define FTFx_FPROTL1_REG (FTFx->FPROT1)
#define FTFx_FPROTL0_REG (FTFx->FPROT0)
#endif

#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED && FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_PROTECTION_REGISTER
#if defined(FTFA_FPROTS_PROTS_MASK) || defined(FTFE_FPROTS_PROTS_MASK) || defined(FTFL_FPROTS_PROTS_MASK)
#define FTFx_FPROTSH_REG (FTFx->FPROTS[1])
#define FTFx_FPROTSL_REG (FTFx->FPROTS[0])
#else
#define FTFx_FPROTSH_REG (FTFx->FPROTSH)
#define FTFx_FPROTSL_REG (FTFx->FPROTSL)
#endif
#endif

#if defined(FTFA_XACC_XA_MASK) || defined(FTFE_XACC_XA_MASK) || defined(FTFL_XACC_XA_MASK)
#define FTFx_XACCH3_REG (FTFx->XACC[0])
#define FTFx_XACCL3_REG (FTFx->XACC[4])
#else
#define FTFx_XACCH3_REG (FTFx->XACCH3)
#define FTFx_XACCL3_REG (FTFx->XACCL3)
#endif

#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED && FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_ACCESS_REGISTER
#if defined(FTFA_XACCS_XA_S_MASK) || defined(FTFE_XACCS_XA_S_MASK) || defined(FTFL_XACCS_XA_S_MASK)
#define FTFx_XACCSH_REG (FTFx->XACCS[1])
#define FTFx_XACCSL_REG (FTFx->XACCS[0])
#else
#define FTFx_XACCSH_REG (FTFx->XACCSH)
#define FTFx_XACCSL_REG (FTFx->XACCSL)
#endif
#endif
/*@}*/

/*!
 * @brief Enumeration for access segment property.
 */
enum _flash_access_segment_property
{
    kFLASH_AccessSegmentBase = 256UL,
};

/*!
 * @brief Enumeration for flash config area.
 */
enum _flash_config_area_range
{
    kFLASH_ConfigAreaStart = 0x400U,
    kFLASH_ConfigAreaEnd = 0x40FU
};

/*!
 * @name Flash register access type defines
 * @{
 */
#define FTFx_REG8_ACCESS_TYPE volatile uint8_t *
#define FTFx_REG32_ACCESS_TYPE volatile uint32_t *
/*@}*/

/*!
 * @brief MSCM prefetch speculation defines.
 */
#define MSCM_OCMDR_OCMC1_DFDS_MASK (0x10U)
#define MSCM_OCMDR_OCMC1_DFCS_MASK (0x20U)

#define MSCM_OCMDR_OCMC1_DFDS_SHIFT (4U)
#define MSCM_OCMDR_OCMC1_DFCS_SHIFT (5U)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

#if FLASH_DRIVER_IS_FLASH_RESIDENT
/*! @brief Copy flash_run_command() to RAM*/
static void copy_flash_run_command(uint32_t *flashRunCommand);
/*! @brief Copy flash_cache_clear_command() to RAM*/
static void copy_flash_common_bit_operation(uint32_t *flashCommonBitOperation);
/*! @brief Check whether flash execute-in-ram functions are ready*/
static status_t flash_check_execute_in_ram_function_info(flash_config_t *config);
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

/*! @brief Internal function Flash command sequence. Called by driver APIs only*/
static status_t flash_command_sequence(flash_config_t *config);

/*! @brief Perform the cache clear to the flash*/
void flash_cache_clear(flash_config_t *config);

/*! @brief Validates the range and alignment of the given address range.*/
static status_t flash_check_range(flash_config_t *config,
                                  uint32_t startAddress,
                                  uint32_t lengthInBytes,
                                  uint32_t alignmentBaseline);
/*! @brief Gets the right address, sector and block size of current flash type which is indicated by address.*/
static status_t flash_get_matched_operation_info(flash_config_t *config,
                                                 uint32_t address,
                                                 flash_operation_config_t *info);
/*! @brief Validates the given user key for flash erase APIs.*/
static status_t flash_check_user_key(uint32_t key);

#if FLASH_SSD_IS_FLEXNVM_ENABLED
/*! @brief Updates FlexNVM memory partition status according to data flash 0 IFR.*/
static status_t flash_update_flexnvm_memory_partition_status(flash_config_t *config);
#endif /* FLASH_SSD_IS_FLEXNVM_ENABLED */

#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
/*! @brief Validates the range of the given resource address.*/
static status_t flash_check_resource_range(uint32_t start,
                                           uint32_t lengthInBytes,
                                           uint32_t alignmentBaseline,
                                           flash_read_resource_option_t option);
#endif /* FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD) && FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD
/*! @brief Validates the gived swap control option.*/
static status_t flash_check_swap_control_option(flash_swap_control_option_t option);
#endif /* FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP) && FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP
/*! @brief Validates the gived address to see if it is equal to swap indicator address in pflash swap IFR.*/
static status_t flash_validate_swap_indicator_address(flash_config_t *config, uint32_t address);
#endif /* FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP */

#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
/*! @brief Validates the gived flexram function option.*/
static inline status_t flasn_check_flexram_function_option_range(flash_flexram_function_option_t option);
#endif /* FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD */

/*! @brief Gets the flash protection information (region size, region count).*/
static status_t flash_get_protection_info(flash_config_t *config, flash_protection_config_t *info);

#if defined(FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL) && FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL
/*! @brief Gets the flash Execute-Only access information (Segment size, Segment count).*/
static status_t flash_get_access_info(flash_config_t *config, flash_access_config_t *info);
#endif /* FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL */

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Access to FTFx->FCCOB */
volatile uint32_t *const kFCCOBx = (volatile uint32_t *)&FTFx_FCCOB3_REG;
/*! @brief Access to FTFx->FPROT */
volatile uint32_t *const kFPROTL = (volatile uint32_t *)&FTFx_FPROT_LOW_REG;
#if defined(FTFx_FPROT_HIGH_REG)
volatile uint32_t *const kFPROTH = (volatile uint32_t *)&FTFx_FPROT_HIGH_REG;
#endif

#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED && FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_PROTECTION_REGISTER
volatile uint8_t *const kFPROTSL = (volatile uint8_t *)&FTFx_FPROTSL_REG;
volatile uint8_t *const kFPROTSH = (volatile uint8_t *)&FTFx_FPROTSH_REG;
#endif

#if FLASH_DRIVER_IS_FLASH_RESIDENT
/*! @brief A function pointer used to point to relocated flash_run_command() */
static void (*callFlashRunCommand)(FTFx_REG8_ACCESS_TYPE ftfx_fstat);
/*! @brief A function pointer used to point to relocated flash_common_bit_operation() */
static void (*callFlashCommonBitOperation)(FTFx_REG32_ACCESS_TYPE base,
                                           uint32_t bitMask,
                                           uint32_t bitShift,
                                           uint32_t bitValue);

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
const static uint16_t s_flashRunCommandFunctionCode[] = {
    0x2180, /* MOVS  R1, #128 ; 0x80 */
    0x7001, /* STRB  R1, [R0] */
    /* @4: */
    0x7802, /* LDRB  R2, [R0] */
    0x420a, /* TST   R2, R1 */
    0xd0fc, /* BEQ.N @4 */
    0x4770  /* BX    LR */
};

/*!
 * @brief Position independent code of flash_common_bit_operation()
 *
 * Note1: The prototype of C function is shown as below:
 * @code
 *   void flash_common_bit_operation(FTFx_REG32_ACCESS_TYPE base, uint32_t bitMask, uint32_t bitShift, uint32_t
 * bitValue)
 *   {
 *       if (bitMask)
 *       {
 *           uint32_t value = (((uint32_t)(((uint32_t)(bitValue)) << bitShift)) & bitMask);
 *           *base = (*base & (~bitMask)) | value;
 *       }
 *
 *       __ISB();
 *       __DSB();
 *   }
 * @endcode
 * Note2: The binary code is generated by IAR 7.70.1
 */
const static uint16_t s_flashCommonBitOperationFunctionCode[] = {
    0xb510, /* PUSH  {R4, LR} */
    0x2900, /* CMP   R1, #0 */
    0xd005, /* BEQ.N @12 */
    0x6804, /* LDR   R4, [R0] */
    0x438c, /* BICS  R4, R4, R1 */
    0x4093, /* LSLS  R3, R3, R2 */
    0x4019, /* ANDS  R1, R1, R3 */
    0x4321, /* ORRS  R1, R1, R4 */
    0x6001, /* STR   R1, [R0] */
    /*  @12: */
    0xf3bf, 0x8f6f, /* ISB */
    0xf3bf, 0x8f4f, /* DSB */
    0xbd10          /* POP   {R4, PC} */
};
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

#if (FLASH_DRIVER_IS_FLASH_RESIDENT && !FLASH_DRIVER_IS_EXPORTED)
/*! @brief A static buffer used to hold flash_run_command() */
static uint32_t s_flashRunCommand[kFLASH_ExecuteInRamFunctionMaxSizeInWords];
/*! @brief A static buffer used to hold flash_common_bit_operation() */
static uint32_t s_flashCommonBitOperation[kFLASH_ExecuteInRamFunctionMaxSizeInWords];
/*! @brief Flash execute-in-ram function information */
static flash_execute_in_ram_function_config_t s_flashExecuteInRamFunctionInfo;
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
const uint16_t kPFlashDensities[] = {
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

/*******************************************************************************
 * Code
 ******************************************************************************/

status_t FLASH_Init(flash_config_t *config)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED
    if (config->FlashMemoryIndex == (uint32_t)kFLASH_MemoryIndexSecondaryFlash)
    {
/* calculate the flash density from SIM_FCFG1.PFSIZE */
#if defined(SIM_FCFG1_CORE1_PFSIZE_MASK)
        uint32_t flashDensity;
        uint8_t pfsize = (SIM->FCFG1 & SIM_FCFG1_CORE1_PFSIZE_MASK) >> SIM_FCFG1_CORE1_PFSIZE_SHIFT;
        if (pfsize == 0xf)
        {
            flashDensity = FSL_FEATURE_FLASH_PFLASH_1_BLOCK_COUNT * FSL_FEATURE_FLASH_PFLASH_1_BLOCK_SIZE;
        }
        else
        {
            flashDensity = ((uint32_t)kPFlashDensities[pfsize]) << 10;
        }
        config->PFlashTotalSize = flashDensity;
#else
        /* Unused code to solve MISRA-C issue*/
        config->PFlashBlockBase = kPFlashDensities[0];
        config->PFlashTotalSize = FSL_FEATURE_FLASH_PFLASH_1_BLOCK_COUNT * FSL_FEATURE_FLASH_PFLASH_1_BLOCK_SIZE;
#endif
        config->PFlashBlockBase = FSL_FEATURE_FLASH_PFLASH_1_START_ADDRESS;
        config->PFlashBlockCount = FSL_FEATURE_FLASH_PFLASH_1_BLOCK_COUNT;
        config->PFlashSectorSize = FSL_FEATURE_FLASH_PFLASH_1_BLOCK_SECTOR_SIZE;
    }
    else
#endif /* FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED */
    {
        uint32_t flashDensity;

/* calculate the flash density from SIM_FCFG1.PFSIZE */
#if defined(SIM_FCFG1_CORE0_PFSIZE_MASK)
        uint8_t pfsize = (SIM->FCFG1 & SIM_FCFG1_CORE0_PFSIZE_MASK) >> SIM_FCFG1_CORE0_PFSIZE_SHIFT;
#elif defined(SIM_FCFG1_PFSIZE_MASK)
        uint8_t pfsize = (SIM->FCFG1 & SIM_FCFG1_PFSIZE_MASK) >> SIM_FCFG1_PFSIZE_SHIFT;
#else
#error "Unknown flash size"
#endif
        /* PFSIZE=0xf means that on customer parts the IFR was not correctly programmed.
         * We just use the pre-defined flash size in feature file here to support pre-production parts */
        if (pfsize == 0xf)
        {
            flashDensity = FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT * FSL_FEATURE_FLASH_PFLASH_BLOCK_SIZE;
        }
        else
        {
            flashDensity = ((uint32_t)kPFlashDensities[pfsize]) << 10;
        }

        /* fill out a few of the structure members */
        config->PFlashBlockBase = FSL_FEATURE_FLASH_PFLASH_START_ADDRESS;
        config->PFlashTotalSize = flashDensity;
        config->PFlashBlockCount = FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT;
        config->PFlashSectorSize = FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE;
    }

    {
#if defined(FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL) && FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL
#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED && FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_ACCESS_REGISTER
        if (config->FlashMemoryIndex == (uint32_t)kFLASH_MemoryIndexSecondaryFlash)
        {
            config->PFlashAccessSegmentSize = kFLASH_AccessSegmentBase << FTFx->FACSSS;
            config->PFlashAccessSegmentCount = FTFx->FACSNS;
        }
        else
#endif
        {
            config->PFlashAccessSegmentSize = kFLASH_AccessSegmentBase << FTFx->FACSS;
            config->PFlashAccessSegmentCount = FTFx->FACSN;
        }
#else
        config->PFlashAccessSegmentSize = 0;
        config->PFlashAccessSegmentCount = 0;
#endif /* FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL */
    }

    config->PFlashCallback = NULL;

/* copy required flash commands to RAM */
#if (FLASH_DRIVER_IS_FLASH_RESIDENT && !FLASH_DRIVER_IS_EXPORTED)
    if (kStatus_FLASH_Success != flash_check_execute_in_ram_function_info(config))
    {
        s_flashExecuteInRamFunctionInfo.activeFunctionCount = 0;
        s_flashExecuteInRamFunctionInfo.flashRunCommand = s_flashRunCommand;
        s_flashExecuteInRamFunctionInfo.flashCommonBitOperation = s_flashCommonBitOperation;
        config->flashExecuteInRamFunctionInfo = &s_flashExecuteInRamFunctionInfo.activeFunctionCount;
        FLASH_PrepareExecuteInRamFunctions(config);
    }
#endif

    config->FlexRAMBlockBase = FSL_FEATURE_FLASH_FLEX_RAM_START_ADDRESS;
    config->FlexRAMTotalSize = FSL_FEATURE_FLASH_FLEX_RAM_SIZE;

#if FLASH_SSD_IS_FLEXNVM_ENABLED
    {
        status_t returnCode;
        config->DFlashBlockBase = FSL_FEATURE_FLASH_FLEX_NVM_START_ADDRESS;
        returnCode = flash_update_flexnvm_memory_partition_status(config);
        if (returnCode != kStatus_FLASH_Success)
        {
            return returnCode;
        }
    }
#endif

    return kStatus_FLASH_Success;
}

status_t FLASH_SetCallback(flash_config_t *config, flash_callback_t callback)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    config->PFlashCallback = callback;

    return kStatus_FLASH_Success;
}

#if FLASH_DRIVER_IS_FLASH_RESIDENT
status_t FLASH_PrepareExecuteInRamFunctions(flash_config_t *config)
{
    flash_execute_in_ram_function_config_t *flashExecuteInRamFunctionInfo;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    flashExecuteInRamFunctionInfo = (flash_execute_in_ram_function_config_t *)config->flashExecuteInRamFunctionInfo;

    copy_flash_run_command(flashExecuteInRamFunctionInfo->flashRunCommand);
    copy_flash_common_bit_operation(flashExecuteInRamFunctionInfo->flashCommonBitOperation);
    flashExecuteInRamFunctionInfo->activeFunctionCount = kFLASH_ExecuteInRamFunctionTotalNum;

    return kStatus_FLASH_Success;
}
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

status_t FLASH_EraseAll(flash_config_t *config, uint32_t key)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* preparing passing parameter to erase all flash blocks */
    kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_ERASE_ALL_BLOCK, 0xFFFFFFU);

    /* Validate the user key */
    returnCode = flash_check_user_key(key);
    if (returnCode)
    {
        return returnCode;
    }

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    flash_cache_clear(config);

#if FLASH_SSD_IS_FLEXNVM_ENABLED
    /* Data flash IFR will be erased by erase all command, so we need to
     *  update FlexNVM memory partition status synchronously */
    if (returnCode == kStatus_FLASH_Success)
    {
        returnCode = flash_update_flexnvm_memory_partition_status(config);
    }
#endif

    return returnCode;
}

status_t FLASH_Erase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint32_t key)
{
    uint32_t sectorSize;
    flash_operation_config_t flashOperationInfo;
    uint32_t endAddress;      /* storing end address */
    uint32_t numberOfSectors; /* number of sectors calculated by endAddress */
    status_t returnCode;

    flash_get_matched_operation_info(config, start, &flashOperationInfo);

    /* Check the supplied address range. */
    returnCode = flash_check_range(config, start, lengthInBytes, flashOperationInfo.sectorCmdAddressAligment);
    if (returnCode)
    {
        return returnCode;
    }

    start = flashOperationInfo.convertedAddress;
    sectorSize = flashOperationInfo.activeSectorSize;

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
        kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_ERASE_SECTOR, start);

        /* Validate the user key */
        returnCode = flash_check_user_key(key);
        if (returnCode)
        {
            return returnCode;
        }

        /* calling flash command sequence function to execute the command */
        returnCode = flash_command_sequence(config);

        /* calling flash callback function if it is available */
        if (config->PFlashCallback)
        {
            config->PFlashCallback();
        }

        /* checking the success of command execution */
        if (kStatus_FLASH_Success != returnCode)
        {
            break;
        }
        else
        {
            /* Increment to the next sector */
            start += sectorSize;
        }
    }

    flash_cache_clear(config);

    return (returnCode);
}

#if defined(FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD) && FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD
status_t FLASH_EraseAllUnsecure(flash_config_t *config, uint32_t key)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Prepare passing parameter to erase all flash blocks (unsecure). */
    kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_ERASE_ALL_BLOCK_UNSECURE, 0xFFFFFFU);

    /* Validate the user key */
    returnCode = flash_check_user_key(key);
    if (returnCode)
    {
        return returnCode;
    }

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    flash_cache_clear(config);

#if FLASH_SSD_IS_FLEXNVM_ENABLED
    /* Data flash IFR will be erased by erase all unsecure command, so we need to
     *  update FlexNVM memory partition status synchronously */
    if (returnCode == kStatus_FLASH_Success)
    {
        returnCode = flash_update_flexnvm_memory_partition_status(config);
    }
#endif

    return returnCode;
}
#endif /* FSL_FEATURE_FLASH_HAS_ERASE_ALL_BLOCKS_UNSECURE_CMD */

status_t FLASH_EraseAllExecuteOnlySegments(flash_config_t *config, uint32_t key)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* preparing passing parameter to erase all execute-only segments
     * 1st element for the FCCOB register */
    kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_ERASE_ALL_EXECUTE_ONLY_SEGMENT, 0xFFFFFFU);

    /* Validate the user key */
    returnCode = flash_check_user_key(key);
    if (returnCode)
    {
        return returnCode;
    }

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    flash_cache_clear(config);

    return returnCode;
}

status_t FLASH_Program(flash_config_t *config, uint32_t start, uint32_t *src, uint32_t lengthInBytes)
{
    status_t returnCode;
    flash_operation_config_t flashOperationInfo;

    if (src == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    flash_get_matched_operation_info(config, start, &flashOperationInfo);

    /* Check the supplied address range. */
    returnCode = flash_check_range(config, start, lengthInBytes, flashOperationInfo.blockWriteUnitSize);
    if (returnCode)
    {
        return returnCode;
    }

    start = flashOperationInfo.convertedAddress;

    while (lengthInBytes > 0)
    {
        /* preparing passing parameter to program the flash block */
        kFCCOBx[1] = *src++;
        if (4 == flashOperationInfo.blockWriteUnitSize)
        {
            kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_PROGRAM_LONGWORD, start);
        }
        else if (8 == flashOperationInfo.blockWriteUnitSize)
        {
            kFCCOBx[2] = *src++;
            kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_PROGRAM_PHRASE, start);
        }
        else
        {
        }

        /* calling flash command sequence function to execute the command */
        returnCode = flash_command_sequence(config);

        /* calling flash callback function if it is available */
        if (config->PFlashCallback)
        {
            config->PFlashCallback();
        }

        /* checking for the success of command execution */
        if (kStatus_FLASH_Success != returnCode)
        {
            break;
        }
        else
        {
            /* update start address for next iteration */
            start += flashOperationInfo.blockWriteUnitSize;

            /* update lengthInBytes for next iteration */
            lengthInBytes -= flashOperationInfo.blockWriteUnitSize;
        }
    }

    flash_cache_clear(config);

    return (returnCode);
}

status_t FLASH_ProgramOnce(flash_config_t *config, uint32_t index, uint32_t *src, uint32_t lengthInBytes)
{
    status_t returnCode;

    if ((config == NULL) || (src == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* pass paramters to FTFx */
    kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_1_2(FTFx_PROGRAM_ONCE, index, 0xFFFFU);

    kFCCOBx[1] = *src;

/* Note: Have to seperate the first index from the rest if it equals 0
 * to avoid a pointless comparison of unsigned int to 0 compiler warning */
#if FLASH_PROGRAM_ONCE_IS_8BYTES_UNIT_SUPPORT
#if FLASH_PROGRAM_ONCE_IS_4BYTES_UNIT_SUPPORT
    if (((index == FLASH_PROGRAM_ONCE_MIN_ID_8BYTES) ||
         /* Range check */
         ((index >= FLASH_PROGRAM_ONCE_MIN_ID_8BYTES + 1) && (index <= FLASH_PROGRAM_ONCE_MAX_ID_8BYTES))) &&
        (lengthInBytes == 8))
#endif /* FLASH_PROGRAM_ONCE_IS_4BYTES_UNIT_SUPPORT */
    {
        kFCCOBx[2] = *(src + 1);
    }
#endif /* FLASH_PROGRAM_ONCE_IS_8BYTES_UNIT_SUPPORT */

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    flash_cache_clear(config);

    return returnCode;
}

#if defined(FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD) && FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD
status_t FLASH_ProgramSection(flash_config_t *config, uint32_t start, uint32_t *src, uint32_t lengthInBytes)
{
    status_t returnCode;
    uint32_t sectorSize;
    flash_operation_config_t flashOperationInfo;
#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
    bool needSwitchFlexRamMode = false;
#endif /* FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD */

    if (src == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    flash_get_matched_operation_info(config, start, &flashOperationInfo);

    /* Check the supplied address range. */
    returnCode = flash_check_range(config, start, lengthInBytes, flashOperationInfo.sectionCmdAddressAligment);
    if (returnCode)
    {
        return returnCode;
    }

    start = flashOperationInfo.convertedAddress;
    sectorSize = flashOperationInfo.activeSectorSize;

#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
    /* Switch function of FlexRAM if needed */
    if (!(FTFx->FCNFG & FTFx_FCNFG_RAMRDY_MASK))
    {
        needSwitchFlexRamMode = true;

        returnCode = FLASH_SetFlexramFunction(config, kFLASH_FlexramFunctionOptionAvailableAsRam);
        if (returnCode != kStatus_FLASH_Success)
        {
            return kStatus_FLASH_SetFlexramAsRamError;
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

            if (lengthTobeProgrammedOfCurrentSector > kFLASH_AccelerationRamSize)
            {
                programSizeOfCurrentPass = kFLASH_AccelerationRamSize;
            }
            else
            {
                programSizeOfCurrentPass = lengthTobeProgrammedOfCurrentSector;
            }

            /* Copy data to FlexRAM */
            memcpy((void *)FSL_FEATURE_FLASH_FLEX_RAM_START_ADDRESS, src + currentOffset / 4, programSizeOfCurrentPass);
            /* Set start address of the data to be programmed */
            kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_PROGRAM_SECTION, start + currentOffset);
            /* Set program size in terms of FEATURE_FLASH_SECTION_CMD_ADDRESS_ALIGMENT */
            numberOfPhases = programSizeOfCurrentPass / flashOperationInfo.sectionCmdAddressAligment;

            kFCCOBx[1] = BYTES_JOIN_TO_WORD_2_2(numberOfPhases, 0xFFFFU);

            /* Peform command sequence */
            returnCode = flash_command_sequence(config);

            /* calling flash callback function if it is available */
            if (config->PFlashCallback)
            {
                config->PFlashCallback();
            }

            if (returnCode != kStatus_FLASH_Success)
            {
                flash_cache_clear(config);
                return returnCode;
            }

            lengthTobeProgrammedOfCurrentSector -= programSizeOfCurrentPass;
            currentOffset += programSizeOfCurrentPass;
        }

        src += currentOffset / 4;
        start += currentOffset;
        lengthInBytes -= currentOffset;
    }

    flash_cache_clear(config);

#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
    /* Restore function of FlexRAM if needed. */
    if (needSwitchFlexRamMode)
    {
        returnCode = FLASH_SetFlexramFunction(config, kFLASH_FlexramFunctionOptionAvailableForEeprom);
        if (returnCode != kStatus_FLASH_Success)
        {
            return kStatus_FLASH_RecoverFlexramAsEepromError;
        }
    }
#endif /* FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD */

    return returnCode;
}
#endif /* FSL_FEATURE_FLASH_HAS_PROGRAM_SECTION_CMD */

#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_EepromWrite(flash_config_t *config, uint32_t start, uint8_t *src, uint32_t lengthInBytes)
{
    status_t returnCode;
    bool needSwitchFlexRamMode = false;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Validates the range of the given address */
    if ((start < config->FlexRAMBlockBase) ||
        ((start + lengthInBytes) > (config->FlexRAMBlockBase + config->EEpromTotalSize)))
    {
        return kStatus_FLASH_AddressError;
    }

    returnCode = kStatus_FLASH_Success;

    /* Switch function of FlexRAM if needed */
    if (!(FTFx->FCNFG & FTFx_FCNFG_EEERDY_MASK))
    {
        needSwitchFlexRamMode = true;

        returnCode = FLASH_SetFlexramFunction(config, kFLASH_FlexramFunctionOptionAvailableForEeprom);
        if (returnCode != kStatus_FLASH_Success)
        {
            return kStatus_FLASH_SetFlexramAsEepromError;
        }
    }

    /* Write data to FlexRAM when it is used as EEPROM emulator */
    while (lengthInBytes > 0)
    {
        if ((!(start & 0x3U)) && (lengthInBytes >= 4))
        {
            *(uint32_t *)start = *(uint32_t *)src;
            start += 4;
            src += 4;
            lengthInBytes -= 4;
        }
        else if ((!(start & 0x1U)) && (lengthInBytes >= 2))
        {
            *(uint16_t *)start = *(uint16_t *)src;
            start += 2;
            src += 2;
            lengthInBytes -= 2;
        }
        else
        {
            *(uint8_t *)start = *src;
            start += 1;
            src += 1;
            lengthInBytes -= 1;
        }
        /* Wait till EEERDY bit is set */
        while (!(FTFx->FCNFG & FTFx_FCNFG_EEERDY_MASK))
        {
        }

        /* Check for protection violation error */
        if (FTFx->FSTAT & FTFx_FSTAT_FPVIOL_MASK)
        {
            return kStatus_FLASH_ProtectionViolation;
        }
    }

    /* Switch function of FlexRAM if needed */
    if (needSwitchFlexRamMode)
    {
        returnCode = FLASH_SetFlexramFunction(config, kFLASH_FlexramFunctionOptionAvailableAsRam);
        if (returnCode != kStatus_FLASH_Success)
        {
            return kStatus_FLASH_RecoverFlexramAsRamError;
        }
    }

    return returnCode;
}
#endif /* FLASH_SSD_IS_FLEXNVM_ENABLED */

#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
status_t FLASH_ReadResource(
    flash_config_t *config, uint32_t start, uint32_t *dst, uint32_t lengthInBytes, flash_read_resource_option_t option)
{
    status_t returnCode;
    flash_operation_config_t flashOperationInfo;

    if ((config == NULL) || (dst == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    flash_get_matched_operation_info(config, start, &flashOperationInfo);

    /* Check the supplied address range. */
    returnCode =
        flash_check_resource_range(start, lengthInBytes, flashOperationInfo.resourceCmdAddressAligment, option);
    if (returnCode != kStatus_FLASH_Success)
    {
        return returnCode;
    }

    while (lengthInBytes > 0)
    {
        /* preparing passing parameter */
        kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_READ_RESOURCE, start);
        if (flashOperationInfo.resourceCmdAddressAligment == 4)
        {
            kFCCOBx[2] = BYTES_JOIN_TO_WORD_1_3(option, 0xFFFFFFU);
        }
        else if (flashOperationInfo.resourceCmdAddressAligment == 8)
        {
            kFCCOBx[1] = BYTES_JOIN_TO_WORD_1_3(option, 0xFFFFFFU);
        }
        else
        {
        }

        /* calling flash command sequence function to execute the command */
        returnCode = flash_command_sequence(config);

        if (kStatus_FLASH_Success != returnCode)
        {
            break;
        }

        /* fetch data */
        *dst++ = kFCCOBx[1];
        if (flashOperationInfo.resourceCmdAddressAligment == 8)
        {
            *dst++ = kFCCOBx[2];
        }
        /* update start address for next iteration */
        start += flashOperationInfo.resourceCmdAddressAligment;
        /* update lengthInBytes for next iteration */
        lengthInBytes -= flashOperationInfo.resourceCmdAddressAligment;
    }

    return (returnCode);
}
#endif /* FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD */

status_t FLASH_ReadOnce(flash_config_t *config, uint32_t index, uint32_t *dst, uint32_t lengthInBytes)
{
    status_t returnCode;

    if ((config == NULL) || (dst == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* pass paramters to FTFx */
    kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_1_2(FTFx_READ_ONCE, index, 0xFFFFU);

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    if (kStatus_FLASH_Success == returnCode)
    {
        *dst = kFCCOBx[1];
/* Note: Have to seperate the first index from the rest if it equals 0
 *       to avoid a pointless comparison of unsigned int to 0 compiler warning */
#if FLASH_PROGRAM_ONCE_IS_8BYTES_UNIT_SUPPORT
#if FLASH_PROGRAM_ONCE_IS_4BYTES_UNIT_SUPPORT
        if (((index == FLASH_PROGRAM_ONCE_MIN_ID_8BYTES) ||
             /* Range check */
             ((index >= FLASH_PROGRAM_ONCE_MIN_ID_8BYTES + 1) && (index <= FLASH_PROGRAM_ONCE_MAX_ID_8BYTES))) &&
            (lengthInBytes == 8))
#endif /* FLASH_PROGRAM_ONCE_IS_4BYTES_UNIT_SUPPORT */
        {
            *(dst + 1) = kFCCOBx[2];
        }
#endif /* FLASH_PROGRAM_ONCE_IS_8BYTES_UNIT_SUPPORT */
    }

    return returnCode;
}

status_t FLASH_GetSecurityState(flash_config_t *config, flash_security_state_t *state)
{
    /* store data read from flash register */
    uint8_t registerValue;

    if ((config == NULL) || (state == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Get flash security register value */
    registerValue = FTFx->FSEC;

    /* check the status of the flash security bits in the security register */
    if (FLASH_SECURITY_STATE_UNSECURED == (registerValue & FTFx_FSEC_SEC_MASK))
    {
        /* Flash in unsecured state */
        *state = kFLASH_SecurityStateNotSecure;
    }
    else
    {
        /* Flash in secured state
         * check for backdoor key security enable bit */
        if (FLASH_SECURITY_STATE_KEYEN == (registerValue & FTFx_FSEC_KEYEN_MASK))
        {
            /* Backdoor key security enabled */
            *state = kFLASH_SecurityStateBackdoorEnabled;
        }
        else
        {
            /* Backdoor key security disabled */
            *state = kFLASH_SecurityStateBackdoorDisabled;
        }
    }

    return (kStatus_FLASH_Success);
}

status_t FLASH_SecurityBypass(flash_config_t *config, const uint8_t *backdoorKey)
{
    uint8_t registerValue; /* registerValue */
    status_t returnCode;   /* return code variable */

    if ((config == NULL) || (backdoorKey == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* set the default return code as kStatus_Success */
    returnCode = kStatus_FLASH_Success;

    /* Get flash security register value */
    registerValue = FTFx->FSEC;

    /* Check to see if flash is in secure state (any state other than 0x2)
     * If not, then skip this since flash is not secure */
    if (0x02 != (registerValue & 0x03))
    {
        /* preparing passing parameter to erase a flash block */
        kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_SECURITY_BY_PASS, 0xFFFFFFU);
        kFCCOBx[1] = BYTES_JOIN_TO_WORD_1_1_1_1(backdoorKey[0], backdoorKey[1], backdoorKey[2], backdoorKey[3]);
        kFCCOBx[2] = BYTES_JOIN_TO_WORD_1_1_1_1(backdoorKey[4], backdoorKey[5], backdoorKey[6], backdoorKey[7]);

        /* calling flash command sequence function to execute the command */
        returnCode = flash_command_sequence(config);
    }

    return (returnCode);
}

status_t FLASH_VerifyEraseAll(flash_config_t *config, flash_margin_value_t margin)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* preparing passing parameter to verify all block command */
    kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_1_2(FTFx_VERIFY_ALL_BLOCK, margin, 0xFFFFU);

    /* calling flash command sequence function to execute the command */
    return flash_command_sequence(config);
}

status_t FLASH_VerifyErase(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, flash_margin_value_t margin)
{
    /* Check arguments. */
    uint32_t blockSize;
    flash_operation_config_t flashOperationInfo;
    uint32_t nextBlockStartAddress;
    uint32_t remainingBytes;
    status_t returnCode;

    flash_get_matched_operation_info(config, start, &flashOperationInfo);

    returnCode = flash_check_range(config, start, lengthInBytes, flashOperationInfo.sectionCmdAddressAligment);
    if (returnCode)
    {
        return returnCode;
    }

    flash_get_matched_operation_info(config, start, &flashOperationInfo);
    start = flashOperationInfo.convertedAddress;
    blockSize = flashOperationInfo.activeBlockSize;

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

        numberOfPhrases = verifyLength / flashOperationInfo.sectionCmdAddressAligment;

        /* Fill in verify section command parameters. */
        kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_VERIFY_SECTION, start);
        kFCCOBx[1] = BYTES_JOIN_TO_WORD_2_1_1(numberOfPhrases, margin, 0xFFU);

        /* calling flash command sequence function to execute the command */
        returnCode = flash_command_sequence(config);
        if (returnCode)
        {
            return returnCode;
        }

        remainingBytes -= verifyLength;
        start += verifyLength;
        nextBlockStartAddress += blockSize;
    }

    return kStatus_FLASH_Success;
}

status_t FLASH_VerifyProgram(flash_config_t *config,
                             uint32_t start,
                             uint32_t lengthInBytes,
                             const uint32_t *expectedData,
                             flash_margin_value_t margin,
                             uint32_t *failedAddress,
                             uint32_t *failedData)
{
    status_t returnCode;
    flash_operation_config_t flashOperationInfo;

    if (expectedData == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    flash_get_matched_operation_info(config, start, &flashOperationInfo);

    returnCode = flash_check_range(config, start, lengthInBytes, flashOperationInfo.checkCmdAddressAligment);
    if (returnCode)
    {
        return returnCode;
    }

    start = flashOperationInfo.convertedAddress;

    while (lengthInBytes)
    {
        /* preparing passing parameter to program check the flash block */
        kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_PROGRAM_CHECK, start);
        kFCCOBx[1] = BYTES_JOIN_TO_WORD_1_3(margin, 0xFFFFFFU);
        kFCCOBx[2] = *expectedData;

        /* calling flash command sequence function to execute the command */
        returnCode = flash_command_sequence(config);

        /* checking for the success of command execution */
        if (kStatus_FLASH_Success != returnCode)
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

        lengthInBytes -= flashOperationInfo.checkCmdAddressAligment;
        expectedData += flashOperationInfo.checkCmdAddressAligment / sizeof(*expectedData);
        start += flashOperationInfo.checkCmdAddressAligment;
    }

    return (returnCode);
}

status_t FLASH_VerifyEraseAllExecuteOnlySegments(flash_config_t *config, flash_margin_value_t margin)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* preparing passing parameter to verify erase all execute-only segments command */
    kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_1_2(FTFx_VERIFY_ALL_EXECUTE_ONLY_SEGMENT, margin, 0xFFFFU);

    /* calling flash command sequence function to execute the command */
    return flash_command_sequence(config);
}

status_t FLASH_IsProtected(flash_config_t *config,
                           uint32_t start,
                           uint32_t lengthInBytes,
                           flash_protection_state_t *protection_state)
{
    uint32_t endAddress;           /* end address for protection check */
    uint32_t regionCheckedCounter; /* increments each time the flash address was checked for
                                    * protection status */
    uint32_t regionCounter;        /* incrementing variable used to increment through the flash
                                    * protection regions */
    uint32_t protectStatusCounter; /* increments each time a flash region was detected as protected */

    uint8_t flashRegionProtectStatus[FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT]; /* array of the protection
                                                                      * status for each
                                                                      * protection region */
    uint32_t flashRegionAddress[FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT +
                                1];                /* array of the start addresses for each flash
                                 * protection region. Note this is REGION_COUNT+1
                                 * due to requiring the next start address after
                                 * the end of flash for loop-check purposes below */
    flash_protection_config_t flashProtectionInfo; /* flash protection information */
    status_t returnCode;

    if (protection_state == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Check the supplied address range. */
    returnCode = flash_check_range(config, start, lengthInBytes, FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE);
    if (returnCode)
    {
        return returnCode;
    }

    /* Get necessary flash protection information. */
    returnCode = flash_get_protection_info(config, &flashProtectionInfo);
    if (returnCode)
    {
        return returnCode;
    }

    /* calculating Flash end address */
    endAddress = start + lengthInBytes;

    /* populate the flashRegionAddress array with the start address of each flash region */
    regionCounter = 0; /* make sure regionCounter is initialized to 0 first */

    /* populate up to 33rd element of array, this is the next address after end of flash array */
    while (regionCounter <= flashProtectionInfo.regionCount)
    {
        flashRegionAddress[regionCounter] =
            flashProtectionInfo.regionBase + flashProtectionInfo.regionSize * regionCounter;
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
    while (regionCounter < flashProtectionInfo.regionCount)
    {
#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED && FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_PROTECTION_REGISTER
        if (config->FlashMemoryIndex == (uint32_t)kFLASH_MemoryIndexSecondaryFlash)
        {
            if (regionCounter < 8)
            {
                flashRegionProtectStatus[regionCounter] = (FTFx_FPROTSL_REG >> regionCounter) & (0x01u);
            }
            else if ((regionCounter >= 8) && (regionCounter < 16))
            {
                flashRegionProtectStatus[regionCounter] = (FTFx_FPROTSH_REG >> (regionCounter - 8)) & (0x01u);
            }
            else
            {
                break;
            }
        }
        else
#endif
        {
            /* Note: So far protection region count may be 16/20/24/32/64 */
            if (regionCounter < 8)
            {
                flashRegionProtectStatus[regionCounter] = (FTFx_FPROTL3_REG >> regionCounter) & (0x01u);
            }
            else if ((regionCounter >= 8) && (regionCounter < 16))
            {
                flashRegionProtectStatus[regionCounter] = (FTFx_FPROTL2_REG >> (regionCounter - 8)) & (0x01u);
            }
#if defined(FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT) && (FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT > 16)
#if (FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT == 20)
            else if ((regionCounter >= 16) && (regionCounter < 20))
            {
                flashRegionProtectStatus[regionCounter] = (FTFx_FPROTL1_REG >> (regionCounter - 16)) & (0x01u);
            }
#else
            else if ((regionCounter >= 16) && (regionCounter < 24))
            {
                flashRegionProtectStatus[regionCounter] = (FTFx_FPROTL1_REG >> (regionCounter - 16)) & (0x01u);
            }
#endif /* (FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT == 20) */
#endif
#if defined(FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT) && (FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT > 24)
            else if ((regionCounter >= 24) && (regionCounter < 32))
            {
                flashRegionProtectStatus[regionCounter] = (FTFx_FPROTL0_REG >> (regionCounter - 24)) & (0x01u);
            }
#endif
#if defined(FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT) && \
    (FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT == 64)
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
                break;
            }
        }

        regionCounter++;
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
            start += flashProtectionInfo.regionSize; /* increment to an address within the next region */
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

    return (returnCode);
}

status_t FLASH_IsExecuteOnly(flash_config_t *config,
                             uint32_t start,
                             uint32_t lengthInBytes,
                             flash_execute_only_access_state_t *access_state)
{
#if defined(FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL) && FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL
    flash_access_config_t flashAccessInfo; /* flash Execute-Only information */
#endif                                     /* FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL */
    status_t returnCode;

    if (access_state == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Check the supplied address range. */
    returnCode = flash_check_range(config, start, lengthInBytes, FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE);
    if (returnCode)
    {
        return returnCode;
    }

#if defined(FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL) && FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL
    /* Get necessary flash Execute-Only information. */
    returnCode = flash_get_access_info(config, &flashAccessInfo);
    if (returnCode)
    {
        return returnCode;
    }

    {
        uint32_t executeOnlySegmentCounter = 0;

        /* calculating end address */
        uint32_t endAddress = start + lengthInBytes;

        /* Aligning start address and end address */
        uint32_t alignedStartAddress = ALIGN_DOWN(start, flashAccessInfo.SegmentSize);
        uint32_t alignedEndAddress = ALIGN_UP(endAddress, flashAccessInfo.SegmentSize);

        uint32_t segmentIndex = 0;
        uint32_t maxSupportedExecuteOnlySegmentCount =
            (alignedEndAddress - alignedStartAddress) / flashAccessInfo.SegmentSize;

        while (start < endAddress)
        {
            uint32_t xacc;

            segmentIndex = (start - flashAccessInfo.SegmentBase) / flashAccessInfo.SegmentSize;

#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED && FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_ACCESS_REGISTER
            if (config->FlashMemoryIndex == (uint32_t)kFLASH_MemoryIndexSecondaryFlash)
            {
                /* For secondary flash, The two XACCS registers allow up to 16 restricted segments of equal memory size.
                 */
                if (segmentIndex < 8)
                {
                    xacc = *(const volatile uint8_t *)&FTFx_XACCSL_REG;
                }
                else if (segmentIndex < flashAccessInfo.SegmentCount)
                {
                    xacc = *(const volatile uint8_t *)&FTFx_XACCSH_REG;
                    segmentIndex -= 8;
                }
                else
                {
                    break;
                }
            }
            else
#endif
            {
                /* For primary flash, The eight XACC registers allow up to 64 restricted segments of equal memory size.
                 */
                if (segmentIndex < 32)
                {
                    xacc = *(const volatile uint32_t *)&FTFx_XACCL3_REG;
                }
                else if (segmentIndex < flashAccessInfo.SegmentCount)
                {
                    xacc = *(const volatile uint32_t *)&FTFx_XACCH3_REG;
                    segmentIndex -= 32;
                }
                else
                {
                    break;
                }
            }

            /* Determine if this address range is in a execute-only protection flash segment. */
            if ((~xacc) & (1u << segmentIndex))
            {
                executeOnlySegmentCounter++;
            }

            start += flashAccessInfo.SegmentSize;
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
    }
#else
    *access_state = kFLASH_AccessStateUnLimited;
#endif /* FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL */

    return (returnCode);
}

status_t FLASH_GetProperty(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t *value)
{
    if ((config == NULL) || (value == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    switch (whichProperty)
    {
        case kFLASH_PropertyPflashSectorSize:
            *value = config->PFlashSectorSize;
            break;

        case kFLASH_PropertyPflashTotalSize:
            *value = config->PFlashTotalSize;
            break;

        case kFLASH_PropertyPflashBlockSize:
            *value = config->PFlashTotalSize / FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT;
            break;

        case kFLASH_PropertyPflashBlockCount:
            *value = config->PFlashBlockCount;
            break;

        case kFLASH_PropertyPflashBlockBaseAddr:
            *value = config->PFlashBlockBase;
            break;

        case kFLASH_PropertyPflashFacSupport:
#if defined(FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL)
            *value = FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL;
#else
            *value = 0;
#endif /* FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL */
            break;

        case kFLASH_PropertyPflashAccessSegmentSize:
            *value = config->PFlashAccessSegmentSize;
            break;

        case kFLASH_PropertyPflashAccessSegmentCount:
            *value = config->PFlashAccessSegmentCount;
            break;

        case kFLASH_PropertyFlexRamBlockBaseAddr:
            *value = config->FlexRAMBlockBase;
            break;

        case kFLASH_PropertyFlexRamTotalSize:
            *value = config->FlexRAMTotalSize;
            break;

#if FLASH_SSD_IS_FLEXNVM_ENABLED
        case kFLASH_PropertyDflashSectorSize:
            *value = FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_SECTOR_SIZE;
            break;
        case kFLASH_PropertyDflashTotalSize:
            *value = config->DFlashTotalSize;
            break;
        case kFLASH_PropertyDflashBlockSize:
            *value = FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_SIZE;
            break;
        case kFLASH_PropertyDflashBlockCount:
            *value = FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_COUNT;
            break;
        case kFLASH_PropertyDflashBlockBaseAddr:
            *value = config->DFlashBlockBase;
            break;
        case kFLASH_PropertyEepromTotalSize:
            *value = config->EEpromTotalSize;
            break;
#endif /* FLASH_SSD_IS_FLEXNVM_ENABLED */

        default: /* catch inputs that are not recognized */
            return kStatus_FLASH_UnknownProperty;
    }

    return kStatus_FLASH_Success;
}

#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED
status_t FLASH_SetProperty(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t value)
{
    status_t status = kStatus_FLASH_Success;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    switch (whichProperty)
    {
        case kFLASH_PropertyFlashMemoryIndex:
            if ((value != (uint32_t)kFLASH_MemoryIndexPrimaryFlash) &&
                (value != (uint32_t)kFLASH_MemoryIndexSecondaryFlash))
            {
                return kStatus_FLASH_InvalidPropertyValue;
            }
            config->FlashMemoryIndex = value;
            break;

        case kFLASH_PropertyPflashSectorSize:
        case kFLASH_PropertyPflashTotalSize:
        case kFLASH_PropertyPflashBlockSize:
        case kFLASH_PropertyPflashBlockCount:
        case kFLASH_PropertyPflashBlockBaseAddr:
        case kFLASH_PropertyPflashFacSupport:
        case kFLASH_PropertyPflashAccessSegmentSize:
        case kFLASH_PropertyPflashAccessSegmentCount:
        case kFLASH_PropertyFlexRamBlockBaseAddr:
        case kFLASH_PropertyFlexRamTotalSize:
#if FLASH_SSD_IS_FLEXNVM_ENABLED
        case kFLASH_PropertyDflashSectorSize:
        case kFLASH_PropertyDflashTotalSize:
        case kFLASH_PropertyDflashBlockSize:
        case kFLASH_PropertyDflashBlockCount:
        case kFLASH_PropertyDflashBlockBaseAddr:
        case kFLASH_PropertyEepromTotalSize:
#endif /* FLASH_SSD_IS_FLEXNVM_ENABLED */
            status = kStatus_FLASH_ReadOnlyProperty;
            break;
        default: /* catch inputs that are not recognized */
            status = kStatus_FLASH_UnknownProperty;
            break;
    }

    return status;
}
#endif /* FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED */

#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
status_t FLASH_SetFlexramFunction(flash_config_t *config, flash_flexram_function_option_t option)
{
    status_t status;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    status = flasn_check_flexram_function_option_range(option);
    if (status != kStatus_FLASH_Success)
    {
        return status;
    }

    /* preparing passing parameter to verify all block command */
    kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_1_2(FTFx_SET_FLEXRAM_FUNCTION, option, 0xFFFFU);

    /* calling flash command sequence function to execute the command */
    return flash_command_sequence(config);
}
#endif /* FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD) && FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD
status_t FLASH_SwapControl(flash_config_t *config,
                           uint32_t address,
                           flash_swap_control_option_t option,
                           flash_swap_state_config_t *returnInfo)
{
    status_t returnCode;

    if ((config == NULL) || (returnInfo == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    if (address & (FSL_FEATURE_FLASH_PFLASH_SWAP_CONTROL_CMD_ADDRESS_ALIGMENT - 1))
    {
        return kStatus_FLASH_AlignmentError;
    }

    /* Make sure address provided is in the lower half of Program flash but not in the Flash Configuration Field */
    if ((address >= (config->PFlashTotalSize / 2)) ||
        ((address >= kFLASH_ConfigAreaStart) && (address <= kFLASH_ConfigAreaEnd)))
    {
        return kStatus_FLASH_SwapIndicatorAddressError;
    }

    /* Check the option. */
    returnCode = flash_check_swap_control_option(option);
    if (returnCode)
    {
        return returnCode;
    }

    kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_3(FTFx_SWAP_CONTROL, address);
    kFCCOBx[1] = BYTES_JOIN_TO_WORD_1_3(option, 0xFFFFFFU);

    returnCode = flash_command_sequence(config);

    returnInfo->flashSwapState = (flash_swap_state_t)FTFx_FCCOB5_REG;
    returnInfo->currentSwapBlockStatus = (flash_swap_block_status_t)FTFx_FCCOB6_REG;
    returnInfo->nextSwapBlockStatus = (flash_swap_block_status_t)FTFx_FCCOB7_REG;

    return returnCode;
}
#endif /* FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP) && FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP
status_t FLASH_Swap(flash_config_t *config, uint32_t address, flash_swap_function_option_t option)
{
    flash_swap_state_config_t returnInfo;
    status_t returnCode;

    memset(&returnInfo, 0xFFU, sizeof(returnInfo));

    do
    {
        returnCode = FLASH_SwapControl(config, address, kFLASH_SwapControlOptionReportStatus, &returnInfo);
        if (returnCode != kStatus_FLASH_Success)
        {
            return returnCode;
        }

        if (kFLASH_SwapFunctionOptionDisable == option)
        {
            if (returnInfo.flashSwapState == kFLASH_SwapStateDisabled)
            {
                return kStatus_FLASH_Success;
            }
            else if (returnInfo.flashSwapState == kFLASH_SwapStateUninitialized)
            {
                /* The swap system changed to the DISABLED state with Program flash block 0
                 * located at relative flash address 0x0_0000 */
                returnCode = FLASH_SwapControl(config, address, kFLASH_SwapControlOptionDisableSystem, &returnInfo);
            }
            else
            {
                /* Swap disable should be requested only when swap system is in the uninitialized state */
                return kStatus_FLASH_SwapSystemNotInUninitialized;
            }
        }
        else
        {
            /* When first swap: the initial swap state is Uninitialized, flash swap inidicator address is unset,
             *    the swap procedure should be Uninitialized -> Update-Erased -> Complete.
             * After the first swap has been completed, the flash swap inidicator address cannot be modified
             *    unless EraseAllBlocks command is issued, the swap procedure is changed to Update -> Update-Erased ->
             *    Complete. */
            switch (returnInfo.flashSwapState)
            {
                case kFLASH_SwapStateUninitialized:
                    /* If current swap mode is Uninitialized, Initialize Swap to Initialized/READY state. */
                    returnCode =
                        FLASH_SwapControl(config, address, kFLASH_SwapControlOptionIntializeSystem, &returnInfo);
                    break;
                case kFLASH_SwapStateReady:
                    /* Validate whether the address provided to the swap system is matched to
                     * swap indicator address in the IFR */
                    returnCode = flash_validate_swap_indicator_address(config, address);
                    if (returnCode == kStatus_FLASH_Success)
                    {
                        /* If current swap mode is Initialized/Ready, Initialize Swap to UPDATE state. */
                        returnCode =
                            FLASH_SwapControl(config, address, kFLASH_SwapControlOptionSetInUpdateState, &returnInfo);
                    }
                    break;
                case kFLASH_SwapStateUpdate:
                    /* If current swap mode is Update, Erase indicator sector in non active block
                     * to proceed swap system to update-erased state */
                    returnCode = FLASH_Erase(config, address + (config->PFlashTotalSize >> 1),
                                             FSL_FEATURE_FLASH_PFLASH_SECTOR_CMD_ADDRESS_ALIGMENT, kFLASH_ApiEraseKey);
                    break;
                case kFLASH_SwapStateUpdateErased:
                    /* If current swap mode is Update or Update-Erased, progress Swap to COMPLETE State */
                    returnCode =
                        FLASH_SwapControl(config, address, kFLASH_SwapControlOptionSetInCompleteState, &returnInfo);
                    break;
                case kFLASH_SwapStateComplete:
                    break;
                case kFLASH_SwapStateDisabled:
                    /* When swap system is in disabled state, We need to clear swap system back to uninitialized
                     * by issuing EraseAllBlocks command */
                    returnCode = kStatus_FLASH_SwapSystemNotInUninitialized;
                    break;
                default:
                    returnCode = kStatus_FLASH_InvalidArgument;
                    break;
            }
        }
        if (returnCode != kStatus_FLASH_Success)
        {
            break;
        }
    } while (!((kFLASH_SwapStateComplete == returnInfo.flashSwapState) && (kFLASH_SwapFunctionOptionEnable == option)));

    return returnCode;
}
#endif /* FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP */

#if defined(FSL_FEATURE_FLASH_HAS_PROGRAM_PARTITION_CMD) && FSL_FEATURE_FLASH_HAS_PROGRAM_PARTITION_CMD
status_t FLASH_ProgramPartition(flash_config_t *config,
                                flash_partition_flexram_load_option_t option,
                                uint32_t eepromDataSizeCode,
                                uint32_t flexnvmPartitionCode)
{
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* eepromDataSizeCode[7:6], flexnvmPartitionCode[7:4] should be all 1'b0
     *  or it will cause access error. */
    /* eepromDataSizeCode &= 0x3FU;  */
    /* flexnvmPartitionCode &= 0x0FU; */

    /* preparing passing parameter to program the flash block */
    kFCCOBx[0] = BYTES_JOIN_TO_WORD_1_2_1(FTFx_PROGRAM_PARTITION, 0xFFFFU, option);
    kFCCOBx[1] = BYTES_JOIN_TO_WORD_1_1_2(eepromDataSizeCode, flexnvmPartitionCode, 0xFFFFU);

    /* calling flash command sequence function to execute the command */
    returnCode = flash_command_sequence(config);

    flash_cache_clear(config);

#if FLASH_SSD_IS_FLEXNVM_ENABLED
    /* Data flash IFR will be updated by program partition command during reset sequence,
     * so we just set reserved values for partitioned FlexNVM size here */
    config->EEpromTotalSize = FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_RESERVED;
    config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif

    return (returnCode);
}
#endif /* FSL_FEATURE_FLASH_HAS_PROGRAM_PARTITION_CMD */

status_t FLASH_PflashSetProtection(flash_config_t *config, pflash_protection_status_t *protectStatus)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED && FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_PROTECTION_REGISTER
    if (config->FlashMemoryIndex == (uint32_t)kFLASH_MemoryIndexSecondaryFlash)
    {
        *kFPROTSL = protectStatus->valueLow32b.prots16b.protsl;
        if (protectStatus->valueLow32b.prots16b.protsl != *kFPROTSL)
        {
            return kStatus_FLASH_CommandFailure;
        }

        *kFPROTSH = protectStatus->valueLow32b.prots16b.protsh;
        if (protectStatus->valueLow32b.prots16b.protsh != *kFPROTSH)
        {
            return kStatus_FLASH_CommandFailure;
        }
    }
    else
#endif
    {
        *kFPROTL = protectStatus->valueLow32b.protl32b;
        if (protectStatus->valueLow32b.protl32b != *kFPROTL)
        {
            return kStatus_FLASH_CommandFailure;
        }

#if defined(FTFx_FPROT_HIGH_REG)
        *kFPROTH = protectStatus->valueHigh32b.proth32b;
        if (protectStatus->valueHigh32b.proth32b != *kFPROTH)
        {
            return kStatus_FLASH_CommandFailure;
        }
#endif
    }

    return kStatus_FLASH_Success;
}

status_t FLASH_PflashGetProtection(flash_config_t *config, pflash_protection_status_t *protectStatus)
{
    if ((config == NULL) || (protectStatus == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED && FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_PROTECTION_REGISTER
    if (config->FlashMemoryIndex == (uint32_t)kFLASH_MemoryIndexSecondaryFlash)
    {
        protectStatus->valueLow32b.prots16b.protsl = *kFPROTSL;
        protectStatus->valueLow32b.prots16b.protsh = *kFPROTSH;
    }
    else
#endif
    {
        protectStatus->valueLow32b.protl32b = *kFPROTL;
#if defined(FTFx_FPROT_HIGH_REG)
        protectStatus->valueHigh32b.proth32b = *kFPROTH;
#endif
    }

    return kStatus_FLASH_Success;
}

#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_DflashSetProtection(flash_config_t *config, uint8_t protectStatus)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    if ((config->DFlashTotalSize == 0) || (config->DFlashTotalSize == FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED))
    {
        return kStatus_FLASH_CommandNotSupported;
    }

    FTFx->FDPROT = protectStatus;

    if (FTFx->FDPROT != protectStatus)
    {
        return kStatus_FLASH_CommandFailure;
    }

    return kStatus_FLASH_Success;
}
#endif /* FLASH_SSD_IS_FLEXNVM_ENABLED */

#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_DflashGetProtection(flash_config_t *config, uint8_t *protectStatus)
{
    if ((config == NULL) || (protectStatus == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    if ((config->DFlashTotalSize == 0) || (config->DFlashTotalSize == FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED))
    {
        return kStatus_FLASH_CommandNotSupported;
    }

    *protectStatus = FTFx->FDPROT;

    return kStatus_FLASH_Success;
}
#endif /* FLASH_SSD_IS_FLEXNVM_ENABLED */

#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_EepromSetProtection(flash_config_t *config, uint8_t protectStatus)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    if ((config->EEpromTotalSize == 0) || (config->EEpromTotalSize == FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_RESERVED))
    {
        return kStatus_FLASH_CommandNotSupported;
    }

    FTFx->FEPROT = protectStatus;

    if (FTFx->FEPROT != protectStatus)
    {
        return kStatus_FLASH_CommandFailure;
    }

    return kStatus_FLASH_Success;
}
#endif /* FLASH_SSD_IS_FLEXNVM_ENABLED */

#if FLASH_SSD_IS_FLEXNVM_ENABLED
status_t FLASH_EepromGetProtection(flash_config_t *config, uint8_t *protectStatus)
{
    if ((config == NULL) || (protectStatus == NULL))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    if ((config->EEpromTotalSize == 0) || (config->EEpromTotalSize == FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_RESERVED))
    {
        return kStatus_FLASH_CommandNotSupported;
    }

    *protectStatus = FTFx->FEPROT;

    return kStatus_FLASH_Success;
}
#endif /* FLASH_SSD_IS_FLEXNVM_ENABLED */

status_t FLASH_PflashSetPrefetchSpeculation(flash_prefetch_speculation_status_t *speculationStatus)
{
#if FLASH_DRIVER_IS_FLASH_RESIDENT
    void (*flashCommonBitOperationCallback)(FTFx_REG32_ACCESS_TYPE base, uint32_t bitMask, uint32_t bitShift,
                                            uint32_t bitValue);
    uint32_t flashCommonBitOperationBuffer[kFLASH_ExecuteInRamFunctionMaxSizeInWords];

    assert(sizeof(s_flashCommonBitOperationFunctionCode) <= (kFLASH_ExecuteInRamFunctionMaxSizeInWords * 4));

    memcpy((void *)flashCommonBitOperationBuffer, (void *)s_flashCommonBitOperationFunctionCode,
           sizeof(s_flashCommonBitOperationFunctionCode));
    flashCommonBitOperationCallback = (void (*)(FTFx_REG32_ACCESS_TYPE base, uint32_t bitMask, uint32_t bitShift,
                                                uint32_t bitValue))((uint32_t)flashCommonBitOperationBuffer + 1);
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

#if defined(FSL_FEATURE_FLASH_HAS_MCM_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_MCM_FLASH_CACHE_CONTROLS
    {
        FTFx_REG32_ACCESS_TYPE regBase;
#if defined(MCM)
        regBase = (FTFx_REG32_ACCESS_TYPE)&MCM->PLACR;
#elif defined(MCM0)
        regBase = (FTFx_REG32_ACCESS_TYPE)&MCM0->PLACR;
#endif
        if (speculationStatus->instructionOption == kFLASH_prefetchSpeculationOptionDisable)
        {
            if (speculationStatus->dataOption == kFLASH_prefetchSpeculationOptionEnable)
            {
                return kStatus_FLASH_InvalidSpeculationOption;
            }
            else
            {
#if FLASH_DRIVER_IS_FLASH_RESIDENT
                flashCommonBitOperationCallback(regBase, MCM_PLACR_DFCS_MASK, MCM_PLACR_DFCS_SHIFT, 1U);
#else
                *regBase |= MCM_PLACR_DFCS_MASK;
#endif
            }
        }
        else
        {
#if FLASH_DRIVER_IS_FLASH_RESIDENT
            flashCommonBitOperationCallback(regBase, MCM_PLACR_DFCS_MASK, MCM_PLACR_DFCS_SHIFT, 0U);
#else
            *regBase &= ~MCM_PLACR_DFCS_MASK;
#endif
            if (speculationStatus->dataOption == kFLASH_prefetchSpeculationOptionEnable)
            {
#if FLASH_DRIVER_IS_FLASH_RESIDENT
                flashCommonBitOperationCallback(regBase, MCM_PLACR_EFDS_MASK, MCM_PLACR_EFDS_SHIFT, 1U);
#else
                *regBase |= MCM_PLACR_EFDS_MASK;
#endif
            }
            else
            {
#if FLASH_DRIVER_IS_FLASH_RESIDENT
                flashCommonBitOperationCallback(regBase, MCM_PLACR_EFDS_MASK, MCM_PLACR_EFDS_SHIFT, 0U);
#else
                *regBase &= ~MCM_PLACR_EFDS_MASK;
#endif
            }
        }
    }
#elif defined(FSL_FEATURE_FLASH_HAS_FMC_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_FMC_FLASH_CACHE_CONTROLS
    {
        FTFx_REG32_ACCESS_TYPE regBase;
        uint32_t b0dpeMask, b0ipeMask;
#if FLASH_DRIVER_IS_FLASH_RESIDENT
        uint32_t b0dpeShift, b0ipeShift;
#endif
#if defined(FMC_PFB01CR_B0DPE_MASK)
        regBase = (FTFx_REG32_ACCESS_TYPE)&FMC->PFB01CR;
        b0dpeMask = FMC_PFB01CR_B0DPE_MASK;
        b0ipeMask = FMC_PFB01CR_B0IPE_MASK;
#if FLASH_DRIVER_IS_FLASH_RESIDENT
        b0dpeShift = FMC_PFB01CR_B0DPE_SHIFT;
        b0ipeShift = FMC_PFB01CR_B0IPE_SHIFT;
#endif
#elif defined(FMC_PFB0CR_B0DPE_MASK)
        regBase = (FTFx_REG32_ACCESS_TYPE)&FMC->PFB0CR;
        b0dpeMask = FMC_PFB0CR_B0DPE_MASK;
        b0ipeMask = FMC_PFB0CR_B0IPE_MASK;
#if FLASH_DRIVER_IS_FLASH_RESIDENT
        b0dpeShift = FMC_PFB0CR_B0DPE_SHIFT;
        b0ipeShift = FMC_PFB0CR_B0IPE_SHIFT;
#endif
#endif
        if (speculationStatus->instructionOption == kFLASH_prefetchSpeculationOptionEnable)
        {
#if FLASH_DRIVER_IS_FLASH_RESIDENT
            flashCommonBitOperationCallback(regBase, b0ipeMask, b0ipeShift, 1U);
#else
            *regBase |= b0ipeMask;
#endif
        }
        else
        {
#if FLASH_DRIVER_IS_FLASH_RESIDENT
            flashCommonBitOperationCallback(regBase, b0ipeMask, b0ipeShift, 0U);
#else
            *regBase &= ~b0ipeMask;
#endif
        }
        if (speculationStatus->dataOption == kFLASH_prefetchSpeculationOptionEnable)
        {
#if FLASH_DRIVER_IS_FLASH_RESIDENT
            flashCommonBitOperationCallback(regBase, b0dpeMask, b0dpeShift, 1U);
#else
            *regBase |= b0dpeMask;
#endif
        }
        else
        {
#if FLASH_DRIVER_IS_FLASH_RESIDENT
            flashCommonBitOperationCallback(regBase, b0dpeMask, b0dpeShift, 0U);
#else
            *regBase &= ~b0dpeMask;
#endif
        }

/* Invalidate Prefetch Speculation Buffer */
#if defined(FMC_PFB01CR_S_INV_MASK)
        FMC->PFB01CR |= FMC_PFB01CR_S_INV_MASK;
#elif defined(FMC_PFB0CR_S_INV_MASK)
        FMC->PFB0CR |= FMC_PFB0CR_S_INV_MASK;
#endif
    }
#elif defined(FSL_FEATURE_FLASH_HAS_MSCM_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_MSCM_FLASH_CACHE_CONTROLS
    {
        FTFx_REG32_ACCESS_TYPE regBase;
        uint32_t flashSpeculationMask, dataPrefetchMask;
#if FLASH_DRIVER_IS_FLASH_RESIDENT
        uint32_t flashSpeculationShift, dataPrefetchShift;
        flashSpeculationShift = MSCM_OCMDR_OCMC1_DFCS_SHIFT;
        dataPrefetchShift = MSCM_OCMDR_OCMC1_DFDS_SHIFT;
#endif

        regBase = (FTFx_REG32_ACCESS_TYPE)&MSCM->OCMDR[0];
        flashSpeculationMask = MSCM_OCMDR_OCMC1_DFCS_MASK;
        dataPrefetchMask = MSCM_OCMDR_OCMC1_DFDS_MASK;

        if (speculationStatus->instructionOption == kFLASH_prefetchSpeculationOptionDisable)
        {
            if (speculationStatus->dataOption == kFLASH_prefetchSpeculationOptionEnable)
            {
                return kStatus_FLASH_InvalidSpeculationOption;
            }
            else
            {
#if FLASH_DRIVER_IS_FLASH_RESIDENT
                flashCommonBitOperationCallback(regBase, flashSpeculationMask, flashSpeculationShift, 1U);
#else
                *regBase |= flashSpeculationMask;
#endif
            }
        }
        else
        {
            *regBase &= ~flashSpeculationMask;
            if (speculationStatus->dataOption == kFLASH_prefetchSpeculationOptionEnable)
            {
#if FLASH_DRIVER_IS_FLASH_RESIDENT
                flashCommonBitOperationCallback(regBase, dataPrefetchMask, dataPrefetchShift, 0U);
#else
                *regBase &= ~dataPrefetchMask;
#endif
            }
            else
            {
#if FLASH_DRIVER_IS_FLASH_RESIDENT
                flashCommonBitOperationCallback(regBase, dataPrefetchMask, dataPrefetchShift, 1U);
#else
                *regBase |= dataPrefetchMask;
#endif
            }
        }
    }
#else
#if FLASH_DRIVER_IS_FLASH_RESIDENT
    flashCommonBitOperationCallback((FTFx_REG32_ACCESS_TYPE)0, 0, 0, 0);
#endif
#endif /* FSL_FEATURE_FTFx_MCM_FLASH_CACHE_CONTROLS */

    return kStatus_FLASH_Success;
}

status_t FLASH_PflashGetPrefetchSpeculation(flash_prefetch_speculation_status_t *speculationStatus)
{
    memset(speculationStatus, 0, sizeof(flash_prefetch_speculation_status_t));

    /* Assuming that all speculation options are enabled. */
    speculationStatus->instructionOption = kFLASH_prefetchSpeculationOptionEnable;
    speculationStatus->dataOption = kFLASH_prefetchSpeculationOptionEnable;

#if defined(FSL_FEATURE_FLASH_HAS_MCM_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_MCM_FLASH_CACHE_CONTROLS
    {
        uint32_t value;
#if defined(MCM)
        value = MCM->PLACR;
#elif defined(MCM0)
        value = MCM0->PLACR;
#endif
        if (value & MCM_PLACR_DFCS_MASK)
        {
            /* Speculation buffer is off. */
            speculationStatus->instructionOption = kFLASH_prefetchSpeculationOptionDisable;
            speculationStatus->dataOption = kFLASH_prefetchSpeculationOptionDisable;
        }
        else
        {
            /* Speculation buffer is on for instruction. */
            if (!(value & MCM_PLACR_EFDS_MASK))
            {
                /* Speculation buffer is off for data. */
                speculationStatus->dataOption = kFLASH_prefetchSpeculationOptionDisable;
            }
        }
    }
#elif defined(FSL_FEATURE_FLASH_HAS_FMC_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_FMC_FLASH_CACHE_CONTROLS
    {
        uint32_t value;
        uint32_t b0dpeMask, b0ipeMask;
#if defined(FMC_PFB01CR_B0DPE_MASK)
        value = FMC->PFB01CR;
        b0dpeMask = FMC_PFB01CR_B0DPE_MASK;
        b0ipeMask = FMC_PFB01CR_B0IPE_MASK;
#elif defined(FMC_PFB0CR_B0DPE_MASK)
        value = FMC->PFB0CR;
        b0dpeMask = FMC_PFB0CR_B0DPE_MASK;
        b0ipeMask = FMC_PFB0CR_B0IPE_MASK;
#endif
        if (!(value & b0dpeMask))
        {
            /* Do not prefetch in response to data references. */
            speculationStatus->dataOption = kFLASH_prefetchSpeculationOptionDisable;
        }
        if (!(value & b0ipeMask))
        {
            /* Do not prefetch in response to instruction fetches. */
            speculationStatus->instructionOption = kFLASH_prefetchSpeculationOptionDisable;
        }
    }
#elif defined(FSL_FEATURE_FLASH_HAS_MSCM_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_MSCM_FLASH_CACHE_CONTROLS
    {
        uint32_t value;
        uint32_t flashSpeculationMask, dataPrefetchMask;
        value = MSCM->OCMDR[0];
        flashSpeculationMask = MSCM_OCMDR_OCMC1_DFCS_MASK;
        dataPrefetchMask = MSCM_OCMDR_OCMC1_DFDS_MASK;

        if (value & flashSpeculationMask)
        {
            /* Speculation buffer is off. */
            speculationStatus->instructionOption = kFLASH_prefetchSpeculationOptionDisable;
            speculationStatus->dataOption = kFLASH_prefetchSpeculationOptionDisable;
        }
        else
        {
            /* Speculation buffer is on for instruction. */
            if (value & dataPrefetchMask)
            {
                /* Speculation buffer is off for data. */
                speculationStatus->dataOption = kFLASH_prefetchSpeculationOptionDisable;
            }
        }
    }
#endif

    return kStatus_FLASH_Success;
}

#if FLASH_DRIVER_IS_FLASH_RESIDENT
/*!
 * @brief Copy PIC of flash_run_command() to RAM
 */
static void copy_flash_run_command(uint32_t *flashRunCommand)
{
    assert(sizeof(s_flashRunCommandFunctionCode) <= (kFLASH_ExecuteInRamFunctionMaxSizeInWords * 4));

    /* Since the value of ARM function pointer is always odd, but the real start address
     * of function memory should be even, that's why +1 operation exist. */
    memcpy((void *)flashRunCommand, (void *)s_flashRunCommandFunctionCode, sizeof(s_flashRunCommandFunctionCode));
    callFlashRunCommand = (void (*)(FTFx_REG8_ACCESS_TYPE ftfx_fstat))((uint32_t)flashRunCommand + 1);
}
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

/*!
 * @brief Flash Command Sequence
 *
 * This function is used to perform the command write sequence to the flash.
 *
 * @param driver Pointer to storage for the driver runtime state.
 * @return An error code or kStatus_FLASH_Success
 */
static status_t flash_command_sequence(flash_config_t *config)
{
    uint8_t registerValue;

#if FLASH_DRIVER_IS_FLASH_RESIDENT
    /* clear RDCOLERR & ACCERR & FPVIOL flag in flash status register */
    FTFx->FSTAT = FTFx_FSTAT_RDCOLERR_MASK | FTFx_FSTAT_ACCERR_MASK | FTFx_FSTAT_FPVIOL_MASK;

    status_t returnCode = flash_check_execute_in_ram_function_info(config);
    if (kStatus_FLASH_Success != returnCode)
    {
        return returnCode;
    }

    /* We pass the ftfx_fstat address as a parameter to flash_run_comamnd() instead of using
     * pre-processed MICRO sentences or operating global variable in flash_run_comamnd()
     * to make sure that flash_run_command() will be compiled into position-independent code (PIC). */
    callFlashRunCommand((FTFx_REG8_ACCESS_TYPE)(&FTFx->FSTAT));
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
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

    /* Check error bits */
    /* Get flash status register value */
    registerValue = FTFx->FSTAT;

    /* checking access error */
    if (registerValue & FTFx_FSTAT_ACCERR_MASK)
    {
        return kStatus_FLASH_AccessError;
    }
    /* checking protection error */
    else if (registerValue & FTFx_FSTAT_FPVIOL_MASK)
    {
        return kStatus_FLASH_ProtectionViolation;
    }
    /* checking MGSTAT0 non-correctable error */
    else if (registerValue & FTFx_FSTAT_MGSTAT0_MASK)
    {
        return kStatus_FLASH_CommandFailure;
    }
    else
    {
        return kStatus_FLASH_Success;
    }
}

#if FLASH_DRIVER_IS_FLASH_RESIDENT
/*!
 * @brief Copy PIC of flash_common_bit_operation() to RAM
 *
 */
static void copy_flash_common_bit_operation(uint32_t *flashCommonBitOperation)
{
    assert(sizeof(s_flashCommonBitOperationFunctionCode) <= (kFLASH_ExecuteInRamFunctionMaxSizeInWords * 4));

    /* Since the value of ARM function pointer is always odd, but the real start address
     * of function memory should be even, that's why +1 operation exist. */
    memcpy((void *)flashCommonBitOperation, (void *)s_flashCommonBitOperationFunctionCode,
           sizeof(s_flashCommonBitOperationFunctionCode));
    callFlashCommonBitOperation = (void (*)(FTFx_REG32_ACCESS_TYPE base, uint32_t bitMask, uint32_t bitShift,
                                            uint32_t bitValue))((uint32_t)flashCommonBitOperation + 1);
}
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

/*!
 * @brief Flash Cache Clear
 *
 * This function is used to perform the cache clear to the flash.
 */
#if (defined(__GNUC__))
/* #pragma GCC push_options */
/* #pragma GCC optimize("O0") */
void __attribute__((optimize("O0"))) flash_cache_clear(flash_config_t *config)
#else
#if (defined(__ICCARM__))
#pragma optimize = none
#endif
#if (defined(__CC_ARM))
#pragma push
#pragma O0
#endif
void flash_cache_clear(flash_config_t *config)
#endif
{
#if FLASH_DRIVER_IS_FLASH_RESIDENT
    FTFx_REG32_ACCESS_TYPE regBase = (FTFx_REG32_ACCESS_TYPE)0;
    status_t returnCode = flash_check_execute_in_ram_function_info(config);
    if (kStatus_FLASH_Success != returnCode)
    {
        return;
    }

/* We pass the ftfx register address as a parameter to flash_common_bit_operation() instead of using
 * pre-processed MACROs or a global variable in flash_common_bit_operation()
 * to make sure that flash_common_bit_operation() will be compiled into position-independent code (PIC). */
#if defined(FSL_FEATURE_FLASH_HAS_MCM_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_MCM_FLASH_CACHE_CONTROLS
#if defined(MCM)
    regBase = (FTFx_REG32_ACCESS_TYPE)&MCM->PLACR;
    callFlashCommonBitOperation(regBase, MCM_PLACR_CFCC_MASK, MCM_PLACR_CFCC_SHIFT, 1U);
#endif
#if defined(MCM0)
    regBase = (FTFx_REG32_ACCESS_TYPE)&MCM0->PLACR;
    callFlashCommonBitOperation(regBase, MCM_PLACR_CFCC_MASK, MCM_PLACR_CFCC_SHIFT, 1U);
#endif
#if defined(MCM1)
    regBase = (FTFx_REG32_ACCESS_TYPE)&MCM1->PLACR;
    callFlashCommonBitOperation(regBase, MCM_PLACR_CFCC_MASK, MCM_PLACR_CFCC_SHIFT, 1U);
#endif
#elif defined(FSL_FEATURE_FLASH_HAS_FMC_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_FMC_FLASH_CACHE_CONTROLS
#if defined(FMC_PFB01CR_CINV_WAY_MASK)
    regBase = (FTFx_REG32_ACCESS_TYPE)&FMC->PFB01CR;
    callFlashCommonBitOperation(regBase, FMC_PFB01CR_CINV_WAY_MASK, FMC_PFB01CR_CINV_WAY_SHIFT, 0xFU);
#else
    regBase = (FTFx_REG32_ACCESS_TYPE)&FMC->PFB0CR;
    callFlashCommonBitOperation(regBase, FMC_PFB0CR_CINV_WAY_MASK, FMC_PFB0CR_CINV_WAY_SHIFT, 0xFU);
#endif
#elif defined(FSL_FEATURE_FLASH_HAS_MSCM_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_MSCM_FLASH_CACHE_CONTROLS
    regBase = (FTFx_REG32_ACCESS_TYPE)&MSCM->OCMDR[0];
#if defined(MSCM_OCMDR_OCM1_MASK)
    callFlashCommonBitOperation(regBase, MSCM_OCMDR_OCM1_MASK, MSCM_OCMDR_OCM1_SHIFT, 0x3U);
#else
    callFlashCommonBitOperation(regBase, MSCM_OCMDR_OCMC1_MASK, MSCM_OCMDR_OCMC1_SHIFT, 0x3U);
#endif
#if FLASH_SSD_IS_FLEXNVM_ENABLED
    regBase = (FTFx_REG32_ACCESS_TYPE)&MSCM->OCMDR[1];
#if defined(MSCM_OCMDR_OCM1_MASK)
    callFlashCommonBitOperation(regBase, MSCM_OCMDR_OCM1_MASK, MSCM_OCMDR_OCM1_SHIFT, 0x3U);
#else
    callFlashCommonBitOperation(regBase, MSCM_OCMDR_OCMC1_MASK, MSCM_OCMDR_OCMC1_SHIFT, 0x3U);
#endif
#endif
#else
#if defined(FMC_PFB0CR_S_INV_MASK)
    regBase = (FTFx_REG32_ACCESS_TYPE)&FMC->PFB0CR;
    callFlashCommonBitOperation(regBase, FMC_PFB0CR_S_INV_MASK, FMC_PFB0CR_S_INV_SHIFT, 1U);
#elif defined(FMC_PFB01CR_S_INV_MASK)
    regBase = (FTFx_REG32_ACCESS_TYPE)&FMC->PFB01CR;
    callFlashCommonBitOperation(regBase, FMC_PFB01CR_S_INV_MASK, FMC_PFB01CR_S_INV_SHIFT, 1U);
#endif
/* #error "Unknown flash cache controller" */
#endif /* FSL_FEATURE_FTFx_MCM_FLASH_CACHE_CONTROLS */

    callFlashCommonBitOperation(regBase, 0, 0, 0);
#else

#if defined(FSL_FEATURE_FLASH_HAS_MCM_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_MCM_FLASH_CACHE_CONTROLS
#if defined(MCM)
    MCM->PLACR |= MCM_PLACR_CFCC_MASK;
#endif
#if defined(MCM0)
    MCM0->PLACR |= MCM_PLACR_CFCC_MASK;
#endif
#if defined(MCM1)
    MCM1->PLACR |= MCM_PLACR_CFCC_MASK;
#endif
#elif defined(FSL_FEATURE_FLASH_HAS_FMC_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_FMC_FLASH_CACHE_CONTROLS
#if defined(FMC_PFB01CR_CINV_WAY_MASK)
    FMC->PFB01CR = (FMC->PFB01CR & ~FMC_PFB01CR_CINV_WAY_MASK) | FMC_PFB01CR_CINV_WAY(~0);
#else
    FMC->PFB0CR = (FMC->PFB0CR & ~FMC_PFB0CR_CINV_WAY_MASK) | FMC_PFB0CR_CINV_WAY(~0);
#endif
#elif defined(FSL_FEATURE_FLASH_HAS_MSCM_FLASH_CACHE_CONTROLS) && FSL_FEATURE_FLASH_HAS_MSCM_FLASH_CACHE_CONTROLS
#if defined(MSCM_OCMDR_OCM1_MASK)
    MSCM->OCMDR[0] |= MSCM_OCMDR_OCM1(3);
#else
    MSCM->OCMDR[0] |= MSCM_OCMDR_OCMC1(3);
#endif
#if FLASH_SSD_IS_FLEXNVM_ENABLED
#if defined(MSCM_OCMDR_OCM1_MASK)
    MSCM->OCMDR[1] |= MSCM_OCMDR_OCM1(3);
#else
    MSCM->OCMDR[1] |= MSCM_OCMDR_OCMC1(3);
#endif
#endif
#else
#if defined(FMC_PFB0CR_S_INV_MASK)
    FMC->PFB0CR |= FMC_PFB0CR_S_INV_MASK;
#elif defined(FMC_PFB01CR_S_INV_MASK)
    FMC->PFB01CR |= FMC_PFB01CR_S_INV_MASK;
#endif
/*    #error "Unknown flash cache controller" */
#endif /* FSL_FEATURE_FTFx_MCM_FLASH_CACHE_CONTROLS */

    /* Memory barriers for good measure.
     * All Cache, Branch predictor and TLB maintenance operations before this instruction complete */
    __ISB();
    __DSB();
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */
}
#if (defined(__CC_ARM))
#pragma pop
#endif
#if (defined(__GNUC__))
/* #pragma GCC pop_options */
#endif

#if FLASH_DRIVER_IS_FLASH_RESIDENT
/*! @brief Check whether flash execute-in-ram functions are ready  */
static status_t flash_check_execute_in_ram_function_info(flash_config_t *config)
{
    flash_execute_in_ram_function_config_t *flashExecuteInRamFunctionInfo;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    flashExecuteInRamFunctionInfo = (flash_execute_in_ram_function_config_t *)config->flashExecuteInRamFunctionInfo;

    if ((config->flashExecuteInRamFunctionInfo) &&
        (kFLASH_ExecuteInRamFunctionTotalNum == flashExecuteInRamFunctionInfo->activeFunctionCount))
    {
        return kStatus_FLASH_Success;
    }

    return kStatus_FLASH_ExecuteInRamFunctionNotReady;
}
#endif /* FLASH_DRIVER_IS_FLASH_RESIDENT */

/*! @brief Validates the range and alignment of the given address range.*/
static status_t flash_check_range(flash_config_t *config,
                                  uint32_t startAddress,
                                  uint32_t lengthInBytes,
                                  uint32_t alignmentBaseline)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Verify the start and length are alignmentBaseline aligned. */
    if ((startAddress & (alignmentBaseline - 1)) || (lengthInBytes & (alignmentBaseline - 1)))
    {
        return kStatus_FLASH_AlignmentError;
    }

    /* check for valid range of the target addresses */
    if (
#if FLASH_SSD_IS_FLEXNVM_ENABLED
        ((startAddress >= config->DFlashBlockBase) &&
         ((startAddress + lengthInBytes) <= (config->DFlashBlockBase + config->DFlashTotalSize))) ||
#endif
        ((startAddress >= config->PFlashBlockBase) &&
         ((startAddress + lengthInBytes) <= (config->PFlashBlockBase + config->PFlashTotalSize))))
    {
        return kStatus_FLASH_Success;
    }

    return kStatus_FLASH_AddressError;
}

/*! @brief Gets the right address, sector and block size of current flash type which is indicated by address.*/
static status_t flash_get_matched_operation_info(flash_config_t *config,
                                                 uint32_t address,
                                                 flash_operation_config_t *info)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Clean up info Structure*/
    memset(info, 0, sizeof(flash_operation_config_t));

#if FLASH_SSD_IS_FLEXNVM_ENABLED
    if ((address >= config->DFlashBlockBase) && (address <= (config->DFlashBlockBase + config->DFlashTotalSize)))
    {
        /* When required by the command, address bit 23 selects between program flash memory
         * (=0) and data flash memory (=1).*/
        info->convertedAddress = address - config->DFlashBlockBase + 0x800000U;
        info->activeSectorSize = FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_SECTOR_SIZE;
        info->activeBlockSize = config->DFlashTotalSize / FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_COUNT;

        info->blockWriteUnitSize = FSL_FEATURE_FLASH_FLEX_NVM_BLOCK_WRITE_UNIT_SIZE;
        info->sectorCmdAddressAligment = FSL_FEATURE_FLASH_FLEX_NVM_SECTOR_CMD_ADDRESS_ALIGMENT;
        info->sectionCmdAddressAligment = FSL_FEATURE_FLASH_FLEX_NVM_SECTION_CMD_ADDRESS_ALIGMENT;
        info->resourceCmdAddressAligment = FSL_FEATURE_FLASH_FLEX_NVM_RESOURCE_CMD_ADDRESS_ALIGMENT;
        info->checkCmdAddressAligment = FSL_FEATURE_FLASH_FLEX_NVM_CHECK_CMD_ADDRESS_ALIGMENT;
    }
    else
#endif /* FLASH_SSD_IS_FLEXNVM_ENABLED */
    {
        info->convertedAddress = address - config->PFlashBlockBase;
        info->activeSectorSize = config->PFlashSectorSize;
        info->activeBlockSize = config->PFlashTotalSize / config->PFlashBlockCount;
#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED
        if (config->FlashMemoryIndex == (uint32_t)kFLASH_MemoryIndexSecondaryFlash)
        {
#if FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_PROTECTION_REGISTER || FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_ACCESS_REGISTER
            /* When required by the command, address bit 23 selects between main flash memory
             * (=0) and secondary flash memory (=1).*/
            info->convertedAddress += 0x800000U;
#endif
            info->blockWriteUnitSize = FSL_FEATURE_FLASH_PFLASH_1_BLOCK_WRITE_UNIT_SIZE;
        }
        else
#endif /* FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED */
        {
            info->blockWriteUnitSize = FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE;
        }

        info->sectorCmdAddressAligment = FSL_FEATURE_FLASH_PFLASH_SECTOR_CMD_ADDRESS_ALIGMENT;
        info->sectionCmdAddressAligment = FSL_FEATURE_FLASH_PFLASH_SECTION_CMD_ADDRESS_ALIGMENT;
        info->resourceCmdAddressAligment = FSL_FEATURE_FLASH_PFLASH_RESOURCE_CMD_ADDRESS_ALIGMENT;
        info->checkCmdAddressAligment = FSL_FEATURE_FLASH_PFLASH_CHECK_CMD_ADDRESS_ALIGMENT;
    }

    return kStatus_FLASH_Success;
}

/*! @brief Validates the given user key for flash erase APIs.*/
static status_t flash_check_user_key(uint32_t key)
{
    /* Validate the user key */
    if (key != kFLASH_ApiEraseKey)
    {
        return kStatus_FLASH_EraseKeyError;
    }

    return kStatus_FLASH_Success;
}

#if FLASH_SSD_IS_FLEXNVM_ENABLED
/*! @brief Updates FlexNVM memory partition status according to data flash 0 IFR.*/
static status_t flash_update_flexnvm_memory_partition_status(flash_config_t *config)
{
    struct
    {
        uint32_t reserved0;
        uint8_t FlexNVMPartitionCode;
        uint8_t EEPROMDataSetSize;
        uint16_t reserved1;
    } dataIFRReadOut;
    status_t returnCode;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Get FlexNVM memory partition info from data flash IFR */
    returnCode = FLASH_ReadResource(config, DFLASH_IFR_READRESOURCE_START_ADDRESS, (uint32_t *)&dataIFRReadOut,
                                    sizeof(dataIFRReadOut), kFLASH_ResourceOptionFlashIfr);
    if (returnCode != kStatus_FLASH_Success)
    {
        return kStatus_FLASH_PartitionStatusUpdateFailure;
    }

    /* Fill out partitioned EEPROM size */
    dataIFRReadOut.EEPROMDataSetSize &= 0x0FU;
    switch (dataIFRReadOut.EEPROMDataSetSize)
    {
        case 0x00U:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0000;
            break;
        case 0x01U:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0001;
            break;
        case 0x02U:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0010;
            break;
        case 0x03U:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0011;
            break;
        case 0x04U:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0100;
            break;
        case 0x05U:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0101;
            break;
        case 0x06U:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0110;
            break;
        case 0x07U:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_0111;
            break;
        case 0x08U:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1000;
            break;
        case 0x09U:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1001;
            break;
        case 0x0AU:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1010;
            break;
        case 0x0BU:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1011;
            break;
        case 0x0CU:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1100;
            break;
        case 0x0DU:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1101;
            break;
        case 0x0EU:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1110;
            break;
        case 0x0FU:
            config->EEpromTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_1111;
            break;
        default:
            config->EEpromTotalSize = FLEX_NVM_EEPROM_SIZE_FOR_EEESIZE_RESERVED;
            break;
    }

    /* Fill out partitioned DFlash size */
    dataIFRReadOut.FlexNVMPartitionCode &= 0x0FU;
    switch (dataIFRReadOut.FlexNVMPartitionCode)
    {
        case 0x00U:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0000 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0000;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0000 */
            break;
        case 0x01U:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0001 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0001;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0001 */
            break;
        case 0x02U:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0010 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0010;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0010 */
            break;
        case 0x03U:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0011 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0011;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0011 */
            break;
        case 0x04U:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0100 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0100;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0100 */
            break;
        case 0x05U:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0101 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0101;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0101 */
            break;
        case 0x06U:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0110 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0110;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0110 */
            break;
        case 0x07U:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0111 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0111;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_0111 */
            break;
        case 0x08U:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1000 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1000;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1000 */
            break;
        case 0x09U:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1001 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1001;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1001 */
            break;
        case 0x0AU:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1010 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1010;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1010 */
            break;
        case 0x0BU:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1011 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1011;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1011 */
            break;
        case 0x0CU:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1100 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1100;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1100 */
            break;
        case 0x0DU:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1101 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1101;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1101 */
            break;
        case 0x0EU:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1110 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1110;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1110 */
            break;
        case 0x0FU:
#if (FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1111 != 0xFFFFFFFF)
            config->DFlashTotalSize = FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1111;
#else
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
#endif /* FSL_FEATURE_FLASH_FLEX_NVM_DFLASH_SIZE_FOR_DEPART_1111 */
            break;
        default:
            config->DFlashTotalSize = FLEX_NVM_DFLASH_SIZE_FOR_DEPART_RESERVED;
            break;
    }

    return kStatus_FLASH_Success;
}
#endif /* FLASH_SSD_IS_FLEXNVM_ENABLED */

#if defined(FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD) && FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD
/*! @brief Validates the range of the given resource address.*/
static status_t flash_check_resource_range(uint32_t start,
                                           uint32_t lengthInBytes,
                                           uint32_t alignmentBaseline,
                                           flash_read_resource_option_t option)
{
    status_t status;
    uint32_t maxReadbleAddress;

    if ((start & (alignmentBaseline - 1)) || (lengthInBytes & (alignmentBaseline - 1)))
    {
        return kStatus_FLASH_AlignmentError;
    }

    status = kStatus_FLASH_Success;

    maxReadbleAddress = start + lengthInBytes - 1;
    if (option == kFLASH_ResourceOptionVersionId)
    {
        if ((start != kFLASH_ResourceRangeVersionIdStart) ||
            ((start + lengthInBytes - 1) != kFLASH_ResourceRangeVersionIdEnd))
        {
            status = kStatus_FLASH_InvalidArgument;
        }
    }
    else if (option == kFLASH_ResourceOptionFlashIfr)
    {
        if (maxReadbleAddress < kFLASH_ResourceRangePflashIfrSizeInBytes)
        {
        }
#if defined(FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP) && FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP
        else if ((start >= kFLASH_ResourceRangePflashSwapIfrStart) &&
                 (maxReadbleAddress <= kFLASH_ResourceRangePflashSwapIfrEnd))
        {
        }
#endif /* FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP */
        else if ((start >= kFLASH_ResourceRangeDflashIfrStart) &&
                 (maxReadbleAddress <= kFLASH_ResourceRangeDflashIfrEnd))
        {
        }
        else
        {
            status = kStatus_FLASH_InvalidArgument;
        }
    }
    else
    {
        status = kStatus_FLASH_InvalidArgument;
    }

    return status;
}
#endif /* FSL_FEATURE_FLASH_HAS_READ_RESOURCE_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD) && FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD
/*! @brief Validates the gived swap control option.*/
static status_t flash_check_swap_control_option(flash_swap_control_option_t option)
{
    if ((option == kFLASH_SwapControlOptionIntializeSystem) || (option == kFLASH_SwapControlOptionSetInUpdateState) ||
        (option == kFLASH_SwapControlOptionSetInCompleteState) || (option == kFLASH_SwapControlOptionReportStatus) ||
        (option == kFLASH_SwapControlOptionDisableSystem))
    {
        return kStatus_FLASH_Success;
    }

    return kStatus_FLASH_InvalidArgument;
}
#endif /* FSL_FEATURE_FLASH_HAS_SWAP_CONTROL_CMD */

#if defined(FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP) && FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP
/*! @brief Validates the gived address to see if it is equal to swap indicator address in pflash swap IFR.*/
static status_t flash_validate_swap_indicator_address(flash_config_t *config, uint32_t address)
{
    flash_swap_ifr_field_data_t flashSwapIfrFieldData;
    uint32_t swapIndicatorAddress;

    status_t returnCode;
    returnCode =
        FLASH_ReadResource(config, kFLASH_ResourceRangePflashSwapIfrStart, flashSwapIfrFieldData.flashSwapIfrData,
                           sizeof(flashSwapIfrFieldData.flashSwapIfrData), kFLASH_ResourceOptionFlashIfr);

    if (returnCode != kStatus_FLASH_Success)
    {
        return returnCode;
    }

    /* The high bits value of Swap Indicator Address is stored in Program Flash Swap IFR Field,
     * the low severval bit value of Swap Indicator Address is always 1'b0 */
    swapIndicatorAddress = (uint32_t)flashSwapIfrFieldData.flashSwapIfrField.swapIndicatorAddress *
                           FSL_FEATURE_FLASH_PFLASH_SWAP_CONTROL_CMD_ADDRESS_ALIGMENT;
    if (address != swapIndicatorAddress)
    {
        return kStatus_FLASH_SwapIndicatorAddressError;
    }

    return returnCode;
}
#endif /* FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP */

#if defined(FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD) && FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD
/*! @brief Validates the gived flexram function option.*/
static inline status_t flasn_check_flexram_function_option_range(flash_flexram_function_option_t option)
{
    if ((option != kFLASH_FlexramFunctionOptionAvailableAsRam) &&
        (option != kFLASH_FlexramFunctionOptionAvailableForEeprom))
    {
        return kStatus_FLASH_InvalidArgument;
    }

    return kStatus_FLASH_Success;
}
#endif /* FSL_FEATURE_FLASH_HAS_SET_FLEXRAM_FUNCTION_CMD */

/*! @brief Gets the flash protection information (region size, region count).*/
static status_t flash_get_protection_info(flash_config_t *config, flash_protection_config_t *info)
{
    uint32_t pflashTotalSize;

    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Clean up info Structure*/
    memset(info, 0, sizeof(flash_protection_config_t));

/* Note: KW40 has a secondary flash, but it doesn't have independent protection register*/
#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED && (!FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_PROTECTION_REGISTER)
    pflashTotalSize = FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT * FSL_FEATURE_FLASH_PFLASH_BLOCK_SIZE +
                      FSL_FEATURE_FLASH_PFLASH_1_BLOCK_COUNT * FSL_FEATURE_FLASH_PFLASH_1_BLOCK_SIZE;
    info->regionBase = FSL_FEATURE_FLASH_PFLASH_START_ADDRESS;
#else
    pflashTotalSize = config->PFlashTotalSize;
    info->regionBase = config->PFlashBlockBase;
#endif

#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED && FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_PROTECTION_REGISTER
    if (config->FlashMemoryIndex == (uint32_t)kFLASH_MemoryIndexSecondaryFlash)
    {
        info->regionCount = FSL_FEATURE_FLASH_PFLASH_1_PROTECTION_REGION_COUNT;
    }
    else
#endif
    {
        info->regionCount = FSL_FEATURE_FLASH_PFLASH_PROTECTION_REGION_COUNT;
    }

    /* Calculate the size of the flash protection region
     * If the flash density is > 32KB, then protection region is 1/32 of total flash density
     * Else if flash density is < 32KB, then flash protection region is set to 1KB */
    if (pflashTotalSize > info->regionCount * 1024)
    {
        info->regionSize = (pflashTotalSize) / info->regionCount;
    }
    else
    {
        info->regionSize = 1024;
    }

    return kStatus_FLASH_Success;
}

#if defined(FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL) && FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL
/*! @brief Gets the flash Execute-Only access information (Segment size, Segment count).*/
static status_t flash_get_access_info(flash_config_t *config, flash_access_config_t *info)
{
    if (config == NULL)
    {
        return kStatus_FLASH_InvalidArgument;
    }

    /* Clean up info Structure*/
    memset(info, 0, sizeof(flash_access_config_t));

/* Note: KW40 has a secondary flash, but it doesn't have independent access register*/
#if FLASH_SSD_IS_SECONDARY_FLASH_SUPPORTED && (!FLASH_SSD_SECONDARY_FLASH_HAS_ITS_OWN_ACCESS_REGISTER)
    info->SegmentBase = FSL_FEATURE_FLASH_PFLASH_START_ADDRESS;
#else
    info->SegmentBase = config->PFlashBlockBase;
#endif
    info->SegmentSize = config->PFlashAccessSegmentSize;
    info->SegmentCount = config->PFlashAccessSegmentCount;

    return kStatus_FLASH_Success;
}
#endif /* FSL_FEATURE_FLASH_HAS_ACCESS_CONTROL */
