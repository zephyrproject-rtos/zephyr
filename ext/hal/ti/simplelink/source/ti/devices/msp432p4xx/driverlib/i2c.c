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
#include <ti/devices/msp432p4xx/driverlib/i2c.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>

void I2C_initMaster(uint32_t moduleInstance,
        const eUSCI_I2C_MasterConfig *config)
{
    uint_fast16_t preScalarValue;

    ASSERT(
            (EUSCI_B_I2C_CLOCKSOURCE_ACLK == config->selectClockSource)
                    || (EUSCI_B_I2C_CLOCKSOURCE_SMCLK
                            == config->selectClockSource));

    ASSERT(
            (EUSCI_B_I2C_SET_DATA_RATE_400KBPS == config->dataRate)
                    || (EUSCI_B_I2C_SET_DATA_RATE_100KBPS == config->dataRate)
                    || (EUSCI_B_I2C_SET_DATA_RATE_1MBPS == config->dataRate));

    ASSERT(
            (EUSCI_B_I2C_NO_AUTO_STOP == config->autoSTOPGeneration)
                    || (EUSCI_B_I2C_SET_BYTECOUNT_THRESHOLD_FLAG
                            == config->autoSTOPGeneration)
                    || (EUSCI_B_I2C_SEND_STOP_AUTOMATICALLY_ON_BYTECOUNT_THRESHOLD
                            == config->autoSTOPGeneration));

    /* Disable the USCI module and clears the other bits of control register */
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_SWRST_OFS) =
            1;

    /* Configure Automatic STOP condition generation */
    EUSCI_B_CMSIS(moduleInstance)->CTLW1 = (EUSCI_B_CMSIS(moduleInstance)->CTLW1
            & ~EUSCI_B_CTLW1_ASTP_MASK) | (config->autoSTOPGeneration);

    /* Byte Count Threshold */
    EUSCI_B_CMSIS(moduleInstance)->TBCNT = config->byteCounterThreshold;

    /*
     * Configure as I2C master mode.
     * UCMST = Master mode
     * UCMODE_3 = I2C mode
     * UCSYNC = Synchronous mode
     */
    EUSCI_B_CMSIS(moduleInstance)->CTLW0 = (EUSCI_B_CMSIS(moduleInstance)->CTLW0
            & ~EUSCI_B_CTLW0_SSEL_MASK)
            | (config->selectClockSource | EUSCI_B_CTLW0_MST
                    | EUSCI_B_CTLW0_MODE_3 | EUSCI_B_CTLW0_SYNC
                    | EUSCI_B_CTLW0_SWRST);

    /*
     * Compute the clock divider that achieves the fastest speed less than or
     * equal to the desired speed.  The numerator is biased to favor a larger
     * clock divider so that the resulting clock is always less than or equal
     * to the desired clock, never greater.
     */
    preScalarValue = (uint16_t) (config->i2cClk / config->dataRate);

    EUSCI_B_CMSIS(moduleInstance)->BRW = preScalarValue;
}

void I2C_initSlave(uint32_t moduleInstance, uint_fast16_t slaveAddress,
        uint_fast8_t slaveAddressOffset, uint32_t slaveOwnAddressEnable)
{
    ASSERT(
            (EUSCI_B_I2C_OWN_ADDRESS_OFFSET0 == slaveAddressOffset)
                    || (EUSCI_B_I2C_OWN_ADDRESS_OFFSET1 == slaveAddressOffset)
                    || (EUSCI_B_I2C_OWN_ADDRESS_OFFSET2 == slaveAddressOffset)
                    || (EUSCI_B_I2C_OWN_ADDRESS_OFFSET3 == slaveAddressOffset));

    /* Disable the USCI module */
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_SWRST_OFS) =
            1;

    /* Clear USCI master mode */
    EUSCI_B_CMSIS(moduleInstance)->CTLW0 = (EUSCI_B_CMSIS(moduleInstance)->CTLW0
            & (~EUSCI_B_CTLW0_MST))
            | (EUSCI_B_CTLW0_MODE_3 + EUSCI_B_CTLW0_SYNC);

    /* Set up the slave address. */
    HWREG16(
            (uint32_t) &EUSCI_B_CMSIS(moduleInstance)->I2COA0
                    + slaveAddressOffset) = slaveAddress
            + slaveOwnAddressEnable;
}

void I2C_enableModule(uint32_t moduleInstance)
{
    /* Reset the UCSWRST bit to enable the USCI Module */
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_SWRST_OFS) =
            0;
}

void I2C_disableModule(uint32_t moduleInstance)
{
    /* Set the UCSWRST bit to disable the USCI Module */
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_SWRST_OFS) =
            1;
    ;
}

void I2C_setSlaveAddress(uint32_t moduleInstance, uint_fast16_t slaveAddress)
{
    /* Set the address of the slave with which the master will communicate */
    EUSCI_B_CMSIS(moduleInstance)->I2CSA = (slaveAddress);
}

void I2C_setMode(uint32_t moduleInstance, uint_fast8_t mode)
{
    ASSERT(
            (EUSCI_B_I2C_TRANSMIT_MODE == mode)
                    || (EUSCI_B_I2C_RECEIVE_MODE == mode));

    EUSCI_B_CMSIS(moduleInstance)->CTLW0 = (EUSCI_B_CMSIS(moduleInstance)->CTLW0
            & (~EUSCI_B_I2C_TRANSMIT_MODE)) | mode;

}

uint8_t I2C_masterReceiveSingleByte(uint32_t moduleInstance)
{
    //Set USCI in Receive mode
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_TR_OFS) =
            0;

    //Send start
    EUSCI_B_CMSIS(moduleInstance)->CTLW0 |= (EUSCI_B_CTLW0_TXSTT
            + EUSCI_B_CTLW0_TXSTP);

    //Poll for receive interrupt flag.
    while (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
            EUSCI_B_IFG_RXIFG_OFS))
        ;

    //Send single byte data.
    return (EUSCI_B_CMSIS(moduleInstance)->RXBUF & EUSCI_B_RXBUF_RXBUF_MASK);
}

void I2C_slavePutData(uint32_t moduleInstance, uint8_t transmitData)
{
    //Send single byte data.
    EUSCI_B_CMSIS(moduleInstance)->TXBUF = transmitData;
}

uint8_t I2C_slaveGetData(uint32_t moduleInstance)
{
    //Read a byte.
    return (EUSCI_B_CMSIS(moduleInstance)->RXBUF & EUSCI_B_RXBUF_RXBUF_MASK);
}

uint8_t I2C_isBusBusy(uint32_t moduleInstance)
{
    //Return the bus busy status.
    return BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->STATW,
            EUSCI_B_STATW_BBUSY_OFS);
}

void I2C_masterSendSingleByte(uint32_t moduleInstance, uint8_t txData)
{
    //Store current TXIE status
    uint16_t txieStatus = EUSCI_B_CMSIS(moduleInstance)->IE & EUSCI_B_IE_TXIE0;

    //Disable transmit interrupt enable
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IE, EUSCI_B_IE_TXIE0_OFS) = 0;

    //Send start condition.
    EUSCI_B_CMSIS(moduleInstance)->CTLW0 |= EUSCI_B_CTLW0_TR
            + EUSCI_B_CTLW0_TXSTT;

    //Poll for transmit interrupt flag and start condition flag.
    while ((BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0,
                EUSCI_B_CTLW0_TXSTT_OFS)
                || !BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                        EUSCI_B_IFG_TXIFG0_OFS)));

    //Send single byte data.
    EUSCI_B_CMSIS(moduleInstance)->TXBUF = txData;

    //Poll for transmit interrupt flag.
    while (!(EUSCI_B_CMSIS(moduleInstance)->IFG & EUSCI_B_IFG_TXIFG))
        ;

    //Send stop condition.
    EUSCI_B_CMSIS(moduleInstance)->CTLW0 |= EUSCI_B_CTLW0_TXSTP;

    //Clear transmit interrupt flag before enabling interrupt again
    EUSCI_B_CMSIS(moduleInstance)->IFG &= ~(EUSCI_B_IFG_TXIFG);

    //Reinstate transmit interrupt enable
    EUSCI_B_CMSIS(moduleInstance)->IE |= txieStatus;
}

bool I2C_masterSendSingleByteWithTimeout(uint32_t moduleInstance,
        uint8_t txData, uint32_t timeout)
{
    uint_fast16_t txieStatus;
    uint32_t timeout2 = timeout;

    ASSERT(timeout > 0);

    //Store current TXIE status
    txieStatus = EUSCI_B_CMSIS(moduleInstance)->IE & EUSCI_B_IE_TXIE0;

    //Disable transmit interrupt enable
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IE, EUSCI_B_IE_TXIE0_OFS) = 0;

    //Send start condition.
    EUSCI_B_CMSIS(moduleInstance)->CTLW0 |= EUSCI_B_CTLW0_TR
            + EUSCI_B_CTLW0_TXSTT;

    //Poll for transmit interrupt flag.
    while ((!(EUSCI_B_CMSIS(moduleInstance)->IFG & EUSCI_B_IFG_TXIFG))
            && --timeout)
        ;

    //Check if transfer timed out
    if (timeout == 0)
        return false;

    //Send single byte data.
    EUSCI_B_CMSIS(moduleInstance)->TXBUF = txData;

    //Poll for transmit interrupt flag.
    while ((!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
            EUSCI_B_IFG_TXIFG0_OFS)) && --timeout2)
        ;

    //Check if transfer timed out
    if (timeout2 == 0)
        return false;

    //Send stop condition.
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_TXSTP_OFS) =
            1;

    //Clear transmit interrupt flag before enabling interrupt again
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG, EUSCI_B_IFG_TXIFG0_OFS) =
            0;

    //Reinstate transmit interrupt enable
    EUSCI_B_CMSIS(moduleInstance)->IE |= txieStatus;

    return true;
}

void I2C_masterSendMultiByteStart(uint32_t moduleInstance, uint8_t txData)
{
    //Store current transmit interrupt enable
    uint16_t txieStatus = EUSCI_B_CMSIS(moduleInstance)->IE & EUSCI_B_IE_TXIE0;

    //Disable transmit interrupt enable
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IE, EUSCI_B_IE_TXIE0_OFS) = 0;

    //Send start condition.
    EUSCI_B_CMSIS(moduleInstance)->CTLW0 |= EUSCI_B_CTLW0_TR
            + EUSCI_B_CTLW0_TXSTT;

    //Poll for transmit interrupt flag and start condition flag.
    while (BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0,
                EUSCI_B_CTLW0_TXSTT_OFS)
                || !BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                        EUSCI_B_IFG_TXIFG0_OFS));

    //Send single byte data.
    EUSCI_B_CMSIS(moduleInstance)->TXBUF = txData;

    //Reinstate transmit interrupt enable
    EUSCI_B_CMSIS(moduleInstance)->IE |= txieStatus;
}

bool I2C_masterSendMultiByteStartWithTimeout(uint32_t moduleInstance,
        uint8_t txData, uint32_t timeout)
{
    uint_fast16_t txieStatus;

    ASSERT(timeout > 0);

    //Store current transmit interrupt enable
    txieStatus = EUSCI_B_CMSIS(moduleInstance)->IE & EUSCI_B_IE_TXIE0;

    //Disable transmit interrupt enable
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IE, EUSCI_B_IE_TXIE0_OFS) = 0;

    //Send start condition.
    EUSCI_B_CMSIS(moduleInstance)->CTLW0 |= EUSCI_B_CTLW0_TR
            + EUSCI_B_CTLW0_TXSTT;

    //Poll for transmit interrupt flag and start condition flag.
    while ((BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0,
                EUSCI_B_CTLW0_TXSTT_OFS)
                || !BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                        EUSCI_B_IFG_TXIFG0_OFS)) && --timeout);


    //Check if transfer timed out
    if (timeout == 0)
        return false;

    //Send single byte data.
    EUSCI_B_CMSIS(moduleInstance)->TXBUF = txData;

    //Reinstate transmit interrupt enable
    EUSCI_B_CMSIS(moduleInstance)->IE |= txieStatus;

    return true;
}

void I2C_masterSendMultiByteNext(uint32_t moduleInstance, uint8_t txData)
{
    //If interrupts are not used, poll for flags
    if (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IE, EUSCI_B_IE_TXIE0_OFS))
    {
        //Poll for transmit interrupt flag.
        while (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                EUSCI_B_IFG_TXIFG0_OFS))
            ;
    }

    //Send single byte data.
    EUSCI_B_CMSIS(moduleInstance)->TXBUF = txData;
}

bool I2C_masterSendMultiByteNextWithTimeout(uint32_t moduleInstance,
        uint8_t txData, uint32_t timeout)
{
    ASSERT(timeout > 0);

    //If interrupts are not used, poll for flags
    if (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IE, EUSCI_B_IE_TXIE0_OFS))
    {
        //Poll for transmit interrupt flag.
        while ((!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                EUSCI_B_IFG_TXIFG0_OFS)) && --timeout)
            ;

        //Check if transfer timed out
        if (timeout == 0)
            return false;
    }

    //Send single byte data.
    EUSCI_B_CMSIS(moduleInstance)->TXBUF = txData;

    return true;
}

bool I2C_masterSendMultiByteFinish(uint32_t moduleInstance, uint8_t txData)
{   
    //If interrupts are not used, poll for flags
    if (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IE, EUSCI_B_IE_TXIE0_OFS))
    {
        //Poll for transmit interrupt flag.
        while (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                EUSCI_B_IFG_TXIFG0_OFS))
            ;
    }

    //Send single byte data.
    EUSCI_B_CMSIS(moduleInstance)->TXBUF = txData;

    //Poll for transmit interrupt flag.
    while (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
            EUSCI_B_IFG_TXIFG0_OFS)
            && !BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                    EUSCI_B_IFG_NACKIFG_OFS))
        ;
    if(BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG, EUSCI_B_IFG_NACKIFG_OFS))
        return false;
    
    //Send stop condition.
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_TXSTP_OFS) =
            1;
            
    return true;
}

bool I2C_masterSendMultiByteFinishWithTimeout(uint32_t moduleInstance,
        uint8_t txData, uint32_t timeout)
{
    uint32_t timeout2 = timeout;

    ASSERT(timeout > 0);

    //If interrupts are not used, poll for flags
    if (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IE, EUSCI_B_IE_TXIE0_OFS))
    {
        //Poll for transmit interrupt flag.
        while ((!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                EUSCI_B_IFG_TXIFG0_OFS)) && --timeout)
            ;

        //Check if transfer timed out
        if (timeout == 0)
            return false;
    }

    //Send single byte data.
    EUSCI_B_CMSIS(moduleInstance)->TXBUF = txData;

    //Poll for transmit interrupt flag.
    while ((!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
            EUSCI_B_IFG_TXIFG0_OFS)) && --timeout2
            && !BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                    EUSCI_B_IFG_NACKIFG_OFS))
        ;

    //Check if transfer timed out
    if (timeout2 == 0)
        return false;

    //Send stop condition.
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_TXSTP_OFS) =
            1;

    return true;
}

void I2C_masterSendMultiByteStop(uint32_t moduleInstance)
{
    //If interrupts are not used, poll for flags
    if (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IE, EUSCI_B_IE_TXIE0_OFS))
    {
        //Poll for transmit interrupt flag.
        while (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                EUSCI_B_IFG_TXIFG0_OFS))
            ;
    }

    //Send stop condition.
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_TXSTP_OFS) =
            1;
}

bool I2C_masterSendMultiByteStopWithTimeout(uint32_t moduleInstance,
        uint32_t timeout)
{
    ASSERT(timeout > 0);

    //If interrupts are not used, poll for flags
    if (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IE, EUSCI_B_IE_TXIE0_OFS))
    {
        //Poll for transmit interrupt flag.
        while ((!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                EUSCI_B_IFG_TXIFG0_OFS)) && --timeout)
            ;

        //Check if transfer timed out
        if (timeout == 0)
            return false;
    }

    //Send stop condition.
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_TXSTP_OFS) =
            1;

    return 0x01;
}

void I2C_masterReceiveStart(uint32_t moduleInstance)
{
    //Set USCI in Receive mode
    EUSCI_B_CMSIS(moduleInstance)->CTLW0 = (EUSCI_B_CMSIS(moduleInstance)->CTLW0
            & (~EUSCI_B_CTLW0_TR)) | EUSCI_B_CTLW0_TXSTT;
}

uint8_t I2C_masterReceiveMultiByteNext(uint32_t moduleInstance)
{
    return (EUSCI_B_CMSIS(moduleInstance)->RXBUF & EUSCI_B_RXBUF_RXBUF_MASK);
}

uint8_t I2C_masterReceiveMultiByteFinish(uint32_t moduleInstance)
{
    //Send stop condition.
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_TXSTP_OFS) =
            1;

    //Wait for Stop to finish
    while (BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0,
            EUSCI_B_CTLW0_TXSTP_OFS))
    {
        // Wait for RX buffer
        while (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                EUSCI_B_IFG_RXIFG_OFS))
            ;
    }

    /* Capture data from receive buffer after setting stop bit due to
     MSP430 I2C critical timing. */
    return (EUSCI_B_CMSIS(moduleInstance)->RXBUF & EUSCI_B_RXBUF_RXBUF_MASK);
}

bool I2C_masterReceiveMultiByteFinishWithTimeout(uint32_t moduleInstance,
        uint8_t *txData, uint32_t timeout)
{
    uint32_t timeout2 = timeout;

    ASSERT(timeout > 0);

    //Send stop condition.
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_TXSTP_OFS) =
            1;

    //Wait for Stop to finish
    while (BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0,
            EUSCI_B_CTLW0_TXSTP_OFS) && --timeout)
        ;

    //Check if transfer timed out
    if (timeout == 0)
        return false;

    // Wait for RX buffer
    while ((!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
            EUSCI_B_IFG_RXIFG_OFS)) && --timeout2)
        ;

    //Check if transfer timed out
    if (timeout2 == 0)
        return false;

    //Capture data from receive buffer after setting stop bit due to
    //MSP430 I2C critical timing.
    *txData = (EUSCI_B_CMSIS(moduleInstance)->RXBUF & EUSCI_B_RXBUF_RXBUF_MASK);

    return true;
}

void I2C_masterReceiveMultiByteStop(uint32_t moduleInstance)
{
    //Send stop condition.
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_TXSTP_OFS) =
            1;
}

uint8_t I2C_masterReceiveSingle(uint32_t moduleInstance)
{
    //Polling RXIFG0 if RXIE is not enabled
    if (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IE, EUSCI_B_IE_RXIE0_OFS))
    {
        while (!BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->IFG,
                EUSCI_B_IFG_RXIFG0_OFS))
            ;
    }

    //Read a byte.
    return (EUSCI_B_CMSIS(moduleInstance)->RXBUF & EUSCI_B_RXBUF_RXBUF_MASK);
}

uint32_t I2C_getReceiveBufferAddressForDMA(uint32_t moduleInstance)
{
    return (uint32_t) &EUSCI_B_CMSIS(moduleInstance)->RXBUF;
}

uint32_t I2C_getTransmitBufferAddressForDMA(uint32_t moduleInstance)
{
    return (uint32_t) &EUSCI_B_CMSIS(moduleInstance)->TXBUF;
}

uint8_t I2C_masterIsStopSent(uint32_t moduleInstance)
{
    return BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0,
            EUSCI_B_CTLW0_TXSTP_OFS);
}

bool I2C_masterIsStartSent(uint32_t moduleInstance)
{
    return BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0,
            EUSCI_B_CTLW0_TXSTT_OFS);
}

void I2C_masterSendStart(uint32_t moduleInstance)
{
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_TXSTT_OFS) =
            1;
}

void I2C_enableMultiMasterMode(uint32_t moduleInstance)
{
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_SWRST_OFS) =
            1;
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_MM_OFS) =
            1;
}

void I2C_disableMultiMasterMode(uint32_t moduleInstance)
{
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_SWRST_OFS) =
            1;
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_MM_OFS) =
            0;
}

void I2C_enableInterrupt(uint32_t moduleInstance, uint_fast16_t mask)
{
    ASSERT(
            0x00
                    == (mask
                            & ~(EUSCI_B_I2C_STOP_INTERRUPT
                                    + EUSCI_B_I2C_START_INTERRUPT
                                    + EUSCI_B_I2C_NAK_INTERRUPT
                                    + EUSCI_B_I2C_ARBITRATIONLOST_INTERRUPT
                                    + EUSCI_B_I2C_BIT9_POSITION_INTERRUPT
                                    + EUSCI_B_I2C_CLOCK_LOW_TIMEOUT_INTERRUPT
                                    + EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT0
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT1
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT2
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT3
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT0
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT1
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT2
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT3)));

    //Enable the interrupt masked bit
    EUSCI_B_CMSIS(moduleInstance)->IE |= mask;
}

void I2C_disableInterrupt(uint32_t moduleInstance, uint_fast16_t mask)
{
    ASSERT(
            0x00
                    == (mask
                            & ~(EUSCI_B_I2C_STOP_INTERRUPT
                                    + EUSCI_B_I2C_START_INTERRUPT
                                    + EUSCI_B_I2C_NAK_INTERRUPT
                                    + EUSCI_B_I2C_ARBITRATIONLOST_INTERRUPT
                                    + EUSCI_B_I2C_BIT9_POSITION_INTERRUPT
                                    + EUSCI_B_I2C_CLOCK_LOW_TIMEOUT_INTERRUPT
                                    + EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT0
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT1
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT2
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT3
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT0
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT1
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT2
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT3)));

    //Disable the interrupt masked bit
    EUSCI_B_CMSIS(moduleInstance)->IE &= ~(mask);
}

void I2C_clearInterruptFlag(uint32_t moduleInstance, uint_fast16_t mask)
{
    ASSERT(
            0x00
                    == (mask
                            & ~(EUSCI_B_I2C_STOP_INTERRUPT
                                    + EUSCI_B_I2C_START_INTERRUPT
                                    + EUSCI_B_I2C_NAK_INTERRUPT
                                    + EUSCI_B_I2C_ARBITRATIONLOST_INTERRUPT
                                    + EUSCI_B_I2C_BIT9_POSITION_INTERRUPT
                                    + EUSCI_B_I2C_CLOCK_LOW_TIMEOUT_INTERRUPT
                                    + EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT0
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT1
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT2
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT3
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT0
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT1
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT2
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT3)));
    //Clear the I2C interrupt source.
    EUSCI_B_CMSIS(moduleInstance)->IFG &= ~(mask);
}

uint_fast16_t I2C_getInterruptStatus(uint32_t moduleInstance, uint16_t mask)
{
    ASSERT(
            0x00
                    == (mask
                            & ~(EUSCI_B_I2C_STOP_INTERRUPT
                                    + EUSCI_B_I2C_START_INTERRUPT
                                    + EUSCI_B_I2C_NAK_INTERRUPT
                                    + EUSCI_B_I2C_ARBITRATIONLOST_INTERRUPT
                                    + EUSCI_B_I2C_BIT9_POSITION_INTERRUPT
                                    + EUSCI_B_I2C_CLOCK_LOW_TIMEOUT_INTERRUPT
                                    + EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT0
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT1
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT2
                                    + EUSCI_B_I2C_TRANSMIT_INTERRUPT3
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT0
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT1
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT2
                                    + EUSCI_B_I2C_RECEIVE_INTERRUPT3)));
    //Return the interrupt status of the request masked bit.
    return EUSCI_B_CMSIS(moduleInstance)->IFG & mask;
}

uint_fast16_t I2C_getEnabledInterruptStatus(uint32_t moduleInstance)
{
    return I2C_getInterruptStatus(moduleInstance,
            EUSCI_B_CMSIS(moduleInstance)->IE);
}

uint_fast16_t I2C_getMode(uint32_t moduleInstance)
{
    //Read the I2C mode.
    return (EUSCI_B_CMSIS(moduleInstance)->CTLW0 & EUSCI_B_CTLW0_TR);
}

void I2C_registerInterrupt(uint32_t moduleInstance, void (*intHandler)(void))
{
    switch (moduleInstance)
    {
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

void I2C_unregisterInterrupt(uint32_t moduleInstance)
{
    switch (moduleInstance)
    {
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

void I2C_slaveSendNAK(uint32_t moduleInstance)
{
    BITBAND_PERI(EUSCI_B_CMSIS(moduleInstance)->CTLW0, EUSCI_B_CTLW0_TXNACK_OFS) =
            1;
}
