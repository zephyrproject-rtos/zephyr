/**
 * \file
 *
 * \brief Instance description for GMAC
 *
 * Copyright (c) 2019 Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAME53_GMAC_INSTANCE_
#define _SAME53_GMAC_INSTANCE_

/* ========== Register definition for GMAC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_GMAC_NCR               (0x42000800) /**< \brief (GMAC) Network Control Register */
#define REG_GMAC_NCFGR             (0x42000804) /**< \brief (GMAC) Network Configuration Register */
#define REG_GMAC_NSR               (0x42000808) /**< \brief (GMAC) Network Status Register */
#define REG_GMAC_UR                (0x4200080C) /**< \brief (GMAC) User Register */
#define REG_GMAC_DCFGR             (0x42000810) /**< \brief (GMAC) DMA Configuration Register */
#define REG_GMAC_TSR               (0x42000814) /**< \brief (GMAC) Transmit Status Register */
#define REG_GMAC_RBQB              (0x42000818) /**< \brief (GMAC) Receive Buffer Queue Base Address */
#define REG_GMAC_TBQB              (0x4200081C) /**< \brief (GMAC) Transmit Buffer Queue Base Address */
#define REG_GMAC_RSR               (0x42000820) /**< \brief (GMAC) Receive Status Register */
#define REG_GMAC_ISR               (0x42000824) /**< \brief (GMAC) Interrupt Status Register */
#define REG_GMAC_IER               (0x42000828) /**< \brief (GMAC) Interrupt Enable Register */
#define REG_GMAC_IDR               (0x4200082C) /**< \brief (GMAC) Interrupt Disable Register */
#define REG_GMAC_IMR               (0x42000830) /**< \brief (GMAC) Interrupt Mask Register */
#define REG_GMAC_MAN               (0x42000834) /**< \brief (GMAC) PHY Maintenance Register */
#define REG_GMAC_RPQ               (0x42000838) /**< \brief (GMAC) Received Pause Quantum Register */
#define REG_GMAC_TPQ               (0x4200083C) /**< \brief (GMAC) Transmit Pause Quantum Register */
#define REG_GMAC_TPSF              (0x42000840) /**< \brief (GMAC) TX partial store and forward Register */
#define REG_GMAC_RPSF              (0x42000844) /**< \brief (GMAC) RX partial store and forward Register */
#define REG_GMAC_RJFML             (0x42000848) /**< \brief (GMAC) RX Jumbo Frame Max Length Register */
#define REG_GMAC_HRB               (0x42000880) /**< \brief (GMAC) Hash Register Bottom [31:0] */
#define REG_GMAC_HRT               (0x42000884) /**< \brief (GMAC) Hash Register Top [63:32] */
#define REG_GMAC_SAB0              (0x42000888) /**< \brief (GMAC) Specific Address Bottom [31:0] Register 0 */
#define REG_GMAC_SAT0              (0x4200088C) /**< \brief (GMAC) Specific Address Top [47:32] Register 0 */
#define REG_GMAC_SAB1              (0x42000890) /**< \brief (GMAC) Specific Address Bottom [31:0] Register 1 */
#define REG_GMAC_SAT1              (0x42000894) /**< \brief (GMAC) Specific Address Top [47:32] Register 1 */
#define REG_GMAC_SAB2              (0x42000898) /**< \brief (GMAC) Specific Address Bottom [31:0] Register 2 */
#define REG_GMAC_SAT2              (0x4200089C) /**< \brief (GMAC) Specific Address Top [47:32] Register 2 */
#define REG_GMAC_SAB3              (0x420008A0) /**< \brief (GMAC) Specific Address Bottom [31:0] Register 3 */
#define REG_GMAC_SAT3              (0x420008A4) /**< \brief (GMAC) Specific Address Top [47:32] Register 3 */
#define REG_GMAC_TIDM0             (0x420008A8) /**< \brief (GMAC) Type ID Match Register 0 */
#define REG_GMAC_TIDM1             (0x420008AC) /**< \brief (GMAC) Type ID Match Register 1 */
#define REG_GMAC_TIDM2             (0x420008B0) /**< \brief (GMAC) Type ID Match Register 2 */
#define REG_GMAC_TIDM3             (0x420008B4) /**< \brief (GMAC) Type ID Match Register 3 */
#define REG_GMAC_WOL               (0x420008B8) /**< \brief (GMAC) Wake on LAN */
#define REG_GMAC_IPGS              (0x420008BC) /**< \brief (GMAC) IPG Stretch Register */
#define REG_GMAC_SVLAN             (0x420008C0) /**< \brief (GMAC) Stacked VLAN Register */
#define REG_GMAC_TPFCP             (0x420008C4) /**< \brief (GMAC) Transmit PFC Pause Register */
#define REG_GMAC_SAMB1             (0x420008C8) /**< \brief (GMAC) Specific Address 1 Mask Bottom [31:0] Register */
#define REG_GMAC_SAMT1             (0x420008CC) /**< \brief (GMAC) Specific Address 1 Mask Top [47:32] Register */
#define REG_GMAC_NSC               (0x420008DC) /**< \brief (GMAC) Tsu timer comparison nanoseconds Register */
#define REG_GMAC_SCL               (0x420008E0) /**< \brief (GMAC) Tsu timer second comparison Register */
#define REG_GMAC_SCH               (0x420008E4) /**< \brief (GMAC) Tsu timer second comparison Register */
#define REG_GMAC_EFTSH             (0x420008E8) /**< \brief (GMAC) PTP Event Frame Transmitted Seconds High Register */
#define REG_GMAC_EFRSH             (0x420008EC) /**< \brief (GMAC) PTP Event Frame Received Seconds High Register */
#define REG_GMAC_PEFTSH            (0x420008F0) /**< \brief (GMAC) PTP Peer Event Frame Transmitted Seconds High Register */
#define REG_GMAC_PEFRSH            (0x420008F4) /**< \brief (GMAC) PTP Peer Event Frame Received Seconds High Register */
#define REG_GMAC_OTLO              (0x42000900) /**< \brief (GMAC) Octets Transmitted [31:0] Register */
#define REG_GMAC_OTHI              (0x42000904) /**< \brief (GMAC) Octets Transmitted [47:32] Register */
#define REG_GMAC_FT                (0x42000908) /**< \brief (GMAC) Frames Transmitted Register */
#define REG_GMAC_BCFT              (0x4200090C) /**< \brief (GMAC) Broadcast Frames Transmitted Register */
#define REG_GMAC_MFT               (0x42000910) /**< \brief (GMAC) Multicast Frames Transmitted Register */
#define REG_GMAC_PFT               (0x42000914) /**< \brief (GMAC) Pause Frames Transmitted Register */
#define REG_GMAC_BFT64             (0x42000918) /**< \brief (GMAC) 64 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT127           (0x4200091C) /**< \brief (GMAC) 65 to 127 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT255           (0x42000920) /**< \brief (GMAC) 128 to 255 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT511           (0x42000924) /**< \brief (GMAC) 256 to 511 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT1023          (0x42000928) /**< \brief (GMAC) 512 to 1023 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT1518          (0x4200092C) /**< \brief (GMAC) 1024 to 1518 Byte Frames Transmitted Register */
#define REG_GMAC_GTBFT1518         (0x42000930) /**< \brief (GMAC) Greater Than 1518 Byte Frames Transmitted Register */
#define REG_GMAC_TUR               (0x42000934) /**< \brief (GMAC) Transmit Underruns Register */
#define REG_GMAC_SCF               (0x42000938) /**< \brief (GMAC) Single Collision Frames Register */
#define REG_GMAC_MCF               (0x4200093C) /**< \brief (GMAC) Multiple Collision Frames Register */
#define REG_GMAC_EC                (0x42000940) /**< \brief (GMAC) Excessive Collisions Register */
#define REG_GMAC_LC                (0x42000944) /**< \brief (GMAC) Late Collisions Register */
#define REG_GMAC_DTF               (0x42000948) /**< \brief (GMAC) Deferred Transmission Frames Register */
#define REG_GMAC_CSE               (0x4200094C) /**< \brief (GMAC) Carrier Sense Errors Register */
#define REG_GMAC_ORLO              (0x42000950) /**< \brief (GMAC) Octets Received [31:0] Received */
#define REG_GMAC_ORHI              (0x42000954) /**< \brief (GMAC) Octets Received [47:32] Received */
#define REG_GMAC_FR                (0x42000958) /**< \brief (GMAC) Frames Received Register */
#define REG_GMAC_BCFR              (0x4200095C) /**< \brief (GMAC) Broadcast Frames Received Register */
#define REG_GMAC_MFR               (0x42000960) /**< \brief (GMAC) Multicast Frames Received Register */
#define REG_GMAC_PFR               (0x42000964) /**< \brief (GMAC) Pause Frames Received Register */
#define REG_GMAC_BFR64             (0x42000968) /**< \brief (GMAC) 64 Byte Frames Received Register */
#define REG_GMAC_TBFR127           (0x4200096C) /**< \brief (GMAC) 65 to 127 Byte Frames Received Register */
#define REG_GMAC_TBFR255           (0x42000970) /**< \brief (GMAC) 128 to 255 Byte Frames Received Register */
#define REG_GMAC_TBFR511           (0x42000974) /**< \brief (GMAC) 256 to 511Byte Frames Received Register */
#define REG_GMAC_TBFR1023          (0x42000978) /**< \brief (GMAC) 512 to 1023 Byte Frames Received Register */
#define REG_GMAC_TBFR1518          (0x4200097C) /**< \brief (GMAC) 1024 to 1518 Byte Frames Received Register */
#define REG_GMAC_TMXBFR            (0x42000980) /**< \brief (GMAC) 1519 to Maximum Byte Frames Received Register */
#define REG_GMAC_UFR               (0x42000984) /**< \brief (GMAC) Undersize Frames Received Register */
#define REG_GMAC_OFR               (0x42000988) /**< \brief (GMAC) Oversize Frames Received Register */
#define REG_GMAC_JR                (0x4200098C) /**< \brief (GMAC) Jabbers Received Register */
#define REG_GMAC_FCSE              (0x42000990) /**< \brief (GMAC) Frame Check Sequence Errors Register */
#define REG_GMAC_LFFE              (0x42000994) /**< \brief (GMAC) Length Field Frame Errors Register */
#define REG_GMAC_RSE               (0x42000998) /**< \brief (GMAC) Receive Symbol Errors Register */
#define REG_GMAC_AE                (0x4200099C) /**< \brief (GMAC) Alignment Errors Register */
#define REG_GMAC_RRE               (0x420009A0) /**< \brief (GMAC) Receive Resource Errors Register */
#define REG_GMAC_ROE               (0x420009A4) /**< \brief (GMAC) Receive Overrun Register */
#define REG_GMAC_IHCE              (0x420009A8) /**< \brief (GMAC) IP Header Checksum Errors Register */
#define REG_GMAC_TCE               (0x420009AC) /**< \brief (GMAC) TCP Checksum Errors Register */
#define REG_GMAC_UCE               (0x420009B0) /**< \brief (GMAC) UDP Checksum Errors Register */
#define REG_GMAC_TISUBN            (0x420009BC) /**< \brief (GMAC) 1588 Timer Increment [15:0] Sub-Nanoseconds Register */
#define REG_GMAC_TSH               (0x420009C0) /**< \brief (GMAC) 1588 Timer Seconds High [15:0] Register */
#define REG_GMAC_TSSSL             (0x420009C8) /**< \brief (GMAC) 1588 Timer Sync Strobe Seconds [31:0] Register */
#define REG_GMAC_TSSN              (0x420009CC) /**< \brief (GMAC) 1588 Timer Sync Strobe Nanoseconds Register */
#define REG_GMAC_TSL               (0x420009D0) /**< \brief (GMAC) 1588 Timer Seconds [31:0] Register */
#define REG_GMAC_TN                (0x420009D4) /**< \brief (GMAC) 1588 Timer Nanoseconds Register */
#define REG_GMAC_TA                (0x420009D8) /**< \brief (GMAC) 1588 Timer Adjust Register */
#define REG_GMAC_TI                (0x420009DC) /**< \brief (GMAC) 1588 Timer Increment Register */
#define REG_GMAC_EFTSL             (0x420009E0) /**< \brief (GMAC) PTP Event Frame Transmitted Seconds Low Register */
#define REG_GMAC_EFTN              (0x420009E4) /**< \brief (GMAC) PTP Event Frame Transmitted Nanoseconds */
#define REG_GMAC_EFRSL             (0x420009E8) /**< \brief (GMAC) PTP Event Frame Received Seconds Low Register */
#define REG_GMAC_EFRN              (0x420009EC) /**< \brief (GMAC) PTP Event Frame Received Nanoseconds */
#define REG_GMAC_PEFTSL            (0x420009F0) /**< \brief (GMAC) PTP Peer Event Frame Transmitted Seconds Low Register */
#define REG_GMAC_PEFTN             (0x420009F4) /**< \brief (GMAC) PTP Peer Event Frame Transmitted Nanoseconds */
#define REG_GMAC_PEFRSL            (0x420009F8) /**< \brief (GMAC) PTP Peer Event Frame Received Seconds Low Register */
#define REG_GMAC_PEFRN             (0x420009FC) /**< \brief (GMAC) PTP Peer Event Frame Received Nanoseconds */
#define REG_GMAC_RLPITR            (0x42000A70) /**< \brief (GMAC) Receive LPI transition Register */
#define REG_GMAC_RLPITI            (0x42000A74) /**< \brief (GMAC) Receive LPI Time Register */
#define REG_GMAC_TLPITR            (0x42000A78) /**< \brief (GMAC) Receive LPI transition Register */
#define REG_GMAC_TLPITI            (0x42000A7C) /**< \brief (GMAC) Receive LPI Time Register */
#else
#define REG_GMAC_NCR               (*(RwReg  *)0x42000800UL) /**< \brief (GMAC) Network Control Register */
#define REG_GMAC_NCFGR             (*(RwReg  *)0x42000804UL) /**< \brief (GMAC) Network Configuration Register */
#define REG_GMAC_NSR               (*(RoReg  *)0x42000808UL) /**< \brief (GMAC) Network Status Register */
#define REG_GMAC_UR                (*(RwReg  *)0x4200080CUL) /**< \brief (GMAC) User Register */
#define REG_GMAC_DCFGR             (*(RwReg  *)0x42000810UL) /**< \brief (GMAC) DMA Configuration Register */
#define REG_GMAC_TSR               (*(RwReg  *)0x42000814UL) /**< \brief (GMAC) Transmit Status Register */
#define REG_GMAC_RBQB              (*(RwReg  *)0x42000818UL) /**< \brief (GMAC) Receive Buffer Queue Base Address */
#define REG_GMAC_TBQB              (*(RwReg  *)0x4200081CUL) /**< \brief (GMAC) Transmit Buffer Queue Base Address */
#define REG_GMAC_RSR               (*(RwReg  *)0x42000820UL) /**< \brief (GMAC) Receive Status Register */
#define REG_GMAC_ISR               (*(RwReg  *)0x42000824UL) /**< \brief (GMAC) Interrupt Status Register */
#define REG_GMAC_IER               (*(WoReg  *)0x42000828UL) /**< \brief (GMAC) Interrupt Enable Register */
#define REG_GMAC_IDR               (*(WoReg  *)0x4200082CUL) /**< \brief (GMAC) Interrupt Disable Register */
#define REG_GMAC_IMR               (*(RoReg  *)0x42000830UL) /**< \brief (GMAC) Interrupt Mask Register */
#define REG_GMAC_MAN               (*(RwReg  *)0x42000834UL) /**< \brief (GMAC) PHY Maintenance Register */
#define REG_GMAC_RPQ               (*(RoReg  *)0x42000838UL) /**< \brief (GMAC) Received Pause Quantum Register */
#define REG_GMAC_TPQ               (*(RwReg  *)0x4200083CUL) /**< \brief (GMAC) Transmit Pause Quantum Register */
#define REG_GMAC_TPSF              (*(RwReg  *)0x42000840UL) /**< \brief (GMAC) TX partial store and forward Register */
#define REG_GMAC_RPSF              (*(RwReg  *)0x42000844UL) /**< \brief (GMAC) RX partial store and forward Register */
#define REG_GMAC_RJFML             (*(RwReg  *)0x42000848UL) /**< \brief (GMAC) RX Jumbo Frame Max Length Register */
#define REG_GMAC_HRB               (*(RwReg  *)0x42000880UL) /**< \brief (GMAC) Hash Register Bottom [31:0] */
#define REG_GMAC_HRT               (*(RwReg  *)0x42000884UL) /**< \brief (GMAC) Hash Register Top [63:32] */
#define REG_GMAC_SAB0              (*(RwReg  *)0x42000888UL) /**< \brief (GMAC) Specific Address Bottom [31:0] Register 0 */
#define REG_GMAC_SAT0              (*(RwReg  *)0x4200088CUL) /**< \brief (GMAC) Specific Address Top [47:32] Register 0 */
#define REG_GMAC_SAB1              (*(RwReg  *)0x42000890UL) /**< \brief (GMAC) Specific Address Bottom [31:0] Register 1 */
#define REG_GMAC_SAT1              (*(RwReg  *)0x42000894UL) /**< \brief (GMAC) Specific Address Top [47:32] Register 1 */
#define REG_GMAC_SAB2              (*(RwReg  *)0x42000898UL) /**< \brief (GMAC) Specific Address Bottom [31:0] Register 2 */
#define REG_GMAC_SAT2              (*(RwReg  *)0x4200089CUL) /**< \brief (GMAC) Specific Address Top [47:32] Register 2 */
#define REG_GMAC_SAB3              (*(RwReg  *)0x420008A0UL) /**< \brief (GMAC) Specific Address Bottom [31:0] Register 3 */
#define REG_GMAC_SAT3              (*(RwReg  *)0x420008A4UL) /**< \brief (GMAC) Specific Address Top [47:32] Register 3 */
#define REG_GMAC_TIDM0             (*(RwReg  *)0x420008A8UL) /**< \brief (GMAC) Type ID Match Register 0 */
#define REG_GMAC_TIDM1             (*(RwReg  *)0x420008ACUL) /**< \brief (GMAC) Type ID Match Register 1 */
#define REG_GMAC_TIDM2             (*(RwReg  *)0x420008B0UL) /**< \brief (GMAC) Type ID Match Register 2 */
#define REG_GMAC_TIDM3             (*(RwReg  *)0x420008B4UL) /**< \brief (GMAC) Type ID Match Register 3 */
#define REG_GMAC_WOL               (*(RwReg  *)0x420008B8UL) /**< \brief (GMAC) Wake on LAN */
#define REG_GMAC_IPGS              (*(RwReg  *)0x420008BCUL) /**< \brief (GMAC) IPG Stretch Register */
#define REG_GMAC_SVLAN             (*(RwReg  *)0x420008C0UL) /**< \brief (GMAC) Stacked VLAN Register */
#define REG_GMAC_TPFCP             (*(RwReg  *)0x420008C4UL) /**< \brief (GMAC) Transmit PFC Pause Register */
#define REG_GMAC_SAMB1             (*(RwReg  *)0x420008C8UL) /**< \brief (GMAC) Specific Address 1 Mask Bottom [31:0] Register */
#define REG_GMAC_SAMT1             (*(RwReg  *)0x420008CCUL) /**< \brief (GMAC) Specific Address 1 Mask Top [47:32] Register */
#define REG_GMAC_NSC               (*(RwReg  *)0x420008DCUL) /**< \brief (GMAC) Tsu timer comparison nanoseconds Register */
#define REG_GMAC_SCL               (*(RwReg  *)0x420008E0UL) /**< \brief (GMAC) Tsu timer second comparison Register */
#define REG_GMAC_SCH               (*(RwReg  *)0x420008E4UL) /**< \brief (GMAC) Tsu timer second comparison Register */
#define REG_GMAC_EFTSH             (*(RoReg  *)0x420008E8UL) /**< \brief (GMAC) PTP Event Frame Transmitted Seconds High Register */
#define REG_GMAC_EFRSH             (*(RoReg  *)0x420008ECUL) /**< \brief (GMAC) PTP Event Frame Received Seconds High Register */
#define REG_GMAC_PEFTSH            (*(RoReg  *)0x420008F0UL) /**< \brief (GMAC) PTP Peer Event Frame Transmitted Seconds High Register */
#define REG_GMAC_PEFRSH            (*(RoReg  *)0x420008F4UL) /**< \brief (GMAC) PTP Peer Event Frame Received Seconds High Register */
#define REG_GMAC_OTLO              (*(RoReg  *)0x42000900UL) /**< \brief (GMAC) Octets Transmitted [31:0] Register */
#define REG_GMAC_OTHI              (*(RoReg  *)0x42000904UL) /**< \brief (GMAC) Octets Transmitted [47:32] Register */
#define REG_GMAC_FT                (*(RoReg  *)0x42000908UL) /**< \brief (GMAC) Frames Transmitted Register */
#define REG_GMAC_BCFT              (*(RoReg  *)0x4200090CUL) /**< \brief (GMAC) Broadcast Frames Transmitted Register */
#define REG_GMAC_MFT               (*(RoReg  *)0x42000910UL) /**< \brief (GMAC) Multicast Frames Transmitted Register */
#define REG_GMAC_PFT               (*(RoReg  *)0x42000914UL) /**< \brief (GMAC) Pause Frames Transmitted Register */
#define REG_GMAC_BFT64             (*(RoReg  *)0x42000918UL) /**< \brief (GMAC) 64 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT127           (*(RoReg  *)0x4200091CUL) /**< \brief (GMAC) 65 to 127 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT255           (*(RoReg  *)0x42000920UL) /**< \brief (GMAC) 128 to 255 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT511           (*(RoReg  *)0x42000924UL) /**< \brief (GMAC) 256 to 511 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT1023          (*(RoReg  *)0x42000928UL) /**< \brief (GMAC) 512 to 1023 Byte Frames Transmitted Register */
#define REG_GMAC_TBFT1518          (*(RoReg  *)0x4200092CUL) /**< \brief (GMAC) 1024 to 1518 Byte Frames Transmitted Register */
#define REG_GMAC_GTBFT1518         (*(RoReg  *)0x42000930UL) /**< \brief (GMAC) Greater Than 1518 Byte Frames Transmitted Register */
#define REG_GMAC_TUR               (*(RoReg  *)0x42000934UL) /**< \brief (GMAC) Transmit Underruns Register */
#define REG_GMAC_SCF               (*(RoReg  *)0x42000938UL) /**< \brief (GMAC) Single Collision Frames Register */
#define REG_GMAC_MCF               (*(RoReg  *)0x4200093CUL) /**< \brief (GMAC) Multiple Collision Frames Register */
#define REG_GMAC_EC                (*(RoReg  *)0x42000940UL) /**< \brief (GMAC) Excessive Collisions Register */
#define REG_GMAC_LC                (*(RoReg  *)0x42000944UL) /**< \brief (GMAC) Late Collisions Register */
#define REG_GMAC_DTF               (*(RoReg  *)0x42000948UL) /**< \brief (GMAC) Deferred Transmission Frames Register */
#define REG_GMAC_CSE               (*(RoReg  *)0x4200094CUL) /**< \brief (GMAC) Carrier Sense Errors Register */
#define REG_GMAC_ORLO              (*(RoReg  *)0x42000950UL) /**< \brief (GMAC) Octets Received [31:0] Received */
#define REG_GMAC_ORHI              (*(RoReg  *)0x42000954UL) /**< \brief (GMAC) Octets Received [47:32] Received */
#define REG_GMAC_FR                (*(RoReg  *)0x42000958UL) /**< \brief (GMAC) Frames Received Register */
#define REG_GMAC_BCFR              (*(RoReg  *)0x4200095CUL) /**< \brief (GMAC) Broadcast Frames Received Register */
#define REG_GMAC_MFR               (*(RoReg  *)0x42000960UL) /**< \brief (GMAC) Multicast Frames Received Register */
#define REG_GMAC_PFR               (*(RoReg  *)0x42000964UL) /**< \brief (GMAC) Pause Frames Received Register */
#define REG_GMAC_BFR64             (*(RoReg  *)0x42000968UL) /**< \brief (GMAC) 64 Byte Frames Received Register */
#define REG_GMAC_TBFR127           (*(RoReg  *)0x4200096CUL) /**< \brief (GMAC) 65 to 127 Byte Frames Received Register */
#define REG_GMAC_TBFR255           (*(RoReg  *)0x42000970UL) /**< \brief (GMAC) 128 to 255 Byte Frames Received Register */
#define REG_GMAC_TBFR511           (*(RoReg  *)0x42000974UL) /**< \brief (GMAC) 256 to 511Byte Frames Received Register */
#define REG_GMAC_TBFR1023          (*(RoReg  *)0x42000978UL) /**< \brief (GMAC) 512 to 1023 Byte Frames Received Register */
#define REG_GMAC_TBFR1518          (*(RoReg  *)0x4200097CUL) /**< \brief (GMAC) 1024 to 1518 Byte Frames Received Register */
#define REG_GMAC_TMXBFR            (*(RoReg  *)0x42000980UL) /**< \brief (GMAC) 1519 to Maximum Byte Frames Received Register */
#define REG_GMAC_UFR               (*(RoReg  *)0x42000984UL) /**< \brief (GMAC) Undersize Frames Received Register */
#define REG_GMAC_OFR               (*(RoReg  *)0x42000988UL) /**< \brief (GMAC) Oversize Frames Received Register */
#define REG_GMAC_JR                (*(RoReg  *)0x4200098CUL) /**< \brief (GMAC) Jabbers Received Register */
#define REG_GMAC_FCSE              (*(RoReg  *)0x42000990UL) /**< \brief (GMAC) Frame Check Sequence Errors Register */
#define REG_GMAC_LFFE              (*(RoReg  *)0x42000994UL) /**< \brief (GMAC) Length Field Frame Errors Register */
#define REG_GMAC_RSE               (*(RoReg  *)0x42000998UL) /**< \brief (GMAC) Receive Symbol Errors Register */
#define REG_GMAC_AE                (*(RoReg  *)0x4200099CUL) /**< \brief (GMAC) Alignment Errors Register */
#define REG_GMAC_RRE               (*(RoReg  *)0x420009A0UL) /**< \brief (GMAC) Receive Resource Errors Register */
#define REG_GMAC_ROE               (*(RoReg  *)0x420009A4UL) /**< \brief (GMAC) Receive Overrun Register */
#define REG_GMAC_IHCE              (*(RoReg  *)0x420009A8UL) /**< \brief (GMAC) IP Header Checksum Errors Register */
#define REG_GMAC_TCE               (*(RoReg  *)0x420009ACUL) /**< \brief (GMAC) TCP Checksum Errors Register */
#define REG_GMAC_UCE               (*(RoReg  *)0x420009B0UL) /**< \brief (GMAC) UDP Checksum Errors Register */
#define REG_GMAC_TISUBN            (*(RwReg  *)0x420009BCUL) /**< \brief (GMAC) 1588 Timer Increment [15:0] Sub-Nanoseconds Register */
#define REG_GMAC_TSH               (*(RwReg  *)0x420009C0UL) /**< \brief (GMAC) 1588 Timer Seconds High [15:0] Register */
#define REG_GMAC_TSSSL             (*(RwReg  *)0x420009C8UL) /**< \brief (GMAC) 1588 Timer Sync Strobe Seconds [31:0] Register */
#define REG_GMAC_TSSN              (*(RwReg  *)0x420009CCUL) /**< \brief (GMAC) 1588 Timer Sync Strobe Nanoseconds Register */
#define REG_GMAC_TSL               (*(RwReg  *)0x420009D0UL) /**< \brief (GMAC) 1588 Timer Seconds [31:0] Register */
#define REG_GMAC_TN                (*(RwReg  *)0x420009D4UL) /**< \brief (GMAC) 1588 Timer Nanoseconds Register */
#define REG_GMAC_TA                (*(WoReg  *)0x420009D8UL) /**< \brief (GMAC) 1588 Timer Adjust Register */
#define REG_GMAC_TI                (*(RwReg  *)0x420009DCUL) /**< \brief (GMAC) 1588 Timer Increment Register */
#define REG_GMAC_EFTSL             (*(RoReg  *)0x420009E0UL) /**< \brief (GMAC) PTP Event Frame Transmitted Seconds Low Register */
#define REG_GMAC_EFTN              (*(RoReg  *)0x420009E4UL) /**< \brief (GMAC) PTP Event Frame Transmitted Nanoseconds */
#define REG_GMAC_EFRSL             (*(RoReg  *)0x420009E8UL) /**< \brief (GMAC) PTP Event Frame Received Seconds Low Register */
#define REG_GMAC_EFRN              (*(RoReg  *)0x420009ECUL) /**< \brief (GMAC) PTP Event Frame Received Nanoseconds */
#define REG_GMAC_PEFTSL            (*(RoReg  *)0x420009F0UL) /**< \brief (GMAC) PTP Peer Event Frame Transmitted Seconds Low Register */
#define REG_GMAC_PEFTN             (*(RoReg  *)0x420009F4UL) /**< \brief (GMAC) PTP Peer Event Frame Transmitted Nanoseconds */
#define REG_GMAC_PEFRSL            (*(RoReg  *)0x420009F8UL) /**< \brief (GMAC) PTP Peer Event Frame Received Seconds Low Register */
#define REG_GMAC_PEFRN             (*(RoReg  *)0x420009FCUL) /**< \brief (GMAC) PTP Peer Event Frame Received Nanoseconds */
#define REG_GMAC_RLPITR            (*(RoReg  *)0x42000A70UL) /**< \brief (GMAC) Receive LPI transition Register */
#define REG_GMAC_RLPITI            (*(RoReg  *)0x42000A74UL) /**< \brief (GMAC) Receive LPI Time Register */
#define REG_GMAC_TLPITR            (*(RoReg  *)0x42000A78UL) /**< \brief (GMAC) Receive LPI transition Register */
#define REG_GMAC_TLPITI            (*(RoReg  *)0x42000A7CUL) /**< \brief (GMAC) Receive LPI Time Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* ========== Instance parameters for GMAC peripheral ========== */
#define GMAC_CLK_AHB_ID             14       // Index of AHB clock

#endif /* _SAME53_GMAC_INSTANCE_ */
