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

#include "uart_imx.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*******************************************************************************
 * Initialization and Configuration functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : UART_Init
 * Description   : This function initializes the module according to uart
 *                 initialize structure.
 *
 *END**************************************************************************/
void UART_Init(UART_Type* base, const uart_init_config_t* initConfig)
{
    assert(initConfig);

    /* Disable UART Module. */
    UART_UCR1_REG(base) &= ~UART_UCR1_UARTEN_MASK;

    /* Reset UART register to its default value. */
    UART_Deinit(base);

    /* Set UART data word length, stop bit count, parity mode and communication
     * direction according to uart init struct, disable RTS hardware flow
     * control. */
    UART_UCR2_REG(base) |= (initConfig->wordLength |
                            initConfig->stopBitNum |
                            initConfig->parity     |
                            initConfig->direction  |
                            UART_UCR2_IRTS_MASK);

    /* For imx family device, UARTs are used in MUXED mode,
     * so that this bit should always be set.*/
    UART_UCR3_REG(base) |= UART_UCR3_RXDMUXSEL_MASK;

    /* Set BaudRate according to uart initialize struct. */
    /* Baud Rate = Ref Freq / (16 * (UBMR + 1)/(UBIR+1)) */
    UART_SetBaudRate(base, initConfig->clockRate, initConfig->baudRate);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_Deinit
 * Description   : This function reset Uart module register content to its
 *                 default value.
 *
 *END**************************************************************************/
void UART_Deinit(UART_Type* base)
{
    /* Disable UART Module */
    UART_UCR1_REG(base) &= ~UART_UCR1_UARTEN_MASK;

    /* Reset UART Module Register content to default value */
    UART_UCR1_REG(base)  = 0x0;
    UART_UCR2_REG(base)  = UART_UCR2_SRST_MASK;
    UART_UCR3_REG(base)  = UART_UCR3_DSR_MASK |
                           UART_UCR3_DCD_MASK |
                           UART_UCR3_RI_MASK;
    UART_UCR4_REG(base)  = UART_UCR4_CTSTL(32);
    UART_UFCR_REG(base)  = UART_UFCR_TXTL(2) | UART_UFCR_RXTL(1);
    UART_UESC_REG(base)  = UART_UESC_ESC_CHAR(0x2B);
    UART_UTIM_REG(base)  = 0x0;
    UART_ONEMS_REG(base) = 0x0;
    UART_UTS_REG(base)   = UART_UTS_TXEMPTY_MASK | UART_UTS_RXEMPTY_MASK;
    UART_UMCR_REG(base)  = 0x0;

    /* Reset the transmit and receive state machines, all FIFOs and register
     * USR1, USR2, UBIR, UBMR, UBRC, URXD, UTXD and UTS[6-3]. */
    UART_UCR2_REG(base) &= ~UART_UCR2_SRST_MASK;
    while (!(UART_UCR2_REG(base) & UART_UCR2_SRST_MASK));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetBaudRate
 * Description   :
 *
 *END**************************************************************************/
void UART_SetBaudRate(UART_Type* base, uint32_t clockRate, uint32_t baudRate)
{
    uint32_t numerator;
    uint32_t denominator;
    uint32_t divisor;
    uint32_t refFreqDiv;
    uint32_t divider = 1;

    /* get the approximately maximum divisor */
    numerator = clockRate;
    denominator = baudRate << 4;
    divisor = 1;

    while (denominator != 0)
    {
        divisor = denominator;
        denominator = numerator % denominator;
        numerator = divisor;
    }

    numerator = clockRate / divisor;
    denominator = (baudRate << 4) / divisor;

    /* numerator ranges from 1 ~ 7 * 64k */
    /* denominator ranges from 1 ~ 64k */
    if ((numerator > (UART_UBIR_INC_MASK * 7)) ||
        (denominator > UART_UBIR_INC_MASK))
    {
        uint32_t m = (numerator - 1) / (UART_UBIR_INC_MASK * 7) + 1;
        uint32_t n = (denominator - 1) / UART_UBIR_INC_MASK + 1;
        uint32_t max = m > n ? m : n;
        numerator /= max;
        denominator /= max;
        if (0 == numerator)
            numerator = 1;
        if (0 == denominator)
            denominator = 1;
    }
    divider = (numerator - 1) / UART_UBIR_INC_MASK + 1;

    switch (divider)
    {
        case 1:
            refFreqDiv = 0x05;
            break;
        case 2:
            refFreqDiv = 0x04;
            break;
        case 3:
            refFreqDiv = 0x03;
            break;
        case 4:
            refFreqDiv = 0x02;
            break;
        case 5:
            refFreqDiv = 0x01;
            break;
        case 6:
            refFreqDiv = 0x00;
            break;
        case 7:
            refFreqDiv = 0x06;
            break;
        default:
        refFreqDiv = 0x05;
    }

    UART_UFCR_REG(base) &= ~UART_UFCR_RFDIV_MASK;
    UART_UFCR_REG(base) |= UART_UFCR_RFDIV(refFreqDiv);
    UART_UBIR_REG(base) = UART_UBIR_INC(denominator - 1);
    UART_UBMR_REG(base) = UART_UBMR_MOD(numerator / divider - 1);
    UART_ONEMS_REG(base) = UART_ONEMS_ONEMS(clockRate/(1000 * divider));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetInvertCmd
 * Description   : This function is used to set the polarity of UART signal.
 *                 The polarity of Tx and Rx can be set separately.
 *
 *END**************************************************************************/
void UART_SetInvertCmd(UART_Type* base, uint32_t direction, bool invert)
{
    assert((direction & uartDirectionTx) || (direction & uartDirectionRx));

    if (invert)
    {
        if (direction & UART_UCR2_RXEN_MASK)
            UART_UCR4_REG(base) |= UART_UCR4_INVR_MASK;
        if (direction & UART_UCR2_TXEN_MASK)
            UART_UCR3_REG(base) |= UART_UCR3_INVT_MASK;
    }
    else
    {
        if (direction & UART_UCR2_RXEN_MASK)
            UART_UCR4_REG(base) &= ~UART_UCR4_INVR_MASK;
        if (direction & UART_UCR2_TXEN_MASK)
            UART_UCR3_REG(base) &= ~UART_UCR3_INVT_MASK;
    }
}

/*******************************************************************************
 * Low Power Mode functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetDozeMode
 * Description   : This function is used to set UART enable condition in the
 *                 DOZE state.
 *
 *END**************************************************************************/
void UART_SetDozeMode(UART_Type* base, bool enable)
{
    if (enable)
        UART_UCR1_REG(base) &= UART_UCR1_DOZE_MASK;
    else
        UART_UCR1_REG(base) |= ~UART_UCR1_DOZE_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetLowPowerMode
 * Description   : This function is used to set UART enable condition of the
 *                 UART low power feature.
 *
 *END**************************************************************************/
void UART_SetLowPowerMode(UART_Type* base, bool enable)
{
    if (enable)
        UART_UCR4_REG(base) &= ~UART_UCR4_LPBYP_MASK;
    else
        UART_UCR4_REG(base) |= UART_UCR4_LPBYP_MASK;
}

/*******************************************************************************
 * Interrupt and Flag control functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetIntCmd
 * Description   : This function is used to set the enable condition of
 *                 specific UART interrupt source. The available interrupt
 *                 source can be select from uart_int_source enumeration.
 *
 *END**************************************************************************/
void UART_SetIntCmd(UART_Type* base, uint32_t intSource, bool enable)
{
    volatile uint32_t* uart_reg = 0;
    uint32_t uart_mask = 0;

    uart_reg = (uint32_t *)((uint32_t)base + (intSource >> 16));
    uart_mask = (1 << (intSource & 0x0000FFFF));

    if (enable)
        *uart_reg |= uart_mask;
    else
        *uart_reg &= ~uart_mask;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_GetStatusFlag
 * Description   : This function is used to get the current status of specific
 *                 UART status flag. The available status flag can be select
 *                 from uart_status_flag & uart_interrupt_flag enumeration.
 *
 *END**************************************************************************/
/*
bool UART_GetStatusFlag(UART_Type* base, uint32_t flag)
{
    volatile uint32_t* uart_reg = 0;

    uart_reg = (uint32_t *)((uint32_t)base + (flag >> 16));
    return (bool)((*uart_reg >> (flag & 0x0000FFFF)) & 0x1);
}
*/

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_ClearStatusFlag
 * Description   : This function is used to get the current status
 *                 of specific UART status flag. The available status
 *                 flag can be select from uart_status_flag &
 *                 uart_interrupt_flag enumeration.
 *
 *END**************************************************************************/
void UART_ClearStatusFlag(UART_Type* base, uint32_t flag)
{
    volatile uint32_t* uart_reg = 0;
    uint32_t uart_mask = 0;

    uart_reg = (uint32_t *)((uint32_t)base + (flag >> 16));
    uart_mask = (1 << (flag & 0x0000FFFF));

    /* write 1 to clear. */
    *uart_reg = uart_mask;
}

/*******************************************************************************
 * DMA control functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetDmaCmd
 * Description   : This function is used to set the enable condition of
 *                 specific UART DMA source. The available DMA
 *                 source can be select from uart_dma_source enumeration.
 *
 *END**************************************************************************/
void UART_SetDmaCmd(UART_Type* base, uint32_t dmaSource, bool enable)
{
    volatile uint32_t* uart_reg = 0;
    uint32_t uart_mask = 0;

    uart_reg = (uint32_t *)((uint32_t)base + (dmaSource >> 16));
    uart_mask = (1 << (dmaSource & 0x0000FFFF));
    if (enable)
        *uart_reg |= uart_mask;
    else
        *uart_reg &= ~uart_mask;
}

/*******************************************************************************
 * Hardware Flow control and Modem Signal functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetRtsFlowCtrlCmd
 * Description   : This function is used to set the enable condition of RTS
 *                 Hardware flow control.
 *
 *END**************************************************************************/
void UART_SetRtsFlowCtrlCmd(UART_Type* base, bool enable)
{
    if (enable)
        UART_UCR2_REG(base) &= ~UART_UCR2_IRTS_MASK;
    else
        UART_UCR2_REG(base) |= UART_UCR2_IRTS_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetCtsFlowCtrlCmd
 * Description   : This function is used to set the enable condition of CTS
 *                 auto control. if CTS control is enabled, the CTS_B pin will
 *                 be controlled by the receiver, otherwise the CTS_B pin will
 *                 controlled by UART_CTSPinCtrl function.
 *
 *END**************************************************************************/
void UART_SetCtsFlowCtrlCmd(UART_Type* base, bool enable)
{
    if (enable)
        UART_UCR2_REG(base) |= UART_UCR2_CTSC_MASK;
    else
        UART_UCR2_REG(base) &= ~UART_UCR2_CTSC_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetCtsPinLevel
 * Description   : This function is used to control the CTS_B pin state when
 *                 auto CTS control is disabled.
 *                 The CTS_B pin is low (active)
 *                 The CTS_B pin is high (inactive)
 *
 *END**************************************************************************/
void UART_SetCtsPinLevel(UART_Type* base, bool active)
{
    if (active)
        UART_UCR2_REG(base) |= UART_UCR2_CTS_MASK;
    else
        UART_UCR2_REG(base) &= ~UART_UCR2_CTS_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetModemMode
 * Description   : This function is used to set the role(DTE/DCE) of UART module
 *                 in RS-232 communication.
 *
 *END**************************************************************************/
void UART_SetModemMode(UART_Type* base, uint32_t mode)
{
    assert((mode == uartModemModeDce) || (mode == uartModemModeDte));

    if (uartModemModeDce == mode)
        UART_UFCR_REG(base) &= ~UART_UFCR_DCEDTE_MASK;
    else
        UART_UFCR_REG(base) |= UART_UFCR_DCEDTE_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetDtrPinLevel
 * Description   : This function is used to set the pin state of
 *                 DSR pin(for DCE mode) or DTR pin(for DTE mode) for the
 *                 modem interface.
 *
 *END**************************************************************************/
void UART_SetDtrPinLevel(UART_Type* base, bool active)
{
    if (active)
        UART_UCR3_REG(base) |= UART_UCR3_DSR_MASK;
    else
        UART_UCR3_REG(base) &= ~UART_UCR3_DSR_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetDcdPinLevel
 * Description   : This function is used to set the pin state of
 *                 DCD pin. THIS FUNCTION IS FOR DCE MODE ONLY.
 *
 *END**************************************************************************/
void UART_SetDcdPinLevel(UART_Type* base, bool active)
{
    if (active)
        UART_UCR3_REG(base) |= UART_UCR3_DCD_MASK;
    else
        UART_UCR3_REG(base) &= ~UART_UCR3_DCD_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetRiPinLevel
 * Description   : This function is used to set the pin state of
 *                 RI pin. THIS FUNCTION IS FOR DCE MODE ONLY.
 *
 *END**************************************************************************/
void UART_SetRiPinLevel(UART_Type* base, bool active)
{
    if (active)
        UART_UCR3_REG(base) |= UART_UCR3_RI_MASK;
    else
        UART_UCR3_REG(base) &= ~UART_UCR3_RI_MASK;
}

/*******************************************************************************
 * Multiprocessor and RS-485 functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : UART_Putchar9
 * Description   : This function is used to send 9 Bits length data in
 *                 RS-485 Multidrop mode.
 *
 *END**************************************************************************/
void UART_Putchar9(UART_Type* base, uint16_t data)
{
    assert(data <= 0x1FF);

    if (data & 0x0100)
        UART_UMCR_REG(base) |= UART_UMCR_TXB8_MASK;
    else
        UART_UMCR_REG(base) &= ~UART_UMCR_TXB8_MASK;
    UART_UTXD_REG(base) = (data & UART_UTXD_TX_DATA_MASK);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_Getchar9
 * Description   : This functions is used to receive 9 Bits length data in
 *                 RS-485 Multidrop mode.
 *
 *END**************************************************************************/
uint16_t UART_Getchar9(UART_Type* base)
{
    uint16_t rxData = UART_URXD_REG(base);

    if (rxData & UART_URXD_PRERR_MASK)
    {
        rxData = (rxData & 0x00FF) | 0x0100;
    }
    else
    {
        rxData &= 0x00FF;
    }

    return rxData;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetMultidropMode
 * Description   : This function is used to set the enable condition of
 *                 9-Bits data or Multidrop mode.
 *
 *END**************************************************************************/
void UART_SetMultidropMode(UART_Type* base, bool enable)
{
    if (enable)
        UART_UMCR_REG(base) |= UART_UMCR_MDEN_MASK;
    else
        UART_UMCR_REG(base) &= ~UART_UMCR_MDEN_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetSlaveAddressDetectCmd
 * Description   : This function is used to set the enable condition of
 *                 Automatic Address Detect Mode.
 *
 *END**************************************************************************/
void UART_SetSlaveAddressDetectCmd(UART_Type* base, bool enable)
{
    if (enable)
        UART_UMCR_REG(base) |= UART_UMCR_SLAM_MASK;
    else
        UART_UMCR_REG(base) &= ~UART_UMCR_SLAM_MASK;
}

/*******************************************************************************
 * IrDA control functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetIrDACmd
 * Description   : This function is used to set the enable condition of
 *                 IrDA Mode.
 *
 *END**************************************************************************/
void UART_SetIrDACmd(UART_Type* base, bool enable)
{
    if (enable)
        UART_UCR1_REG(base) |= UART_UCR1_IREN_MASK;
    else
        UART_UCR1_REG(base) &= ~UART_UCR1_IREN_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetIrDAVoteClock
 * Description   : This function is used to set the clock for the IR pulsed
 *                 vote logic. The available clock can be select from
 *                 uart_irda_vote_clock enumeration.
 *
 *END**************************************************************************/
void UART_SetIrDAVoteClock(UART_Type* base, uint32_t voteClock)
{
    assert((voteClock == uartIrdaVoteClockSampling) || \
           (voteClock == uartIrdaVoteClockReference));

    if (uartIrdaVoteClockSampling == voteClock)
        UART_UCR4_REG(base) |= UART_UCR4_IRSC_MASK;
    else
        UART_UCR4_REG(base) &= ~UART_UCR4_IRSC_MASK;
}

/*******************************************************************************
 * Misc. functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetAutoBaudRateCmd
 * Description   : This function is used to set the enable condition of
 *                 Automatic Baud Rate Detection feature.
 *
 *END**************************************************************************/
void UART_SetAutoBaudRateCmd(UART_Type* base, bool enable)
{
    if (enable)
        UART_UCR1_REG(base) |= UART_UCR1_ADBR_MASK;
    else
        UART_UCR1_REG(base) &= ~UART_UCR1_ADBR_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SendBreakChar
 * Description   : This function is used to send BREAK character.It is
 *                 important that SNDBRK is asserted high for a sufficient
 *                 period of time to generate a valid BREAK.
 *
 *END**************************************************************************/
void UART_SendBreakChar(UART_Type* base, bool active)
{
    if (active)
        UART_UCR1_REG(base) |= UART_UCR1_SNDBRK_MASK;
    else
        UART_UCR1_REG(base) &= ~UART_UCR1_SNDBRK_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_SetEscapeDecectCmd
 * Description   : This function is used to set the enable condition of
 *                 Escape Sequence Detection feature.
 *
 *END**************************************************************************/
void UART_SetEscapeDecectCmd(UART_Type* base, bool enable)
{
    if (enable)
        UART_UCR2_REG(base) |= UART_UCR2_ESCEN_MASK;
    else
        UART_UCR2_REG(base) &= ~UART_UCR2_ESCEN_MASK;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
