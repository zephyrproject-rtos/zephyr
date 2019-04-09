/******************************************************************************
*  Filename:       aux_sysif.c
*  Revised:        2018-04-17 14:54:06 +0200 (Tue, 17 Apr 2018)
*  Revision:       51890
*
*  Description:    Driver for the AUX System Interface
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

#include "aux_sysif.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  AUXSYSIFOpModeChange
    #define AUXSYSIFOpModeChange            NOROM_AUXSYSIFOpModeChange
#endif


//*****************************************************************************
//
// Used in AUXSYSIFOpModeChange() to control the change of the operational mode.
//
//*****************************************************************************
static const uint8_t g_OpMode_to_order[4] = {1,2,0,3};
static const uint8_t g_Order_to_OpMode[4] = {2,0,1,3};

//*****************************************************************************
//
// Controls AUX operational mode change
//
//*****************************************************************************
void
AUXSYSIFOpModeChange(uint32_t targetOpMode)
{
    uint32_t currentOpMode;
    uint32_t currentOrder;
    uint32_t nextMode;

    // Check the argument
    ASSERT((targetOpMode == AUX_SYSIF_OPMODEREQ_REQ_PDLP)||
           (targetOpMode == AUX_SYSIF_OPMODEREQ_REQ_PDA) ||
           (targetOpMode == AUX_SYSIF_OPMODEREQ_REQ_LP)  ||
           (targetOpMode == AUX_SYSIF_OPMODEREQ_REQ_A));

    do {
       currentOpMode = HWREG(AUX_SYSIF_BASE + AUX_SYSIF_O_OPMODEREQ);
       while ( currentOpMode != HWREG(AUX_SYSIF_BASE + AUX_SYSIF_O_OPMODEACK));
       if (currentOpMode != targetOpMode)
       {
           currentOrder = g_OpMode_to_order[currentOpMode];
           if ( currentOrder < g_OpMode_to_order[targetOpMode])
           {
               nextMode = g_Order_to_OpMode[currentOrder + 1];
           }
           else
           {
               nextMode = g_Order_to_OpMode[currentOrder - 1];
           }
           HWREG(AUX_SYSIF_BASE + AUX_SYSIF_O_OPMODEREQ) = nextMode;
       }
    } while ( currentOpMode != targetOpMode );
}
