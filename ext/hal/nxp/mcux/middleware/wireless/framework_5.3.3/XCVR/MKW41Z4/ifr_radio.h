/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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
#ifndef __IFR_RADIO_H__
/* clang-format off */
#define __IFR_RADIO_H__
/* clang-format on */

#include <stdint.h>
/* clang-format off */
#define _FSL_XCVR_H_
/* clang-format on */

/*!
 * @addtogroup xcvr
 * @{
 */

/*! @file*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define IFR_EOF_SYMBOL          (0xFEED0E0FU) /* < Denotes the "End of File" for IFR data */
#define IFR_VERSION_HDR         (0xABCD0000U) /* < Constant value for upper 16 bits of IFR data header */
#define IFR_VERSION_MASK        (0x0000FFFFU) /* < Mask for version number (lower 16 bits) of IFR data header */
#define IFR_SW_ID_MIN           (0x00000000U) /* < Lower limit of SW trim IDs */
#define IFR_SW_ID_MAX           (0x0000FFFFU) /* < Lower limit of SW trim IDs */

#define IS_A_SW_ID(x)           ((IFR_SW_ID_MIN < (x)) && (IFR_SW_ID_MAX >= (x)))

/* K3 valid registers support */
#if (defined(CPU_K32W042S1M2CAx_M0P) || defined(CPU_K32W042S1M2VPJ_M0P))
#define IS_VALID_REG_ADDR(x)    (((x) & 0xFFFF0000U) == 0x41000000U) /* Valid addresses are 0x410xxxxx */
#endif /* (defined(CPU_K32W042S1M2CAx_M0P) || defined(CPU_K32W042S1M2VPJ_M0P)) */
/* KW41 and KW35/36 valid registers support */
#if (defined(CPU_MKW41Z256VHT4) || defined(CPU_MKW41Z512VHT4) || \
     defined(CPU_MKW31Z256VHT4) || defined(CPU_MKW31Z512VHT4) || \
     defined(CPU_MKW21Z256VHT4) || defined(CPU_MKW21Z512VHT4) || \
     defined(CPU_MKW35A512VFP4) || defined(CPU_MKW36A512VFP4) )

#define IS_VALID_REG_ADDR(x)    (((x) & 0xFFFF0000U) == 0x40050000U) /* Valid addresses are 0x4005xxxx */
#endif

#define MAKE_MASK(size)         ((1 << (size)) - 1)
#define MAKE_MASKSHFT(size, bitpos)  (MAKE_MASK(size) << (bitpos))

#define IFR_TZA_CAP_TUNE_MASK           (0x0000000FU)
#define IFR_TZA_CAP_TUNE_SHIFT          (0)
#define IFR_BBF_CAP_TUNE_MASK           (0x000F0000U)
#define IFR_BBF_CAP_TUNE_SHIFT          (16)
#define IFR_RES_TUNE2_MASK              (0x00F00000U)
#define IFR_RES_TUNE2_SHIFT             (20)

/* \var  typedef uint8_t IFR_ERROR_T */
/* \brief  The IFR error reporting type. */
/* See #IFR_ERROR_T_enum for the enumeration definitions. */
typedef uint8_t IFR_ERROR_T;

/* \brief  The enumerations used to describe IFR errors. */
enum IFR_ERROR_T_enum
{
    IFR_SUCCESS = 0,
    INVALID_POINTER = 1, /* < NULL pointer error */
    INVALID_DEST_SIZE_SHIFT = 2, /* < the bits won't fit as specified in the destination */
};

/* \var  typedef uint16_t SW_TRIM_ID_T */
/* \brief  The SW trim ID type. */
/* See #SW_TRIM_ID_T_enum for the enumeration definitions. */
typedef uint16_t SW_TRIM_ID_T;

/* \brief  The enumerations used to define SW trim IDs. */
enum SW_TRIM_ID_T_enum
{
    Q_RELATIVE_GAIN_BY_PART = 0, /* < Q vs I relative gain trim ID */
    ADC_GAIN = 1, /* < ADC gain trim ID */
    ZB_FILT_TRIM = 2, /* < Baseband Bandwidth filter trim ID for BLE */
    BLE_FILT_TRIM = 3, /* < Baseband Bandwidth filter trim ID for BLE */
    TRIM_STATUS = 4, /* < Status result of the trim process (error indications) */
    TRIM_VERSION = 0xABCD, /* < Version number of the IFR trim algorithm/format. */
};

/* \var  typedef uint32_t IFR_TRIM_STATUS_T */
/* \brief  The definition of failure bits stored in IFR trim status word. */
/* See #IFR_TRIM_STATUS_T_enum for the enumeration definitions. */
typedef uint32_t IFR_TRIM_STATUS_T;

/* \brief  The enumerations used to describe trim algorithm failures in the status entry in IFR. */
/* This enum represents multiple values which can be OR'd together in a single status word. */
enum IFR_TRIM_STATUS_T_enum
{
    TRIM_ALGORITHM_SUCCESS = 0,
    BGAP_VOLTAGE_TRIM_FAILED = 1, /* < algorithm failure in BGAP voltagetrim */
    IQMC_GAIN_ADJ_FAILED = 2, /* < algorithm failure in IQMC gain trim */
    IQMC_PHASE_ADJ_FAILED = 4, /* < algorithm failure in IQMC phase trim */
    IQMC_DC_GAIN_ADJ_FAILED = 8, /* < */
    ADC_GAIN_TRIM_FAILED = 10, /* <*/
    ZB_FILT_TRIM_FAILED = 20, /* < */
    BLE_FILT_TRIM_FAILED = 40, /* < */
};

/* \var  typedef struct IFR_SW_TRIM_TBL_ENTRY_T */
/* \brief  Structure defining an entry in a table used to contain values to be passed back from IFR */
/*  handling routine to XCVR driver software. */
typedef struct
{
    SW_TRIM_ID_T trim_id; /* < The assigned ID */
    uint32_t trim_value; /* < The value fetched from IFR.*/
    uint8_t valid; /* < validity of the trim_value field after IFR processing is complete (TRUE/FALSE).*/
} IFR_SW_TRIM_TBL_ENTRY_T;

/*******************************************************************************
 * API
 ******************************************************************************/
/*!
 * @brief Reads a location in block 1 IFR for use by the radio.
 *
 * This function handles reading IFR data from flash memory for trim loading.
 *
 * @param read_addr the address in the IFR to be read.
 */ 
uint32_t read_resource_ifr(uint32_t read_addr);

/*!
 * @brief Reads a location in a simulated data array to support IFR handler testing.
 *
 * This function handles reading  data from a const table for testing the trim loading functions.
 *
 * @param read_addr the address in the IFR to be read.
 */ 
uint32_t read_resource(uint16_t resource_id);

/*!
 * @brief Main IFR handler function called by XCVR driver software to process trim table.
 *
 * This function handles reading  data from IFR and either loading to registers or storing to a SW trim values table.
 *
 * @param sw_trim_tbl pointer to the table used to store software trim values.
 * @param num_entries the number of entries that can be stored in the SW trim table.
 */ 
void handle_ifr(IFR_SW_TRIM_TBL_ENTRY_T * sw_trim_tbl, uint16_t num_entries);

/*!
 * @brief Handler function to read die_id from IFR locations..
 *
 * This function handles reading die ID value for debug and testing usage.
 *
 * @return the value of the die ID field.
 */ 
uint32_t handle_ifr_die_id(void);

/*!
 * @brief Handler function to read KW chip version from IFR locations..
 *
 * This function handles reading KW chip version for debug and testing usage.
 *
 * @return the value of the KW version field.
 */ 
uint32_t handle_ifr_die_kw_type(void);

/*!
 * @brief Debug function to dump the IFR contents to a RAM array.
 *
 * This function handles reading  data from IFR and storing to a RAM array for debug.
 *
 * @param dump_tbl pointer to the table used to store IFR entry values.
 * @param num_entries the number of entries that can be stored in the dump table.
 */ 
void dump_ifr(uint32_t * dump_tbl, uint8_t num_entries);

#endif /*__IFR_RADIO_H__ */

