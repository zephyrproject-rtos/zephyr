/******************************************************************************
*  Filename:       aux_tdc.h
*  Revised:        2017-06-05 12:13:49 +0200 (Mon, 05 Jun 2017)
*  Revision:       49096
*
*  Description:    Defines and prototypes for the AUX Time-to-Digital Converter
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

//*****************************************************************************
//
//! \addtogroup aux_group
//! @{
//! \addtogroup auxtdc_api
//! @{
//
//*****************************************************************************

#ifndef __AUX_TDC_H__
#define __AUX_TDC_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_ints.h"
#include "../inc/hw_aux_tdc.h"
#include "debug.h"

//*****************************************************************************
//
// Support for DriverLib in ROM:
// This section renames all functions that are not "static inline", so that
// calling these functions will default to implementation in flash. At the end
// of this file a second renaming will change the defaults to implementation in
// ROM for available functions.
//
// To force use of the implementation in flash, e.g. for debugging:
// - Globally: Define DRIVERLIB_NOROM at project level
// - Per function: Use prefix "NOROM_" when calling the function
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #define AUXTDCConfigSet                 NOROM_AUXTDCConfigSet
    #define AUXTDCMeasurementDone           NOROM_AUXTDCMeasurementDone
#endif

//*****************************************************************************
//
// Defines for the status of a AUX TDC measurement.
//
//*****************************************************************************
#define AUX_TDC_BUSY            0x00000001
#define AUX_TDC_TIMEOUT         0x00000002
#define AUX_TDC_DONE            0x00000004

//*****************************************************************************
//
// Defines for the control of a AUX TDC.
//
//*****************************************************************************
#define AUX_TDC_RUNSYNC         0x00000001
#define AUX_TDC_RUN             0x00000002
#define AUX_TDC_ABORT           0x00000003

//*****************************************************************************
//
// Defines for possible states of the TDC internal state machine.
//
//*****************************************************************************
#define AUXTDC_WAIT_START         (AUX_TDC_STAT_STATE_WAIT_START)
#define AUXTDC_WAIT_START_CNTEN   (AUX_TDC_STAT_STATE_WAIT_START_STOP_CNT_EN)
#define AUXTDC_IDLE               (AUX_TDC_STAT_STATE_IDLE)
#define AUXTDC_CLRCNT             (AUX_TDC_STAT_STATE_CLR_CNT)
#define AUXTDC_WAIT_STOP          (AUX_TDC_STAT_STATE_WAIT_STOP)
#define AUXTDC_WAIT_STOP_CNTDOWN  (AUX_TDC_STAT_STATE_WAIT_STOP_CNTDWN)
#define AUXTDC_GETRESULTS         (AUX_TDC_STAT_STATE_GET_RESULT)
#define AUXTDC_POR                (AUX_TDC_STAT_STATE_POR)
#define AUXTDC_WAIT_CLRCNT_DONE   (AUX_TDC_STAT_STATE_WAIT_CLR_CNT_DONE)
#define AUXTDC_START_FALL         (AUX_TDC_STAT_STATE_START_FALL)
#define AUXTDC_FORCE_STOP         (AUX_TDC_STAT_STATE_FORCE_STOP)

//*****************************************************************************
//
// Defines for controlling the AUX TDC. Values can be passed to AUXTDCConfigSet().
//
//*****************************************************************************
#define AUXTDC_STOPPOL_RIS                (AUX_TDC_TRIGSRC_STOP_POL_HIGH) // Rising edge polarity for stop event
#define AUXTDC_STOPPOL_FALL               (AUX_TDC_TRIGSRC_STOP_POL_LOW)  // Falling edge polarity for stop event

#define AUXTDC_STOP_AUXIO0                (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO0)
#define AUXTDC_STOP_AUXIO1                (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO1)
#define AUXTDC_STOP_AUXIO2                (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO2)
#define AUXTDC_STOP_AUXIO3                (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO3)
#define AUXTDC_STOP_AUXIO4                (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO4)
#define AUXTDC_STOP_AUXIO5                (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO5)
#define AUXTDC_STOP_AUXIO6                (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO6)
#define AUXTDC_STOP_AUXIO7                (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO7)
#define AUXTDC_STOP_AUXIO8                (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO8)
#define AUXTDC_STOP_AUXIO9                (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO9)
#define AUXTDC_STOP_AUXIO10               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO10)
#define AUXTDC_STOP_AUXIO11               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO11)
#define AUXTDC_STOP_AUXIO12               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO12)
#define AUXTDC_STOP_AUXIO13               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO13)
#define AUXTDC_STOP_AUXIO14               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO14)
#define AUXTDC_STOP_AUXIO15               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO15)
#define AUXTDC_STOP_AUXIO16               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO16)
#define AUXTDC_STOP_AUXIO17               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO17)
#define AUXTDC_STOP_AUXIO18               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO18)
#define AUXTDC_STOP_AUXIO19               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO19)
#define AUXTDC_STOP_AUXIO20               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO20)
#define AUXTDC_STOP_AUXIO21               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO21)
#define AUXTDC_STOP_AUXIO22               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO22)
#define AUXTDC_STOP_AUXIO23               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO23)
#define AUXTDC_STOP_AUXIO24               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO24)
#define AUXTDC_STOP_AUXIO25               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO25)
#define AUXTDC_STOP_AUXIO26               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO26)
#define AUXTDC_STOP_AUXIO27               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO27)
#define AUXTDC_STOP_AUXIO28               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO28)
#define AUXTDC_STOP_AUXIO29               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO29)
#define AUXTDC_STOP_AUXIO30               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO30)
#define AUXTDC_STOP_AUXIO31               (AUX_TDC_TRIGSRC_STOP_SRC_AUXIO31)
#define AUXTDC_STOP_MANUAL_EV             (AUX_TDC_TRIGSRC_STOP_SRC_MANUAL_EV)
#define AUXTDC_STOP_AON_RTC_CH2_DLY       (AUX_TDC_TRIGSRC_STOP_SRC_AON_RTC_CH2_DLY)
#define AUXTDC_STOP_AON_RTC_4KHZ          (AUX_TDC_TRIGSRC_STOP_SRC_AON_RTC_4KHZ)
#define AUXTDC_STOP_AON_BATMON_BAT_UPD    (AUX_TDC_TRIGSRC_STOP_SRC_AON_BATMON_BAT_UPD)
#define AUXTDC_STOP_AON_BATMON_TEMP_UPD   (AUX_TDC_TRIGSRC_STOP_SRC_AON_BATMON_TEMP_UPD)
#define AUXTDC_STOP_SCLK_LF               (AUX_TDC_TRIGSRC_STOP_SRC_SCLK_LF)
#define AUXTDC_STOP_PWR_DWN               (AUX_TDC_TRIGSRC_STOP_SRC_PWR_DWN)
#define AUXTDC_STOP_MCU_ACTIVE            (AUX_TDC_TRIGSRC_STOP_SRC_MCU_ACTIVE)
#define AUXTDC_STOP_VDDR_RECHARGE         (AUX_TDC_TRIGSRC_STOP_SRC_VDDR_RECHARGE)
#define AUXTDC_STOP_TIMER2_EV0            (AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER2_EV0)
#define AUXTDC_STOP_TIMER2_EV1            (AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER2_EV1)
#define AUXTDC_STOP_TIMER2_EV2            (AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER2_EV2)
#define AUXTDC_STOP_TIMER2_EV3            (AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER2_EV3)
#define AUXTDC_STOP_TIMER2_PULSE          (AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER2_PULSE)
#define AUXTDC_STOP_TDC_DONE              (AUX_TDC_TRIGSRC_STOP_SRC_AUX_TDC_DONE)
#define AUXTDC_STOP_ADC_IRQ               (AUX_TDC_TRIGSRC_STOP_SRC_AUX_ADC_IRQ)
#define AUXTDC_STOP_ADC_FIFO_NOT_EMPTY    (AUX_TDC_TRIGSRC_STOP_SRC_AUX_ADC_FIFO_NOT_EMPTY)
#define AUXTDC_STOP_NO_EVENT              (AUX_TDC_TRIGSRC_STOP_SRC_NO_EVENT)
#define AUXTDC_STOP_ADC_DONE              (AUX_TDC_TRIGSRC_STOP_SRC_AUX_ADC_DONE)
#define AUXTDC_STOP_ADC_FIFO_ALMOST_FULL  (AUX_TDC_TRIGSRC_STOP_SRC_AUX_ADC_FIFO_ALMOST_FULL)
#define AUXTDC_STOP_ISRC_RESET            (AUX_TDC_TRIGSRC_STOP_SRC_AUX_ISRC_RESET_N)
#define AUXTDC_STOP_OBSMUX0               (AUX_TDC_TRIGSRC_STOP_SRC_MCU_OBSMUX0)
#define AUXTDC_STOP_OBSMUX1               (AUX_TDC_TRIGSRC_STOP_SRC_MCU_OBSMUX1)
#define AUXTDC_STOP_SMPH_AUTOTAKE_DONE    (AUX_TDC_TRIGSRC_STOP_SRC_AUX_SMPH_AUTOTAKE_DONE)
#define AUXTDC_STOP_TDC_PRE               (AUX_TDC_TRIGSRC_STOP_SRC_AUX_TDC_PRE)
#define AUXTDC_STOP_TIMER0_EV             (AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER0_EV)
#define AUXTDC_STOP_TIMER1_EV             (AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER1_EV)
#define AUXTDC_STOP_AON_RTC_CH2           (AUX_TDC_TRIGSRC_STOP_SRC_AON_RTC_CH2)
#define AUXTDC_STOP_AUX_COMPA             (AUX_TDC_TRIGSRC_STOP_SRC_AUX_COMPA)
#define AUXTDC_STOP_AUX_COMPB             (AUX_TDC_TRIGSRC_STOP_SRC_AUX_COMPB)
#define AUXTDC_STOP_ACLK_REF              (AUX_TDC_TRIGSRC_STOP_SRC_ACLK_REF)
#define AUXTDC_STOP_MCU_EV                (AUX_TDC_TRIGSRC_STOP_SRC_MCU_EV)

#define AUXTDC_STARTPOL_RIS               (AUX_TDC_TRIGSRC_START_POL_HIGH) // Rising edge polarity for start event
#define AUXTDC_STARTPOL_FALL              (AUX_TDC_TRIGSRC_START_POL_LOW)  // Falling edge polarity for start event

#define AUXTDC_START_AUXIO0               (AUX_TDC_TRIGSRC_START_SRC_AUXIO0)
#define AUXTDC_START_AUXIO1               (AUX_TDC_TRIGSRC_START_SRC_AUXIO1)
#define AUXTDC_START_AUXIO2               (AUX_TDC_TRIGSRC_START_SRC_AUXIO2)
#define AUXTDC_START_AUXIO3               (AUX_TDC_TRIGSRC_START_SRC_AUXIO3)
#define AUXTDC_START_AUXIO4               (AUX_TDC_TRIGSRC_START_SRC_AUXIO4)
#define AUXTDC_START_AUXIO5               (AUX_TDC_TRIGSRC_START_SRC_AUXIO5)
#define AUXTDC_START_AUXIO6               (AUX_TDC_TRIGSRC_START_SRC_AUXIO6)
#define AUXTDC_START_AUXIO7               (AUX_TDC_TRIGSRC_START_SRC_AUXIO7)
#define AUXTDC_START_AUXIO8               (AUX_TDC_TRIGSRC_START_SRC_AUXIO8)
#define AUXTDC_START_AUXIO9               (AUX_TDC_TRIGSRC_START_SRC_AUXIO9)
#define AUXTDC_START_AUXIO10              (AUX_TDC_TRIGSRC_START_SRC_AUXIO10)
#define AUXTDC_START_AUXIO11              (AUX_TDC_TRIGSRC_START_SRC_AUXIO11)
#define AUXTDC_START_AUXIO12              (AUX_TDC_TRIGSRC_START_SRC_AUXIO12)
#define AUXTDC_START_AUXIO13              (AUX_TDC_TRIGSRC_START_SRC_AUXIO13)
#define AUXTDC_START_AUXIO14              (AUX_TDC_TRIGSRC_START_SRC_AUXIO14)
#define AUXTDC_START_AUXIO15              (AUX_TDC_TRIGSRC_START_SRC_AUXIO15)
#define AUXTDC_START_AUXIO16              (AUX_TDC_TRIGSRC_START_SRC_AUXIO16)
#define AUXTDC_START_AUXIO17              (AUX_TDC_TRIGSRC_START_SRC_AUXIO17)
#define AUXTDC_START_AUXIO18              (AUX_TDC_TRIGSRC_START_SRC_AUXIO18)
#define AUXTDC_START_AUXIO19              (AUX_TDC_TRIGSRC_START_SRC_AUXIO19)
#define AUXTDC_START_AUXIO20              (AUX_TDC_TRIGSRC_START_SRC_AUXIO20)
#define AUXTDC_START_AUXIO21              (AUX_TDC_TRIGSRC_START_SRC_AUXIO21)
#define AUXTDC_START_AUXIO22              (AUX_TDC_TRIGSRC_START_SRC_AUXIO22)
#define AUXTDC_START_AUXIO23              (AUX_TDC_TRIGSRC_START_SRC_AUXIO23)
#define AUXTDC_START_AUXIO24              (AUX_TDC_TRIGSRC_START_SRC_AUXIO24)
#define AUXTDC_START_AUXIO25              (AUX_TDC_TRIGSRC_START_SRC_AUXIO25)
#define AUXTDC_START_AUXIO26              (AUX_TDC_TRIGSRC_START_SRC_AUXIO26)
#define AUXTDC_START_AUXIO27              (AUX_TDC_TRIGSRC_START_SRC_AUXIO27)
#define AUXTDC_START_AUXIO28              (AUX_TDC_TRIGSRC_START_SRC_AUXIO28)
#define AUXTDC_START_AUXIO29              (AUX_TDC_TRIGSRC_START_SRC_AUXIO29)
#define AUXTDC_START_AUXIO30              (AUX_TDC_TRIGSRC_START_SRC_AUXIO30)
#define AUXTDC_START_AUXIO31              (AUX_TDC_TRIGSRC_START_SRC_AUXIO31)
#define AUXTDC_START_MANUAL_EV            (AUX_TDC_TRIGSRC_START_SRC_MANUAL_EV)
#define AUXTDC_START_AON_RTC_CH2_DLY      (AUX_TDC_TRIGSRC_START_SRC_AON_RTC_CH2_DLY)
#define AUXTDC_START_AON_RTC_4KHZ         (AUX_TDC_TRIGSRC_START_SRC_AON_RTC_4KHZ)
#define AUXTDC_START_AON_BATMON_BAT_UPD   (AUX_TDC_TRIGSRC_START_SRC_AON_BATMON_BAT_UPD)
#define AUXTDC_START_AON_BATMON_TEMP_UPD  (AUX_TDC_TRIGSRC_START_SRC_AON_BATMON_TEMP_UPD)
#define AUXTDC_START_SCLK_LF              (AUX_TDC_TRIGSRC_START_SRC_SCLK_LF)
#define AUXTDC_START_PWR_DWN              (AUX_TDC_TRIGSRC_START_SRC_PWR_DWN)
#define AUXTDC_START_MCU_ACTIVE           (AUX_TDC_TRIGSRC_START_SRC_MCU_ACTIVE)
#define AUXTDC_START_VDDR_RECHARGE        (AUX_TDC_TRIGSRC_START_SRC_VDDR_RECHARGE)
#define AUXTDC_START_TIMER2_EV0           (AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER2_EV0)
#define AUXTDC_START_TIMER2_EV1           (AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER2_EV1)
#define AUXTDC_START_TIMER2_EV2           (AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER2_EV2)
#define AUXTDC_START_TIMER2_EV3           (AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER2_EV3)
#define AUXTDC_START_TIMER2_PULSE         (AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER2_PULSE)
#define AUXTDC_START_TDC_DONE             (AUX_TDC_TRIGSRC_START_SRC_AUX_TDC_DONE)
#define AUXTDC_START_ADC_IRQ              (AUX_TDC_TRIGSRC_START_SRC_AUX_ADC_IRQ)
#define AUXTDC_START_ADC_FIFO_NOT_EMPTY   (AUX_TDC_TRIGSRC_START_SRC_AUX_ADC_FIFO_NOT_EMPTY)
#define AUXTDC_START_NO_EVENT             (AUX_TDC_TRIGSRC_START_SRC_NO_EVENT)
#define AUXTDC_START_ADC_DONE             (AUX_TDC_TRIGSRC_START_SRC_AUX_ADC_DONE)
#define AUXTDC_START_ADC_FIFO_ALMOST_FULL (AUX_TDC_TRIGSRC_START_SRC_AUX_ADC_FIFO_ALMOST_FULL)
#define AUXTDC_START_ISRC_RESET           (AUX_TDC_TRIGSRC_START_SRC_AUX_ISRC_RESET_N)
#define AUXTDC_START_OBSMUX0              (AUX_TDC_TRIGSRC_START_SRC_MCU_OBSMUX0)
#define AUXTDC_START_OBSMUX1              (AUX_TDC_TRIGSRC_START_SRC_MCU_OBSMUX1)
#define AUXTDC_START_SMPH_AUTOTAKE_DONE   (AUX_TDC_TRIGSRC_START_SRC_AUX_SMPH_AUTOTAKE_DONE)
#define AUXTDC_START_TDC_PRE              (AUX_TDC_TRIGSRC_START_SRC_AUX_TDC_PRE)
#define AUXTDC_START_TIMER0_EV            (AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER0_EV)
#define AUXTDC_START_TIMER1_EV            (AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER1_EV)
#define AUXTDC_START_AON_RTC_CH2          (AUX_TDC_TRIGSRC_START_SRC_AON_RTC_CH2)
#define AUXTDC_START_AUX_COMPA            (AUX_TDC_TRIGSRC_START_SRC_AUX_COMPA)
#define AUXTDC_START_AUX_COMPB            (AUX_TDC_TRIGSRC_START_SRC_AUX_COMPB)
#define AUXTDC_START_ACLK_REF             (AUX_TDC_TRIGSRC_START_SRC_ACLK_REF)
#define AUXTDC_START_MCU_EV               (AUX_TDC_TRIGSRC_START_SRC_MCU_EV)

//*****************************************************************************
//
// Defines for the possible saturation values set using AUXTDCLimitSet().
//
//*****************************************************************************
#define AUXTDC_SAT_4096         (AUX_TDC_SATCFG_LIMIT_R12)
#define AUXTDC_SAT_8192         (AUX_TDC_SATCFG_LIMIT_R13)
#define AUXTDC_SAT_16384        (AUX_TDC_SATCFG_LIMIT_R14)
#define AUXTDC_SAT_32768        (AUX_TDC_SATCFG_LIMIT_R15)
#define AUXTDC_SAT_65536        (AUX_TDC_SATCFG_LIMIT_R16)
#define AUXTDC_SAT_131072       (AUX_TDC_SATCFG_LIMIT_R17)
#define AUXTDC_SAT_262144       (AUX_TDC_SATCFG_LIMIT_R18)
#define AUXTDC_SAT_524288       (AUX_TDC_SATCFG_LIMIT_R19)
#define AUXTDC_SAT_1048576      (AUX_TDC_SATCFG_LIMIT_R20)
#define AUXTDC_SAT_2097152      (AUX_TDC_SATCFG_LIMIT_R21)
#define AUXTDC_SAT_4194304      (AUX_TDC_SATCFG_LIMIT_R22)
#define AUXTDC_SAT_8388608      (AUX_TDC_SATCFG_LIMIT_R23)
#define AUXTDC_SAT_16777216     (AUX_TDC_SATCFG_LIMIT_R24)
#define AUXTDC_NUM_SAT_VALS     16

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

#ifdef DRIVERLIB_DEBUG
//*****************************************************************************
//
//! \internal
//! \brief Checks an AUX TDC base address.
//!
//! This function determines if a AUX TDC port base address is valid.
//!
//! \param ui32Base is the base address of the AUX TDC port.
//!
//! \return Returns \c true if the base address is valid and \c false
//! otherwise.
//
//*****************************************************************************
static bool
AUXTDCBaseValid(uint32_t ui32Base)
{
    return(ui32Base == AUX_TDC_BASE);
}
#endif

//*****************************************************************************
//
//! \brief Get the status of the AUX TDC internal state machine.
//!
//! This function will return the current state of the AUX TDC internal state
//! machine.
//! \param ui32Base is base address of the AUX TDC
//!
//! \return Returns the current state of the state machine.
//! Possible states for the state machine are:
//! - \ref AUXTDC_WAIT_START
//! - \ref AUXTDC_WAIT_START_CNTEN
//! - \ref AUXTDC_IDLE
//! - \ref AUXTDC_CLRCNT
//! - \ref AUXTDC_WAIT_STOP
//! - \ref AUXTDC_WAIT_STOP_CNTDOWN
//! - \ref AUXTDC_GETRESULTS
//! - \ref AUXTDC_POR
//! - \ref AUXTDC_WAIT_CLRCNT_DONE
//! - \ref AUXTDC_START_FALL
//! - \ref AUXTDC_FORCE_STOP.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AUXTDCStatusGet(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(AUXTDCBaseValid(ui32Base));

    // Return the status value for the correct ADI Slave.
    return((HWREG(ui32Base + AUX_TDC_O_STAT) & AUX_TDC_STAT_STATE_M) >>
           AUX_TDC_STAT_STATE_S);
}

//*****************************************************************************
//
//! \brief Configure the operation of the AUX TDC.
//!
//! Use this function to configure the start and stop event for the AUX TDC.
//!
//! The \c ui32StartCondition must be a bitwise OR of the start event and the
//! polarity of the start event. The start events are:
//! - \ref AUXTDC_START_AUXIO0
//! - \ref AUXTDC_START_AUXIO1
//! - \ref AUXTDC_START_AUXIO2
//! - \ref AUXTDC_START_AUXIO3
//! - \ref AUXTDC_START_AUXIO4
//! - \ref AUXTDC_START_AUXIO5
//! - \ref AUXTDC_START_AUXIO6
//! - \ref AUXTDC_START_AUXIO7
//! - \ref AUXTDC_START_AUXIO8
//! - \ref AUXTDC_START_AUXIO9
//! - \ref AUXTDC_START_AUXIO10
//! - \ref AUXTDC_START_AUXIO11
//! - \ref AUXTDC_START_AUXIO12
//! - \ref AUXTDC_START_AUXIO13
//! - \ref AUXTDC_START_AUXIO14
//! - \ref AUXTDC_START_AUXIO15
//! - \ref AUXTDC_START_AUXIO16
//! - \ref AUXTDC_START_AUXIO17
//! - \ref AUXTDC_START_AUXIO18
//! - \ref AUXTDC_START_AUXIO19
//! - \ref AUXTDC_START_AUXIO20
//! - \ref AUXTDC_START_AUXIO21
//! - \ref AUXTDC_START_AUXIO22
//! - \ref AUXTDC_START_AUXIO23
//! - \ref AUXTDC_START_AUXIO24
//! - \ref AUXTDC_START_AUXIO25
//! - \ref AUXTDC_START_AUXIO26
//! - \ref AUXTDC_START_AUXIO27
//! - \ref AUXTDC_START_AUXIO28
//! - \ref AUXTDC_START_AUXIO29
//! - \ref AUXTDC_START_AUXIO30
//! - \ref AUXTDC_START_AUXIO31
//! - \ref AUXTDC_START_MANUAL_EV
//! - \ref AUXTDC_START_AON_RTC_CH2_DLY
//! - \ref AUXTDC_START_AON_RTC_4KHZ
//! - \ref AUXTDC_START_AON_BATMON_BAT_UPD
//! - \ref AUXTDC_START_AON_BATMON_TEMP_UPD
//! - \ref AUXTDC_START_SCLK_LF
//! - \ref AUXTDC_START_PWR_DWN
//! - \ref AUXTDC_START_MCU_ACTIVE
//! - \ref AUXTDC_START_VDDR_RECHARGE
//! - \ref AUXTDC_START_TIMER2_EV0
//! - \ref AUXTDC_START_TIMER2_EV1
//! - \ref AUXTDC_START_TIMER2_EV2
//! - \ref AUXTDC_START_TIMER2_EV3
//! - \ref AUXTDC_START_TIMER2_PULSE
//! - \ref AUXTDC_START_TDC_DONE
//! - \ref AUXTDC_START_ADC_IRQ
//! - \ref AUXTDC_START_ADC_FIFO_NOT_EMPTY
//! - \ref AUXTDC_START_NO_EVENT
//! - \ref AUXTDC_START_ADC_DONE
//! - \ref AUXTDC_START_ADC_FIFO_ALMOST_FULL
//! - \ref AUXTDC_START_ISRC_RESET
//! - \ref AUXTDC_START_OBSMUX0
//! - \ref AUXTDC_START_OBSMUX1
//! - \ref AUXTDC_START_SMPH_AUTOTAKE_DONE
//! - \ref AUXTDC_START_TDC_PRE
//! - \ref AUXTDC_START_TIMER0_EV
//! - \ref AUXTDC_START_TIMER1_EV
//! - \ref AUXTDC_START_AON_RTC_CH2
//! - \ref AUXTDC_START_AUX_COMPA
//! - \ref AUXTDC_START_AUX_COMPB
//! - \ref AUXTDC_START_ACLK_REF
//! - \ref AUXTDC_START_MCU_EV
//!
//! The polarity of the start event is either rising \ref AUXTDC_STARTPOL_RIS
//! or falling \ref AUXTDC_STARTPOL_FALL.
//!
//! The \c ui32StopCondition must be a bitwise OR of the stop event and the
//! polarity of the stop event. The stop events are:
//! - \ref AUXTDC_STOP_AUXIO0
//! - \ref AUXTDC_STOP_AUXIO1
//! - \ref AUXTDC_STOP_AUXIO2
//! - \ref AUXTDC_STOP_AUXIO3
//! - \ref AUXTDC_STOP_AUXIO4
//! - \ref AUXTDC_STOP_AUXIO5
//! - \ref AUXTDC_STOP_AUXIO6
//! - \ref AUXTDC_STOP_AUXIO7
//! - \ref AUXTDC_STOP_AUXIO8
//! - \ref AUXTDC_STOP_AUXIO9
//! - \ref AUXTDC_STOP_AUXIO10
//! - \ref AUXTDC_STOP_AUXIO11
//! - \ref AUXTDC_STOP_AUXIO12
//! - \ref AUXTDC_STOP_AUXIO13
//! - \ref AUXTDC_STOP_AUXIO14
//! - \ref AUXTDC_STOP_AUXIO15
//! - \ref AUXTDC_STOP_AUXIO16
//! - \ref AUXTDC_STOP_AUXIO17
//! - \ref AUXTDC_STOP_AUXIO18
//! - \ref AUXTDC_STOP_AUXIO19
//! - \ref AUXTDC_STOP_AUXIO20
//! - \ref AUXTDC_STOP_AUXIO21
//! - \ref AUXTDC_STOP_AUXIO22
//! - \ref AUXTDC_STOP_AUXIO23
//! - \ref AUXTDC_STOP_AUXIO24
//! - \ref AUXTDC_STOP_AUXIO25
//! - \ref AUXTDC_STOP_AUXIO26
//! - \ref AUXTDC_STOP_AUXIO27
//! - \ref AUXTDC_STOP_AUXIO28
//! - \ref AUXTDC_STOP_AUXIO29
//! - \ref AUXTDC_STOP_AUXIO30
//! - \ref AUXTDC_STOP_AUXIO31
//! - \ref AUXTDC_STOP_MANUAL_EV
//! - \ref AUXTDC_STOP_AON_RTC_CH2_DLY
//! - \ref AUXTDC_STOP_AON_RTC_4KHZ
//! - \ref AUXTDC_STOP_AON_BATMON_BAT_UPD
//! - \ref AUXTDC_STOP_AON_BATMON_TEMP_UPD
//! - \ref AUXTDC_STOP_SCLK_LF
//! - \ref AUXTDC_STOP_PWR_DWN
//! - \ref AUXTDC_STOP_MCU_ACTIVE
//! - \ref AUXTDC_STOP_VDDR_RECHARGE
//! - \ref AUXTDC_STOP_TIMER2_EV0
//! - \ref AUXTDC_STOP_TIMER2_EV1
//! - \ref AUXTDC_STOP_TIMER2_EV2
//! - \ref AUXTDC_STOP_TIMER2_EV3
//! - \ref AUXTDC_STOP_TIMER2_PULSE
//! - \ref AUXTDC_STOP_TDC_DONE
//! - \ref AUXTDC_STOP_ADC_IRQ
//! - \ref AUXTDC_STOP_ADC_FIFO_NOT_EMPTY
//! - \ref AUXTDC_STOP_NO_EVENT
//! - \ref AUXTDC_STOP_ADC_DONE
//! - \ref AUXTDC_STOP_ADC_FIFO_ALMOST_FULL
//! - \ref AUXTDC_STOP_ISRC_RESET
//! - \ref AUXTDC_STOP_OBSMUX0
//! - \ref AUXTDC_STOP_OBSMUX1
//! - \ref AUXTDC_STOP_SMPH_AUTOTAKE_DONE
//! - \ref AUXTDC_STOP_TDC_PRE
//! - \ref AUXTDC_STOP_TIMER0_EV
//! - \ref AUXTDC_STOP_TIMER1_EV
//! - \ref AUXTDC_STOP_AON_RTC_CH2
//! - \ref AUXTDC_STOP_AUX_COMPA
//! - \ref AUXTDC_STOP_AUX_COMPB
//! - \ref AUXTDC_STOP_ACLK_REF
//! - \ref AUXTDC_STOP_MCU_EV
//!
//! The polarity of the stop event is either rising \ref AUXTDC_STOPPOL_RIS
//! or falling \ref AUXTDC_STOPPOL_FALL.
//!
//! \note The AUX TDC should only be configured when the AUX TDC is in the Idle
//! state. To ensure that software does not lock up, it is recommended to
//! ensure that the AUX TDC is actually in idle when calling \ref AUXTDCConfigSet().
//! This can be tested using \ref AUXTDCIdle().
//!
//! \param ui32Base is base address of the AUX TDC.
//! \param ui32StartCondition is AUX TDC a bitwise OR of a start event and polarity.
//! \param ui32StopCondition is AUX TDC a bitwise OR of a stop event and polarity.
//!
//! \return None
//!
//! \sa \ref AUXTDCConfigSet(), \ref AUXTDCIdle()
//
//*****************************************************************************
extern void AUXTDCConfigSet(uint32_t ui32Base, uint32_t ui32StartCondition,
                            uint32_t ui32StopCondition);

//*****************************************************************************
//
//! \brief Check if the AUX TDC is in idle mode.
//!
//! This function can be used to check whether the AUX TDC internal state
//! machine is in idle mode. This is required before setting the polarity
//! of the start and stop event.
//!
//! \param ui32Base is the base address of the AUX TDC.
//!
//! \return Returns \c true if state machine is in idle and returns \c false
//! if the state machine is in any other state.
//
//*****************************************************************************
__STATIC_INLINE bool
AUXTDCIdle(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(AUXTDCBaseValid(ui32Base));

    // Check if the AUX TDC is in the Idle state.
    return (((HWREG(ui32Base + AUX_TDC_O_STAT) & AUX_TDC_STAT_STATE_M) ==
             AUX_TDC_STAT_STATE_IDLE) ? true : false);
}

//*****************************************************************************
//
//! \brief Enable the AUX TDC for a measurement.
//!
//! This function is used for arming the AUX TDC to begin a measurement as
//! soon as the start condition is met. There are two run modes:
//! - \ref AUX_TDC_RUNSYNC will wait for a falling event of the start pulse before
//!   starting measurement on next rising edge of start. This guarantees an edge
//!   triggered start and is recommended for frequency measurements. If the
//!   first falling edge is close to the start command it may be missed, but
//!   the TDC shall catch later falling edges and in any case guarantee a
//!   measurement start synchronous to the rising edge of the start event.
//! - The \ref AUX_TDC_RUN is asynchronous start and asynchronous stop mode. Using
//!   this a TDC measurement may start immediately if start is high and hence it
//!   may not give precise edge to edge measurements. This mode is only
//!   recommended when start pulse is guaranteed to arrive at least 7 clock
//!   periods after command.
//!
//! \note The AUX TDC should be configured and in Idle mode before calling this
//! function.
//!
//! \param ui32Base is the base address of the AUX TDC.
//! \param ui32RunMode is the run mode for the AUX TDC.
//! - \ref AUX_TDC_RUNSYNC : Synchronous run mode.
//! - \ref AUX_TDC_RUN : Asynchronous run mode.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
AUXTDCEnable(uint32_t ui32Base, uint32_t ui32RunMode)
{
    // Check the arguments.
    ASSERT(AUXTDCBaseValid(ui32Base));
    ASSERT((ui32RunMode == AUX_TDC_RUN) ||
           (ui32RunMode == AUX_TDC_RUNSYNC));

    // Enable the AUX TDC.
    HWREG(ui32Base + AUX_TDC_O_CTL) = ui32RunMode;
}

//*****************************************************************************
//
//! \brief Force the AUX TDC back to Idle mode.
//!
//! This function will force the AUX TDC in Idle mode. The internal state
//! machine will not go directly to Idle mode, so it is left to the programmer to
//! ensure that the state machine is in Idle mode before doing any new
//! configuration. This can be checked using \ref AUXTDCIdle().
//!
//! \param ui32Base is the base address of the AUX TDC.
//!
//! \return None
//!
//! \sa \ref AUXTDCIdle()
//
//*****************************************************************************
__STATIC_INLINE void
AUXTDCIdleForce(uint32_t ui32Base)
{
    // Check the arguments
    ASSERT(AUXTDCBaseValid(ui32Base));

    // Abort operation of AUX TDC and force into Idle mode.
    HWREG(ui32Base + AUX_TDC_O_CTL) = AUX_TDC_CTL_CMD_ABORT;
}

//*****************************************************************************
//
//! \brief Check if the AUX TDC is done measuring.
//!
//! This function can be used to check whether the AUX TDC has finished a
//! measurement. The AUX TDC may have completed a measurement for two reasons.
//! Either it finish successfully \ref AUX_TDC_DONE or it failed due to a timeout
//! \ref AUX_TDC_TIMEOUT. If the AUX TDC is still measuring it this function
//! will return \ref AUX_TDC_BUSY.
//!
//! \param ui32Base is the base address of the AUX TDC.
//!
//! \return Returns the current status of a measurement:
//! - \ref AUX_TDC_DONE : An AUX TDC measurement finished successfully.
//! - \ref AUX_TDC_TIMEOUT : An AUX TDC measurement failed due to timeout.
//! - \ref AUX_TDC_BUSY : An AUX TDC measurement is being performed.
//
//*****************************************************************************
extern uint32_t AUXTDCMeasurementDone(uint32_t ui32Base);

//*****************************************************************************
//
//! \brief Get the value of the latest measurement.
//!
//! This function is used for retrieving the value of the latest measurement
//! performed by the AUX TDC.
//!
//! \param ui32Base is the base address of the AUX TDC.
//!
//! \return Returns the result of the latest measurement.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AUXTDCMeasurementGet(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(AUXTDCBaseValid(ui32Base));

    // Return the measurement.
    return (HWREG(ui32Base + AUX_TDC_O_RESULT));
}

//*****************************************************************************
//
//! \brief Set the saturation limit of the measurement.
//!
//! This function is used to set a saturation limit for the event accumulation
//! register. The saturation limit is defined as a bit width of the
//! accumulation register and therefore increases in power of 2.
//!
//! \param ui32Base is base address of the AUX TDC.
//! \param ui32Limit is the saturation limit.
//! - \ref AUXTDC_SAT_4096
//! - \ref AUXTDC_SAT_8192
//! - \ref AUXTDC_SAT_16384
//! - \ref AUXTDC_SAT_32768
//! - \ref AUXTDC_SAT_65536
//! - \ref AUXTDC_SAT_131072
//! - \ref AUXTDC_SAT_262144
//! - \ref AUXTDC_SAT_524288
//! - \ref AUXTDC_SAT_1048576
//! - \ref AUXTDC_SAT_2097152
//! - \ref AUXTDC_SAT_4194304
//! - \ref AUXTDC_SAT_8388608
//! - \ref AUXTDC_SAT_16777216 (default)
//!
//! \return None
//!
//! \note The actual value of the accumulation register might increase slightly beyond
//! the saturation value before the saturation takes effect.
//!
//! \sa \ref AUXTDCLimitGet()
//
//*****************************************************************************
__STATIC_INLINE void
AUXTDCLimitSet(uint32_t ui32Base, uint32_t ui32Limit)
{
    // Check the arguments.
    ASSERT(AUXTDCBaseValid(ui32Base));
    ASSERT(ui32Limit < AUXTDC_NUM_SAT_VALS);

    // Set the saturation limit.
    HWREG(ui32Base + AUX_TDC_O_SATCFG) = ui32Limit;
}

//*****************************************************************************
//
//! \brief Get the saturation limit of the measurement.
//!
//! This function is used to retrieve the current saturation for the
//! accumulator register.
//!
//! \param ui32Base is base address of the AUX TDC.
//!
//! \return Returns the saturation limit.
//! - \ref AUXTDC_SAT_4096
//! - \ref AUXTDC_SAT_8192
//! - \ref AUXTDC_SAT_16384
//! - \ref AUXTDC_SAT_32768
//! - \ref AUXTDC_SAT_65536
//! - \ref AUXTDC_SAT_131072
//! - \ref AUXTDC_SAT_262144
//! - \ref AUXTDC_SAT_524288
//! - \ref AUXTDC_SAT_1048576
//! - \ref AUXTDC_SAT_2097152
//! - \ref AUXTDC_SAT_4194304
//! - \ref AUXTDC_SAT_8388608
//! - \ref AUXTDC_SAT_16777216
//!
//! \sa \ref AUXTDCLimitSet()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AUXTDCLimitGet(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(AUXTDCBaseValid(ui32Base));

    // Return the saturation limit.
    return (HWREG(ui32Base + AUX_TDC_O_SATCFG));
}

//*****************************************************************************
//
//! \brief Enables the counter if possible.
//!
//! This function can be used to enable the AUX TDC stop/compare event counter.
//! The counter can be used to measure multiple periods of a clock signal.
//! For each stop/compare event the counter will be decremented by one and
//! the measurement will continue running until the value of the counter
//! reaches 0. The current value of the counter can be read using
//! \ref AUXTDCCounterGet(). The reset value of the counter can be set using
//! \ref AUXTDCCounterSet().
//!
//! \param ui32Base is base address of the AUX TDC.
//!
//! \return Returns \c true if the counter was successfully enabled. If the
//! AUX TDC is not in Idle mode, the counter can not be enabled, and the
//! return value will be \c false.
//!
//! \sa \ref AUXTDCCounterGet(), \ref AUXTDCCounterSet()
//
//*****************************************************************************
__STATIC_INLINE bool
AUXTDCCounterEnable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(AUXTDCBaseValid(ui32Base));

    // Check if the AUX TDC is in idle mode. If not in Idle mode, the counter
    // will not be enabled.
    if(!((HWREG(ui32Base + AUX_TDC_O_STAT) & AUX_TDC_STAT_STATE_M) ==
            AUX_TDC_STAT_STATE_IDLE))
    {
        return false;
    }

    // Enable the counter.
    HWREG(ui32Base + AUX_TDC_O_TRIGCNTCFG) = AUX_TDC_TRIGCNTCFG_EN;

    // Counter successfully enabled.
    return true;
}

//*****************************************************************************
//
//! \brief Disables the counter if possible.
//!
//! This function can be used to disable the AUX TDC stop/compare event counter.
//!
//! \param ui32Base is base address of the AUX TDC.
//!
//! \return Returns \c true if the counter was successfully disabled. If the
//! AUX TDC is not in Idle mode, the counter can not be disabled, and the
//! return value will be \c false.
//!
//! \sa \ref AUXTDCCounterEnable() for more information on how to use the counter.
//
//*****************************************************************************
__STATIC_INLINE bool
AUXTDCCounterDisable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(AUXTDCBaseValid(ui32Base));

    // Check if the AUX TDC is in Idle mode. If not in Idle mode, the counter
    // will not be disabled.
    if(!((HWREG(ui32Base + AUX_TDC_O_STAT) & AUX_TDC_STAT_STATE_M) ==
            AUX_TDC_STAT_STATE_IDLE))
    {
        return false;
    }

    // Disable the counter.
    HWREG(ui32Base + AUX_TDC_O_TRIGCNTCFG) = 0;

    // Counter successfully disabled.
    return true;
}

//*****************************************************************************
//
//! \brief Set the reset number of counter compare/stop event to ignore before taking
//! a measurement.
//!
//! This function loads the reset value of the counter with the specified
//! number of events to ignore. A reset in this context means the counter
//! has been disabled and then enabled.
//!
//! \param ui32Base is base address of the AUX TDC.
//! \param ui32Events is the number of compare/stop events to load into the
//! counter.
//!
//! \return Returns \c true if the counter was successfully updated. If the
//! AUX TDC is not in Idle mode, the counter can not be updated, and the
//! return value will be \c false.
//!
//! \sa \ref AUXTDCCounterEnable()
//
//*****************************************************************************
__STATIC_INLINE bool
AUXTDCCounterSet(uint32_t ui32Base, uint32_t ui32Events)
{
    // Check the arguments.
    ASSERT(AUXTDCBaseValid(ui32Base));

    // Check if the AUX TDC is in idle mode. If not in idle mode, the counter
    // will not be disabled.
    if(!((HWREG(ui32Base + AUX_TDC_O_STAT) & AUX_TDC_STAT_STATE_M) ==
            AUX_TDC_STAT_STATE_IDLE))
    {
        return false;
    }

    // Update the reset counter value.
    HWREG(ui32Base + AUX_TDC_O_TRIGCNTLOAD) = ui32Events;

    // Counter successfully updated.
    return true;
}

//*****************************************************************************
//
//! \brief Get the current number of counter compare/stop event to ignore before
//! taking a measurement.
//!
//! This function returns the current value of compare/stop events before
//! a measurement is registered. This value is decremented by one for each
//! registered compare/stop event and will always be less than or equal the
//! reset value of the counter set using \ref AUXTDCCounterSet().
//!
//! \param ui32Base is base address of the AUX TDC.
//!
//! \return Returns the current value of compare/stop events ignored before a
//! measurement is performed.
//!
//! \sa \ref AUXTDCCounterEnable().
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AUXTDCCounterGet(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(AUXTDCBaseValid(ui32Base));

    // Return the current counter value.
    return (HWREG(ui32Base + AUX_TDC_O_TRIGCNT));
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_AUXTDCConfigSet
        #undef  AUXTDCConfigSet
        #define AUXTDCConfigSet                 ROM_AUXTDCConfigSet
    #endif
    #ifdef ROM_AUXTDCMeasurementDone
        #undef  AUXTDCMeasurementDone
        #define AUXTDCMeasurementDone           ROM_AUXTDCMeasurementDone
    #endif
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __AUX_TDC_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
