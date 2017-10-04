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
#include <ti/devices/msp432p4xx/driverlib/gpio.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>


static const uint32_t GPIO_PORT_TO_INT[] =
{ 0x00,
INT_PORT1,
INT_PORT2,
INT_PORT3,
INT_PORT4,
INT_PORT5,
INT_PORT6 };

static const uint32_t GPIO_PORT_TO_BASE[] =
{   0x00,
    (uint32_t)P1,
    (uint32_t)P1+1,
    (uint32_t)P3,
    (uint32_t)P3+1,
    (uint32_t)P5,
    (uint32_t)P5+1,
    (uint32_t)P7,
    (uint32_t)P7+1,
    (uint32_t)P9,
    (uint32_t)P9+1,
    (uint32_t)PJ
};

void GPIO_setAsOutputPin(uint_fast8_t selectedPort, uint_fast16_t selectedPins)
{
    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    HWREG16(baseAddress + OFS_LIB_PASEL0) &= ~selectedPins;
    HWREG16(baseAddress + OFS_LIB_PASEL1) &= ~selectedPins;
    HWREG16(baseAddress + OFS_LIB_PADIR) |= selectedPins;
}


void GPIO_setAsInputPin(uint_fast8_t selectedPort, uint_fast16_t selectedPins)
{
    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    HWREG16(baseAddress + OFS_LIB_PASEL0) &= ~selectedPins;
    HWREG16(baseAddress + OFS_LIB_PASEL1) &= ~selectedPins;
    HWREG16(baseAddress + OFS_LIB_PADIR) &= ~selectedPins;
    HWREG16(baseAddress + OFS_LIB_PAREN) &= ~selectedPins;
}


void GPIO_setAsPeripheralModuleFunctionOutputPin(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins, uint_fast8_t mode)
{

    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    HWREG16(baseAddress + OFS_LIB_PADIR) |= selectedPins;
    switch (mode)
    {
    case GPIO_PRIMARY_MODULE_FUNCTION:
        HWREG16(baseAddress + OFS_LIB_PASEL0) |= selectedPins;
        HWREG16(baseAddress + OFS_LIB_PASEL1) &= ~selectedPins;
        break;
    case GPIO_SECONDARY_MODULE_FUNCTION:
        HWREG16(baseAddress + OFS_LIB_PASEL0) &= ~selectedPins;
        HWREG16(baseAddress + OFS_LIB_PASEL1) |= selectedPins;
        break;
    case GPIO_TERTIARY_MODULE_FUNCTION:
        HWREG16(baseAddress + OFS_LIB_PASEL0) |= selectedPins;
        HWREG16(baseAddress + OFS_LIB_PASEL1) |= selectedPins;
        break;
    }
}


void GPIO_setAsPeripheralModuleFunctionInputPin(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins, uint_fast8_t mode)
{
    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    HWREG16(baseAddress + OFS_LIB_PADIR) &= ~selectedPins;
    switch (mode)
    {
    case GPIO_PRIMARY_MODULE_FUNCTION:
        HWREG16(baseAddress + OFS_LIB_PASEL0) |= selectedPins;
        HWREG16(baseAddress + OFS_LIB_PASEL1) &= ~selectedPins;
        break;
    case GPIO_SECONDARY_MODULE_FUNCTION:
        HWREG16(baseAddress + OFS_LIB_PASEL0) &= ~selectedPins;
        HWREG16(baseAddress + OFS_LIB_PASEL1) |= selectedPins;
        break;
    case GPIO_TERTIARY_MODULE_FUNCTION:
        HWREG16(baseAddress + OFS_LIB_PASEL0) |= selectedPins;
        HWREG16(baseAddress + OFS_LIB_PASEL1) |= selectedPins;
        break;
    }
}


void GPIO_setOutputHighOnPin(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins)
{

    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    HWREG16(baseAddress + OFS_LIB_PAOUT) |= selectedPins;
}


void GPIO_setOutputLowOnPin(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins)
{

    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    HWREG16(baseAddress + OFS_LIB_PAOUT) &= ~selectedPins;
}


void GPIO_toggleOutputOnPin(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins)
{

    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    HWREG16(baseAddress + OFS_LIB_PAOUT) ^= selectedPins;
}


void GPIO_setAsInputPinWithPullDownResistor(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins)
{

    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    HWREG16(baseAddress + OFS_LIB_PASEL0) &= ~selectedPins;
    HWREG16(baseAddress + OFS_LIB_PASEL1) &= ~selectedPins;

    HWREG16(baseAddress + OFS_LIB_PADIR) &= ~selectedPins;
    HWREG16(baseAddress + OFS_LIB_PAREN) |= selectedPins;
    HWREG16(baseAddress + OFS_LIB_PAOUT) &= ~selectedPins;
}


void GPIO_setAsInputPinWithPullUpResistor(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins)
{

    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    HWREG16(baseAddress + OFS_LIB_PASEL0) &= ~selectedPins;
    HWREG16(baseAddress + OFS_LIB_PASEL1) &= ~selectedPins;
    HWREG16(baseAddress + OFS_LIB_PADIR) &= ~selectedPins;
    HWREG16(baseAddress + OFS_LIB_PAREN) |= selectedPins;
    HWREG16(baseAddress + OFS_LIB_PAOUT) |= selectedPins;
}


uint8_t GPIO_getInputPinValue(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins)
{
    uint_fast16_t inputPinValue;
    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    inputPinValue = HWREG16(baseAddress + OFS_LIB_PAIN) & (selectedPins);

    if (inputPinValue > 0)
        return GPIO_INPUT_PIN_HIGH;
    return GPIO_INPUT_PIN_LOW;
}


void GPIO_enableInterrupt(uint_fast8_t selectedPort, uint_fast16_t selectedPins)
{

    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    HWREG16(baseAddress + OFS_LIB_PAIE) |= selectedPins;
}


void GPIO_disableInterrupt(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins)
{

    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    HWREG16(baseAddress + OFS_LIB_PAIE) &= ~selectedPins;
}


uint_fast16_t GPIO_getInterruptStatus(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins)
{

    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];

    return HWREG16(baseAddress + OFS_LIB_PAIFG) & selectedPins;
}


void GPIO_clearInterruptFlag(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins)
{

    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];


    HWREG16(baseAddress + OFS_LIB_PAIFG) &= ~selectedPins;
}


void GPIO_interruptEdgeSelect(uint_fast8_t selectedPort,
        uint_fast16_t selectedPins, uint_fast8_t edgeSelect)
{

    uint32_t baseAddress = GPIO_PORT_TO_BASE[selectedPort];


    if (GPIO_LOW_TO_HIGH_TRANSITION == edgeSelect)
        HWREG16(baseAddress + OFS_LIB_PAIES) &= ~selectedPins;
    else
        HWREG16(baseAddress + OFS_LIB_PAIES) |= selectedPins;
}

uint_fast16_t GPIO_getEnabledInterruptStatus(uint_fast8_t selectedPort)
{
    uint_fast16_t pendingInts;
    uint32_t baseAddr;

    pendingInts = GPIO_getInterruptStatus(selectedPort, 0xFFFF);
    baseAddr = GPIO_PORT_TO_BASE[selectedPort];

    ASSERT(baseAddr != 0xFFFF);

    switch (selectedPort)
    {
    case GPIO_PORT_P1:
    case GPIO_PORT_P3:
    case GPIO_PORT_P5:
    case GPIO_PORT_P7:
    case GPIO_PORT_P9:
        return (HWREG8(baseAddr + OFS_LIB_P1IE) & pendingInts);
    case GPIO_PORT_P2:
    case GPIO_PORT_P4:
    case GPIO_PORT_P6:
    case GPIO_PORT_P8:
    case GPIO_PORT_P10:
        return (HWREG8(baseAddr + OFS_LIB_P2IE) & pendingInts);
    case GPIO_PORT_PJ:
        return (HWREG16(baseAddr + OFS_LIB_PAIE) & pendingInts);
    default:
        return 0;
    }
}


void GPIO_setDriveStrengthHigh(uint_fast8_t selectedPort,
        uint_fast8_t selectedPins)
{
    uint32_t baseAddr;

    baseAddr = GPIO_PORT_TO_BASE[selectedPort];

    HWREG8(baseAddr + OFS_LIB_PADS) |= selectedPins;

}

void GPIO_setDriveStrengthLow(uint_fast8_t selectedPort,
        uint_fast8_t selectedPins)
{
    uint32_t baseAddr;

    baseAddr = GPIO_PORT_TO_BASE[selectedPort];

    HWREG8(baseAddr + OFS_LIB_PADS) &= ~selectedPins;

}

void GPIO_registerInterrupt(uint_fast8_t selectedPort, void (*intHandler)(void))
{
    uint32_t wPortInt;

    wPortInt = GPIO_PORT_TO_INT[selectedPort];

    //
    // Register the interrupt handler, returning an error if an error occurs.
    //
    Interrupt_registerInterrupt(wPortInt, intHandler);

    //
    // Enable the system control interrupt.
    //
    Interrupt_enableInterrupt(wPortInt);
}


void GPIO_unregisterInterrupt(uint_fast8_t selectedPort)
{
    uint32_t wPortInt;

    wPortInt = GPIO_PORT_TO_INT[selectedPort];

    //
    // Disable the interrupt.
    //
    Interrupt_disableInterrupt(wPortInt);

    //
    // Unregister the interrupt handler.
    //
    Interrupt_unregisterInterrupt(wPortInt);
}


