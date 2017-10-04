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
#ifndef SPI_H_
#define SPI_H_

//*****************************************************************************
//
//! \addtogroup spi_api
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

#include <stdbool.h>
#include <stdint.h>
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/eusci.h>

/* Configuration Defines */
#define EUSCI_SPI_CLOCKSOURCE_ACLK    EUSCI_B_CTLW0_SSEL__ACLK
#define EUSCI_SPI_CLOCKSOURCE_SMCLK   EUSCI_B_CTLW0_SSEL__SMCLK

#define EUSCI_SPI_MSB_FIRST    EUSCI_B_CTLW0_MSB
#define EUSCI_SPI_LSB_FIRST    0x00

#define EUSCI_SPI_BUSY        EUSCI_A_STATW_BUSY
#define EUSCI_SPI_NOT_BUSY    0x00

#define EUSCI_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT    0x00
#define EUSCI_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT    EUSCI_B_CTLW0_CKPH

#define EUSCI_SPI_3PIN                      EUSCI_B_CTLW0_MODE_0
#define EUSCI_SPI_4PIN_UCxSTE_ACTIVE_HIGH   EUSCI_B_CTLW0_MODE_1
#define EUSCI_SPI_4PIN_UCxSTE_ACTIVE_LOW    EUSCI_B_CTLW0_MODE_2

#define EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_HIGH    EUSCI_B_CTLW0_CKPL
#define EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_LOW     0x00

#define EUSCI_SPI_TRANSMIT_INTERRUPT    EUSCI_B_IE_TXIE_OFS
#define EUSCI_SPI_RECEIVE_INTERRUPT     EUSCI_B_IE_RXIE_OFS

#define EUSCI_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVE           EUSCI_B_CTLW0_STEM
#define EUSCI_SPI_PREVENT_CONFLICTS_WITH_OTHER_MASTERS    0x00

//*****************************************************************************
//
//!     ypedef eUSCI_SPI_MasterConfig
//! \brief Type definition for \link _eUSCI_SPI_MasterConfig \endlink structure
//!
//! \struct _eUSCI_SPI_MasterConfig
//! \brief Configuration structure for master mode in the \b SPI module. See
//!          \link SPI_initMaster \endlink for parameter documentation.
//
//*****************************************************************************
typedef struct _eUSCI_SPI_MasterConfig
{
    uint_fast8_t selectClockSource;
    uint32_t clockSourceFrequency;
    uint32_t desiredSpiClock;
    uint_fast16_t msbFirst;
    uint_fast16_t clockPhase;
    uint_fast16_t clockPolarity;
    uint_fast16_t spiMode;
} eUSCI_SPI_MasterConfig;

//*****************************************************************************
//
//!     ypedef eUSCI_SPI_SlaveConfig
//! \brief Type definition for \link _eUSCI_SPI_SlaveConfig \endlink structure
//!
//! \struct _eUSCI_SPI_SlaveConfig
//! \brief Configuration structure for slave mode in the \b SPI module. See
//!          \link SPI_initSlave \endlink for parameter documentation.
//
//*****************************************************************************
typedef struct _eUSCI_SPI_SlaveConfig
{
    uint_fast16_t msbFirst;
    uint_fast16_t clockPhase;
    uint_fast16_t clockPolarity;
    uint_fast16_t spiMode;
} eUSCI_SPI_SlaveConfig;

//*****************************************************************************
//
//! Initializes the SPI Master block.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//! \param config Configuration structure for SPI master mode
//!
//! <hr>
//! <b>Configuration options for \link eUSCI_SPI_MasterConfig \endlink structure.</b>
//! <hr>
//!
//! \param selectClockSource selects clock source. Valid values are
//!         - \b  EUSCI_SPI_CLOCKSOURCE_ACLK
//!         - \b  EUSCI_SPI_CLOCKSOURCE_SMCLK
//! \param clockSourceFrequency is the frequency of the selected clock source
//! \param desiredSpiClock is the desired clock rate for SPI communication
//! \param msbFirst controls the direction of the receive and transmit shift
//!      register. Valid values are
//!         - \b  EUSCI_SPI_MSB_FIRST
//!         - \b  EUSCI_SPI_LSB_FIRST [Default Value]
//! \param clockPhase is clock phase select.
//!         Valid values are
//!         - \b  EUSCI_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
//!                                                          [Default Value]
//!         - \b  EUSCI_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
//! \param clockPolarity is clock polarity select.
//!         Valid values are
//!         - \b  EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
//!         - \b  EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_LOW  [Default Value]
//! \param spiMode is SPI mode select.
//!         Valid values are
//!         - \b  EUSCI_SPI_3PIN [Default Value]
//!         - \b  EUSCI_SPI_4PIN_UCxSTE_ACTIVE_HIGH
//!         - \b  EUSCI_SPI_4PIN_UCxSTE_ACTIVE_LOW
//! Upon successful initialization of the SPI master block, this function
//! will have set the bus speed for the master, but the SPI Master block
//! still remains disabled and must be enabled with SPI_enableModule()
//!
//! Modified bits are \b UCCKPH, \b UCCKPL, \b UC7BIT, \b UCMSB,\b UCSSELx,
//! \b UCSWRST bits of \b UCAxCTLW0 register
//!
//! \return true
//
//*****************************************************************************
extern bool SPI_initMaster(uint32_t moduleInstance,
        const eUSCI_SPI_MasterConfig *config);

//*****************************************************************************
//
//! Selects 4Pin Functionality
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//! \param select4PinFunctionality selects Clock source. Valid values are
//!         - \b EUSCI_SPI_PREVENT_CONFLICTS_WITH_OTHER_MASTERS
//!         - \b EUSCI_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVE
//! This function should be invoked only in 4-wire mode. Invoking this function
//! has no effect in 3-wire mode.
//!
//! Modified bits are \b UCSTEM bit of \b UCAxCTLW0 register
//!
//! \return true
//
//*****************************************************************************
extern void SPI_selectFourPinFunctionality(uint32_t moduleInstance,
        uint_fast8_t select4PinFunctionality);

//*****************************************************************************
//
//! Initializes the SPI Master clock.At the end of this function call, SPI
//! module is left enabled.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//! \param clockSourceFrequency is the frequency of the selected clock source
//! \param desiredSpiClock is the desired clock rate for SPI communication.
//!
//! Modified bits are \b UCSWRST bit of \b UCAxCTLW0 register and
//! \b UCAxBRW register
//!
//! \return None
//
//*****************************************************************************
extern void SPI_changeMasterClock(uint32_t moduleInstance,
        uint32_t clockSourceFrequency, uint32_t desiredSpiClock);

//*****************************************************************************
//
//! Initializes the SPI Slave block.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//! \param config Configuration structure for SPI slave mode
//!
//! <hr>
//! <b>Configuration options for \link eUSCI_SPI_SlaveConfig \endlink structure.</b>
//! <hr>
//!
//! \param msbFirst controls the direction of the receive and transmit shift
//!      register. Valid values are
//!         - \b  EUSCI_SPI_MSB_FIRST
//!         - \b  EUSCI_SPI_LSB_FIRST [Default Value]
//! \param clockPhase is clock phase select.
//!         Valid values are
//!         - \b  EUSCI_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
//!                                                          [Default Value]
//!         - \b  EUSCI_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
//! \param clockPolarity is clock polarity select.
//!         Valid values are
//!         - \b  EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
//!         - \b  EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_LOW [Default Value]
//! \param spiMode is SPI mode select.
//!         Valid values are
//!         - \b  EUSCI_SPI_3PIN [Default Value]
//!         - \b  EUSCI_SPI_4PIN_UCxSTE_ACTIVE_HIGH
//!         - \b  EUSCI_SPI_4PIN_UCxSTE_ACTIVE_LOW
//! Upon successful initialization of the SPI slave block, this function
//! will have initialized the slave block, but the SPI Slave block
//! still remains disabled and must be enabled with SPI_enableModule()
//!
//! Modified bits are \b UCMSB, \b UC7BIT, \b UCMST, \b UCCKPL, \b UCCKPH,
//! \b UCMODE, \b UCSWRST bits of \b UCAxCTLW0
//!
//! \return true
//*****************************************************************************
extern bool SPI_initSlave(uint32_t moduleInstance,
        const eUSCI_SPI_SlaveConfig *config);

//*****************************************************************************
//
//! Changes the SPI clock phase and polarity.At the end of this function call,
//! SPI module is left enabled.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//! \param clockPhase is clock phase select.
//!         Valid values are:
//!             - \b EUSCI_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
//!                                                          [Default Value]
//!             - \b EUSCI_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
//! \param clockPolarity is clock polarity select.
//!         Valid values are:
//!             - \b EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
//!             - \b EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_LOW  [Default Value]
//!
//! Modified bits are \b UCSWRST, \b UCCKPH, \b UCCKPL, \b UCSWRST bits of
//! \b UCAxCTLW0
//!
//! \return None
//
//*****************************************************************************
extern void SPI_changeClockPhasePolarity(uint32_t moduleInstance,
        uint_fast16_t clockPhase, uint_fast16_t clockPolarity);

//*****************************************************************************
//
//! Transmits a byte from the SPI Module.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//! \param transmitData data to be transmitted from the SPI module
//!
//! This function will place the supplied data into SPI transmit data register
//! to start transmission
//!
//! Modified register is \b UCAxTXBUF
//!
//! \return None.
//
//*****************************************************************************
extern void SPI_transmitData(uint32_t moduleInstance,
        uint_fast8_t transmitData);

//*****************************************************************************
//
//! Receives a byte that has been sent to the SPI Module.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//!
//! This function reads a byte of data from the SPI receive data Register.
//!
//! \return Returns the byte received from by the SPI module, cast as an
//! uint8_t.
//
//*****************************************************************************
extern uint8_t SPI_receiveData(uint32_t moduleInstance);

//*****************************************************************************
//
//! Enables the SPI block.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//!
//! This will enable operation of the SPI block.
//! Modified bits are \b UCSWRST bit of \b UCAxCTLW0 register.
//!
//! \return None.
//
//*****************************************************************************
extern void SPI_enableModule(uint32_t moduleInstance);

//*****************************************************************************
//
//! Disables the SPI block.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//!
//! This will disable operation of the SPI block.
//!
//! Modified bits are \b UCSWRST bit of \b UCAxCTLW0 register.
//!
//! \return None.
//
//*****************************************************************************
extern void SPI_disableModule(uint32_t moduleInstance);

//*****************************************************************************
//
//! Returns the address of the RX Buffer of the SPI for the DMA module.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//!
//! Returns the address of the SPI RX Buffer. This can be used in conjunction
//! with the DMA to store the received data directly to memory.
//!
//! \return NONE
//
//*****************************************************************************
extern uint32_t SPI_getReceiveBufferAddressForDMA(uint32_t moduleInstance);

//*****************************************************************************
//
//! Returns the address of the TX Buffer of the SPI for the DMA module.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//!
//! Returns the address of the SPI TX Buffer. This can be used in conjunction
//! with the DMA to obtain transmitted data directly from memory.
//!
//! \return NONE
//
//*****************************************************************************
extern uint32_t SPI_getTransmitBufferAddressForDMA(uint32_t moduleInstance);

//*****************************************************************************
//
//! Indicates whether or not the SPI bus is busy.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//!
//! This function returns an indication of whether or not the SPI bus is
//! busy.This function checks the status of the bus via UCBBUSY bit
//!
//! \return EUSCI_SPI_BUSY if the SPI module transmitting or receiving
//! is busy; otherwise, returns EUSCI_SPI_NOT_BUSY.
//
//*****************************************************************************
extern uint_fast8_t SPI_isBusy(uint32_t moduleInstance);

//*****************************************************************************
//
//! Enables individual SPI interrupt sources.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//! \param mask is the bit mask of the interrupt sources to be enabled.
//!
//! Enables the indicated SPI interrupt sources.  Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor.
//!
//! The mask parameter is the logical OR of any of the following:
//!       - \b EUSCI_SPI_RECEIVE_INTERRUPT Receive interrupt
//!       - \b EUSCI_SPI_TRANSMIT_INTERRUPT Transmit interrupt
//!
//! Modified registers are \b UCAxIFG and \b UCAxIE
//!
//! \return None.
//
//*****************************************************************************
extern void SPI_enableInterrupt(uint32_t moduleInstance, uint_fast8_t mask);

//*****************************************************************************
//
//! Disables individual SPI interrupt sources.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//! \param mask is the bit mask of the interrupt sources to be
//! disabled.
//!
//! Disables the indicated SPI interrupt sources.  Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor.
//!
//! The mask parameter is the logical OR of any of the following:
//!       - \b EUSCI_SPI_RECEIVE_INTERRUPT Receive interrupt
//!       - \b EUSCI_SPI_TRANSMIT_INTERRUPT Transmit interrupt
//!
//! Modified register is \b UCAxIE
//!
//! \return None.
//
//*****************************************************************************
extern void SPI_disableInterrupt(uint32_t moduleInstance, uint_fast8_t mask);

//*****************************************************************************
//
//! Gets the current SPI interrupt status.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//! \param mask Mask of interrupt to filter. This can include:
//!          - \b EUSCI_SPI_RECEIVE_INTERRUPT -Receive interrupt
//!          - \b EUSCI_SPI_TRANSMIT_INTERRUPT - Transmit interrupt
//!
//! Modified registers are \b UCAxIFG.
//!
//! \return The current interrupt status as the mask of the set flags
//! Mask parameter can be either any of the following selection:
//! - \b EUSCI_SPI_RECEIVE_INTERRUPT -Receive interrupt
//! - \b EUSCI_SPI_TRANSMIT_INTERRUPT - Transmit interrupt
//
//*****************************************************************************
extern uint_fast8_t SPI_getInterruptStatus(uint32_t moduleInstance,
        uint16_t mask);

//*****************************************************************************
//
//! Gets the current SPI interrupt status masked with the enabled interrupts.
//! This function is useful to call in ISRs to get a list of pending
//! interrupts that are actually enabled and could have caused
//! the ISR.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//! Modified registers are \b UCAxIFG.
//!
//! \return The current interrupt status as the mask of the set flags
//! Mask parameter can be either any of the following selection:
//! - \b EUSCI_SPI_RECEIVE_INTERRUPT -Receive interrupt
//! - \b EUSCI_SPI_TRANSMIT_INTERRUPT - Transmit interrupt
//
//*****************************************************************************
extern uint_fast8_t SPI_getEnabledInterruptStatus(uint32_t moduleInstance);

//*****************************************************************************
//
//! Clears the selected SPI interrupt status flag.
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!
//! \param mask is the masked interrupt flag to be cleared.
//!
//! The mask parameter is the logical OR of any of the following:
//! - \b EUSCI_SPI_RECEIVE_INTERRUPT -Receive interrupt
//! - \b EUSCI_SPI_TRANSMIT_INTERRUPT - Transmit interrupt
//! Modified registers are \b UCAxIFG.
//!
//! \return None
//
//*****************************************************************************
extern void SPI_clearInterruptFlag(uint32_t moduleInstance, uint_fast8_t mask);

//*****************************************************************************
//
//! Registers an interrupt handler for the timer capture compare interrupt.
//!
//! \param moduleInstance is the instance of the eUSCI (SPI) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param intHandler is a pointer to the function to be called when the
//! timer capture compare interrupt occurs.
//!
//! This function registers the handler to be called when a timer
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific SPI interrupts must be enabled
//! via SPI_enableInterrupt().  It is the interrupt handler's responsibility to
//! clear the interrupt source via SPI_clearInterruptFlag().
//!
//! \return None.
//
//*****************************************************************************
extern void SPI_registerInterrupt(uint32_t moduleInstance,
        void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the timer
//!
//! \param moduleInstance is the instance of the eUSCI A/B module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_A0_BASE
//!         - \b EUSCI_A1_BASE
//!         - \b EUSCI_A2_BASE
//!         - \b EUSCI_A3_BASE
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
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
extern void SPI_unregisterInterrupt(uint32_t moduleInstance);

/* Backwards Compatibility Layer */
#define EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT            0x00
#define EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT EUSCI_B_CTLW0_CKPH

#define EUSCI_B_SPI_MSB_FIRST                                  EUSCI_B_CTLW0_MSB
#define EUSCI_B_SPI_LSB_FIRST                                              0x00

#define EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH            EUSCI_B_CTLW0_CKPL
#define EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW                           0x00

#define EUSCI_B_SPI_CLOCKSOURCE_ACLK                    EUSCI_B_CTLW0_SSEL__ACLK
#define EUSCI_B_SPI_CLOCKSOURCE_SMCLK                  EUSCI_B_CTLW0_SSEL__SMCLK

#define EUSCI_B_SPI_3PIN                                    EUSCI_B_CTLW0_MODE_0
#define EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_HIGH                 EUSCI_B_CTLW0_MODE_1
#define EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_LOW                  EUSCI_B_CTLW0_MODE_2

#define EUSCI_B_SPI_PREVENT_CONFLICTS_WITH_OTHER_MASTERS                   0x00
#define EUSCI_B_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVE            EUSCI_B_CTLW0_STEM

#define EUSCI_B_SPI_TRANSMIT_INTERRUPT                          EUSCI_B_IE_TXIE0
#define EUSCI_B_SPI_RECEIVE_INTERRUPT                           EUSCI_B_IE_RXIE0

#define EUSCI_B_SPI_BUSY                                     EUSCI_B_STATW_BBUSY
#define EUSCI_B_SPI_NOT_BUSY                                               0x00

#define EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT            0x00
#define EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT EUSCI_A_CTLW0_CKPH

#define EUSCI_A_SPI_MSB_FIRST                                 EUSCI_A_CTLW0_MSB
#define EUSCI_A_SPI_LSB_FIRST                                              0x00

#define EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_HIGH            EUSCI_A_CTLW0_CKPL
#define EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW                           0x00

#define EUSCI_A_SPI_CLOCKSOURCE_ACLK                   EUSCI_A_CTLW0_SSEL__ACLK
#define EUSCI_A_SPI_CLOCKSOURCE_SMCLK                 EUSCI_A_CTLW0_SSEL__SMCLK

#define EUSCI_A_SPI_3PIN                                   EUSCI_A_CTLW0_MODE_0
#define EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_HIGH                EUSCI_A_CTLW0_MODE_1
#define EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_LOW                 EUSCI_A_CTLW0_MODE_2

#define EUSCI_A_SPI_PREVENT_CONFLICTS_WITH_OTHER_MASTERS                   0x00
#define EUSCI_A_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVE            EUSCI_A_CTLW0_STEM

#define EUSCI_A_SPI_TRANSMIT_INTERRUPT                          EUSCI_A_IE_TXIE
#define EUSCI_A_SPI_RECEIVE_INTERRUPT                           EUSCI_A_IE_RXIE

#define EUSCI_A_SPI_BUSY                                    EUSCI_B_STATW_BBUSY
#define EUSCI_A_SPI_NOT_BUSY                                               0x00

extern void EUSCI_A_SPI_select4PinFunctionality(uint32_t baseAddress,
        uint8_t select4PinFunctionality);
extern void EUSCI_A_SPI_masterChangeClock(uint32_t baseAddress,
        uint32_t clockSourceFrequency, uint32_t desiredSpiClock);
extern bool EUSCI_A_SPI_slaveInit(uint32_t baseAddress, uint16_t msbFirst,
        uint16_t clockPhase, uint16_t clockPolarity, uint16_t spiMode);
extern void EUSCI_A_SPI_changeClockPhasePolarity(uint32_t baseAddress,
        uint16_t clockPhase, uint16_t clockPolarity);
extern void EUSCI_A_SPI_transmitData(uint32_t baseAddress,
        uint8_t transmitData);
extern uint8_t EUSCI_A_SPI_receiveData(uint32_t baseAddress);
extern void EUSCI_A_SPI_enableInterrupt(uint32_t baseAddress, uint8_t mask);
extern void EUSCI_A_SPI_disableInterrupt(uint32_t baseAddress, uint8_t mask);
extern uint8_t EUSCI_A_SPI_getInterruptStatus(uint32_t baseAddress,
        uint8_t mask);
extern void EUSCI_A_SPI_clearInterruptFlag(uint32_t baseAddress, uint8_t mask);
extern void EUSCI_A_SPI_enable(uint32_t baseAddress);
extern void EUSCI_A_SPI_disable(uint32_t baseAddress);
extern uint32_t EUSCI_A_SPI_getReceiveBufferAddressForDMA(uint32_t baseAddress);
extern uint32_t EUSCI_A_SPI_getTransmitBufferAddressForDMA(
        uint32_t baseAddress);
extern bool EUSCI_A_SPI_isBusy(uint32_t baseAddress);
extern void EUSCI_B_SPI_select4PinFunctionality(uint32_t baseAddress,
        uint8_t select4PinFunctionality);
extern void EUSCI_B_SPI_masterChangeClock(uint32_t baseAddress,
        uint32_t clockSourceFrequency, uint32_t desiredSpiClock);
extern bool EUSCI_B_SPI_slaveInit(uint32_t baseAddress, uint16_t msbFirst,
        uint16_t clockPhase, uint16_t clockPolarity, uint16_t spiMode);
extern void EUSCI_B_SPI_changeClockPhasePolarity(uint32_t baseAddress,
        uint16_t clockPhase, uint16_t clockPolarity);
extern void EUSCI_B_SPI_transmitData(uint32_t baseAddress,
        uint8_t transmitData);
extern uint8_t EUSCI_B_SPI_receiveData(uint32_t baseAddress);
extern void EUSCI_B_SPI_enableInterrupt(uint32_t baseAddress, uint8_t mask);
extern void EUSCI_B_SPI_disableInterrupt(uint32_t baseAddress, uint8_t mask);
extern uint8_t EUSCI_B_SPI_getInterruptStatus(uint32_t baseAddress,
        uint8_t mask);
extern void EUSCI_B_SPI_clearInterruptFlag(uint32_t baseAddress, uint8_t mask);
extern void EUSCI_B_SPI_enable(uint32_t baseAddress);
extern void EUSCI_B_SPI_disable(uint32_t baseAddress);
extern uint32_t EUSCI_B_SPI_getReceiveBufferAddressForDMA(uint32_t baseAddress);
extern uint32_t EUSCI_B_SPI_getTransmitBufferAddressForDMA(
        uint32_t baseAddress);
extern bool EUSCI_B_SPI_isBusy(uint32_t baseAddress);

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

#endif /* SPI_H_ */

