/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file ifr_mkw40z4_radio.h
* Header file for the MKW40Z4 Radio IFR bits handling.
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

#ifndef __IFR_MKW40Z4_RADIO_H__
#define __IFR_MKW40Z4_RADIO_H__

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include <stdint.h>
#include "EmbeddedTypes.h"

/*! *********************************************************************************
*************************************************************************************
* Macros
*************************************************************************************
********************************************************************************** */
#define IFR_EOF_SYMBOL          (0xFEED0E0F) /* Denotes the "End of File" for IFR data */
#define IFR_VERSION_HDR         (0xABCD0000) /* Constant value for upper 16 bits of IFR data header */
#define IFR_VERSION_MASK        (0x0000FFFF) /* Mask for version number (lower 16 bits) of IFR data header */
#define IFR_SW_ID_MIN           (0x00000000) /* Lower limit of SW trim IDs */
#define IFR_SW_ID_MAX           (0x0000FFFF) /* Lower limit of SW trim IDs */

#define IS_A_SW_ID(x)           ((IFR_SW_ID_MIN<x) && (IFR_SW_ID_MAX>=x))
#define IS_VALID_REG_ADDR(x)    (((x)&0xFFFF0000) == 0x40050000)  /* Valid addresses are 0x4005xxxx */

#define MAKE_MASK(size)         ((1<<(size))-1)
#define MAKE_MASKSHFT(size,bitpos)  (MAKE_MASK(size)<<bitpos)
#define RDRSRC                  0x03 

#define IFR_TZA_CAP_TUNE_MASK   (0x0000000F)
#define IFR_TZA_CAP_TUNE_SHIFT  (0)
#define IFR_BBF_CAP_TUNE_MASK   (0x000F0000)
#define IFR_BBF_CAP_TUNE_SHIFT  (16)
#define IFR_RES_TUNE2_MASK      (0x00F00000)
#define IFR_RES_TUNE2_SHIFT     (20)

/*! *********************************************************************************
*************************************************************************************
* Enums
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
* \var    typedef uint8_t IFR_ERROR_T
*
* \brief  The IFR error reporting type.
*
*     See #IFR_ERROR_T_enum for the enumeration definitions.
*
********************************************************************************** */
typedef uint8_t IFR_ERROR_T;

/*! *********************************************************************************
* \brief  The enumerations used to describe IFR errors.
*
********************************************************************************** */
enum IFR_ERROR_T_enum
{
    IFR_SUCCESS = 0,
    INVALID_POINTER = 1,         /* NULL pointer error */
    INVALID_DEST_SIZE_SHIFT = 2, /* The bits won't fit as specified in the destination */
};

/*! *********************************************************************************
* \var    typedef uint16_t SW_TRIM_ID_T
*
* \brief  The SW trim ID type.
*
*     See #SW_TRIM_ID_T_enum for the enumeration definitions.
*
********************************************************************************** */
typedef uint16_t SW_TRIM_ID_T;

/*! *********************************************************************************
* \brief  The enumerations used to define SW trim IDs.
*
********************************************************************************** */
enum SW_TRIM_ID_T_enum
{
    Q_RELATIVE_GAIN_BY_PART = 0, /* Q vs I relative gain trim ID */
    ADC_GAIN = 1,                /* ADC gain trim ID */
    ZB_FILT_TRIM = 2,            /* Baseband Bandwidth filter trim ID for BLE */
    BLE_FILT_TRIM = 3,           /* Baseband Bandwidth filter trim ID for BLE */
    TRIM_STATUS = 4,             /* Status result of the trim process (error indications) */
    TRIM_VERSION = 0xABCD,       /* Version number of the IFR trim algorithm/format. */
};

/*! *********************************************************************************
* \var    typedef uint32_t IFR_TRIM_STATUS_T
*
* \brief  The definition of failure bits stored in IFR trim status word.
*
*     See #IFR_TRIM_STATUS_T_enum for the enumeration definitions.
*
********************************************************************************** */
typedef uint32_t IFR_TRIM_STATUS_T;

/*! *********************************************************************************
* \brief  The enumerations used to describe trim algorithm failures in the status entry in IFR.
*         This enum represents multiple values which can be OR'd together in a single status word.
*
********************************************************************************** */
enum IFR_TRIM_STATUS_T_enum
{
    TRIM_ALGORITHM_SUCCESS = 0,
    BGAP_VOLTAGE_TRIM_FAILED = 1, /* Algorithm failure in BGAP voltagetrim */
    IQMC_GAIN_ADJ_FAILED = 2,     /* Algorithm failure in IQMC gain trim */
    IQMC_PHASE_ADJ_FAILED = 4,    /* Algorithm failure in IQMC phase trim */
    IQMC_DC_GAIN_ADJ_FAILED = 8,
    ADC_GAIN_TRIM_FAILED = 10,
    ZB_FILT_TRIM_FAILED = 20,
    BLE_FILT_TRIM_FAILED = 40,
};

/*! *********************************************************************************
*************************************************************************************
* Structures
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
* \var   typedef struct IFR_SW_TRIM_TBL_ENTRY_T
*
* \brief  Structure defining an entry in a table used to contain values to be passed back from IFR
*         handling routine to XCVR driver software.  
*
********************************************************************************** */
typedef struct
{
    SW_TRIM_ID_T trim_id;  /* The assigned ID */
    uint32_t trim_value;   /* The value fetched from IFR. */
    uint8_t valid;         /* Validity of the trim_value field after IFR processing is complete (TRUE/FALSE). */
} IFR_SW_TRIM_TBL_ENTRY_T;

/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */
uint32_t read_resource_ifr(uint32_t read_addr); /* Reads IFR and it appropriate location */
uint32_t read_resource(uint16_t resource_id);
void handle_ifr(IFR_SW_TRIM_TBL_ENTRY_T * sw_trim_tbl, uint16_t num_entries);
uint32_t handle_ifr_die_id(void);
uint32_t handle_ifr_die_kw_type(void);
void dump_ifr(uint32_t * dump_tbl, uint8_t num_entries);

#endif /* __IFR_MKW40Z4_RADIO_H__ */