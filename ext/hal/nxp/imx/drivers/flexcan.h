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

#ifndef __FLEXCAN_H__
#define __FLEXCAN_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "device_imx.h"

/* Start of section using anonymous unions. */
#if defined(__ARMCC_VERSION)
  #pragma push
  #pragma anon_unions
#elif defined(__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined(__IAR_SYSTEMS_ICC__)
  #pragma language=extended
#else
  #error Not supported compiler type
#endif

/*!
 * @addtogroup flexcan_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief FlexCAN message buffer CODE for Rx buffers. */
enum _flexcan_msgbuf_code_rx
{
    flexcanRxInactive = 0x0, /*!< MB is not active. */
    flexcanRxFull     = 0x2, /*!< MB is full. */
    flexcanRxEmpty    = 0x4, /*!< MB is active and empty. */
    flexcanRxOverrun  = 0x6, /*!< MB is overwritten into a full buffer. */
    flexcanRxBusy     = 0x8, /*!< FlexCAN is updating the contents of the MB. */
                             /*!  The CPU must not access the MB. */
    flexcanRxRanswer  = 0xA, /*!< A frame was configured to recognize a Remote Request Frame */
                             /*!  and transmit a Response Frame in return. */
    flexcanRxNotUsed  = 0xF, /*!< Not used. */
};

/*! @brief FlexCAN message buffer CODE FOR Tx buffers. */
enum _flexcan_msgbuf_code_tx
{
    flexcanTxInactive    = 0x8, /*!< MB is not active. */
    flexcanTxAbort       = 0x9, /*!< MB is aborted. */
    flexcanTxDataOrRemte = 0xC, /*!< MB is a TX Data Frame(when MB RTR = 0) or */
                                /*!< MB is a TX Remote Request Frame (when MB RTR = 1). */
    flexcanTxTanswer     = 0xE, /*!< MB is a TX Response Request Frame from. */
                                /*!  an incoming Remote Request Frame. */
    flexcanTxNotUsed     = 0xF, /*!< Not used. */
};

/*! @brief FlexCAN operation modes. */
enum _flexcan_operatining_modes
{
    flexcanNormalMode     = 0x1, /*!< Normal mode or user mode @internal gui name="Normal". */
    flexcanListenOnlyMode = 0x2, /*!< Listen-only mode @internal gui name="Listen-only". */
    flexcanLoopBackMode   = 0x4, /*!< Loop-back mode @internal gui name="Loop back". */
};

/*! @brief FlexCAN RX mask mode. */
enum _flexcan_rx_mask_mode
{
    flexcanRxMaskGlobal     = 0x0, /*!< Rx global mask. */
    flexcanRxMaskIndividual = 0x1, /*!< Rx individual mask. */
};

/*! @brief The ID type used in rx matching process. */
enum _flexcan_rx_mask_id_type
{
    flexcanRxMaskIdStd = 0x0, /*!< Standard ID. */
    flexcanRxMaskIdExt = 0x1, /*!< Extended ID. */
};

/*! @brief FlexCAN error interrupt source enumeration. */
enum _flexcan_interrutpt
{
    flexcanIntRxWarning = 0x01, /*!< Tx Warning interrupt source. */
    flexcanIntTxWarning = 0x02, /*!< Tx Warning interrupt source. */
    flexcanIntWakeUp    = 0x04, /*!< Wake Up interrupt source. */
    flexcanIntBusOff    = 0x08, /*!< Bus Off interrupt source.  */
    flexcanIntError     = 0x10, /*!< Error interrupt source. */
};

/*! @brief FlexCAN error interrupt flags. */
enum _flexcan_status_flag
{
    flexcanStatusSynch        = CAN_ESR1_SYNCH_MASK,    /*!< Bus Synchronized flag. */
    flexcanStatusTxWarningInt = CAN_ESR1_TWRN_INT_MASK, /*!< Tx Warning initerrupt flag. */
    flexcanStatusRxWarningInt = CAN_ESR1_RWRN_INT_MASK, /*!< Tx Warning initerrupt flag. */
    flexcanStatusBit1Err      = CAN_ESR1_BIT1_ERR_MASK, /*!< Bit0 Error flag. */
    flexcanStatusBit0Err      = CAN_ESR1_BIT0_ERR_MASK, /*!< Bit1 Error flag. */
    flexcanStatusAckErr       = CAN_ESR1_ACK_ERR_MASK,  /*!< Ack Error flag. */
    flexcanStatusCrcErr       = CAN_ESR1_CRC_ERR_MASK,  /*!< CRC Error flag. */
    flexcanStatusFrameErr     = CAN_ESR1_FRM_ERR_MASK,  /*!< Frame Error flag. */
    flexcanStatusStuffingErr  = CAN_ESR1_STF_ERR_MASK,  /*!< Stuffing Error flag. */
    flexcanStatusTxWarning    = CAN_ESR1_TX_WRN_MASK,   /*!< Tx Warning flag. */
    flexcanStatusRxWarning    = CAN_ESR1_RX_WRN_MASK,   /*!< Rx Warning flag. */
    flexcanStatusIdle         = CAN_ESR1_IDLE_MASK,     /*!< FlexCAN Idle flag. */
    flexcanStatusTransmitting = CAN_ESR1_TX_MASK,       /*!< Trasmitting flag. */
    flexcanStatusFltConf      = CAN_ESR1_FLT_CONF_MASK, /*!< Fault Config flag. */
    flexcanStatusReceiving    = CAN_ESR1_RX_MASK,       /*!< Receiving flag. */
    flexcanStatusBusOff       = CAN_ESR1_BOFF_INT_MASK, /*!< Bus Off interrupt flag. */
    flexcanStatusError        = CAN_ESR1_ERR_INT_MASK,  /*!< Error interrupt flag. */
    flexcanStatusWake         = CAN_ESR1_WAK_INT_MASK,  /*!< Wake Up interrupt flag. */
};

/*! @brief The id filter element type selection. */
enum _flexcan_rx_fifo_id_element_format
{
    flexcanRxFifoIdElementFormatA = 0x0, /*!< One full ID (standard and extended) per ID Filter Table element. */
    flexcanRxFifoIdElementFormatB = 0x1, /*!< Two full standard IDs or two partial 14-bit (standard and extended) IDs per ID Filter Table element. */
    flexcanRxFifoIdElementFormatC = 0x2, /*!< Four partial 8-bit Standard IDs per ID Filter Table element. */
    flexcanRxFifoIdElementFormatD = 0x3, /*!< All frames rejected. */
};

/*! @brief FlexCAN Rx FIFO filters number. */
enum _flexcan_rx_fifo_filter_id_number
{
    flexcanRxFifoIdFilterNum8   = 0x0, /*!<   8 Rx FIFO Filters. @internal gui name="8 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum16  = 0x1, /*!<  16 Rx FIFO Filters. @internal gui name="16 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum24  = 0x2, /*!<  24 Rx FIFO Filters. @internal gui name="24 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum32  = 0x3, /*!<  32 Rx FIFO Filters. @internal gui name="32 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum40  = 0x4, /*!<  40 Rx FIFO Filters. @internal gui name="40 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum48  = 0x5, /*!<  48 Rx FIFO Filters. @internal gui name="48 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum56  = 0x6, /*!<  56 Rx FIFO Filters. @internal gui name="56 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum64  = 0x7, /*!<  64 Rx FIFO Filters. @internal gui name="64 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum72  = 0x8, /*!<  72 Rx FIFO Filters. @internal gui name="72 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum80  = 0x9, /*!<  80 Rx FIFO Filters. @internal gui name="80 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum88  = 0xA, /*!<  88 Rx FIFO Filters. @internal gui name="88 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum96  = 0xB, /*!<  96 Rx FIFO Filters. @internal gui name="96 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum104 = 0xC, /*!< 104 Rx FIFO Filters. @internal gui name="104 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum112 = 0xD, /*!< 112 Rx FIFO Filters. @internal gui name="112 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum120 = 0xE, /*!< 120 Rx FIFO Filters. @internal gui name="120 Rx FIFO Filters" */
    flexcanRxFifoIdFilterNum128 = 0xF, /*!< 128 Rx FIFO Filters. @internal gui name="128 Rx FIFO Filters" */
};

/*! @brief FlexCAN RX FIFO ID filter table structure. */
typedef struct _flexcan_id_table
{
    uint32_t *idFilter;   /*!< Rx FIFO ID filter elements. */
    bool isRemoteFrame;   /*!< Remote frame. */
    bool isExtendedFrame; /*!< Extended frame. */
} flexcan_id_table_t;

/*! @brief FlexCAN message buffer structure. */
typedef struct _flexcan_msgbuf
{
    union
    {
        uint32_t cs; /*!< Code and Status. */
        struct
        {
            uint32_t timeStamp : 16;
            uint32_t dlc       : 4;
            uint32_t rtr       : 1;
            uint32_t ide       : 1;
            uint32_t srr       : 1;
            uint32_t reserved1 : 1;
            uint32_t code      : 4;
            uint32_t reserved2 : 4;
        };
    };

    union
    {
        uint32_t id; /*!< Message Buffer ID. */
        struct
        {
            uint32_t idExt     : 18;
            uint32_t idStd     : 11;
            uint32_t prio      : 3;
        };
    };

    union
    {
        uint32_t word0; /*!< Bytes of the FlexCAN message. */
        struct
        {
            uint8_t data3;
            uint8_t data2;
            uint8_t data1;
            uint8_t data0;
        };
    };

    union
    {
        uint32_t word1; /*!< Bytes of the FlexCAN message. */
        struct
        {
            uint8_t data7;
            uint8_t data6;
            uint8_t data5;
            uint8_t data4;
        };
    };
} flexcan_msgbuf_t;

/*! @brief FlexCAN timing-related structures. */
typedef struct _flexcan_timing
{
    uint32_t preDiv;     /*!< Clock pre divider. */
    uint32_t rJumpwidth; /*!< Resync jump width. */
    uint32_t phaseSeg1;  /*!< Phase segment 1. */
    uint32_t phaseSeg2;  /*!< Phase segment 2. */
    uint32_t propSeg;    /*!< Propagation segment. */
} flexcan_timing_t;

/*! @brief FlexCAN module initialization structure. */
typedef struct _flexcan_init_config
{
    flexcan_timing_t timing;        /*!< Desired FlexCAN module timing configuration. */
    uint32_t         operatingMode; /*!< Desired FlexCAN module operating mode. */
    uint8_t          maxMsgBufNum;  /*!< The maximal number of available message buffer. */
} flexcan_init_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name FlexCAN Initialization and Configuration functions
 * @{
 */

/*!
 * @brief Initialize FlexCAN module with given initialization structure.
 *
 * @param base CAN base pointer.
 * @param initConfig CAN initialization structure (see @ref flexcan_init_config_t structure).
 */
void FLEXCAN_Init(CAN_Type* base, const flexcan_init_config_t* initConfig);

/*!
 * @brief This function reset FlexCAN module register content to its default value.
 *
 * @param base FlexCAN base pointer.
 */
void FLEXCAN_Deinit(CAN_Type* base);

/*!
 * @brief This function is used to Enable the FlexCAN Module.
 *
 * @param base FlexCAN base pointer.
 */
void FLEXCAN_Enable(CAN_Type* base);

/*!
 * @brief This function is used to Disable the FlexCAN Module.
 *
 * @param base FlexCAN base pointer.
 */
void FLEXCAN_Disable(CAN_Type* base);

/*!
 * @brief Sets the FlexCAN time segments for setting up bit rate.
 *
 * @param base FlexCAN base pointer.
 * @param timing FlexCAN time segments, which need to be set for the bit rate (See @ref flexcan_timing_t structure).
 */
void FLEXCAN_SetTiming(CAN_Type* base, const flexcan_timing_t* timing);

/*!
 * @brief Set operation mode.
 *
 * @param base FlexCAN base pointer.
 * @param mode Set an operation mode.
 */
void FLEXCAN_SetOperatingMode(CAN_Type* base, uint8_t mode);

/*!
 * @brief Set the maximum number of Message Buffers.
 *
 * @param base FlexCAN base pointer.
 * @param bufNum Maximum number of message buffers.
 */
void FLEXCAN_SetMaxMsgBufNum(CAN_Type* base, uint32_t bufNum);

/*!
 * @brief Get the working status of FlexCAN module.
 *
 * @param base FlexCAN base pointer.
 * @return - true: FLEXCAN module is either in Normal Mode, Listen-Only Mode or Loop-Back Mode.
 *         - false: FLEXCAN module is either in Disable Mode, Stop Mode or Freeze Mode.
 */
static inline bool FLEXCAN_IsModuleReady(CAN_Type* base)
{
    return !((CAN_MCR_REG(base) >> CAN_MCR_NOT_RDY_SHIFT) & 0x1);
}

/*!
 * @brief Set the Transmit Abort feature enablement.
 *
 * @param base FlexCAN base pointer.
 * @param enable Enable/Disable Transmit Abort feature.
 *               - true: Enable Transmit Abort feature.
 *               - false: Disable Transmit Abort feature.
 */
void FLEXCAN_SetAbortCmd(CAN_Type* base, bool enable);

/*!
 * @brief Set the local transmit priority enablement.
 *
 * @param base FlexCAN base pointer.
 * @param enable Enable/Disable local transmit periority.
 *               - true: Transmit MB with highest local priority.
 *               - false: Transmit MB with lowest MB number.
 */
void FLEXCAN_SetLocalPrioCmd(CAN_Type* base, bool enable);

/*!
 * @brief Set the Rx matching process priority.
 *
 * @param base FlexCAN base pointer.
 * @param priority Set Rx matching process priority.
 *                 - true: Matching starts from Mailboxes and continues on Rx FIFO.
 *                 - false: Matching starts from Rx FIFO and continues on Mailboxes.
 */
void FLEXCAN_SetMatchPrioCmd(CAN_Type* base, bool priority);

/*@}*/

/*!
 * @name FlexCAN Message buffer control functions
 * @{
 */

/*!
 * @brief Get message buffer pointer for transition.
 *
 * @param base FlexCAN base pointer.
 * @param msgBufIdx message buffer index.
 * @return message buffer pointer.
 */
flexcan_msgbuf_t* FLEXCAN_GetMsgBufPtr(CAN_Type* base, uint8_t msgBufIdx);

/*!
 * @brief Locks the FlexCAN Rx message buffer.
 *
 * @param base FlexCAN base pointer.
 * @param msgBufIdx Index of the message buffer
 * @return - true: Lock Rx Message Buffer successful. 
 *         - false: Lock Rx Message Buffer failed.
 */
bool FLEXCAN_LockRxMsgBuf(CAN_Type* base, uint8_t msgBufIdx);

/*!
 * @brief Unlocks the FlexCAN Rx message buffer.
 *
 * @param base FlexCAN base pointer.
 * @return current free run timer counter value.
 */
uint16_t FLEXCAN_UnlockAllRxMsgBuf(CAN_Type* base);

/*@}*/

/*!
 * @name FlexCAN Interrupts and flags management functions
 * @{
 */

/*!
 * @brief Enables/Disables the FlexCAN Message Buffer interrupt.
 *
 * @param base FlexCAN base pointer.
 * @param msgBufIdx Index of the message buffer.
 * @param enable Enables/Disables interrupt.
 *               - true: Enable Message Buffer interrupt.
 *               - disable: Disable Message Buffer interrupt.
 */
void FLEXCAN_SetMsgBufIntCmd(CAN_Type* base, uint8_t msgBufIdx, bool enable);

/*!
 * @brief Gets the individual FlexCAN MB interrupt flag.
 *
 * @param base FlexCAN base pointer.
 * @param msgBufIdx Index of the message buffer.
 * @retval true: Message Buffer Interrupt is pending.
 * @retval false: There is no Message Buffer Interrupt.
 */
bool FLEXCAN_GetMsgBufStatusFlag(CAN_Type* base, uint8_t msgBufIdx);

/*!
 * @brief Clears the interrupt flag of the message buffers.
 *
 * @param base FlexCAN base pointer.
 * @param msgBufIdx Index of the message buffer.
 */
void FLEXCAN_ClearMsgBufStatusFlag(CAN_Type* base, uint32_t msgBufIdx);

/*!
 * @brief Enables error interrupt of the FlexCAN module.
 *
 * @param base FlexCAN base pointer.
 * @param errorSrc The interrupt source (see @ref _flexcan_interrutpt enumeration).
 * @param enable Choose enable or disable.
 */
void FLEXCAN_SetErrIntCmd(CAN_Type* base, uint32_t errorSrc, bool enable);

/*!
 * @brief Gets the FlexCAN module interrupt flag.
 *
 * @param base FlexCAN base pointer.
 * @param errFlags FlexCAN error flags (see @ref _flexcan_status_flag enumeration).
 * @return The individual Message Buffer interrupt flag (0 and 1 are the flag value)
 */
uint32_t FLEXCAN_GetErrStatusFlag(CAN_Type* base, uint32_t errFlags);

/*!
 * @brief Clears the interrupt flag of the FlexCAN module.
 *
 * @param base FlexCAN base pointer.
 * @param errFlags The value to be written to the interrupt flag1 register (see @ref _flexcan_status_flag enumeration).
 */
void FLEXCAN_ClearErrStatusFlag(CAN_Type* base, uint32_t errFlags);

/*!
 * @brief Get the error counter of FlexCAN module.
 *
 * @param base FlexCAN base pointer.
 * @param txError Tx_Err_Counter pointer.
 * @param rxError Rx_Err_Counter pointer.
 */
void FLEXCAN_GetErrCounter(CAN_Type* base, uint8_t* txError, uint8_t* rxError);

/*@}*/

/*!
 * @name Rx FIFO management functions
 * @{
 */

/*!
 * @brief Enables the Rx FIFO.
 *
 * @param base FlexCAN base pointer.
 * @param numOfFilters The number of Rx FIFO filters
 */
void FLEXCAN_EnableRxFifo(CAN_Type* base, uint8_t numOfFilters);

/*!
 * @brief Disables the Rx FIFO.
 *
 * @param base FlexCAN base pointer.
 */
void FLEXCAN_DisableRxFifo(CAN_Type* base);

/*!
 * @brief Set the number of the Rx FIFO filters.
 *
 * @param base FlexCAN base pointer.
 * @param numOfFilters The number of Rx FIFO filters.
 */
void FLEXCAN_SetRxFifoFilterNum(CAN_Type* base, uint32_t numOfFilters);

/*!
 * @brief Set the FlexCAN Rx FIFO fields.
 *
 * @param base FlexCAN base pointer.
 * @param idFormat The format of the Rx FIFO ID Filter Table Elements
 * @param idFilterTable The ID filter table elements which contain RTR bit, IDE bit and RX message ID.
 */
void FLEXCAN_SetRxFifoFilter(CAN_Type* base, uint32_t idFormat, flexcan_id_table_t *idFilterTable);

/*!
 * @brief Gets the FlexCAN Rx FIFO data pointer.
 *
 * @param base FlexCAN base pointer.
 * @return Rx FIFO data pointer.
 */
flexcan_msgbuf_t* FLEXCAN_GetRxFifoPtr(CAN_Type* base);

/*!
 * @brief Gets the FlexCAN Rx FIFO information.
 *        The return value indicates which Identifier Acceptance Filter
 *        (see Rx FIFO Structure) was hit by the received message.
 * @param base FlexCAN base pointer.
 * @return Rx FIFO filter number.
 */
uint16_t FLEXCAN_GetRxFifoInfo(CAN_Type* base);

/*@}*/

/*!
 * @name Rx Mask Setting functions
 * @{
 */

/*!
 * @brief Set the Rx masking mode.
 *
 * @param base FlexCAN base pointer.
 * @param mode The FlexCAN Rx mask mode (see @ref _flexcan_rx_mask_mode enumeration).
 */
void FLEXCAN_SetRxMaskMode(CAN_Type* base, uint32_t mode);

/*!
 * @brief Set the remote trasmit request mask enablement.
 *
 * @param base FlexCAN base pointer.
 * @param enable Enable/Disable remote trasmit request mask.
 *               - true: Enable RTR matching judgement.
 *               - false: Disable RTR matching judgement.
 */
void FLEXCAN_SetRxMaskRtrCmd(CAN_Type* base, bool enable);

/*!
 * @brief Set the FlexCAN RX global mask.
 *
 * @param base FlexCAN base pointer.
 * @param mask Rx Global mask.
 */
void FLEXCAN_SetRxGlobalMask(CAN_Type* base, uint32_t mask);

/*!
 * @brief Set the FlexCAN Rx individual mask for ID filtering in the Rx MBs and the Rx FIFO.
 *
 * @param base FlexCAN base pointer.
 * @param msgBufIdx Index of the message buffer.
 * @param mask Individual mask
 */
void FLEXCAN_SetRxIndividualMask(CAN_Type* base, uint32_t msgBufIdx, uint32_t mask);

/*!
 * @brief Set the FlexCAN RX Message Buffer BUF14 mask.
 *
 * @param base FlexCAN base pointer.
 * @param mask Message Buffer BUF14 mask.
 */
void FLEXCAN_SetRxMsgBuff14Mask(CAN_Type* base, uint32_t mask);

/*!
 * @brief Set the FlexCAN RX Message Buffer BUF15 mask.
 *
 * @param base FlexCAN base pointer.
 * @param mask Message Buffer BUF15 mask.
 */
void FLEXCAN_SetRxMsgBuff15Mask(CAN_Type* base, uint32_t mask);

/*!
 * @brief Set the FlexCAN RX Fifo global mask.
 *
 * @param base FlexCAN base pointer.
 * @param mask Rx Fifo Global mask.
 */
void FLEXCAN_SetRxFifoGlobalMask(CAN_Type* base, uint32_t mask);

/*@}*/

/*!
 * @name Misc. Functions
 * @{
 */

/*!
 * @brief Enable/disable the FlexCAN self wakeup feature.
 *
 * @param base FlexCAN base pointer.
 * @param lpfEnable The low pass filter for Rx self wakeup feature enablement.
 * @param enable The self wakeup feature enablement.
 */
void FLEXCAN_SetSelfWakeUpCmd(CAN_Type* base, bool lpfEnable, bool enable);

/*!
 * @brief Enable/Disable the FlexCAN self reception feature.
 *
 * @param base FlexCAN base pointer.
 * @param enable Enable/Disable self reception feature.
 *               - true: Enable self reception feature.
 *               - false: Disable self reception feature.
 */
void FLEXCAN_SetSelfReceptionCmd(CAN_Type* base, bool enable);

/*!
 * @brief Enable/disable the enhance FlexCAN Rx vote.
 *
 * @param base FlexCAN base pointer.
 * @param enable Enable/Disable FlexCAN Rx vote mechanism
 *               - true: Three samples are used to determine the value of the received bit.
 *               - false: Just one sample is used to determine the bit value.
 */
void FLEXCAN_SetRxVoteCmd(CAN_Type* base, bool enable);

/*!
 * @brief Enable/disable the Auto Busoff recover feature.
 *
 * @param base FlexCAN base pointer.
 * @param enable Enable/Disable Auto Busoff Recover
 *               - true: Enable Auto Bus Off recover feature.
 *               - false: Disable Auto Bus Off recover feature.
 */
void FLEXCAN_SetAutoBusOffRecoverCmd(CAN_Type* base, bool enable);

/*!
 * @brief Enable/disable the Time Sync feature.
 *
 * @param base FlexCAN base pointer.
 * @param enable Enable/Disable the Time Sync
 *               - true: Enable Time Sync feature.
 *               - false: Disable Time Sync feature.
 */
void FLEXCAN_SetTimeSyncCmd(CAN_Type* base, bool enable);

/*!
 * @brief Enable/disable the Auto Remote Response feature.
 *
 * @param base FlexCAN base pointer.
 * @param enable Enable/Disable the Auto Remote Response feature
 *               - true: Enable Auto Remote Response feature.
 *               - false: Disable Auto Remote Response feature.
 */
void FLEXCAN_SetAutoRemoteResponseCmd(CAN_Type* base, bool enable);

/*!
 * @brief Enable/disable the Glitch Filter Width when FLEXCAN enters the STOP mode.
 *
 * @param base FlexCAN base pointer.
 * @param filterWidth The Glitch Filter Width.
 */
static inline void FLEXCAN_SetGlitchFilterWidth(CAN_Type* base, uint8_t filterWidth)
{
    CAN_GFWR_REG(base) = filterWidth;
}

/*!
 * @brief Get the lowest inactive message buffer number.
 *
 * @param base FlexCAN base pointer.
 * @return bit 22-16 : The lowest number inactive Mailbox.
 *         bit 14    : Indicates whether the number content is valid or not.
 *         bit 13    : This bit indicates whether there is any inactive Mailbox.
 */
static inline uint32_t FLEXCAN_GetLowestInactiveMsgBuf(CAN_Type* base)
{
    return CAN_ESR2_REG(base);
}

/*!
 * @brief Set the Tx Arbitration Start Delay number.
 *        This function is used to optimize the transmit performance.
 *        For more information about to set this value, see the Chip Reference Manual.
 *
 * @param base FlexCAN base pointer.
 * @param tasd The lowest number inactive Mailbox.
 */
static inline void FLEXCAN_SetTxArbitrationStartDelay(CAN_Type* base, uint8_t tasd)
{
    assert(tasd < 32);
    CAN_CTRL2_REG(base) = (CAN_CTRL2_REG(base) & ~CAN_CTRL2_TASD_MASK) | CAN_CTRL2_TASD(tasd);
}

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#if defined(__ARMCC_VERSION)
  #pragma pop
#elif defined(__GNUC__)
  /* leave anonymous unions enabled */
#elif defined(__IAR_SYSTEMS_ICC__)
  #pragma language=default
#else
  #error Not supported compiler type
#endif

#endif /* __FLEXCAN_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
