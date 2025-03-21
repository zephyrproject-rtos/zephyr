/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is based on dw1000_regs.h and dw1000_mac.c from
 * https://github.com/Decawave/mynewt-dw1000-core.git
 * (d6b1414f1b4527abda7521a304baa1c648244108)
 * The content was modified and restructured to meet the
 * coding style and resolve namespace issues.
 *
 * This file is derived from material that is:
 *
 * Copyright (C) 2017-2018, Decawave Limited, All Rights Reserved
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef ZEPHYR_INCLUDE_DW1000_REGS_H_
#define ZEPHYR_INCLUDE_DW1000_REGS_H_

/* Device ID register, includes revision info (0xDECA0130) */
#define DWT_DEV_ID_ID               0x00
#define DWT_DEV_ID_LEN              4
/* Revision */
#define DWT_DEV_ID_REV_MASK         0x0000000FUL
/* Version */
#define DWT_DEV_ID_VER_MASK         0x000000F0UL
/* The MODEL identifies the device. The DW1000 is device type 0x01 */
#define DWT_DEV_ID_MODEL_MASK       0x0000FF00UL
/* Register Identification Tag 0XDECA */
#define DWT_DEV_ID_RIDTAG_MASK      0xFFFF0000UL

/* IEEE Extended Unique Identifier (63:0) */
#define DWT_EUI_64_ID               0x01
#define DWT_EUI_64_OFFSET           0x00
#define DWT_EUI_64_LEN              8

/* PAN ID (31:16) and Short Address (15:0) */
#define DWT_PANADR_ID               0x03
#define DWT_PANADR_LEN              4
#define DWT_PANADR_SHORT_ADDR_OFFSET 0
/* Short Address */
#define DWT_PANADR_SHORT_ADDR_MASK  0x0000FFFFUL
#define DWT_PANADR_PAN_ID_OFFSET     2
/* PAN Identifier */
#define DWT_PANADR_PAN_ID_MASK      0xFFFF00F0UL

#define DWT_REG_05_ID_RESERVED      0x05

/* System Configuration (31:0) */
#define DWT_SYS_CFG_ID              0x04
#define DWT_SYS_CFG_LEN             4
/* Access mask to SYS_CFG_ID */
#define DWT_SYS_CFG_MASK            0xF047FFFFUL
/* Frame filtering options all frames allowed */
#define DWT_SYS_CFG_FF_ALL_EN       0x000001FEUL
/* Frame Filtering Enable. This bit enables the frame filtering functionality */
#define DWT_SYS_CFG_FFE             0x00000001UL
/* Frame Filtering Behave as a Co-ordinator */
#define DWT_SYS_CFG_FFBC            0x00000002UL
/* Frame Filtering Allow Beacon frame reception */
#define DWT_SYS_CFG_FFAB            0x00000004UL
/* Frame Filtering Allow Data frame reception */
#define DWT_SYS_CFG_FFAD            0x00000008UL
/* Frame Filtering Allow Acknowledgment frame reception */
#define DWT_SYS_CFG_FFAA            0x00000010UL
/* Frame Filtering Allow MAC command frame reception */
#define DWT_SYS_CFG_FFAM            0x00000020UL
/* Frame Filtering Allow Reserved frame types */
#define DWT_SYS_CFG_FFAR            0x00000040UL
/* Frame Filtering Allow frames with frame type field of 4, (binary 100) */
#define DWT_SYS_CFG_FFA4            0x00000080UL
/* Frame Filtering Allow frames with frame type field of 5, (binary 101) */
#define DWT_SYS_CFG_FFA5            0x00000100UL
/* Host interrupt polarity */
#define DWT_SYS_CFG_HIRQ_POL        0x00000200UL
/* SPI data launch edge */
#define DWT_SYS_CFG_SPI_EDGE        0x00000400UL
/* Disable frame check error handling */
#define DWT_SYS_CFG_DIS_FCE         0x00000800UL
/* Disable Double RX Buffer */
#define DWT_SYS_CFG_DIS_DRXB        0x00001000UL
/* Disable receiver abort on PHR error */
#define DWT_SYS_CFG_DIS_PHE         0x00002000UL
/* Disable Receiver Abort on RSD error */
#define DWT_SYS_CFG_DIS_RSDE        0x00004000UL
/* initial seed value for the FCS generation and checking function */
#define DWT_SYS_CFG_FCS_INIT2F      0x00008000UL
#define DWT_SYS_CFG_PHR_MODE_SHFT   16
/* Standard Frame mode */
#define DWT_SYS_CFG_PHR_MODE_00     0x00000000UL
/* Long Frames mode */
#define DWT_SYS_CFG_PHR_MODE_11     0x00030000UL
/* Disable Smart TX Power control */
#define DWT_SYS_CFG_DIS_STXP        0x00040000UL
/* Receiver Mode 110 kbps data rate */
#define DWT_SYS_CFG_RXM110K         0x00400000UL
/* Receive Wait Timeout Enable. */
#define DWT_SYS_CFG_RXWTOE          0x10000000UL
/*
 * Receiver Auto-Re-enable.
 * This bit is used to cause the receiver to re-enable automatically
 */
#define DWT_SYS_CFG_RXAUTR          0x20000000UL
/* Automatic Acknowledgement Enable */
#define DWT_SYS_CFG_AUTOACK         0x40000000UL
/* Automatic Acknowledgement Pending bit control */
#define DWT_SYS_CFG_AACKPEND        0x80000000UL

/* System Time Counter (40-bit) */
#define DWT_SYS_TIME_ID             0x06
#define DWT_SYS_TIME_OFFSET         0x00
/* Note 40 bit register */
#define DWT_SYS_TIME_LEN            5

#define DWT_REG_07_ID_RESERVED      0x07

/* Transmit Frame Control */
#define DWT_TX_FCTRL_ID             0x08
/* Note 40 bit register */
#define DWT_TX_FCTRL_LEN            5
/* Bit mask to access Transmit Frame Length */
#define DWT_TX_FCTRL_TFLEN_MASK     0x0000007FUL
/* Bit mask to access Transmit Frame Length Extension */
#define DWT_TX_FCTRL_TFLE_MASK      0x00000380UL
/* Bit mask to access Frame Length field */
#define DWT_TX_FCTRL_FLE_MASK       0x000003FFUL
/* Bit mask to access Transmit Bit Rate */
#define DWT_TX_FCTRL_TXBR_MASK      0x00006000UL
/* Bit mask to access Transmit Pulse Repetition Frequency */
#define DWT_TX_FCTRL_TXPRF_MASK     0x00030000UL
/* Bit mask to access Transmit Preamble Symbol Repetitions (PSR). */
#define DWT_TX_FCTRL_TXPSR_MASK     0x000C0000UL
/* Bit mask to access Preamble Extension */
#define DWT_TX_FCTRL_PE_MASK        0x00300000UL
/* Bit mask to access Transmit Preamble Symbol Repetitions (PSR). */
#define DWT_TX_FCTRL_TXPSR_PE_MASK  0x003C0000UL
/* FSCTRL has fields which should always be written zero */
#define DWT_TX_FCTRL_SAFE_MASK_32   0xFFFFE3FFUL
/* Transmit Bit Rate = 110k */
#define DWT_TX_FCTRL_TXBR_110k      0x00000000UL
/* Transmit Bit Rate = 850k */
#define DWT_TX_FCTRL_TXBR_850k      0x00002000UL
/* Transmit Bit Rate = 6.8M */
#define DWT_TX_FCTRL_TXBR_6M        0x00004000UL
/* Shift to access Data Rate field */
#define DWT_TX_FCTRL_TXBR_SHFT      13
/* Transmit Ranging enable */
#define DWT_TX_FCTRL_TR             0x00008000UL
/* Shift to access Ranging bit */
#define DWT_TX_FCTRL_TR_SHFT        15
/* Shift to access Pulse Repetition Frequency field */
#define DWT_TX_FCTRL_TXPRF_SHFT     16
/* Transmit Pulse Repetition Frequency = 4 Mhz */
#define DWT_TX_FCTRL_TXPRF_4M       0x00000000UL
/* Transmit Pulse Repetition Frequency = 16 Mhz */
#define DWT_TX_FCTRL_TXPRF_16M      0x00010000UL
/* Transmit Pulse Repetition Frequency = 64 Mhz */
#define DWT_TX_FCTRL_TXPRF_64M      0x00020000UL
/* Shift to access Preamble Symbol Repetitions field */
#define DWT_TX_FCTRL_TXPSR_SHFT     18
/*
 * shift to access Preamble length Extension to allow specification
 * of non-standard values
 */
#define DWT_TX_FCTRL_PE_SHFT        20
/* Bit mask to access Preamble Extension = 16 */
#define DWT_TX_FCTRL_TXPSR_PE_16    0x00000000UL
/* Bit mask to access Preamble Extension = 64 */
#define DWT_TX_FCTRL_TXPSR_PE_64    0x00040000UL
/* Bit mask to access Preamble Extension = 128 */
#define DWT_TX_FCTRL_TXPSR_PE_128   0x00140000UL
/* Bit mask to access Preamble Extension = 256 */
#define DWT_TX_FCTRL_TXPSR_PE_256   0x00240000UL
/* Bit mask to access Preamble Extension = 512 */
#define DWT_TX_FCTRL_TXPSR_PE_512   0x00340000UL
/* Bit mask to access Preamble Extension = 1024 */
#define DWT_TX_FCTRL_TXPSR_PE_1024  0x00080000UL
/* Bit mask to access Preamble Extension = 1536 */
#define DWT_TX_FCTRL_TXPSR_PE_1536  0x00180000UL
/* Bit mask to access Preamble Extension = 2048 */
#define DWT_TX_FCTRL_TXPSR_PE_2048  0x00280000UL
/* Bit mask to access Preamble Extension = 4096 */
#define DWT_TX_FCTRL_TXPSR_PE_4096  0x000C0000UL
/* Shift to access transmit buffer index offset */
#define DWT_TX_FCTRL_TXBOFFS_SHFT   22
/* Bit mask to access Transmit buffer index offset 10-bit field */
#define DWT_TX_FCTRL_TXBOFFS_MASK   0xFFC00000UL
/* Bit mask to access Inter-Frame Spacing field */
#define DWT_TX_FCTRL_IFSDELAY_MASK  0xFF00000000ULL

/* Transmit Data Buffer */
#define DWT_TX_BUFFER_ID            0x09
#define DWT_TX_BUFFER_LEN           1024

/* Delayed Send or Receive Time (40-bit) */
#define DWT_DX_TIME_ID              0x0A
#define DWT_DX_TIME_LEN             5

#define DWT_REG_0B_ID_RESERVED      0x0B

/* Receive Frame Wait Timeout Period */
#define DWT_RX_FWTO_ID              0x0C
#define DWT_RX_FWTO_OFFSET          0x00
#define DWT_RX_FWTO_LEN             2
#define DWT_RX_FWTO_MASK            0xFFFF

/* System Control Register */
#define DWT_SYS_CTRL_ID             0x0D
#define DWT_SYS_CTRL_OFFSET         0x00
#define DWT_SYS_CTRL_LEN            4
/*
 * System Control Register access mask
 * (all unused fields should always be written as zero)
 */
#define DWT_SYS_CTRL_MASK_32        0x010003CFUL
/* Suppress Auto-FCS Transmission (on this frame) */
#define DWT_SYS_CTRL_SFCST          0x00000001UL
/* Start Transmitting Now */
#define DWT_SYS_CTRL_TXSTRT         0x00000002UL
/* Transmitter Delayed Sending (initiates sending when SYS_TIME == TXD_TIME */
#define DWT_SYS_CTRL_TXDLYS         0x00000004UL
/* Cancel Suppression of auto-FCS transmission (on the current frame) */
#define DWT_SYS_CTRL_CANSFCS        0x00000008UL
/* Transceiver Off. Force Transciever OFF abort TX or RX immediately */
#define DWT_SYS_CTRL_TRXOFF         0x00000040UL
/* Wait for Response */
#define DWT_SYS_CTRL_WAIT4RESP      0x00000080UL
/* Enable Receiver Now */
#define DWT_SYS_CTRL_RXENAB         0x00000100UL
/*
 * Receiver Delayed Enable
 * (Enables Receiver when SY_TIME[0x??] == RXD_TIME[0x??] CHECK comment
 */
#define DWT_SYS_CTRL_RXDLYE         0x00000200UL
/*
 * Host side receiver buffer pointer toggle - toggles 0/1
 * host side data set pointer
 */
#define DWT_SYS_CTRL_HSRBTOGGLE     0x01000000UL
#define DWT_SYS_CTRL_HRBT           DWT_SYS_CTRL_HSRBTOGGLE
#define DWT_SYS_CTRL_HRBT_OFFSET    3

/* System Event Mask Register */
#define DWT_SYS_MASK_ID             0x0E
#define DWT_SYS_MASK_LEN            4
/*
 * System Event Mask Register access mask
 * (all unused fields should always be written as zero)
 */
#define DWT_SYS_MASK_MASK_32        0x3FF7FFFEUL
/* Mask clock PLL lock event */
#define DWT_SYS_MASK_MCPLOCK        0x00000002UL
/* Mask clock PLL lock event */
#define DWT_SYS_MASK_MESYNCR        0x00000004UL
/* Mask automatic acknowledge trigger event */
#define DWT_SYS_MASK_MAAT           0x00000008UL
/* Mask transmit frame begins event */
#define DWT_SYS_MASK_MTXFRB         0x00000010UL
/* Mask transmit preamble sent event */
#define DWT_SYS_MASK_MTXPRS         0x00000020UL
/* Mask transmit PHY Header Sent event */
#define DWT_SYS_MASK_MTXPHS         0x00000040UL
/* Mask transmit frame sent event */
#define DWT_SYS_MASK_MTXFRS         0x00000080UL
/* Mask receiver preamble detected event */
#define DWT_SYS_MASK_MRXPRD         0x00000100UL
/* Mask receiver SFD detected event */
#define DWT_SYS_MASK_MRXSFDD        0x00000200UL
/* Mask LDE processing done event */
#define DWT_SYS_MASK_MLDEDONE       0x00000400UL
/* Mask receiver PHY header detect event */
#define DWT_SYS_MASK_MRXPHD         0x00000800UL
/* Mask receiver PHY header error event */
#define DWT_SYS_MASK_MRXPHE         0x00001000UL
/* Mask receiver data frame ready event */
#define DWT_SYS_MASK_MRXDFR         0x00002000UL
/* Mask receiver FCS good event */
#define DWT_SYS_MASK_MRXFCG         0x00004000UL
/* Mask receiver FCS error event */
#define DWT_SYS_MASK_MRXFCE         0x00008000UL
/* Mask receiver Reed Solomon Frame Sync Loss event */
#define DWT_SYS_MASK_MRXRFSL        0x00010000UL
/* Mask Receive Frame Wait Timeout event */
#define DWT_SYS_MASK_MRXRFTO        0x00020000UL
/* Mask leading edge detection processing error event */
#define DWT_SYS_MASK_MLDEERR        0x00040000UL
/* Mask Receiver Overrun event */
#define DWT_SYS_MASK_MRXOVRR        0x00100000UL
/* Mask Preamble detection timeout event */
#define DWT_SYS_MASK_MRXPTO         0x00200000UL
/* Mask GPIO interrupt event */
#define DWT_SYS_MASK_MGPIOIRQ       0x00400000UL
/* Mask SLEEP to INIT event */
#define DWT_SYS_MASK_MSLP2INIT      0x00800000UL
/* Mask RF PLL Loosing Lock warning event */
#define DWT_SYS_MASK_MRFPLLLL       0x01000000UL
/* Mask Clock PLL Loosing Lock warning event */
#define DWT_SYS_MASK_MCPLLLL        0x02000000UL
/* Mask Receive SFD timeout event */
#define DWT_SYS_MASK_MRXSFDTO       0x04000000UL
/* Mask Half Period Delay Warning event */
#define DWT_SYS_MASK_MHPDWARN       0x08000000UL
/* Mask Transmit Buffer Error event */
#define DWT_SYS_MASK_MTXBERR        0x10000000UL
/* Mask Automatic Frame Filtering rejection event */
#define DWT_SYS_MASK_MAFFREJ        0x20000000UL

/* System event Status Register */
#define DWT_SYS_STATUS_ID           0x0F
#define DWT_SYS_STATUS_OFFSET       0x00
/* Note 40 bit register */
#define DWT_SYS_STATUS_LEN          5
/*
 * System event Status Register access mask
 * (all unused fields should always be written as zero)
 */
#define DWT_SYS_STATUS_MASK_32      0xFFF7FFFFUL
/* Interrupt Request Status READ ONLY */
#define DWT_SYS_STATUS_IRQS         0x00000001UL
/* Clock PLL Lock */
#define DWT_SYS_STATUS_CPLOCK       0x00000002UL
/* External Sync Clock Reset */
#define DWT_SYS_STATUS_ESYNCR       0x00000004UL
/* Automatic Acknowledge Trigger */
#define DWT_SYS_STATUS_AAT          0x00000008UL
/* Transmit Frame Begins */
#define DWT_SYS_STATUS_TXFRB        0x00000010UL
/* Transmit Preamble Sent */
#define DWT_SYS_STATUS_TXPRS        0x00000020UL
/* Transmit PHY Header Sent */
#define DWT_SYS_STATUS_TXPHS        0x00000040UL
/*
 * Transmit Frame Sent:
 * This is set when the transmitter has completed the sending of a frame
 */
#define DWT_SYS_STATUS_TXFRS        0x00000080UL
/* Receiver Preamble Detected status */
#define DWT_SYS_STATUS_RXPRD        0x00000100UL
/* Receiver Start Frame Delimiter Detected. */
#define DWT_SYS_STATUS_RXSFDD       0x00000200UL
/* LDE processing done */
#define DWT_SYS_STATUS_LDEDONE      0x00000400UL
/* Receiver PHY Header Detect */
#define DWT_SYS_STATUS_RXPHD        0x00000800UL
/* Receiver PHY Header Error */
#define DWT_SYS_STATUS_RXPHE        0x00001000UL
/* Receiver Data Frame Ready */
#define DWT_SYS_STATUS_RXDFR        0x00002000UL
/* Receiver FCS Good */
#define DWT_SYS_STATUS_RXFCG        0x00004000UL
/* Receiver FCS Error */
#define DWT_SYS_STATUS_RXFCE        0x00008000UL
/* Receiver Reed Solomon Frame Sync Loss */
#define DWT_SYS_STATUS_RXRFSL       0x00010000UL
/* Receive Frame Wait Timeout */
#define DWT_SYS_STATUS_RXRFTO       0x00020000UL
/* Leading edge detection processing error */
#define DWT_SYS_STATUS_LDEERR       0x00040000UL
/* bit19 reserved */
#define DWT_SYS_STATUS_reserved     0x00080000UL
/* Receiver Overrun */
#define DWT_SYS_STATUS_RXOVRR       0x00100000UL
/* Preamble detection timeout */
#define DWT_SYS_STATUS_RXPTO        0x00200000UL
/* GPIO interrupt */
#define DWT_SYS_STATUS_GPIOIRQ      0x00400000UL
/* SLEEP to INIT */
#define DWT_SYS_STATUS_SLP2INIT     0x00800000UL
/* RF PLL Losing Lock */
#define DWT_SYS_STATUS_RFPLL_LL     0x01000000UL
/* Clock PLL Losing Lock */
#define DWT_SYS_STATUS_CLKPLL_LL    0x02000000UL
/* Receive SFD timeout */
#define DWT_SYS_STATUS_RXSFDTO      0x04000000UL
/* Half Period Delay Warning */
#define DWT_SYS_STATUS_HPDWARN      0x08000000UL
/* Transmit Buffer Error */
#define DWT_SYS_STATUS_TXBERR       0x10000000UL
/* Automatic Frame Filtering rejection */
#define DWT_SYS_STATUS_AFFREJ       0x20000000UL
/* Host Side Receive Buffer Pointer */
#define DWT_SYS_STATUS_HSRBP        0x40000000UL
/* IC side Receive Buffer Pointer READ ONLY */
#define DWT_SYS_STATUS_ICRBP        0x80000000UL
/* Receiver Reed-Solomon Correction Status */
#define DWT_SYS_STATUS_RXRSCS       0x0100000000ULL
/* Receiver Preamble Rejection */
#define DWT_SYS_STATUS_RXPREJ       0x0200000000ULL
/* Transmit power up time error */
#define DWT_SYS_STATUS_TXPUTE       0x0400000000ULL
/*
 * These bits are the 16 high bits of status register TXPUTE and
 * HPDWARN flags
 */
#define DWT_SYS_STATUS_TXERR        (0x0408)
/* All RX events after a correct packet reception mask. */
#define DWT_SYS_STATUS_ALL_RX_GOOD (DWT_SYS_STATUS_RXDFR |  \
				    DWT_SYS_STATUS_RXFCG |  \
				    DWT_SYS_STATUS_RXPRD |  \
				    DWT_SYS_STATUS_RXSFDD | \
				    DWT_SYS_STATUS_RXPHD)
/* All TX events mask. */
#define DWT_SYS_STATUS_ALL_TX      (DWT_SYS_STATUS_AAT |   \
				    DWT_SYS_STATUS_TXFRB | \
				    DWT_SYS_STATUS_TXPRS | \
				    DWT_SYS_STATUS_TXPHS | \
				    DWT_SYS_STATUS_TXFRS)
/* All double buffer events mask. */
#define DWT_SYS_STATUS_ALL_DBLBUFF (DWT_SYS_STATUS_RXDFR | \
				    DWT_SYS_STATUS_RXFCG)
/* All RX errors mask. */
#define DWT_SYS_STATUS_ALL_RX_ERR  (DWT_SYS_STATUS_RXPHE |   \
				    DWT_SYS_STATUS_RXFCE |   \
				    DWT_SYS_STATUS_RXRFSL |  \
				    DWT_SYS_STATUS_RXOVRR |  \
				    DWT_SYS_STATUS_RXSFDTO | \
				    DWT_SYS_STATUS_AFFREJ)
#define DWT_SYS_MASK_ALL_RX_ERR  (DWT_SYS_MASK_MRXPHE |	  \
				  DWT_SYS_MASK_MRXFCE |	  \
				  DWT_SYS_MASK_MRXRFSL |  \
				  DWT_SYS_STATUS_RXOVRR | \
				  DWT_SYS_MASK_MRXSFDTO | \
				  DWT_SYS_MASK_MAFFREJ)
/*
 * User defined RX timeouts
 * (frame wait timeout and preamble detect timeout) mask.
 */
#define DWT_SYS_STATUS_ALL_RX_TO    (DWT_SYS_STATUS_RXRFTO | \
				     DWT_SYS_STATUS_RXPTO)
#define DWT_SYS_MASK_ALL_RX_TO      (DWT_SYS_MASK_MRXRFTO | \
				     DWT_SYS_MASK_MRXPTO)

/* RX Frame Information (in double buffer set) */
#define DWT_RX_FINFO_ID             0x10
#define DWT_RX_FINFO_OFFSET         0x00
#define DWT_RX_FINFO_LEN            4
/*
 * System event Status Register access mask
 * (all unused fields should always be written as zero)
 */
#define DWT_RX_FINFO_MASK_32        0xFFFFFBFFUL
/* Receive Frame Length (0 to 127) */
#define DWT_RX_FINFO_RXFLEN_MASK    0x0000007FUL
/* Receive Frame Length Extension (0 to 7)<<7 */
#define DWT_RX_FINFO_RXFLE_MASK     0x00000380UL
/* Receive Frame Length Extension (0 to 1023) */
#define DWT_RX_FINFO_RXFL_MASK_1023 0x000003FFUL

/* Receive Non-Standard Preamble Length */
#define DWT_RX_FINFO_RXNSPL_MASK    0x00001800UL
/*
 * RX Preamble Repetition.
 * 00 = 16 symbols, 01 = 64 symbols, 10 = 1024 symbols, 11 = 4096 symbols
 */
#define DWT_RX_FINFO_RXPSR_MASK     0x000C0000UL

/* Receive Preamble Length = RXPSR+RXNSPL */
#define DWT_RX_FINFO_RXPEL_MASK     0x000C1800UL
/* Receive Preamble length = 64 */
#define DWT_RX_FINFO_RXPEL_64       0x00040000UL
/* Receive Preamble length = 128 */
#define DWT_RX_FINFO_RXPEL_128      0x00040800UL
/* Receive Preamble length = 256 */
#define DWT_RX_FINFO_RXPEL_256      0x00041000UL
/* Receive Preamble length = 512 */
#define DWT_RX_FINFO_RXPEL_512      0x00041800UL
/* Receive Preamble length = 1024 */
#define DWT_RX_FINFO_RXPEL_1024     0x00080000UL
/* Receive Preamble length = 1536 */
#define DWT_RX_FINFO_RXPEL_1536     0x00080800UL
/* Receive Preamble length = 2048 */
#define DWT_RX_FINFO_RXPEL_2048     0x00081000UL
/* Receive Preamble length = 4096 */
#define DWT_RX_FINFO_RXPEL_4096     0x000C0000UL

/* Receive Bit Rate report. This field reports the received bit rate */
#define DWT_RX_FINFO_RXBR_MASK      0x00006000UL
/* Received bit rate = 110 kbps */
#define DWT_RX_FINFO_RXBR_110k      0x00000000UL
/* Received bit rate = 850 kbps */
#define DWT_RX_FINFO_RXBR_850k      0x00002000UL
/* Received bit rate = 6.8 Mbps */
#define DWT_RX_FINFO_RXBR_6M        0x00004000UL
#define DWT_RX_FINFO_RXBR_SHIFT     13

/*
 * Receiver Ranging. Ranging bit in the received PHY header
 * identifying the frame as a ranging packet.
 */
#define DWT_RX_FINFO_RNG            0x00008000UL
#define DWT_RX_FINFO_RNG_SHIFT      15

/* RX Pulse Repetition Rate report */
#define DWT_RX_FINFO_RXPRF_MASK     0x00030000UL
/* PRF being employed in the receiver = 16M */
#define DWT_RX_FINFO_RXPRF_16M      0x00010000UL
/* PRF being employed in the receiver = 64M */
#define DWT_RX_FINFO_RXPRF_64M      0x00020000UL
#define DWT_RX_FINFO_RXPRF_SHIFT    16

/* Preamble Accumulation Count */
#define DWT_RX_FINFO_RXPACC_MASK    0xFFF00000UL
#define DWT_RX_FINFO_RXPACC_SHIFT   20

/* Receive Data Buffer (in double buffer set) */
#define DWT_RX_BUFFER_ID            0x11
#define DWT_RX_BUFFER_LEN           1024

/* Rx Frame Quality information (in double buffer set) */
#define DWT_RX_FQUAL_ID             0x12
/* Note 64 bit register */
#define DWT_RX_FQUAL_LEN            8
/* Standard Deviation of Noise */
#define DWT_RX_EQUAL_STD_NOISE_MASK 0x0000FFFFULL
#define DWT_RX_EQUAL_STD_NOISE_SHIFT 0
#define DWT_STD_NOISE_MASK          DWT_RX_EQUAL_STD_NOISE_MASK
#define DWT_STD_NOISE_SHIFT         DWT_RX_EQUAL_STD_NOISE_SHIFT
/* First Path Amplitude point 2 */
#define DWT_RX_EQUAL_FP_AMPL2_MASK  0xFFFF0000ULL
#define DWT_RX_EQUAL_FP_AMPL2_SHIFT 16
#define DWT_FP_AMPL2_MASK           DWT_RX_EQUAL_FP_AMPL2_MASK
#define DWT_FP_AMPL2_SHIFT          DWT_RX_EQUAL_FP_AMPL2_SHIFT
/* First Path Amplitude point 3 */
#define DWT_RX_EQUAL_PP_AMPL3_MASK  0x0000FFFF00000000ULL
#define DWT_RX_EQUAL_PP_AMPL3_SHIFT 32
#define DWT_PP_AMPL3_MASK           DWT_RX_EQUAL_PP_AMPL3_MASK
#define DWT_PP_AMPL3_SHIFT          DWT_RX_EQUAL_PP_AMPL3_SHIFT
/* Channel Impulse Response Max Growth */
#define DWT_RX_EQUAL_CIR_MXG_MASK   0xFFFF000000000000ULL
#define DWT_RX_EQUAL_CIR_MXG_SHIFT  48
#define DWT_CIR_MXG_MASK            DWT_RX_EQUAL_CIR_MXG_MASK
#define DWT_CIR_MXG_SHIFT           DWT_RX_EQUAL_CIR_MXG_SHIFT

/* Receiver Time Tracking Interval (in double buffer set) */
#define DWT_RX_TTCKI_ID             0x13
#define DWT_RX_TTCKI_LEN            4

/* Receiver Time Tracking Offset (in double buffer set) */
#define DWT_RX_TTCKO_ID             0x14
/* Note 40 bit register */
#define DWT_RX_TTCKO_LEN            5
/*
 * Receiver Time Tracking Offset access mask
 * (all unused fields should always be written as zero)
 */
#define DWT_RX_TTCKO_MASK_32        0xFF07FFFFUL
/* RX time tracking offset. This RXTOFS value is a 19-bit signed quantity */
#define DWT_RX_TTCKO_RXTOFS_MASK    0x0007FFFFUL
/* This 8-bit field reports an internal re-sampler delay value */
#define DWT_RX_TTCKO_RSMPDEL_MASK   0xFF000000UL
/*
 * This 7-bit field reports the receive carrier phase adjustment
 * at time the ranging timestamp is made.
 */
#define DWT_RX_TTCKO_RCPHASE_MASK   0x7F0000000000ULL

/* Receive Message Time of Arrival (in double buffer set) */
#define DWT_RX_TIME_ID              0x15
#define DWT_RX_TIME_LLEN            14
/* read only 5 bytes (the adjusted timestamp (40:0)) */
#define DWT_RX_TIME_RX_STAMP_LEN    5
#define DWT_RX_STAMP_LEN            DWT_RX_TIME_RX_STAMP_LEN
/* byte 0..4 40 bit Reports the fully adjusted time of reception. */
#define DWT_RX_TIME_RX_STAMP_OFFSET  0
/* byte 5..6 16 bit First path index. */
#define DWT_RX_TIME_FP_INDEX_OFFSET  5
/* byte 7..8 16 bit First Path Amplitude point 1 */
#define DWT_RX_TIME_FP_AMPL1_OFFSET  7
/* byte 9..13 40 bit Raw Timestamp for the frame */
#define DWT_RX_TIME_FP_RAWST_OFFSET  9

#define DWT_REG_16_ID_RESERVED      0x16

/* Transmit Message Time of Sending */
#define DWT_TX_TIME_ID              0x17
#define DWT_TX_TIME_LLEN            10
/* 40-bits = 5 bytes */
#define DWT_TX_TIME_TX_STAMP_LEN    5
#define DWT_TX_STAMP_LEN            DWT_TX_TIME_TX_STAMP_LEN
/* byte 0..4 40 bit Reports the fully adjusted time of transmission */
#define DWT_TX_TIME_TX_STAMP_OFFSET  0
/* byte 5..9 40 bit Raw Timestamp for the frame */
#define DWT_TX_TIME_TX_RAWST_OFFSET  5

/* 16-bit Delay from Transmit to Antenna */
#define DWT_TX_ANTD_ID              0x18
#define DWT_TX_ANTD_OFFSET          0x00
#define DWT_TX_ANTD_LEN             2

/* System State information READ ONLY */
#define DWT_SYS_STATE_ID            0x19
#define DWT_SYS_STATE_LEN           5

/* 7:0 TX _STATE Bits 3:0 */
#define DWT_TX_STATE_OFFSET         0x00
#define DWT_TX_STATE_MASK           0x07
#define DWT_TX_STATE_IDLE           0x00
#define DWT_TX_STATE_PREAMBLE       0x01
#define DWT_TX_STATE_SFD            0x02
#define DWT_TX_STATE_PHR            0x03
#define DWT_TX_STATE_SDE            0x04
#define DWT_TX_STATE_DATA           0x05
#define DWT_TX_STATE_RSP_DATE       0x06
#define DWT_TX_STATE_TAIL           0x07

#define DWT_RX_STATE_OFFSET         0x01
#define DWT_RX_STATE_IDLE           0x00
#define DWT_RX_STATE_START_ANALOG   0x01
#define DWT_RX_STATE_RX_RDY         0x04
#define DWT_RX_STATE_PREAMBLE_FOUND 0x05
#define DWT_RX_STATE_PRMBL_TIMEOUT  0x06
#define DWT_RX_STATE_SFD_FOUND      0x07
#define DWT_RX_STATE_CNFG_PHR_RX    0x08
#define DWT_RX_STATE_PHR_RX_STRT    0x09
#define DWT_RX_STATE_DATA_RATE_RDY  0x0A
#define DWT_RX_STATE_DATA_RX_SEQ    0x0C
#define DWT_RX_STATE_CNFG_DATA_RX   0x0D
#define DWT_RX_STATE_PHR_NOT_OK     0x0E
#define DWT_RX_STATE_LAST_SYMBOL    0x0F
#define DWT_RX_STATE_WAIT_RSD_DONE  0x10
#define DWT_RX_STATE_RSD_OK         0x11
#define DWT_RX_STATE_RSD_NOT_OK     0x12
#define DWT_RX_STATE_RECONFIG_110   0x13
#define DWT_RX_STATE_WAIT_110_PHR   0x14

#define DWT_PMSC_STATE_OFFSET       0x02
#define DWT_PMSC_STATE_INIT         0x00
#define DWT_PMSC_STATE_IDLE         0x01
#define DWT_PMSC_STATE_TX_WAIT      0x02
#define DWT_PMSC_STATE_RX_WAIT      0x03
#define DWT_PMSC_STATE_TX           0x04
#define DWT_PMSC_STATE_RX           0x05

/*
 * Acknowledge (31:24 preamble symbol delay before auto ACK is sent) and
 * response (19:0 - unit 1us) timer
 */
/* Acknowledgement Time and Response Time */
#define DWT_ACK_RESP_T_ID           0x1A
#define DWT_ACK_RESP_T_LEN          4
/* Acknowledgement Time and Response access mask */
#define DWT_ACK_RESP_T_MASK         0xFF0FFFFFUL
#define DWT_ACK_RESP_T_W4R_TIM_OFFSET 0
/* Wait-for-Response turn-around Time 20 bit field */
#define DWT_ACK_RESP_T_W4R_TIM_MASK 0x000FFFFFUL
#define DWT_W4R_TIM_MASK            DWT_ACK_RESP_T_W4R_TIM_MASK
#define DWT_ACK_RESP_T_ACK_TIM_OFFSET 3
/* Auto-Acknowledgement turn-around Time */
#define DWT_ACK_RESP_T_ACK_TIM_MASK 0xFF000000UL
#define DWT_ACK_TIM_MASK            DWT_ACK_RESP_T_ACK_TIM_MASK

#define DWT_REG_1B_ID_RESERVED      0x1B
#define DWT_REG_1C_ID_RESERVED      0x1C

/* Sniff Mode Configuration */
#define DWT_RX_SNIFF_ID             0x1D
#define DWT_RX_SNIFF_OFFSET         0x00
#define DWT_RX_SNIFF_LEN            4
#define DWT_RX_SNIFF_MASK           0x0000FF0FUL
/* SNIFF Mode ON time. Specified in units of PAC */
#define DWT_RX_SNIFF_SNIFF_ONT_MASK 0x0000000FUL
#define DWT_SNIFF_ONT_MASK          DWT_RX_SNIFF_SNIFF_ONT_MASK
/*
 * SNIFF Mode OFF time specified in units of approximately 1mkS,
 * or 128 system clock cycles.
 */
#define DWT_RX_SNIFF_SNIFF_OFFT_MASK 0x0000FF00UL
#define DWT_SNIFF_OFFT_MASK         DWT_RX_SNIFF_SNIFF_OFFT_MASK

/* TX Power Control */
#define DWT_TX_POWER_ID             0x1E
#define DWT_TX_POWER_LEN            4
/*
 * Mask and shift definition for Smart Transmit Power Control:
 *
 * This is the normal power setting used for frames that do not fall.
 */
#define DWT_TX_POWER_BOOSTNORM_MASK 0x00000000UL
#define DWT_BOOSTNORM_MASK          DWT_TX_POWER_BOOSTNORM_MASK
#define DWT_TX_POWER_BOOSTNORM_SHIFT 0
/*
 * This value sets the power applied during transmission
 * at the 6.8 Mbps data rate frames that are less than 0.5 ms duration
 */
#define DWT_TX_POWER_BOOSTP500_MASK 0x00000000UL
#define DWT_BOOSTP500_MASK          DWT_TX_POWER_BOOSTP500_MASK
#define DWT_TX_POWER_BOOSTP500_SHIFT 8
/*
 * This value sets the power applied during transmission
 * at the 6.8 Mbps data rate frames that are less than 0.25 ms duration
 */
#define DWT_TX_POWER_BOOSTP250_MASK 0x00000000UL
#define DWT_BOOSTP250_MASK          DWT_TX_POWER_BOOSTP250_MASK
#define DWT_TX_POWER_BOOSTP250_SHIFT 16
/*
 * This value sets the power applied during transmission
 * at the 6.8 Mbps data rate frames that are less than 0.125 ms
 */
#define DWT_TX_POWER_BOOSTP125_MASK 0x00000000UL
#define DWT_BOOSTP125_MASK          DWT_TX_POWER_BOOSTP125_MASK
#define DWT_TX_POWER_BOOSTP125_SHIFT 24
/*
 * Mask and shift definition for Manual Transmit Power Control
 * (DIS_STXP=1 in SYS_CFG)
 */
#define DWT_TX_POWER_MAN_DEFAULT    0x0E080222UL
/*
 * This power setting is applied during the transmission
 * of the PHY header (PHR) portion of the frame.
 */
#define DWT_TX_POWER_TXPOWPHR_MASK  0x0000FF00UL
/*
 * This power setting is applied during the transmission
 * of the synchronisation header (SHR) and data portions of the frame.
 */
#define DWT_TX_POWER_TXPOWSD_MASK   0x00FF0000UL

/* Channel Control */
#define DWT_CHAN_CTRL_ID            0x1F
#define DWT_CHAN_CTRL_LEN           4
/* Channel Control Register access mask */
#define DWT_CHAN_CTRL_MASK          0xFFFF00FFUL
/* Supported channels are 1, 2, 3, 4, 5, and 7. */
#define DWT_CHAN_CTRL_TX_CHAN_MASK  0x0000000FUL
/* Bits 0..3        TX channel number 0-15 selection */
#define DWT_CHAN_CTRL_TX_CHAN_SHIFT 0
#define DWT_CHAN_CTRL_RX_CHAN_MASK  0x000000F0UL
/* Bits 4..7        RX channel number 0-15 selection */
#define DWT_CHAN_CTRL_RX_CHAN_SHIFT 4
/*
 * Bits 18..19      Specify (Force) RX Pulse Repetition Rate:
 *		    00 = 4 MHz, 01 = 16 MHz, 10 = 64MHz.
 */
#define DWT_CHAN_CTRL_RXFPRF_MASK   0x000C0000UL
#define DWT_CHAN_CTRL_RXFPRF_SHIFT  18
/*
 * Specific RXFPRF configuration:
 *
 * Specify (Force) RX Pulse Repetition Rate:
 *		    00 = 4 MHz, 01 = 16 MHz, 10 = 64MHz.
 */
#define DWT_CHAN_CTRL_RXFPRF_4      0x00000000UL
/*
 * Specify (Force) RX Pulse Repetition Rate:
 *		    00 = 4 MHz, 01 = 16 MHz, 10 = 64MHz.
 */
#define DWT_CHAN_CTRL_RXFPRF_16     0x00040000UL
/*
 * Specify (Force) RX Pulse Repetition Rate:
 *		    00 = 4 MHz, 01 = 16 MHz, 10 = 64MHz.
 */
#define DWT_CHAN_CTRL_RXFPRF_64     0x00080000UL
/* Bits 22..26      TX Preamble Code selection, 1 to 24. */
#define DWT_CHAN_CTRL_TX_PCOD_MASK  0x07C00000UL
#define DWT_CHAN_CTRL_TX_PCOD_SHIFT 22
/* Bits 27..31      RX Preamble Code selection, 1 to 24. */
#define DWT_CHAN_CTRL_RX_PCOD_MASK  0xF8000000UL
#define DWT_CHAN_CTRL_RX_PCOD_SHIFT 27
/* Bit 17 This bit enables a non-standard DecaWave proprietary SFD sequence. */
#define DWT_CHAN_CTRL_DWSFD         0x00020000UL
#define DWT_CHAN_CTRL_DWSFD_SHIFT   17
/* Bit 20 Non-standard SFD in the transmitter */
#define DWT_CHAN_CTRL_TNSSFD        0x00100000UL
#define DWT_CHAN_CTRL_TNSSFD_SHIFT  20
/* Bit 21 Non-standard SFD in the receiver */
#define DWT_CHAN_CTRL_RNSSFD        0x00200000UL
#define DWT_CHAN_CTRL_RNSSFD_SHIFT  21

#define DWT_REG_20_ID_RESERVED      0x20

/* User-specified short/long TX/RX SFD sequences */
#define DWT_USR_SFD_ID              0x21
#define DWT_USR_SFD_LEN             41
/* Decawave non-standard SFD length for 110 kbps */
#define DWT_DW_NS_SFD_LEN_110K      64
/* Decawave non-standard SFD length for 850 kbps */
#define DWT_DW_NS_SFD_LEN_850K      16
/* Decawave non-standard SFD length for 6.8 Mbps */
#define DWT_DW_NS_SFD_LEN_6M8       8

#define DWT_REG_22_ID_RESERVED      0x22

/* Automatic Gain Control configuration */
#define DWT_AGC_CTRL_ID             0x23
#define DWT_AGC_CTRL_LEN            32
#define DWT_AGC_CFG_STS_ID          DWT_AGC_CTRL_ID
#define DWT_AGC_CTRL1_OFFSET        (0x02)
#define DWT_AGC_CTRL1_LEN           2
/* Access mask to AGC configuration and control register */
#define DWT_AGC_CTRL1_MASK          0x0001
/* Disable AGC Measurement. The DIS_AM bit is set by default. */
#define DWT_AGC_CTRL1_DIS_AM        0x0001
/*
 * Offset from AGC_CTRL_ID in bytes.
 * Please take care not to write other values to this register as doing so
 * may cause the DW1000 to malfunction
 */
#define DWT_AGC_TUNE1_OFFSET        (0x04)
#define DWT_AGC_TUNE1_LEN           2
/* It is a 16-bit tuning register for the AGC. */
#define DWT_AGC_TUNE1_MASK          0xFFFF
#define DWT_AGC_TUNE1_16M           0x8870
#define DWT_AGC_TUNE1_64M           0x889B
/*
 * Offset from AGC_CTRL_ID in bytes.
 * Please take care not to write other values to this register as doing so
 * may cause the DW1000 to malfunction
 */
#define DWT_AGC_TUNE2_OFFSET        (0x0C)
#define DWT_AGC_TUNE2_LEN           4
#define DWT_AGC_TUNE2_MASK          0xFFFFFFFFUL
#define DWT_AGC_TUNE2_VAL           0X2502A907UL
/*
 * Offset from AGC_CTRL_ID in bytes.
 * Please take care not to write other values to this register as doing so
 * may cause the DW1000 to malfunction
 */
#define DWT_AGC_TUNE3_LEN           2
#define DWT_AGC_TUNE3_MASK          0xFFFF
#define DWT_AGC_TUNE3_VAL           0X0055
#define DWT_AGC_STAT1_OFFSET        (0x1E)
#define DWT_AGC_STAT1_LEN           3
#define DWT_AGC_STAT1_MASK          0x0FFFFF
/* This 5-bit gain value relates to input noise power measurement. */
#define DWT_AGC_STAT1_EDG1_MASK     0x0007C0
/* This 9-bit value relates to the input noise power measurement. */
#define DWT_AGC_STAT1_EDG2_MASK     0x0FF800

/* External synchronisation control */
#define DWT_EXT_SYNC_ID             0x24
#define DWT_EXT_SYNC_LEN            12
#define DWT_EC_CTRL_OFFSET          (0x00)
#define DWT_EC_CTRL_LEN             4
/*
 * Sub-register 0x00 is the External clock synchronisation counter
 * configuration register
 */
#define DWT_EC_CTRL_MASK            0x00000FFBUL
/* External transmit synchronisation mode enable */
#define DWT_EC_CTRL_OSTSM           0x00000001UL
/* External receive synchronisation mode enable */
#define DWT_EC_CTRL_OSRSM           0x00000002UL
/* PLL lock detect enable */
#define DWT_EC_CTRL_PLLLCK          0x04
/* External timebase reset mode enable */
#define DWT_EC_CTRL_OSTRM           0x00000800UL
/*
 * Wait counter used for external transmit synchronisation and
 * external timebase reset
 */
#define DWT_EC_CTRL_WAIT_MASK       0x000007F8UL
#define DWT_EC_RXTC_OFFSET          (0x04)
#define DWT_EC_RXTC_LEN             4
/* External clock synchronisation counter captured on RMARKER */
#define DWT_EC_RXTC_MASK            0xFFFFFFFFUL
#define DWT_EC_GOLP                 (0x08)
#define DWT_EC_GOLP_LEN             4
/*
 * Sub-register 0x08 is the External clock offset to first path 1 GHz counter,
 * EC_GOLP
 */
#define DWT_EC_GOLP_MASK            0x0000003FUL
/*
 * This register contains the 1 GHz count from the arrival of the RMARKER and
 * the next edge of the external clock.
 */
#define DWT_EC_GOLP_OFFSET_EXT_MASK 0x0000003FUL

/* Read access to accumulator data */
#define DWT_ACC_MEM_ID              0x25
#define DWT_ACC_MEM_LEN             4064

/* Peripheral register bus 1 access - GPIO control */
#define DWT_GPIO_CTRL_ID            0x26
#define DWT_GPIO_CTRL_LEN           44

/* Sub-register 0x00 is the GPIO Mode Control Register */
#define DWT_GPIO_MODE_OFFSET        0x00
#define DWT_GPIO_MODE_LEN           4
#define DWT_GPIO_MODE_MASK          0x00FFFFC0UL

/* Mode Selection for GPIO0/RXOKLED */
#define DWT_GPIO_MSGP0_MASK         0x000000C0UL
/* Mode Selection for GPIO1/SFDLED */
#define DWT_GPIO_MSGP1_MASK         0x00000300UL
/* Mode Selection for GPIO2/RXLED */
#define DWT_GPIO_MSGP2_MASK         0x00000C00UL
/* Mode Selection for GPIO3/TXLED */
#define DWT_GPIO_MSGP3_MASK         0x00003000UL
/* Mode Selection for GPIO4/EXTPA */
#define DWT_GPIO_MSGP4_MASK         0x0000C000UL
/* Mode Selection for GPIO5/EXTTXE */
#define DWT_GPIO_MSGP5_MASK         0x00030000UL
/* Mode Selection for GPIO6/EXTRXE */
#define DWT_GPIO_MSGP6_MASK         0x000C0000UL
/* Mode Selection for SYNC/GPIO7 */
#define DWT_GPIO_MSGP7_MASK         0x00300000UL
/* Mode Selection for IRQ/GPIO8 */
#define DWT_GPIO_MSGP8_MASK         0x00C00000UL

/* The pin operates as the RXLED output */
#define DWT_GPIO_PIN2_RXLED         0x00000400UL
/* The pin operates as the TXLED output */
#define DWT_GPIO_PIN3_TXLED         0x00001000UL
/* The pin operates as the EXTPA output */
#define DWT_GPIO_PIN4_EXTPA         0x00004000UL
/* The pin operates as the EXTTXE output */
#define DWT_GPIO_PIN5_EXTTXE        0x00010000UL
/* The pin operates as the EXTRXE output */
#define DWT_GPIO_PIN6_EXTRXE        0x00040000UL

/* Sub-register 0x08 is the GPIO Direction Control Register */
#define DWT_GPIO_DIR_OFFSET         0x08
#define DWT_GPIO_DIR_LEN            3
#define DWT_GPIO_DIR_MASK           0x0011FFFFUL

/*
 * GPIO0 only changed if the GxM0 mask bit has a value of 1
 * for the write operation
 */
#define DWT_GxP0                    0x00000001UL
/* GPIO1. (See GDP0). */
#define DWT_GxP1                    0x00000002UL
/* GPIO2. (See GDP0). */
#define DWT_GxP2                    0x00000004UL
/* GPIO3. (See GDP0). */
#define DWT_GxP3                    0x00000008UL
/* GPIO4. (See GDP0). */
#define DWT_GxP4                    0x00000100UL
/* GPIO5. (See GDP0). */
#define DWT_GxP5                    0x00000200UL
/* GPIO6. (See GDP0). */
#define DWT_GxP6                    0x00000400UL
/* GPIO7. (See GDP0). */
#define DWT_GxP7                    0x00000800UL
/* GPIO8 */
#define DWT_GxP8                    0x00010000UL

/* Mask for GPIO0 */
#define DWT_GxM0                    0x00000010UL
/* Mask for GPIO1. (See GDM0). */
#define DWT_GxM1                    0x00000020UL
/* Mask for GPIO2. (See GDM0). */
#define DWT_GxM2                    0x00000040UL
/* Mask for GPIO3. (See GDM0). */
#define DWT_GxM3                    0x00000080UL
/* Mask for GPIO4. (See GDM0). */
#define DWT_GxM4                    0x00001000UL
/* Mask for GPIO5. (See GDM0). */
#define DWT_GxM5                    0x00002000UL
/* Mask for GPIO6. (See GDM0). */
#define DWT_GxM6                    0x00004000UL
/* Mask for GPIO7. (See GDM0). */
#define DWT_GxM7                    0x00008000UL
/* Mask for GPIO8. (See GDM0). */
#define DWT_GxM8                    0x00100000UL

/*
 * Direction Selection for GPIO0. 1 = input, 0 = output.
 * Only changed if the GDM0 mask bit has a value of 1 for the write operation
 */
#define DWT_GDP0    GxP0
/* Direction Selection for GPIO1. (See GDP0). */
#define DWT_GDP1    GxP1
/* Direction Selection for GPIO2. (See GDP0). */
#define DWT_GDP2    GxP2
/* Direction Selection for GPIO3. (See GDP0). */
#define DWT_GDP3    GxP3
/* Direction Selection for GPIO4. (See GDP0). */
#define DWT_GDP4    GxP4
/* Direction Selection for GPIO5. (See GDP0). */
#define DWT_GDP5    GxP5
/* Direction Selection for GPIO6. (See GDP0). */
#define DWT_GDP6    GxP6
/* Direction Selection for GPIO7. (See GDP0). */
#define DWT_GDP7    GxP7
/* Direction Selection for GPIO8 */
#define DWT_GDP8    GxP8

/* Mask for setting the direction of GPIO0 */
#define DWT_GDM0    GxM0
/* Mask for setting the direction of GPIO1. (See GDM0). */
#define DWT_GDM1    GxM1
/* Mask for setting the direction of GPIO2. (See GDM0). */
#define DWT_GDM2    GxM2
/* Mask for setting the direction of GPIO3. (See GDM0). */
#define DWT_GDM3    GxM3
/* Mask for setting the direction of GPIO4. (See GDM0). */
#define DWT_GDM4    GxM4
/* Mask for setting the direction of GPIO5. (See GDM0). */
#define DWT_GDM5    GxM5
/* Mask for setting the direction of GPIO6. (See GDM0). */
#define DWT_GDM6    GxM6
/* Mask for setting the direction of GPIO7. (See GDM0). */
#define DWT_GDM7    GxM7
/* Mask for setting the direction of GPIO8. (See GDM0). */
#define DWT_GDM8    GxM8

/* Sub-register 0x0C is the GPIO data output register. */
#define DWT_GPIO_DOUT_OFFSET        0x0C
#define DWT_GPIO_DOUT_LEN           3
#define DWT_GPIO_DOUT_MASK          DWT_GPIO_DIR_MASK

/* Sub-register 0x10 is the GPIO interrupt enable register */
#define DWT_GPIO_IRQE_OFFSET        0x10
#define DWT_GPIO_IRQE_LEN           4
#define DWT_GPIO_IRQE_MASK          0x000001FFUL
/* IRQ bit0 */
#define DWT_GIRQx0                  0x00000001UL
/* IRQ bit1 */
#define DWT_GIRQx1                  0x00000002UL
/* IRQ bit2 */
#define DWT_GIRQx2                  0x00000004UL
/* IRQ bit3 */
#define DWT_GIRQx3                  0x00000008UL
/* IRQ bit4 */
#define DWT_GIRQx4                  0x00000010UL
/* IRQ bit5 */
#define DWT_GIRQx5                  0x00000020UL
/* IRQ bit6 */
#define DWT_GIRQx6                  0x00000040UL
/* IRQ bit7 */
#define DWT_GIRQx7                  0x00000080UL
/* IRQ bit8 */
#define DWT_GIRQx8                  0x00000100UL
/* GPIO IRQ Enable for GPIO0 input. Value 1 = enable, 0 = disable */
#define DWT_GIRQE0  GIRQx0
#define DWT_GIRQE1  GIRQx1
#define DWT_GIRQE2  GIRQx2
#define DWT_GIRQE3  GIRQx3
#define DWT_GIRQE4  GIRQx4
#define DWT_GIRQE5  GIRQx5
#define DWT_GIRQE6  GIRQx6
#define DWT_GIRQE7  GIRQx7
#define DWT_GIRQE8  GIRQx8

/* Sub-register 0x14 is the GPIO interrupt sense selection register */
#define DWT_GPIO_ISEN_OFFSET        0x14
#define DWT_GPIO_ISEN_LEN           4
#define DWT_GPIO_ISEN_MASK          DWT_GPIO_IRQE_MASK
/* GPIO IRQ Sense selection GPIO0 input.
 * Value 0 = High or Rising-Edge,
 * 1 = Low or falling-edge.
 */
#define DWT_GISEN0  GIRQx0
#define DWT_GISEN1  GIRQx1
#define DWT_GISEN2  GIRQx2
#define DWT_GISEN3  GIRQx3
#define DWT_GISEN4  GIRQx4
#define DWT_GISEN5  GIRQx5
#define DWT_GISEN6  GIRQx6
#define DWT_GISEN7  GIRQx7
#define DWT_GISEN8  GIRQx8

/* Sub-register 0x18 is the GPIO interrupt mode selection register */
#define DWT_GPIO_IMODE_OFFSET       0x18
#define DWT_GPIO_IMODE_LEN          4
#define DWT_GPIO_IMODE_MASK         DWT_GPIO_IRQE_MASK
/* GPIO IRQ Mode selection for GPIO0 input.
 * Value 0 = Level sensitive interrupt.
 * Value 1 = Edge triggered interrupt
 */
#define DWT_GIMOD0  GIRQx0
#define DWT_GIMOD1  GIRQx1
#define DWT_GIMOD2  GIRQx2
#define DWT_GIMOD3  GIRQx3
#define DWT_GIMOD4  GIRQx4
#define DWT_GIMOD5  GIRQx5
#define DWT_GIMOD6  GIRQx6
#define DWT_GIMOD7  GIRQx7
#define DWT_GIMOD8  GIRQx8

/* Sub-register 0x1C is the GPIO interrupt "Both Edge" selection register */
#define DWT_GPIO_IBES_OFFSET        0x1C
#define DWT_GPIO_IBES_LEN           4
#define DWT_GPIO_IBES_MASK          DWT_GPIO_IRQE_MASK
/* GPIO IRQ "Both Edge" selection for GPIO0 input.
 * Value 0 = GPIO_IMODE register selects the edge.
 * Value 1 = Both edges trigger the interrupt.
 */
#define DWT_GIBES0  GIRQx0
#define DWT_GIBES1  GIRQx1
#define DWT_GIBES2  GIRQx2
#define DWT_GIBES3  GIRQx3
#define DWT_GIBES4  GIRQx4
#define DWT_GIBES5  GIRQx5
#define DWT_GIBES6  GIRQx6
#define DWT_GIBES7  GIRQx7
#define DWT_GIBES8  GIRQx8

/* Sub-register 0x20 is the GPIO interrupt clear register */
#define DWT_GPIO_ICLR_OFFSET        0x20
#define DWT_GPIO_ICLR_LEN           4
#define DWT_GPIO_ICLR_MASK          DWT_GPIO_IRQE_MASK
/* GPIO IRQ latch clear for GPIO0 input.
 * Write 1 to clear the GPIO0 interrupt latch.
 * Writing 0 has no effect. Reading returns zero
 */
#define DWT_GICLR0  GIRQx0
#define DWT_GICLR1  GIRQx1
#define DWT_GICLR2  GIRQx2
#define DWT_GICLR3  GIRQx3
#define DWT_GICLR4  GIRQx4
#define DWT_GICLR5  GIRQx5
#define DWT_GICLR6  GIRQx6
#define DWT_GICLR7  GIRQx7
#define DWT_GICLR8  GIRQx8

/* Sub-register 0x24 is the GPIO interrupt de-bounce enable register */
#define DWT_GPIO_IDBE_OFFSET        0x24
#define DWT_GPIO_IDBE_LEN           4
#define DWT_GPIO_IDBE_MASK          DWT_GPIO_IRQE_MASK
/* GPIO IRQ de-bounce enable for GPIO0.
 * Value 1 = de-bounce enabled.
 * Value 0 = de-bounce disabled
 */
#define DWT_GIDBE0  GIRQx0
#define DWT_GIDBE1  GIRQx1
#define DWT_GIDBE2  GIRQx2
#define DWT_GIDBE3  GIRQx3
#define DWT_GIDBE4  GIRQx4
#define DWT_GIDBE5  GIRQx5
#define DWT_GIDBE6  GIRQx6
#define DWT_GIDBE7  GIRQx7
/* Value 1 = de-bounce enabled, 0 = de-bounce disabled */
#define DWT_GIDBE8  GIRQx8

/* Sub-register 0x28 allows the raw state of the GPIO pin to be read. */
#define DWT_GPIO_RAW_OFFSET         0x28
#define DWT_GPIO_RAW_LEN            4
#define DWT_GPIO_RAW_MASK           DWT_GPIO_IRQE_MASK
/* This bit reflects the raw state of GPIO0 .. GPIO8 */
#define DWT_GRAWP0  GIRQx0
#define DWT_GRAWP1  GIRQx1
#define DWT_GRAWP2  GIRQx2
#define DWT_GRAWP3  GIRQx3
#define DWT_GRAWP4  GIRQx4
#define DWT_GRAWP5  GIRQx5
#define DWT_GRAWP6  GIRQx6
#define DWT_GRAWP7  GIRQx7
#define DWT_GRAWP8  GIRQx8

/* Digital Receiver configuration */
#define DWT_DRX_CONF_ID             0x27
#define DWT_DRX_CONF_LEN            44
/* Sub-register 0x02 is a 16-bit tuning register. */
#define DWT_DRX_TUNE0b_OFFSET       (0x02)
#define DWT_DRX_TUNE0b_LEN          2
/* 7.2.40.2 Sub-Register 0x27:02 DRX_TUNE0b */
#define DWT_DRX_TUNE0b_MASK         0xFFFF
#define DWT_DRX_TUNE0b_110K_STD     0x000A
#define DWT_DRX_TUNE0b_110K_NSTD    0x0016
#define DWT_DRX_TUNE0b_850K_STD     0x0001
#define DWT_DRX_TUNE0b_850K_NSTD    0x0006
#define DWT_DRX_TUNE0b_6M8_STD      0x0001
#define DWT_DRX_TUNE0b_6M8_NSTD     0x0002

/* 7.2.40.3 Sub-Register 0x27:04 DRX_TUNE1a */
#define DWT_DRX_TUNE1a_OFFSET       0x04
#define DWT_DRX_TUNE1a_LEN          2
#define DWT_DRX_TUNE1a_MASK         0xFFFF
#define DWT_DRX_TUNE1a_PRF16        0x0087
#define DWT_DRX_TUNE1a_PRF64        0x008D

/* 7.2.40.4 Sub-Register 0x27:06 DRX_TUNE1b */
#define DWT_DRX_TUNE1b_OFFSET       0x06
#define DWT_DRX_TUNE1b_LEN          2
#define DWT_DRX_TUNE1b_MASK         0xFFFF
#define DWT_DRX_TUNE1b_110K         0x0064
#define DWT_DRX_TUNE1b_850K_6M8     0x0020
#define DWT_DRX_TUNE1b_6M8_PRE64    0x0010

/* 7.2.40.5 Sub-Register 0x27:08 DRX_TUNE2 */
#define DWT_DRX_TUNE2_OFFSET        0x08
#define DWT_DRX_TUNE2_LEN           4
#define DWT_DRX_TUNE2_MASK          0xFFFFFFFFUL
#define DWT_DRX_TUNE2_PRF16_PAC8    0x311A002DUL
#define DWT_DRX_TUNE2_PRF16_PAC16   0x331A0052UL
#define DWT_DRX_TUNE2_PRF16_PAC32   0x351A009AUL
#define DWT_DRX_TUNE2_PRF16_PAC64   0x371A011DUL
#define DWT_DRX_TUNE2_PRF64_PAC8    0x313B006BUL
#define DWT_DRX_TUNE2_PRF64_PAC16   0x333B00BEUL
#define DWT_DRX_TUNE2_PRF64_PAC32   0x353B015EUL
#define DWT_DRX_TUNE2_PRF64_PAC64   0x373B0296UL

/* WARNING: Please do NOT set DRX_SFDTOC to zero
 * (disabling SFD detection timeout) since this risks IC malfunction
 * due to prolonged receiver activity in the event of false preamble detection.
 */
/* 7.2.40.7 Sub-Register 0x27:20 DRX_SFDTOC */
#define DWT_DRX_SFDTOC_OFFSET       0x20
#define DWT_DRX_SFDTOC_LEN          2
#define DWT_DRX_SFDTOC_MASK         0xFFFF

/* 7.2.40.9 Sub-Register 0x27:24 DRX_PRETOC */
#define DWT_DRX_PRETOC_OFFSET       0x24
#define DWT_DRX_PRETOC_LEN          2
#define DWT_DRX_PRETOC_MASK         0xFFFF

/* 7.2.40.10 Sub-Register 0x27:26 DRX_TUNE4H */
#define DWT_DRX_TUNE4H_OFFSET       0x26
#define DWT_DRX_TUNE4H_LEN          2
#define DWT_DRX_TUNE4H_MASK         0xFFFF
#define DWT_DRX_TUNE4H_PRE64        0x0010
#define DWT_DRX_TUNE4H_PRE128PLUS   0x0028

/*
 * Offset from DRX_CONF_ID in bytes to 21-bit signed
 * RX carrier integrator value
 */
#define DWT_DRX_CARRIER_INT_OFFSET  0x28
#define DWT_DRX_CARRIER_INT_LEN     3
#define DWT_DRX_CARRIER_INT_MASK    0x001FFFFF

/* 7.2.40.11 Sub-Register 0x27:2C - RXPACC_NOSAT */
#define DWT_RPACC_NOSAT_OFFSET      0x2C
#define DWT_RPACC_NOSAT_LEN         2
#define DWT_RPACC_NOSAT_MASK        0xFFFF

/* Analog RF Configuration */
#define DWT_RF_CONF_ID              0x28
#define DWT_RF_CONF_LEN             58
/* TX enable */
#define DWT_RF_CONF_TXEN_MASK       0x00400000UL
/* RX enable */
#define DWT_RF_CONF_RXEN_MASK       0x00200000UL
/* Turn on power all LDOs */
#define DWT_RF_CONF_TXPOW_MASK      0x001F0000UL
/* Enable PLLs */
#define DWT_RF_CONF_PLLEN_MASK      0x0000E000UL
/* Enable TX blocks */
#define DWT_RF_CONF_TXBLOCKSEN_MASK 0x00001F00UL
#define DWT_RF_CONF_TXPLLPOWEN_MASK (DWT_RF_CONF_PLLEN_MASK | \
				     DWT_RF_CONF_TXPOW_MASK)
#define DWT_RF_CONF_TXALLEN_MASK    (DWT_RF_CONF_TXEN_MASK |  \
				     DWT_RF_CONF_TXPOW_MASK | \
				     DWT_RF_CONF_PLLEN_MASK | \
				     DWT_RF_CONF_TXBLOCKSEN_MASK)
/* Analog RX Control Register */
#define DWT_RF_RXCTRLH_OFFSET       0x0B
#define DWT_RF_RXCTRLH_LEN          1
/* RXCTRLH value for narrow bandwidth channels */
#define DWT_RF_RXCTRLH_NBW          0xD8
/* RXCTRLH value for wide bandwidth channels */
#define DWT_RF_RXCTRLH_WBW          0xBC
/* Analog TX Control Register */
#define DWT_RF_TXCTRL_OFFSET        0x0C
#define DWT_RF_TXCTRL_LEN           4
/* Transmit mixer tuning register */
#define DWT_RF_TXCTRL_TXMTUNE_MASK  0x000001E0UL
/* Transmit mixer Q-factor tuning register */
#define DWT_RF_TXCTRL_TXTXMQ_MASK   0x00000E00UL
/* 32-bit value to program to Sub-Register 0x28:0C RF_TXCTRL */
#define DWT_RF_TXCTRL_CH1           0x00005C40UL
/* 32-bit value to program to Sub-Register 0x28:0C RF_TXCTRL */
#define DWT_RF_TXCTRL_CH2           0x00045CA0UL
/* 32-bit value to program to Sub-Register 0x28:0C RF_TXCTRL */
#define DWT_RF_TXCTRL_CH3           0x00086CC0UL
/* 32-bit value to program to Sub-Register 0x28:0C RF_TXCTRL */
#define DWT_RF_TXCTRL_CH4           0x00045C80UL
/* 32-bit value to program to Sub-Register 0x28:0C RF_TXCTRL */
#define DWT_RF_TXCTRL_CH5           0x001E3FE0UL
/* 32-bit value to program to Sub-Register 0x28:0C RF_TXCTRL */
#define DWT_RF_TXCTRL_CH7           0x001E7DE0UL

#define DWT_RF_STATUS_OFFSET        0x2C

#define DWT_REG_29_ID_RESERVED      0x29

/* Transmitter calibration block */
#define DWT_TX_CAL_ID               0x2A
#define DWT_TX_CAL_LEN              52
/* SAR control */
#define DWT_TC_SARL_SAR_C                   0
/* Cause bug in register block TX_CAL, we need to read 1 byte in a time */
/* Latest SAR reading for Voltage level */
#define DWT_TC_SARL_SAR_LVBAT_OFFSET    3
/* Latest SAR reading for Temperature level */
#define DWT_TC_SARL_SAR_LTEMP_OFFSET    4
/* SAR reading of Temperature level taken at last wakeup event */
#define DWT_TC_SARW_SAR_WTEMP_OFFSET    0x06
/* SAR reading of Voltage level taken at last wakeup event */
#define DWT_TC_SARW_SAR_WVBAT_OFFSET    0x07
/* Transmitter Calibration Pulse Generator Delay */
#define DWT_TC_PGDELAY_OFFSET       0x0B
#define DWT_TC_PGDELAY_LEN          1
/* Recommended value for channel 1 */
#define DWT_TC_PGDELAY_CH1          0xC9
/* Recommended value for channel 2 */
#define DWT_TC_PGDELAY_CH2          0xC2
/* Recommended value for channel 3 */
#define DWT_TC_PGDELAY_CH3          0xC5
/* Recommended value for channel 4 */
#define DWT_TC_PGDELAY_CH4          0x95
/* Recommended value for channel 5 */
#define DWT_TC_PGDELAY_CH5          0xC0
/* Recommended value for channel 7 */
#define DWT_TC_PGDELAY_CH7          0x93
/* Transmitter Calibration Pulse Generator Test */
#define DWT_TC_PGTEST_OFFSET        0x0C
#define DWT_TC_PGTEST_LEN           1
/* Normal operation */
#define DWT_TC_PGTEST_NORMAL        0x00
/* Continuous Wave (CW) Test Mode */
#define DWT_TC_PGTEST_CW            0x13

/* Frequency synthesiser control block */
#define DWT_FS_CTRL_ID              0x2B
#define DWT_FS_CTRL_LEN             21
/*
 * Offset from FS_CTRL_ID in bytes, reserved area.
 * Please take care not to write to this area as doing so
 * may cause the DW1000 to malfunction.
 */
#define DWT_FS_RES1_OFFSET          0x00
#define DWT_FS_RES1_LEN             7
/* Frequency synthesiser PLL configuration */
#define DWT_FS_PLLCFG_OFFSET        0x07
#define DWT_FS_PLLCFG_LEN           5
/* Operating Channel 1 */
#define DWT_FS_PLLCFG_CH1           0x09000407UL
/* Operating Channel 2 */
#define DWT_FS_PLLCFG_CH2           0x08400508UL
/* Operating Channel 3 */
#define DWT_FS_PLLCFG_CH3           0x08401009UL
/* Operating Channel 4 (same as 2) */
#define DWT_FS_PLLCFG_CH4           DWT_FS_PLLCFG_CH2
/* Operating Channel 5 */
#define DWT_FS_PLLCFG_CH5           0x0800041DUL
/* Operating Channel 7 (same as 5) */
#define DWT_FS_PLLCFG_CH7           DWT_FS_PLLCFG_CH5
/* Frequency synthesiser PLL Tuning */
#define DWT_FS_PLLTUNE_OFFSET       0x0B
#define DWT_FS_PLLTUNE_LEN          1
/* Operating Channel 1 */
#define DWT_FS_PLLTUNE_CH1          0x1E
/* Operating Channel 2 */
#define DWT_FS_PLLTUNE_CH2          0x26
/* Operating Channel 3 */
#define DWT_FS_PLLTUNE_CH3          0x56
/* Operating Channel 4 (same as 2) */
#define DWT_FS_PLLTUNE_CH4          DWT_FS_PLLTUNE_CH2
/* Operating Channel 5 */
#define DWT_FS_PLLTUNE_CH5          0xBE
/* Operating Channel 7 (same as 5) */
#define DWT_FS_PLLTUNE_CH7          DWT_FS_PLLTUNE_CH5
/*
 * Offset from FS_CTRL_ID in bytes.
 * Please take care not to write to this area as doing so
 * may cause the DW1000 to malfunction.
 */
#define DWT_FS_RES2_OFFSET          0x0C
#define DWT_FS_RES2_LEN             2
/* Frequency synthesiser Crystal trim */
#define DWT_FS_XTALT_OFFSET         0x0E
#define DWT_FS_XTALT_LEN            1
/*
 * Crystal Trim.
 * Crystals may be trimmed using this register setting to tune out errors,
 * see 8.1 IC Calibration Crystal Oscillator Trim.
 */
#define DWT_FS_XTALT_MASK           0x1F
#define DWT_FS_XTALT_MIDRANGE       0x10
/*
 * Offset from FS_CTRL_ID in bytes.
 * Please take care not to write to this area as doing so
 * may cause the DW1000 to malfunction.
 */
#define DWT_FS_RES3_OFFSET          0x0F
#define DWT_FS_RES3_LEN             6

/* Always-On register set */
#define DWT_AON_ID                  0x2C
#define DWT_AON_LEN                 12
/*
 * Offset from AON_ID in bytes
 * Used to control what the DW1000 IC does as it wakes up from
 * low-power SLEEP or DEEPSLEEPstates.
 */
#define DWT_AON_WCFG_OFFSET         0x00
#define DWT_AON_WCFG_LEN            2
/* Access mask to AON_WCFG register */
#define DWT_AON_WCFG_MASK           0x09CB
/* On Wake-up Run the (temperature and voltage) Analog-to-Digital Converters */
#define DWT_AON_WCFG_ONW_RADC       0x0001
/* On Wake-up turn on the Receiver */
#define DWT_AON_WCFG_ONW_RX         0x0002
/*
 * On Wake-up load the EUI from OTP memory into Register file:
 * 0x01 Extended Unique Identifier.
 */
#define DWT_AON_WCFG_ONW_LEUI       0x0008
/*
 * On Wake-up load configurations from the AON memory
 * into the host interface register set
 */
#define DWT_AON_WCFG_ONW_LDC        0x0040
/* On Wake-up load the Length64 receiver operating parameter set */
#define DWT_AON_WCFG_ONW_L64P       0x0080
/*
 * Preserve Sleep. This bit determines what the DW1000 does
 * with respect to the ARXSLP and ATXSLP sleep controls
 */
#define DWT_AON_WCFG_PRES_SLEEP     0x0100
/* On Wake-up load the LDE microcode. */
#define DWT_AON_WCFG_ONW_LLDE       0x0800
/* On Wake-up load the LDO tune value. */
#define DWT_AON_WCFG_ONW_LLDO       0x1000
/*
 * The bits in this register in general cause direct activity
 * within the AON block with respect to the stored AON memory
 */
#define DWT_AON_CTRL_OFFSET         0x02
#define DWT_AON_CTRL_LEN            1
/* Access mask to AON_CTRL register */
#define DWT_AON_CTRL_MASK           0x8F
/*
 * When this bit is set the DW1000 will copy the user configurations
 * from the AON memory to the host interface register set.
 */
#define DWT_AON_CTRL_RESTORE        0x01
/*
 * When this bit is set the DW1000 will copy the user configurations
 * from the host interface register set into the AON memory
 */
#define DWT_AON_CTRL_SAVE           0x02
/* Upload the AON block configurations to the AON */
#define DWT_AON_CTRL_UPL_CFG        0x04
/* Direct AON memory access read */
#define DWT_AON_CTRL_DCA_READ       0x08
/* Direct AON memory access enable bit */
#define DWT_AON_CTRL_DCA_ENAB       0x80
/* AON Direct Access Read Data Result */
#define DWT_AON_RDAT_OFFSET         0x03
#define DWT_AON_RDAT_LEN            1
/* AON Direct Access Address */
#define DWT_AON_ADDR_OFFSET         0x04
#define DWT_AON_ADDR_LEN            1
/* Address of low-power oscillator calibration value (lower byte) */
#define DWT_AON_ADDR_LPOSC_CAL_0    117
/* Address of low-power oscillator calibration value (lower byte) */
#define DWT_AON_ADDR_LPOSC_CAL_1    118

/* 32-bit configuration register for the always on block. */
#define DWT_AON_CFG0_OFFSET         0x06
#define DWT_AON_CFG0_LEN            4
/* This is the sleep enable configuration bit */
#define DWT_AON_CFG0_SLEEP_EN           0x00000001UL
/* Wake using WAKEUP pin */
#define DWT_AON_CFG0_WAKE_PIN           0x00000002UL
/* Wake using SPI access SPICSn */
#define DWT_AON_CFG0_WAKE_SPI           0x00000004UL
/* Wake when sleep counter elapses */
#define DWT_AON_CFG0_WAKE_CNT           0x00000008UL
/* Low power divider enable configuration */
#define DWT_AON_CFG0_LPDIV_EN           0x00000010UL
/*
 * Divider count for dividing the raw DW1000 XTAL oscillator frequency
 * to set an LP clock frequency
 */
#define DWT_AON_CFG0_LPCLKDIVA_MASK     0x0000FFE0UL
#define DWT_AON_CFG0_LPCLKDIVA_SHIFT    5
/* Sleep time. This field configures the sleep time count elapse value */
#define DWT_AON_CFG0_SLEEP_TIM          0xFFFF0000UL
#define DWT_AON_CFG0_SLEEP_SHIFT        16
#define DWT_AON_CFG0_SLEEP_TIM_OFFSET   2
#define DWT_AON_CFG1_OFFSET         0x0A
#define DWT_AON_CFG1_LEN            2
/* access mask to AON_CFG1 */
#define DWT_AON_CFG1_MASK           0x0007
/* This bit enables the sleep counter */
#define DWT_AON_CFG1_SLEEP_CEN      0x0001
/*
 * This bit needs to be set to 0 for correct operation
 * in the SLEEP state within the DW1000
 */
#define DWT_AON_CFG1_SMXX           0x0002
/*
 * This bit enables the calibration function that measures
 * the period of the ICs internal low powered oscillator.
 */
#define DWT_AON_CFG1_LPOSC_CAL      0x0004

/* One Time Programmable Memory Interface */
#define DWT_OTP_IF_ID               0x2D
#define DWT_OTP_IF_LEN              18
/* 32-bit register. The data value to be programmed into an OTP location */
#define DWT_OTP_WDAT                0x00
#define DWT_OTP_WDAT_LEN            4
/* 16-bit register used to select the address within the OTP memory block */
#define DWT_OTP_ADDR                0x04
#define DWT_OTP_ADDR_LEN            2
/*
 * This 11-bit field specifies the address within OTP memory
 * that will be accessed read or written.
 */
#define DWT_OTP_ADDR_MASK           0x07FF
/* used to control the operation of the OTP memory */
#define DWT_OTP_CTRL                0x06
#define DWT_OTP_CTRL_LEN            2
#define DWT_OTP_CTRL_MASK           0x8002
/* This bit forces the OTP into manual read mode */
#define DWT_OTP_CTRL_OTPRDEN        0x0001
/*
 * This bit commands a read operation from the address specified
 * in the OTP_ADDR register
 */
#define DWT_OTP_CTRL_OTPREAD        0x0002
/* This bit forces a load of LDE microcode */
#define DWT_OTP_CTRL_LDELOAD        0x8000
/*
 * Setting this bit will cause the contents of OTP_WDAT to be written
 * to OTP_ADDR.
 */
#define DWT_OTP_CTRL_OTPPROG        0x0040
#define DWT_OTP_STAT                0x08
#define DWT_OTP_STAT_LEN            2
#define DWT_OTP_STAT_MASK           0x0003
/* OTP Programming Done */
#define DWT_OTP_STAT_OTPPRGD        0x0001
/* OTP Programming Voltage OK */
#define DWT_OTP_STAT_OTPVPOK        0x0002
/* 32-bit register. The data value read from an OTP location will appear here */
#define DWT_OTP_RDAT                0x0A
#define DWT_OTP_RDAT_LEN            4
/*
 * 32-bit register. The data value stored in the OTP SR (0x400) location
 * will appear here after power up
 */
#define DWT_OTP_SRDAT               0x0E
#define DWT_OTP_SRDAT_LEN           4
/*
 * 8-bit special function register used to select and
 * load special receiver operational parameter
 */
#define DWT_OTP_SF                  0x12
#define DWT_OTP_SF_LEN              1
#define DWT_OTP_SF_MASK             0x63
/*
 * This bit when set initiates a load of the operating parameter set
 * selected by the OPS_SEL
 */
#define DWT_OTP_SF_OPS_KICK         0x01
/* This bit when set initiates a load of the LDO tune code */
#define DWT_OTP_SF_LDO_KICK         0x02
#define DWT_OTP_SF_OPS_SEL_SHFT     5
#define DWT_OTP_SF_OPS_SEL_MASK     0x60
/* Operating parameter set selection: Length64 */
#define DWT_OTP_SF_OPS_SEL_L64      0x00
/* Operating parameter set selection: Tight */
#define DWT_OTP_SF_OPS_SEL_TIGHT    0x40

/* Leading edge detection control block */
#define DWT_LDE_IF_ID               0x2E
#define DWT_LDE_IF_LEN              0
/*
 * 16-bit status register reporting the threshold that was used
 * to find the first path
 */
#define DWT_LDE_THRESH_OFFSET       0x0000
#define DWT_LDE_THRESH_LEN          2
/*8-bit configuration register */
#define DWT_LDE_CFG1_OFFSET         0x0806
#define DWT_LDE_CFG1_LEN            1
/* Number of Standard Deviations mask. */
#define DWT_LDE_CFG1_NSTDEV_MASK    0x1F
/* Peak Multiplier mask. */
#define DWT_LDE_CFG1_PMULT_MASK     0xE0
/*
 * Reporting the position within the accumulator that the LDE algorithm
 * has determined to contain the maximum
 */
#define DWT_LDE_PPINDX_OFFSET       0x1000
#define DWT_LDE_PPINDX_LEN          2
/*
 * Reporting the magnitude of the peak signal seen
 * in the accumulator data memory
 */
#define DWT_LDE_PPAMPL_OFFSET       0x1002
#define DWT_LDE_PPAMPL_LEN          2
/* 16-bit configuration register for setting the receive antenna delay */
#define DWT_LDE_RXANTD_OFFSET       0x1804
#define DWT_LDE_RXANTD_LEN          2
/* 16-bit LDE configuration tuning register */
#define DWT_LDE_CFG2_OFFSET         0x1806
#define DWT_LDE_CFG2_LEN            2
/*
 * 16-bit configuration register for setting
 * the replica avoidance coefficient
 */
#define DWT_LDE_REPC_OFFSET         0x2804
#define DWT_LDE_REPC_LEN            2
#define DWT_LDE_REPC_PCODE_1        0x5998
#define DWT_LDE_REPC_PCODE_2        0x5998
#define DWT_LDE_REPC_PCODE_3        0x51EA
#define DWT_LDE_REPC_PCODE_4        0x428E
#define DWT_LDE_REPC_PCODE_5        0x451E
#define DWT_LDE_REPC_PCODE_6        0x2E14
#define DWT_LDE_REPC_PCODE_7        0x8000
#define DWT_LDE_REPC_PCODE_8        0x51EA
#define DWT_LDE_REPC_PCODE_9        0x28F4
#define DWT_LDE_REPC_PCODE_10       0x3332
#define DWT_LDE_REPC_PCODE_11       0x3AE0
#define DWT_LDE_REPC_PCODE_12       0x3D70
#define DWT_LDE_REPC_PCODE_13       0x3AE0
#define DWT_LDE_REPC_PCODE_14       0x35C2
#define DWT_LDE_REPC_PCODE_15       0x2B84
#define DWT_LDE_REPC_PCODE_16       0x35C2
#define DWT_LDE_REPC_PCODE_17       0x3332
#define DWT_LDE_REPC_PCODE_18       0x35C2
#define DWT_LDE_REPC_PCODE_19       0x35C2
#define DWT_LDE_REPC_PCODE_20       0x47AE
#define DWT_LDE_REPC_PCODE_21       0x3AE0
#define DWT_LDE_REPC_PCODE_22       0x3850
#define DWT_LDE_REPC_PCODE_23       0x30A2
#define DWT_LDE_REPC_PCODE_24       0x3850

/* Digital Diagnostics Interface */
#define DWT_DIG_DIAG_ID             0x2F
#define DWT_DIG_DIAG_LEN            41

/* Event Counter Control */
#define DWT_EVC_CTRL_OFFSET         0x00
#define DWT_EVC_CTRL_LEN            4
/*
 * Access mask to Register for bits should always be set to zero
 * to avoid any malfunction of the device.
 */
#define DWT_EVC_CTRL_MASK           0x00000003UL
/* Event Counters Enable bit */
#define DWT_EVC_EN                  0x00000001UL
#define DWT_EVC_CLR                 0x00000002UL

/* PHR Error Event Counter */
#define DWT_EVC_PHE_OFFSET          0x04
#define DWT_EVC_PHE_LEN             2
#define DWT_EVC_PHE_MASK            0x0FFF
/* Reed Solomon decoder (Frame Sync Loss) Error Event Counter */
#define DWT_EVC_RSE_OFFSET          0x06
#define DWT_EVC_RSE_LEN             2
#define DWT_EVC_RSE_MASK            0x0FFF

/*
 * The EVC_FCG field is a 12-bit counter of the frames received with
 * good CRC/FCS sequence.
 */
#define DWT_EVC_FCG_OFFSET          0x08
#define DWT_EVC_FCG_LEN             2
#define DWT_EVC_FCG_MASK            0x0FFF
/*
 * The EVC_FCE field is a 12-bit counter of the frames received with
 * bad CRC/FCS sequence.
 */
#define DWT_EVC_FCE_OFFSET          0x0A
#define DWT_EVC_FCE_LEN             2
#define DWT_EVC_FCE_MASK            0x0FFF

/*
 * The EVC_FFR field is a 12-bit counter of the frames rejected
 * by the receive frame filtering function.
 */
#define DWT_EVC_FFR_OFFSET          0x0C
#define DWT_EVC_FFR_LEN             2
#define DWT_EVC_FFR_MASK            0x0FFF
/* The EVC_OVR field is a 12-bit counter of receive overrun events */
#define DWT_EVC_OVR_OFFSET          0x0E
#define DWT_EVC_OVR_LEN             2
#define DWT_EVC_OVR_MASK            0x0FFF

/* The EVC_STO field is a 12-bit counter of SFD Timeout Error events */
#define DWT_EVC_STO_OFFSET          0x10
#define DWT_EVC_OVR_LEN             2
#define DWT_EVC_OVR_MASK            0x0FFF
/* The EVC_PTO field is a 12-bit counter of Preamble detection Timeout events */
#define DWT_EVC_PTO_OFFSET          0x12
#define DWT_EVC_PTO_LEN             2
#define DWT_EVC_PTO_MASK            0x0FFF

/*
 * The EVC_FWTO field is a 12-bit counter of receive
 * frame wait timeout events
 */
#define DWT_EVC_FWTO_OFFSET         0x14
#define DWT_EVC_FWTO_LEN            2
#define DWT_EVC_FWTO_MASK           0x0FFF
/*
 * The EVC_TXFS field is a 12-bit counter of transmit frames sent.
 * This is incremented every time a frame is sent
 */
#define DWT_EVC_TXFS_OFFSET         0x16
#define DWT_EVC_TXFS_LEN            2
#define DWT_EVC_TXFS_MASK           0x0FFF

/* The EVC_HPW field is a 12-bit counter of Half Period Warnings. */
#define DWT_EVC_HPW_OFFSET          0x18
#define DWT_EVC_HPW_LEN             2
#define DWT_EVC_HPW_MASK            0x0FFF
/* The EVC_TPW field is a 12-bit counter of Transmitter Power-Up Warnings. */
#define DWT_EVC_TPW_OFFSET          0x1A
#define DWT_EVC_TPW_LEN             2
#define DWT_EVC_TPW_MASK            0x0FFF

/*
 * Offset from DIG_DIAG_ID in bytes,
 * Please take care not to write to this area as doing so
 * may cause the DW1000 to malfunction.
 */
#define DWT_EVC_RES1_OFFSET         0x1C

#define DWT_DIAG_TMC_OFFSET         0x24
#define DWT_DIAG_TMC_LEN            2
#define DWT_DIAG_TMC_MASK           0x0010
/*
 * This test mode is provided to help support regulatory approvals
 * spectral testing. When the TX_PSTM bit is set it enables a
 * repeating transmission of the data from the TX_BUFFER
 */
#define DWT_DIAG_TMC_TX_PSTM        0x0010

#define DWT_REG_30_ID_RESERVED      0x30
#define DWT_REG_31_ID_RESERVED      0x31
#define DWT_REG_32_ID_RESERVED      0x32
#define DWT_REG_33_ID_RESERVED      0x33
#define DWT_REG_34_ID_RESERVED      0x34
#define DWT_REG_35_ID_RESERVED      0x35

/* Power Management System Control Block */
#define DWT_PMSC_ID                 0x36
#define DWT_PMSC_LEN                48
#define DWT_PMSC_CTRL0_OFFSET       0x00
#define DWT_PMSC_CTRL0_LEN          4
/* Access mask to register PMSC_CTRL0 */
#define DWT_PMSC_CTRL0_MASK         0xF18F847FUL
/*
 * The system clock will run off the 19.2 MHz XTI clock until the PLL is
 * calibrated and locked, then it will switch over the 125 MHz PLL clock
 */
#define DWT_PMSC_CTRL0_SYSCLKS_AUTO 0x00000000UL
/* Force system clock to be the 19.2 MHz XTI clock. */
#define DWT_PMSC_CTRL0_SYSCLKS_19M  0x00000001UL
/* Force system clock to the 125 MHz PLL clock. */
#define DWT_PMSC_CTRL0_SYSCLKS_125M 0x00000002UL
/* The RX clock will be disabled until it is required for an RX operation */
#define DWT_PMSC_CTRL0_RXCLKS_AUTO  0x00000000UL
/* Force RX clock enable and sourced clock from the 19.2 MHz XTI clock */
#define DWT_PMSC_CTRL0_RXCLKS_19M   0x00000004UL
/* Force RX clock enable and sourced from the 125 MHz PLL clock */
#define DWT_PMSC_CTRL0_RXCLKS_125M  0x00000008UL
/* Force RX clock off. */
#define DWT_PMSC_CTRL0_RXCLKS_OFF   0x0000000CUL
/* The TX clock will be disabled until it is required for a TX operation */
#define DWT_PMSC_CTRL0_TXCLKS_AUTO  0x00000000UL
/* Force TX clock enable and sourced clock from the 19.2 MHz XTI clock */
#define DWT_PMSC_CTRL0_TXCLKS_19M   0x00000010UL
/* Force TX clock enable and sourced from the 125 MHz PLL clock */
#define DWT_PMSC_CTRL0_TXCLKS_125M  0x00000020UL
/* Force TX clock off */
#define DWT_PMSC_CTRL0_TXCLKS_OFF   0x00000030UL
/* Force Accumulator Clock Enable */
#define DWT_PMSC_CTRL0_FACE         0x00000040UL
/* GPIO clock enable */
#define DWT_PMSC_CTRL0_GPCE         0x00010000UL
/* GPIO reset (NOT), active low */
#define DWT_PMSC_CTRL0_GPRN         0x00020000UL
/* GPIO De-bounce Clock Enable */
#define DWT_PMSC_CTRL0_GPDCE        0x00040000UL
/* Kilohertz Clock Enable */
#define DWT_PMSC_CTRL0_KHZCLEN      0x00800000UL
/* Enable PLL2 on/off sequencing by SNIFF mode */
#define DWT_PMSC_CTRL0_PLL2_SEQ_EN  0x01000000UL
#define DWT_PMSC_CTRL0_SOFTRESET_OFFSET 3
/* Assuming only 4th byte of the register is read */
#define DWT_PMSC_CTRL0_RESET_ALL    0x00
/* Assuming only 4th byte of the register is read */
#define DWT_PMSC_CTRL0_RESET_RX     0xE0
/* Assuming only 4th byte of the register is read */
#define DWT_PMSC_CTRL0_RESET_CLEAR  0xF0
#define DWT_PMSC_CTRL1_OFFSET       0x04
#define DWT_PMSC_CTRL1_LEN          4
/* Access mask to register PMSC_CTRL1 */
#define DWT_PMSC_CTRL1_MASK         0xFC02F802UL
/* Automatic transition from receive mode into the INIT state */
#define DWT_PMSC_CTRL1_ARX2INIT     0x00000002UL
/*
 * If this bit is set then the DW1000 will automatically transition
 * into SLEEP or DEEPSLEEP mode after transmission of a frame
 */
#define DWT_PMSC_CTRL1_ATXSLP       0x00000800UL
/*
 * This bit is set then the DW1000 will automatically transition
 * into SLEEP mode after a receive attempt
 */
#define DWT_PMSC_CTRL1_ARXSLP       0x00001000UL
/* Snooze Enable */
#define DWT_PMSC_CTRL1_SNOZE        0x00002000UL
/* The SNOZR bit is set to allow the snooze timer to repeat twice */
#define DWT_PMSC_CTRL1_SNOZR        0x00004000UL
/* This enables a special 1 GHz clock used for some external SYNC modes */
#define DWT_PMSC_CTRL1_PLLSYN       0x00008000UL
/* This bit enables the running of the LDE algorithm */
#define DWT_PMSC_CTRL1_LDERUNE      0x00020000UL
/* Kilohertz clock divisor */
#define DWT_PMSC_CTRL1_KHZCLKDIV_MASK   0xFC000000UL
/*
 * Writing this to PMSC CONTROL 1 register (bits 10-3) disables
 * PMSC control of analog RF subsystems
 */
#define DWT_PMSC_CTRL1_PKTSEQ_DISABLE   0x00
/*
 * Writing this to PMSC CONTROL 1 register (bits 10-3) enables
 * PMSC control of analog RF subsystems
 */
#define DWT_PMSC_CTRL1_PKTSEQ_ENABLE    0xE7
#define DWT_PMSC_RES1_OFFSET        0x08
/* PMSC Snooze Time Register */
#define DWT_PMSC_SNOZT_OFFSET       0x0C
#define DWT_PMSC_SNOZT_LEN          1
#define DWT_PMSC_RES2_OFFSET        0x10
#define DWT_PMSC_RES3_OFFSET        0x24
#define DWT_PMSC_TXFINESEQ_OFFSET   0x26
/* Writing this disables fine grain sequencing in the transmitter */
#define DWT_PMSC_TXFINESEQ_DISABLE  0x0
/* Writing this enables fine grain sequencing in the transmitter */
#define DWT_PMSC_TXFINESEQ_ENABLE   0x0B74
#define DWT_PMSC_LEDC_OFFSET        0x28
#define DWT_PMSC_LEDC_LEN           4
/* 32-bit LED control register. */
#define DWT_PMSC_LEDC_MASK          0x000001FFUL
/*
 * This field determines how long the LEDs remain lit after an event
 * that causes them to be set on.
 */
#define DWT_PMSC_LEDC_BLINK_TIM_MASK 0x000000FFUL
/* Blink Enable. When this bit is set to 1 the LED blink feature is enabled. */
#define DWT_PMSC_LEDC_BLNKEN        0x00000100UL
/*
 * Default blink time. Blink time is expressed in multiples of 14 ms.
 * The value defined here is ~225 ms.
 */
#define DWT_PMSC_LEDC_BLINK_TIME_DEF 0x10
/* Command a blink of all LEDs */
#define DWT_PMSC_LEDC_BLINK_NOW_ALL 0x000F0000UL

#define DWT_REG_37_ID_RESERVED      0x37
#define DWT_REG_38_ID_RESERVED      0x38
#define DWT_REG_39_ID_RESERVED      0x39
#define DWT_REG_3A_ID_RESERVED      0x3A
#define DWT_REG_3B_ID_RESERVED      0x3B
#define DWT_REG_3C_ID_RESERVED      0x3C
#define DWT_REG_3D_ID_RESERVED      0x3D
#define DWT_REG_3E_ID_RESERVED      0x3E
#define DWT_REG_3F_ID_RESERVED      0x3F


/*
 * Map the channel number to the index in the configuration arrays below.
 *                  Channel: na  1  2  3  4  5 na  7
 */
const uint8_t dwt_ch_to_cfg[] = {0, 0, 1, 2, 3, 4, 0, 5};

/* Defaults from Table 38: Sub-Register 0x28:0C– RF_TXCTRL values */
const uint32_t dwt_txctrl_defs[] = {
	DWT_RF_TXCTRL_CH1,
	DWT_RF_TXCTRL_CH2,
	DWT_RF_TXCTRL_CH3,
	DWT_RF_TXCTRL_CH4,
	DWT_RF_TXCTRL_CH5,
	DWT_RF_TXCTRL_CH7,
};

/* Defaults from Table 43: Sub-Register 0x2B:07 – FS_PLLCFG values */
const uint32_t dwt_pllcfg_defs[] = {
	DWT_FS_PLLCFG_CH1,
	DWT_FS_PLLCFG_CH2,
	DWT_FS_PLLCFG_CH3,
	DWT_FS_PLLCFG_CH4,
	DWT_FS_PLLCFG_CH5,
	DWT_FS_PLLCFG_CH7
};

/* Defaults from Table 44: Sub-Register 0x2B:0B – FS_PLLTUNE values */
const uint8_t dwt_plltune_defs[] = {
	DWT_FS_PLLTUNE_CH1,
	DWT_FS_PLLTUNE_CH2,
	DWT_FS_PLLTUNE_CH3,
	DWT_FS_PLLTUNE_CH4,
	DWT_FS_PLLTUNE_CH5,
	DWT_FS_PLLTUNE_CH7
};

/* Defaults from Table 37: Sub-Register 0x28:0B– RF_RXCTRLH values */
const uint8_t dwt_rxctrlh_defs[] = {
	DWT_RF_RXCTRLH_NBW,
	DWT_RF_RXCTRLH_NBW,
	DWT_RF_RXCTRLH_NBW,
	DWT_RF_RXCTRLH_WBW,
	DWT_RF_RXCTRLH_NBW,
	DWT_RF_RXCTRLH_WBW
};

/* Defaults from Table 40: Sub-Register 0x2A:0B – TC_PGDELAY */
const uint8_t dwt_pgdelay_defs[] = {
	DWT_TC_PGDELAY_CH1,
	DWT_TC_PGDELAY_CH2,
	DWT_TC_PGDELAY_CH3,
	DWT_TC_PGDELAY_CH4,
	DWT_TC_PGDELAY_CH5,
	DWT_TC_PGDELAY_CH7
};

/*
 * Defaults from Table 19: Reference values for Register file:
 *     0x1E – Transmit Power Control for Smart Transmit Power Control
 *     Transmit Power Control values for 16 MHz, with DIS_STXP = 0
 */
const uint32_t dwt_txpwr_stxp0_16[] = {
	0x15355575,
	0x15355575,
	0x0F2F4F6F,
	0x1F1F3F5F,
	0x0E082848,
	0x32527292
};

/*
 * Defaults from Table 19: Reference values for Register file:
 *     0x1E – Transmit Power Control for Smart Transmit Power Control
 *     Transmit Power Control values for 64 MHz, with DIS_STXP = 0
 */
const uint32_t dwt_txpwr_stxp0_64[] = {
	 0x07274767,
	 0x07274767,
	 0x2B4B6B8B,
	 0x3A5A7A9A,
	 0x25456585,
	 0x5171B1D1
};

/*
 * Default from Table 20: Reference values Register file:
 *     0x1E – Transmit Power Control for Manual Transmit Power Control
 *     Transmit Power Control values for 16 MHz, with DIS_STXP = 1
 */
const uint32_t dwt_txpwr_stxp1_16[] = {
	0x75757575,
	0x75757575,
	0x6F6F6F6F,
	0x5F5F5F5F,
	0x48484848,
	0x92929292
};

/*
 * Default from Table 20: Reference values Register file:
 *     0x1E – Transmit Power Control for Manual Transmit Power Control
 *     Transmit Power Control values for 64 MHz, with DIS_STXP = 1
 */
const uint32_t dwt_txpwr_stxp1_64[] = {
	0x67676767,
	0x67676767,
	0x8B8B8B8B,
	0x9A9A9A9A,
	0x85858585,
	0xD1D1D1D1
};

enum dwt_pulse_repetition_frequency {
	DWT_PRF_16M = 0,
	DWT_PRF_64M,
	DWT_NUMOF_PRFS,
};

/* Defaults from Table 24: Sub-Register 0x23:04 – AGC_TUNE1 values */
const uint16_t dwt_agc_tune1_defs[] = {
	DWT_AGC_TUNE1_16M,
	DWT_AGC_TUNE1_64M
};

enum dwt_baud_rate {
	DWT_BR_110K = 0,
	DWT_BR_850K,
	DWT_BR_6M8,
	DWT_NUMOF_BRS,
};

/* Decawave non-standard SFD lengths */
const uint8_t dwt_ns_sfdlen[] = {
	DWT_DW_NS_SFD_LEN_110K,
	DWT_DW_NS_SFD_LEN_850K,
	DWT_DW_NS_SFD_LEN_6M8
};

/* Defaults from Table 30: Sub-Register 0x27:02 – DRX_TUNE0b values */
const uint16_t dwt_tune0b_defs[DWT_NUMOF_BRS][2] = {
	{
		DWT_DRX_TUNE0b_110K_STD,
		DWT_DRX_TUNE0b_110K_NSTD
	},
	{
		DWT_DRX_TUNE0b_850K_STD,
		DWT_DRX_TUNE0b_850K_NSTD
	},
	{
		DWT_DRX_TUNE0b_6M8_STD,
		DWT_DRX_TUNE0b_6M8_NSTD
	}
};

/* Defaults from Table 31: Sub-Register 0x27:04 – DRX_TUNE1a values */
const uint16_t dwt_tune1a_defs[] = {
	DWT_DRX_TUNE1a_PRF16,
	DWT_DRX_TUNE1a_PRF64
};

enum dwt_acquisition_chunk_size {
	DWT_PAC8 = 0,
	DWT_PAC16,
	DWT_PAC32,
	DWT_PAC64,
	DWT_NUMOF_PACS,
};

/* Defaults from Table 33: Sub-Register 0x27:08 – DRX_TUNE2 values */
const uint32_t dwt_tune2_defs[DWT_NUMOF_PRFS][DWT_NUMOF_PACS] = {
	{
		DWT_DRX_TUNE2_PRF16_PAC8,
		DWT_DRX_TUNE2_PRF16_PAC16,
		DWT_DRX_TUNE2_PRF16_PAC32,
		DWT_DRX_TUNE2_PRF16_PAC64
	},
	{
		DWT_DRX_TUNE2_PRF64_PAC8,
		DWT_DRX_TUNE2_PRF64_PAC16,
		DWT_DRX_TUNE2_PRF64_PAC32,
		DWT_DRX_TUNE2_PRF64_PAC64
	}
};

/*
 * Defaults from Table 51:
 * Sub-Register 0x2E:2804 – LDE_REPC configurations for (850 kbps & 6.8 Mbps)
 *
 * For 110 kbps the values have to be divided by 8.
 */
const uint16_t dwt_lde_repc_defs[] = {
	0,
	DWT_LDE_REPC_PCODE_1,
	DWT_LDE_REPC_PCODE_2,
	DWT_LDE_REPC_PCODE_3,
	DWT_LDE_REPC_PCODE_4,
	DWT_LDE_REPC_PCODE_5,
	DWT_LDE_REPC_PCODE_6,
	DWT_LDE_REPC_PCODE_7,
	DWT_LDE_REPC_PCODE_8,
	DWT_LDE_REPC_PCODE_9,
	DWT_LDE_REPC_PCODE_10,
	DWT_LDE_REPC_PCODE_11,
	DWT_LDE_REPC_PCODE_12,
	DWT_LDE_REPC_PCODE_13,
	DWT_LDE_REPC_PCODE_14,
	DWT_LDE_REPC_PCODE_15,
	DWT_LDE_REPC_PCODE_16,
	DWT_LDE_REPC_PCODE_17,
	DWT_LDE_REPC_PCODE_18,
	DWT_LDE_REPC_PCODE_19,
	DWT_LDE_REPC_PCODE_20,
	DWT_LDE_REPC_PCODE_21,
	DWT_LDE_REPC_PCODE_22,
	DWT_LDE_REPC_PCODE_23,
	DWT_LDE_REPC_PCODE_24
};

enum dwt_plen_idx {
	DWT_PLEN_64 = 0,
	DWT_PLEN_128,
	DWT_PLEN_256,
	DWT_PLEN_512,
	DWT_PLEN_1024,
	DWT_PLEN_2048,
	DWT_PLEN_4096,
	DWT_NUM_OF_PLEN,
};

/*
 * Transmit Preamble Symbol Repetitions (TXPSR) and Preamble Extension (PE)
 * constants for TX_FCTRL - Transmit Frame Control register.
 * From Table 16: Preamble length selection
 *       BIT(19) | BIT(18) | BIT(21) | BIT(20)
 */
const uint32_t dwt_plen_cfg[] = {
	(0       | BIT(18) | 0       | 0),
	(0       | BIT(18) | 0       | BIT(20)),
	(0       | BIT(18) | BIT(21) | 0),
	(0       | BIT(18) | BIT(21) | BIT(20)),
	(BIT(19) | 0       | 0       | 0),
	(BIT(19) | 0       | BIT(21) | 0),
	(BIT(19) | BIT(18) | 0       | 0),
};

/*
 * Noise Threshold Multiplier (default NTM is 13) and
 * Peak Multiplier (default PMULT is 3).
 */
#define DWT_DEFAULT_LDE_CFG1		((3 << 5) | 13)

/* From Table 50: Sub-Register 0x2E:1806– LDE_CFG2 values */
#define DWT_DEFAULT_LDE_CFG2_PRF64	0x0607
#define DWT_DEFAULT_LDE_CFG2_PRF16	0x1607

#define DWT_RX_SIG_PWR_A_CONST_PRF64	121.74
#define DWT_RX_SIG_PWR_A_CONST_PRF16	113.77

#define DWT_DEVICE_ID		0xDECA0130
#define DWT_SFDTOC_DEF		0x1041

#define DWT_OTP_LDOTUNE_ADDR	0x04
#define DWT_OTP_PARTID_ADDR	0x06
#define DWT_OTP_LOTID_ADDR	0x07
#define DWT_OTP_VBAT_ADDR	0x08
#define DWT_OTP_VTEMP_ADDR	0x09
#define DWT_OTP_XTRIM_ADDR	0x1E

#endif /* ZEPHYR_INCLUDE_DW1000_REGS_H_ */
