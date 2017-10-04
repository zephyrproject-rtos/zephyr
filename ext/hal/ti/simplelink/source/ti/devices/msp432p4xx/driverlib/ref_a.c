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
#include <ti/devices/msp432p4xx/driverlib/ref_a.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>

void REF_A_setReferenceVoltage(uint_fast8_t referenceVoltageSelect)
{
    ASSERT(referenceVoltageSelect <= REF_A_VREF2_5V);

    REF_A->CTL0 = (REF_A->CTL0 &  ~REF_A_CTL0_VSEL_3) | referenceVoltageSelect;
}

void REF_A_disableTempSensor(void)
{
    BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_TCOFF_OFS) = 1;
}

void REF_A_enableTempSensor(void)
{
    BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_TCOFF_OFS) = 0;
}

void REF_A_enableReferenceVoltageOutput(void)
{
    BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_OUT_OFS) = 1;
}

void REF_A_disableReferenceVoltageOutput(void)
{
    BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_OUT_OFS) = 0;
}

void REF_A_enableReferenceVoltage(void)
{
    BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_ON_OFS) = 1;
}

void REF_A_disableReferenceVoltage(void)
{
    BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_ON_OFS) = 0;
}

uint_fast8_t REF_A_getBandgapMode(void)
{
    return (REF_A->CTL0 & REF_A_CTL0_BGMODE);
}

bool REF_A_isBandgapActive(void)
{
    return BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_BGACT_OFS);
}

bool REF_A_isRefGenBusy(void)
{
    return BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_GENBUSY_OFS);
}

bool REF_A_isRefGenActive(void)
{
    return BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_GENACT_OFS);
}

bool REF_A_getBufferedBandgapVoltageStatus(void)
{
    return BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_BGRDY_OFS);
}

bool REF_A_getVariableReferenceVoltageStatus(void)
{
    return BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_GENRDY_OFS);
}

void REF_A_setReferenceVoltageOneTimeTrigger(void)
{
    BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_GENOT_OFS) = 1;
}

void REF_A_setBufferedBandgapVoltageOneTimeTrigger(void)
{
    BITBAND_PERI(REF_A->CTL0,REF_A_CTL0_BGOT_OFS) = 1;
}

