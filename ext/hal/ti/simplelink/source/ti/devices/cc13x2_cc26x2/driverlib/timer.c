/******************************************************************************
*  Filename:       timer.c
*  Revised:        2017-04-26 18:27:45 +0200 (Wed, 26 Apr 2017)
*  Revision:       48852
*
*  Description:    Driver for the General Purpose Timer
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

#include "timer.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  TimerConfigure
    #define TimerConfigure                  NOROM_TimerConfigure
    #undef  TimerLevelControl
    #define TimerLevelControl               NOROM_TimerLevelControl
    #undef  TimerStallControl
    #define TimerStallControl               NOROM_TimerStallControl
    #undef  TimerWaitOnTriggerControl
    #define TimerWaitOnTriggerControl       NOROM_TimerWaitOnTriggerControl
    #undef  TimerIntRegister
    #define TimerIntRegister                NOROM_TimerIntRegister
    #undef  TimerIntUnregister
    #define TimerIntUnregister              NOROM_TimerIntUnregister
    #undef  TimerMatchUpdateMode
    #define TimerMatchUpdateMode            NOROM_TimerMatchUpdateMode
    #undef  TimerIntervalLoadMode
    #define TimerIntervalLoadMode           NOROM_TimerIntervalLoadMode
#endif

//*****************************************************************************
//
//! \brief Gets the timer interrupt number.
//!
//! Given a timer base address, this function returns the corresponding
//! interrupt number.
//!
//! \param ui32Base is the base address of the timer module.
//!
//! \return Returns a timer interrupt number, or -1 if \c ui32Base is invalid.
//
//*****************************************************************************
static uint32_t
TimerIntNumberGet(uint32_t ui32Base)
{
    uint32_t ui32Int;

    // Loop through the table that maps timer base addresses to interrupt
    // numbers.
    switch(ui32Base)
    {
    case GPT0_BASE :
        ui32Int = INT_GPT0A;
        break;
    case GPT1_BASE :
        ui32Int = INT_GPT1A;
        break;
    case GPT2_BASE :
        ui32Int = INT_GPT2A;
        break;
    case GPT3_BASE :
        ui32Int = INT_GPT3A;
        break;
    default :
        ui32Int = 0x0;
    }

    // Return the interrupt number or (-1) if not base address is not matched.
    return (ui32Int);
}

//*****************************************************************************
//
// Configures the timer(s)
//
//*****************************************************************************
void
TimerConfigure(uint32_t ui32Base, uint32_t ui32Config)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Config == TIMER_CFG_ONE_SHOT) ||
           (ui32Config == TIMER_CFG_ONE_SHOT_UP) ||
           (ui32Config == TIMER_CFG_PERIODIC) ||
           (ui32Config == TIMER_CFG_PERIODIC_UP) ||
           ((ui32Config & 0xFF000000) == TIMER_CFG_SPLIT_PAIR));
    ASSERT(((ui32Config & 0xFF000000) != TIMER_CFG_SPLIT_PAIR) ||
           ((((ui32Config & 0x000000FF) == TIMER_CFG_A_ONE_SHOT) ||
             ((ui32Config & 0x000000FF) == TIMER_CFG_A_ONE_SHOT_UP) ||
             ((ui32Config & 0x000000FF) == TIMER_CFG_A_PERIODIC) ||
             ((ui32Config & 0x000000FF) == TIMER_CFG_A_PERIODIC_UP) ||
             ((ui32Config & 0x000000FF) == TIMER_CFG_A_CAP_COUNT) ||
             ((ui32Config & 0x000000FF) == TIMER_CFG_A_CAP_COUNT_UP) ||
             ((ui32Config & 0x000000FF) == TIMER_CFG_A_CAP_TIME) ||
             ((ui32Config & 0x000000FF) == TIMER_CFG_A_CAP_TIME_UP) ||
             ((ui32Config & 0x000000FF) == TIMER_CFG_A_PWM)) &&
            (((ui32Config & 0x0000FF00) == TIMER_CFG_B_ONE_SHOT) ||
             ((ui32Config & 0x0000FF00) == TIMER_CFG_B_ONE_SHOT_UP) ||
             ((ui32Config & 0x0000FF00) == TIMER_CFG_B_PERIODIC) ||
             ((ui32Config & 0x0000FF00) == TIMER_CFG_B_PERIODIC_UP) ||
             ((ui32Config & 0x0000FF00) == TIMER_CFG_B_CAP_COUNT) ||
             ((ui32Config & 0x0000FF00) == TIMER_CFG_B_CAP_COUNT_UP) ||
             ((ui32Config & 0x0000FF00) == TIMER_CFG_B_CAP_TIME) ||
             ((ui32Config & 0x0000FF00) == TIMER_CFG_B_CAP_TIME_UP) ||
             ((ui32Config & 0x0000FF00) == TIMER_CFG_B_PWM))));

    // Disable the timers.
    HWREG(ui32Base + GPT_O_CTL) &= ~(GPT_CTL_TAEN | GPT_CTL_TBEN);

    // Set the global timer configuration.
    HWREG(ui32Base + GPT_O_CFG) = ui32Config >> 24;

    // Set the configuration of the A and B timers. Note that the B timer
    // configuration is ignored by the hardware in 32-bit modes.
    HWREG(ui32Base + GPT_O_TAMR) = (ui32Config & 0xFF) | GPT_TAMR_TAPWMIE;
    HWREG(ui32Base + GPT_O_TBMR) =
        ((ui32Config >> 8) & 0xFF) | GPT_TBMR_TBPWMIE;
}

//*****************************************************************************
//
// Controls the output level
//
//*****************************************************************************
void
TimerLevelControl(uint32_t ui32Base, uint32_t ui32Timer, bool bInvert)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));

    // Set the output levels as requested.
    ui32Timer &= GPT_CTL_TAPWML | GPT_CTL_TBPWML;
    HWREG(ui32Base + GPT_O_CTL) = (bInvert ?
                                   (HWREG(ui32Base + GPT_O_CTL) | ui32Timer) :
                                   (HWREG(ui32Base + GPT_O_CTL) &
                                   ~(ui32Timer)));
}

//*****************************************************************************
//
// Controls the stall handling
//
//*****************************************************************************
void
TimerStallControl(uint32_t ui32Base, uint32_t ui32Timer, bool bStall)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));

    // Set the stall mode.
    ui32Timer &= GPT_CTL_TASTALL | GPT_CTL_TBSTALL;
    HWREG(ui32Base + GPT_O_CTL) = (bStall ?
                                   (HWREG(ui32Base + GPT_O_CTL) | ui32Timer) :
                                   (HWREG(ui32Base + GPT_O_CTL) & ~(ui32Timer)));
}

//*****************************************************************************
//
// Controls the wait on trigger handling
//
//*****************************************************************************
void
TimerWaitOnTriggerControl(uint32_t ui32Base, uint32_t ui32Timer, bool bWait)
{
    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));

    // Set the wait on trigger mode for timer A.
    if(ui32Timer & TIMER_A)
    {
        if(bWait)
        {
            HWREG(ui32Base + GPT_O_TAMR) |= GPT_TAMR_TAWOT;
        }
        else
        {
            HWREG(ui32Base + GPT_O_TAMR) &= ~(GPT_TAMR_TAWOT);
        }
    }

    // Set the wait on trigger mode for timer B.
    if(ui32Timer & TIMER_B)
    {
        if(bWait)
        {
            HWREG(ui32Base + GPT_O_TBMR) |= GPT_TBMR_TBWOT;
        }
        else
        {
            HWREG(ui32Base + GPT_O_TBMR) &= ~(GPT_TBMR_TBWOT);
        }
    }
}

//*****************************************************************************
//
// Registers an interrupt handler for the timer interrupt
//
//*****************************************************************************
void
TimerIntRegister(uint32_t ui32Base, uint32_t ui32Timer, void (*pfnHandler)(void))
{
    uint32_t ui32Int;

    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));

    // Get the interrupt number for this timer module.
    ui32Int = TimerIntNumberGet(ui32Base);

    // Register an interrupt handler for timer A if requested.
    if(ui32Timer & TIMER_A)
    {
        // Register the interrupt handler.
        IntRegister(ui32Int, pfnHandler);

        // Enable the interrupt.
        IntEnable(ui32Int);
    }

    // Register an interrupt handler for timer B if requested.
    if(ui32Timer & TIMER_B)
    {
        // Register the interrupt handler.
        IntRegister(ui32Int + 1, pfnHandler);

        // Enable the interrupt.
        IntEnable(ui32Int + 1);
    }
}

//*****************************************************************************
//
// Unregisters an interrupt handler for the timer interrupt
//
//*****************************************************************************
void
TimerIntUnregister(uint32_t ui32Base, uint32_t ui32Timer)
{
    uint32_t ui32Int;

    // Check the arguments.
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) ||
           (ui32Timer == TIMER_BOTH));

    // Get the interrupt number for this timer module.
    ui32Int = TimerIntNumberGet(ui32Base);

    // Unregister the interrupt handler for timer A if requested.
    if(ui32Timer & TIMER_A)
    {
        // Disable the interrupt.
        IntDisable(ui32Int);

        // Unregister the interrupt handler.
        IntUnregister(ui32Int);
    }

    // Unregister the interrupt handler for timer B if requested.
    if(ui32Timer & TIMER_B)
    {
        // Disable the interrupt.
        IntDisable(ui32Int + 1);

        // Unregister the interrupt handler.
        IntUnregister(ui32Int + 1);
    }
}

//*****************************************************************************
//
// Sets the Match Register Update mode
//
//*****************************************************************************
void
TimerMatchUpdateMode(uint32_t ui32Base, uint32_t ui32Timer, uint32_t ui32Mode)
{
    // Check the arguments
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) || (ui32Timer == TIMER_BOTH));
    ASSERT((ui32Mode == TIMER_MATCHUPDATE_NEXTCYCLE) || (ui32Mode == TIMER_MATCHUPDATE_TIMEOUT));

    // Set mode for timer A
    if(ui32Timer & TIMER_A)
    {
        if(ui32Mode == TIMER_MATCHUPDATE_NEXTCYCLE)
        {
            HWREG(ui32Base + GPT_O_TAMR) &= ~(GPT_TAMR_TAMRSU);
        }
        else
        {
            HWREG(ui32Base + GPT_O_TAMR) |= GPT_TAMR_TAMRSU;
        }
    }

    // Set mode for timer B
    if(ui32Timer & TIMER_B)
    {
        if(ui32Mode == TIMER_MATCHUPDATE_NEXTCYCLE)
        {
            HWREG(ui32Base + GPT_O_TBMR) &= ~(GPT_TBMR_TBMRSU);
        }
        else
        {
            HWREG(ui32Base + GPT_O_TBMR) |= GPT_TBMR_TBMRSU;
        }
    }
}

//*****************************************************************************
//
// Sets the Interval Load mode
//
//*****************************************************************************
void
TimerIntervalLoadMode(uint32_t ui32Base, uint32_t ui32Timer, uint32_t ui32Mode)
{
    // Check the arguments
    ASSERT(TimerBaseValid(ui32Base));
    ASSERT((ui32Timer == TIMER_A) || (ui32Timer == TIMER_B) || (ui32Timer == TIMER_BOTH));
    ASSERT((ui32Mode == TIMER_INTERVALLOAD_NEXTCYCLE) || (ui32Mode == TIMER_INTERVALLOAD_TIMEOUT));

    // Set mode for timer A
    if(ui32Timer & TIMER_A)
    {
        if(ui32Mode == TIMER_INTERVALLOAD_NEXTCYCLE)
        {
            HWREG(ui32Base + GPT_O_TAMR) &= ~(GPT_TAMR_TAILD);
        }
        else
        {
            HWREG(ui32Base + GPT_O_TAMR) |= GPT_TAMR_TAILD;
        }
    }

    // Set mode for timer B
    if(ui32Timer & TIMER_B)
    {
        if(ui32Mode == TIMER_INTERVALLOAD_NEXTCYCLE)
        {
            HWREG(ui32Base + GPT_O_TBMR) &= ~(GPT_TBMR_TBILD);
        }
        else
        {
            HWREG(ui32Base + GPT_O_TBMR) |= GPT_TBMR_TBILD;
        }
    }
}
