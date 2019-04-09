/******************************************************************************
*  Filename:       uart.c
*  Revised:        2017-06-05 12:13:49 +0200 (Mon, 05 Jun 2017)
*  Revision:       49096
*
*  Description:    Driver for the UART.
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

#include "uart.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  UARTFIFOLevelGet
    #define UARTFIFOLevelGet                NOROM_UARTFIFOLevelGet
    #undef  UARTConfigSetExpClk
    #define UARTConfigSetExpClk             NOROM_UARTConfigSetExpClk
    #undef  UARTConfigGetExpClk
    #define UARTConfigGetExpClk             NOROM_UARTConfigGetExpClk
    #undef  UARTDisable
    #define UARTDisable                     NOROM_UARTDisable
    #undef  UARTCharGetNonBlocking
    #define UARTCharGetNonBlocking          NOROM_UARTCharGetNonBlocking
    #undef  UARTCharGet
    #define UARTCharGet                     NOROM_UARTCharGet
    #undef  UARTCharPutNonBlocking
    #define UARTCharPutNonBlocking          NOROM_UARTCharPutNonBlocking
    #undef  UARTCharPut
    #define UARTCharPut                     NOROM_UARTCharPut
    #undef  UARTIntRegister
    #define UARTIntRegister                 NOROM_UARTIntRegister
    #undef  UARTIntUnregister
    #define UARTIntUnregister               NOROM_UARTIntUnregister
#endif

//*****************************************************************************
//
// Gets the FIFO level at which interrupts are generated
//
//*****************************************************************************
void
UARTFIFOLevelGet(uint32_t ui32Base, uint32_t *pui32TxLevel,
                 uint32_t *pui32RxLevel)
{
    uint32_t ui32Temp;

    // Check the arguments.
    ASSERT(UARTBaseValid(ui32Base));

    // Read the FIFO level register.
    ui32Temp = HWREG(ui32Base + UART_O_IFLS);

    // Extract the transmit and receive FIFO levels.
    *pui32TxLevel = ui32Temp & UART_IFLS_TXSEL_M;
    *pui32RxLevel = ui32Temp & UART_IFLS_RXSEL_M;
}

//*****************************************************************************
//
// Sets the configuration of a UART
//
//*****************************************************************************
void
UARTConfigSetExpClk(uint32_t ui32Base, uint32_t ui32UARTClk,
                    uint32_t ui32Baud, uint32_t ui32Config)
{
    uint32_t ui32Div;

    // Check the arguments.
    ASSERT(UARTBaseValid(ui32Base));
    ASSERT(ui32Baud != 0);

    // Stop the UART.
    UARTDisable(ui32Base);

    // Compute the fractional baud rate divider.
    ui32Div = (((ui32UARTClk * 8) / ui32Baud) + 1) / 2;

    // Set the baud rate.
    HWREG(ui32Base + UART_O_IBRD) = ui32Div / 64;
    HWREG(ui32Base + UART_O_FBRD) = ui32Div % 64;

    // Set parity, data length, and number of stop bits.
    HWREG(ui32Base + UART_O_LCRH) = ui32Config;
}

//*****************************************************************************
//
// Gets the current configuration of a UART
//
//*****************************************************************************
void
UARTConfigGetExpClk(uint32_t ui32Base, uint32_t ui32UARTClk,
                    uint32_t *pui32Baud, uint32_t *pui32Config)
{
    uint32_t ui32Int, ui32Frac;

    // Check the arguments.
    ASSERT(UARTBaseValid(ui32Base));

    // Compute the baud rate.
    ui32Int = HWREG(ui32Base + UART_O_IBRD);
    ui32Frac = HWREG(ui32Base + UART_O_FBRD);
    *pui32Baud = (ui32UARTClk * 4) / ((64 * ui32Int) + ui32Frac);

    // Get the parity, data length, and number of stop bits.
    *pui32Config = (HWREG(ui32Base + UART_O_LCRH) &
                    (UART_LCRH_SPS | UART_LCRH_WLEN_M | UART_LCRH_STP2 |
                     UART_LCRH_EPS | UART_LCRH_PEN));
}

//*****************************************************************************
//
// Disables transmitting and receiving
//
//*****************************************************************************
void
UARTDisable(uint32_t ui32Base)
{

    // Check the arguments.
    ASSERT(UARTBaseValid(ui32Base));

    // Wait for end of TX.
    while(HWREG(ui32Base + UART_O_FR) & UART_FR_BUSY)
    {
    }

    // Disable the FIFO.
    HWREG(ui32Base + UART_O_LCRH) &= ~(UART_LCRH_FEN);

    // Disable the UART.
    HWREG(ui32Base + UART_O_CTL) &= ~(UART_CTL_UARTEN | UART_CTL_TXE |
                                      UART_CTL_RXE);
}

//*****************************************************************************
//
// Receives a character from the specified port
//
//*****************************************************************************
int32_t
UARTCharGetNonBlocking(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(UARTBaseValid(ui32Base));

    // See if there are any characters in the receive FIFO.
    if(!(HWREG(ui32Base + UART_O_FR) & UART_FR_RXFE))
    {
        // Read and return the next character.
        return(HWREG(ui32Base + UART_O_DR));
    }
    else
    {
        // There are no characters, so return a failure.
        return(-1);
    }
}

//*****************************************************************************
//
// Waits for a character from the specified port
//
//*****************************************************************************
int32_t
UARTCharGet(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(UARTBaseValid(ui32Base));

    // Wait until a char is available.
    while(HWREG(ui32Base + UART_O_FR) & UART_FR_RXFE)
    {
    }

    // Now get the character.
    return(HWREG(ui32Base + UART_O_DR));
}

//*****************************************************************************
//
// Sends a character to the specified port
//
//*****************************************************************************
bool
UARTCharPutNonBlocking(uint32_t ui32Base, uint8_t ui8Data)
{
    // Check the arguments.
    ASSERT(UARTBaseValid(ui32Base));

    // See if there is space in the transmit FIFO.
    if(!(HWREG(ui32Base + UART_O_FR) & UART_FR_TXFF))
    {
        // Write this character to the transmit FIFO.
        HWREG(ui32Base + UART_O_DR) = ui8Data;

        // Success.
        return(true);
    }
    else
    {
        // There is no space in the transmit FIFO, so return a failure.
        return(false);
    }
}

//*****************************************************************************
//
// Waits to send a character from the specified port
//
//*****************************************************************************
void
UARTCharPut(uint32_t ui32Base, uint8_t ui8Data)
{
    // Check the arguments.
    ASSERT(UARTBaseValid(ui32Base));

    // Wait until space is available.
    while(HWREG(ui32Base + UART_O_FR) & UART_FR_TXFF)
    {
    }

    // Send the char.
    HWREG(ui32Base + UART_O_DR) = ui8Data;
}

//*****************************************************************************
//
// Registers an interrupt handler for a UART interrupt
//
//*****************************************************************************
void
UARTIntRegister(uint32_t ui32Base, void (*pfnHandler)(void))
{
    // Check the arguments.
    ASSERT(UARTBaseValid(ui32Base));

    // Register and enable the interrupt handler.
    // (Doing the '& 0xFFFF' to catch both buffered and unbufferd offsets)
    if (( ui32Base & 0xFFFF ) == ( UART0_BASE & 0xFFFF )) {
        IntRegister(INT_UART0_COMB, pfnHandler);
        IntEnable(INT_UART0_COMB);
    } else {
        IntRegister(INT_UART1_COMB, pfnHandler);
        IntEnable(INT_UART1_COMB);
    }
}

//*****************************************************************************
//
// Unregisters an interrupt handler for a UART interrupt
//
//*****************************************************************************
void
UARTIntUnregister(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(UARTBaseValid(ui32Base));

    // Disable and unregister the interrupt.
    // (Doing the '& 0xFFFF' to catch both buffered and unbufferd offsets)
    if (( ui32Base & 0xFFFF ) == ( UART0_BASE & 0xFFFF )) {
        IntDisable(INT_UART0_COMB);
        IntUnregister(INT_UART0_COMB);
    } else {
        IntDisable(INT_UART1_COMB);
        IntUnregister(INT_UART1_COMB);
    }
}
