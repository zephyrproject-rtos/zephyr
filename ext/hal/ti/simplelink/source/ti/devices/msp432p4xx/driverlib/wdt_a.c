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
/* Standard Includes */
#include <stdint.h>

/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/wdt_a.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>

#ifdef __MCU_HAS_SYSCTL_A__
#include <ti/devices/msp432p4xx/driverlib/sysctl_a.h>
#else
#include <ti/devices/msp432p4xx/driverlib/sysctl.h>
#endif


void WDT_A_holdTimer(void)
{
    //Set Hold bit
    uint8_t newWDTStatus = (WDT_A->CTL | WDT_A_CTL_HOLD);

    WDT_A->CTL = WDT_A_CTL_PW + newWDTStatus;
}

void WDT_A_startTimer(void)
{
    //Reset Hold bit
    uint8_t newWDTStatus = (WDT_A->CTL & ~(WDT_A_CTL_HOLD));

    WDT_A->CTL = WDT_A_CTL_PW + newWDTStatus;
}

void WDT_A_clearTimer(void)
{
    //Set Counter Clear bit
    uint8_t newWDTStatus = (WDT_A->CTL | WDT_A_CTL_CNTCL);

    WDT_A->CTL = WDT_A_CTL_PW + newWDTStatus;
}

void WDT_A_initWatchdogTimer(uint_fast8_t clockSelect,
        uint_fast8_t clockIterations)
{
    WDT_A->CTL = WDT_A_CTL_PW + WDT_A_CTL_CNTCL + WDT_A_CTL_HOLD +
            clockSelect + clockIterations;
}

void WDT_A_initIntervalTimer(uint_fast8_t clockSelect,
        uint_fast8_t clockIterations)
{

    WDT_A->CTL = WDT_A_CTL_PW + WDT_A_CTL_CNTCL + WDT_A_CTL_HOLD + WDT_A_CTL_TMSEL
            + clockSelect + clockIterations;
}

void WDT_A_setPasswordViolationReset(uint_fast8_t resetType)
{
#ifdef __MCU_HAS_SYSCTL_A__
    SysCtl_A_setWDTPasswordViolationResetType(resetType);
#else
    SysCtl_setWDTPasswordViolationResetType(resetType);
#endif
}

void WDT_A_setTimeoutReset(uint_fast8_t resetType)
{
#ifdef __MCU_HAS_SYSCTL_A__
    SysCtl_A_setWDTTimeoutResetType(resetType);
#else
    SysCtl_setWDTTimeoutResetType(resetType);
#endif
}

void WDT_A_registerInterrupt(void (*intHandler)(void))
{
    //
    // Register the interrupt handler, returning an error if an error occurs.
    //
    Interrupt_registerInterrupt(INT_WDT_A, intHandler);

    //
    // Enable the system control interrupt.
    //
    Interrupt_enableInterrupt (INT_WDT_A);
}

void WDT_A_unregisterInterrupt(void)
{
    //
    // Disable the interrupt.
    //
    Interrupt_disableInterrupt (INT_WDT_A);

    //
    // Unregister the interrupt handler.
    //
    Interrupt_unregisterInterrupt(INT_WDT_A);
}

