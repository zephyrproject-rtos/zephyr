/***************************************************************************//**
* \file cy_smif_memslot.h
* \version 1.20.1
*
* \brief
*  This file provides the constants and parameter values for the memory-level
*  APIs of the SMIF driver.
*
* Note:
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#if !defined(CY_SMIF_MEMORYSLOT_H)
#define CY_SMIF_MEMORYSLOT_H

#include <stdint.h>
#include <stdbool.h>
#include "cy_syslib.h"
#include "cy_device_headers.h"
#include "cy_smif.h"

#if defined(__cplusplus)
extern "C" {
#endif


/**
* \addtogroup group_smif_macros_status
* \{
*/

/***************************************
*            Constants
****************************************/
#define CY_SMIF_DEVICE_BUSY        (1U)    /**< The external memory is busy */
#define CY_SMIF_DEVICE_READY       (0U)    /**< The external memory is ready */

/** \} group_smif_macros_status */

/**
* \addtogroup group_smif_macros_cmd
* \{
*/
#define CY_SMIF_CMD_WITHOUT_PARAM  (0U)    /**< No parameter */
#define CY_SMIF_TX_LAST_BYTE       (1U)    /**< The last byte in the command transmission
                                            * (SS is set high after the transmission)
                                            */
#define CY_SMIF_TX_NOT_LAST_BYTE   (0U)     /**< Not the last byte in the command transmission
                                            * (SS remains low after the transmission)
                                            */
#define CY_SMIF_READ_ONE_BYTE      (1U)     /**< Read 1 byte */
#define CY_SMIF_WRITE_ONE_BYTE     (1U)     /**< Write 1 byte */
#define CY_SMIF_WRITE_TWO_BYTES    (2U)     /**< Write 2 bytes */
#define CY_SMIF_ONE_WORD           (4U)     /**< 4 bytes */

#define CY_SMIF_DUAL_QUAD_DISABLED (0U)     /**< The dual quad transmission mode is disabled */
#define CY_SMIF_DUAL_QUAD_ENABLED  (1U)     /**< The dual quad transmission mode is enabled */


/** \} group_smif_macros_status */

/**
* \addtogroup group_smif_macros_flags
* \{
*/

#define CY_SMIF_FLAG_ALL_DISABLED       (0U) /**< All memory configuration flags are disabled */
/** Enables the write capability for the memory slave in the memory-mapped
 * mode. Valid when the memory-mapped mode is enabled */
#define CY_SMIF_FLAG_WR_EN              (SMIF_DEVICE_CTL_WR_EN_Msk)
/** Determines if the device is memory-mapped. If enabled, this memory slot will
 * be initialized in System init */
#define CY_SMIF_FLAG_MEMORY_MAPPED      (2U)
#define CY_SMIF_FLAG_DETECT_SFDP        (4U) /**< Enables the Autodetect using the SFDP */
/** Enables the crypto support for this memory slave. All access to the
* memory device goes through the encryption/decryption
* Valid when the memory-mapped mode is enabled */
#define CY_SMIF_FLAG_CRYPTO_EN          (SMIF_DEVICE_CTL_CRYPTO_EN_Msk)

/** \} group_smif_macros_flags */

/**
* \addtogroup group_smif_macros_sfdp
* \{
*/

/***************************************
*            SFDP constants
****************************************/
#define CY_SMIF_SFDP_ADDRESS_LENGTH                 (0x03U)                 /**< The length of the SFDP address */
#define CY_SMIF_SFDP_LENGTH                         (0xFFU)                 /**< The length of the SFDP */
#define CY_SMIF_SFDP_SING_BYTE_00                   (0x00U)                 /**< The SFDP Signature byte 0x00. Should be "S" */
#define CY_SMIF_SFDP_SING_BYTE_01                   (0x01U)                 /**< The SFDP Signature byte 0x01. Should be "F" */
#define CY_SMIF_SFDP_SING_BYTE_02                   (0x02U)                 /**< The SFDP Signature byte 0x02. Should be "D" */
#define CY_SMIF_SFDP_SING_BYTE_03                   (0x03U)                 /**< The SFDP Signature byte 0x03. Should be "P" */
#define CY_SMIF_SFDP_MINOR_REV                      (0x04U)                 /**< The SFDP Header byte 0x04. Defines the JEDEC JESD216 Revision */
#define CY_SMIF_SFDP_MAJOR_REV                      (0x05U)                 /**< The SFDP Header byte 0x05. Defines the SFDP Major Revision */
#define CY_SMIF_SFDP_MAJOR_REV_1                    (0x01U)                 /**< The SFDP Major Revision is 1 */
#define CY_SMIF_SFDP_JEDEC_REV_B                    (0x06U)                 /**< The JEDEC JESD216 Revision is B */
#define CY_SMIF_SFDP_PARAM_TABLE_PTR                (0x0CU)                 /**< Specifies the start of the JEDEC Basic Flash
                                                                            * Parameter Table in the SFDP structure
                                                                            */
#define CY_SMIF_SFDP_THREE_BYTES_ADDR_CODE          (0x00U)                 /**< Code for the SFDP Address Bytes Number 3 */
#define CY_SMIF_SFDP_THREE_OR_FOUR_BYTES_ADDR_CODE  (0x01U)                 /**< Code for the SFDP Address Bytes Number 3 or 4 */
#define CY_SMIF_SFDP_FOUR_BYTES_ADDR_CODE           (0x02U)                 /**< Code for the SFDP Address Bytes Number 4 */
#define CY_SMIF_THREE_BYTES_ADDR                    (0x03U)                 /**< The address Bytes Number is 3 */
#define CY_SMIF_FOUR_BYTES_ADDR                     (0x04U)                 /**< The address Bytes Number is 4 */
#define CY_SMIF_READ_MODE_BYTE                      (0x5AU)                 /**< The mode byte for the SMIF read */
#define CY_SMIF_WR_STS_REG1_CMD                     (0x01U)                 /**< The write status register 1 command */
#define CY_SMIF_SINGLE_PROGRAM_CMD                  (0x02U)                 /**< The command for a single SMIF program */
#define CY_SMIF_SINGLE_READ_CMD                     (0x03U)                 /**< The command for a single SMIF read */
#define CY_SMIF_WR_DISABLE_CMD                      (0x04U)                 /**< The Write Disable command */
#define CY_SMIF_RD_STS_REG1_CMD                     (0x05U)                 /**< The read status register 1 command */
#define CY_SMIF_WR_ENABLE_CMD                       (0x06U)                 /**< The Write Enable command */
#define CY_SMIF_RD_STS_REG2_T1_CMD                  (0x35U)                 /**< The read status register 2 type 1 command */
#define CY_SMIF_WR_STS_REG2_CMD                     (0x3EU)                 /**< The write status register 2 command */
#define CY_SMIF_RD_STS_REG2_T2_CMD                  (0x3FU)                 /**< The read status register 2 type 2 command */
#define CY_SMIF_CHIP_ERASE_CMD                      (0x60U)                 /**< The Chip Erase command */
#define CY_SMIF_QE_BIT_STS_REG2_T1                  (0x02U)                 /**< The QE bit is in status register 2 type 1.
                                                                            * It should be written as the second byte.
                                                                            */
#define CY_SMIF_SFDP_ERASE_TIME_1MS                 (1U)                    /**< Units of Erase Typical Time in ms */
#define CY_SMIF_SFDP_ERASE_TIME_16MS                (16U)                   /**< Units of Erase Typical Time in ms */
#define CY_SMIF_SFDP_ERASE_TIME_128MS               (128U)                  /**< Units of Erase Typical Time in ms */
#define CY_SMIF_SFDP_ERASE_TIME_1S                  (1000U)                 /**< Units of Erase Typical Time in ms */
            
#define CY_SMIF_SFDP_CHIP_ERASE_TIME_16MS           (16U)                   /**< Units of Chip Erase Typical Time in ms */
#define CY_SMIF_SFDP_CHIP_ERASE_TIME_256MS          (256U)                  /**< Units of Chip Erase Typical Time in ms */
#define CY_SMIF_SFDP_CHIP_ERASE_TIME_4S             (4000U)                 /**< Units of Chip Erase Typical Time in ms */
#define CY_SMIF_SFDP_CHIP_ERASE_TIME_64S            (64000U)                /**< Units of Chip Erase Typical Time in ms */
            
#define CY_SMIF_SFDP_PROG_TIME_8US                  (8U)                    /**< Units of Page Program Typical Time in us */
#define CY_SMIF_SFDP_PROG_TIME_64US                 (64U)                   /**< Units of Page Program Typical Time in us */
            
#define CY_SMIF_SFDP_UNIT_0                         (0U)                    /**< Units of Basic Flash Parameter Table Time Parameters */
#define CY_SMIF_SFDP_UNIT_1                         (1U)                    /**< Units of Basic Flash Parameter Table Time Parameters */
#define CY_SMIF_SFDP_UNIT_2                         (2U)                    /**< Units of Basic Flash Parameter Table Time Parameters */
#define CY_SMIF_SFDP_UNIT_3                         (3U)                    /**< Units of Basic Flash Parameter Table Time Parameters */


#define CY_SMIF_STS_REG_BUSY_MASK                   (0x01U)                 /**< The busy mask for the status registers */
#define CY_SMIF_NO_COMMAND_OR_MODE                  (0xFFFFFFFFUL)          /**< No command or mode present */
#define CY_SMIF_SFDP_QER_0                          (0x00UL)                /**< The quad Enable Requirements case 0 */
#define CY_SMIF_SFDP_QER_1                          (0x01UL)                /**< The quad Enable Requirements case 1 */
#define CY_SMIF_SFDP_QER_2                          (0x02UL)                /**< The quad Enable Requirements case 2 */
#define CY_SMIF_SFDP_QER_3                          (0x03UL)                /**< The quad Enable Requirements case 3 */
#define CY_SMIF_SFDP_QER_4                          (0x04UL)                /**< The quad Enable Requirements case 4 */
#define CY_SMIF_SFDP_QER_5                          (0x05UL)                /**< The quad Enable Requirements case 5 */
#define CY_SMIF_SFDP_QE_BIT_1_OF_SR_2               (0x02UL)                /**< The QE is bit 1 of the status register 2 */
#define CY_SMIF_SFDP_QE_BIT_6_OF_SR_1               (0x40UL)                /**< The QE is bit 6 of the status register 1 */
#define CY_SMIF_SFDP_QE_BIT_7_OF_SR_2               (0x80UL)                /**< The QE is bit 7 of the status register 2 */
#define CY_SMIF_SFDP_BFPT_BYTE_02                   (0x02U)                 /**< The byte 0x02 of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_04                   (0x04U)                 /**< The byte 0x04 of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_08                   (0x08U)                 /**< The byte 0x08 of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_09                   (0x09U)                 /**< The byte 0x09 of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_0A                   (0x0AU)                 /**< The byte 0x0A of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_0B                   (0x0BU)                 /**< The byte 0x0B of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_0C                   (0x0CU)                 /**< The byte 0x0C of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_0D                   (0x0DU)                 /**< The byte 0x0D of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_0E                   (0x0EU)                 /**< The byte 0x0E of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_0F                   (0x0FU)                 /**< The byte 0x0F of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_1C                   (0x1CU)                 /**< The byte 0x1C of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_1D                   (0x1DU)                 /**< The byte 0x1D of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_28                   (0x28U)                 /**< The byte 0x28 of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_BYTE_3A                   (0x3AU)                 /**< The byte 0x3A of the JEDEC Basic Flash Parameter Table */
#define CY_SMIF_SFDP_BFPT_ERASE_BYTE                (36U)                   /**< The byte 36 of the JEDEC Basic Flash Parameter Table */

#define CY_SMIF_JEDEC_BFPT_10TH_DWORD               (9U)                    /**< Offset to JEDEC Basic Flash Parameter Table: 10th DWORD  */
#define CY_SMIF_JEDEC_BFPT_11TH_DWORD               (10U)                   /**< Offset to JEDEC Basic Flash Parameter Table: 11th DWORD  */

/* ----------------------------  1st DWORD  ---------------------------- */
#define CY_SMIF_SFDP_FAST_READ_1_1_4_Pos            (6UL)                   /**< The SFDP 1-1-4 fast read support (Bit 6)               */
#define CY_SMIF_SFDP_FAST_READ_1_1_4_Msk            (0x40UL)                /**< The SFDP 1-1-4 fast read support (Bitfield-Mask: 0x01) */
#define CY_SMIF_SFDP_FAST_READ_1_4_4_Pos            (5UL)                   /**< The SFDP 1-4-4 fast read support (Bit 5)               */
#define CY_SMIF_SFDP_FAST_READ_1_4_4_Msk            (0x20UL)                /**< The SFDP 1-4-4 fast read support (Bitfield-Mask: 0x01) */
#define CY_SMIF_SFDP_FAST_READ_1_2_2_Pos            (4UL)                   /**< The SFDP 1-2-2 fast read support (Bit 4)               */
#define CY_SMIF_SFDP_FAST_READ_1_2_2_Msk            (0x10UL)                /**< The SFDP 1-2-2 fast read support (Bitfield-Mask: 0x01) */
#define CY_SMIF_SFDP_ADDRESS_BYTES_Pos              (1UL)                   /**< The SFDP number of address bytes (Bit 1)               */
#define CY_SMIF_SFDP_ADDRESS_BYTES_Msk              (0x06UL)                /**< The SFDP number of address bytes (Bitfield-Mask: 0x03) */
#define CY_SMIF_SFDP_FAST_READ_1_1_2_Pos            (0UL)                   /**< The SFDP 1-1-2 fast read support (Bit 0)               */
#define CY_SMIF_SFDP_FAST_READ_1_1_2_Msk            (0x01UL)                /**< The SFDP 1-1-2 fast read support (Bitfield-Mask: 0x01) */

/* ----------------------------  2nd DWORD  ---------------------------- */
#define CY_SMIF_SFDP_SIZE_ABOVE_4GB_Msk             (0x80000000UL)          /**< Flash memory density bit define if it >= 4 Gbit  or <= 2Gbit*/

/* ----------------------------  3rd DWORD  ---------------------------- */
#define CY_SMIF_SFDP_1_4_4_DUMMY_CYCLES_Pos         (0UL)                   /**< The SFDP number of 1-4-4 fast read dummy cycles (Bit 0)               */
#define CY_SMIF_SFDP_1_4_4_DUMMY_CYCLES_Msk         (0x1FUL)                /**< The SFDP number of 1-4-4 fast read dummy cycles (Bitfield-Mask: 0x1F) */
#define CY_SMIF_SFDP_1_4_4_MODE_CYCLES_Pos          (5UL)                   /**< The SFDP number of 1-4-4 fast read mode cycles (Bit 5)                */
#define CY_SMIF_SFDP_1_4_4_MODE_CYCLES_Msk          (0xE0UL)                /**< The SFDP number of 1-4-4 fast read mode cycles (Bitfield-Mask: 0x07)  */
#define CY_SMIF_SFDP_1_1_4_DUMMY_CYCLES_Pos         (0UL)                   /**< The SFDP number of 1-1-4 fast read dummy cycles (Bit 0)               */
#define CY_SMIF_SFDP_1_1_4_DUMMY_CYCLES_Msk         (0x1FUL)                /**< The SFDP number of 1-1-4 fast read dummy cycles (Bitfield-Mask: 0x1F) */
#define CY_SMIF_SFDP_1_1_4_MODE_CYCLES_Pos          (5UL)                   /**< The SFDP number of 1-1-4 fast read mode cycles (Bit 5)                */
#define CY_SMIF_SFDP_1_1_4_MODE_CYCLES_Msk          (0xE0UL)                /**< The SFDP number of 1-1-4 fast read mode cycles (Bitfield-Mask: 0x07)  */

/* ----------------------------  4th DWORD  ---------------------------- */
#define CY_SMIF_SFDP_1_1_2_DUMMY_CYCLES_Pos         (0UL)                   /**< The SFDP number of 1_1_2 fast read dummy cycles (Bit 0)               */
#define CY_SMIF_SFDP_1_1_2_DUMMY_CYCLES_Msk         (0x1FUL)                /**< The SFDP number of 1_1_2 fast read dummy cycles (Bitfield-Mask: 0x1F) */
#define CY_SMIF_SFDP_1_1_2_MODE_CYCLES_Pos          (5UL)                   /**< The SFDP number of 1_1_2 fast read mode cycles (Bit 5)                */
#define CY_SMIF_SFDP_1_1_2_MODE_CYCLES_Msk          (0xE0UL)                /**< The SFDP number of 1_1_2 fast read mode cycles (Bitfield-Mask: 0x07)  */
#define CY_SMIF_SFDP_1_2_2_DUMMY_CYCLES_Pos         (0UL)                   /**< The SFDP number of 1_2_2 fast read dummy cycles (Bit 0)               */
#define CY_SMIF_SFDP_1_2_2_DUMMY_CYCLES_Msk         (0x1FUL)                /**< The SFDP number of 1_2_2 fast read dummy cycles (Bitfield-Mask: 0x1F) */
#define CY_SMIF_SFDP_1_2_2_MODE_CYCLES_Pos          (5UL)                   /**< The SFDP number of 1_2_2 fast read mode cycles (Bit 5)                */
#define CY_SMIF_SFDP_1_2_2_MODE_CYCLES_Msk          (0xE0UL)                /**< The SFDP number of 1_2_2 fast read mode cycles (Bitfield-Mask: 0x07)  */

/* ----------------------------  10th DWORD  --------------------------- */
#define CY_SMIF_SFDP_ERASE_T1_COUNT_Pos             (4UL)                   /**< Erase Type 1 Erase, Typical time: count (Bits 8:4) */
#define CY_SMIF_SFDP_ERASE_T1_COUNT_Msk             (0x1F0UL)               /**< Erase Type 1 Erase, Typical time: count (Bitfield-Mask ) */
#define CY_SMIF_SFDP_ERASE_T1_UNITS_Pos             (9UL)                   /**< Erase Type 1 Erase, Typical time: units (Bits 10:9) */
#define CY_SMIF_SFDP_ERASE_T1_UNITS_Msk             (0x600UL)               /**< Erase Type 1 Erase, Typical time: units (Bitfield-Mask ) */
#define CY_SMIF_SFDP_ERASE_MUL_COUNT_Pos            (0UL)                   /**< Multiplier from typical erase time to maximum erase time (Bits 3:0) */
#define CY_SMIF_SFDP_ERASE_MUL_COUNT_Msk            (0x0FUL)                /**< Multiplier from typical erase time to maximum erase time (Bitfield-Mask ) */


/* ----------------------------  11th DWORD  --------------------------- */
#define CY_SMIF_SFDP_PAGE_SIZE_Pos                  (4UL)                   /**< The SFDP page size (Bit 4)                                    */
#define CY_SMIF_SFDP_PAGE_SIZE_Msk                  (0xF0UL)                /**< The SFDP page size (Bitfield-Mask: 0x0F)                      */
#define CY_SMIF_SFDP_PAGE_PROG_COUNT_Pos            (8UL)                   /**< The SFDP Chip Page Program Typical time: count (Bits 12:8)    */
#define CY_SMIF_SFDP_PAGE_PROG_COUNT_Msk            (0x1F00UL)              /**< The SFDP Chip Page Program Typical time: count (Bitfield-Mask)*/
#define CY_SMIF_SFDP_PAGE_PROG_UNITS_Pos            (13UL)                  /**< The SFDP Chip Page Program Typical time: units (Bit 13)       */
#define CY_SMIF_SFDP_PAGE_PROG_UNITS_Msk            (0x2000UL)              /**< The SFDP Chip Page Program Typical time: units (Bitfield-Mask)*/
#define CY_SMIF_SFDP_CHIP_ERASE_COUNT_Pos           (24UL)                  /**< The SFDP Chip Erase Typical time: count (Bits 28:24)          */
#define CY_SMIF_SFDP_CHIP_ERASE_COUNT_Msk           (0x1F000000UL)          /**< The SFDP Chip Erase Typical time: count (Bitfield-Mask)       */
#define CY_SMIF_SFDP_CHIP_ERASE_UNITS_Pos           (29UL)                  /**< The SFDP Chip Erase Typical time: units (Bits 29:30)          */
#define CY_SMIF_SFDP_CHIP_ERASE_UNITS_Msk           (0x60000000UL)          /**< The SFDP Chip Erase Typical time: units (Bitfield-Mask)       */
#define CY_SMIF_SFDP_PROG_MUL_COUNT_Pos             (0UL)                   /**< Multiplier from typical time to max time for Page or byte program (Bits 3:0)          */
#define CY_SMIF_SFDP_PROG_MUL_COUNT_Msk             (0x0FUL)                /**< Multiplier from typical time to max time for Page or byte program (Bitfield-Mask)       */

/* ----------------------------  15th DWORD  --------------------------- */
#define CY_SMIF_SFDP_QE_REQUIREMENTS_Pos            (4UL)                   /**< The SFDP quad enable requirements field (Bit 4)               */
#define CY_SMIF_SFDP_QE_REQUIREMENTS_Msk            (0x70UL)                /**< The SFDP quad enable requirements field (Bitfield-Mask: 0x07) */

/** \cond INTERNAL */

#define CY_SMIF_BYTES_IN_WORD                   (4U)
#define CY_SMIF_BITS_IN_BYTE                    (8U)
#define CY_SMIF_BITS_IN_BYTE_ABOVE_4GB          (3U)                        /** density of memory above 4GBit stored as poser of 2 */


#define CY_SMIF_MEM_ADDR_VALID(addr, size)  (0U == ((addr)%(size)))   /* This address must be a multiple of the SMIF XIP memory size */
#define CY_SMIF_MEM_MAPPED_SIZE_VALID(size) (((size) >= 0x10000U) && (0U == ((size)&((size)-1U))) ) /* must be a power of 2 and greater or equal than 64 KB */
#define CY_SMIF_MEM_ADDR_SIZE_VALID(addrSize)   ((0U < (addrSize)) && ((addrSize) <= 4U))


/** \endcond*/
/** \} group_smif_macros_sfdp */


/**
* \addtogroup group_smif_data_structures_memslot
* \{
*/

/** This command structure is used to store the Read/Write command
 * configuration. */
typedef struct
{
    uint32_t command;                       /**< The 8-bit command. This value is 0xFFFFFFFF when there is no command present */
    cy_en_smif_txfr_width_t cmdWidth;       /**< The width of the command transfer */
    cy_en_smif_txfr_width_t addrWidth;      /**< The width of the address transfer */
    uint32_t mode;                          /**< The 8-bit mode byte. This value is 0xFFFFFFFF when there is no mode present */
    cy_en_smif_txfr_width_t modeWidth;      /**< The width of the mode transfer */
    uint32_t dummyCycles;                   /**< The number of the dummy cycles. A zero value suggests no dummy cycles */
    cy_en_smif_txfr_width_t dataWidth;      /**< The width of the data transfer */
} cy_stc_smif_mem_cmd_t;


/**
*
* This configuration structure of the SMIF memory device is used to store
* device-specific parameters.
* These parameters are used to set up the memory mode initialization and the
* memory API.
*/
typedef struct
{
    uint32_t numOfAddrBytes;                    /**< This specifies the number of address bytes used by the 
                                                 * memory slave device, valid values 1-4 */
    uint32_t memSize;                           /**< The size of the memory */
    cy_stc_smif_mem_cmd_t* readCmd;             /**< This specifies the Read command */
    cy_stc_smif_mem_cmd_t* writeEnCmd;          /**< This specifies the Write Enable command */
    cy_stc_smif_mem_cmd_t* writeDisCmd;         /**< This specifies the Write Disable command */
    cy_stc_smif_mem_cmd_t* eraseCmd;            /**< This specifies the Erase command */
    uint32_t eraseSize;                         /**< This specifies the sector size of each Erase */
    cy_stc_smif_mem_cmd_t* chipEraseCmd;        /**< This specifies the Chip Erase command */
    cy_stc_smif_mem_cmd_t* programCmd;          /**< This specifies the Program command */
    uint32_t programSize;                       /**< This specifies the page size for programming */
    cy_stc_smif_mem_cmd_t* readStsRegWipCmd;    /**< This specifies the command to read the WIP-containing status register  */
    cy_stc_smif_mem_cmd_t* readStsRegQeCmd;     /**< This specifies the command to read the QE-containing status register */
    cy_stc_smif_mem_cmd_t* writeStsRegQeCmd;    /**< This specifies the command to write into the QE-containing status register */
    cy_stc_smif_mem_cmd_t* readSfdpCmd;         /**< This specifies the read SFDP command */
    uint32_t stsRegBusyMask;                    /**< The Busy mask for the status registers */
    uint32_t stsRegQuadEnableMask;              /**< The QE mask for the status registers */
    uint32_t eraseTime;                         /**< Max time for erase type 1 cycle time in ms */
    uint32_t chipEraseTime;                     /**< Max time for chip erase cycle time in ms */
    uint32_t programTime;                       /**< Max time for page program cycle time in us */
} cy_stc_smif_mem_device_cfg_t;


/**
*
* This SMIF memory configuration structure is used to store the memory configuration for the memory mode of operation.
* This data structure is stored in a fixed location in the flash. The data structure is required
* for the initialization of the SMIF in the SystemInit.
*/
typedef struct
{
    /** Determines the slave select where the memory device is placed */
    cy_en_smif_slave_select_t slaveSelect;
    /** Determines if the device is memory-mapped, enables the Autodetect
     * using the SFDP, enables the write capability, or enables the crypto
     * support for this memory slave */
    uint32_t flags;
    /** The data-line selection options for a slave device */
    cy_en_smif_data_select_t dataSelect;
    /** The base address the memory slave is mapped to in the PSoC memory map.
     * This address must be a multiple of the SMIF XIP memory size/capacity. The
     * SMIF XIP memory region should NOT overlap with other memory regions
     * (with exception to dual quad mode). Valid when the memory-mapped mode is
     * enabled.
     */
    uint32_t baseAddress;
    /** The size/capacity allocated in the PSoC memory map for the memory slave
     * device. The capacity is allocated from the base address. The capacity
     * must be a power of 2 and greater or equal than 64 KB. Valid when
     * memory-mapped mode is enabled
     */
    uint32_t memMappedSize;
    /** Defines if this memory device is one of the devices in the dual quad SPI
     * configuration. Equals the sum of the slave-slot numbers.  */
    uint32_t dualQuadSlots;
    cy_stc_smif_mem_device_cfg_t* deviceCfg;   /**< The configuration of the device */
} cy_stc_smif_mem_config_t;


/**
*
* This SMIF memory configuration structure is used to store the memory configuration for the memory mode of operation.
* This data structure is stored in a fixed location in the flash. The data structure is required
* for the initialization of the SMIF in the SystemInit.
*/
typedef struct
{
    uint32_t memCount;                         /**< The number of SMIF memory defined  */
    cy_stc_smif_mem_config_t** memConfig;      /**< The pointer to the array of the memory configuration structures of size Memory_count */
    uint32_t majorVersion;                     /**< The version of the SMIF driver */
    uint32_t minorVersion;                     /**< The version of the SMIF Driver */
} cy_stc_smif_block_config_t;


/** \} group_smif_data_structures_memslot */


/**
* \addtogroup group_smif_mem_slot_functions
* \{
*/
cy_en_smif_status_t    Cy_SMIF_Memslot_Init(SMIF_Type *base,
                                cy_stc_smif_block_config_t * const blockConfig,
                                cy_stc_smif_context_t *context);
void        Cy_SMIF_Memslot_DeInit(SMIF_Type *base);
cy_en_smif_status_t    Cy_SMIF_Memslot_CmdWriteEnable( SMIF_Type *base,
                                        cy_stc_smif_mem_config_t const *memDevice,
                                        cy_stc_smif_context_t const *context);
cy_en_smif_status_t    Cy_SMIF_Memslot_CmdWriteDisable(SMIF_Type *base,
                                         cy_stc_smif_mem_config_t const *memDevice,
                                         cy_stc_smif_context_t const *context);
bool Cy_SMIF_Memslot_IsBusy(SMIF_Type *base, cy_stc_smif_mem_config_t *memDevice,
                                    cy_stc_smif_context_t const *context);
cy_en_smif_status_t    Cy_SMIF_Memslot_QuadEnable(SMIF_Type *base,
                                        cy_stc_smif_mem_config_t *memDevice,
                                        cy_stc_smif_context_t const *context);
cy_en_smif_status_t    Cy_SMIF_Memslot_CmdReadSts(SMIF_Type *base,
                                        cy_stc_smif_mem_config_t const *memDevice,
                                        uint8_t *status, uint8_t command,
                                        cy_stc_smif_context_t const *context);
cy_en_smif_status_t    Cy_SMIF_Memslot_CmdWriteSts(SMIF_Type *base,
                                        cy_stc_smif_mem_config_t const *memDevice,
                                        void const *status, uint8_t command,
                                        cy_stc_smif_context_t const *context);
cy_en_smif_status_t    Cy_SMIF_Memslot_CmdChipErase(SMIF_Type *base,
                                        cy_stc_smif_mem_config_t const *memDevice,
                                        cy_stc_smif_context_t const *context);
cy_en_smif_status_t    Cy_SMIF_Memslot_CmdSectorErase(SMIF_Type *base,
                                            cy_stc_smif_mem_config_t *memDevice,
                                            uint8_t const *sectorAddr,
                                            cy_stc_smif_context_t const *context);
cy_en_smif_status_t    Cy_SMIF_Memslot_CmdProgram(SMIF_Type *base,
                                    cy_stc_smif_mem_config_t const *memDevice,
                                    uint8_t const *addr,
                                    uint8_t *writeBuff,
                                    uint32_t size,
                                    cy_smif_event_cb_t cmdCmpltCb,
                                    cy_stc_smif_context_t *context);
cy_en_smif_status_t    Cy_SMIF_Memslot_CmdRead(SMIF_Type *base,
                                    cy_stc_smif_mem_config_t const *memDevice,
                                    uint8_t const *addr,
                                    uint8_t *readBuff,
                                    uint32_t size,
                                    cy_smif_event_cb_t cmdCmpltCb,
                                    cy_stc_smif_context_t *context);
cy_en_smif_status_t    Cy_SMIF_Memslot_SfdpDetect(SMIF_Type *base,
                                    cy_stc_smif_mem_device_cfg_t *device,
                                    cy_en_smif_slave_select_t slaveSelect,
                                    cy_en_smif_data_select_t dataSelect,
                                    cy_stc_smif_context_t *context);


/** \} group_smif_mem_slot_functions */


#if defined(__cplusplus)
}
#endif

#endif /* (CY_SMIF_MEMORYSLOT_H) */


/* [] END OF FILE */
