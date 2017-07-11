/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file KW4xXcvrDrv.h
* This is a header file for the transceiver driver interface.
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

#ifndef __KW4X_XCVR_DRV_H__
#define __KW4X_XCVR_DRV_H__

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "EmbeddedTypes.h"
#include "fsl_device_registers.h"

/*! *********************************************************************************
*************************************************************************************
* Constants
*************************************************************************************
********************************************************************************** */
#define channelMapTableSize (128)

#define gPllDenom_c 0x02000000      /* Denominator is a constant value */
#define IQMCalibrationTrials (32)   /* Number of times the calibration is repeated & averaged */
#define IQMCalibrationIter (0x80)   /* Number of iterations per IQMC trial calibration */
#define ENGINEERING_TRIM_BYPASS (0) /* Forces Hardware DCOC calibration to run, regardless of presence of trims in IFR */
  
/* ANA_SPARE Bit Fields */
#define XCVR_ANA_SPARE_HPM_LSB_INVERT_MASK       0xC000u
#define XCVR_ANA_SPARE_HPM_LSB_INVERT_SHIFT      14
#define XCVR_ANA_SPARE_HPM_LSB_INVERT_WIDTH      2
#define XCVR_ANA_SPARE_HPM_LSB_INVERT(x)         (((uint32_t)(((uint32_t)(x))<<XCVR_ANA_SPARE_HPM_LSB_INVERT_SHIFT))&XCVR_ANA_SPARE_HPM_LSB_INVERT_MASK)

/*!
 * @name Register XCVR_ANA_SPARE, field HPM_LSB_INVERT[15:14] (RW)
 *
 * Provides individual inversion settings for the two HPM LSB Array Units
 */
/*@{*/
/*! @brief Read current value of the XCVR_ANA_SPARE_HPM_LSB_INVERT field. */
#define XCVR_RD_ANA_SPARE_HPM_LSB_INVERT(base) ((XCVR_ANA_SPARE_REG(base) & XCVR_ANA_SPARE_HPM_LSB_INVERT_MASK) >> XCVR_ANA_SPARE_HPM_LSB_INVERT_SHIFT)
#define XCVR_BRD_ANA_SPARE_HPM_LSB_INVERT(base) (BME_UBFX32(&XCVR_ANA_SPARE_REG(base), XCVR_ANA_SPARE_HPM_LSB_INVERT_SHIFT, XCVR_ANA_SPARE_HPM_LSB_INVERT_WIDTH))         

/*! @brief Set the HPM_LSB_INVERT field to a new value. */
#define XCVR_WR_ANA_SPARE_HPM_LSB_INVERT(base, value) (XCVR_RMW_ANA_SPARE(base, XCVR_ANA_SPARE_HPM_LSB_INVERT_MASK, XCVR_ANA_SPARE_HPM_LSB_INVERT(value)))
#define XCVR_BWR_ANA_SPARE_HPM_LSB_INVERT(base, value) (BME_BFI32(&XCVR_ANA_SPARE_REG(base), ((uint32_t)(value) << XCVR_ANA_SPARE_HPM_LSB_INVERT_SHIFT), XCVR_ANA_SPARE_HPM_LSB_INVERT_SHIFT, XCVR_ANA_SPARE_HPM_LSB_INVERT_WIDTH))   
/*@}*/

/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */
typedef enum
{
    gXcvrSuccess_c = 0,
    gXcvrInvalidParameters_c,
    gXcvrUnsupportedOperation_c
} xcvrStatus_t;

typedef enum
{
    NO_ERRORS = 0,
    PLL_CTUNE_FAIL = 1,
    PLL_CYCLE_SLIP_FAIL = 2,
    PLL_FREQ_TARG_FAIL = 4,
    PLL_TSM_ABORT_FAIL = 8,
} healthStatus_t;

typedef struct pllChannel_tag
{
    unsigned int integer;
    unsigned int numerator;
} pllChannel_t;

typedef enum
{
    BLE = 0x00,
    ZIGBEE = 0x01,  
    INVALID_MODE = 0xFF
} radio_mode_t;

typedef enum 
{
    NONE = 0,
    FAD_ENABLED = 1,
    LPPS_ENABLED = 2
} FAD_LPPS_CTRL_T;

typedef enum
{
    WRONG_RADIO_ID_DETECTED = 1,
    CALIBRATION_INVALID = 2,
} XCVR_PANIC_ID_T;

typedef void (*panic_fptr)(uint32_t panic_id, uint32_t location, uint32_t extra1, uint32_t extra2);

/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */
#ifdef __cplusplus
extern "C" {
#endif

/* Normal radio functions */
void XcvrInit ( radio_mode_t radioMode ); /* Full XCVR initialization for a radio mode */
xcvrStatus_t XcvrChangeMode ( radio_mode_t radioMode ); /* Change from one radio mode to another */
void XcvrEnaNBRSSIMeas( bool_t IIRnbEnable );
xcvrStatus_t XcvrOverrideFrequency ( uint32_t freq, uint32_t refOsc );
void XcvrRegisterPanicCb ( panic_fptr fptr ); /* allow upper layers to provide PANIC callback */
healthStatus_t XcvrHealthCheck ( void ); /* allow upper layers to poll the radio health */
void XcvrFadLppsControl(FAD_LPPS_CTRL_T control);

/* Customer level trim functions */
xcvrStatus_t XcvrSetXtalTrim(int8_t xtalTrim);
int8_t  XcvrGetXtalTrim(void);

/* Radio debug functions */
xcvrStatus_t XcvrSetGain ( uint8_t entry );
xcvrStatus_t XcvrOverrideChannel ( uint8_t channel, uint8_t useMappedChannel );
uint32_t XcvrGetFreq ( void );
void XcvrForceRxWu ( void );
void XcvrForceRxWd ( void );
void XcvrForceTxWu ( void );
void XcvrForceTxWd ( void );
void XcvrTxTest ( void );
void XcvrIQMCal ( void );       /* Manual IQ Mismatch Calibration */
void XcvrManualDCOCCal (uint8_t chnum); /* Allow tests to run multiple SW DC calibrations */

#ifdef __cplusplus
}
#endif

#endif /* __KW4X_XCVR_DRV_H__ */