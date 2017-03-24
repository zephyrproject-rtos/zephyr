/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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
 * o Neither the name of the copyright holder nor the names of its
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
#ifndef _FSL_ENET_H_
#define _FSL_ENET_H_

#include "fsl_common.h"

/*!
 * @addtogroup enet
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief Defines the driver version. */
#define FSL_ENET_DRIVER_VERSION (MAKE_VERSION(2, 1, 1)) /*!< Version 2.1.1. */
/*@}*/

/*! @name Control and status region bit masks of the receive buffer descriptor. */
/*@{*/
#define ENET_BUFFDESCRIPTOR_RX_EMPTY_MASK 0x8000U       /*!< Empty bit mask. */
#define ENET_BUFFDESCRIPTOR_RX_SOFTOWNER1_MASK 0x4000U  /*!< Software owner one mask. */
#define ENET_BUFFDESCRIPTOR_RX_WRAP_MASK 0x2000U        /*!< Next buffer descriptor is the start address. */
#define ENET_BUFFDESCRIPTOR_RX_SOFTOWNER2_Mask 0x1000U  /*!< Software owner two mask. */
#define ENET_BUFFDESCRIPTOR_RX_LAST_MASK 0x0800U        /*!< Last BD of the frame mask. */
#define ENET_BUFFDESCRIPTOR_RX_MISS_MASK 0x0100U        /*!< Received because of the promiscuous mode. */
#define ENET_BUFFDESCRIPTOR_RX_BROADCAST_MASK 0x0080U   /*!< Broadcast packet mask. */
#define ENET_BUFFDESCRIPTOR_RX_MULTICAST_MASK 0x0040U   /*!< Multicast packet mask. */
#define ENET_BUFFDESCRIPTOR_RX_LENVLIOLATE_MASK 0x0020U /*!< Length violation mask. */
#define ENET_BUFFDESCRIPTOR_RX_NOOCTET_MASK 0x0010U     /*!< Non-octet aligned frame mask. */
#define ENET_BUFFDESCRIPTOR_RX_CRC_MASK 0x0004U         /*!< CRC error mask. */
#define ENET_BUFFDESCRIPTOR_RX_OVERRUN_MASK 0x0002U     /*!< FIFO overrun mask. */
#define ENET_BUFFDESCRIPTOR_RX_TRUNC_MASK 0x0001U       /*!< Frame is truncated mask. */
/*@}*/

/*! @name Control and status bit masks of the transmit buffer descriptor. */
/*@{*/
#define ENET_BUFFDESCRIPTOR_TX_READY_MASK 0x8000U       /*!< Ready bit mask. */
#define ENET_BUFFDESCRIPTOR_TX_SOFTOWENER1_MASK 0x4000U /*!< Software owner one mask. */
#define ENET_BUFFDESCRIPTOR_TX_WRAP_MASK 0x2000U        /*!< Wrap buffer descriptor mask. */
#define ENET_BUFFDESCRIPTOR_TX_SOFTOWENER2_MASK 0x1000U /*!< Software owner two mask. */
#define ENET_BUFFDESCRIPTOR_TX_LAST_MASK 0x0800U        /*!< Last BD of the frame mask. */
#define ENET_BUFFDESCRIPTOR_TX_TRANMITCRC_MASK 0x0400U  /*!< Transmit CRC mask. */
/*@}*/

/* Extended control regions for enhanced buffer descriptors. */
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
/*! @name First extended control region bit masks of the receive buffer descriptor. */
/*@{*/
#define ENET_BUFFDESCRIPTOR_RX_IPV4_MASK 0x0001U             /*!< Ipv4 frame mask. */
#define ENET_BUFFDESCRIPTOR_RX_IPV6_MASK 0x0002U             /*!< Ipv6 frame mask. */
#define ENET_BUFFDESCRIPTOR_RX_VLAN_MASK 0x0004U             /*!< VLAN frame mask. */
#define ENET_BUFFDESCRIPTOR_RX_PROTOCOLCHECKSUM_MASK 0x0010U /*!< Protocol checksum error mask. */
#define ENET_BUFFDESCRIPTOR_RX_IPHEADCHECKSUM_MASK 0x0020U   /*!< IP header checksum error mask. */
/*@}*/

/*! @name Second extended control region bit masks of the receive buffer descriptor. */
/*@{*/
#define ENET_BUFFDESCRIPTOR_RX_INTERRUPT_MASK 0x0080U /*!< BD interrupt mask. */
#define ENET_BUFFDESCRIPTOR_RX_UNICAST_MASK 0x0100U   /*!< Unicast frame mask. */
#define ENET_BUFFDESCRIPTOR_RX_COLLISION_MASK 0x0200U /*!< BD collision mask. */
#define ENET_BUFFDESCRIPTOR_RX_PHYERR_MASK 0x0400U    /*!< PHY error mask. */
#define ENET_BUFFDESCRIPTOR_RX_MACERR_MASK 0x8000U    /*!< Mac error mask. */
/*@}*/

/*! @name First extended control region bit masks of the transmit buffer descriptor. */
/*@{*/
#define ENET_BUFFDESCRIPTOR_TX_ERR_MASK 0x8000U              /*!< Transmit error mask. */
#define ENET_BUFFDESCRIPTOR_TX_UNDERFLOWERR_MASK 0x2000U     /*!< Underflow error mask. */
#define ENET_BUFFDESCRIPTOR_TX_EXCCOLLISIONERR_MASK 0x1000U  /*!< Excess collision error mask. */
#define ENET_BUFFDESCRIPTOR_TX_FRAMEERR_MASK 0x0800U         /*!< Frame error mask. */
#define ENET_BUFFDESCRIPTOR_TX_LATECOLLISIONERR_MASK 0x0400U /*!< Late collision error mask. */
#define ENET_BUFFDESCRIPTOR_TX_OVERFLOWERR_MASK 0x0200U      /*!< Overflow error mask. */
#define ENET_BUFFDESCRIPTOR_TX_TIMESTAMPERR_MASK 0x0100U     /*!< Timestamp error mask. */
/*@}*/

/*! @name Second extended control region bit masks of the transmit buffer descriptor. */
/*@{*/
#define ENET_BUFFDESCRIPTOR_TX_INTERRUPT_MASK 0x4000U /*!< Interrupt mask. */
#define ENET_BUFFDESCRIPTOR_TX_TIMESTAMP_MASK 0x2000U /*!< Timestamp flag mask. */
/*@}*/
#endif                                                /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

/*! @brief Defines the receive error status flag mask. */
#define ENET_BUFFDESCRIPTOR_RX_ERR_MASK                                        \
    (ENET_BUFFDESCRIPTOR_RX_TRUNC_MASK | ENET_BUFFDESCRIPTOR_RX_OVERRUN_MASK | \
     ENET_BUFFDESCRIPTOR_RX_LENVLIOLATE_MASK | ENET_BUFFDESCRIPTOR_RX_NOOCTET_MASK | ENET_BUFFDESCRIPTOR_RX_CRC_MASK)
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
#define ENET_BUFFDESCRIPTOR_RX_EXT_ERR_MASK \
    (ENET_BUFFDESCRIPTOR_RX_MACERR_MASK | ENET_BUFFDESCRIPTOR_RX_PHYERR_MASK | ENET_BUFFDESCRIPTOR_RX_COLLISION_MASK)
#endif
#define ENET_TX_INTERRUPT (kENET_TxFrameInterrupt | kENET_TxBufferInterrupt)
#define ENET_RX_INTERRUPT (kENET_RxFrameInterrupt | kENET_RxBufferInterrupt)
#define ENET_TS_INTERRUPT (kENET_TsTimerInterrupt | kENET_TsAvailInterrupt)
#define ENET_ERR_INTERRUPT (kENET_BabrInterrupt | kENET_BabtInterrupt | kENET_EBusERInterrupt | \
    kENET_LateCollisionInterrupt | kENET_RetryLimitInterrupt | kENET_UnderrunInterrupt | kENET_PayloadRxInterrupt)


/*! @name Defines the maximum Ethernet frame size. */
/*@{*/
#define ENET_FRAME_MAX_FRAMELEN 1518U     /*!< Default maximum Ethernet frame size. */
/*@}*/

#define ENET_FIFO_MIN_RX_FULL 5U /*!< ENET minimum receive FIFO full. */
#define ENET_RX_MIN_BUFFERSIZE 256U /*!< ENET minimum buffer size. */

/*! @brief Defines the PHY address scope for the ENET. */
#define ENET_PHY_MAXADDRESS (ENET_MMFR_PA_MASK >> ENET_MMFR_PA_SHIFT)

/*! @brief Defines the status return codes for transaction. */
enum _enet_status
{
    kStatus_ENET_RxFrameError = MAKE_STATUS(kStatusGroup_ENET, 0U), /*!< A frame received but data error happen. */
    kStatus_ENET_RxFrameFail = MAKE_STATUS(kStatusGroup_ENET, 1U),  /*!< Failed to receive a frame. */
    kStatus_ENET_RxFrameEmpty = MAKE_STATUS(kStatusGroup_ENET, 2U), /*!< No frame arrive. */
    kStatus_ENET_TxFrameBusy =
        MAKE_STATUS(kStatusGroup_ENET, 3U),                       /*!< Transmit buffer descriptors are under process. */
    kStatus_ENET_TxFrameFail = MAKE_STATUS(kStatusGroup_ENET, 4U) /*!< Transmit frame fail. */
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
    ,
    kStatus_ENET_PtpTsRingFull = MAKE_STATUS(kStatusGroup_ENET, 5U), /*!< Timestamp ring full. */
    kStatus_ENET_PtpTsRingEmpty = MAKE_STATUS(kStatusGroup_ENET, 6U) /*!< Timestamp ring empty. */
#endif                                                               /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
};

/*! @brief Defines the RMII or MII mode for data interface between the MAC and the PHY. */
typedef enum _enet_mii_mode
{
    kENET_MiiMode = 0U, /*!< MII mode for data interface. */
    kENET_RmiiMode      /*!< RMII mode for data interface. */
} enet_mii_mode_t;

/*! @brief Defines the 10 Mbps or 100 Mbps speed for the  MII data interface. */
typedef enum _enet_mii_speed
{
    kENET_MiiSpeed10M = 0U, /*!< Speed 10 Mbps. */
    kENET_MiiSpeed100M      /*!< Speed 100 Mbps. */
} enet_mii_speed_t;

/*! @brief Defines the half or full duplex for the MII data interface. */
typedef enum _enet_mii_duplex
{
    kENET_MiiHalfDuplex = 0U, /*!< Half duplex mode. */
    kENET_MiiFullDuplex       /*!< Full duplex mode. */
} enet_mii_duplex_t;

/*! @brief Defines the write operation for the MII management frame. */
typedef enum _enet_mii_write
{
    kENET_MiiWriteNoCompliant = 0U, /*!< Write frame operation, but not MII-compliant. */
    kENET_MiiWriteValidFrame        /*!< Write frame operation for a valid MII management frame. */
} enet_mii_write_t;

/*! @brief Defines the read operation for the MII management frame. */
typedef enum _enet_mii_read
{
    kENET_MiiReadValidFrame = 2U, /*!< Read frame operation for a valid MII management frame. */
    kENET_MiiReadNoCompliant = 3U /*!< Read frame operation, but not MII-compliant. */
} enet_mii_read_t;

#if defined (FSL_FEATURE_ENET_HAS_EXTEND_MDIO) && FSL_FEATURE_ENET_HAS_EXTEND_MDIO
/*! @brief Define the MII opcode for extended MDIO_CLAUSES_45 Frame. */
typedef enum _enet_mii_extend_opcode {
    kENET_MiiAddrWrite_C45 = 0U,  /*!< Address Write operation. */
    kENET_MiiWriteFrame_C45 = 1U, /*!< Write frame operation for a valid MII management frame. */
    kENET_MiiReadFrame_C45 = 3U   /*!< Read frame operation for a valid MII management frame. */
} enet_mii_extend_opcode;
#endif  /* FSL_FEATURE_ENET_HAS_EXTEND_MDIO */

/*! @brief Defines a special configuration for ENET MAC controller.
 *
 * These control flags are provided for special user requirements.
 * Normally, these control flags are unused for ENET initialization.
 * For special requirements, set the flags to
 * macSpecialConfig in the enet_config_t.
 * The kENET_ControlStoreAndFwdDisable is used to disable the FIFO store
 * and forward. FIFO store and forward means that the FIFO read/send is started
 * when a complete frame is stored in TX/RX FIFO. If this flag is set,
 * configure rxFifoFullThreshold and txFifoWatermark
 * in the enet_config_t.
 */
typedef enum _enet_special_control_flag
{
    kENET_ControlFlowControlEnable = 0x0001U,       /*!< Enable ENET flow control: pause frame. */
    kENET_ControlRxPayloadCheckEnable = 0x0002U,    /*!< Enable ENET receive payload length check. */
    kENET_ControlRxPadRemoveEnable = 0x0004U,       /*!< Padding is removed from received frames. */
    kENET_ControlRxBroadCastRejectEnable = 0x0008U, /*!< Enable broadcast frame reject. */
    kENET_ControlMacAddrInsert = 0x0010U,           /*!< Enable MAC address insert. */
    kENET_ControlStoreAndFwdDisable = 0x0020U,      /*!< Enable FIFO store and forward. */
    kENET_ControlSMIPreambleDisable = 0x0040U,      /*!< Enable SMI preamble. */
    kENET_ControlPromiscuousEnable = 0x0080U,       /*!< Enable promiscuous mode. */
    kENET_ControlMIILoopEnable = 0x0100U,           /*!< Enable ENET MII loop back. */
    kENET_ControlVLANTagEnable = 0x0200U            /*!< Enable VLAN tag frame. */
} enet_special_control_flag_t;

/*! @brief List of interrupts supported by the peripheral. This
 * enumeration uses one-bot encoding to allow a logical OR of multiple
 * members. Members usually map to interrupt enable bits in one or more
 * peripheral registers.
 */
typedef enum _enet_interrupt_enable
{
    kENET_BabrInterrupt = ENET_EIR_BABR_MASK,        /*!< Babbling receive error interrupt source */
    kENET_BabtInterrupt = ENET_EIR_BABT_MASK,        /*!< Babbling transmit error interrupt source */
    kENET_GraceStopInterrupt = ENET_EIR_GRA_MASK,    /*!< Graceful stop complete interrupt source */
    kENET_TxFrameInterrupt = ENET_EIR_TXF_MASK,      /*!< TX FRAME interrupt source */
    kENET_TxBufferInterrupt = ENET_EIR_TXB_MASK,     /*!< TX BUFFER interrupt source */
    kENET_RxFrameInterrupt = ENET_EIR_RXF_MASK,      /*!< RX FRAME interrupt source */
    kENET_RxBufferInterrupt = ENET_EIR_RXB_MASK,     /*!< RX BUFFER interrupt source */
    kENET_MiiInterrupt = ENET_EIR_MII_MASK,          /*!< MII interrupt source */
    kENET_EBusERInterrupt = ENET_EIR_EBERR_MASK,     /*!< Ethernet bus error interrupt source */
    kENET_LateCollisionInterrupt = ENET_EIR_LC_MASK, /*!< Late collision interrupt source */
    kENET_RetryLimitInterrupt = ENET_EIR_RL_MASK,    /*!< Collision Retry Limit interrupt source */
    kENET_UnderrunInterrupt = ENET_EIR_UN_MASK,      /*!< Transmit FIFO underrun interrupt source */
    kENET_PayloadRxInterrupt = ENET_EIR_PLR_MASK,    /*!< Payload Receive interrupt source */
    kENET_WakeupInterrupt = ENET_EIR_WAKEUP_MASK,     /*!< WAKEUP interrupt source */
    kENET_TsAvailInterrupt = ENET_EIR_TS_AVAIL_MASK, /*!< TS AVAIL interrupt source for PTP */
    kENET_TsTimerInterrupt = ENET_EIR_TS_TIMER_MASK  /*!< TS WRAP interrupt source for PTP */
} enet_interrupt_enable_t;

/*! @brief Defines the common interrupt event for callback use. */
typedef enum _enet_event
{
    kENET_RxEvent,     /*!< Receive event. */
    kENET_TxEvent,     /*!< Transmit event. */
    kENET_ErrEvent,    /*!< Error event: BABR/BABT/EBERR/LC/RL/UN/PLR . */
    kENET_WakeUpEvent, /*!< Wake up from sleep mode event. */
    kENET_TimeStampEvent,     /*!< Time stamp event. */
    kENET_TimeStampAvailEvent /*!< Time stamp available event.*/
} enet_event_t;

/*! @brief Defines the transmit accelerator configuration. */
typedef enum _enet_tx_accelerator
{
    kENET_TxAccelIsShift16Enabled = ENET_TACC_SHIFT16_MASK, /*!< Transmit FIFO shift-16. */
    kENET_TxAccelIpCheckEnabled = ENET_TACC_IPCHK_MASK,     /*!< Insert IP header checksum. */
    kENET_TxAccelProtoCheckEnabled = ENET_TACC_PROCHK_MASK  /*!< Insert protocol checksum. */
} enet_tx_accelerator_t;

/*! @brief Defines the receive accelerator configuration. */
typedef enum _enet_rx_accelerator
{
    kENET_RxAccelPadRemoveEnabled = ENET_RACC_PADREM_MASK,  /*!< Padding removal for short IP frames. */
    kENET_RxAccelIpCheckEnabled = ENET_RACC_IPDIS_MASK,     /*!< Discard with wrong IP header checksum. */
    kENET_RxAccelProtoCheckEnabled = ENET_RACC_PRODIS_MASK, /*!< Discard with wrong protocol checksum. */
    kENET_RxAccelMacCheckEnabled = ENET_RACC_LINEDIS_MASK,  /*!< Discard with Mac layer errors. */
    kENET_RxAccelisShift16Enabled = ENET_RACC_SHIFT16_MASK  /*!< Receive FIFO shift-16. */
} enet_rx_accelerator_t;

#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
/*! @brief Defines the ENET PTP message related constant. */
typedef enum _enet_ptp_event_type
{
    kENET_PtpEventMsgType = 3U,  /*!< PTP event message type. */
    kENET_PtpSrcPortIdLen = 10U, /*!< PTP message sequence id length. */
    kENET_PtpEventPort = 319U,   /*!< PTP event port number. */
    kENET_PtpGnrlPort = 320U     /*!< PTP general port number. */
} enet_ptp_event_type_t;

/*! @brief Defines the IEEE 1588 PTP timer channel numbers. */
typedef enum _enet_ptp_timer_channel
{
    kENET_PtpTimerChannel1 = 0U, /*!< IEEE 1588 PTP timer Channel 1. */
    kENET_PtpTimerChannel2,      /*!< IEEE 1588 PTP timer Channel 2. */
    kENET_PtpTimerChannel3,      /*!< IEEE 1588 PTP timer Channel 3. */
    kENET_PtpTimerChannel4       /*!< IEEE 1588 PTP timer Channel 4. */
} enet_ptp_timer_channel_t;

/*! @brief Defines the capture or compare mode for IEEE 1588 PTP timer channels. */
typedef enum _enet_ptp_timer_channel_mode
{
    kENET_PtpChannelDisable = 0U,                  /*!< Disable timer channel. */
    kENET_PtpChannelRisingCapture = 1U,            /*!< Input capture on rising edge. */
    kENET_PtpChannelFallingCapture = 2U,           /*!< Input capture on falling edge. */
    kENET_PtpChannelBothCapture = 3U,              /*!< Input capture on both edges. */
    kENET_PtpChannelSoftCompare = 4U,              /*!< Output compare software only. */
    kENET_PtpChannelToggleCompare = 5U,            /*!< Toggle output on compare. */
    kENET_PtpChannelClearCompare = 6U,             /*!< Clear output on compare. */
    kENET_PtpChannelSetCompare = 7U,               /*!< Set output on compare. */
    kENET_PtpChannelClearCompareSetOverflow = 10U, /*!< Clear output on compare, set output on overflow. */
    kENET_PtpChannelSetCompareClearOverflow = 11U, /*!< Set output on compare, clear output on overflow. */
    kENET_PtpChannelPulseLowonCompare = 14U,       /*!< Pulse output low on compare for one IEEE 1588 clock cycle. */
    kENET_PtpChannelPulseHighonCompare = 15U       /*!< Pulse output high on compare for one IEEE 1588 clock cycle. */
} enet_ptp_timer_channel_mode_t;
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

/*! @brief Defines the receive buffer descriptor structure for the little endian system.*/
typedef struct _enet_rx_bd_struct
{
    uint16_t length;  /*!< Buffer descriptor data length. */
    uint16_t control; /*!< Buffer descriptor control and status. */
    uint8_t *buffer;  /*!< Data buffer pointer. */
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
    uint16_t controlExtend0;  /*!< Extend buffer descriptor control0. */
    uint16_t controlExtend1;  /*!< Extend buffer descriptor control1. */
    uint16_t payloadCheckSum; /*!< Internal payload checksum. */
    uint8_t headerLength;     /*!< Header length. */
    uint8_t protocolTyte;     /*!< Protocol type. */
    uint16_t reserved0;
    uint16_t controlExtend2; /*!< Extend buffer descriptor control2. */
    uint32_t timestamp;      /*!< Timestamp. */
    uint16_t reserved1;
    uint16_t reserved2;
    uint16_t reserved3;
    uint16_t reserved4;
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
} enet_rx_bd_struct_t;

/*! @brief Defines the enhanced transmit buffer descriptor structure for the little endian system. */
typedef struct _enet_tx_bd_struct
{
    uint16_t length;  /*!< Buffer descriptor data length. */
    uint16_t control; /*!< Buffer descriptor control and status. */
    uint8_t *buffer;  /*!< Data buffer pointer. */
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
    uint16_t controlExtend0; /*!< Extend buffer descriptor control0. */
    uint16_t controlExtend1; /*!< Extend buffer descriptor control1. */
    uint16_t reserved0;
    uint16_t reserved1;
    uint16_t reserved2;
    uint16_t controlExtend2; /*!< Extend buffer descriptor control2. */
    uint32_t timestamp;      /*!< Timestamp. */
    uint16_t reserved3;
    uint16_t reserved4;
    uint16_t reserved5;
    uint16_t reserved6;
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
} enet_tx_bd_struct_t;

/*! @brief Defines the ENET data error statistic structure. */
typedef struct _enet_data_error_stats
{
    uint32_t statsRxLenGreaterErr; /*!< Receive length greater than RCR[MAX_FL]. */
    uint32_t statsRxAlignErr;      /*!< Receive non-octet alignment/ */
    uint32_t statsRxFcsErr;        /*!< Receive CRC error. */
    uint32_t statsRxOverRunErr;    /*!< Receive over run. */
    uint32_t statsRxTruncateErr;   /*!< Receive truncate. */
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
    uint32_t statsRxProtocolChecksumErr; /*!< Receive protocol checksum error. */
    uint32_t statsRxIpHeadChecksumErr;   /*!< Receive IP header checksum error. */
    uint32_t statsRxMacErr;              /*!< Receive Mac error. */
    uint32_t statsRxPhyErr;              /*!< Receive PHY error. */
    uint32_t statsRxCollisionErr;        /*!< Receive collision. */
    uint32_t statsTxErr;                 /*!< The error happen when transmit the frame. */
    uint32_t statsTxFrameErr;            /*!< The transmit frame is error. */
    uint32_t statsTxOverFlowErr;         /*!< Transmit overflow. */
    uint32_t statsTxLateCollisionErr;    /*!< Transmit late collision. */
    uint32_t statsTxExcessCollisionErr;  /*!< Transmit excess collision.*/
    uint32_t statsTxUnderFlowErr;        /*!< Transmit under flow error. */
    uint32_t statsTxTsErr;               /*!< Transmit time stamp error. */
#endif                                   /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
} enet_data_error_stats_t;

/*! @brief Defines the receive buffer descriptor configuration structure.
 *
 * Note that for the internal DMA requirements, the buffers have a corresponding alignment requirements.
 * 1. The aligned receive and transmit buffer size must be evenly divisible by ENET_BUFF_ALIGNMENT.
 *    when the data buffers are in cacheable region when cache is enabled, all those size should be
 *    aligned to the maximum value of "ENET_BUFF_ALIGNMENT" and the cache line size.
 * 2. The aligned transmit and receive buffer descriptor start address must be at
 *    least 64 bit aligned. However, it's recommended to be evenly divisible by ENET_BUFF_ALIGNMENT.
 *    buffer descriptors should be put in non-cacheable region when cache is enabled.
 * 3. The aligned transmit and receive data buffer start address must be evenly divisible by ENET_BUFF_ALIGNMENT.
 *    Receive buffers should be continuous with the total size equal to "rxBdNumber * rxBuffSizeAlign".
 *    Transmit buffers should be continuous with the total size equal to "txBdNumber * txBuffSizeAlign".
 *    when the data buffers are in cacheable region when cache is enabled, all those size should be
 *    aligned to the maximum value of "ENET_BUFF_ALIGNMENT" and the cache line size. 
 */
typedef struct _enet_buffer_config
{
    uint16_t rxBdNumber;                              /*!< Receive buffer descriptor number. */
    uint16_t txBdNumber;                              /*!< Transmit buffer descriptor number. */
    uint32_t rxBuffSizeAlign;                         /*!< Aligned receive data buffer size. */
    uint32_t txBuffSizeAlign;                         /*!< Aligned transmit data buffer size. */
    volatile enet_rx_bd_struct_t *rxBdStartAddrAlign; /*!< Aligned receive buffer descriptor start address. */
    volatile enet_tx_bd_struct_t *txBdStartAddrAlign; /*!< Aligned transmit buffer descriptor start address. */
    uint8_t *rxBufferAlign;                           /*!< Receive data buffer start address. */
    uint8_t *txBufferAlign;                           /*!< Transmit data buffer start address. */
} enet_buffer_config_t;

#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
/*! @brief Defines the ENET PTP time stamp structure. */
typedef struct _enet_ptp_time
{
    uint64_t second;     /*!< Second. */
    uint32_t nanosecond; /*!< Nanosecond. */
} enet_ptp_time_t;

/*! @brief Defines the structure for the ENET PTP message data and timestamp data.*/
typedef struct _enet_ptp_time_data
{
    uint8_t version;                             /*!< PTP version. */
    uint8_t sourcePortId[kENET_PtpSrcPortIdLen]; /*!< PTP source port ID. */
    uint16_t sequenceId;                         /*!< PTP sequence ID. */
    uint8_t messageType;                         /*!< PTP message type. */
    enet_ptp_time_t timeStamp;                   /*!< PTP timestamp. */
} enet_ptp_time_data_t;

/*! @brief Defines the ENET PTP ring buffer structure for the PTP message timestamp store.*/
typedef struct _enet_ptp_time_data_ring
{
    uint32_t front;                  /*!< The first index of the ring. */
    uint32_t end;                    /*!< The end index of the ring. */
    uint32_t size;                   /*!< The size of the ring. */
    enet_ptp_time_data_t *ptpTsData; /*!< PTP message data structure. */
} enet_ptp_time_data_ring_t;

/*! @brief Defines the ENET PTP configuration structure. */
typedef struct _enet_ptp_config
{
    uint8_t ptpTsRxBuffNum;            /*!< Receive 1588 timestamp buffer number*/
    uint8_t ptpTsTxBuffNum;            /*!< Transmit 1588 timestamp buffer number*/
    enet_ptp_time_data_t *rxPtpTsData; /*!< The start address of 1588 receive timestamp buffers */
    enet_ptp_time_data_t *txPtpTsData; /*!< The start address of 1588 transmit timestamp buffers */
    enet_ptp_timer_channel_t channel;  /*!< Used for ERRATA_2579: the PTP 1588 timer channel for time interrupt. */
    uint32_t ptp1588ClockSrc_Hz;       /*!< The clock source of the PTP 1588 timer. */
} enet_ptp_config_t;
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

#if defined (FSL_FEATURE_ENET_HAS_INTERRUPT_COALESCE) && FSL_FEATURE_ENET_HAS_INTERRUPT_COALESCE
/*! @brief Defines the interrupt coalescing configure structure. */
typedef struct _enet_intcoalesce_config
{
    uint8_t txCoalesceFrameCount[FSL_FEATURE_ENET_QUEUE]; /*!< Transmit interrupt coalescing frame count threshold. */
    uint16_t txCoalesceTimeCount[FSL_FEATURE_ENET_QUEUE]; /*!< Transmit interrupt coalescing timer count threshold. */
    uint8_t rxCoalesceFrameCount[FSL_FEATURE_ENET_QUEUE]; /*!< Receive interrupt coalescing frame count threshold. */
    uint16_t rxCoalesceTimeCount[FSL_FEATURE_ENET_QUEUE]; /*!< Receive interrupt coalescing timer count threshold. */
} enet_intcoalesce_config_t;
#endif /* FSL_FEATURE_ENET_HAS_INTERRUPT_COALESCE */

/*! @brief Defines the basic configuration structure for the ENET device.
 *
 * Note:
 *  1. macSpecialConfig is used for a special control configuration, a logical OR of
 *  "enet_special_control_flag_t". For a special configuration for MAC,
 *  set this parameter to 0.
 *  2. txWatermark is used for a cut-through operation. It is in steps of 64 bytes.
 *  0/1  - 64 bytes written to TX FIFO before transmission of a frame begins.
 *  2    - 128 bytes written to TX FIFO ....
 *  3    - 192 bytes written to TX FIFO ....
 *  The maximum of txWatermark is 0x2F   - 4032 bytes written to TX FIFO.
 *  txWatermark allows minimizing the transmit latency to set the txWatermark to 0 or 1
 *  or for larger bus access latency 3 or larger due to contention for the system bus.
 *  3. rxFifoFullThreshold is similar to the txWatermark for cut-through operation in RX.
 *  It is in 64-bit words. The minimum is ENET_FIFO_MIN_RX_FULL and the maximum is 0xFF.
 *  If the end of the frame is stored in FIFO and the frame size if smaller than the
 *  txWatermark, the frame is still transmitted. The rule  is the
 *  same for rxFifoFullThreshold in the receive direction.
 *  4. When "kENET_ControlFlowControlEnable" is set in the macSpecialConfig, ensure
 *  that the pauseDuration, rxFifoEmptyThreshold, and rxFifoStatEmptyThreshold
 *  are set for flow control enabled case.
 *  5. When "kENET_ControlStoreAndFwdDisabled" is set in the macSpecialConfig, ensure
 *  that the rxFifoFullThreshold and txFifoWatermark are set for store and forward disable.
 *  6. The rxAccelerConfig and txAccelerConfig default setting with 0 - accelerator
 *  are disabled. The "enet_tx_accelerator_t" and "enet_rx_accelerator_t" are
 *  recommended to be used to enable the transmit and receive accelerator.
 *  After the accelerators are enabled, the store and forward feature should be enabled.
 *  As a result, kENET_ControlStoreAndFwdDisabled should not be set.
 */
typedef struct _enet_config
{
    uint32_t macSpecialConfig;    /*!< Mac special configuration. A logical OR of "enet_special_control_flag_t". */
    uint32_t interrupt;           /*!< Mac interrupt source. A logical OR of "enet_interrupt_enable_t". */
    uint16_t rxMaxFrameLen;       /*!< Receive maximum frame length. */
    enet_mii_mode_t miiMode;      /*!< MII mode. */
    enet_mii_speed_t miiSpeed;    /*!< MII Speed. */
    enet_mii_duplex_t miiDuplex;  /*!< MII duplex. */
    uint8_t rxAccelerConfig;      /*!< Receive accelerator, A logical OR of "enet_rx_accelerator_t". */
    uint8_t txAccelerConfig;      /*!< Transmit accelerator, A logical OR of "enet_rx_accelerator_t". */
    uint16_t pauseDuration;       /*!< For flow control enabled case: Pause duration. */
    uint8_t rxFifoEmptyThreshold; /*!< For flow control enabled case:  when RX FIFO level reaches this value,
                                     it makes MAC generate XOFF pause frame. */
#if defined (FSL_FEATURE_ENET_HAS_RECEIVE_STATUS_THRESHOLD) && FSL_FEATURE_ENET_HAS_RECEIVE_STATUS_THRESHOLD
    uint8_t rxFifoStatEmptyThreshold; /*!< For flow control enabled case: number of frames in the receive FIFO,
                                    independent of size, that can be accept. If the limit is reached, reception
                                    continues and a pause frame is triggered. */
#endif                                /* FSL_FEATURE_ENET_HAS_RECEIVE_STATUS_THRESHOLD */
    uint8_t rxFifoFullThreshold;      /*!< For store and forward disable case, the data required in RX FIFO to notify
                                      the MAC receive ready status. */
    uint8_t txFifoWatermark;          /*!< For store and forward disable case, the data required in TX FIFO
                                      before a frame transmit start. */
#if defined (FSL_FEATURE_ENET_HAS_INTERRUPT_COALESCE) && FSL_FEATURE_ENET_HAS_INTERRUPT_COALESCE
    enet_intcoalesce_config_t *intCoalesceCfg; /* If the interrupt coalsecence is not required in the ring n(0,1,2), please set
                                         to NULL. */
#endif /* FSL_FEATURE_ENET_HAS_INTERRUPT_COALESCE */
} enet_config_t;

/* Forward declaration of the handle typedef. */
typedef struct _enet_handle enet_handle_t;

/*! @brief ENET callback function. */
typedef void (*enet_callback_t)(ENET_Type *base, enet_handle_t *handle, enet_event_t event, void *userData);

/*! @brief Defines the ENET handler structure. */
struct _enet_handle
{
    volatile enet_rx_bd_struct_t *rxBdBase;    /*!< Receive buffer descriptor base address pointer. */
    volatile enet_rx_bd_struct_t *rxBdCurrent; /*!< The current available receive buffer descriptor pointer. */
    volatile enet_tx_bd_struct_t *txBdBase;    /*!< Transmit buffer descriptor base address pointer. */
    volatile enet_tx_bd_struct_t *txBdCurrent; /*!< The current available transmit buffer descriptor pointer. */
    uint32_t rxBuffSizeAlign;                  /*!< Receive buffer size alignment. */
    uint32_t txBuffSizeAlign;                  /*!< Transmit buffer size alignment. */
    enet_callback_t callback;                  /*!< Callback function. */
    void *userData;                            /*!< Callback function parameter.*/
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
    volatile enet_tx_bd_struct_t *txBdDirtyStatic; /*!< The dirty transmit buffer descriptor for error static update. */
    volatile enet_tx_bd_struct_t *txBdDirtyTime;   /*!< The dirty transmit buffer descriptor for time stamp update. */
    uint64_t msTimerSecond;                        /*!< The second for Master PTP timer .*/
    enet_ptp_time_data_ring_t rxPtpTsDataRing;     /*!< Receive PTP 1588 time stamp data ring buffer. */
    enet_ptp_time_data_ring_t txPtpTsDataRing;     /*!< Transmit PTP 1588 time stamp data ring buffer. */
#endif                                             /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
  * @name Initialization and de-initialization
  * @{
  */

/*!
 * @brief Gets the ENET default configuration structure.
 *
 * The purpose of this API is to get the default ENET MAC controller
 * configuration structure for ENET_Init(). Users may use the initialized
 * structure unchanged in ENET_Init() or modify fields of the
 * structure before calling ENET_Init().
 * This is an example.
   @code
   enet_config_t config;
   ENET_GetDefaultConfig(&config);
   @endcode
 * @param config The ENET mac controller configuration structure pointer.
 */
void ENET_GetDefaultConfig(enet_config_t *config);

/*!
 * @brief Initializes the ENET module.
 *
 * This function ungates the module clock and initializes it with the ENET configuration.
 *
 * @param base    ENET peripheral base address.
 * @param handle  ENET handler pointer.
 * @param config  ENET Mac configuration structure pointer.
 *        The "enet_config_t" type mac configuration return from ENET_GetDefaultConfig
 *        can be used directly. It is also possible to verify the Mac configuration using other methods.
 * @param bufferConfig  ENET buffer configuration structure pointer.
 *        The buffer configuration should be prepared for ENET Initialization.
 * @param macAddr  ENET mac address of the Ethernet device. This Mac address should be
 *        provided.
 * @param srcClock_Hz The internal module clock source for MII clock.
 *
 * @note ENET has two buffer descriptors legacy buffer descriptors and
 * enhanced IEEE 1588 buffer descriptors. The legacy descriptor is used by default. To
 * use the IEEE 1588 feature, use the enhanced IEEE 1588 buffer descriptor
 * by defining "ENET_ENHANCEDBUFFERDESCRIPTOR_MODE" and calling ENET_Ptp1588Configure()
 * to configure the 1588 feature and related buffers after calling ENET_Init().
 */
void ENET_Init(ENET_Type *base,
               enet_handle_t *handle,
               const enet_config_t *config,
               const enet_buffer_config_t *bufferConfig,
               uint8_t *macAddr,
               uint32_t srcClock_Hz);
/*!
 * @brief Deinitializes the ENET module.

 * This function gates the module clock, clears ENET interrupts, and disables the ENET module.
 *
 * @param base  ENET peripheral base address.
 */
void ENET_Deinit(ENET_Type *base);

/*!
 * @brief Resets the ENET module.
 *
 * This function restores the ENET module to the reset state.
 * Note that this function sets all registers to the
 * reset state. As a result, the ENET module can't work after calling this function.
 *
 * @param base  ENET peripheral base address.
 */
static inline void ENET_Reset(ENET_Type *base)
{
    base->ECR |= ENET_ECR_RESET_MASK;
}

/* @} */

/*!
 * @name MII interface operation
 * @{
 */

/*!
 * @brief Sets the ENET MII speed and duplex.
 *
 * @param base  ENET peripheral base address.
 * @param speed The speed of the RMII mode.
 * @param duplex The duplex of the RMII mode.
 */
void ENET_SetMII(ENET_Type *base, enet_mii_speed_t speed, enet_mii_duplex_t duplex);

/*!
 * @brief Sets the ENET SMI (serial management interface) - MII management interface.
 *
 * @param base  ENET peripheral base address.
 * @param srcClock_Hz This is the ENET module clock frequency. Normally it's the system clock. See clock distribution.
 * @param isPreambleDisabled The preamble disable flag.
 *        - true   Enables the preamble.
 *        - false  Disables the preamble.
 */
void ENET_SetSMI(ENET_Type *base, uint32_t srcClock_Hz, bool isPreambleDisabled);

/*!
 * @brief Gets the ENET SMI- MII management interface configuration.
 *
 * This API is used to get the SMI configuration to check whether the MII management
 * interface has been set.
 *
 * @param base  ENET peripheral base address.
 * @return The SMI setup status true or false.
 */
static inline bool ENET_GetSMI(ENET_Type *base)
{
    return (0 != (base->MSCR & 0x7E));
}

/*!
 * @brief Reads data from the PHY register through an SMI interface.
 *
 * @param base  ENET peripheral base address.
 * @return The data read from PHY
 */
static inline uint32_t ENET_ReadSMIData(ENET_Type *base)
{
    return (uint32_t)((base->MMFR & ENET_MMFR_DATA_MASK) >> ENET_MMFR_DATA_SHIFT);
}

/*!
 * @brief Starts an SMI (Serial Management Interface) read command.
 *
 * @param base  ENET peripheral base address.
 * @param phyAddr The PHY address.
 * @param phyReg The PHY register.
 * @param operation The read operation.
 */
void ENET_StartSMIRead(ENET_Type *base, uint32_t phyAddr, uint32_t phyReg, enet_mii_read_t operation);

/*!
 * @brief Starts an SMI write command.
 *
 * @param base  ENET peripheral base address.
 * @param phyAddr The PHY address.
 * @param phyReg The PHY register.
 * @param operation The write operation.
 * @param data The data written to PHY.
 */
void ENET_StartSMIWrite(ENET_Type *base, uint32_t phyAddr, uint32_t phyReg, enet_mii_write_t operation, uint32_t data);

#if defined (FSL_FEATURE_ENET_HAS_EXTEND_MDIO) && FSL_FEATURE_ENET_HAS_EXTEND_MDIO
/*!
 * @brief Starts the extended IEEE802.3 Clause 45 MDIO format SMI read command.
 *
 * @param base  ENET peripheral base address.
 * @param phyAddr The PHY address.
 * @param phyReg The PHY register. For MDIO IEEE802.3 Clause 45,
 *        the phyReg is a 21-bits combination of the devaddr (5 bits device address)
 *        and the regAddr (16 bits phy register): phyReg = (devaddr << 16) | regAddr.
 */
void ENET_StartExtC45SMIRead(ENET_Type *base, uint32_t phyAddr, uint32_t phyReg);

/*!
 * @brief Starts the extended IEEE802.3 Clause 45 MDIO format SMI write command.
 *
 * @param base  ENET peripheral base address.
 * @param phyAddr The PHY address.
 * @param phyReg The PHY register. For MDIO IEEE802.3 Clause 45,
 *        the phyReg is a 21-bits combination of the devaddr (5 bits device address)
 *        and the regAddr (16 bits phy register): phyReg = (devaddr << 16) | regAddr.
 * @param data The data written to PHY.
 */
void ENET_StartExtC45SMIWrite(ENET_Type *base, uint32_t phyAddr, uint32_t phyReg, uint32_t data);
#endif /* FSL_FEATURE_ENET_HAS_EXTEND_MDIO */

/* @} */

/*!
 * @name MAC Address Filter
 * @{
 */

/*!
 * @brief Sets the ENET module Mac address.
 *
 * @param base  ENET peripheral base address.
 * @param macAddr The six-byte Mac address pointer.
 *        The pointer is allocated by application and input into the API.
 */
void ENET_SetMacAddr(ENET_Type *base, uint8_t *macAddr);

/*!
 * @brief Gets the ENET module Mac address.
 *
 * @param base  ENET peripheral base address.
 * @param macAddr The six-byte Mac address pointer.
 *        The pointer is allocated by application and input into the API.
 */
void ENET_GetMacAddr(ENET_Type *base, uint8_t *macAddr);

/*!
 * @brief Adds the ENET device to a multicast group.
 *
 * @param base    ENET peripheral base address.
 * @param address The six-byte multicast group address which is provided by application.
 */
void ENET_AddMulticastGroup(ENET_Type *base, uint8_t *address);

/*!
 * @brief Moves the ENET device from a multicast group.
 *
 * @param base  ENET peripheral base address.
 * @param address The six-byte multicast group address which is provided by application.
 */
void ENET_LeaveMulticastGroup(ENET_Type *base, uint8_t *address);

/* @} */

/*!
 * @name Other basic operations
 * @{
 */

/*!
 * @brief Activates ENET read or receive.
 *
 * @param base  ENET peripheral base address.
 *
 * @note This must be called after the MAC configuration and
 * state are ready. It must be called after the ENET_Init() and
 * ENET_Ptp1588Configure(). This should be called when the ENET receive required.
 */
static inline void ENET_ActiveRead(ENET_Type *base)
{
    base->RDAR = ENET_RDAR_RDAR_MASK;
}

/*!
 * @brief Enables/disables the MAC to enter sleep mode.
 * This function is used to set the MAC enter sleep mode.
 * When entering sleep mode, the magic frame wakeup interrupt should be enabled
 * to wake up MAC from the sleep mode and reset it to normal mode.
 *
 * @param base    ENET peripheral base address.
 * @param enable  True enable sleep mode, false disable sleep mode.
 */
static inline void ENET_EnableSleepMode(ENET_Type *base, bool enable)
{
    if (enable)
    {
        /* When this field is set, MAC enters sleep mode. */
        base->ECR |= ENET_ECR_SLEEP_MASK | ENET_ECR_MAGICEN_MASK;
    }
    else
    { /* MAC exits sleep mode. */
        base->ECR &= ~(ENET_ECR_SLEEP_MASK | ENET_ECR_MAGICEN_MASK);
    }
}

/*!
 * @brief Gets ENET transmit and receive accelerator functions from the MAC controller.
 *
 * @param base  ENET peripheral base address.
 * @param txAccelOption The transmit accelerator option. The "enet_tx_accelerator_t" is
 *         recommended  as the mask to get the exact the accelerator option.
 * @param rxAccelOption The receive accelerator option. The "enet_rx_accelerator_t" is
 *         recommended as the mask to get the exact the accelerator option.
 */
static inline void ENET_GetAccelFunction(ENET_Type *base, uint32_t *txAccelOption, uint32_t *rxAccelOption)
{
    assert(txAccelOption);
    assert(txAccelOption);

    *txAccelOption = base->TACC;
    *rxAccelOption = base->RACC;
}

/* @} */

/*!
 * @name Interrupts
 * @{
 */

/*!
 * @brief Enables the ENET interrupt.
 *
 * This function enables the ENET interrupt according to the provided mask. The mask
 * is a logical OR of enumeration members. See @ref enet_interrupt_enable_t.
 * For example, to enable the TX frame interrupt and RX frame interrupt, do the following.
 * @code
 *     ENET_EnableInterrupts(ENET, kENET_TxFrameInterrupt | kENET_RxFrameInterrupt);
 * @endcode
 *
 * @param base  ENET peripheral base address.
 * @param mask  ENET interrupts to enable. This is a logical OR of the
 *             enumeration :: enet_interrupt_enable_t.
 */
static inline void ENET_EnableInterrupts(ENET_Type *base, uint32_t mask)
{
    base->EIMR |= mask;
}

/*!
 * @brief Disables the ENET interrupt.
 *
 * This function disables the ENET interrupts according to the provided mask. The mask
 * is a logical OR of enumeration members. See @ref enet_interrupt_enable_t.
 * For example, to disable the TX frame interrupt and RX frame interrupt, do the following.
 * @code
 *     ENET_DisableInterrupts(ENET, kENET_TxFrameInterrupt | kENET_RxFrameInterrupt);
 * @endcode
 *
 * @param base  ENET peripheral base address.
 * @param mask  ENET interrupts to disable. This is a logical OR of the
 *             enumeration :: enet_interrupt_enable_t.
 */
static inline void ENET_DisableInterrupts(ENET_Type *base, uint32_t mask)
{
    base->EIMR &= ~mask;
}

/*!
 * @brief Gets the ENET interrupt status flag.
 *
 * @param base  ENET peripheral base address.
 * @return The event status of the interrupt source. This is the logical OR of members
 *         of the enumeration :: enet_interrupt_enable_t.
 */
static inline uint32_t ENET_GetInterruptStatus(ENET_Type *base)
{
    return base->EIR;
}

/*!
 * @brief Clears the ENET interrupt events status flag.
 *
 * This function clears enabled ENET interrupts according to the provided mask. The mask
 * is a logical OR of enumeration members. See the @ref enet_interrupt_enable_t.
 * For example, to clear the TX frame interrupt and RX frame interrupt, do the following.
 * @code
 *     ENET_ClearInterruptStatus(ENET, kENET_TxFrameInterrupt | kENET_RxFrameInterrupt);
 * @endcode
 *
 * @param base  ENET peripheral base address.
 * @param mask  ENET interrupt source to be cleared.
 * This is the logical OR of members of the enumeration :: enet_interrupt_enable_t.
 */
static inline void ENET_ClearInterruptStatus(ENET_Type *base, uint32_t mask)
{
    base->EIR = mask;
}

/* @} */

/*!
 * @name Transactional operation
 * @{
 */

/*!
 * @brief Sets the callback function.
 * This API is provided for the application callback required case when ENET
 * interrupt is enabled. This API should be called after calling ENET_Init.
 *
 * @param handle ENET handler pointer. Should be provided by application.
 * @param callback The ENET callback function.
 * @param userData The callback function parameter.
 */
void ENET_SetCallback(enet_handle_t *handle, enet_callback_t callback, void *userData);

/*!
 * @brief Gets the ENET the error statistics of a received frame.
 *
 * This API must be called after the ENET_GetRxFrameSize and before the ENET_ReadFrame().
 * If the ENET_GetRxFrameSize returns kStatus_ENET_RxFrameError,
 * the ENET_GetRxErrBeforeReadFrame can be used to get the exact error statistics.
 * This is an example.
 * @code
 *       status = ENET_GetRxFrameSize(&g_handle, &length);
 *       if (status == kStatus_ENET_RxFrameError)
 *       {
 *           // Get the error information of the received frame.
 *           ENET_GetRxErrBeforeReadFrame(&g_handle, &eErrStatic);
 *           // update the receive buffer.
 *           ENET_ReadFrame(EXAMPLE_ENET, &g_handle, NULL, 0);
 *       }
 * @endcode
 * @param handle The ENET handler structure pointer. This is the same handler pointer used in the ENET_Init.
 * @param eErrorStatic The error statistics structure pointer.
 */
void ENET_GetRxErrBeforeReadFrame(enet_handle_t *handle, enet_data_error_stats_t *eErrorStatic);

#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
/*!
 * @brief Gets the ENET transmit frame statistics after the data send.
 *
 * This interface gets the error statistics of the transmit frame.
 * Because the error information is reported by the uDMA after the data delivery, this interface
 * should be called after the data transmit API. It is recommended to call this function on
 * transmit interrupt handler. After calling the ENET_SendFrame, the
 * transmit interrupt notifies the transmit completion.
 *
 * @param handle The PTP handler pointer. This is the same handler pointer used in the ENET_Init.
 * @param eErrorStatic The error statistics structure pointer.
 * @return The execute status.
 */
status_t ENET_GetTxErrAfterSendFrame(enet_handle_t *handle, enet_data_error_stats_t *eErrorStatic);
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
/*!
* @brief Gets the size of the read frame.
* This function gets a received frame size from the ENET buffer descriptors.
* @note The FCS of the frame is automatically removed by Mac and the size is the length without the FCS.
* After calling ENET_GetRxFrameSize, ENET_ReadFrame() should be called to update the
* receive buffers If the result is not "kStatus_ENET_RxFrameEmpty".
*
* @param handle The ENET handler structure. This is the same handler pointer used in the ENET_Init.
* @param length The length of the valid frame received.
* @retval kStatus_ENET_RxFrameEmpty No frame received. Should not call ENET_ReadFrame to read frame.
* @retval kStatus_ENET_RxFrameError Data error happens. ENET_ReadFrame should be called with NULL data
*         and NULL length to update the receive buffers.
* @retval kStatus_Success Receive a frame Successfully then the ENET_ReadFrame
*         should be called with the right data buffer and the captured data length input.
*/
status_t ENET_GetRxFrameSize(enet_handle_t *handle, uint32_t *length);

/*!
 * @brief Reads a frame from the ENET device.
 * This function reads a frame (both the data and the length) from the ENET buffer descriptors.
 * The ENET_GetRxFrameSize should be used to get the size of the prepared data buffer.
 * This is an example.
 * @code
 *       uint32_t length;
 *       enet_handle_t g_handle;
 *       //Get the received frame size firstly.
 *       status = ENET_GetRxFrameSize(&g_handle, &length);
 *       if (length != 0)
 *       {
 *           //Allocate memory here with the size of "length"
 *           uint8_t *data = memory allocate interface;
 *           if (!data)
 *           {
 *               ENET_ReadFrame(ENET, &g_handle, NULL, 0);
 *               //Add the console warning log.
 *           }
 *           else
 *           {
 *              status = ENET_ReadFrame(ENET, &g_handle, data, length);
 *              //Call stack input API to deliver the data to stack
 *           }
 *       }
 *       else if (status == kStatus_ENET_RxFrameError)
 *       {
 *          //Update the received buffer when a error frame is received.
 *           ENET_ReadFrame(ENET, &g_handle, NULL, 0);
 *       }
 * @endcode
 * @param base  ENET peripheral base address.
 * @param handle The ENET handler structure. This is the same handler pointer used in the ENET_Init.
 * @param data The data buffer provided by user to store the frame which memory size should be at least "length".
 * @param length The size of the data buffer which is still the length of the received frame.
 * @return The execute status, successful or failure.
 */
status_t ENET_ReadFrame(ENET_Type *base, enet_handle_t *handle, uint8_t *data, uint32_t length);

/*!
 * @brief Transmits an ENET frame.
 * @note The CRC is automatically appended to the data. Input the data
 * to send without the CRC.
 *
 * @param base  ENET peripheral base address.
 * @param handle The ENET handler pointer. This is the same handler pointer used in the ENET_Init.
 * @param data The data buffer provided by user to be send.
 * @param length The length of the data to be send.
 * @retval kStatus_Success  Send frame succeed.
 * @retval kStatus_ENET_TxFrameBusy  Transmit buffer descriptor is busy under transmission.
 *         The transmit busy happens when the data send rate is over the MAC capacity.
 *         The waiting mechanism is recommended to be added after each call return with
 *         kStatus_ENET_TxFrameBusy.
 */
status_t ENET_SendFrame(ENET_Type *base, enet_handle_t *handle, uint8_t *data, uint32_t length);

/*!
 * @brief The transmit IRQ handler.
 *
 * @param base  ENET peripheral base address.
 * @param handle The ENET handler pointer.
 */
void ENET_TransmitIRQHandler(ENET_Type *base, enet_handle_t *handle);

/*!
 * @brief The receive IRQ handler.
 *
 * @param base  ENET peripheral base address.
 * @param handle The ENET handler pointer.
 */
void ENET_ReceiveIRQHandler(ENET_Type *base, enet_handle_t *handle);

/*!
 * @brief The error IRQ handler.
 *
 * @param base  ENET peripheral base address.
 * @param handle The ENET handler pointer.
 */
void ENET_ErrorIRQHandler(ENET_Type *base, enet_handle_t *handle);

/*!
 * @brief the common IRQ handler for the tx/rx/error etc irq handler.
 *
 * This is used for the combined tx/rx/error interrupt for single ring (ring 0).
 *
 * @param base  ENET peripheral base address.
 */
void ENET_CommonFrame0IRQHandler(ENET_Type *base);
/* @} */

#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
/*!
 * @name ENET PTP 1588 function operation
 * @{
 */

/*!
 * @brief Configures the ENET PTP IEEE 1588 feature with the basic configuration.
 * The function sets the clock for PTP 1588 timer and enables
 * time stamp interrupts and transmit interrupts for PTP 1588 features.
 * This API should be called when the 1588 feature is enabled
 * or the ENET_ENHANCEDBUFFERDESCRIPTOR_MODE is defined.
 * ENET_Init should be called before calling this API.
 *
 * @note The PTP 1588 time-stamp second increase though time-stamp interrupt handler
 *  and the transmit time-stamp store is done through transmit interrupt handler.
 *  As a result, the TS interrupt and TX interrupt are enabled when you call this API.
 *
 * @param base    ENET peripheral base address.
 * @param handle  ENET handler pointer.
 * @param ptpConfig The ENET PTP1588 configuration.
 */
void ENET_Ptp1588Configure(ENET_Type *base, enet_handle_t *handle, enet_ptp_config_t *ptpConfig);

/*!
 * @brief Starts the ENET PTP 1588 Timer.
 * This function is used to initialize the PTP timer. After the PTP starts,
 * the PTP timer starts running.
 *
 * @param base  ENET peripheral base address.
 * @param ptpClkSrc The clock source of the PTP timer.
 */
void ENET_Ptp1588StartTimer(ENET_Type *base, uint32_t ptpClkSrc);

/*!
 * @brief Stops the ENET PTP 1588 Timer.
 * This function is used to stops the ENET PTP timer.
 *
 * @param base  ENET peripheral base address.
 */
static inline void ENET_Ptp1588StopTimer(ENET_Type *base)
{
    /* Disable PTP timer and reset the timer. */
    base->ATCR &= ~ENET_ATCR_EN_MASK;
    base->ATCR |= ENET_ATCR_RESTART_MASK;
}

/*!
 * @brief Adjusts the ENET PTP 1588 timer.
 *
 * @param base  ENET peripheral base address.
 * @param corrIncrease The correction increment value. This value is added every time the correction
 *       timer expires. A value less than the PTP timer frequency(1/ptpClkSrc) slows down the timer,
 *        a value greater than the 1/ptpClkSrc speeds up the timer.
 * @param corrPeriod The PTP timer correction counter wrap-around value. This defines after how
 *       many timer clock the correction counter should be reset and trigger a correction
 *       increment on the timer. A value of 0 disables the correction counter and no correction occurs.
 */
void ENET_Ptp1588AdjustTimer(ENET_Type *base, uint32_t corrIncrease, uint32_t corrPeriod);

/*!
 * @brief Sets the ENET PTP 1588 timer channel mode.
 *
 * @param base  ENET peripheral base address.
 * @param channel The ENET PTP timer channel number.
 * @param mode The PTP timer channel mode, see "enet_ptp_timer_channel_mode_t".
 * @param intEnable Enables or disables the interrupt.
 */
static inline void ENET_Ptp1588SetChannelMode(ENET_Type *base,
                                              enet_ptp_timer_channel_t channel,
                                              enet_ptp_timer_channel_mode_t mode,
                                              bool intEnable)
{
    uint32_t tcrReg = 0;

    tcrReg = ENET_TCSR_TMODE(mode) | ENET_TCSR_TIE(intEnable);
    /* Disable channel mode first. */
    base->CHANNEL[channel].TCSR = 0;
    base->CHANNEL[channel].TCSR = tcrReg;
}

#if defined(FSL_FEATURE_ENET_HAS_TIMER_PWCONTROL) && FSL_FEATURE_ENET_HAS_TIMER_PWCONTROL
/*!
 * @brief Sets ENET PTP 1588 timer channel mode pulse width.
 *
 * For the input "mode" in ENET_Ptp1588SetChannelMode, the kENET_PtpChannelPulseLowonCompare
 * kENET_PtpChannelPulseHighonCompare only support the pulse width for one 1588 clock.
 * this function is extended for control the pulse width from 1 to 32 1588 clock cycles. 
 * so call this function if you need to set the timer channel mode for 
 * kENET_PtpChannelPulseLowonCompare or kENET_PtpChannelPulseHighonCompare
 * with pulse width more than one 1588 clock,
 *
 * @param base  ENET peripheral base address.
 * @param channel The ENET PTP timer channel number.
 * @param isOutputLow  True --- timer channel is configured for output compare 
 *                              pulse output low.
 *                     false --- timer channel is configured for output compare
 *                              pulse output high.
 * @param pulseWidth  The pulse width control value, range from 0 ~ 31.
 *                     0  --- pulse width is one 1588 clock cycle.
 *                     31 --- pulse width is thirty two 1588 clock cycles.      
 * @param intEnable Enables or disables the interrupt.
 */
static inline void ENET_Ptp1588SetChannelOutputPulseWidth(ENET_Type *base,
                                              enet_ptp_timer_channel_t channel,
                                              bool isOutputLow,
                                              uint8_t pulseWidth,
                                              bool intEnable)
{
    uint32_t tcrReg;

    tcrReg = ENET_TCSR_TIE(intEnable) | ENET_TCSR_TPWC(pulseWidth);
    
    if (isOutputLow)
    {
        tcrReg |= ENET_TCSR_TMODE(kENET_PtpChannelPulseLowonCompare);
    }
    else
    {
        tcrReg |= ENET_TCSR_TMODE(kENET_PtpChannelPulseHighonCompare);
    }

    /* Disable channel mode first. */
    base->CHANNEL[channel].TCSR = 0;
    base->CHANNEL[channel].TCSR = tcrReg;
}
#endif   /* FSL_FEATURE_ENET_HAS_TIMER_PWCONTROL */

/*!
 * @brief Sets the ENET PTP 1588 timer channel comparison value.
 *
 * @param base  ENET peripheral base address.
 * @param channel The PTP timer channel, see "enet_ptp_timer_channel_t".
 * @param cmpValue The compare value for the compare setting.
 */
static inline void ENET_Ptp1588SetChannelCmpValue(ENET_Type *base, enet_ptp_timer_channel_t channel, uint32_t cmpValue)
{
    base->CHANNEL[channel].TCCR = cmpValue;
}

/*!
 * @brief Gets the ENET PTP 1588 timer channel status.
 *
 * @param base  ENET peripheral base address.
 * @param channel The IEEE 1588 timer channel number.
 * @return True or false, Compare or capture operation status
 */
static inline bool ENET_Ptp1588GetChannelStatus(ENET_Type *base, enet_ptp_timer_channel_t channel)
{
    return (0 != (base->CHANNEL[channel].TCSR & ENET_TCSR_TF_MASK));
}

/*!
 * @brief Clears the ENET PTP 1588 timer channel status.
 *
 * @param base  ENET peripheral base address.
 * @param channel The IEEE 1588 timer channel number.
 */
static inline void ENET_Ptp1588ClearChannelStatus(ENET_Type *base, enet_ptp_timer_channel_t channel)
{
    base->CHANNEL[channel].TCSR |= ENET_TCSR_TF_MASK;
    base->TGSR = (1U << channel);
}

/*!
 * @brief Gets the current ENET time from the PTP 1588 timer.
 *
 * @param base  ENET peripheral base address.
 * @param handle The ENET state pointer. This is the same state pointer used in the ENET_Init.
 * @param ptpTime The PTP timer structure.
 */
void ENET_Ptp1588GetTimer(ENET_Type *base, enet_handle_t *handle, enet_ptp_time_t *ptpTime);

/*!
 * @brief Sets the ENET PTP 1588 timer to the assigned time.
 *
 * @param base  ENET peripheral base address.
 * @param handle The ENET state pointer. This is the same state pointer used in the ENET_Init.
 * @param ptpTime The timer to be set to the PTP timer.
 */
void ENET_Ptp1588SetTimer(ENET_Type *base, enet_handle_t *handle, enet_ptp_time_t *ptpTime);

/*!
 * @brief The IEEE 1588 PTP time stamp interrupt handler.
 *
 * @param base  ENET peripheral base address.
 * @param handle The ENET state pointer. This is the same state pointer used in the ENET_Init.
 */
void ENET_Ptp1588TimerIRQHandler(ENET_Type *base, enet_handle_t *handle);

/*!
 * @brief Gets the time stamp of the received frame.
 *
 * This function is used for PTP stack to get the timestamp captured by the ENET driver.
 *
 * @param handle The ENET handler pointer.This is the same state pointer used in
 *        ENET_Init.
 * @param ptpTimeData The special PTP timestamp data for search the receive timestamp.
 * @retval kStatus_Success Get 1588 timestamp success.
 * @retval kStatus_ENET_PtpTsRingEmpty 1588 timestamp ring empty.
 * @retval kStatus_ENET_PtpTsRingFull 1588 timestamp ring full.
 */
status_t ENET_GetRxFrameTime(enet_handle_t *handle, enet_ptp_time_data_t *ptpTimeData);

/*!
 * @brief Gets the time stamp of the transmit frame.
 *
 * This function is used for PTP stack to get the timestamp captured by the ENET driver.
 *
 * @param handle The ENET handler pointer.This is the same state pointer used in
 *        ENET_Init.
 * @param ptpTimeData The special PTP timestamp data for search the receive timestamp.
 * @retval kStatus_Success Get 1588 timestamp success.
 * @retval kStatus_ENET_PtpTsRingEmpty 1588 timestamp ring empty.
 * @retval kStatus_ENET_PtpTsRingFull 1588 timestamp ring full.
 */
status_t ENET_GetTxFrameTime(enet_handle_t *handle, enet_ptp_time_data_t *ptpTimeData);
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_ENET_H_ */
