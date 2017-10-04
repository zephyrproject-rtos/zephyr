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
#include <ti/devices/msp432p4xx/driverlib/pss.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>
#include <ti/devices/msp432p4xx/driverlib/cpu.h>

static void __PSSUnlock()
{
    PSS->KEY = PSS_KEY_VALUE;
}

static void __PSSLock()
{
    PSS->KEY = 0;
}


void PSS_enableForcedDCDCOperation(void)
{
    __PSSUnlock();

    BITBAND_PERI(PSS->CTL0, PSS_CTL0_DCDC_FORCE_OFS) = 1;

    __PSSLock();
}

void PSS_disableForcedDCDCOperation(void)
{
    __PSSUnlock();

    BITBAND_PERI(PSS->CTL0, PSS_CTL0_DCDC_FORCE_OFS) = 0;

    __PSSLock();

}

void PSS_enableHighSidePinToggle(bool activeLow)
{
    __PSSUnlock();

    if (activeLow)
        PSS->CTL0 |= (PSS_CTL0_SVMHOE | PSS_CTL0_SVMHOUTPOLAL);
    else
    {
        BITBAND_PERI(PSS->CTL0, PSS_CTL0_SVMHOUTPOLAL_OFS) = 0;
        BITBAND_PERI(PSS->CTL0, PSS_CTL0_SVMHOE_OFS) = 1;
    }

    __PSSLock();
}

void PSS_disableHighSidePinToggle(void)
{
    __PSSUnlock();

    BITBAND_PERI(PSS->CTL0, PSS_CTL0_SVMHOE_OFS) = 0;

    __PSSLock();
}

void PSS_enableHighSide(void)
{
    __PSSUnlock();

    BITBAND_PERI(PSS->CTL0, PSS_CTL0_SVSMHOFF_OFS) = 0;

    __PSSLock();
}

void PSS_disableHighSide(void)
{
    __PSSUnlock();

    BITBAND_PERI(PSS->CTL0, PSS_CTL0_SVSMHOFF_OFS) = 1;

    __PSSLock();
}

void PSS_setHighSidePerformanceMode(uint_fast8_t powerMode)
{
    __PSSUnlock();

    if (powerMode == PSS_FULL_PERFORMANCE_MODE)
        BITBAND_PERI(PSS->CTL0, PSS_CTL0_SVSMHLP_OFS) = 0;
    else
        BITBAND_PERI(PSS->CTL0, PSS_CTL0_SVSMHLP_OFS) = 1;

    __PSSLock();
}

uint_fast8_t PSS_getHighSidePerformanceMode(void)
{
    if (BITBAND_PERI(PSS->CTL0, PSS_CTL0_SVSMHLP_OFS))
        return PSS_NORMAL_PERFORMANCE_MODE;
    else
        return PSS_FULL_PERFORMANCE_MODE;
}

void PSS_enableHighSideMonitor(void)
{
    __PSSUnlock();

    BITBAND_PERI(PSS->CTL0, PSS_CTL0_SVSMHS_OFS) = 1;

    __PSSLock();
}

void PSS_disableHighSideMonitor(void)
{
    __PSSUnlock();

    BITBAND_PERI(PSS->CTL0, PSS_CTL0_SVSMHS_OFS) = 0;

    __PSSLock();
}

void PSS_setHighSideVoltageTrigger(uint_fast8_t triggerVoltage)
{
    __PSSUnlock();

    ASSERT(!(triggerVoltage & 0xF8))

    PSS->CTL0 &= ~PSS_CTL0_SVSMHTH_MASK;
    PSS->CTL0 |= (triggerVoltage & 0x07) << PSS_CTL0_SVSMHTH_OFS;

    __PSSLock();
}

uint_fast8_t PSS_getHighSideVoltageTrigger(void)
{
    return (uint_fast8_t)((PSS->CTL0 & PSS_CTL0_SVSMHTH_MASK)
            >> PSS_CTL0_SVSMHTH_OFS);
}

void PSS_enableInterrupt(void)
{
    __PSSUnlock();
    BITBAND_PERI(PSS->IE,PSS_IE_SVSMHIE_OFS) = 1;
    __PSSLock();
}

void PSS_disableInterrupt(void)
{
    __PSSUnlock();
    BITBAND_PERI(PSS->IE,PSS_IE_SVSMHIE_OFS) = 0;
    __PSSLock();
}

uint32_t PSS_getInterruptStatus(void)
{
    return PSS->IFG;
}

void PSS_clearInterruptFlag(void)
{
    __PSSUnlock();
    BITBAND_PERI(PSS->CLRIFG,PSS_CLRIFG_CLRSVSMHIFG_OFS) = 0;
    __PSSLock();
}

void PSS_registerInterrupt(void (*intHandler)(void))
{
    //
    // Register the interrupt handler, returning an error if an error occurs.
    //
    Interrupt_registerInterrupt(INT_PSS, intHandler);

    //
    // Enable the system control interrupt.
    //
    Interrupt_enableInterrupt(INT_PSS);
}

void PSS_unregisterInterrupt(void)
{
    //
    // Disable the interrupt.
    //
    Interrupt_disableInterrupt(INT_PSS);

    //
    // Unregister the interrupt handler.
    //
    Interrupt_unregisterInterrupt(INT_PSS);
}
