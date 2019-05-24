/******************************************************************************
*  Filename:       rfc.h
*  Revised:        2018-08-08 14:03:25 +0200 (Wed, 08 Aug 2018)
*  Revision:       52338
*
*  Description:    Defines and prototypes for the RF Core.
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
//! \addtogroup rfc_api
//! @{
//
//*****************************************************************************

#ifndef __RFC_H__
#define __RFC_H__

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
#include "../inc/hw_rfc_pwr.h"
#include "../inc/hw_rfc_dbell.h"
#include "../inc/hw_fcfg1.h"
#include "../inc/hw_adi_3_refsys.h"
#include "../inc/hw_adi.h"
#include "rf_common_cmd.h"
#include "rf_prop_cmd.h"
#include "rf_ble_cmd.h"

// Definition of RFTRIM container
typedef struct {
   uint32_t configIfAdc;
   uint32_t configRfFrontend;
   uint32_t configSynth;
   uint32_t configMiscAdc;
} rfTrim_t;

// Definition of maximum search depth used by the RFCOverrideUpdate function
#define RFC_MAX_SEARCH_DEPTH     5
#define RFC_PA_TYPE_ADDRESS		 0x21000345
#define RFC_PA_TYPE_MASK 		 0x04
#define RFC_PA_GAIN_ADDRESS		 0x2100034C
#define RFC_PA_GAIN_MASK		 0x003FFFFF
#define RFC_FE_MODE_ESCAPE_VALUE 0xFF
#define RFC_FE_OVERRIDE_ADDRESS  0x0703
#define RFC_FE_OVERRIDE_MASK     0x0000FFFF

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
    #define RFCCpeIntGetAndClear            NOROM_RFCCpeIntGetAndClear
    #define RFCDoorbellSendTo               NOROM_RFCDoorbellSendTo
    #define RFCSynthPowerDown               NOROM_RFCSynthPowerDown
    #define RFCCpePatchReset                NOROM_RFCCpePatchReset
    #define RFCOverrideSearch               NOROM_RFCOverrideSearch
    #define RFCOverrideUpdate               NOROM_RFCOverrideUpdate
    #define RFCHwIntGetAndClear             NOROM_RFCHwIntGetAndClear
    #define RFCAnaDivTxOverride             NOROM_RFCAnaDivTxOverride
#endif

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Enable the RF core clocks.
//!
//! As soon as the RF core is started it will handle clock control
//! autonomously. No check should be performed to check the clocks. Instead
//! the radio can be ping'ed through the command interface.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
RFCClockEnable(void)
{
    // Enable basic clocks to get the CPE run
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) = RFC_PWR_PWMCLKEN_CPERAM
                                                    | RFC_PWR_PWMCLKEN_CPE
                                                    | RFC_PWR_PWMCLKEN_RFC;
}


//*****************************************************************************
//
//! \brief Disable the RF core clocks.
//!
//! As soon as the RF core is started it will handle clock control
//! autonomously. No check should be performed to check the clocks. Instead
//! the radio can be ping'ed through the command interface.
//!
//! When disabling clocks it is the programmers responsibility that the
//! RF core clocks are safely gated. I.e. the RF core should be safely
//! 'parked'.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
RFCClockDisable(void)
{
    // Disable all clocks
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) = 0x0;
}


//*****************************************************************************
//
//! Clear HW interrupt flags
//
//*****************************************************************************
__STATIC_INLINE void
RFCCpeIntClear(uint32_t ui32Mask)
{
    // Clear the masked pending interrupts.
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) = ~ui32Mask;
}


//*****************************************************************************
//
//! Clear CPE interrupt flags.
//
//*****************************************************************************
__STATIC_INLINE void
RFCHwIntClear(uint32_t ui32Mask)
{
    // Clear the masked pending interrupts.
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIFG) = ~ui32Mask;
}


//*****************************************************************************
//
//! Select interrupt sources to CPE0 (assign to INT_RFC_CPE_0 interrupt vector).
//
//*****************************************************************************
__STATIC_INLINE void
RFCCpe0IntSelect(uint32_t ui32Mask)
{
    // Multiplex RF Core interrupts to CPE0 IRQ.
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEISL) &= ~ui32Mask;
}


//*****************************************************************************
//
//! Select interrupt sources to CPE1 (assign to INT_RFC_CPE_1 interrupt vector).
//
//*****************************************************************************
__STATIC_INLINE void
RFCCpe1IntSelect(uint32_t ui32Mask)
{
    // Multiplex RF Core interrupts to CPE1 IRQ.
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEISL) |= ui32Mask;
}


//*****************************************************************************
//
//! Enable CPEx interrupt sources.
//
//*****************************************************************************
__STATIC_INLINE void
RFCCpeIntEnable(uint32_t ui32Mask)
{
    // Enable CPE interrupts from RF Core.
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIEN) |= ui32Mask;
}


//*****************************************************************************
//
//! Select, clear, and enable interrupt sources to CPE0.
//
//*****************************************************************************
__STATIC_INLINE void
RFCCpe0IntSelectClearEnable(uint32_t ui32Mask)
{
    // Multiplex RF Core interrupts to CPE0 IRQ.
    RFCCpe0IntSelect(ui32Mask);

    // Clear the masked interrupts.
    RFCCpeIntClear(ui32Mask);

    // Enable the masked interrupts.
    RFCCpeIntEnable(ui32Mask);
}


//*****************************************************************************
//
//! Select, clear, and enable interrupt sources to CPE1.
//
//*****************************************************************************
__STATIC_INLINE void
RFCCpe1IntSelectClearEnable(uint32_t ui32Mask)
{
    // Multiplex RF Core interrupts to CPE1 IRQ.
    RFCCpe1IntSelect(ui32Mask);

    // Clear the masked interrupts.
    RFCCpeIntClear(ui32Mask);

    // Enable the masked interrupts.
    RFCCpeIntEnable(ui32Mask);
}


//*****************************************************************************
//
//! Enable HW interrupt sources.
//
//*****************************************************************************
__STATIC_INLINE void
RFCHwIntEnable(uint32_t ui32Mask)
{
    // Enable the masked interrupts
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIEN) |= ui32Mask;
}


//*****************************************************************************
//
//! Disable CPE interrupt sources.
//
//*****************************************************************************
__STATIC_INLINE void
RFCCpeIntDisable(uint32_t ui32Mask)
{
    // Disable the masked interrupts
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIEN) &= ~ui32Mask;
}


//*****************************************************************************
//
//! Disable HW interrupt sources.
//
//*****************************************************************************
__STATIC_INLINE void
RFCHwIntDisable(uint32_t ui32Mask)
{
    // Disable the masked interrupts
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIEN) &= ~ui32Mask;
}


//*****************************************************************************
//
//! Get and clear CPE interrupt flags.
//
//*****************************************************************************
extern uint32_t RFCCpeIntGetAndClear(uint32_t ui32Mask);


//*****************************************************************************
//
//! Clear ACK interrupt flag.
//
//*****************************************************************************
__STATIC_INLINE void
RFCAckIntClear(void)
{
    // Clear any pending interrupts.
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0x0;
}


//*****************************************************************************
//
//! Send a radio operation to the doorbell and wait for an acknowledgment.
//
//*****************************************************************************
extern uint32_t RFCDoorbellSendTo(uint32_t pOp);


//*****************************************************************************
//
//! This function implements a fast way to turn off the synthesizer.
//
//*****************************************************************************
extern void RFCSynthPowerDown(void);


//*****************************************************************************
//
//! Reset previously patched CPE RAM to a state where it can be patched again.
//
//*****************************************************************************
extern void RFCCpePatchReset(void);


//*****************************************************************************
//
// Function to search an override list for the provided pattern within the search depth.
//
//*****************************************************************************
extern uint8_t RFCOverrideSearch(const uint32_t *pOverride, const uint32_t pattern, const uint32_t mask, const uint8_t searchDepth);


//*****************************************************************************
//
//! Function to update override list
//
//*****************************************************************************
extern uint8_t RFCOverrideUpdate(rfc_radioOp_t *pOpSetup, uint32_t *pParams);


//*****************************************************************************
//
//! Get and clear HW interrupt flags.
//
//*****************************************************************************
extern uint32_t RFCHwIntGetAndClear(uint32_t ui32Mask);


//*****************************************************************************
//
//! Get the type of currently selected PA.
//
//*****************************************************************************
__STATIC_INLINE bool
RFCGetPaType(void)
{
    return (bool)(HWREGB(RFC_PA_TYPE_ADDRESS) & RFC_PA_TYPE_MASK);
}

//*****************************************************************************
//
//! Get the gain of currently selected PA.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
RFCGetPaGain(void)
{
    return (HWREG(RFC_PA_GAIN_ADDRESS) & RFC_PA_GAIN_MASK);
}


//*****************************************************************************
//
//! Function to calculate the proper override run-time for the High Gain PA.
//
//*****************************************************************************
extern uint32_t RFCAnaDivTxOverride(uint8_t loDivider, uint8_t frontEndMode);


//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_RFCCpeIntGetAndClear
        #undef  RFCCpeIntGetAndClear
        #define RFCCpeIntGetAndClear            ROM_RFCCpeIntGetAndClear
    #endif
    #ifdef ROM_RFCDoorbellSendTo
        #undef  RFCDoorbellSendTo
        #define RFCDoorbellSendTo               ROM_RFCDoorbellSendTo
    #endif
    #ifdef ROM_RFCSynthPowerDown
        #undef  RFCSynthPowerDown
        #define RFCSynthPowerDown               ROM_RFCSynthPowerDown
    #endif
    #ifdef ROM_RFCCpePatchReset
        #undef  RFCCpePatchReset
        #define RFCCpePatchReset                ROM_RFCCpePatchReset
    #endif
    #ifdef ROM_RFCOverrideSearch
        #undef  RFCOverrideSearch
        #define RFCOverrideSearch               ROM_RFCOverrideSearch
    #endif
    #ifdef ROM_RFCOverrideUpdate
        #undef  RFCOverrideUpdate
        #define RFCOverrideUpdate               ROM_RFCOverrideUpdate
    #endif
    #ifdef ROM_RFCHwIntGetAndClear
        #undef  RFCHwIntGetAndClear
        #define RFCHwIntGetAndClear             ROM_RFCHwIntGetAndClear
    #endif
    #ifdef ROM_RFCAnaDivTxOverride
        #undef  RFCAnaDivTxOverride
        #define RFCAnaDivTxOverride             ROM_RFCAnaDivTxOverride
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

#endif //  __RFC_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//
//*****************************************************************************
