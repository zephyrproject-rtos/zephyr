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

#ifndef __UART_IMX_H__
#define __UART_IMX_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup uart_imx_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Uart module initialization structure. */
typedef struct _uart_init_config
{
    uint32_t clockRate;  /*!< Current UART module clock freq. */
    uint32_t baudRate;   /*!< Desired UART baud rate. */
    uint32_t wordLength; /*!< Data bits in one frame. */
    uint32_t stopBitNum; /*!< Number of stop bits in one frame. */
    uint32_t parity;     /*!< Parity error check mode of this module. */
    uint32_t direction;  /*!< Data transfer direction of this module. */
} uart_init_config_t;

/*! @brief UART number of data bits in a character. */
enum _uart_word_length
{
    uartWordLength7Bits = 0x0,               /*!< One character has 7 bits. */
    uartWordLength8Bits = UART_UCR2_WS_MASK, /*!< One character has 8 bits. */
};

/*! @brief UART number of stop bits. */
enum _uart_stop_bit_num
{
    uartStopBitNumOne = 0x0,                 /*!< One bit Stop. */
    uartStopBitNumTwo = UART_UCR2_STPB_MASK, /*!< Two bits Stop. */
};

/*! @brief UART parity mode. */
enum _uart_partity_mode
{
    uartParityDisable = 0x0,                                       /*!< Parity error check disabled. */
    uartParityEven    = UART_UCR2_PREN_MASK,                       /*!< Even error check is selected. */
    uartParityOdd     = UART_UCR2_PREN_MASK | UART_UCR2_PROE_MASK, /*!< Odd error check is selected. */
};

/*! @brief Data transfer direction. */
enum _uart_direction_mode
{
    uartDirectionDisable = 0x0,                                       /*!< Both Tx and Rx are disabled. */
    uartDirectionTx      = UART_UCR2_TXEN_MASK,                       /*!< Tx is enabled. */
    uartDirectionRx      = UART_UCR2_RXEN_MASK,                       /*!< Rx is enabled. */
    uartDirectionTxRx    = UART_UCR2_TXEN_MASK | UART_UCR2_RXEN_MASK, /*!< Both Tx and Rx are enabled. */
};

/*! @brief This enumeration contains the settings for all of the UART interrupt configurations. */
enum _uart_interrupt
{
    uartIntAutoBaud            = 0x0080000F, /*!< Automatic baud rate detection Interrupt Enable. */
    uartIntTxReady             = 0x0080000D, /*!< transmitter ready Interrupt Enable. */
    uartIntIdle                = 0x0080000C, /*!< IDLE Interrupt Enable. */
    uartIntRxReady             = 0x00800009, /*!< Receiver Ready Interrupt Enable. */
    uartIntTxEmpty             = 0x00800006, /*!< Transmitter Empty Interrupt Enable. */
    uartIntRtsDelta            = 0x00800005, /*!< RTS Delta Interrupt Enable. */
    uartIntEscape              = 0x0084000F, /*!< Escape Sequence Interrupt Enable. */
    uartIntRts                 = 0x00840004, /*!< Request to Send Interrupt Enable. */
    uartIntAgingTimer          = 0x00840003, /*!< Aging Timer Interrupt Enable. */
    uartIntDtr                 = 0x0088000D, /*!< Data Terminal Ready Interrupt Enable. */
    uartIntParityError         = 0x0088000C, /*!< Parity Error Interrupt Enable.  */
    uartIntFrameError          = 0x0088000B, /*!< Frame Error Interrupt Enable. */
    uartIntDcd                 = 0x00880009, /*!< Data Carrier Detect Interrupt Enable. */
    uartIntRi                  = 0x00880008, /*!< Ring Indicator Interrupt Enable. */
    uartIntRxDs                = 0x00880006, /*!< Receive Status Interrupt Enable. */
    uartInttAirWake            = 0x00880005, /*!< Asynchronous IR WAKE Interrupt Enable. */
    uartIntAwake               = 0x00880004, /*!< Asynchronous WAKE Interrupt Enable. */
    uartIntDtrDelta            = 0x00880003, /*!< Data Terminal Ready Delta Interrupt Enable. */
    uartIntAutoBaudCnt         = 0x00880000, /*!< Autobaud Counter Interrupt Enable. */
    uartIntIr                  = 0x008C0008, /*!< Serial Infrared Interrupt Enable. */
    uartIntWake                = 0x008C0007, /*!< WAKE Interrupt Enable. */
    uartIntTxComplete          = 0x008C0003, /*!< TransmitComplete Interrupt Enable. */
    uartIntBreakDetect         = 0x008C0002, /*!< BREAK Condition Detected Interrupt Enable. */
    uartIntRxOverrun           = 0x008C0001, /*!< Receiver Overrun Interrupt Enable. */
    uartIntRxDataReady         = 0x008C0000, /*!< Receive Data Ready Interrupt Enable. */
    uartIntRs485SlaveAddrMatch = 0x00B80003, /*!< RS-485 Slave Address Detected Interrupt Enable. */
};

/*! @brief Flag for UART interrupt/DMA status check or polling status. */
enum _uart_status_flag
{
    uartStatusRxCharReady         = 0x0000000F, /*!< Rx Character Ready Flag. */
    uartStatusRxError             = 0x0000000E, /*!< Rx Error Detect Flag. */
    uartStatusRxOverrunError      = 0x0000000D, /*!< Rx Overrun Flag. */
    uartStatusRxFrameError        = 0x0000000C, /*!< Rx Frame Error Flag. */
    uartStatusRxBreakDetect       = 0x0000000B, /*!< Rx Break Detect Flag. */
    uartStatusRxParityError       = 0x0000000A, /*!< Rx Parity Error Flag. */
    uartStatusParityError         = 0x0094000F, /*!< Parity Error Interrupt Flag. */
    uartStatusRtsStatus           = 0x0094000E, /*!< RTS_B Pin Status Flag. */
    uartStatusTxReady             = 0x0094000D, /*!< Transmitter Ready Interrupt/DMA Flag. */
    uartStatusRtsDelta            = 0x0094000C, /*!< RTS Delta Flag. */
    uartStatusEscape              = 0x0094000B, /*!< Escape Sequence Interrupt Flag. */
    uartStatusFrameError          = 0x0094000A, /*!< Frame Error Interrupt Flag. */
    uartStatusRxReady             = 0x00940009, /*!< Receiver Ready Interrupt/DMA Flag. */
    uartStatusAgingTimer          = 0x00940008, /*!< Ageing Timer Interrupt Flag. */
    uartStatusDtrDelta            = 0x00940007, /*!< DTR Delta Flag. */
    uartStatusRxDs                = 0x00940006, /*!< Receiver IDLE Interrupt Flag. */
    uartStatustAirWake            = 0x00940005, /*!< Asynchronous IR WAKE Interrupt Flag. */
    uartStatusAwake               = 0x00940004, /*!< Asynchronous WAKE Interrupt Flag. */
    uartStatusRs485SlaveAddrMatch = 0x00940003, /*!< RS-485 Slave Address Detected Interrupt Flag. */
    uartStatusAutoBaud            = 0x0098000F, /*!< Automatic Baud Rate Detect Complete Flag. */
    uartStatusTxEmpty             = 0x0098000E, /*!< Transmit Buffer FIFO Empty. */
    uartStatusDtr                 = 0x0098000D, /*!< DTR edge triggered interrupt flag. */
    uartStatusIdle                = 0x0098000C, /*!< Idle Condition Flag. */
    uartStatusAutoBaudCntStop     = 0x0098000B, /*!< Autobaud Counter Stopped Flag. */
    uartStatusRiDelta             = 0x0098000A, /*!< Ring Indicator Delta Flag. */
    uartStatusRi                  = 0x00980009, /*!< Ring Indicator Input Flag. */
    uartStatusIr                  = 0x00980008, /*!< Serial Infrared Interrupt Flag. */
    uartStatusWake                = 0x00980007, /*!< Wake Flag. */
    uartStatusDcdDelta            = 0x00980006, /*!< Data Carrier Detect Delta Flag. */
    uartStatusDcd                 = 0x00980005, /*!< Data Carrier Detect Input Flag. */
    uartStatusRts                 = 0x00980004, /*!< RTS Edge Triggered Interrupt Flag. */
    uartStatusTxComplete          = 0x00980003, /*!< Transmitter Complete Flag. */
    uartStatusBreakDetect         = 0x00980002, /*!< BREAK Condition Detected Flag. */
    uartStatusRxOverrun           = 0x00980001, /*!< Overrun Error Flag. */
    uartStatusRxDataReady         = 0x00980000, /*!< Receive Data Ready Flag. */
};

/*! @brief The events generate the DMA Request. */
enum _uart_dma
{
    uartDmaRxReady    = 0x00800008, /*!< Receive Ready DMA Enable. */
    uartDmaTxReady    = 0x00800003, /*!< Transmitter Ready DMA Enable. */
    uartDmaAgingTimer = 0x00800002, /*!< Aging DMA Timer Enable. */
    uartDmaIdle       = 0x008C0006, /*!< DMA IDLE Condition Detected Interrupt Enable. */
};

/*! @brief RTS pin interrupt trigger edge. */
enum _uart_rts_int_trigger_edge
{
    uartRtsTriggerEdgeRising  = UART_UCR2_RTEC(0), /*!< RTS pin interrupt triggered on rising edge. */
    uartRtsTriggerEdgeFalling = UART_UCR2_RTEC(1), /*!< RTS pin interrupt triggered on falling edge. */
    uartRtsTriggerEdgeBoth    = UART_UCR2_RTEC(2), /*!< RTS pin interrupt triggered on both edge. */
};

/*! @brief UART module modem role selections. */
enum _uart_modem_mode
{
    uartModemModeDce = 0,                     /*!< UART module works as DCE. */
    uartModemModeDte = UART_UFCR_DCEDTE_MASK, /*!< UART module works as DTE. */
};

/*! @brief DTR pin interrupt trigger edge. */
enum _uart_dtr_int_trigger_edge
{
    uartDtrTriggerEdgeRising  = UART_UCR3_DPEC(0), /*!< DTR pin interrupt triggered on rising edge. */
    uartDtrTriggerEdgeFalling = UART_UCR3_DPEC(1), /*!< DTR pin interrupt triggered on falling edge. */
    uartDtrTriggerEdgeBoth    = UART_UCR3_DPEC(2), /*!< DTR pin interrupt triggered on both edge. */
};

/*! @brief IrDA vote clock selections. */
enum _uart_irda_vote_clock
{
    uartIrdaVoteClockSampling  = 0x0,                 /*!< The vote logic uses the sampling clock (16x baud rate) for normal operation. */
    uartIrdaVoteClockReference = UART_UCR4_IRSC_MASK, /*!< The vote logic uses the UART reference clock. */
};

/*! @brief UART module Rx Idle condition selections. */
enum _uart_rx_idle_condition
{
     uartRxIdleMoreThan4Frames  = UART_UCR1_ICD(0), /*!< Idle for more than 4 frames. */
     uartRxIdleMoreThan8Frames  = UART_UCR1_ICD(1), /*!< Idle for more than 8 frames. */
     uartRxIdleMoreThan16Frames = UART_UCR1_ICD(2), /*!< Idle for more than 16 frames. */
     uartRxIdleMoreThan32Frames = UART_UCR1_ICD(3), /*!< Idle for more than 32 frames. */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name UART Initialization and Configuration functions
 * @{
 */

/*!
 * @brief Initialize UART module with given initialization structure.
 *
 * @param base UART base pointer.
 * @param initConfig UART initialization structure (see @ref uart_init_config_t structure above).
 */
void UART_Init(UART_Type* base, const uart_init_config_t* initConfig);

/*!
 * @brief This function reset UART module register content to its default value.
 *
 * @param base UART base pointer.
 */
void UART_Deinit(UART_Type* base);

/*!
 * @brief This function is used to Enable the UART Module.
 *
 * @param base UART base pointer.
 */
static inline void UART_Enable(UART_Type* base)
{
    UART_UCR1_REG(base) |= UART_UCR1_UARTEN_MASK;
}

/*!
 * @brief This function is used to Disable the UART Module.
 *
 * @param base UART base pointer.
 */
static inline void UART_Disable(UART_Type* base)
{
    UART_UCR1_REG(base) &= ~UART_UCR1_UARTEN_MASK;
}

/*!
 * @brief This function is used to set the baud rate of UART Module.
 *
 * @param base UART base pointer.
 * @param clockRate UART module clock frequency.
 * @param baudRate Desired UART module baud rate.
 */
void UART_SetBaudRate(UART_Type* base, uint32_t clockRate, uint32_t baudRate);

/*!
 * @brief This function is used to set the transform direction of UART Module.
 *
 * @param base UART base pointer.
 * @param direction UART transfer direction (see @ref _uart_direction_mode enumeration).
 */
static inline void UART_SetDirMode(UART_Type* base, uint32_t direction)
{
    assert((direction & uartDirectionTx) || (direction & uartDirectionRx));

    UART_UCR2_REG(base) = (UART_UCR2_REG(base) & ~(UART_UCR2_RXEN_MASK | UART_UCR2_TXEN_MASK)) | direction;
}

/*!
 * @brief This function is used to set the number of frames RXD is allowed to
 *        be idle before an idle condition is reported. The available condition
 *        can be select from @ref _uart_idle_condition enumeration.
 *
 * @param base UART base pointer.
 * @param idleCondition The condition that an idle condition is reported
 *                      (see @ref _uart_idle_condition enumeration).
 */
static inline void UART_SetRxIdleCondition(UART_Type* base, uint32_t idleCondition)
{
    assert(idleCondition <= uartRxIdleMoreThan32Frames);

    UART_UCR1_REG(base) = (UART_UCR1_REG(base) & ~UART_UCR1_ICD_MASK) | idleCondition;
}

/*!
 * @brief This function is used to set the polarity of UART signal. The polarity
 *        of Tx and Rx can be set separately.
 *
 * @param base UART base pointer.
 * @param direction UART transfer direction (see @ref _uart_direction_mode enumeration).
 * @param invert Set true to invert the polarity of UART signal.
 */
void UART_SetInvertCmd(UART_Type* base, uint32_t direction, bool invert);

/*@}*/

/*!
 * @name Low Power Mode functions.
 * @{
 */

/*!
 * @brief This function is used to set UART enable condition in the DOZE state.
 *
 * @param base UART base pointer.
 * @param enable Enable/Disable UART module in doze mode.
 *               - true: Enable UART module in doze mode.
 *               - false: Disable UART module in doze mode.
 */
void UART_SetDozeMode(UART_Type* base, bool enable);

/*!
 * @brief This function is used to set UART enable condition of the UART low power feature.
 *
 * @param base UART base pointer.
 * @param enable Enable/Disable UART module low power feature.
 *               - true: Enable UART module low power feature.
 *               - false: Disable UART module low power feature.
 */
void UART_SetLowPowerMode(UART_Type* base, bool enable);

/*@}*/

/*!
 * @name Data transfer functions.
 * @{
 */

/*!
 * @brief This function is used to send data in RS-232 and IrDA Mode.
 *        A independent 9 Bits RS-485 send data function is provided.
 *
 * @param base UART base pointer.
 * @param data Data to be set through UART module.
 */
static inline void UART_Putchar(UART_Type* base, uint8_t data)
{
    UART_UTXD_REG(base) = (data & UART_UTXD_TX_DATA_MASK);
}

/*!
 * @brief This function is used to receive data in RS-232 and IrDA Mode.
 *        A independent 9 Bits RS-485 receive data function is provided.
 *
 * @param base UART base pointer.
 * @return The data received from UART module.
 */
static inline uint8_t UART_Getchar(UART_Type* base)
{
    return (uint8_t)(UART_URXD_REG(base) & UART_URXD_RX_DATA_MASK);
}

/*@}*/

/*!
 * @name Interrupt and Flag control functions.
 * @{
 */

/*!
 * @brief This function is used to set the enable condition of
 *        specific UART interrupt source. The available interrupt
 *        source can be select from @ref _uart_interrupt enumeration.
 *
 * @param base UART base pointer.
 * @param intSource Available interrupt source for this module.
 * @param enable Enable/Disable corresponding interrupt.
 *               - true: Enable corresponding interrupt.
 *               - false: Disable corresponding interrupt.
 */
void UART_SetIntCmd(UART_Type* base, uint32_t intSource, bool enable);

/*!
 * @brief This function is used to get the current status of specific
 *        UART status flag(including interrupt flag). The available
 *        status flag can be select from @ref _uart_status_flag enumeration.
 *
 * @param base UART base pointer.
 * @param flag Status flag to check.
 * @return current state of corresponding status flag.
 */
static inline bool UART_GetStatusFlag(UART_Type* base, uint32_t flag){
    volatile uint32_t* uart_reg = 0;

    uart_reg = (uint32_t *)((uint32_t)base + (flag >> 16));
    return (bool)((*uart_reg >> (flag & 0x0000FFFF)) & 0x1);
}

/*!
 * @brief This function is used to get the current status
 *        of specific UART status flag. The available status
 *        flag can be select from @ref _uart_status_flag enumeration.
 *
 * @param base UART base pointer.
 * @param flag Status flag to clear.
 */
void UART_ClearStatusFlag(UART_Type* base, uint32_t flag);

/*@}*/

/*!
 * @name DMA control functions.
 * @{
 */

/*!
 * @brief This function is used to set the enable condition of
 *        specific UART DMA source. The available DMA source
 *        can be select from @ref _uart_dma enumeration.
 *
 * @param base UART base pointer.
 * @param dmaSource The Event that can generate DMA request.
 * @param enable Enable/Disable corresponding DMA source.
 *               - true: Enable corresponding DMA source.
 *               - false: Disable corresponding DMA source.
 */
void UART_SetDmaCmd(UART_Type* base, uint32_t dmaSource, bool enable);

/*@}*/

/*!
 * @name FIFO control functions.
 * @{
 */

/*!
 * @brief This function is used to set the watermark of UART Tx FIFO.
 *        A maskable interrupt is generated whenever the data level in
 *        the TxFIFO falls below the Tx FIFO watermark.
 *
 * @param base UART base pointer.
 * @param watermark The Tx FIFO watermark.
 */
static inline void UART_SetTxFifoWatermark(UART_Type* base, uint8_t watermark)
{
    assert((watermark >= 2) && (watermark <= 32));
    UART_UFCR_REG(base) = (UART_UFCR_REG(base) & ~UART_UFCR_TXTL_MASK) | UART_UFCR_TXTL(watermark);
}

/*!
 * @brief This function is used to set the watermark of UART Rx FIFO.
 *        A maskable interrupt is generated whenever the data level in
 *        the RxFIFO reaches the Rx FIFO watermark.
 *
 * @param base UART base pointer.
 * @param watermark The Rx FIFO watermark.
 */
static inline void UART_SetRxFifoWatermark(UART_Type* base, uint8_t watermark)
{
    assert(watermark <= 32);
    UART_UFCR_REG(base) = (UART_UFCR_REG(base) & ~UART_UFCR_RXTL_MASK) | UART_UFCR_RXTL(watermark);
}

/*@}*/

/*!
 * @name Hardware Flow control and Modem Signal functions.
 * @{
 */

/*!
 * @brief This function is used to set the enable condition of RTS
 *        Hardware flow control.
 *
 * @param base UART base pointer.
 * @param enable Enable/Disbale RTS hardware flow control.
 *               - true: Enable RTS hardware flow control.
 *               - false: Disbale RTS hardware flow control.
 */
void UART_SetRtsFlowCtrlCmd(UART_Type* base, bool enable);

/*!
 * @brief This function is used to set the RTS interrupt trigger edge.
 *        The available trigger edge can be select from
 *        @ref _uart_rts_trigger_edge enumeration.
 *
 * @param base UART base pointer.
 * @param triggerEdge Available RTS pin interrupt trigger edge.
 */
static inline void UART_SetRtsIntTriggerEdge(UART_Type* base, uint32_t triggerEdge)
{
    assert((triggerEdge == uartRtsTriggerEdgeRising)  || \
           (triggerEdge == uartRtsTriggerEdgeFalling) || \
           (triggerEdge == uartRtsTriggerEdgeBoth));

    UART_UCR2_REG(base) = (UART_UCR2_REG(base) & ~UART_UCR2_RTEC_MASK) | triggerEdge;
}


/*!
 * @brief This function is used to set the enable condition of CTS
 *        auto control. if CTS control is enabled, the CTS_B pin
 *        is controlled by the receiver, otherwise the CTS_B pin is
 *        controlled by UART_CTSPinCtrl function.
 *
 * @param base UART base pointer.
 * @param enable Enable/Disable CTS auto control.
 *               - true: Enable CTS auto control.
 *               - false: Disable CTS auto control.
 */
void UART_SetCtsFlowCtrlCmd(UART_Type* base, bool enable);

/*!
 * @brief This function is used to control the CTS_B pin state when
 *        auto CTS control is disabled.
 *        The CTS_B pin is low(active)
 *        The CTS_B pin is high(inactive)
 *
 * @param base UART base pointer.
 * @param active The CTS_B pin state to set.
 *               - true: the CTS_B pin active;
 *               - false: the CTS_B pin inactive.
 */
void UART_SetCtsPinLevel(UART_Type* base, bool active);

/*!
 * @brief This function is used to set the auto CTS_B pin control
 *        trigger level. The CTS_B pin is de-asserted when
 *        Rx FIFO reach CTS trigger level.
 *
 * @param base UART base pointer.
 * @param triggerLevel Auto CTS_B pin control trigger level.
 */
static inline void UART_SetCtsTriggerLevel(UART_Type* base, uint8_t triggerLevel)
{
    assert(triggerLevel <= 32);
    UART_UCR4_REG(base) = (UART_UCR4_REG(base) & ~UART_UCR4_CTSTL_MASK) | UART_UCR4_CTSTL(triggerLevel);
}

/*!
 * @brief This function is used to set the role (DTE/DCE) of UART module
 *        in RS-232 communication.
 *
 * @param base UART base pointer.
 * @param mode The role(DTE/DCE) of UART module (see @ref _uart_modem_mode enumeration).
 */
void UART_SetModemMode(UART_Type* base, uint32_t mode);

/*!
 * @brief This function is used to set the edge of DTR_B (DCE) or
 *        DSR_B (DTE) on which an interrupt is generated.
 *
 * @param base UART base pointer.
 * @param triggerEdge The trigger edge on which an interrupt is generated
 *                    (see @ref _uart_dtr_trigger_edge enumeration above).
 */
static inline void UART_SetDtrIntTriggerEdge(UART_Type* base, uint32_t triggerEdge)
{
    assert((triggerEdge == uartDtrTriggerEdgeRising)  || \
           (triggerEdge == uartDtrTriggerEdgeFalling) || \
           (triggerEdge == uartDtrTriggerEdgeBoth));

    UART_UCR3_REG(base) = (UART_UCR3_REG(base) & ~UART_UCR3_DPEC_MASK) | triggerEdge;
}

/*!
 * @brief This function is used to set the pin state of DSR pin(for DCE mode)
 *        or DTR pin(for DTE mode) for the modem interface.
 *
 * @param base UART base pointer.
 * @param active The state of DSR pin.
 *               - true: DSR/DTR pin is logic one.
 *               - false: DSR/DTR pin is logic zero.
 */
void UART_SetDtrPinLevel(UART_Type* base, bool active);

/*!
 * @brief This function is used to set the pin state of
 *        DCD pin. THIS FUNCTION IS FOR DCE MODE ONLY.
 *
 * @param base UART base pointer.
 * @param active The state of DCD pin.
 *               - true: DCD_B pin is logic one (DCE mode)
 *               - false: DCD_B pin is logic zero (DCE mode)
 */
void UART_SetDcdPinLevel(UART_Type* base, bool active);

/*!
 * @brief This function is used to set the pin state of
 *        RI pin. THIS FUNCTION IS FOR DCE MODE ONLY.
 *
 * @param base UART base pointer.
 * @param active The state of RI pin.
 *               - true: RI_B pin is logic one (DCE mode)
 *               - false: RI_B pin is logic zero (DCE mode)
 */
void UART_SetRiPinLevel(UART_Type* base, bool active);

/*@}*/

/*!
 * @name Multiprocessor and RS-485 functions.
 * @{
 */

/*!
 * @brief This function is used to send 9 Bits length data in
 *        RS-485 Multidrop mode.
 *
 * @param base UART base pointer.
 * @param data Data(9 bits) to be set through UART module.
 */
void UART_Putchar9(UART_Type* base, uint16_t data);

/*!
 * @brief This functions is used to receive 9 Bits length data in
 *        RS-485 Multidrop mode.
 *
 * @param base UART base pointer.
 * @return The data(9 bits) received from UART module.
 */
uint16_t UART_Getchar9(UART_Type* base);

/*!
 * @brief This function is used to set the enable condition of
 *        9-Bits data or Multidrop mode.
 *
 * @param base UART base pointer.
 * @param enable Enable/Disable Multidrop mode.
 *               - true: Enable Multidrop mode.
 *               - false: Disable Multidrop mode.
 */
void UART_SetMultidropMode(UART_Type* base, bool enable);

/*!
 * @brief This function is used to set the enable condition of
 *        Automatic Address Detect Mode.
 *
 * @param base UART base pointer.
 * @param enable Enable/Disable Automatic Address Detect mode.
 *               - true: Enable Automatic Address Detect mode.
 *               - false: Disable Automatic Address Detect mode.
 */
void UART_SetSlaveAddressDetectCmd(UART_Type* base, bool enable);

/*!
 * @brief This function is used to set the slave address char
 *        that the receiver tries to detect.
 *
 * @param base UART base pointer.
 * @param slaveAddress The slave to detect.
 */
static inline void UART_SetSlaveAddress(UART_Type* base, uint8_t slaveAddress)
{
    UART_UMCR_REG(base) = (UART_UMCR_REG(base) & ~UART_UMCR_SLADDR_MASK) | \
                          UART_UMCR_SLADDR(slaveAddress);
}

/*@}*/

/*!
 * @name IrDA control functions.
 * @{
 */

/*!
 * @brief This function is used to set the enable condition of
 *        IrDA Mode.
 *
 * @param base UART base pointer.
 * @param enable Enable/Disable IrDA mode.
 *               - true: Enable IrDA mode.
 *               - false: Disable IrDA mode.
 */
void UART_SetIrDACmd(UART_Type* base, bool enable);

/*!
 * @brief This function is used to set the clock for the IR pulsed
 *        vote logic. The available clock can be select from
 *        @ref _uart_irda_vote_clock enumeration.
 *
 * @param base UART base pointer.
 * @param voteClock The available IrDA vote clock selection.
 */
void UART_SetIrDAVoteClock(UART_Type* base, uint32_t voteClock);

/*@}*/

/*!
 * @name Misc. functions.
 * @{
 */

/*!
 * @brief This function is used to set the enable condition of
 *        Automatic Baud Rate Detection feature.
 *
 * @param base UART base pointer.
 * @param enable Enable/Disable Automatic Baud Rate Detection feature.
 *               - true: Enable Automatic Baud Rate Detection feature.
 *               - false: Disable Automatic Baud Rate Detection feature.
 */
void UART_SetAutoBaudRateCmd(UART_Type* base, bool enable);

/*!
 * @brief This function is used to read the current value of Baud Rate
 *        Count Register value. this counter is used by Auto Baud Rate
 *        Detect feature.
 *
 * @param base UART base pointer.
 * @return Current Baud Rate Count Register value.
 */
static inline uint16_t UART_ReadBaudRateCount(UART_Type* base)
{
    return (uint16_t)(UART_UBRC_REG(base) & UART_UBRC_BCNT_MASK);
}

/*!
 * @brief This function is used to send BREAK character.It is
 *        important that SNDBRK is asserted high for a sufficient
 *        period of time to generate a valid BREAK.
 *
 * @param base UART base pointer.
 * @param active Asserted high to generate BREAK.
 *               - true: Generate BREAK character.
 *               - false: Stop generate BREAK character.
 */
void UART_SendBreakChar(UART_Type* base, bool active);

/*!
 * @brief This function is used to Enable/Disable the Escape
 *        Sequence Decection feature.
 *
 * @param base UART base pointer.
 * @param enable Enable/Disable Escape Sequence Decection.
 *               - true: Enable Escape Sequence Decection.
 *               - false: Disable Escape Sequence Decection.
 */
void UART_SetEscapeDecectCmd(UART_Type* base, bool enable);

/*!
 * @brief This function is used to set the enable condition of
 *        Escape Sequence Detection feature.
 *
 * @param base UART base pointer.
 * @param escapeChar The Escape Character to detect.
 */
static inline void UART_SetEscapeChar(UART_Type* base, uint8_t escapeChar)
{
    UART_UESC_REG(base) = (UART_UESC_REG(base) & ~UART_UESC_ESC_CHAR_MASK) | \
                          UART_UESC_ESC_CHAR(escapeChar);
}

/*!
 * @brief This function is used to set the maximum time interval (in ms)
 *                 allowed between escape characters.
 *
 * @param base UART base pointer.
 * @param timerInterval Maximum time interval allowed between escape characters.
 */
static inline void UART_SetEscapeTimerInterval(UART_Type* base, uint16_t timerInterval)
{
    assert(timerInterval <= 0xFFF);
    UART_UTIM_REG(base) = (UART_UTIM_REG(base) & ~UART_UTIM_TIM_MASK) | \
                          UART_UTIM_TIM(timerInterval);
}

/*@}*/

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* __UART_IMX_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
