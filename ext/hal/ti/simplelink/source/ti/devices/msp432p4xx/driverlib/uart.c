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
#include <ti/devices/msp432p4xx/driverlib/uart.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>
#include <ti/devices/msp432p4xx/driverlib/eusci.h>

bool UART_initModule(uint32_t moduleInstance, const eUSCI_UART_Config *config)
{
    bool retVal = true;

    ASSERT(
            (EUSCI_A_UART_MODE == config->uartMode)
            || (EUSCI_A_UART_IDLE_LINE_MULTI_PROCESSOR_MODE
                    == config->uartMode)
            || (EUSCI_A_UART_ADDRESS_BIT_MULTI_PROCESSOR_MODE
                    == config->uartMode)
            || (EUSCI_A_UART_AUTOMATIC_BAUDRATE_DETECTION_MODE
                    == config->uartMode));

    ASSERT(
            (EUSCI_A_UART_CLOCKSOURCE_ACLK == config->selectClockSource)
            || (EUSCI_A_UART_CLOCKSOURCE_SMCLK
                    == config->selectClockSource));

    ASSERT(
            (EUSCI_A_UART_MSB_FIRST == config->msborLsbFirst)
            || (EUSCI_A_UART_LSB_FIRST == config->msborLsbFirst));

    ASSERT(
            (EUSCI_A_UART_ONE_STOP_BIT == config->numberofStopBits)
            || (EUSCI_A_UART_TWO_STOP_BITS == config->numberofStopBits));

    ASSERT(
            (EUSCI_A_UART_NO_PARITY == config->parity)
            || (EUSCI_A_UART_ODD_PARITY == config->parity)
            || (EUSCI_A_UART_EVEN_PARITY == config->parity));

    /* Disable the USCI Module */
    BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;

    /* Clock source select */
    EUSCI_A_CMSIS(moduleInstance)->CTLW0 =
            (EUSCI_A_CMSIS(moduleInstance)->CTLW0 & ~EUSCI_A_CTLW0_SSEL_MASK)
                    | config->selectClockSource;

    /* MSB, LSB select */
    if (config->msborLsbFirst)
        BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_MSB_OFS) = 1;
    else
        BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_MSB_OFS) = 0;

    /* UCSPB = 0(1 stop bit) OR 1(2 stop bits) */
    if (config->numberofStopBits)
        BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_SPB_OFS) = 1;
    else
        BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_SPB_OFS) = 0;

    /* Parity */
    switch (config->parity)
    {
    case EUSCI_A_UART_NO_PARITY:
        BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_PEN_OFS) = 0;
        break;
    case EUSCI_A_UART_ODD_PARITY:
        BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_PEN_OFS) = 1;
        BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_PAR_OFS) = 0;
        break;
    case EUSCI_A_UART_EVEN_PARITY:
        BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_PEN_OFS) = 1;
        BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_PAR_OFS) = 1;
        break;
    }

    /* BaudRate Control Register */
    EUSCI_A_CMSIS(moduleInstance)->BRW = config->clockPrescalar;
    EUSCI_A_CMSIS(moduleInstance)->MCTLW = ((config->secondModReg << 8)
            + (config->firstModReg << 4) + config->overSampling);

    /* Asynchronous mode & 8 bit character select & clear mode */
    EUSCI_A_CMSIS(moduleInstance)->CTLW0 =
            (EUSCI_A_CMSIS(moduleInstance)->CTLW0
                    & ~(EUSCI_A_CTLW0_SYNC | EUSCI_A_CTLW0_SEVENBIT | EUSCI_A_CTLW0_MODE_3 | EUSCI_A_CTLW0_RXEIE | EUSCI_A_CTLW0_BRKIE | EUSCI_A_CTLW0_DORM
                            | EUSCI_A_CTLW0_TXADDR | EUSCI_A_CTLW0_TXBRK)) | config->uartMode;

    return retVal;
}

void UART_transmitData(uint32_t moduleInstance, uint_fast8_t transmitData)
{
    /* If interrupts are not used, poll for flags */
    if (!BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->IE, EUSCI_A_IE_TXIE_OFS))
        while (!BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->IFG, EUSCI_A_IFG_TXIFG_OFS))
            ;

    EUSCI_A_CMSIS(moduleInstance)->TXBUF = transmitData;
}

uint8_t UART_receiveData(uint32_t moduleInstance)
{
    /* If interrupts are not used, poll for flags */
    if (!BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->IE, EUSCI_A_IE_RXIE_OFS))
        while (!BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->IFG, EUSCI_A_IFG_RXIFG_OFS))
            ;

    return EUSCI_A_CMSIS(moduleInstance)->RXBUF;
}

void UART_enableModule(uint32_t moduleInstance)
{
    /* Reset the UCSWRST bit to enable the USCI Module */
    BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 0;
}

void UART_disableModule(uint32_t moduleInstance)
{
    /* Set the UCSWRST bit to disable the USCI Module */
    BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;
}

uint_fast8_t UART_queryStatusFlags(uint32_t moduleInstance, uint_fast8_t mask)
{
    ASSERT(
            0x00 != mask
            && (EUSCI_A_UART_LISTEN_ENABLE + EUSCI_A_UART_FRAMING_ERROR
                    + EUSCI_A_UART_OVERRUN_ERROR
                    + EUSCI_A_UART_PARITY_ERROR
                    + EUSCI_A_UART_BREAK_DETECT
                    + EUSCI_A_UART_RECEIVE_ERROR
                    + EUSCI_A_UART_ADDRESS_RECEIVED
                    + EUSCI_A_UART_IDLELINE + EUSCI_A_UART_BUSY));

    return EUSCI_A_CMSIS(moduleInstance)->STATW & mask;
}

void UART_setDormant(uint32_t moduleInstance)
{
    BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_DORM_OFS) = 1;
}

void UART_resetDormant(uint32_t moduleInstance)
{
    BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_DORM_OFS) = 0;
}

void UART_transmitAddress(uint32_t moduleInstance, uint_fast8_t transmitAddress)
{
    /* Set UCTXADDR bit */
    BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_TXADDR_OFS) = 1;

    /* Place next byte to be sent into the transmit buffer */
    EUSCI_A_CMSIS(moduleInstance)->TXBUF = transmitAddress;
}

void UART_transmitBreak(uint32_t moduleInstance)
{
    /* Set UCTXADDR bit */
    BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_TXBRK_OFS) = 1;

    /* If current mode is automatic baud-rate detection */
    if (EUSCI_A_UART_AUTOMATIC_BAUDRATE_DETECTION_MODE
            == (EUSCI_A_CMSIS(moduleInstance)->CTLW0
                    & EUSCI_A_UART_AUTOMATIC_BAUDRATE_DETECTION_MODE))
        EUSCI_A_CMSIS(moduleInstance)->TXBUF =
        EUSCI_A_UART_AUTOMATICBAUDRATE_SYNC;
    else
        EUSCI_A_CMSIS(moduleInstance)->TXBUF = DEFAULT_SYNC;

    /* If interrupts are not used, poll for flags */
    if (!BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->IE, EUSCI_A_IE_TXIE_OFS))
        while (!BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->IFG, EUSCI_A_IFG_TXIFG_OFS))
            ;
}

uint32_t UART_getReceiveBufferAddressForDMA(uint32_t moduleInstance)
{
    return (uint32_t)&EUSCI_A_CMSIS(moduleInstance)->RXBUF;
}

uint32_t UART_getTransmitBufferAddressForDMA(uint32_t moduleInstance)
{
    return (uint32_t)&EUSCI_B_CMSIS(moduleInstance)->TXBUF;
}

void UART_selectDeglitchTime(uint32_t moduleInstance, uint32_t deglitchTime)
{
    ASSERT(
            (EUSCI_A_UART_DEGLITCH_TIME_2ns == deglitchTime)
            || (EUSCI_A_UART_DEGLITCH_TIME_50ns == deglitchTime)
            || (EUSCI_A_UART_DEGLITCH_TIME_100ns == deglitchTime)
            || (EUSCI_A_UART_DEGLITCH_TIME_200ns == deglitchTime));

    EUSCI_A_CMSIS(moduleInstance)->CTLW1 =
            (EUSCI_A_CMSIS(moduleInstance)->CTLW1 & ~(EUSCI_A_CTLW1_GLIT_MASK))
                    | deglitchTime;

}

void UART_enableInterrupt(uint32_t moduleInstance, uint_fast8_t mask)
{
    uint_fast8_t locMask;

    ASSERT(
            !(mask
                    & ~(EUSCI_A_UART_RECEIVE_INTERRUPT
                            | EUSCI_A_UART_TRANSMIT_INTERRUPT
                            | EUSCI_A_UART_RECEIVE_ERRONEOUSCHAR_INTERRUPT
                            | EUSCI_A_UART_BREAKCHAR_INTERRUPT
                            | EUSCI_A_UART_STARTBIT_INTERRUPT
                            | EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT)));

    locMask = (mask
            & (EUSCI_A_UART_RECEIVE_INTERRUPT | EUSCI_A_UART_TRANSMIT_INTERRUPT
                    | EUSCI_A_UART_STARTBIT_INTERRUPT
                    | EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT));

    EUSCI_A_CMSIS(moduleInstance)->IE |= locMask;

    locMask = (mask
            & (EUSCI_A_UART_RECEIVE_ERRONEOUSCHAR_INTERRUPT
                    | EUSCI_A_UART_BREAKCHAR_INTERRUPT));
    EUSCI_A_CMSIS(moduleInstance)->CTLW0 |= locMask;
}

void UART_disableInterrupt(uint32_t moduleInstance, uint_fast8_t mask)
{
    uint_fast8_t locMask;

    ASSERT(
            !(mask
                    & ~(EUSCI_A_UART_RECEIVE_INTERRUPT
                            | EUSCI_A_UART_TRANSMIT_INTERRUPT
                            | EUSCI_A_UART_RECEIVE_ERRONEOUSCHAR_INTERRUPT
                            | EUSCI_A_UART_BREAKCHAR_INTERRUPT
                            | EUSCI_A_UART_STARTBIT_INTERRUPT
                            | EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT)));

    locMask = (mask
            & (EUSCI_A_UART_RECEIVE_INTERRUPT | EUSCI_A_UART_TRANSMIT_INTERRUPT
                    | EUSCI_A_UART_STARTBIT_INTERRUPT
                    | EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT));
    EUSCI_A_CMSIS(moduleInstance)->IE &= ~locMask;

    locMask = (mask
            & (EUSCI_A_UART_RECEIVE_ERRONEOUSCHAR_INTERRUPT
                    | EUSCI_A_UART_BREAKCHAR_INTERRUPT));
    EUSCI_A_CMSIS(moduleInstance)->CTLW0 &= ~locMask;
}

uint_fast8_t UART_getInterruptStatus(uint32_t moduleInstance, uint8_t mask)
{
    ASSERT(
            !(mask
                    & ~(EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG
                            | EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG
                            | EUSCI_A_UART_STARTBIT_INTERRUPT_FLAG
                            | EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT_FLAG)));

    return EUSCI_A_CMSIS(moduleInstance)->IFG & mask;
}

uint_fast8_t UART_getEnabledInterruptStatus(uint32_t moduleInstance)
{
    uint_fast8_t intStatus = UART_getInterruptStatus(moduleInstance,
    EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG | EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG);
    uint_fast8_t intEnabled = EUSCI_A_CMSIS(moduleInstance)->IE;

    if (!(intEnabled & EUSCI_A_UART_RECEIVE_INTERRUPT))
    {
        intStatus &= ~EUSCI_A_UART_RECEIVE_INTERRUPT;
    }

    if (!(intEnabled & EUSCI_A_UART_TRANSMIT_INTERRUPT))
    {
        intStatus &= ~EUSCI_A_UART_TRANSMIT_INTERRUPT;
    }

    intEnabled = EUSCI_A_CMSIS(moduleInstance)->CTLW0;

    if (!(intEnabled & EUSCI_A_UART_RECEIVE_ERRONEOUSCHAR_INTERRUPT))
    {
        intStatus &= ~EUSCI_A_UART_RECEIVE_ERRONEOUSCHAR_INTERRUPT;
    }

    if (!(intEnabled & EUSCI_A_UART_BREAKCHAR_INTERRUPT))
    {
        intStatus &= ~EUSCI_A_UART_BREAKCHAR_INTERRUPT;
    }

    return intStatus;
}

void UART_clearInterruptFlag(uint32_t moduleInstance, uint_fast8_t mask)
{
    ASSERT(
            !(mask
                    & ~(EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG
                            | EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG
                            | EUSCI_A_UART_STARTBIT_INTERRUPT_FLAG
                            | EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT_FLAG)));

    //Clear the UART interrupt source.
    EUSCI_A_CMSIS(moduleInstance)->IFG &= ~(mask);
}

void UART_registerInterrupt(uint32_t moduleInstance, void (*intHandler)(void))
{
    switch (moduleInstance)
    {
    case EUSCI_A0_BASE:
        Interrupt_registerInterrupt(INT_EUSCIA0, intHandler);
        Interrupt_enableInterrupt(INT_EUSCIA0);
        break;
    case EUSCI_A1_BASE:
        Interrupt_registerInterrupt(INT_EUSCIA1, intHandler);
        Interrupt_enableInterrupt(INT_EUSCIA1);
        break;
#ifdef EUSCI_A2_BASE
    case EUSCI_A2_BASE:
        Interrupt_registerInterrupt(INT_EUSCIA2, intHandler);
        Interrupt_enableInterrupt(INT_EUSCIA2);
        break;
#endif
#ifdef EUSCI_A3_BASE
    case EUSCI_A3_BASE:
        Interrupt_registerInterrupt(INT_EUSCIA3, intHandler);
        Interrupt_enableInterrupt(INT_EUSCIA3);
        break;
#endif
    default:
        ASSERT(false);
    }
}

void UART_unregisterInterrupt(uint32_t moduleInstance)
{
    switch (moduleInstance)
    {
    case EUSCI_A0_BASE:
        Interrupt_disableInterrupt(INT_EUSCIA0);
        Interrupt_unregisterInterrupt(INT_EUSCIA0);
        break;
    case EUSCI_A1_BASE:
        Interrupt_disableInterrupt(INT_EUSCIA1);
        Interrupt_unregisterInterrupt(INT_EUSCIA1);
        break;
#ifdef EUSCI_A2_BASE
    case EUSCI_A2_BASE:
        Interrupt_disableInterrupt(INT_EUSCIA2);
        Interrupt_unregisterInterrupt(INT_EUSCIA2);
        break;
#endif
#ifdef EUSCI_A3_BASE
    case EUSCI_A3_BASE:
        Interrupt_disableInterrupt(INT_EUSCIA3);
        Interrupt_unregisterInterrupt(INT_EUSCIA3);
        break;
#endif
    default:
        ASSERT(false);
    }
}
