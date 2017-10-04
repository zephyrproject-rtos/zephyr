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
#include <ti/devices/msp432p4xx/driverlib/comp_e.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>

static uint16_t __getRegisterSettingForInput(uint32_t input)
{
    switch (input)
    {
    case COMP_E_INPUT0:
        return COMP_E_CTL0_IPSEL_0;
    case COMP_E_INPUT1:
        return COMP_E_CTL0_IPSEL_1;
    case COMP_E_INPUT2:
        return COMP_E_CTL0_IPSEL_2;
    case COMP_E_INPUT3:
        return COMP_E_CTL0_IPSEL_3;
    case COMP_E_INPUT4:
        return COMP_E_CTL0_IPSEL_4;
    case COMP_E_INPUT5:
        return COMP_E_CTL0_IPSEL_5;
    case COMP_E_INPUT6:
        return COMP_E_CTL0_IPSEL_6;
    case COMP_E_INPUT7:
        return COMP_E_CTL0_IPSEL_7;
    case COMP_E_INPUT8:
        return COMP_E_CTL0_IPSEL_8;
    case COMP_E_INPUT9:
        return COMP_E_CTL0_IPSEL_9;
    case COMP_E_INPUT10:
        return COMP_E_CTL0_IPSEL_10;
    case COMP_E_INPUT11:
        return COMP_E_CTL0_IPSEL_11;
    case COMP_E_INPUT12:
        return COMP_E_CTL0_IPSEL_12;
    case COMP_E_INPUT13:
        return COMP_E_CTL0_IPSEL_13;
    case COMP_E_INPUT14:
        return COMP_E_CTL0_IPSEL_14;
    case COMP_E_INPUT15:
        return COMP_E_CTL0_IPSEL_15;
    case COMP_E_VREF:
        return COMP_E_VREF;
    default:
        ASSERT(false);
        return 0x11;
    }

}

bool COMP_E_initModule(uint32_t comparator, const COMP_E_Config *config)
{
    uint_fast8_t positiveTerminalInput = __getRegisterSettingForInput(
            config->positiveTerminalInput);
    uint_fast8_t negativeTerminalInput = __getRegisterSettingForInput(
            config->negativeTerminalInput);
    bool retVal = true;

    ASSERT(positiveTerminalInput < 0x10); ASSERT(negativeTerminalInput < 0x10);
    ASSERT(positiveTerminalInput != negativeTerminalInput);
    ASSERT(
            config->outputFilterEnableAndDelayLevel
            <= COMP_E_FILTEROUTPUT_DLYLVL4);

    /* Reset COMPE Control 1 & Interrupt Registers for initialization */
    COMP_E_CMSIS(comparator)->CTL0 = 0;
    COMP_E_CMSIS(comparator)->INT = 0;

    // Set the Positive Terminal
    if (COMP_E_VREF != positiveTerminalInput)
    {
        // Enable Positive Terminal Input Mux and Set to the appropriate input
        COMP_E_CMSIS(comparator)->CTL0 |= COMP_E_CTL0_IPEN
                + positiveTerminalInput;

        // Disable the input buffer
        COMP_E_CMSIS(comparator)->CTL3 |= (1 << positiveTerminalInput);
    } else
    {
        //  Reset and Set COMPE Control 2 Register
        BITBAND_PERI(COMP_E_CMSIS(comparator)->CTL2,COMP_E_CTL2_RSEL_OFS) = 0;
    }

    // Set the Negative Terminal
    if (COMP_E_VREF != negativeTerminalInput)
    {
        // Enable Negative Terminal Input Mux and Set  to the appropriate input
        COMP_E_CMSIS(comparator)->CTL0 |= COMP_E_CTL0_IMEN
                + (negativeTerminalInput << 8);

        // Disable the input buffer
        COMP_E_CMSIS(comparator)->CTL3 |= (1 << negativeTerminalInput);
    } else
    {
        // Reset and Set COMPE Control 2 Register
        BITBAND_PERI(COMP_E_CMSIS(comparator)->CTL2, COMP_E_CTL2_RSEL_OFS) = 1;
    }

    // Reset and Set COMPE Control 1 Register
    COMP_E_CMSIS(comparator)->CTL1 = config->powerMode
            + config->outputFilterEnableAndDelayLevel
            + config->invertedOutputPolarity;

    return retVal;
}

void COMP_E_setReferenceVoltage(uint32_t comparator,
        uint_fast16_t supplyVoltageReferenceBase,
        uint_fast16_t lowerLimitSupplyVoltageFractionOf32,
        uint_fast16_t upperLimitSupplyVoltageFractionOf32)
{
    ASSERT(supplyVoltageReferenceBase <= COMP_E_VREFBASE2_5V);
    ASSERT(upperLimitSupplyVoltageFractionOf32 <= 32);
    ASSERT(lowerLimitSupplyVoltageFractionOf32 <= 32); 
    ASSERT(upperLimitSupplyVoltageFractionOf32
            >= lowerLimitSupplyVoltageFractionOf32);

    BITBAND_PERI(COMP_E_CMSIS(comparator)->CTL1, COMP_E_CTL1_MRVS_OFS) = 0;
    COMP_E_CMSIS(comparator)->CTL2 &= COMP_E_CTL2_RSEL;

    // Set Voltage Source(Vcc | Vref, resistor ladder or not)
    if (COMP_E_REFERENCE_AMPLIFIER_DISABLED == supplyVoltageReferenceBase)
    {
        COMP_E_CMSIS(comparator)->CTL2 |= COMP_E_CTL2_RS_1;
    } else if (lowerLimitSupplyVoltageFractionOf32 == 32)
    {
        COMP_E_CMSIS(comparator)->CTL2 |= COMP_E_CTL2_RS_3;
    } else
    {
        COMP_E_CMSIS(comparator)->CTL2 |= COMP_E_CTL2_RS_2;
    }

    // Set COMPE Control 2 Register
    COMP_E_CMSIS(comparator)->CTL2 |= supplyVoltageReferenceBase
            + ((upperLimitSupplyVoltageFractionOf32 - 1) << 8)
            + (lowerLimitSupplyVoltageFractionOf32 - 1);
}

void COMP_E_setReferenceAccuracy(uint32_t comparator,
        uint_fast16_t referenceAccuracy)
{
    ASSERT(
            (referenceAccuracy == COMP_E_ACCURACY_STATIC)
            || (referenceAccuracy == COMP_E_ACCURACY_CLOCKED));

    if (referenceAccuracy)
        BITBAND_PERI(COMP_E_CMSIS(comparator)->CTL2, COMP_E_CTL2_REFACC_OFS) = 1;
    else
        BITBAND_PERI(COMP_E_CMSIS(comparator)->CTL2, COMP_E_CTL2_REFACC_OFS) = 0;

}

void COMP_E_setPowerMode(uint32_t comparator, uint_fast16_t powerMode)
{
    COMP_E_CMSIS(comparator)->CTL1 = (COMP_E_CMSIS(comparator)->CTL1
            & ~(COMP_E_CTL1_PWRMD_MASK)) | powerMode;
}

void COMP_E_enableModule(uint32_t comparator)
{
    BITBAND_PERI(COMP_E_CMSIS(comparator)->CTL1, COMP_E_CTL1_ON_OFS) = 1;
}

void COMP_E_disableModule(uint32_t comparator)
{
    BITBAND_PERI(COMP_E_CMSIS(comparator)->CTL1, COMP_E_CTL1_ON_OFS) = 0;
}

void COMP_E_shortInputs(uint32_t comparator)
{
    BITBAND_PERI(COMP_E_CMSIS(comparator)->CTL1, COMP_E_CTL1_SHORT_OFS) = 1;
}

void COMP_E_unshortInputs(uint32_t comparator)
{
    BITBAND_PERI(COMP_E_CMSIS(comparator)->CTL1, COMP_E_CTL1_SHORT_OFS) = 0;
}

void COMP_E_disableInputBuffer(uint32_t comparator, uint_fast16_t inputPort)
{
    ASSERT(inputPort <= COMP_E_INPUT15);

    COMP_E_CMSIS(comparator)->CTL3 |= (inputPort);
}

void COMP_E_enableInputBuffer(uint32_t comparator, uint_fast16_t inputPort)
{
    ASSERT(inputPort <= COMP_E_INPUT15);

    COMP_E_CMSIS(comparator)->CTL3 &= ~(inputPort);
}

void COMP_E_swapIO(uint32_t comparator)
{
    COMP_E_CMSIS(comparator)->CTL1 ^= COMP_E_CTL1_EX; // Toggle CEEX bit
}

uint8_t COMP_E_outputValue(uint32_t comparator)
{
    return COMP_E_CMSIS(comparator)->CTL1 & COMP_E_CTL1_OUT;
}

void COMP_E_enableInterrupt(uint32_t comparator, uint_fast16_t mask)
{
    // Set the Interrupt enable bit
    COMP_E_CMSIS(comparator)->INT |= mask;
}

uint_fast16_t COMP_E_getEnabledInterruptStatus(uint32_t comparator)
{
    return COMP_E_getInterruptStatus(comparator) &
    COMP_E_CMSIS(comparator)->INT;
}

void COMP_E_disableInterrupt(uint32_t comparator, uint_fast16_t mask)
{
    COMP_E_CMSIS(comparator)->INT &= ~(mask);
}

void COMP_E_clearInterruptFlag(uint32_t comparator, uint_fast16_t mask)
{
    COMP_E_CMSIS(comparator)->INT &= ~(mask);
}

uint_fast16_t COMP_E_getInterruptStatus(uint32_t comparator)
{
    return (COMP_E_CMSIS(comparator)->INT & (COMP_E_OUTPUT_INTERRUPT_FLAG |
    COMP_E_INTERRUPT_FLAG_INVERTED_POLARITY |
    COMP_E_INTERRUPT_FLAG_READY));
}

void COMP_E_setInterruptEdgeDirection(uint32_t comparator,
        uint_fast8_t edgeDirection)
{
    ASSERT(edgeDirection <= COMP_E_RISINGEDGE);

    // Set the edge direction that will trigger an interrupt
    if (COMP_E_RISINGEDGE == edgeDirection)
        BITBAND_PERI(COMP_E_CMSIS(comparator)->CTL1, COMP_E_CTL1_IES_OFS) = 1;
    else if (COMP_E_FALLINGEDGE == edgeDirection)
        BITBAND_PERI(COMP_E_CMSIS(comparator)->CTL1, COMP_E_CTL1_IES_OFS) = 0;
}

void COMP_E_toggleInterruptEdgeDirection(uint32_t comparator)
{
    COMP_E_CMSIS(comparator)->CTL1 ^= COMP_E_CTL1_IES;
}

void COMP_E_registerInterrupt(uint32_t comparator, void (*intHandler)(void))
{
    switch (comparator)
    {
    case COMP_E0_BASE:
        Interrupt_registerInterrupt(INT_COMP_E0, intHandler);
        Interrupt_enableInterrupt(INT_COMP_E0);
        break;
    case COMP_E1_BASE:
        Interrupt_registerInterrupt(INT_COMP_E1, intHandler);
        Interrupt_enableInterrupt(INT_COMP_E1);
        break;
    default:
        ASSERT(false);
    }
}

void COMP_E_unregisterInterrupt(uint32_t comparator)
{
    switch (comparator)
    {
    case COMP_E0_BASE:
        Interrupt_disableInterrupt(INT_COMP_E0);
        Interrupt_unregisterInterrupt(INT_COMP_E0);
        break;
    case COMP_E1_BASE:
        Interrupt_disableInterrupt(INT_COMP_E1);
        Interrupt_unregisterInterrupt(INT_COMP_E1);
        break;
    default:
        ASSERT(false);
    }
}

