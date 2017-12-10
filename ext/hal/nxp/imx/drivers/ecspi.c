/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ecspi.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*******************************************************************************
 * eCSPI Initialization and Configuration functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : ECSPI_Init
 * Description   : Initializes the eCSPI module according to the specified
 *                 parameters in the initConfig.
 *
 *END**************************************************************************/
void ECSPI_Init(ECSPI_Type* base, const ecspi_init_config_t* initConfig)
{
    /* Disable eCSPI module */
    ECSPI_CONREG_REG(base) = 0;

    /* Enable the eCSPI module before write to other registers */
    ECSPI_Enable(base);

    /* eCSPI CONREG Configuration */
    ECSPI_CONREG_REG(base) |= ECSPI_CONREG_BURST_LENGTH(initConfig->burstLength) |
                              ECSPI_CONREG_CHANNEL_SELECT(initConfig->channelSelect);
    ECSPI_CONREG_REG(base) |= initConfig->ecspiAutoStart ? ECSPI_CONREG_SMC_MASK : 0;

    /* eCSPI CONFIGREG Configuration */
    ECSPI_CONFIGREG_REG(base) = ECSPI_CONFIGREG_SCLK_PHA(((initConfig->clockPhase) & 1) << (initConfig->channelSelect)) |
                                ECSPI_CONFIGREG_SCLK_POL(((initConfig->clockPolarity) & 1) << (initConfig->channelSelect));

    /* Master or Slave mode Configuration */
    if(initConfig->mode == ecspiMasterMode)
    {
        /* Set baud rate in bits per second */
        ECSPI_CONREG_REG(base) |= ECSPI_CONREG_CHANNEL_MODE(1 << (initConfig->channelSelect));
        ECSPI_SetBaudRate(base, initConfig->clockRate, initConfig->baudRate);
    }
    else
        ECSPI_CONREG_REG(base) &= ~ECSPI_CONREG_CHANNEL_MODE(1 << (initConfig->channelSelect));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ECSPI_SetSampClockSource
 * Description   : Configure the clock source for the sample period counter.
 *
 *END**************************************************************************/
void ECSPI_SetSampClockSource(ECSPI_Type* base, uint32_t source)
{
    /* Select the clock source */
    if(source == ecspiSclk)
        ECSPI_PERIODREG_REG(base) &= ~ECSPI_PERIODREG_CSRC_MASK;
    else
        ECSPI_PERIODREG_REG(base) |= ECSPI_PERIODREG_CSRC_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ECSPI_SetBaudRate
 * Description   : Calculated the eCSPI baud rate in bits per second.
 *
 *END**************************************************************************/
uint32_t ECSPI_SetBaudRate(ECSPI_Type* base, uint32_t sourceClockInHz, uint32_t bitsPerSec)
{
    uint32_t div, pre_div;
    uint32_t post_baud;         /* baud rate after post divider */
    uint32_t pre_baud;          /* baud rate before pre divider */

    if(sourceClockInHz <= bitsPerSec)
    {
        ECSPI_CONREG_REG(base) &= ~ECSPI_CONREG_PRE_DIVIDER_MASK;
        ECSPI_CONREG_REG(base) &= ~ECSPI_CONREG_POST_DIVIDER_MASK;
        return sourceClockInHz;
    }

    div = sourceClockInHz / bitsPerSec;
    if(div < 16)    /* pre_divider is enough */
    {
        if((sourceClockInHz - bitsPerSec * div) < ((bitsPerSec * (div + 1)) - sourceClockInHz))
            pre_div = div - 1;   /* pre_divider value is one less than the real divider */
        else
            pre_div = div;
        ECSPI_CONREG_REG(base) = (ECSPI_CONREG_REG(base) & (~ECSPI_CONREG_PRE_DIVIDER_MASK)) |
                                 ECSPI_CONREG_PRE_DIVIDER(pre_div);
        ECSPI_CONREG_REG(base) = (ECSPI_CONREG_REG(base) & (~ECSPI_CONREG_POST_DIVIDER_MASK)) |
                                 ECSPI_CONREG_POST_DIVIDER(0);
        return sourceClockInHz / (pre_div + 1);
    }

    pre_baud = bitsPerSec * 16;
    for(div = 1; div < 16; div++)
    {
        post_baud = sourceClockInHz >> div;
        if(post_baud < pre_baud)
        break;
    }

    if(div == 16)     /* divider is not enough, set the biggest ones */
    {
        ECSPI_CONREG_REG(base) |= ECSPI_CONREG_PRE_DIVIDER(15);
        ECSPI_CONREG_REG(base) |= ECSPI_CONREG_POST_DIVIDER(15);
        return post_baud / 16;
    }

    /* find the closed one */
    if((post_baud - bitsPerSec * (post_baud / bitsPerSec)) < ((bitsPerSec * ((post_baud / bitsPerSec) + 1)) - post_baud))
        pre_div = post_baud / bitsPerSec - 1;
    else
        pre_div = post_baud / bitsPerSec;
    ECSPI_CONREG_REG(base) = (ECSPI_CONREG_REG(base) & (~ECSPI_CONREG_PRE_DIVIDER_MASK)) |
                             ECSPI_CONREG_PRE_DIVIDER(pre_div);
    ECSPI_CONREG_REG(base) = (ECSPI_CONREG_REG(base) & (~ECSPI_CONREG_POST_DIVIDER_MASK)) |
                             ECSPI_CONREG_POST_DIVIDER(div);
    return post_baud / (pre_div + 1);
}

/*******************************************************************************
 * DMA management functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : ECSPI_SetDMACmd
 * Description   : Enable or disable the specified DMA Source.
 *
 *END**************************************************************************/
void ECSPI_SetDMACmd(ECSPI_Type* base, uint32_t source, bool enable)
{
    /* Configure the DAM source */
    if(enable)
        ECSPI_DMAREG_REG(base) |= ((uint32_t)(1 << source));
    else
        ECSPI_DMAREG_REG(base) &= ~((uint32_t)(1 << source));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : ECSPI_SetFIFOThreshold
 * Description   : Set the RXFIFO or TXFIFO threshold.
 *
 *END**************************************************************************/
void ECSPI_SetFIFOThreshold(ECSPI_Type* base, uint32_t fifo, uint32_t threshold)
{
    /* configure the RXFIFO and TXFIFO threshold that can triggers a DMA/INT request */
    if(fifo == ecspiTxfifoThreshold)
        ECSPI_DMAREG_REG(base) = (ECSPI_DMAREG_REG(base) & (~ECSPI_DMAREG_TX_THRESHOLD_MASK)) |
                                 ECSPI_DMAREG_TX_THRESHOLD(threshold);
    else
        ECSPI_DMAREG_REG(base) = (ECSPI_DMAREG_REG(base) & (~ECSPI_DMAREG_RX_THRESHOLD_MASK)) |
                                 ECSPI_DMAREG_RX_THRESHOLD(threshold);
}

/*******************************************************************************
 * Interrupts and flags management functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : ECSPI_SetIntCmd
 * Description   : Enable or disable eCSPI interrupts.
 *
 *END**************************************************************************/
void ECSPI_SetIntCmd(ECSPI_Type* base, uint32_t flags, bool enable)
{
    /* Configure the Interrupt source */
    if(enable)
        ECSPI_INTREG_REG(base) |= flags;
    else
        ECSPI_INTREG_REG(base) &= ~flags;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
