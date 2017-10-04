/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#ifndef __RESET_H__
#define __RESET_H__

//*****************************************************************************
//
//! \addtogroup reset_api
//! @{
//
//*****************************************************************************

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

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <stdint.h>

//*****************************************************************************
//
// Control specific variables
//
//*****************************************************************************
#define RESET_KEY   0x6900
#define RESET_HARD_RESET RSTCTL_RESET_REQ_HARD_REQ
#define RESET_SOFT_RESET RSTCTL_RESET_REQ_SOFT_REQ

#define RESET_SRC_0 RSTCTL_HARDRESET_CLR_SRC0
#define RESET_SRC_1 RSTCTL_HARDRESET_CLR_SRC1
#define RESET_SRC_2 RSTCTL_HARDRESET_CLR_SRC2
#define RESET_SRC_3 RSTCTL_HARDRESET_CLR_SRC3
#define RESET_SRC_4 RSTCTL_HARDRESET_CLR_SRC4
#define RESET_SRC_5 RSTCTL_HARDRESET_CLR_SRC5
#define RESET_SRC_6 RSTCTL_HARDRESET_CLR_SRC6
#define RESET_SRC_7 RSTCTL_HARDRESET_CLR_SRC7
#define RESET_SRC_8 RSTCTL_HARDRESET_CLR_SRC8
#define RESET_SRC_9 RSTCTL_HARDRESET_CLR_SRC9
#define RESET_SRC_10 RSTCTL_HARDRESET_CLR_SRC10
#define RESET_SRC_11 RSTCTL_HARDRESET_CLR_SRC11
#define RESET_SRC_12 RSTCTL_HARDRESET_CLR_SRC12
#define RESET_SRC_13 RSTCTL_HARDRESET_CLR_SRC13
#define RESET_SRC_14 RSTCTL_HARDRESET_CLR_SRC14
#define RESET_SRC_15 RSTCTL_HARDRESET_CLR_SRC15

#define RESET_VCCDET    RSTCTL_PSSRESET_STAT_VCCDET
#define RESET_SVSH_TRIP RSTCTL_PSSRESET_STAT_SVSMH
#define RESET_BGREF_BAD RSTCTL_PSSRESET_STAT_BGREF

#define RESET_LPM35   RSTCTL_PCMRESET_STAT_LPM35
#define RESET_LPM45   RSTCTL_PCMRESET_STAT_LPM45

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//! Initiates a soft system reset.
//!
//! \return none
//
//*****************************************************************************
extern void ResetCtl_initiateSoftReset(void);

//*****************************************************************************
//
//! Initiates a soft system reset with a particular source given. This source
//! is generic and can be assigned by the user.
//!
//! \param source Source of the reset. Valid values are:
//!             - \b RESET_SRC_0,
//!             - \b RESET_SRC_1,
//!             - \b RESET_SRC_2,
//!             - \b RESET_SRC_3,
//!             - \b RESET_SRC_4,
//!             - \b RESET_SRC_5,
//!             - \b RESET_SRC_6,
//!             - \b RESET_SRC_7,
//!             - \b RESET_SRC_8,
//!             - \b RESET_SRC_9,
//!             - \b RESET_SRC_10,
//!             - \b RESET_SRC_11,
//!             - \b RESET_SRC_12,
//!             - \b RESET_SRC_13,
//!             - \b RESET_SRC_14,
//!             - \b RESET_SRC_15
//!
//! \return none
//
//*****************************************************************************
extern void ResetCtl_initiateSoftResetWithSource(uint32_t source);

//*****************************************************************************
//
//! Retrieves previous soft reset sources
//!
//! \return the bitwise or of previous reset sources. These sources must be
//! cleared using the \link ResetCtl_clearSoftResetSource \endlink function to
//! be cleared.
//! Possible values include:
//!             - \b RESET_SRC_0,
//!             - \b RESET_SRC_1,
//!             - \b RESET_SRC_2,
//!             - \b RESET_SRC_3,
//!             - \b RESET_SRC_4,
//!             - \b RESET_SRC_5,
//!             - \b RESET_SRC_6,
//!             - \b RESET_SRC_7,
//!             - \b RESET_SRC_8,
//!             - \b RESET_SRC_9,
//!             - \b RESET_SRC_10,
//!             - \b RESET_SRC_11,
//!             - \b RESET_SRC_12,
//!             - \b RESET_SRC_13,
//!             - \b RESET_SRC_14,
//!             - \b RESET_SRC_15
//
//*****************************************************************************
extern uint32_t ResetCtl_getSoftResetSource(void);

//*****************************************************************************
//
//! Clears the reset sources associated with at soft reset
//!
//! \param mask - Bitwise OR of any of the following values:
//!             - \b RESET_SRC_0,
//!             - \b RESET_SRC_1,
//!             - \b RESET_SRC_2,
//!             - \b RESET_SRC_3,
//!             - \b RESET_SRC_4,
//!             - \b RESET_SRC_5,
//!             - \b RESET_SRC_6,
//!             - \b RESET_SRC_7,
//!             - \b RESET_SRC_8,
//!             - \b RESET_SRC_9,
//!             - \b RESET_SRC_10,
//!             - \b RESET_SRC_11,
//!             - \b RESET_SRC_12,
//!             - \b RESET_SRC_13,
//!             - \b RESET_SRC_14,
//!             - \b RESET_SRC_15
//!
//! \return none
//
//*****************************************************************************
extern void ResetCtl_clearSoftResetSource(uint32_t mask);

//*****************************************************************************
//
//! Initiates a hard system reset.
//!
//! \return none
//
//*****************************************************************************
extern void ResetCtl_initiateHardReset(void);

//*****************************************************************************
//
//! Initiates a hard system reset with a particular source given. This source
//! is generic and can be assigned by the user.
//!
//! \param source - Valid values are one the following values:
//!             - \b RESET_SRC_0,
//!             - \b RESET_SRC_1,
//!             - \b RESET_SRC_2,
//!             - \b RESET_SRC_3,
//!             - \b RESET_SRC_4,
//!             - \b RESET_SRC_5,
//!             - \b RESET_SRC_6,
//!             - \b RESET_SRC_7,
//!             - \b RESET_SRC_8,
//!             - \b RESET_SRC_9,
//!             - \b RESET_SRC_10,
//!             - \b RESET_SRC_11,
//!             - \b RESET_SRC_12,
//!             - \b RESET_SRC_13,
//!             - \b RESET_SRC_14,
//!             - \b RESET_SRC_15
//! \return none
//
//*****************************************************************************
extern void ResetCtl_initiateHardResetWithSource(uint32_t source);

//*****************************************************************************
//
//! Retrieves previous hard reset sources
//!
//! \return the bitwise or of previous reset sources. These sources must be
//! cleared using the \link ResetCtl_clearHardResetSource \endlink function to
//! be cleared.
//! Possible values include:
//!             - \b RESET_SRC_0,
//!             - \b RESET_SRC_1,
//!             - \b RESET_SRC_2,
//!             - \b RESET_SRC_3,
//!             - \b RESET_SRC_4,
//!             - \b RESET_SRC_5,
//!             - \b RESET_SRC_6,
//!             - \b RESET_SRC_7,
//!             - \b RESET_SRC_8,
//!             - \b RESET_SRC_9,
//!             - \b RESET_SRC_10,
//!             - \b RESET_SRC_11,
//!             - \b RESET_SRC_12,
//!             - \b RESET_SRC_13,
//!             - \b RESET_SRC_14,
//!             - \b RESET_SRC_15
//
//*****************************************************************************
extern uint32_t ResetCtl_getHardResetSource(void);

//*****************************************************************************
//
//! Clears the reset sources associated with at hard reset
//!
//! \param mask - Bitwise OR of any of the following values:
//!             - \b RESET_SRC_0,
//!             - \b RESET_SRC_1,
//!             - \b RESET_SRC_2,
//!             - \b RESET_SRC_3,
//!             - \b RESET_SRC_4,
//!             - \b RESET_SRC_5,
//!             - \b RESET_SRC_6,
//!             - \b RESET_SRC_7,
//!             - \b RESET_SRC_8,
//!             - \b RESET_SRC_9,
//!             - \b RESET_SRC_10,
//!             - \b RESET_SRC_11,
//!             - \b RESET_SRC_12,
//!             - \b RESET_SRC_13,
//!             - \b RESET_SRC_14,
//!             - \b RESET_SRC_15
//!
//! \return none
//
//*****************************************************************************
extern void ResetCtl_clearHardResetSource(uint32_t mask);

//*****************************************************************************
//
//! Indicates the last cause of a power-on reset (POR) due to PSS operation.
//! Note that the bits returned from this function may be set in different
//! combinations. When a cold power up occurs, the value of all the values ORed
//! together could be returned as a cold power up causes these conditions.
//!
//! \return  Bitwise OR of any of the following values:
//!                 - RESET_VCCDET,
//!                 - RESET_SVSH_TRIP,
//!                 - RESET_BGREF_BAD
//
//*****************************************************************************
extern uint32_t ResetCtl_getPSSSource(void);

//*****************************************************************************
//
//! Clears the  PSS reset source flags
//!
//! \return none
//
//*****************************************************************************
extern void ResetCtl_clearPSSFlags(void);

//*****************************************************************************
//
//! Indicates the last cause of a power-on reset (POR) due to PCM operation.
//!
//! \return  Bitwise OR of any of the following values:
//!                 - RESET_LPM35,
//!                 - RESET_LPM45
//
//*****************************************************************************
extern uint32_t ResetCtl_getPCMSource(void);

//*****************************************************************************
//
//! Clears the corresponding PCM reset source flags
//!
//! \return none
//
//*****************************************************************************
extern void ResetCtl_clearPCMFlags(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif // __RESET_H__
