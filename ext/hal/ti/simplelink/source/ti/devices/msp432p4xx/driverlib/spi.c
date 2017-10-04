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
#include <ti/devices/msp432p4xx/driverlib/spi.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>
#include <ti/devices/msp432p4xx/driverlib/eusci.h>

static bool is_A_Module(uint32_t module)
{
    if (module == EUSCI_A0_BASE || module == EUSCI_A1_BASE
#ifdef EUSCI_A2_BASE
            || module == EUSCI_A2_BASE
#endif
#ifdef EUSCI_A3_BASE
            || module == EUSCI_A3_BASE
#endif
    )
        return true;
    else
        return false;
}

bool SPI_initMaster(uint32_t moduleInstance, const eUSCI_SPI_MasterConfig *config)
{
    if (is_A_Module(moduleInstance))
    {
        ASSERT(
                (EUSCI_A_SPI_CLOCKSOURCE_ACLK == config->selectClockSource)
                || (EUSCI_A_SPI_CLOCKSOURCE_SMCLK
                        == config->selectClockSource));

        ASSERT(
                (EUSCI_A_SPI_MSB_FIRST == config->msbFirst)
                || (EUSCI_A_SPI_LSB_FIRST == config->msbFirst));

        ASSERT(
                (EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
                        == config->clockPhase)
                || (EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
                        == config->clockPhase));

        ASSERT(
                (EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
                        == config->clockPolarity)
                || (EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW
                        == config->clockPolarity));

        ASSERT(
                (EUSCI_A_SPI_3PIN == config->spiMode)
                || (EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_HIGH
                        == config->spiMode)
                || (EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_LOW
                        == config->spiMode));

        //Disable the USCI Module
        BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;

        /*
         * Configure as SPI master mode.
         * Clock phase select, polarity, msb
         * EUSCI_A_CTLW0_MST = Master mode
         * EUSCI_A_CTLW0_SYNC = Synchronous mode
         * UCMODE_0 = 3-pin SPI
         */
        EUSCI_A_CMSIS(moduleInstance)->CTLW0 =
                (EUSCI_A_CMSIS(moduleInstance)->CTLW0
                        & ~(EUSCI_A_CTLW0_SSEL_MASK + EUSCI_A_CTLW0_CKPH + EUSCI_A_CTLW0_CKPL + EUSCI_A_CTLW0_SEVENBIT + EUSCI_A_CTLW0_MSB + EUSCI_A_CTLW0_MST
                                + EUSCI_A_CTLW0_MODE_3 + EUSCI_A_CTLW0_SYNC))
                        | (config->selectClockSource + config->msbFirst
                                + config->clockPhase + config->clockPolarity
                                + EUSCI_A_CTLW0_MST + EUSCI_A_CTLW0_SYNC + config->spiMode);

        EUSCI_A_CMSIS(moduleInstance)->BRW =
                (uint16_t) (config->clockSourceFrequency
                        / config->desiredSpiClock);

        //No modulation
        EUSCI_A_CMSIS(moduleInstance)->MCTLW = 0;

        return true;
    } else
    {
        ASSERT(
                (EUSCI_B_SPI_CLOCKSOURCE_ACLK == config->selectClockSource)
                || (EUSCI_B_SPI_CLOCKSOURCE_SMCLK
                        == config->selectClockSource));

        ASSERT(
                (EUSCI_B_SPI_MSB_FIRST == config->msbFirst)
                || (EUSCI_B_SPI_LSB_FIRST == config->msbFirst));

        ASSERT(
                (EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
                        == config->clockPhase)
                || (EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
                        == config->clockPhase));

        ASSERT(
                (EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
                        == config->clockPolarity)
                || (EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW
                        == config->clockPolarity));

        ASSERT(
                (EUSCI_B_SPI_3PIN == config->spiMode)
                || (EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_HIGH
                        == config->spiMode)
                || (EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_LOW
                        == config->spiMode));

        //Disable the USCI Module
        BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;

        /*
         * Configure as SPI master mode.
         * Clock phase select, polarity, msb
         * EUSCI_A_CTLW0_MST = Master mode
         * EUSCI_A_CTLW0_SYNC = Synchronous mode
         * UCMODE_0 = 3-pin SPI
         */
        EUSCI_B_CMSIS(moduleInstance)->CTLW0 =
                (EUSCI_B_CMSIS(moduleInstance)->CTLW0
                        & ~(EUSCI_A_CTLW0_SSEL_MASK + EUSCI_A_CTLW0_CKPH + EUSCI_A_CTLW0_CKPL + EUSCI_A_CTLW0_SEVENBIT + EUSCI_A_CTLW0_MSB + EUSCI_A_CTLW0_MST
                                + EUSCI_A_CTLW0_MODE_3 + EUSCI_A_CTLW0_SYNC))
                        | (config->selectClockSource + config->msbFirst
                                + config->clockPhase + config->clockPolarity
                                + EUSCI_A_CTLW0_MST + EUSCI_A_CTLW0_SYNC + config->spiMode);

        EUSCI_B_CMSIS(moduleInstance)->BRW =
                (uint16_t) (config->clockSourceFrequency
                        / config->desiredSpiClock);

        return true;
    }

}

void SPI_selectFourPinFunctionality(uint32_t moduleInstance,
        uint_fast8_t select4PinFunctionality)
{
    if (is_A_Module(moduleInstance))
    {
        EUSCI_A_SPI_select4PinFunctionality(moduleInstance,
                select4PinFunctionality);
    } else
    {
        EUSCI_B_SPI_select4PinFunctionality(moduleInstance,
                select4PinFunctionality);
    }

}

void SPI_changeMasterClock(uint32_t moduleInstance,
        uint32_t clockSourceFrequency, uint32_t desiredSpiClock)
{
    if (is_A_Module(moduleInstance))
    {
        EUSCI_A_SPI_masterChangeClock(moduleInstance, clockSourceFrequency,
                desiredSpiClock);
    } else
    {
        EUSCI_B_SPI_masterChangeClock(moduleInstance, clockSourceFrequency,
                desiredSpiClock);
    }

}

bool SPI_initSlave(uint32_t moduleInstance, const eUSCI_SPI_SlaveConfig *config)
{
    if (is_A_Module(moduleInstance))
    {
        ASSERT(
                (EUSCI_A_SPI_MSB_FIRST == config->msbFirst)
                || (EUSCI_A_SPI_LSB_FIRST == config->msbFirst));

        ASSERT(
                (EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
                        == config->clockPhase)
                || (EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
                        == config->clockPhase));

        ASSERT(
                (EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
                        == config->clockPolarity)
                || (EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW
                        == config->clockPolarity));

        ASSERT(
                (EUSCI_A_SPI_3PIN == config->spiMode)
                || (EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_HIGH
                        == config->spiMode)
                || (EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_LOW
                        == config->spiMode));

        //Disable USCI Module
        BITBAND_PERI(EUSCI_A_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;

        //Reset OFS_UCAxCTLW0 register
        EUSCI_A_CMSIS(moduleInstance)->CTLW0 =
                (EUSCI_A_CMSIS(moduleInstance)->CTLW0
                        & ~(EUSCI_A_CTLW0_MSB + EUSCI_A_CTLW0_SEVENBIT + EUSCI_A_CTLW0_MST + EUSCI_A_CTLW0_CKPL + EUSCI_A_CTLW0_CKPH + EUSCI_A_CTLW0_MODE_3))
                        | (config->clockPhase + config->clockPolarity
                                + config->msbFirst + EUSCI_A_CTLW0_SYNC + config->spiMode);

        return true;
    } else
    {
        ASSERT(
                (EUSCI_B_SPI_MSB_FIRST == config->msbFirst)
                || (EUSCI_B_SPI_LSB_FIRST == config->msbFirst));

        ASSERT(
                (EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
                        == config->clockPhase)
                || (EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
                        == config->clockPhase));

        ASSERT(
                (EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
                        == config->clockPolarity)
                || (EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW
                        == config->clockPolarity));

        ASSERT(
                (EUSCI_B_SPI_3PIN == config->spiMode)
                || (EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_HIGH
                        == config->spiMode)
                || (EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_LOW
                        == config->spiMode));

        //Disable USCI Module
        BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;

        //Reset OFS_UCBxCTLW0 register
        EUSCI_B_CMSIS(moduleInstance)->CTLW0 =
                (EUSCI_B_CMSIS(moduleInstance)->CTLW0
                        & ~(EUSCI_A_CTLW0_MSB + EUSCI_A_CTLW0_SEVENBIT + EUSCI_A_CTLW0_MST + EUSCI_A_CTLW0_CKPL + EUSCI_A_CTLW0_CKPH + EUSCI_A_CTLW0_MODE_3))
                        | (config->clockPhase + config->clockPolarity
                                + config->msbFirst + EUSCI_A_CTLW0_SYNC + config->spiMode);

        return true;
    }

}

void SPI_changeClockPhasePolarity(uint32_t moduleInstance,
        uint_fast16_t clockPhase, uint_fast16_t clockPolarity)
{
    if (is_A_Module(moduleInstance))
    {
        EUSCI_A_SPI_changeClockPhasePolarity(moduleInstance, clockPhase,
                clockPolarity);
    } else
    {
        EUSCI_B_SPI_changeClockPhasePolarity(moduleInstance, clockPhase,
                clockPolarity);
    }

}

void SPI_transmitData(uint32_t moduleInstance, uint_fast8_t transmitData)
{
    if (is_A_Module(moduleInstance))
    {
        EUSCI_A_SPI_transmitData(moduleInstance, transmitData);
    } else
    {
        EUSCI_B_SPI_transmitData(moduleInstance, transmitData);
    }

}

uint8_t SPI_receiveData(uint32_t moduleInstance)
{
    if (is_A_Module(moduleInstance))
    {
        return EUSCI_A_SPI_receiveData(moduleInstance);
    } else
    {
        return EUSCI_B_SPI_receiveData(moduleInstance);
    }

}

void SPI_enableModule(uint32_t moduleInstance)
{
    if (is_A_Module(moduleInstance))
    {
        EUSCI_A_SPI_enable(moduleInstance);
    } else
    {
        EUSCI_B_SPI_enable(moduleInstance);
    }

}

void SPI_disableModule(uint32_t moduleInstance)
{
    if (is_A_Module(moduleInstance))
    {
        EUSCI_A_SPI_disable(moduleInstance);
    } else
    {
        EUSCI_B_SPI_disable(moduleInstance);
    }

}

uint32_t SPI_getReceiveBufferAddressForDMA(uint32_t moduleInstance)
{
    if (is_A_Module(moduleInstance))
    {
        return EUSCI_A_SPI_getReceiveBufferAddressForDMA(moduleInstance);
    } else
    {
        return EUSCI_B_SPI_getReceiveBufferAddressForDMA(moduleInstance);
    }

}

uint32_t SPI_getTransmitBufferAddressForDMA(uint32_t moduleInstance)
{
    if (is_A_Module(moduleInstance))
    {
        return EUSCI_A_SPI_getTransmitBufferAddressForDMA(moduleInstance);
    } else
    {
        return EUSCI_B_SPI_getTransmitBufferAddressForDMA(moduleInstance);
    }

}

uint_fast8_t SPI_isBusy(uint32_t moduleInstance)
{
    if (is_A_Module(moduleInstance))
    {
        return EUSCI_A_SPI_isBusy(moduleInstance);
    } else
    {
        return EUSCI_B_SPI_isBusy(moduleInstance);
    }

}

void SPI_enableInterrupt(uint32_t moduleInstance, uint_fast8_t mask)
{
    if (is_A_Module(moduleInstance))
    {
        EUSCI_A_SPI_enableInterrupt(moduleInstance, mask);
    } else
    {
        EUSCI_B_SPI_enableInterrupt(moduleInstance, mask);
    }

}

void SPI_disableInterrupt(uint32_t moduleInstance, uint_fast8_t mask)
{
    if (is_A_Module(moduleInstance))
    {
        EUSCI_A_SPI_disableInterrupt(moduleInstance, mask);
    } else
    {
        EUSCI_B_SPI_disableInterrupt(moduleInstance, mask);
    }

}

uint_fast8_t SPI_getInterruptStatus(uint32_t moduleInstance, uint16_t mask)
{
    if (is_A_Module(moduleInstance))
    {
        return EUSCI_A_SPI_getInterruptStatus(moduleInstance, mask);
    } else
    {
        return EUSCI_B_SPI_getInterruptStatus(moduleInstance, mask);
    }

}

uint_fast8_t SPI_getEnabledInterruptStatus(uint32_t moduleInstance)
{
    if (is_A_Module(moduleInstance))
    {
        return SPI_getInterruptStatus(moduleInstance,
                EUSCI_SPI_TRANSMIT_INTERRUPT | EUSCI_SPI_RECEIVE_INTERRUPT)
                & EUSCI_A_CMSIS(moduleInstance)->IE;

    } else
    {
        return SPI_getInterruptStatus(moduleInstance,
                EUSCI_SPI_TRANSMIT_INTERRUPT | EUSCI_SPI_RECEIVE_INTERRUPT)
                & EUSCI_B_CMSIS(moduleInstance)->IE;

    }
}

void SPI_clearInterruptFlag(uint32_t moduleInstance, uint_fast8_t mask)
{
    if (is_A_Module(moduleInstance))
    {
        EUSCI_A_SPI_clearInterruptFlag(moduleInstance, mask);
    } else
    {
        EUSCI_B_SPI_clearInterruptFlag(moduleInstance, mask);
    }

}

void SPI_registerInterrupt(uint32_t moduleInstance, void (*intHandler)(void))
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
    case EUSCI_B0_BASE:
        Interrupt_registerInterrupt(INT_EUSCIB0, intHandler);
        Interrupt_enableInterrupt(INT_EUSCIB0);
        break;
    case EUSCI_B1_BASE:
        Interrupt_registerInterrupt(INT_EUSCIB1, intHandler);
        Interrupt_enableInterrupt(INT_EUSCIB1);
        break;
#ifdef EUSCI_B2_BASE
    case EUSCI_B2_BASE:
        Interrupt_registerInterrupt(INT_EUSCIB2, intHandler);
        Interrupt_enableInterrupt(INT_EUSCIB2);
        break;
#endif
#ifdef EUSCI_B3_BASE
    case EUSCI_B3_BASE:
        Interrupt_registerInterrupt(INT_EUSCIB3, intHandler);
        Interrupt_enableInterrupt(INT_EUSCIB3);
        break;
#endif
    default:
        ASSERT(false);
    }
}

void SPI_unregisterInterrupt(uint32_t moduleInstance)
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
    case EUSCI_B0_BASE:
        Interrupt_disableInterrupt(INT_EUSCIB0);
        Interrupt_unregisterInterrupt(INT_EUSCIB0);
        break;
    case EUSCI_B1_BASE:
        Interrupt_disableInterrupt(INT_EUSCIB1);
        Interrupt_unregisterInterrupt(INT_EUSCIB1);
        break;
#ifdef EUSCI_B2_BASE
    case EUSCI_B2_BASE:
        Interrupt_disableInterrupt(INT_EUSCIB2);
        Interrupt_unregisterInterrupt(INT_EUSCIB2);
        break;
#endif
#ifdef EUSCI_B3_BASE
    case EUSCI_B3_BASE:
        Interrupt_disableInterrupt(INT_EUSCIB3);
        Interrupt_unregisterInterrupt(INT_EUSCIB3);
        break;
#endif
    default:
        ASSERT(false);
    }

}

/* Backwards Compatibility Layer */

//*****************************************************************************
//
//! \brief Selects 4Pin Functionality
//!
//! This function should be invoked only in 4-wire mode. Invoking this function
//! has no effect in 3-wire mode.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//! \param select4PinFunctionality selects 4 pin functionality
//!        Valid values are:
//!        - \b EUSCI_B_SPI_PREVENT_CONFLICTS_WITH_OTHER_MASTERS
//!        - \b EUSCI_B_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVE
//!
//! Modified bits are \b UCSTEM of \b UCAxCTLW0 register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_B_SPI_select4PinFunctionality(uint32_t baseAddress,
        uint8_t select4PinFunctionality)
{
    ASSERT(
            (EUSCI_B_SPI_PREVENT_CONFLICTS_WITH_OTHER_MASTERS
                    == select4PinFunctionality)
            || (EUSCI_B_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVE
                    == select4PinFunctionality));

    EUSCI_B_CMSIS(baseAddress)->CTLW0 = (EUSCI_B_CMSIS(baseAddress)->CTLW0
            & ~EUSCI_B_CTLW0_STEM) | select4PinFunctionality;
}

//*****************************************************************************
//
//! \brief Initializes the SPI Master clock. At the end of this function call,
//! SPI module is left enabled.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//! \param clockSourceFrequency is the frequency of the slected clock source
//! \param desiredSpiClock is the desired clock rate for SPI communication
//!
//! Modified bits are \b UCSWRST of \b UCAxCTLW0 register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_B_SPI_masterChangeClock(uint32_t baseAddress,
        uint32_t clockSourceFrequency, uint32_t desiredSpiClock)
{
    //Disable the USCI Module
    BITBAND_PERI(EUSCI_B_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;

    EUSCI_B_CMSIS(baseAddress)->BRW = (uint16_t) (clockSourceFrequency
            / desiredSpiClock);

    //Reset the UCSWRST bit to enable the USCI Module
    BITBAND_PERI(EUSCI_B_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 0;
}

//*****************************************************************************
//
//! \brief Initializes the SPI Slave block.
//!
//! Upon successful initialization of the SPI slave block, this function will
//! have initailized the slave block, but the SPI Slave block still remains
//! disabled and must be enabled with EUSCI_B_SPI_enable()
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI Slave module.
//! \param msbFirst controls the direction of the receive and transmit shift
//!        register.
//!        Valid values are:
//!        - \b EUSCI_B_SPI_MSB_FIRST
//!        - \b EUSCI_B_SPI_LSB_FIRST [Default]
//! \param clockPhase is clock phase select.
//!        Valid values are:
//!        - \b EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
//!           [Default]
//!        - \b EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
//! \param clockPolarity is clock polarity select
//!        Valid values are:
//!        - \b EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
//!        - \b EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW [Default]
//! \param spiMode is SPI mode select
//!        Valid values are:
//!        - \b EUSCI_B_SPI_3PIN
//!        - \b EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_HIGH
//!        - \b EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_LOW
//!
//! Modified bits are \b EUSCI_A_CTLW0_MSB, \b EUSCI_A_CTLW0_MST, \b EUSCI_A_CTLW0_SEVENBIT, \b EUSCI_A_CTLW0_CKPL, \b EUSCI_A_CTLW0_CKPH, \b
//! UCMODE and \b UCSWRST of \b UCAxCTLW0 register.
//!
//! \return true
//
//*****************************************************************************
bool EUSCI_B_SPI_slaveInit(uint32_t baseAddress, uint16_t msbFirst,
        uint16_t clockPhase, uint16_t clockPolarity, uint16_t spiMode)
{
    ASSERT(
            (EUSCI_B_SPI_MSB_FIRST == msbFirst)
            || (EUSCI_B_SPI_LSB_FIRST == msbFirst));

    ASSERT(
            (EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
                    == clockPhase)
            || (EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
                    == clockPhase));

    ASSERT(
            (EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH == clockPolarity)
            || (EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW
                    == clockPolarity));

    ASSERT(
            (EUSCI_B_SPI_3PIN == spiMode)
            || (EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_HIGH == spiMode)
            || (EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_LOW == spiMode));

    //Disable USCI Module
    BITBAND_PERI(EUSCI_B_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;

    //Reset OFS_UCBxCTLW0 register
    EUSCI_B_CMSIS(baseAddress)->CTLW0 = (EUSCI_B_CMSIS(baseAddress)->CTLW0
            & ~(EUSCI_A_CTLW0_MSB + EUSCI_A_CTLW0_SEVENBIT + EUSCI_A_CTLW0_MST + EUSCI_A_CTLW0_CKPL + EUSCI_A_CTLW0_CKPH + EUSCI_A_CTLW0_MODE_3))
            | (clockPhase + clockPolarity + msbFirst + EUSCI_A_CTLW0_SYNC + spiMode);

    return true;
}

//*****************************************************************************
//
//! \brief Changes the SPI colock phase and polarity. At the end of this
//! function call, SPI module is left enabled.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//! \param clockPhase is clock phase select.
//!        Valid values are:
//!        - \b EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
//!           [Default]
//!        - \b EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
//! \param clockPolarity is clock polarity select
//!        Valid values are:
//!        - \b EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
//!        - \b EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW [Default]
//!
//! Modified bits are \b EUSCI_A_CTLW0_CKPL, \b EUSCI_A_CTLW0_CKPH and \b UCSWRST of \b UCAxCTLW0
//! register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_B_SPI_changeClockPhasePolarity(uint32_t baseAddress,
        uint16_t clockPhase, uint16_t clockPolarity)
{

    ASSERT(
            (EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH == clockPolarity)
            || (EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW
                    == clockPolarity));

    ASSERT(
            (EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
                    == clockPhase)
            || (EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
                    == clockPhase));

    //Disable the USCI Module
    BITBAND_PERI(EUSCI_B_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;

    EUSCI_B_CMSIS(baseAddress)->CTLW0 = (EUSCI_B_CMSIS(baseAddress)->CTLW0
            & ~(EUSCI_A_CTLW0_CKPH + EUSCI_A_CTLW0_CKPL)) | (clockPhase + clockPolarity);

    //Reset the UCSWRST bit to enable the USCI Module
    BITBAND_PERI(EUSCI_B_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 0;
}

//*****************************************************************************
//
//! \brief Transmits a byte from the SPI Module.
//!
//! This function will place the supplied data into SPI trasmit data register
//! to start transmission.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//! \param transmitData data to be transmitted from the SPI module
//!
//! \return None
//
//*****************************************************************************
void EUSCI_B_SPI_transmitData(uint32_t baseAddress, uint8_t transmitData)
{
    EUSCI_B_CMSIS(baseAddress)->TXBUF = transmitData;
}

//*****************************************************************************
//
//! \brief Receives a byte that has been sent to the SPI Module.
//!
//! This function reads a byte of data from the SPI receive data Register.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//!
//! \return Returns the byte received from by the SPI module, cast as an
//!         uint8_t.
//
//*****************************************************************************
uint8_t EUSCI_B_SPI_receiveData(uint32_t baseAddress)
{
    return EUSCI_B_CMSIS(baseAddress)->RXBUF;
}

//*****************************************************************************
//
//! \brief Enables individual SPI interrupt sources.
//!
//! Enables the indicated SPI interrupt sources.  Only the sources that are
//! enabled can be reflected to the processor interrupt; disabled sources have
//! no effect on the processor. Does not clear interrupt flags.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//! \param mask is the bit mask of the interrupt sources to be enabled.
//!        Mask value is the logical OR of any of the following:
//!        - \b EUSCI_B_SPI_TRANSMIT_INTERRUPT
//!        - \b EUSCI_B_SPI_RECEIVE_INTERRUPT
//!
//! Modified bits of \b UCAxIFG register and bits of \b UCAxIE register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_B_SPI_enableInterrupt(uint32_t baseAddress, uint8_t mask)
{
    ASSERT(
            !(mask
                    & ~(EUSCI_B_SPI_RECEIVE_INTERRUPT
                            | EUSCI_B_SPI_TRANSMIT_INTERRUPT)));

    EUSCI_B_CMSIS(baseAddress)->IE |= mask;
}

//*****************************************************************************
//
//! \brief Disables individual SPI interrupt sources.
//!
//! Disables the indicated SPI interrupt sources. Only the sources that are
//! enabled can be reflected to the processor interrupt; disabled sources have
//! no effect on the processor.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//! \param mask is the bit mask of the interrupt sources to be disabled.
//!        Mask value is the logical OR of any of the following:
//!        - \b EUSCI_B_SPI_TRANSMIT_INTERRUPT
//!        - \b EUSCI_B_SPI_RECEIVE_INTERRUPT
//!
//! Modified bits of \b UCAxIE register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_B_SPI_disableInterrupt(uint32_t baseAddress, uint8_t mask)
{
    ASSERT(
            !(mask
                    & ~(EUSCI_B_SPI_RECEIVE_INTERRUPT
                            | EUSCI_B_SPI_TRANSMIT_INTERRUPT)));

    EUSCI_B_CMSIS(baseAddress)->IE &= ~mask;
}

//*****************************************************************************
//
//! \brief Gets the current SPI interrupt status.
//!
//! This returns the interrupt status for the SPI module based on which flag is
//! passed.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//! \param mask is the masked interrupt flag status to be returned.
//!        Mask value is the logical OR of any of the following:
//!        - \b EUSCI_B_SPI_TRANSMIT_INTERRUPT
//!        - \b EUSCI_B_SPI_RECEIVE_INTERRUPT
//!
//! \return Logical OR of any of the following:
//!         - \b EUSCI_B_SPI_TRANSMIT_INTERRUPT
//!         - \b EUSCI_B_SPI_RECEIVE_INTERRUPT
//!         \n indicating the status of the masked interrupts
//
//*****************************************************************************
uint8_t EUSCI_B_SPI_getInterruptStatus(uint32_t baseAddress, uint8_t mask)
{
    ASSERT(
            !(mask
                    & ~(EUSCI_B_SPI_RECEIVE_INTERRUPT
                            | EUSCI_B_SPI_TRANSMIT_INTERRUPT)));

    return EUSCI_B_CMSIS(baseAddress)->IFG & mask;
}

//*****************************************************************************
//
//! \brief Clears the selected SPI interrupt status flag.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//! \param mask is the masked interrupt flag to be cleared.
//!        Mask value is the logical OR of any of the following:
//!        - \b EUSCI_B_SPI_TRANSMIT_INTERRUPT
//!        - \b EUSCI_B_SPI_RECEIVE_INTERRUPT
//!
//! Modified bits of \b UCAxIFG register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_B_SPI_clearInterruptFlag(uint32_t baseAddress, uint8_t mask)
{
    ASSERT(
            !(mask
                    & ~(EUSCI_B_SPI_RECEIVE_INTERRUPT
                            | EUSCI_B_SPI_TRANSMIT_INTERRUPT)));

    EUSCI_B_CMSIS(baseAddress)->IFG &= ~mask;
}

//*****************************************************************************
//
//! \brief Enables the SPI block.
//!
//! This will enable operation of the SPI block.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//!
//! Modified bits are \b UCSWRST of \b UCBxCTLW0 register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_B_SPI_enable(uint32_t baseAddress)
{
    //Reset the UCSWRST bit to enable the USCI Module
    BITBAND_PERI(EUSCI_B_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 0;
}

//*****************************************************************************
//
//! \brief Disables the SPI block.
//!
//! This will disable operation of the SPI block.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//!
//! Modified bits are \b UCSWRST of \b UCBxCTLW0 register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_B_SPI_disable(uint32_t baseAddress)
{
    //Set the UCSWRST bit to disable the USCI Module
    BITBAND_PERI(EUSCI_B_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;
}

//*****************************************************************************
//
//! \brief Returns the address of the RX Buffer of the SPI for the DMA module.
//!
//! Returns the address of the SPI RX Buffer. This can be used in conjunction
//! with the DMA to store the received data directly to memory.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//!
//! \return the address of the RX Buffer
//
//*****************************************************************************
uint32_t EUSCI_B_SPI_getReceiveBufferAddressForDMA(uint32_t baseAddress)
{
    return ((uint32_t)(&EUSCI_B_CMSIS(baseAddress)->RXBUF));
}

//*****************************************************************************
//
//! \brief Returns the address of the TX Buffer of the SPI for the DMA module.
//!
//! Returns the address of the SPI TX Buffer. This can be used in conjunction
//! with the DMA to obtain transmitted data directly from memory.
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//!
//! \return the address of the TX Buffer
//
//*****************************************************************************
uint32_t EUSCI_B_SPI_getTransmitBufferAddressForDMA(uint32_t baseAddress)
{
    return ((uint32_t)(&EUSCI_B_CMSIS(baseAddress)->TXBUF));
}

//*****************************************************************************
//
//! \brief Indicates whether or not the SPI bus is busy.
//!
//! This function returns an indication of whether or not the SPI bus is
//! busy.This function checks the status of the bus via UCBBUSY bit
//!
//! \param baseAddress is the base address of the EUSCI_B_SPI module.
//!
//! \return true if busy, false otherwise
//
//*****************************************************************************
bool EUSCI_B_SPI_isBusy(uint32_t baseAddress)
{
    //Return the bus busy status.
    return BITBAND_PERI(EUSCI_B_CMSIS(baseAddress)->STATW, EUSCI_B_STATW_BBUSY_OFS);
}

//*****************************************************************************
//
//! \brief Selects 4Pin Functionality
//!
//! This function should be invoked only in 4-wire mode. Invoking this function
//! has no effect in 3-wire mode.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//! \param select4PinFunctionality selects 4 pin functionality
//!        Valid values are:
//!        - \b EUSCI_A_SPI_PREVENT_CONFLICTS_WITH_OTHER_MASTERS
//!        - \b EUSCI_A_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVE
//!
//! Modified bits are \b UCSTEM of \b UCAxCTLW0 register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_A_SPI_select4PinFunctionality(uint32_t baseAddress,
        uint8_t select4PinFunctionality)
{
    ASSERT(
            (EUSCI_A_SPI_PREVENT_CONFLICTS_WITH_OTHER_MASTERS
                    == select4PinFunctionality)
            || (EUSCI_A_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVE
                    == select4PinFunctionality));

    EUSCI_A_CMSIS(baseAddress)->CTLW0 = (EUSCI_A_CMSIS(baseAddress)->CTLW0
            & ~EUSCI_A_CTLW0_STEM) | select4PinFunctionality;
}

//*****************************************************************************
//
//! \brief Initializes the SPI Master clock. At the end of this function call,
//! SPI module is left enabled.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//! \param clockSourceFrequency is the frequency of the slected clock source
//! \param desiredSpiClock is the desired clock rate for SPI communication
//!
//! Modified bits are \b UCSWRST of \b UCAxCTLW0 register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_A_SPI_masterChangeClock(uint32_t baseAddress,
        uint32_t clockSourceFrequency, uint32_t desiredSpiClock)
{
    //Disable the USCI Module
    BITBAND_PERI(EUSCI_A_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;

    EUSCI_A_CMSIS(baseAddress)->BRW = (uint16_t) (clockSourceFrequency
            / desiredSpiClock);

    //Reset the UCSWRST bit to enable the USCI Module
    BITBAND_PERI(EUSCI_A_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 0;
}

//*****************************************************************************
//
//! \brief Initializes the SPI Slave block.
//!
//! Upon successful initialization of the SPI slave block, this function will
//! have initailized the slave block, but the SPI Slave block still remains
//! disabled and must be enabled with EUSCI_A_SPI_enable()
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI Slave module.
//! \param msbFirst controls the direction of the receive and transmit shift
//!        register.
//!        Valid values are:
//!        - \b EUSCI_A_SPI_MSB_FIRST
//!        - \b EUSCI_A_SPI_LSB_FIRST [Default]
//! \param clockPhase is clock phase select.
//!        Valid values are:
//!        - \b EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
//!           [Default]
//!        - \b EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
//! \param clockPolarity is clock polarity select
//!        Valid values are:
//!        - \b EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
//!        - \b EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW [Default]
//! \param spiMode is SPI mode select
//!        Valid values are:
//!        - \b EUSCI_A_SPI_3PIN
//!        - \b EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_HIGH
//!        - \b EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_LOW
//!
//! Modified bits are \b EUSCI_A_CTLW0_MSB, \b EUSCI_A_CTLW0_MST, \b EUSCI_A_CTLW0_SEVENBIT, \b EUSCI_A_CTLW0_CKPL, \b EUSCI_A_CTLW0_CKPH, \b
//! UCMODE and \b UCSWRST of \b UCAxCTLW0 register.
//!
//! \return true
//
//*****************************************************************************
bool EUSCI_A_SPI_slaveInit(uint32_t baseAddress, uint16_t msbFirst,
        uint16_t clockPhase, uint16_t clockPolarity, uint16_t spiMode)
{
    ASSERT(
            (EUSCI_A_SPI_MSB_FIRST == msbFirst)
            || (EUSCI_A_SPI_LSB_FIRST == msbFirst));

    ASSERT(
            (EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
                    == clockPhase)
            || (EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
                    == clockPhase));

    ASSERT(
            (EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_HIGH == clockPolarity)
            || (EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW
                    == clockPolarity));

    ASSERT(
            (EUSCI_A_SPI_3PIN == spiMode)
            || (EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_HIGH == spiMode)
            || (EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_LOW == spiMode));

    //Disable USCI Module
    BITBAND_PERI(EUSCI_A_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;

    //Reset OFS_UCAxCTLW0 register
    EUSCI_A_CMSIS(baseAddress)->CTLW0 = (EUSCI_A_CMSIS(baseAddress)->CTLW0
            & ~(EUSCI_A_CTLW0_MSB + EUSCI_A_CTLW0_SEVENBIT + EUSCI_A_CTLW0_MST + EUSCI_A_CTLW0_CKPL + EUSCI_A_CTLW0_CKPH + EUSCI_A_CTLW0_MODE_3))
            | (clockPhase + clockPolarity + msbFirst + EUSCI_A_CTLW0_SYNC + spiMode);

    return true;
}

//*****************************************************************************
//
//! \brief Changes the SPI colock phase and polarity. At the end of this
//! function call, SPI module is left enabled.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//! \param clockPhase is clock phase select.
//!        Valid values are:
//!        - \b EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
//!           [Default]
//!        - \b EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
//! \param clockPolarity is clock polarity select
//!        Valid values are:
//!        - \b EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_HIGH
//!        - \b EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW [Default]
//!
//! Modified bits are \b EUSCI_A_CTLW0_CKPL, \b EUSCI_A_CTLW0_CKPH and \b UCSWRST of \b UCAxCTLW0
//! register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_A_SPI_changeClockPhasePolarity(uint32_t baseAddress,
        uint16_t clockPhase, uint16_t clockPolarity)
{

    ASSERT(
            (EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_HIGH == clockPolarity)
            || (EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW
                    == clockPolarity));

    ASSERT(
            (EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT
                    == clockPhase)
            || (EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT
                    == clockPhase));

    //Disable the USCI Module
    BITBAND_PERI(EUSCI_A_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;

    EUSCI_A_CMSIS(baseAddress)->CTLW0 = (EUSCI_A_CMSIS(baseAddress)->CTLW0
            & ~(EUSCI_A_CTLW0_CKPH + EUSCI_A_CTLW0_CKPL)) | (clockPhase + clockPolarity);

    //Reset the UCSWRST bit to enable the USCI Module
    BITBAND_PERI(EUSCI_A_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 0;
}

//*****************************************************************************
//
//! \brief Transmits a byte from the SPI Module.
//!
//! This function will place the supplied data into SPI trasmit data register
//! to start transmission.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//! \param transmitData data to be transmitted from the SPI module
//!
//! \return None
//
//*****************************************************************************
void EUSCI_A_SPI_transmitData(uint32_t baseAddress, uint8_t transmitData)
{
    EUSCI_A_CMSIS(baseAddress)->TXBUF = transmitData;
}

//*****************************************************************************
//
//! \brief Receives a byte that has been sent to the SPI Module.
//!
//! This function reads a byte of data from the SPI receive data Register.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//!
//! \return Returns the byte received from by the SPI module, cast as an
//!         uint8_t.
//
//*****************************************************************************
uint8_t EUSCI_A_SPI_receiveData(uint32_t baseAddress)
{
    return EUSCI_A_CMSIS(baseAddress)->RXBUF;
}

//*****************************************************************************
//
//! \brief Enables individual SPI interrupt sources.
//!
//! Enables the indicated SPI interrupt sources.  Only the sources that are
//! enabled can be reflected to the processor interrupt; disabled sources have
//! no effect on the processor. Does not clear interrupt flags.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//! \param mask is the bit mask of the interrupt sources to be enabled.
//!        Mask value is the logical OR of any of the following:
//!        - \b EUSCI_A_SPI_TRANSMIT_INTERRUPT
//!        - \b EUSCI_A_SPI_RECEIVE_INTERRUPT
//!
//! Modified bits of \b UCAxIFG register and bits of \b UCAxIE register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_A_SPI_enableInterrupt(uint32_t baseAddress, uint8_t mask)
{
    ASSERT(
            !(mask
                    & ~(EUSCI_A_SPI_RECEIVE_INTERRUPT
                            | EUSCI_A_SPI_TRANSMIT_INTERRUPT)));

    EUSCI_A_CMSIS(baseAddress)->IE |= mask;
}

//*****************************************************************************
//
//! \brief Disables individual SPI interrupt sources.
//!
//! Disables the indicated SPI interrupt sources. Only the sources that are
//! enabled can be reflected to the processor interrupt; disabled sources have
//! no effect on the processor.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//! \param mask is the bit mask of the interrupt sources to be disabled.
//!        Mask value is the logical OR of any of the following:
//!        - \b EUSCI_A_SPI_TRANSMIT_INTERRUPT
//!        - \b EUSCI_A_SPI_RECEIVE_INTERRUPT
//!
//! Modified bits of \b UCAxIE register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_A_SPI_disableInterrupt(uint32_t baseAddress, uint8_t mask)
{
    ASSERT(
            !(mask
                    & ~(EUSCI_A_SPI_RECEIVE_INTERRUPT
                            | EUSCI_A_SPI_TRANSMIT_INTERRUPT)));

    EUSCI_A_CMSIS(baseAddress)->IE &= ~mask;
}

//*****************************************************************************
//
//! \brief Gets the current SPI interrupt status.
//!
//! This returns the interrupt status for the SPI module based on which flag is
//! passed.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//! \param mask is the masked interrupt flag status to be returned.
//!        Mask value is the logical OR of any of the following:
//!        - \b EUSCI_A_SPI_TRANSMIT_INTERRUPT
//!        - \b EUSCI_A_SPI_RECEIVE_INTERRUPT
//!
//! \return Logical OR of any of the following:
//!         - \b EUSCI_A_SPI_TRANSMIT_INTERRUPT
//!         - \b EUSCI_A_SPI_RECEIVE_INTERRUPT
//!         \n indicating the status of the masked interrupts
//
//*****************************************************************************
uint8_t EUSCI_A_SPI_getInterruptStatus(uint32_t baseAddress, uint8_t mask)
{
    ASSERT(
            !(mask
                    & ~(EUSCI_A_SPI_RECEIVE_INTERRUPT
                            | EUSCI_A_SPI_TRANSMIT_INTERRUPT)));

    return EUSCI_A_CMSIS(baseAddress)->IFG & mask;
}

//*****************************************************************************
//
//! \brief Clears the selected SPI interrupt status flag.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//! \param mask is the masked interrupt flag to be cleared.
//!        Mask value is the logical OR of any of the following:
//!        - \b EUSCI_A_SPI_TRANSMIT_INTERRUPT
//!        - \b EUSCI_A_SPI_RECEIVE_INTERRUPT
//!
//! Modified bits of \b UCAxIFG register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_A_SPI_clearInterruptFlag(uint32_t baseAddress, uint8_t mask)
{
    ASSERT(
            !(mask
                    & ~(EUSCI_A_SPI_RECEIVE_INTERRUPT
                            | EUSCI_A_SPI_TRANSMIT_INTERRUPT)));

    EUSCI_A_CMSIS(baseAddress)->IFG &= ~mask;
}

//*****************************************************************************
//
//! \brief Enables the SPI block.
//!
//! This will enable operation of the SPI block.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//!
//! Modified bits are \b UCSWRST of \b UCAxCTLW0 register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_A_SPI_enable(uint32_t baseAddress)
{
    //Reset the UCSWRST bit to enable the USCI Module
    BITBAND_PERI(EUSCI_A_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 0;
}

//*****************************************************************************
//
//! \brief Disables the SPI block.
//!
//! This will disable operation of the SPI block.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//!
//! Modified bits are \b UCSWRST of \b UCAxCTLW0 register.
//!
//! \return None
//
//*****************************************************************************
void EUSCI_A_SPI_disable(uint32_t baseAddress)
{
    //Set the UCSWRST bit to disable the USCI Module
    BITBAND_PERI(EUSCI_A_CMSIS(baseAddress)->CTLW0, EUSCI_A_CTLW0_SWRST_OFS) = 1;
}

//*****************************************************************************
//
//! \brief Returns the address of the RX Buffer of the SPI for the DMA module.
//!
//! Returns the address of the SPI RX Buffer. This can be used in conjunction
//! with the DMA to store the received data directly to memory.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//!
//! \return the address of the RX Buffer
//
//*****************************************************************************
uint32_t EUSCI_A_SPI_getReceiveBufferAddressForDMA(uint32_t baseAddress)
{
    return (uint32_t)&EUSCI_A_CMSIS(baseAddress)->RXBUF;
}

//*****************************************************************************
//
//! \brief Returns the address of the TX Buffer of the SPI for the DMA module.
//!
//! Returns the address of the SPI TX Buffer. This can be used in conjunction
//! with the DMA to obtain transmitted data directly from memory.
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//!
//! \return the address of the TX Buffer
//
//*****************************************************************************
uint32_t EUSCI_A_SPI_getTransmitBufferAddressForDMA(uint32_t baseAddress)
{
    return (uint32_t)&EUSCI_A_CMSIS(baseAddress)->TXBUF;
}

//*****************************************************************************
//
//! \brief Indicates whether or not the SPI bus is busy.
//!
//! This function returns an indication of whether or not the SPI bus is
//! busy.This function checks the status of the bus via UCBBUSY bit
//!
//! \param baseAddress is the base address of the EUSCI_A_SPI module.
//!
//! \return true if busy, false otherwise
//*****************************************************************************
bool EUSCI_A_SPI_isBusy(uint32_t baseAddress)
{
    //Return the bus busy status.
    return BITBAND_PERI(EUSCI_A_CMSIS(baseAddress)->STATW, EUSCI_B_STATW_BBUSY_OFS);
}
