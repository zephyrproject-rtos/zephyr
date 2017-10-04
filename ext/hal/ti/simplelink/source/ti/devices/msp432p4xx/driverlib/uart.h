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
#ifndef UART_H_
#define UART_H_

//*****************************************************************************
//
//! \addtogroup uart_api
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

#include <stdint.h>
#include <stdbool.h>

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/eusci.h>

#define DEFAULT_SYNC                                    0x00
#define EUSCI_A_UART_AUTOMATICBAUDRATE_SYNC             0x55

#define EUSCI_A_UART_NO_PARITY                          0x00
#define EUSCI_A_UART_ODD_PARITY                         0x01
#define EUSCI_A_UART_EVEN_PARITY                        0x02

#define EUSCI_A_UART_MSB_FIRST                          EUSCI_A_CTLW0_MSB
#define EUSCI_A_UART_LSB_FIRST                          0x00

#define EUSCI_A_UART_MODE                               EUSCI_A_CTLW0_MODE_0
#define EUSCI_A_UART_IDLE_LINE_MULTI_PROCESSOR_MODE        EUSCI_A_CTLW0_MODE_1
#define EUSCI_A_UART_ADDRESS_BIT_MULTI_PROCESSOR_MODE   EUSCI_A_CTLW0_MODE_2
#define EUSCI_A_UART_AUTOMATIC_BAUDRATE_DETECTION_MODE  EUSCI_A_CTLW0_MODE_3

#define EUSCI_A_UART_CLOCKSOURCE_SMCLK                  EUSCI_A_CTLW0_SSEL__SMCLK
#define EUSCI_A_UART_CLOCKSOURCE_ACLK                   EUSCI_A_CTLW0_SSEL__ACLK

#define EUSCI_A_UART_ONE_STOP_BIT                       0x00
#define EUSCI_A_UART_TWO_STOP_BITS                      EUSCI_A_CTLW0_SPB

#define EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION   0x01
#define EUSCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION  0x00

#define EUSCI_A_UART_RECEIVE_INTERRUPT                  EUSCI_A_IE_RXIE
#define EUSCI_A_UART_TRANSMIT_INTERRUPT                 EUSCI_A_IE_TXIE
#define EUSCI_A_UART_RECEIVE_ERRONEOUSCHAR_INTERRUPT    EUSCI_A_CTLW0_RXEIE
#define EUSCI_A_UART_BREAKCHAR_INTERRUPT                EUSCI_A_CTLW0_BRKIE
#define EUSCI_A_UART_STARTBIT_INTERRUPT                 EUSCI_A_IE_STTIE
#define EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT        EUSCI_B_IE_STPIE

#define EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG             EUSCI_A_IFG_RXIFG
#define EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG            EUSCI_A_IFG_TXIFG
#define EUSCI_A_UART_STARTBIT_INTERRUPT_FLAG            EUSCI_A_IFG_STTIFG
#define EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT_FLAG   EUSCI_A_IFG_TXCPTIFG

#define EUSCI_A_UART_LISTEN_ENABLE                      EUSCI_A_STATW_LISTEN
#define EUSCI_A_UART_FRAMING_ERROR                      EUSCI_A_STATW_FE
#define EUSCI_A_UART_OVERRUN_ERROR                      EUSCI_A_STATW_OE
#define EUSCI_A_UART_PARITY_ERROR                       EUSCI_A_STATW_PE
#define EUSCI_A_UART_BREAK_DETECT                       EUSCI_A_STATW_BRK
#define EUSCI_A_UART_RECEIVE_ERROR                      EUSCI_A_STATW_RXERR
#define EUSCI_A_UART_ADDRESS_RECEIVED                   EUSCI_A_STATW_ADDR_IDLE
#define EUSCI_A_UART_IDLELINE                           EUSCI_A_STATW_ADDR_IDLE
#define EUSCI_A_UART_BUSY                               EUSCI_A_STATW_BUSY

#define EUSCI_A_UART_DEGLITCH_TIME_2ns                  0x00
#define EUSCI_A_UART_DEGLITCH_TIME_50ns                 0x0001
#define EUSCI_A_UART_DEGLITCH_TIME_100ns                0x0002
#define EUSCI_A_UART_DEGLITCH_TIME_200ns                (0x0001 + 0x0002)

//*****************************************************************************
//
//!     ypedef eUSCI_eUSCI_UART_Config
//! \brief Type definition for \link _eUSCI_UART_Config \endlink
//!     structure
//!
//! \struct _eUSCI_eUSCI_UART_Config
//! \brief Configuration structure for compare mode in the \b UART module. See
//!          \link UART_initModule \endlink for parameter
//!                 documentation.
//
//*****************************************************************************
typedef struct _eUSCI_eUSCI_UART_Config
{
    uint_fast8_t selectClockSource;
    uint_fast16_t clockPrescalar;
    uint_fast8_t firstModReg;
    uint_fast8_t secondModReg;
    uint_fast8_t parity;
    uint_fast16_t msborLsbFirst;
    uint_fast16_t numberofStopBits;
    uint_fast16_t uartMode;
    uint_fast8_t overSampling;
} eUSCI_UART_Config;

//*****************************************************************************
//
//! Initialization routine for the UART block. The values to be written
//! into the UCAxBRW and UCAxMCTLW registers should be pre-computed and passed
//! into the initialization function
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//! \param config Configuration structure for the UART module
//!
//! <hr>
//! <b>Configuration options for \link eUSCI_UART_Config \endlink
//!         structure.</b>
//! <hr>
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode.
//! \param selectClockSource selects Clock source. Valid values are
//!       - \b EUSCI_A_UART_CLOCKSOURCE_SMCLK
//!       - \b EUSCI_A_UART_CLOCKSOURCE_ACLK
//! \param clockPrescalar is the value to be written into UCBRx bits
//! \param firstModReg  is First modulation stage register setting. This
//!     value is a pre-calculated value which can be obtained from the Device
//!     User Guide.This value is written into UCBRFx bits of UCAxMCTLW.
//! \param secondModReg is Second modulation stage register setting.
//!     This value is a pre-calculated value which can be obtained from the
//!     Device User Guide. This value is written into UCBRSx bits of
//!     UCAxMCTLW.
//! \param parity is the desired parity. Valid values are
//!      - \b EUSCI_A_UART_NO_PARITY  [Default Value],
//!      - \b EUSCI_A_UART_ODD_PARITY,
//!      - \b EUSCI_A_UART_EVEN_PARITY
//! \param msborLsbFirst controls direction of receive and transmit shift
//!     register. Valid values are
//!      - \b EUSCI_A_UART_MSB_FIRST
//!      - \b EUSCI_A_UART_LSB_FIRST [Default Value]
//! \param numberofStopBits indicates one/two STOP bits
//!      Valid values are
//!      - \b EUSCI_A_UART_ONE_STOP_BIT [Default Value]
//!      - \b EUSCI_A_UART_TWO_STOP_BITS
//! \param uartMode selects the mode of operation
//!      Valid values are
//!      - \b EUSCI_A_UART_MODE  [Default Value],
//!      - \b EUSCI_A_UART_IDLE_LINE_MULTI_PROCESSOR_MODE,
//!      - \b EUSCI_A_UART_ADDRESS_BIT_MULTI_PROCESSOR_MODE,
//!      - \b EUSCI_A_UART_AUTOMATIC_BAUDRATE_DETECTION_MODE
//! \param overSampling indicates low frequency or oversampling baud
//!      generation
//!     Valid values are
//!      - \b EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
//!      - \b EUSCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION
//!
//! Upon successful initialization of the UART block, this function
//! will have initialized the module, but the UART block still remains
//! disabled and must be enabled with UART_enableModule()
//!
//! Refer to
//! <a href="http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html">
//! this calculator </a> for help on calculating values for the parameters.
//!
//! Modified bits are \b UCPEN, \b UCPAR, \b UCMSB, \b UC7BIT, \b UCSPB,
//! \b UCMODEx, \b UCSYNC bits of \b UCAxCTL0 and \b UCSSELx,
//! \b UCSWRST bits of \b UCAxCTL1
//!
//! \return true or
//!         false of the initialization process
//
//*****************************************************************************
extern bool UART_initModule(uint32_t moduleInstance,
        const eUSCI_UART_Config *config);

//*****************************************************************************
//
//! Transmits a byte from the UART Module.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//! \param transmitData data to be transmitted from the UART module
//!
//! This function will place the supplied data into UART transmit data register
//! to start transmission
//!
//! Modified register is \b UCAxTXBUF
//! \return None.
//
//*****************************************************************************
extern void UART_transmitData(uint32_t moduleInstance,
        uint_fast8_t transmitData);

//*****************************************************************************
//
//! Receives a byte that has been sent to the UART Module.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as  EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//!
//! This function reads a byte of data from the UART receive data Register.
//!
//! Modified register is \b UCAxRXBUF
//!
//! \return Returns the byte received from by the UART module, cast as an
//! uint8_t.
//
//*****************************************************************************
extern uint8_t UART_receiveData(uint32_t moduleInstance);

//*****************************************************************************
//
//! Enables the UART block.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//!
//! This will enable operation of the UART block.
//!
//! Modified register is \b UCAxCTL1
//!
//! \return None.
//
//*****************************************************************************
extern void UART_enableModule(uint32_t moduleInstance);

//*****************************************************************************
//
//! Disables the UART block.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//!
//! This will disable operation of the UART block.
//!
//! Modified register is \b UCAxCTL1
//!
//! \return None.
//
//*****************************************************************************
extern void UART_disableModule(uint32_t moduleInstance);

//*****************************************************************************
//
//! Gets the current UART status flags.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//! \param mask is the masked interrupt flag status to be returned.
//!
//! This returns the status for the UART  module based on which
//! flag is passed. mask parameter can be either any of the following
//! selection.
//! - \b EUSCI_A_UART_LISTEN_ENABLE
//! - \b EUSCI_A_UART_FRAMING_ERROR
//! - \b EUSCI_A_UART_OVERRUN_ERROR
//! - \b EUSCI_A_UART_PARITY_ERROR
//! - \b eUARTBREAK_DETECT
//! - \b EUSCI_A_UART_RECEIVE_ERROR
//! - \b EUSCI_A_UART_ADDRESS_RECEIVED
//! - \b EUSCI_A_UART_IDLELINE
//! - \b EUSCI_A_UART_BUSY
//!
//! Modified register is \b UCAxSTAT
//!
//! \return the masked status flag
//
//*****************************************************************************
extern uint_fast8_t UART_queryStatusFlags(uint32_t moduleInstance,
        uint_fast8_t mask);

//*****************************************************************************
//
//! Sets the UART module in dormant mode
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//!
//! Puts USCI in sleep mode
//! Only characters that are preceded by an idle-line or with address bit set
//! UCRXIFG. In UART mode with automatic baud-rate detection, only the
//! combination of a break and synch field sets UCRXIFG.
//!
//! Modified register is \b UCAxCTL1
//!
//! \return None.
//
//*****************************************************************************
extern void UART_setDormant(uint32_t moduleInstance);

//*****************************************************************************
//
//! Re-enables UART module from dormant mode
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//!
//! Not dormant. All received characters set UCRXIFG.
//!
//! Modified bits are \b UCDORM of \b UCAxCTL1 register.
//!
//! \return None.
//
//*****************************************************************************
extern void UART_resetDormant(uint32_t moduleInstance);

//*****************************************************************************
//
//! Transmits the next byte to be transmitted marked as address depending on
//! selected multiprocessor mode
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//! \param transmitAddress is the next byte to be transmitted
//!
//! Modified register is \b UCAxCTL1, \b UCAxTXBUF
//!
//! \return None.
//
//*****************************************************************************
extern void UART_transmitAddress(uint32_t moduleInstance,
        uint_fast8_t transmitAddress);

//*****************************************************************************
//
//! Transmit break. Transmits a break with the next write to the transmit
//! buffer. In UART mode with automatic baud-rate detection,
//! EUSCI_A_UART_AUTOMATICBAUDRATE_SYNC(0x55) must be written into UCAxTXBUF to
//! generate the required break/synch fields.
//! Otherwise, DEFAULT_SYNC(0x00) must be written into the transmit buffer.
//! Also ensures module is ready for transmitting the next data
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  asEUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//!
//! Modified register is \b UCAxCTL1, \b UCAxTXBUF
//!
//! \return None.
//
//*****************************************************************************
extern void UART_transmitBreak(uint32_t moduleInstance);

//*****************************************************************************
//
//! Returns the address of the RX Buffer of the UART for the DMA module.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//!
//! Returns the address of the UART RX Buffer. This can be used in conjunction
//! with the DMA to store the received data directly to memory.
//!
//! \return None
//
//*****************************************************************************
extern uint32_t UART_getReceiveBufferAddressForDMA(uint32_t moduleInstance);

//*****************************************************************************
//
//! Returns the address of the TX Buffer of the UART for the DMA module.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//!
//! Returns the address of the UART TX Buffer. This can be used in conjunction
//! with the DMA to obtain transmitted data directly from memory.
//!
//! \return None
//
//*****************************************************************************
extern uint32_t UART_getTransmitBufferAddressForDMA(uint32_t moduleInstance);

//*****************************************************************************
//
//! Sets the deglitch time
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//! \param deglitchTime is the selected deglitch time
//!     Valid values are
//!         - \b EUSCI_A_UART_DEGLITCH_TIME_2ns
//!         - \b EUSCI_A_UART_DEGLITCH_TIME_50ns
//!         - \b EUSCI_A_UART_DEGLITCH_TIME_100ns
//!         - \b EUSCI_A_UART_DEGLITCH_TIME_200ns
//!
//!
//! Returns the address of the UART TX Buffer. This can be used in conjunction
//! with the DMA to obtain transmitted data directly from memory.
//!
//! \return None
//
//*****************************************************************************
extern void UART_selectDeglitchTime(uint32_t moduleInstance,
        uint32_t deglitchTime);

//*****************************************************************************
//
//! Enables individual UART interrupt sources.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//! \param mask is the bit mask of the interrupt sources to be enabled.
//!
//! Enables the indicated UART interrupt sources.  The interrupt flag is first
//! and then the corresponding interrupt is enabled. Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor.
//!
//! The mask parameter is the logical OR of any of the following:
//! - \b EUSCI_A_UART_RECEIVE_INTERRUPT -Receive interrupt
//! - \b EUSCI_A_UART_TRANSMIT_INTERRUPT - Transmit interrupt
//! - \b EUSCI_A_UART_RECEIVE_ERRONEOUSCHAR_INTERRUPT - Receive
//!                             erroneous-character interrupt enable
//! - \b EUSCI_A_UART_BREAKCHAR_INTERRUPT - Receive break character interrupt
//!                                           enable
//!
//! Modified register is \b UCAxIFG, \b UCAxIE and \b UCAxCTL1
//!
//! \return None.
//
//*****************************************************************************
extern void UART_enableInterrupt(uint32_t moduleInstance, uint_fast8_t mask);

//*****************************************************************************
//
//! Disables individual UART interrupt sources.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//! \param mask is the bit mask of the interrupt sources to be
//! disabled.
//!
//! Disables the indicated UART interrupt sources.  Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor.
//!
//! The mask parameter is the logical OR of any of the following:
//! - \b EUSCI_A_UART_RECEIVE_INTERRUPT -Receive interrupt
//! - \b EUSCI_A_UART_TRANSMIT_INTERRUPT - Transmit interrupt
//! - \b EUSCI_A_UART_RECEIVE_ERRONEOUSCHAR_INTERRUPT - Receive
//!                             erroneous-character interrupt enable
//! - \b EUSCI_A_UART_BREAKCHAR_INTERRUPT - Receive break character interrupt
//!                                             enable
//!
//! Modified register is \b UCAxIFG, \b UCAxIE and \b UCAxCTL1
//! \return None.
//
//*****************************************************************************
extern void UART_disableInterrupt(uint32_t moduleInstance, uint_fast8_t mask);

//*****************************************************************************
//
//! Gets the current UART interrupt status.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//! \param mask is the masked interrupt flag status to be returned.
//!        Mask value is the logical OR of any of the following:
//!        - \b EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG
//!        - \b EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG
//!        - \b EUSCI_A_UART_STARTBIT_INTERRUPT_FLAG
//!        - \b EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT_FLAG
//!
//!
//! \return The current interrupt status as an ORed bit mask:
//! - \b EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG -Receive interrupt flag
//! - \b EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG - Transmit interrupt flag
//
//*****************************************************************************
extern uint_fast8_t UART_getInterruptStatus(uint32_t moduleInstance,
        uint8_t mask);

//*****************************************************************************
//
//! Gets the current UART interrupt status masked with the enabled interrupts.
//! This function is useful to call in ISRs to get a list of pending
//! interrupts that are actually enabled and could have caused
//! the ISR.

//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//!
//! \return The current interrupt status as an ORed bit mask:
//! - \b EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG -Receive interrupt flag
//! - \b EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG - Transmit interrupt flag
//
//*****************************************************************************
extern uint_fast8_t UART_getEnabledInterruptStatus(uint32_t moduleInstance);

//*****************************************************************************
//
//! Clears UART interrupt sources.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode
//! \param mask is a bit mask of the interrupt sources to be cleared.
//!
//! The UART interrupt source is cleared, so that it no longer asserts.
//! The highest interrupt flag is automatically cleared when an interrupt vector
//! generator is used.
//!
//! The mask parameter has the same definition as the mask parameter to
//! EUSCI_A_UART_enableInterrupt().
//!
//! Modified register is \b UCAxIFG
//!
//! \return None.
//
//*****************************************************************************
extern void UART_clearInterruptFlag(uint32_t moduleInstance, uint_fast8_t mask);

//*****************************************************************************
//
//! Registers an interrupt handler for UART interrupts.
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode.
//!
//! \param intHandler is a pointer to the function to be called when the
//! timer capture compare interrupt occurs.
//!
//! This function registers the handler to be called when an UART
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific UART interrupts must be enabled
//! via UART_enableInterrupt().  It is the interrupt handler's responsibility to
//! clear the interrupt source via UART_clearInterruptFlag().
//!
//! \return None.
//
//*****************************************************************************
extern void UART_registerInterrupt(uint32_t moduleInstance,
        void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the UART module
//!
//! \param moduleInstance is the instance of the eUSCI A (UART) module.
//! Valid parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!  <br> It is important to note that for eUSCI modules, only "A" modules such
//!  as EUSCI_A0 can be used. "B" modules such as EUSCI_B0 do not support the
//!  UART mode.
//!
//! This function unregisters the handler to be called when timer
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void UART_unregisterInterrupt(uint32_t moduleInstance);

/* Backwards Compatibility Layer */
#define EUSCI_A_UART_transmitData UART_transmitData
#define EUSCI_A_UART_receiveData UART_receiveData
#define EUSCI_A_UART_enableInterrupt UART_enableInterrupt
#define EUSCI_A_UART_disableInterrupt UART_disableInterrupt
#define EUSCI_A_UART_getInterruptStatus UART_getInterruptStatus
#define EUSCI_A_UART_clearInterruptFlag UART_clearInterruptFlag
#define EUSCI_A_UART_enable UART_enableModule
#define EUSCI_A_UART_disable UART_disableModule
#define EUSCI_A_UART_queryStatusFlags UART_queryStatusFlags
#define EUSCI_A_UART_setDormant UART_setDormant
#define EUSCI_A_UART_resetDormant UART_resetDormant
#define EUSCI_A_UART_transmitAddress UART_transmitAddress
#define EUSCI_A_UART_transmitBreak UART_transmitBreak
#define EUSCI_A_UART_getReceiveBufferAddressForDMA UART_getReceiveBufferAddressForDMA
#define EUSCI_A_UART_getTransmitBufferAddressForDMA UART_getTransmitBufferAddressForDMA
#define EUSCI_A_UART_selectDeglitchTime UART_selectDeglitchTime

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

#endif /* UART_H_ */
