/**
 * \file
 *
 * \brief Instance description for GMAC
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

#ifndef _SAME70_GMAC_INSTANCE_H_
#define _SAME70_GMAC_INSTANCE_H_

/* ========== Register definition for GMAC peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_GMAC_SAB1           (0x40050088) /**< (GMAC) Specific Address 1 Bottom Register 0 */
#define REG_GMAC_SAT1           (0x4005008C) /**< (GMAC) Specific Address 1 Top Register 0 */
#define REG_GMAC_SAB2           (0x40050090) /**< (GMAC) Specific Address 1 Bottom Register 1 */
#define REG_GMAC_SAT2           (0x40050094) /**< (GMAC) Specific Address 1 Top Register 1 */
#define REG_GMAC_SAB3           (0x40050098) /**< (GMAC) Specific Address 1 Bottom Register 2 */
#define REG_GMAC_SAT3           (0x4005009C) /**< (GMAC) Specific Address 1 Top Register 2 */
#define REG_GMAC_SAB4           (0x400500A0) /**< (GMAC) Specific Address 1 Bottom Register 3 */
#define REG_GMAC_SAT4           (0x400500A4) /**< (GMAC) Specific Address 1 Top Register 3 */
#define REG_GMAC_NCR            (0x40050000) /**< (GMAC) Network Control Register */
#define REG_GMAC_NCFGR          (0x40050004) /**< (GMAC) Network Configuration Register */
#define REG_GMAC_NSR            (0x40050008) /**< (GMAC) Network Status Register */
#define REG_GMAC_UR             (0x4005000C) /**< (GMAC) User Register */
#define REG_GMAC_DCFGR          (0x40050010) /**< (GMAC) DMA Configuration Register */
#define REG_GMAC_TSR            (0x40050014) /**< (GMAC) Transmit Status Register */
#define REG_GMAC_RBQB           (0x40050018) /**< (GMAC) Receive Buffer Queue Base Address Register */
#define REG_GMAC_TBQB           (0x4005001C) /**< (GMAC) Transmit Buffer Queue Base Address Register */
#define REG_GMAC_RSR            (0x40050020) /**< (GMAC) Receive Status Register */
#define REG_GMAC_ISR            (0x40050024) /**< (GMAC) Interrupt Status Register */
#define REG_GMAC_IER            (0x40050028) /**< (GMAC) Interrupt Enable Register */
#define REG_GMAC_IDR            (0x4005002C) /**< (GMAC) Interrupt Disable Register */
#define REG_GMAC_IMR            (0x40050030) /**< (GMAC) Interrupt Mask Register */
#define REG_GMAC_MAN            (0x40050034) /**< (GMAC) PHY Maintenance Register */
#define REG_GMAC_RPQ            (0x40050038) /**< (GMAC) Received Pause Quantum Register */
#define REG_GMAC_TPQ            (0x4005003C) /**< (GMAC) Transmit Pause Quantum Register */
#define REG_GMAC_TPSF           (0x40050040) /**< (GMAC) TX Partial Store and Forward Register */
#define REG_GMAC_RPSF           (0x40050044) /**< (GMAC) RX Partial Store and Forward Register */
#define REG_GMAC_RJFML          (0x40050048) /**< (GMAC) RX Jumbo Frame Max Length Register */
#define REG_GMAC_HRB            (0x40050080) /**< (GMAC) Hash Register Bottom */
#define REG_GMAC_HRT            (0x40050084) /**< (GMAC) Hash Register Top */
#define REG_GMAC_TIDM1          (0x400500A8) /**< (GMAC) Type ID Match 1 Register */
#define REG_GMAC_TIDM2          (0x400500AC) /**< (GMAC) Type ID Match 2 Register */
#define REG_GMAC_TIDM3          (0x400500B0) /**< (GMAC) Type ID Match 3 Register */
#define REG_GMAC_TIDM4          (0x400500B4) /**< (GMAC) Type ID Match 4 Register */
#define REG_GMAC_WOL            (0x400500B8) /**< (GMAC) Wake on LAN Register */
#define REG_GMAC_IPGS           (0x400500BC) /**< (GMAC) IPG Stretch Register */
#define REG_GMAC_SVLAN          (0x400500C0) /**< (GMAC) Stacked VLAN Register */
#define REG_GMAC_TPFCP          (0x400500C4) /**< (GMAC) Transmit PFC Pause Register */
#define REG_GMAC_SAMB1          (0x400500C8) /**< (GMAC) Specific Address 1 Mask Bottom Register */
#define REG_GMAC_SAMT1          (0x400500CC) /**< (GMAC) Specific Address 1 Mask Top Register */
#define REG_GMAC_NSC            (0x400500DC) /**< (GMAC) 1588 Timer Nanosecond Comparison Register */
#define REG_GMAC_SCL            (0x400500E0) /**< (GMAC) 1588 Timer Second Comparison Low Register */
#define REG_GMAC_SCH            (0x400500E4) /**< (GMAC) 1588 Timer Second Comparison High Register */
#define REG_GMAC_EFTSH          (0x400500E8) /**< (GMAC) PTP Event Frame Transmitted Seconds High Register */
#define REG_GMAC_EFRSH          (0x400500EC) /**< (GMAC) PTP Event Frame Received Seconds High Register */
#define REG_GMAC_PEFTSH         (0x400500F0) /**< (GMAC) PTP Peer Event Frame Transmitted Seconds High Register */
#define REG_GMAC_PEFRSH         (0x400500F4) /**< (GMAC) PTP Peer Event Frame Received Seconds High Register */
#define REG_GMAC_OTLO           (0x40050100) /**< (GMAC) Octets Transmitted Low Register */
#define REG_GMAC_OTHI           (0x40050104) /**< (GMAC) Octets Transmitted High Register */
#define REG_GMAC_FT             (0x40050108) /**< (GMAC) Frames Transmitted Register */
#define REG_GMAC_BCFT           (0x4005010C) /**< (GMAC) Broadcast Frames Transmitted Register */
#define REG_GMAC_MFT            (0x40050110) /**< (GMAC) Multicast Frames Transmitted Register */
#define REG_GMAC_PFT            (0x40050114) /**< (GMAC) Pause Frames Transmitted Register */
#define REG_GMAC_BFT64          (0x40050118) /**< (GMAC) 64 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT127        (0x4005011C) /**< (GMAC) 65 to 127 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT255        (0x40050120) /**< (GMAC) 128 to 255 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT511        (0x40050124) /**< (GMAC) 256 to 511 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT1023       (0x40050128) /**< (GMAC) 512 to 1023 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT1518       (0x4005012C) /**< (GMAC) 1024 to 1518 Byte Frames Transmitted Register */
#define REG_GMAC_GTBFT1518      (0x40050130) /**< (GMAC) Greater Than 1518 Byte Frames Transmitted Register */
#define REG_GMAC_TUR            (0x40050134) /**< (GMAC) Transmit Underruns Register */
#define REG_GMAC_SCF            (0x40050138) /**< (GMAC) Single Collision Frames Register */
#define REG_GMAC_MCF            (0x4005013C) /**< (GMAC) Multiple Collision Frames Register */
#define REG_GMAC_EC             (0x40050140) /**< (GMAC) Excessive Collisions Register */
#define REG_GMAC_LC             (0x40050144) /**< (GMAC) Late Collisions Register */
#define REG_GMAC_DTF            (0x40050148) /**< (GMAC) Deferred Transmission Frames Register */
#define REG_GMAC_CSE            (0x4005014C) /**< (GMAC) Carrier Sense Errors Register */
#define REG_GMAC_ORLO           (0x40050150) /**< (GMAC) Octets Received Low Received Register */
#define REG_GMAC_ORHI           (0x40050154) /**< (GMAC) Octets Received High Received Register */
#define REG_GMAC_FR             (0x40050158) /**< (GMAC) Frames Received Register */
#define REG_GMAC_BCFR           (0x4005015C) /**< (GMAC) Broadcast Frames Received Register */
#define REG_GMAC_MFR            (0x40050160) /**< (GMAC) Multicast Frames Received Register */
#define REG_GMAC_PFR            (0x40050164) /**< (GMAC) Pause Frames Received Register */
#define REG_GMAC_BFR64          (0x40050168) /**< (GMAC) 64 Byte Frames Received Register */
#define REG_GMAC_TBFR127        (0x4005016C) /**< (GMAC) 65 to 127 Byte Frames Received Register */
#define REG_GMAC_TBFR255        (0x40050170) /**< (GMAC) 128 to 255 Byte Frames Received Register */
#define REG_GMAC_TBFR511        (0x40050174) /**< (GMAC) 256 to 511 Byte Frames Received Register */
#define REG_GMAC_TBFR1023       (0x40050178) /**< (GMAC) 512 to 1023 Byte Frames Received Register */
#define REG_GMAC_TBFR1518       (0x4005017C) /**< (GMAC) 1024 to 1518 Byte Frames Received Register */
#define REG_GMAC_TMXBFR         (0x40050180) /**< (GMAC) 1519 to Maximum Byte Frames Received Register */
#define REG_GMAC_UFR            (0x40050184) /**< (GMAC) Undersize Frames Received Register */
#define REG_GMAC_OFR            (0x40050188) /**< (GMAC) Oversize Frames Received Register */
#define REG_GMAC_JR             (0x4005018C) /**< (GMAC) Jabbers Received Register */
#define REG_GMAC_FCSE           (0x40050190) /**< (GMAC) Frame Check Sequence Errors Register */
#define REG_GMAC_LFFE           (0x40050194) /**< (GMAC) Length Field Frame Errors Register */
#define REG_GMAC_RSE            (0x40050198) /**< (GMAC) Receive Symbol Errors Register */
#define REG_GMAC_AE             (0x4005019C) /**< (GMAC) Alignment Errors Register */
#define REG_GMAC_RRE            (0x400501A0) /**< (GMAC) Receive Resource Errors Register */
#define REG_GMAC_ROE            (0x400501A4) /**< (GMAC) Receive Overrun Register */
#define REG_GMAC_IHCE           (0x400501A8) /**< (GMAC) IP Header Checksum Errors Register */
#define REG_GMAC_TCE            (0x400501AC) /**< (GMAC) TCP Checksum Errors Register */
#define REG_GMAC_UCE            (0x400501B0) /**< (GMAC) UDP Checksum Errors Register */
#define REG_GMAC_TISUBN         (0x400501BC) /**< (GMAC) 1588 Timer Increment Sub-nanoseconds Register */
#define REG_GMAC_TSH            (0x400501C0) /**< (GMAC) 1588 Timer Seconds High Register */
#define REG_GMAC_TSL            (0x400501D0) /**< (GMAC) 1588 Timer Seconds Low Register */
#define REG_GMAC_TN             (0x400501D4) /**< (GMAC) 1588 Timer Nanoseconds Register */
#define REG_GMAC_TA             (0x400501D8) /**< (GMAC) 1588 Timer Adjust Register */
#define REG_GMAC_TI             (0x400501DC) /**< (GMAC) 1588 Timer Increment Register */
#define REG_GMAC_EFTSL          (0x400501E0) /**< (GMAC) PTP Event Frame Transmitted Seconds Low Register */
#define REG_GMAC_EFTN           (0x400501E4) /**< (GMAC) PTP Event Frame Transmitted Nanoseconds Register */
#define REG_GMAC_EFRSL          (0x400501E8) /**< (GMAC) PTP Event Frame Received Seconds Low Register */
#define REG_GMAC_EFRN           (0x400501EC) /**< (GMAC) PTP Event Frame Received Nanoseconds Register */
#define REG_GMAC_PEFTSL         (0x400501F0) /**< (GMAC) PTP Peer Event Frame Transmitted Seconds Low Register */
#define REG_GMAC_PEFTN          (0x400501F4) /**< (GMAC) PTP Peer Event Frame Transmitted Nanoseconds Register */
#define REG_GMAC_PEFRSL         (0x400501F8) /**< (GMAC) PTP Peer Event Frame Received Seconds Low Register */
#define REG_GMAC_PEFRN          (0x400501FC) /**< (GMAC) PTP Peer Event Frame Received Nanoseconds Register */
#define REG_GMAC_ISRPQ          (0x40050400) /**< (GMAC) Interrupt Status Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_ISRPQ0         (0x40050400) /**< (GMAC) Interrupt Status Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_ISRPQ1         (0x40050404) /**< (GMAC) Interrupt Status Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_TBQBAPQ        (0x40050440) /**< (GMAC) Transmit Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_TBQBAPQ0       (0x40050440) /**< (GMAC) Transmit Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_TBQBAPQ1       (0x40050444) /**< (GMAC) Transmit Buffer Queue Base Address Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_RBQBAPQ        (0x40050480) /**< (GMAC) Receive Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_RBQBAPQ0       (0x40050480) /**< (GMAC) Receive Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_RBQBAPQ1       (0x40050484) /**< (GMAC) Receive Buffer Queue Base Address Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_RBSRPQ         (0x400504A0) /**< (GMAC) Receive Buffer Size Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_RBSRPQ0        (0x400504A0) /**< (GMAC) Receive Buffer Size Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_RBSRPQ1        (0x400504A4) /**< (GMAC) Receive Buffer Size Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_CBSCR          (0x400504BC) /**< (GMAC) Credit-Based Shaping Control Register */
#define REG_GMAC_CBSISQA        (0x400504C0) /**< (GMAC) Credit-Based Shaping IdleSlope Register for Queue A */
#define REG_GMAC_CBSISQB        (0x400504C4) /**< (GMAC) Credit-Based Shaping IdleSlope Register for Queue B */
#define REG_GMAC_ST1RPQ         (0x40050500) /**< (GMAC) Screening Type 1 Register Priority Queue  (index = 0) 0 */
#define REG_GMAC_ST1RPQ0        (0x40050500) /**< (GMAC) Screening Type 1 Register Priority Queue  (index = 0) 0 */
#define REG_GMAC_ST1RPQ1        (0x40050504) /**< (GMAC) Screening Type 1 Register Priority Queue  (index = 0) 1 */
#define REG_GMAC_ST1RPQ2        (0x40050508) /**< (GMAC) Screening Type 1 Register Priority Queue  (index = 0) 2 */
#define REG_GMAC_ST1RPQ3        (0x4005050C) /**< (GMAC) Screening Type 1 Register Priority Queue  (index = 0) 3 */
#define REG_GMAC_ST2RPQ         (0x40050540) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 0 */
#define REG_GMAC_ST2RPQ0        (0x40050540) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 0 */
#define REG_GMAC_ST2RPQ1        (0x40050544) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 1 */
#define REG_GMAC_ST2RPQ2        (0x40050548) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 2 */
#define REG_GMAC_ST2RPQ3        (0x4005054C) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 3 */
#define REG_GMAC_ST2RPQ4        (0x40050550) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 4 */
#define REG_GMAC_ST2RPQ5        (0x40050554) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 5 */
#define REG_GMAC_ST2RPQ6        (0x40050558) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 6 */
#define REG_GMAC_ST2RPQ7        (0x4005055C) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 7 */
#define REG_GMAC_IERPQ          (0x40050600) /**< (GMAC) Interrupt Enable Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IERPQ0         (0x40050600) /**< (GMAC) Interrupt Enable Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IERPQ1         (0x40050604) /**< (GMAC) Interrupt Enable Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_IDRPQ          (0x40050620) /**< (GMAC) Interrupt Disable Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IDRPQ0         (0x40050620) /**< (GMAC) Interrupt Disable Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IDRPQ1         (0x40050624) /**< (GMAC) Interrupt Disable Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_IMRPQ          (0x40050640) /**< (GMAC) Interrupt Mask Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IMRPQ0         (0x40050640) /**< (GMAC) Interrupt Mask Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IMRPQ1         (0x40050644) /**< (GMAC) Interrupt Mask Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_ST2ER          (0x400506E0) /**< (GMAC) Screening Type 2 Ethertype Register  (index = 0) 0 */
#define REG_GMAC_ST2ER0         (0x400506E0) /**< (GMAC) Screening Type 2 Ethertype Register  (index = 0) 0 */
#define REG_GMAC_ST2ER1         (0x400506E4) /**< (GMAC) Screening Type 2 Ethertype Register  (index = 0) 1 */
#define REG_GMAC_ST2ER2         (0x400506E8) /**< (GMAC) Screening Type 2 Ethertype Register  (index = 0) 2 */
#define REG_GMAC_ST2ER3         (0x400506EC) /**< (GMAC) Screening Type 2 Ethertype Register  (index = 0) 3 */
#define REG_GMAC_ST2CW00        (0x40050700) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 0) */
#define REG_GMAC_ST2CW10        (0x40050704) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 0) */
#define REG_GMAC_ST2CW01        (0x40050708) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 1) */
#define REG_GMAC_ST2CW11        (0x4005070C) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 1) */
#define REG_GMAC_ST2CW02        (0x40050710) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 2) */
#define REG_GMAC_ST2CW12        (0x40050714) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 2) */
#define REG_GMAC_ST2CW03        (0x40050718) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 3) */
#define REG_GMAC_ST2CW13        (0x4005071C) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 3) */
#define REG_GMAC_ST2CW04        (0x40050720) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 4) */
#define REG_GMAC_ST2CW14        (0x40050724) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 4) */
#define REG_GMAC_ST2CW05        (0x40050728) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 5) */
#define REG_GMAC_ST2CW15        (0x4005072C) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 5) */
#define REG_GMAC_ST2CW06        (0x40050730) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 6) */
#define REG_GMAC_ST2CW16        (0x40050734) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 6) */
#define REG_GMAC_ST2CW07        (0x40050738) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 7) */
#define REG_GMAC_ST2CW17        (0x4005073C) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 7) */
#define REG_GMAC_ST2CW08        (0x40050740) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 8) */
#define REG_GMAC_ST2CW18        (0x40050744) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 8) */
#define REG_GMAC_ST2CW09        (0x40050748) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 9) */
#define REG_GMAC_ST2CW19        (0x4005074C) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 9) */
#define REG_GMAC_ST2CW010       (0x40050750) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 10) */
#define REG_GMAC_ST2CW110       (0x40050754) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 10) */
#define REG_GMAC_ST2CW011       (0x40050758) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 11) */
#define REG_GMAC_ST2CW111       (0x4005075C) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 11) */
#define REG_GMAC_ST2CW012       (0x40050760) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 12) */
#define REG_GMAC_ST2CW112       (0x40050764) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 12) */
#define REG_GMAC_ST2CW013       (0x40050768) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 13) */
#define REG_GMAC_ST2CW113       (0x4005076C) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 13) */
#define REG_GMAC_ST2CW014       (0x40050770) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 14) */
#define REG_GMAC_ST2CW114       (0x40050774) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 14) */
#define REG_GMAC_ST2CW015       (0x40050778) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 15) */
#define REG_GMAC_ST2CW115       (0x4005077C) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 15) */
#define REG_GMAC_ST2CW016       (0x40050780) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 16) */
#define REG_GMAC_ST2CW116       (0x40050784) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 16) */
#define REG_GMAC_ST2CW017       (0x40050788) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 17) */
#define REG_GMAC_ST2CW117       (0x4005078C) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 17) */
#define REG_GMAC_ST2CW018       (0x40050790) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 18) */
#define REG_GMAC_ST2CW118       (0x40050794) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 18) */
#define REG_GMAC_ST2CW019       (0x40050798) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 19) */
#define REG_GMAC_ST2CW119       (0x4005079C) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 19) */
#define REG_GMAC_ST2CW020       (0x400507A0) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 20) */
#define REG_GMAC_ST2CW120       (0x400507A4) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 20) */
#define REG_GMAC_ST2CW021       (0x400507A8) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 21) */
#define REG_GMAC_ST2CW121       (0x400507AC) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 21) */
#define REG_GMAC_ST2CW022       (0x400507B0) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 22) */
#define REG_GMAC_ST2CW122       (0x400507B4) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 22) */
#define REG_GMAC_ST2CW023       (0x400507B8) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 23) */
#define REG_GMAC_ST2CW123       (0x400507BC) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 23) */

#else

#define REG_GMAC_SAB1           (*(__IO uint32_t*)0x40050088U) /**< (GMAC) Specific Address 1 Bottom Register 0 */
#define REG_GMAC_SAT1           (*(__IO uint32_t*)0x4005008CU) /**< (GMAC) Specific Address 1 Top Register 0 */
#define REG_GMAC_SAB2           (*(__IO uint32_t*)0x40050090U) /**< (GMAC) Specific Address 1 Bottom Register 1 */
#define REG_GMAC_SAT2           (*(__IO uint32_t*)0x40050094U) /**< (GMAC) Specific Address 1 Top Register 1 */
#define REG_GMAC_SAB3           (*(__IO uint32_t*)0x40050098U) /**< (GMAC) Specific Address 1 Bottom Register 2 */
#define REG_GMAC_SAT3           (*(__IO uint32_t*)0x4005009CU) /**< (GMAC) Specific Address 1 Top Register 2 */
#define REG_GMAC_SAB4           (*(__IO uint32_t*)0x400500A0U) /**< (GMAC) Specific Address 1 Bottom Register 3 */
#define REG_GMAC_SAT4           (*(__IO uint32_t*)0x400500A4U) /**< (GMAC) Specific Address 1 Top Register 3 */
#define REG_GMAC_NCR            (*(__IO uint32_t*)0x40050000U) /**< (GMAC) Network Control Register */
#define REG_GMAC_NCFGR          (*(__IO uint32_t*)0x40050004U) /**< (GMAC) Network Configuration Register */
#define REG_GMAC_NSR            (*(__I  uint32_t*)0x40050008U) /**< (GMAC) Network Status Register */
#define REG_GMAC_UR             (*(__IO uint32_t*)0x4005000CU) /**< (GMAC) User Register */
#define REG_GMAC_DCFGR          (*(__IO uint32_t*)0x40050010U) /**< (GMAC) DMA Configuration Register */
#define REG_GMAC_TSR            (*(__IO uint32_t*)0x40050014U) /**< (GMAC) Transmit Status Register */
#define REG_GMAC_RBQB           (*(__IO uint32_t*)0x40050018U) /**< (GMAC) Receive Buffer Queue Base Address Register */
#define REG_GMAC_TBQB           (*(__IO uint32_t*)0x4005001CU) /**< (GMAC) Transmit Buffer Queue Base Address Register */
#define REG_GMAC_RSR            (*(__IO uint32_t*)0x40050020U) /**< (GMAC) Receive Status Register */
#define REG_GMAC_ISR            (*(__I  uint32_t*)0x40050024U) /**< (GMAC) Interrupt Status Register */
#define REG_GMAC_IER            (*(__O  uint32_t*)0x40050028U) /**< (GMAC) Interrupt Enable Register */
#define REG_GMAC_IDR            (*(__O  uint32_t*)0x4005002CU) /**< (GMAC) Interrupt Disable Register */
#define REG_GMAC_IMR            (*(__IO uint32_t*)0x40050030U) /**< (GMAC) Interrupt Mask Register */
#define REG_GMAC_MAN            (*(__IO uint32_t*)0x40050034U) /**< (GMAC) PHY Maintenance Register */
#define REG_GMAC_RPQ            (*(__I  uint32_t*)0x40050038U) /**< (GMAC) Received Pause Quantum Register */
#define REG_GMAC_TPQ            (*(__IO uint32_t*)0x4005003CU) /**< (GMAC) Transmit Pause Quantum Register */
#define REG_GMAC_TPSF           (*(__IO uint32_t*)0x40050040U) /**< (GMAC) TX Partial Store and Forward Register */
#define REG_GMAC_RPSF           (*(__IO uint32_t*)0x40050044U) /**< (GMAC) RX Partial Store and Forward Register */
#define REG_GMAC_RJFML          (*(__IO uint32_t*)0x40050048U) /**< (GMAC) RX Jumbo Frame Max Length Register */
#define REG_GMAC_HRB            (*(__IO uint32_t*)0x40050080U) /**< (GMAC) Hash Register Bottom */
#define REG_GMAC_HRT            (*(__IO uint32_t*)0x40050084U) /**< (GMAC) Hash Register Top */
#define REG_GMAC_TIDM1          (*(__IO uint32_t*)0x400500A8U) /**< (GMAC) Type ID Match 1 Register */
#define REG_GMAC_TIDM2          (*(__IO uint32_t*)0x400500ACU) /**< (GMAC) Type ID Match 2 Register */
#define REG_GMAC_TIDM3          (*(__IO uint32_t*)0x400500B0U) /**< (GMAC) Type ID Match 3 Register */
#define REG_GMAC_TIDM4          (*(__IO uint32_t*)0x400500B4U) /**< (GMAC) Type ID Match 4 Register */
#define REG_GMAC_WOL            (*(__IO uint32_t*)0x400500B8U) /**< (GMAC) Wake on LAN Register */
#define REG_GMAC_IPGS           (*(__IO uint32_t*)0x400500BCU) /**< (GMAC) IPG Stretch Register */
#define REG_GMAC_SVLAN          (*(__IO uint32_t*)0x400500C0U) /**< (GMAC) Stacked VLAN Register */
#define REG_GMAC_TPFCP          (*(__IO uint32_t*)0x400500C4U) /**< (GMAC) Transmit PFC Pause Register */
#define REG_GMAC_SAMB1          (*(__IO uint32_t*)0x400500C8U) /**< (GMAC) Specific Address 1 Mask Bottom Register */
#define REG_GMAC_SAMT1          (*(__IO uint32_t*)0x400500CCU) /**< (GMAC) Specific Address 1 Mask Top Register */
#define REG_GMAC_NSC            (*(__IO uint32_t*)0x400500DCU) /**< (GMAC) 1588 Timer Nanosecond Comparison Register */
#define REG_GMAC_SCL            (*(__IO uint32_t*)0x400500E0U) /**< (GMAC) 1588 Timer Second Comparison Low Register */
#define REG_GMAC_SCH            (*(__IO uint32_t*)0x400500E4U) /**< (GMAC) 1588 Timer Second Comparison High Register */
#define REG_GMAC_EFTSH          (*(__I  uint32_t*)0x400500E8U) /**< (GMAC) PTP Event Frame Transmitted Seconds High Register */
#define REG_GMAC_EFRSH          (*(__I  uint32_t*)0x400500ECU) /**< (GMAC) PTP Event Frame Received Seconds High Register */
#define REG_GMAC_PEFTSH         (*(__I  uint32_t*)0x400500F0U) /**< (GMAC) PTP Peer Event Frame Transmitted Seconds High Register */
#define REG_GMAC_PEFRSH         (*(__I  uint32_t*)0x400500F4U) /**< (GMAC) PTP Peer Event Frame Received Seconds High Register */
#define REG_GMAC_OTLO           (*(__I  uint32_t*)0x40050100U) /**< (GMAC) Octets Transmitted Low Register */
#define REG_GMAC_OTHI           (*(__I  uint32_t*)0x40050104U) /**< (GMAC) Octets Transmitted High Register */
#define REG_GMAC_FT             (*(__I  uint32_t*)0x40050108U) /**< (GMAC) Frames Transmitted Register */
#define REG_GMAC_BCFT           (*(__I  uint32_t*)0x4005010CU) /**< (GMAC) Broadcast Frames Transmitted Register */
#define REG_GMAC_MFT            (*(__I  uint32_t*)0x40050110U) /**< (GMAC) Multicast Frames Transmitted Register */
#define REG_GMAC_PFT            (*(__I  uint32_t*)0x40050114U) /**< (GMAC) Pause Frames Transmitted Register */
#define REG_GMAC_BFT64          (*(__I  uint32_t*)0x40050118U) /**< (GMAC) 64 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT127        (*(__I  uint32_t*)0x4005011CU) /**< (GMAC) 65 to 127 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT255        (*(__I  uint32_t*)0x40050120U) /**< (GMAC) 128 to 255 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT511        (*(__I  uint32_t*)0x40050124U) /**< (GMAC) 256 to 511 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT1023       (*(__I  uint32_t*)0x40050128U) /**< (GMAC) 512 to 1023 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT1518       (*(__I  uint32_t*)0x4005012CU) /**< (GMAC) 1024 to 1518 Byte Frames Transmitted Register */
#define REG_GMAC_GTBFT1518      (*(__I  uint32_t*)0x40050130U) /**< (GMAC) Greater Than 1518 Byte Frames Transmitted Register */
#define REG_GMAC_TUR            (*(__I  uint32_t*)0x40050134U) /**< (GMAC) Transmit Underruns Register */
#define REG_GMAC_SCF            (*(__I  uint32_t*)0x40050138U) /**< (GMAC) Single Collision Frames Register */
#define REG_GMAC_MCF            (*(__I  uint32_t*)0x4005013CU) /**< (GMAC) Multiple Collision Frames Register */
#define REG_GMAC_EC             (*(__I  uint32_t*)0x40050140U) /**< (GMAC) Excessive Collisions Register */
#define REG_GMAC_LC             (*(__I  uint32_t*)0x40050144U) /**< (GMAC) Late Collisions Register */
#define REG_GMAC_DTF            (*(__I  uint32_t*)0x40050148U) /**< (GMAC) Deferred Transmission Frames Register */
#define REG_GMAC_CSE            (*(__I  uint32_t*)0x4005014CU) /**< (GMAC) Carrier Sense Errors Register */
#define REG_GMAC_ORLO           (*(__I  uint32_t*)0x40050150U) /**< (GMAC) Octets Received Low Received Register */
#define REG_GMAC_ORHI           (*(__I  uint32_t*)0x40050154U) /**< (GMAC) Octets Received High Received Register */
#define REG_GMAC_FR             (*(__I  uint32_t*)0x40050158U) /**< (GMAC) Frames Received Register */
#define REG_GMAC_BCFR           (*(__I  uint32_t*)0x4005015CU) /**< (GMAC) Broadcast Frames Received Register */
#define REG_GMAC_MFR            (*(__I  uint32_t*)0x40050160U) /**< (GMAC) Multicast Frames Received Register */
#define REG_GMAC_PFR            (*(__I  uint32_t*)0x40050164U) /**< (GMAC) Pause Frames Received Register */
#define REG_GMAC_BFR64          (*(__I  uint32_t*)0x40050168U) /**< (GMAC) 64 Byte Frames Received Register */
#define REG_GMAC_TBFR127        (*(__I  uint32_t*)0x4005016CU) /**< (GMAC) 65 to 127 Byte Frames Received Register */
#define REG_GMAC_TBFR255        (*(__I  uint32_t*)0x40050170U) /**< (GMAC) 128 to 255 Byte Frames Received Register */
#define REG_GMAC_TBFR511        (*(__I  uint32_t*)0x40050174U) /**< (GMAC) 256 to 511 Byte Frames Received Register */
#define REG_GMAC_TBFR1023       (*(__I  uint32_t*)0x40050178U) /**< (GMAC) 512 to 1023 Byte Frames Received Register */
#define REG_GMAC_TBFR1518       (*(__I  uint32_t*)0x4005017CU) /**< (GMAC) 1024 to 1518 Byte Frames Received Register */
#define REG_GMAC_TMXBFR         (*(__I  uint32_t*)0x40050180U) /**< (GMAC) 1519 to Maximum Byte Frames Received Register */
#define REG_GMAC_UFR            (*(__I  uint32_t*)0x40050184U) /**< (GMAC) Undersize Frames Received Register */
#define REG_GMAC_OFR            (*(__I  uint32_t*)0x40050188U) /**< (GMAC) Oversize Frames Received Register */
#define REG_GMAC_JR             (*(__I  uint32_t*)0x4005018CU) /**< (GMAC) Jabbers Received Register */
#define REG_GMAC_FCSE           (*(__I  uint32_t*)0x40050190U) /**< (GMAC) Frame Check Sequence Errors Register */
#define REG_GMAC_LFFE           (*(__I  uint32_t*)0x40050194U) /**< (GMAC) Length Field Frame Errors Register */
#define REG_GMAC_RSE            (*(__I  uint32_t*)0x40050198U) /**< (GMAC) Receive Symbol Errors Register */
#define REG_GMAC_AE             (*(__I  uint32_t*)0x4005019CU) /**< (GMAC) Alignment Errors Register */
#define REG_GMAC_RRE            (*(__I  uint32_t*)0x400501A0U) /**< (GMAC) Receive Resource Errors Register */
#define REG_GMAC_ROE            (*(__I  uint32_t*)0x400501A4U) /**< (GMAC) Receive Overrun Register */
#define REG_GMAC_IHCE           (*(__I  uint32_t*)0x400501A8U) /**< (GMAC) IP Header Checksum Errors Register */
#define REG_GMAC_TCE            (*(__I  uint32_t*)0x400501ACU) /**< (GMAC) TCP Checksum Errors Register */
#define REG_GMAC_UCE            (*(__I  uint32_t*)0x400501B0U) /**< (GMAC) UDP Checksum Errors Register */
#define REG_GMAC_TISUBN         (*(__IO uint32_t*)0x400501BCU) /**< (GMAC) 1588 Timer Increment Sub-nanoseconds Register */
#define REG_GMAC_TSH            (*(__IO uint32_t*)0x400501C0U) /**< (GMAC) 1588 Timer Seconds High Register */
#define REG_GMAC_TSL            (*(__IO uint32_t*)0x400501D0U) /**< (GMAC) 1588 Timer Seconds Low Register */
#define REG_GMAC_TN             (*(__IO uint32_t*)0x400501D4U) /**< (GMAC) 1588 Timer Nanoseconds Register */
#define REG_GMAC_TA             (*(__O  uint32_t*)0x400501D8U) /**< (GMAC) 1588 Timer Adjust Register */
#define REG_GMAC_TI             (*(__IO uint32_t*)0x400501DCU) /**< (GMAC) 1588 Timer Increment Register */
#define REG_GMAC_EFTSL          (*(__I  uint32_t*)0x400501E0U) /**< (GMAC) PTP Event Frame Transmitted Seconds Low Register */
#define REG_GMAC_EFTN           (*(__I  uint32_t*)0x400501E4U) /**< (GMAC) PTP Event Frame Transmitted Nanoseconds Register */
#define REG_GMAC_EFRSL          (*(__I  uint32_t*)0x400501E8U) /**< (GMAC) PTP Event Frame Received Seconds Low Register */
#define REG_GMAC_EFRN           (*(__I  uint32_t*)0x400501ECU) /**< (GMAC) PTP Event Frame Received Nanoseconds Register */
#define REG_GMAC_PEFTSL         (*(__I  uint32_t*)0x400501F0U) /**< (GMAC) PTP Peer Event Frame Transmitted Seconds Low Register */
#define REG_GMAC_PEFTN          (*(__I  uint32_t*)0x400501F4U) /**< (GMAC) PTP Peer Event Frame Transmitted Nanoseconds Register */
#define REG_GMAC_PEFRSL         (*(__I  uint32_t*)0x400501F8U) /**< (GMAC) PTP Peer Event Frame Received Seconds Low Register */
#define REG_GMAC_PEFRN          (*(__I  uint32_t*)0x400501FCU) /**< (GMAC) PTP Peer Event Frame Received Nanoseconds Register */
#define REG_GMAC_ISRPQ          (*(__I  uint32_t*)0x40050400U) /**< (GMAC) Interrupt Status Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_ISRPQ0         (*(__I  uint32_t*)0x40050400U) /**< (GMAC) Interrupt Status Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_ISRPQ1         (*(__I  uint32_t*)0x40050404U) /**< (GMAC) Interrupt Status Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_TBQBAPQ        (*(__IO uint32_t*)0x40050440U) /**< (GMAC) Transmit Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_TBQBAPQ0       (*(__IO uint32_t*)0x40050440U) /**< (GMAC) Transmit Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_TBQBAPQ1       (*(__IO uint32_t*)0x40050444U) /**< (GMAC) Transmit Buffer Queue Base Address Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_RBQBAPQ        (*(__IO uint32_t*)0x40050480U) /**< (GMAC) Receive Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_RBQBAPQ0       (*(__IO uint32_t*)0x40050480U) /**< (GMAC) Receive Buffer Queue Base Address Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_RBQBAPQ1       (*(__IO uint32_t*)0x40050484U) /**< (GMAC) Receive Buffer Queue Base Address Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_RBSRPQ         (*(__IO uint32_t*)0x400504A0U) /**< (GMAC) Receive Buffer Size Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_RBSRPQ0        (*(__IO uint32_t*)0x400504A0U) /**< (GMAC) Receive Buffer Size Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_RBSRPQ1        (*(__IO uint32_t*)0x400504A4U) /**< (GMAC) Receive Buffer Size Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_CBSCR          (*(__IO uint32_t*)0x400504BCU) /**< (GMAC) Credit-Based Shaping Control Register */
#define REG_GMAC_CBSISQA        (*(__IO uint32_t*)0x400504C0U) /**< (GMAC) Credit-Based Shaping IdleSlope Register for Queue A */
#define REG_GMAC_CBSISQB        (*(__IO uint32_t*)0x400504C4U) /**< (GMAC) Credit-Based Shaping IdleSlope Register for Queue B */
#define REG_GMAC_ST1RPQ         (*(__IO uint32_t*)0x40050500U) /**< (GMAC) Screening Type 1 Register Priority Queue  (index = 0) 0 */
#define REG_GMAC_ST1RPQ0        (*(__IO uint32_t*)0x40050500U) /**< (GMAC) Screening Type 1 Register Priority Queue  (index = 0) 0 */
#define REG_GMAC_ST1RPQ1        (*(__IO uint32_t*)0x40050504U) /**< (GMAC) Screening Type 1 Register Priority Queue  (index = 0) 1 */
#define REG_GMAC_ST1RPQ2        (*(__IO uint32_t*)0x40050508U) /**< (GMAC) Screening Type 1 Register Priority Queue  (index = 0) 2 */
#define REG_GMAC_ST1RPQ3        (*(__IO uint32_t*)0x4005050CU) /**< (GMAC) Screening Type 1 Register Priority Queue  (index = 0) 3 */
#define REG_GMAC_ST2RPQ         (*(__IO uint32_t*)0x40050540U) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 0 */
#define REG_GMAC_ST2RPQ0        (*(__IO uint32_t*)0x40050540U) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 0 */
#define REG_GMAC_ST2RPQ1        (*(__IO uint32_t*)0x40050544U) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 1 */
#define REG_GMAC_ST2RPQ2        (*(__IO uint32_t*)0x40050548U) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 2 */
#define REG_GMAC_ST2RPQ3        (*(__IO uint32_t*)0x4005054CU) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 3 */
#define REG_GMAC_ST2RPQ4        (*(__IO uint32_t*)0x40050550U) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 4 */
#define REG_GMAC_ST2RPQ5        (*(__IO uint32_t*)0x40050554U) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 5 */
#define REG_GMAC_ST2RPQ6        (*(__IO uint32_t*)0x40050558U) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 6 */
#define REG_GMAC_ST2RPQ7        (*(__IO uint32_t*)0x4005055CU) /**< (GMAC) Screening Type 2 Register Priority Queue  (index = 0) 7 */
#define REG_GMAC_IERPQ          (*(__O  uint32_t*)0x40050600U) /**< (GMAC) Interrupt Enable Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IERPQ0         (*(__O  uint32_t*)0x40050600U) /**< (GMAC) Interrupt Enable Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IERPQ1         (*(__O  uint32_t*)0x40050604U) /**< (GMAC) Interrupt Enable Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_IDRPQ          (*(__O  uint32_t*)0x40050620U) /**< (GMAC) Interrupt Disable Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IDRPQ0         (*(__O  uint32_t*)0x40050620U) /**< (GMAC) Interrupt Disable Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IDRPQ1         (*(__O  uint32_t*)0x40050624U) /**< (GMAC) Interrupt Disable Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_IMRPQ          (*(__IO uint32_t*)0x40050640U) /**< (GMAC) Interrupt Mask Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IMRPQ0         (*(__IO uint32_t*)0x40050640U) /**< (GMAC) Interrupt Mask Register Priority Queue  (index = 1) 0 */
#define REG_GMAC_IMRPQ1         (*(__IO uint32_t*)0x40050644U) /**< (GMAC) Interrupt Mask Register Priority Queue  (index = 1) 1 */
#define REG_GMAC_ST2ER          (*(__IO uint32_t*)0x400506E0U) /**< (GMAC) Screening Type 2 Ethertype Register  (index = 0) 0 */
#define REG_GMAC_ST2ER0         (*(__IO uint32_t*)0x400506E0U) /**< (GMAC) Screening Type 2 Ethertype Register  (index = 0) 0 */
#define REG_GMAC_ST2ER1         (*(__IO uint32_t*)0x400506E4U) /**< (GMAC) Screening Type 2 Ethertype Register  (index = 0) 1 */
#define REG_GMAC_ST2ER2         (*(__IO uint32_t*)0x400506E8U) /**< (GMAC) Screening Type 2 Ethertype Register  (index = 0) 2 */
#define REG_GMAC_ST2ER3         (*(__IO uint32_t*)0x400506ECU) /**< (GMAC) Screening Type 2 Ethertype Register  (index = 0) 3 */
#define REG_GMAC_ST2CW00        (*(__IO uint32_t*)0x40050700U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 0) */
#define REG_GMAC_ST2CW10        (*(__IO uint32_t*)0x40050704U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 0) */
#define REG_GMAC_ST2CW01        (*(__IO uint32_t*)0x40050708U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 1) */
#define REG_GMAC_ST2CW11        (*(__IO uint32_t*)0x4005070CU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 1) */
#define REG_GMAC_ST2CW02        (*(__IO uint32_t*)0x40050710U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 2) */
#define REG_GMAC_ST2CW12        (*(__IO uint32_t*)0x40050714U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 2) */
#define REG_GMAC_ST2CW03        (*(__IO uint32_t*)0x40050718U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 3) */
#define REG_GMAC_ST2CW13        (*(__IO uint32_t*)0x4005071CU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 3) */
#define REG_GMAC_ST2CW04        (*(__IO uint32_t*)0x40050720U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 4) */
#define REG_GMAC_ST2CW14        (*(__IO uint32_t*)0x40050724U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 4) */
#define REG_GMAC_ST2CW05        (*(__IO uint32_t*)0x40050728U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 5) */
#define REG_GMAC_ST2CW15        (*(__IO uint32_t*)0x4005072CU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 5) */
#define REG_GMAC_ST2CW06        (*(__IO uint32_t*)0x40050730U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 6) */
#define REG_GMAC_ST2CW16        (*(__IO uint32_t*)0x40050734U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 6) */
#define REG_GMAC_ST2CW07        (*(__IO uint32_t*)0x40050738U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 7) */
#define REG_GMAC_ST2CW17        (*(__IO uint32_t*)0x4005073CU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 7) */
#define REG_GMAC_ST2CW08        (*(__IO uint32_t*)0x40050740U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 8) */
#define REG_GMAC_ST2CW18        (*(__IO uint32_t*)0x40050744U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 8) */
#define REG_GMAC_ST2CW09        (*(__IO uint32_t*)0x40050748U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 9) */
#define REG_GMAC_ST2CW19        (*(__IO uint32_t*)0x4005074CU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 9) */
#define REG_GMAC_ST2CW010       (*(__IO uint32_t*)0x40050750U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 10) */
#define REG_GMAC_ST2CW110       (*(__IO uint32_t*)0x40050754U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 10) */
#define REG_GMAC_ST2CW011       (*(__IO uint32_t*)0x40050758U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 11) */
#define REG_GMAC_ST2CW111       (*(__IO uint32_t*)0x4005075CU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 11) */
#define REG_GMAC_ST2CW012       (*(__IO uint32_t*)0x40050760U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 12) */
#define REG_GMAC_ST2CW112       (*(__IO uint32_t*)0x40050764U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 12) */
#define REG_GMAC_ST2CW013       (*(__IO uint32_t*)0x40050768U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 13) */
#define REG_GMAC_ST2CW113       (*(__IO uint32_t*)0x4005076CU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 13) */
#define REG_GMAC_ST2CW014       (*(__IO uint32_t*)0x40050770U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 14) */
#define REG_GMAC_ST2CW114       (*(__IO uint32_t*)0x40050774U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 14) */
#define REG_GMAC_ST2CW015       (*(__IO uint32_t*)0x40050778U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 15) */
#define REG_GMAC_ST2CW115       (*(__IO uint32_t*)0x4005077CU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 15) */
#define REG_GMAC_ST2CW016       (*(__IO uint32_t*)0x40050780U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 16) */
#define REG_GMAC_ST2CW116       (*(__IO uint32_t*)0x40050784U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 16) */
#define REG_GMAC_ST2CW017       (*(__IO uint32_t*)0x40050788U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 17) */
#define REG_GMAC_ST2CW117       (*(__IO uint32_t*)0x4005078CU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 17) */
#define REG_GMAC_ST2CW018       (*(__IO uint32_t*)0x40050790U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 18) */
#define REG_GMAC_ST2CW118       (*(__IO uint32_t*)0x40050794U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 18) */
#define REG_GMAC_ST2CW019       (*(__IO uint32_t*)0x40050798U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 19) */
#define REG_GMAC_ST2CW119       (*(__IO uint32_t*)0x4005079CU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 19) */
#define REG_GMAC_ST2CW020       (*(__IO uint32_t*)0x400507A0U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 20) */
#define REG_GMAC_ST2CW120       (*(__IO uint32_t*)0x400507A4U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 20) */
#define REG_GMAC_ST2CW021       (*(__IO uint32_t*)0x400507A8U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 21) */
#define REG_GMAC_ST2CW121       (*(__IO uint32_t*)0x400507ACU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 21) */
#define REG_GMAC_ST2CW022       (*(__IO uint32_t*)0x400507B0U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 22) */
#define REG_GMAC_ST2CW122       (*(__IO uint32_t*)0x400507B4U) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 22) */
#define REG_GMAC_ST2CW023       (*(__IO uint32_t*)0x400507B8U) /**< (GMAC) Screening Type 2 Compare Word 0 Register  (index = 23) */
#define REG_GMAC_ST2CW123       (*(__IO uint32_t*)0x400507BCU) /**< (GMAC) Screening Type 2 Compare Word 1 Register  (index = 23) */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for GMAC peripheral ========== */
#define GMAC_INSTANCE_ID                         39        

#endif /* _SAME70_GMAC_INSTANCE_ */
