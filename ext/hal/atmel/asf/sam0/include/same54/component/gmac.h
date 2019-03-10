/**
 * \file
 *
 * \brief Component description for GMAC
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

#ifndef _SAME54_GMAC_COMPONENT_
#define _SAME54_GMAC_COMPONENT_

/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR GMAC */
/* ========================================================================== */
/** \addtogroup SAME54_GMAC Ethernet MAC */
/*@{*/

#define GMAC_U2005
#define REV_GMAC                    0x100

/* -------- GMAC_NCR : (GMAC Offset: 0x000) (R/W 32) Network Control Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t LBL:1;            /*!< bit:      1  Loop Back Local                    */
    uint32_t RXEN:1;           /*!< bit:      2  Receive Enable                     */
    uint32_t TXEN:1;           /*!< bit:      3  Transmit Enable                    */
    uint32_t MPE:1;            /*!< bit:      4  Management Port Enable             */
    uint32_t CLRSTAT:1;        /*!< bit:      5  Clear Statistics Registers         */
    uint32_t INCSTAT:1;        /*!< bit:      6  Increment Statistics Registers     */
    uint32_t WESTAT:1;         /*!< bit:      7  Write Enable for Statistics Registers */
    uint32_t BP:1;             /*!< bit:      8  Back pressure                      */
    uint32_t TSTART:1;         /*!< bit:      9  Start Transmission                 */
    uint32_t THALT:1;          /*!< bit:     10  Transmit Halt                      */
    uint32_t TXPF:1;           /*!< bit:     11  Transmit Pause Frame               */
    uint32_t TXZQPF:1;         /*!< bit:     12  Transmit Zero Quantum Pause Frame  */
    uint32_t :2;               /*!< bit: 13..14  Reserved                           */
    uint32_t SRTSM:1;          /*!< bit:     15  Store Receive Time Stamp to Memory */
    uint32_t ENPBPR:1;         /*!< bit:     16  Enable PFC Priority-based Pause Reception */
    uint32_t TXPBPF:1;         /*!< bit:     17  Transmit PFC Priority-based Pause Frame */
    uint32_t FNP:1;            /*!< bit:     18  Flush Next Packet                  */
    uint32_t LPI:1;            /*!< bit:     19  Low Power Idle Enable              */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_NCR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_NCR_OFFSET             0x000        /**< \brief (GMAC_NCR offset) Network Control Register */
#define GMAC_NCR_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_NCR reset_value) Network Control Register */

#define GMAC_NCR_LBL_Pos            1            /**< \brief (GMAC_NCR) Loop Back Local */
#define GMAC_NCR_LBL                (_U_(0x1) << GMAC_NCR_LBL_Pos)
#define GMAC_NCR_RXEN_Pos           2            /**< \brief (GMAC_NCR) Receive Enable */
#define GMAC_NCR_RXEN               (_U_(0x1) << GMAC_NCR_RXEN_Pos)
#define GMAC_NCR_TXEN_Pos           3            /**< \brief (GMAC_NCR) Transmit Enable */
#define GMAC_NCR_TXEN               (_U_(0x1) << GMAC_NCR_TXEN_Pos)
#define GMAC_NCR_MPE_Pos            4            /**< \brief (GMAC_NCR) Management Port Enable */
#define GMAC_NCR_MPE                (_U_(0x1) << GMAC_NCR_MPE_Pos)
#define GMAC_NCR_CLRSTAT_Pos        5            /**< \brief (GMAC_NCR) Clear Statistics Registers */
#define GMAC_NCR_CLRSTAT            (_U_(0x1) << GMAC_NCR_CLRSTAT_Pos)
#define GMAC_NCR_INCSTAT_Pos        6            /**< \brief (GMAC_NCR) Increment Statistics Registers */
#define GMAC_NCR_INCSTAT            (_U_(0x1) << GMAC_NCR_INCSTAT_Pos)
#define GMAC_NCR_WESTAT_Pos         7            /**< \brief (GMAC_NCR) Write Enable for Statistics Registers */
#define GMAC_NCR_WESTAT             (_U_(0x1) << GMAC_NCR_WESTAT_Pos)
#define GMAC_NCR_BP_Pos             8            /**< \brief (GMAC_NCR) Back pressure */
#define GMAC_NCR_BP                 (_U_(0x1) << GMAC_NCR_BP_Pos)
#define GMAC_NCR_TSTART_Pos         9            /**< \brief (GMAC_NCR) Start Transmission */
#define GMAC_NCR_TSTART             (_U_(0x1) << GMAC_NCR_TSTART_Pos)
#define GMAC_NCR_THALT_Pos          10           /**< \brief (GMAC_NCR) Transmit Halt */
#define GMAC_NCR_THALT              (_U_(0x1) << GMAC_NCR_THALT_Pos)
#define GMAC_NCR_TXPF_Pos           11           /**< \brief (GMAC_NCR) Transmit Pause Frame */
#define GMAC_NCR_TXPF               (_U_(0x1) << GMAC_NCR_TXPF_Pos)
#define GMAC_NCR_TXZQPF_Pos         12           /**< \brief (GMAC_NCR) Transmit Zero Quantum Pause Frame */
#define GMAC_NCR_TXZQPF             (_U_(0x1) << GMAC_NCR_TXZQPF_Pos)
#define GMAC_NCR_SRTSM_Pos          15           /**< \brief (GMAC_NCR) Store Receive Time Stamp to Memory */
#define GMAC_NCR_SRTSM              (_U_(0x1) << GMAC_NCR_SRTSM_Pos)
#define GMAC_NCR_ENPBPR_Pos         16           /**< \brief (GMAC_NCR) Enable PFC Priority-based Pause Reception */
#define GMAC_NCR_ENPBPR             (_U_(0x1) << GMAC_NCR_ENPBPR_Pos)
#define GMAC_NCR_TXPBPF_Pos         17           /**< \brief (GMAC_NCR) Transmit PFC Priority-based Pause Frame */
#define GMAC_NCR_TXPBPF             (_U_(0x1) << GMAC_NCR_TXPBPF_Pos)
#define GMAC_NCR_FNP_Pos            18           /**< \brief (GMAC_NCR) Flush Next Packet */
#define GMAC_NCR_FNP                (_U_(0x1) << GMAC_NCR_FNP_Pos)
#define GMAC_NCR_LPI_Pos            19           /**< \brief (GMAC_NCR) Low Power Idle Enable */
#define GMAC_NCR_LPI                (_U_(0x1) << GMAC_NCR_LPI_Pos)
#define GMAC_NCR_MASK               _U_(0x000F9FFE) /**< \brief (GMAC_NCR) MASK Register */

/* -------- GMAC_NCFGR : (GMAC Offset: 0x004) (R/W 32) Network Configuration Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SPD:1;            /*!< bit:      0  Speed                              */
    uint32_t FD:1;             /*!< bit:      1  Full Duplex                        */
    uint32_t DNVLAN:1;         /*!< bit:      2  Discard Non-VLAN FRAMES            */
    uint32_t JFRAME:1;         /*!< bit:      3  Jumbo Frame Size                   */
    uint32_t CAF:1;            /*!< bit:      4  Copy All Frames                    */
    uint32_t NBC:1;            /*!< bit:      5  No Broadcast                       */
    uint32_t MTIHEN:1;         /*!< bit:      6  Multicast Hash Enable              */
    uint32_t UNIHEN:1;         /*!< bit:      7  Unicast Hash Enable                */
    uint32_t MAXFS:1;          /*!< bit:      8  1536 Maximum Frame Size            */
    uint32_t :3;               /*!< bit:  9..11  Reserved                           */
    uint32_t RTY:1;            /*!< bit:     12  Retry Test                         */
    uint32_t PEN:1;            /*!< bit:     13  Pause Enable                       */
    uint32_t RXBUFO:2;         /*!< bit: 14..15  Receive Buffer Offset              */
    uint32_t LFERD:1;          /*!< bit:     16  Length Field Error Frame Discard   */
    uint32_t RFCS:1;           /*!< bit:     17  Remove FCS                         */
    uint32_t CLK:3;            /*!< bit: 18..20  MDC CLock Division                 */
    uint32_t DBW:2;            /*!< bit: 21..22  Data Bus Width                     */
    uint32_t DCPF:1;           /*!< bit:     23  Disable Copy of Pause Frames       */
    uint32_t RXCOEN:1;         /*!< bit:     24  Receive Checksum Offload Enable    */
    uint32_t EFRHD:1;          /*!< bit:     25  Enable Frames Received in Half Duplex */
    uint32_t IRXFCS:1;         /*!< bit:     26  Ignore RX FCS                      */
    uint32_t :1;               /*!< bit:     27  Reserved                           */
    uint32_t IPGSEN:1;         /*!< bit:     28  IP Stretch Enable                  */
    uint32_t RXBP:1;           /*!< bit:     29  Receive Bad Preamble               */
    uint32_t IRXER:1;          /*!< bit:     30  Ignore IPG GRXER                   */
    uint32_t :1;               /*!< bit:     31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_NCFGR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_NCFGR_OFFSET           0x004        /**< \brief (GMAC_NCFGR offset) Network Configuration Register */
#define GMAC_NCFGR_RESETVALUE       _U_(0x00080000) /**< \brief (GMAC_NCFGR reset_value) Network Configuration Register */

#define GMAC_NCFGR_SPD_Pos          0            /**< \brief (GMAC_NCFGR) Speed */
#define GMAC_NCFGR_SPD              (_U_(0x1) << GMAC_NCFGR_SPD_Pos)
#define GMAC_NCFGR_FD_Pos           1            /**< \brief (GMAC_NCFGR) Full Duplex */
#define GMAC_NCFGR_FD               (_U_(0x1) << GMAC_NCFGR_FD_Pos)
#define GMAC_NCFGR_DNVLAN_Pos       2            /**< \brief (GMAC_NCFGR) Discard Non-VLAN FRAMES */
#define GMAC_NCFGR_DNVLAN           (_U_(0x1) << GMAC_NCFGR_DNVLAN_Pos)
#define GMAC_NCFGR_JFRAME_Pos       3            /**< \brief (GMAC_NCFGR) Jumbo Frame Size */
#define GMAC_NCFGR_JFRAME           (_U_(0x1) << GMAC_NCFGR_JFRAME_Pos)
#define GMAC_NCFGR_CAF_Pos          4            /**< \brief (GMAC_NCFGR) Copy All Frames */
#define GMAC_NCFGR_CAF              (_U_(0x1) << GMAC_NCFGR_CAF_Pos)
#define GMAC_NCFGR_NBC_Pos          5            /**< \brief (GMAC_NCFGR) No Broadcast */
#define GMAC_NCFGR_NBC              (_U_(0x1) << GMAC_NCFGR_NBC_Pos)
#define GMAC_NCFGR_MTIHEN_Pos       6            /**< \brief (GMAC_NCFGR) Multicast Hash Enable */
#define GMAC_NCFGR_MTIHEN           (_U_(0x1) << GMAC_NCFGR_MTIHEN_Pos)
#define GMAC_NCFGR_UNIHEN_Pos       7            /**< \brief (GMAC_NCFGR) Unicast Hash Enable */
#define GMAC_NCFGR_UNIHEN           (_U_(0x1) << GMAC_NCFGR_UNIHEN_Pos)
#define GMAC_NCFGR_MAXFS_Pos        8            /**< \brief (GMAC_NCFGR) 1536 Maximum Frame Size */
#define GMAC_NCFGR_MAXFS            (_U_(0x1) << GMAC_NCFGR_MAXFS_Pos)
#define GMAC_NCFGR_RTY_Pos          12           /**< \brief (GMAC_NCFGR) Retry Test */
#define GMAC_NCFGR_RTY              (_U_(0x1) << GMAC_NCFGR_RTY_Pos)
#define GMAC_NCFGR_PEN_Pos          13           /**< \brief (GMAC_NCFGR) Pause Enable */
#define GMAC_NCFGR_PEN              (_U_(0x1) << GMAC_NCFGR_PEN_Pos)
#define GMAC_NCFGR_RXBUFO_Pos       14           /**< \brief (GMAC_NCFGR) Receive Buffer Offset */
#define GMAC_NCFGR_RXBUFO_Msk       (_U_(0x3) << GMAC_NCFGR_RXBUFO_Pos)
#define GMAC_NCFGR_RXBUFO(value)    (GMAC_NCFGR_RXBUFO_Msk & ((value) << GMAC_NCFGR_RXBUFO_Pos))
#define GMAC_NCFGR_LFERD_Pos        16           /**< \brief (GMAC_NCFGR) Length Field Error Frame Discard */
#define GMAC_NCFGR_LFERD            (_U_(0x1) << GMAC_NCFGR_LFERD_Pos)
#define GMAC_NCFGR_RFCS_Pos         17           /**< \brief (GMAC_NCFGR) Remove FCS */
#define GMAC_NCFGR_RFCS             (_U_(0x1) << GMAC_NCFGR_RFCS_Pos)
#define GMAC_NCFGR_CLK_Pos          18           /**< \brief (GMAC_NCFGR) MDC CLock Division */
#define GMAC_NCFGR_CLK_Msk          (_U_(0x7) << GMAC_NCFGR_CLK_Pos)
#define GMAC_NCFGR_CLK(value)       (GMAC_NCFGR_CLK_Msk & ((value) << GMAC_NCFGR_CLK_Pos))
#define GMAC_NCFGR_DBW_Pos          21           /**< \brief (GMAC_NCFGR) Data Bus Width */
#define GMAC_NCFGR_DBW_Msk          (_U_(0x3) << GMAC_NCFGR_DBW_Pos)
#define GMAC_NCFGR_DBW(value)       (GMAC_NCFGR_DBW_Msk & ((value) << GMAC_NCFGR_DBW_Pos))
#define GMAC_NCFGR_DCPF_Pos         23           /**< \brief (GMAC_NCFGR) Disable Copy of Pause Frames */
#define GMAC_NCFGR_DCPF             (_U_(0x1) << GMAC_NCFGR_DCPF_Pos)
#define GMAC_NCFGR_RXCOEN_Pos       24           /**< \brief (GMAC_NCFGR) Receive Checksum Offload Enable */
#define GMAC_NCFGR_RXCOEN           (_U_(0x1) << GMAC_NCFGR_RXCOEN_Pos)
#define GMAC_NCFGR_EFRHD_Pos        25           /**< \brief (GMAC_NCFGR) Enable Frames Received in Half Duplex */
#define GMAC_NCFGR_EFRHD            (_U_(0x1) << GMAC_NCFGR_EFRHD_Pos)
#define GMAC_NCFGR_IRXFCS_Pos       26           /**< \brief (GMAC_NCFGR) Ignore RX FCS */
#define GMAC_NCFGR_IRXFCS           (_U_(0x1) << GMAC_NCFGR_IRXFCS_Pos)
#define GMAC_NCFGR_IPGSEN_Pos       28           /**< \brief (GMAC_NCFGR) IP Stretch Enable */
#define GMAC_NCFGR_IPGSEN           (_U_(0x1) << GMAC_NCFGR_IPGSEN_Pos)
#define GMAC_NCFGR_RXBP_Pos         29           /**< \brief (GMAC_NCFGR) Receive Bad Preamble */
#define GMAC_NCFGR_RXBP             (_U_(0x1) << GMAC_NCFGR_RXBP_Pos)
#define GMAC_NCFGR_IRXER_Pos        30           /**< \brief (GMAC_NCFGR) Ignore IPG GRXER */
#define GMAC_NCFGR_IRXER            (_U_(0x1) << GMAC_NCFGR_IRXER_Pos)
#define GMAC_NCFGR_MASK             _U_(0x77FFF1FF) /**< \brief (GMAC_NCFGR) MASK Register */

/* -------- GMAC_NSR : (GMAC Offset: 0x008) (R/  32) Network Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t MDIO:1;           /*!< bit:      1  MDIO Input Status                  */
    uint32_t IDLE:1;           /*!< bit:      2  PHY Management Logic Idle          */
    uint32_t :29;              /*!< bit:  3..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_NSR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_NSR_OFFSET             0x008        /**< \brief (GMAC_NSR offset) Network Status Register */
#define GMAC_NSR_RESETVALUE         _U_(0x00000004) /**< \brief (GMAC_NSR reset_value) Network Status Register */

#define GMAC_NSR_MDIO_Pos           1            /**< \brief (GMAC_NSR) MDIO Input Status */
#define GMAC_NSR_MDIO               (_U_(0x1) << GMAC_NSR_MDIO_Pos)
#define GMAC_NSR_IDLE_Pos           2            /**< \brief (GMAC_NSR) PHY Management Logic Idle */
#define GMAC_NSR_IDLE               (_U_(0x1) << GMAC_NSR_IDLE_Pos)
#define GMAC_NSR_MASK               _U_(0x00000006) /**< \brief (GMAC_NSR) MASK Register */

/* -------- GMAC_UR : (GMAC Offset: 0x00C) (R/W 32) User Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MII:1;            /*!< bit:      0  MII Mode                           */
    uint32_t :31;              /*!< bit:  1..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_UR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_UR_OFFSET              0x00C        /**< \brief (GMAC_UR offset) User Register */
#define GMAC_UR_RESETVALUE          _U_(0x00000000) /**< \brief (GMAC_UR reset_value) User Register */

#define GMAC_UR_MII_Pos             0            /**< \brief (GMAC_UR) MII Mode */
#define GMAC_UR_MII                 (_U_(0x1) << GMAC_UR_MII_Pos)
#define GMAC_UR_MASK                _U_(0x00000001) /**< \brief (GMAC_UR) MASK Register */

/* -------- GMAC_DCFGR : (GMAC Offset: 0x010) (R/W 32) DMA Configuration Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FBLDO:5;          /*!< bit:  0.. 4  Fixed Burst Length for DMA Data Operations: */
    uint32_t :1;               /*!< bit:      5  Reserved                           */
    uint32_t ESMA:1;           /*!< bit:      6  Endian Swap Mode Enable for Management Descriptor Accesses */
    uint32_t ESPA:1;           /*!< bit:      7  Endian Swap Mode Enable for Packet Data Accesses */
    uint32_t RXBMS:2;          /*!< bit:  8.. 9  Receiver Packet Buffer Memory Size Select */
    uint32_t TXPBMS:1;         /*!< bit:     10  Transmitter Packet Buffer Memory Size Select */
    uint32_t TXCOEN:1;         /*!< bit:     11  Transmitter Checksum Generation Offload Enable */
    uint32_t :4;               /*!< bit: 12..15  Reserved                           */
    uint32_t DRBS:8;           /*!< bit: 16..23  DMA Receive Buffer Size            */
    uint32_t DDRP:1;           /*!< bit:     24  DMA Discard Receive Packets        */
    uint32_t :7;               /*!< bit: 25..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_DCFGR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_DCFGR_OFFSET           0x010        /**< \brief (GMAC_DCFGR offset) DMA Configuration Register */
#define GMAC_DCFGR_RESETVALUE       _U_(0x00020704) /**< \brief (GMAC_DCFGR reset_value) DMA Configuration Register */

#define GMAC_DCFGR_FBLDO_Pos        0            /**< \brief (GMAC_DCFGR) Fixed Burst Length for DMA Data Operations: */
#define GMAC_DCFGR_FBLDO_Msk        (_U_(0x1F) << GMAC_DCFGR_FBLDO_Pos)
#define GMAC_DCFGR_FBLDO(value)     (GMAC_DCFGR_FBLDO_Msk & ((value) << GMAC_DCFGR_FBLDO_Pos))
#define GMAC_DCFGR_ESMA_Pos         6            /**< \brief (GMAC_DCFGR) Endian Swap Mode Enable for Management Descriptor Accesses */
#define GMAC_DCFGR_ESMA             (_U_(0x1) << GMAC_DCFGR_ESMA_Pos)
#define GMAC_DCFGR_ESPA_Pos         7            /**< \brief (GMAC_DCFGR) Endian Swap Mode Enable for Packet Data Accesses */
#define GMAC_DCFGR_ESPA             (_U_(0x1) << GMAC_DCFGR_ESPA_Pos)
#define GMAC_DCFGR_RXBMS_Pos        8            /**< \brief (GMAC_DCFGR) Receiver Packet Buffer Memory Size Select */
#define GMAC_DCFGR_RXBMS_Msk        (_U_(0x3) << GMAC_DCFGR_RXBMS_Pos)
#define GMAC_DCFGR_RXBMS(value)     (GMAC_DCFGR_RXBMS_Msk & ((value) << GMAC_DCFGR_RXBMS_Pos))
#define GMAC_DCFGR_TXPBMS_Pos       10           /**< \brief (GMAC_DCFGR) Transmitter Packet Buffer Memory Size Select */
#define GMAC_DCFGR_TXPBMS           (_U_(0x1) << GMAC_DCFGR_TXPBMS_Pos)
#define GMAC_DCFGR_TXCOEN_Pos       11           /**< \brief (GMAC_DCFGR) Transmitter Checksum Generation Offload Enable */
#define GMAC_DCFGR_TXCOEN           (_U_(0x1) << GMAC_DCFGR_TXCOEN_Pos)
#define GMAC_DCFGR_DRBS_Pos         16           /**< \brief (GMAC_DCFGR) DMA Receive Buffer Size */
#define GMAC_DCFGR_DRBS_Msk         (_U_(0xFF) << GMAC_DCFGR_DRBS_Pos)
#define GMAC_DCFGR_DRBS(value)      (GMAC_DCFGR_DRBS_Msk & ((value) << GMAC_DCFGR_DRBS_Pos))
#define GMAC_DCFGR_DDRP_Pos         24           /**< \brief (GMAC_DCFGR) DMA Discard Receive Packets */
#define GMAC_DCFGR_DDRP             (_U_(0x1) << GMAC_DCFGR_DDRP_Pos)
#define GMAC_DCFGR_MASK             _U_(0x01FF0FDF) /**< \brief (GMAC_DCFGR) MASK Register */

/* -------- GMAC_TSR : (GMAC Offset: 0x014) (R/W 32) Transmit Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t UBR:1;            /*!< bit:      0  Used Bit Read                      */
    uint32_t COL:1;            /*!< bit:      1  Collision Occurred                 */
    uint32_t RLE:1;            /*!< bit:      2  Retry Limit Exceeded               */
    uint32_t TXGO:1;           /*!< bit:      3  Transmit Go                        */
    uint32_t TFC:1;            /*!< bit:      4  Transmit Frame Corruption Due to AHB Error */
    uint32_t TXCOMP:1;         /*!< bit:      5  Transmit Complete                  */
    uint32_t UND:1;            /*!< bit:      6  Transmit Underrun                  */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t HRESP:1;          /*!< bit:      8  HRESP Not OK                       */
    uint32_t :23;              /*!< bit:  9..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TSR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TSR_OFFSET             0x014        /**< \brief (GMAC_TSR offset) Transmit Status Register */
#define GMAC_TSR_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_TSR reset_value) Transmit Status Register */

#define GMAC_TSR_UBR_Pos            0            /**< \brief (GMAC_TSR) Used Bit Read */
#define GMAC_TSR_UBR                (_U_(0x1) << GMAC_TSR_UBR_Pos)
#define GMAC_TSR_COL_Pos            1            /**< \brief (GMAC_TSR) Collision Occurred */
#define GMAC_TSR_COL                (_U_(0x1) << GMAC_TSR_COL_Pos)
#define GMAC_TSR_RLE_Pos            2            /**< \brief (GMAC_TSR) Retry Limit Exceeded */
#define GMAC_TSR_RLE                (_U_(0x1) << GMAC_TSR_RLE_Pos)
#define GMAC_TSR_TXGO_Pos           3            /**< \brief (GMAC_TSR) Transmit Go */
#define GMAC_TSR_TXGO               (_U_(0x1) << GMAC_TSR_TXGO_Pos)
#define GMAC_TSR_TFC_Pos            4            /**< \brief (GMAC_TSR) Transmit Frame Corruption Due to AHB Error */
#define GMAC_TSR_TFC                (_U_(0x1) << GMAC_TSR_TFC_Pos)
#define GMAC_TSR_TXCOMP_Pos         5            /**< \brief (GMAC_TSR) Transmit Complete */
#define GMAC_TSR_TXCOMP             (_U_(0x1) << GMAC_TSR_TXCOMP_Pos)
#define GMAC_TSR_UND_Pos            6            /**< \brief (GMAC_TSR) Transmit Underrun */
#define GMAC_TSR_UND                (_U_(0x1) << GMAC_TSR_UND_Pos)
#define GMAC_TSR_HRESP_Pos          8            /**< \brief (GMAC_TSR) HRESP Not OK */
#define GMAC_TSR_HRESP              (_U_(0x1) << GMAC_TSR_HRESP_Pos)
#define GMAC_TSR_MASK               _U_(0x0000017F) /**< \brief (GMAC_TSR) MASK Register */

/* -------- GMAC_RBQB : (GMAC Offset: 0x018) (R/W 32) Receive Buffer Queue Base Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint32_t ADDR:30;          /*!< bit:  2..31  Receive Buffer Queue Base Address  */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RBQB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RBQB_OFFSET            0x018        /**< \brief (GMAC_RBQB offset) Receive Buffer Queue Base Address */
#define GMAC_RBQB_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_RBQB reset_value) Receive Buffer Queue Base Address */

#define GMAC_RBQB_ADDR_Pos          2            /**< \brief (GMAC_RBQB) Receive Buffer Queue Base Address */
#define GMAC_RBQB_ADDR_Msk          (_U_(0x3FFFFFFF) << GMAC_RBQB_ADDR_Pos)
#define GMAC_RBQB_ADDR(value)       (GMAC_RBQB_ADDR_Msk & ((value) << GMAC_RBQB_ADDR_Pos))
#define GMAC_RBQB_MASK              _U_(0xFFFFFFFC) /**< \brief (GMAC_RBQB) MASK Register */

/* -------- GMAC_TBQB : (GMAC Offset: 0x01C) (R/W 32) Transmit Buffer Queue Base Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint32_t ADDR:30;          /*!< bit:  2..31  Transmit Buffer Queue Base Address */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBQB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBQB_OFFSET            0x01C        /**< \brief (GMAC_TBQB offset) Transmit Buffer Queue Base Address */
#define GMAC_TBQB_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_TBQB reset_value) Transmit Buffer Queue Base Address */

#define GMAC_TBQB_ADDR_Pos          2            /**< \brief (GMAC_TBQB) Transmit Buffer Queue Base Address */
#define GMAC_TBQB_ADDR_Msk          (_U_(0x3FFFFFFF) << GMAC_TBQB_ADDR_Pos)
#define GMAC_TBQB_ADDR(value)       (GMAC_TBQB_ADDR_Msk & ((value) << GMAC_TBQB_ADDR_Pos))
#define GMAC_TBQB_MASK              _U_(0xFFFFFFFC) /**< \brief (GMAC_TBQB) MASK Register */

/* -------- GMAC_RSR : (GMAC Offset: 0x020) (R/W 32) Receive Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BNA:1;            /*!< bit:      0  Buffer Not Available               */
    uint32_t REC:1;            /*!< bit:      1  Frame Received                     */
    uint32_t RXOVR:1;          /*!< bit:      2  Receive Overrun                    */
    uint32_t HNO:1;            /*!< bit:      3  HRESP Not OK                       */
    uint32_t :28;              /*!< bit:  4..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RSR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RSR_OFFSET             0x020        /**< \brief (GMAC_RSR offset) Receive Status Register */
#define GMAC_RSR_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_RSR reset_value) Receive Status Register */

#define GMAC_RSR_BNA_Pos            0            /**< \brief (GMAC_RSR) Buffer Not Available */
#define GMAC_RSR_BNA                (_U_(0x1) << GMAC_RSR_BNA_Pos)
#define GMAC_RSR_REC_Pos            1            /**< \brief (GMAC_RSR) Frame Received */
#define GMAC_RSR_REC                (_U_(0x1) << GMAC_RSR_REC_Pos)
#define GMAC_RSR_RXOVR_Pos          2            /**< \brief (GMAC_RSR) Receive Overrun */
#define GMAC_RSR_RXOVR              (_U_(0x1) << GMAC_RSR_RXOVR_Pos)
#define GMAC_RSR_HNO_Pos            3            /**< \brief (GMAC_RSR) HRESP Not OK */
#define GMAC_RSR_HNO                (_U_(0x1) << GMAC_RSR_HNO_Pos)
#define GMAC_RSR_MASK               _U_(0x0000000F) /**< \brief (GMAC_RSR) MASK Register */

/* -------- GMAC_ISR : (GMAC Offset: 0x024) (R/W 32) Interrupt Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MFS:1;            /*!< bit:      0  Management Frame Sent              */
    uint32_t RCOMP:1;          /*!< bit:      1  Receive Complete                   */
    uint32_t RXUBR:1;          /*!< bit:      2  RX Used Bit Read                   */
    uint32_t TXUBR:1;          /*!< bit:      3  TX Used Bit Read                   */
    uint32_t TUR:1;            /*!< bit:      4  Transmit Underrun                  */
    uint32_t RLEX:1;           /*!< bit:      5  Retry Limit Exceeded               */
    uint32_t TFC:1;            /*!< bit:      6  Transmit Frame Corruption Due to AHB Error */
    uint32_t TCOMP:1;          /*!< bit:      7  Transmit Complete                  */
    uint32_t :2;               /*!< bit:  8.. 9  Reserved                           */
    uint32_t ROVR:1;           /*!< bit:     10  Receive Overrun                    */
    uint32_t HRESP:1;          /*!< bit:     11  HRESP Not OK                       */
    uint32_t PFNZ:1;           /*!< bit:     12  Pause Frame with Non-zero Pause Quantum Received */
    uint32_t PTZ:1;            /*!< bit:     13  Pause Time Zero                    */
    uint32_t PFTR:1;           /*!< bit:     14  Pause Frame Transmitted            */
    uint32_t :3;               /*!< bit: 15..17  Reserved                           */
    uint32_t DRQFR:1;          /*!< bit:     18  PTP Delay Request Frame Received   */
    uint32_t SFR:1;            /*!< bit:     19  PTP Sync Frame Received            */
    uint32_t DRQFT:1;          /*!< bit:     20  PTP Delay Request Frame Transmitted */
    uint32_t SFT:1;            /*!< bit:     21  PTP Sync Frame Transmitted         */
    uint32_t PDRQFR:1;         /*!< bit:     22  PDelay Request Frame Received      */
    uint32_t PDRSFR:1;         /*!< bit:     23  PDelay Response Frame Received     */
    uint32_t PDRQFT:1;         /*!< bit:     24  PDelay Request Frame Transmitted   */
    uint32_t PDRSFT:1;         /*!< bit:     25  PDelay Response Frame Transmitted  */
    uint32_t SRI:1;            /*!< bit:     26  TSU Seconds Register Increment     */
    uint32_t :1;               /*!< bit:     27  Reserved                           */
    uint32_t WOL:1;            /*!< bit:     28  Wake On LAN                        */
    uint32_t TSUCMP:1;         /*!< bit:     29  Tsu timer comparison               */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_ISR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ISR_OFFSET             0x024        /**< \brief (GMAC_ISR offset) Interrupt Status Register */
#define GMAC_ISR_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_ISR reset_value) Interrupt Status Register */

#define GMAC_ISR_MFS_Pos            0            /**< \brief (GMAC_ISR) Management Frame Sent */
#define GMAC_ISR_MFS                (_U_(0x1) << GMAC_ISR_MFS_Pos)
#define GMAC_ISR_RCOMP_Pos          1            /**< \brief (GMAC_ISR) Receive Complete */
#define GMAC_ISR_RCOMP              (_U_(0x1) << GMAC_ISR_RCOMP_Pos)
#define GMAC_ISR_RXUBR_Pos          2            /**< \brief (GMAC_ISR) RX Used Bit Read */
#define GMAC_ISR_RXUBR              (_U_(0x1) << GMAC_ISR_RXUBR_Pos)
#define GMAC_ISR_TXUBR_Pos          3            /**< \brief (GMAC_ISR) TX Used Bit Read */
#define GMAC_ISR_TXUBR              (_U_(0x1) << GMAC_ISR_TXUBR_Pos)
#define GMAC_ISR_TUR_Pos            4            /**< \brief (GMAC_ISR) Transmit Underrun */
#define GMAC_ISR_TUR                (_U_(0x1) << GMAC_ISR_TUR_Pos)
#define GMAC_ISR_RLEX_Pos           5            /**< \brief (GMAC_ISR) Retry Limit Exceeded */
#define GMAC_ISR_RLEX               (_U_(0x1) << GMAC_ISR_RLEX_Pos)
#define GMAC_ISR_TFC_Pos            6            /**< \brief (GMAC_ISR) Transmit Frame Corruption Due to AHB Error */
#define GMAC_ISR_TFC                (_U_(0x1) << GMAC_ISR_TFC_Pos)
#define GMAC_ISR_TCOMP_Pos          7            /**< \brief (GMAC_ISR) Transmit Complete */
#define GMAC_ISR_TCOMP              (_U_(0x1) << GMAC_ISR_TCOMP_Pos)
#define GMAC_ISR_ROVR_Pos           10           /**< \brief (GMAC_ISR) Receive Overrun */
#define GMAC_ISR_ROVR               (_U_(0x1) << GMAC_ISR_ROVR_Pos)
#define GMAC_ISR_HRESP_Pos          11           /**< \brief (GMAC_ISR) HRESP Not OK */
#define GMAC_ISR_HRESP              (_U_(0x1) << GMAC_ISR_HRESP_Pos)
#define GMAC_ISR_PFNZ_Pos           12           /**< \brief (GMAC_ISR) Pause Frame with Non-zero Pause Quantum Received */
#define GMAC_ISR_PFNZ               (_U_(0x1) << GMAC_ISR_PFNZ_Pos)
#define GMAC_ISR_PTZ_Pos            13           /**< \brief (GMAC_ISR) Pause Time Zero */
#define GMAC_ISR_PTZ                (_U_(0x1) << GMAC_ISR_PTZ_Pos)
#define GMAC_ISR_PFTR_Pos           14           /**< \brief (GMAC_ISR) Pause Frame Transmitted */
#define GMAC_ISR_PFTR               (_U_(0x1) << GMAC_ISR_PFTR_Pos)
#define GMAC_ISR_DRQFR_Pos          18           /**< \brief (GMAC_ISR) PTP Delay Request Frame Received */
#define GMAC_ISR_DRQFR              (_U_(0x1) << GMAC_ISR_DRQFR_Pos)
#define GMAC_ISR_SFR_Pos            19           /**< \brief (GMAC_ISR) PTP Sync Frame Received */
#define GMAC_ISR_SFR                (_U_(0x1) << GMAC_ISR_SFR_Pos)
#define GMAC_ISR_DRQFT_Pos          20           /**< \brief (GMAC_ISR) PTP Delay Request Frame Transmitted */
#define GMAC_ISR_DRQFT              (_U_(0x1) << GMAC_ISR_DRQFT_Pos)
#define GMAC_ISR_SFT_Pos            21           /**< \brief (GMAC_ISR) PTP Sync Frame Transmitted */
#define GMAC_ISR_SFT                (_U_(0x1) << GMAC_ISR_SFT_Pos)
#define GMAC_ISR_PDRQFR_Pos         22           /**< \brief (GMAC_ISR) PDelay Request Frame Received */
#define GMAC_ISR_PDRQFR             (_U_(0x1) << GMAC_ISR_PDRQFR_Pos)
#define GMAC_ISR_PDRSFR_Pos         23           /**< \brief (GMAC_ISR) PDelay Response Frame Received */
#define GMAC_ISR_PDRSFR             (_U_(0x1) << GMAC_ISR_PDRSFR_Pos)
#define GMAC_ISR_PDRQFT_Pos         24           /**< \brief (GMAC_ISR) PDelay Request Frame Transmitted */
#define GMAC_ISR_PDRQFT             (_U_(0x1) << GMAC_ISR_PDRQFT_Pos)
#define GMAC_ISR_PDRSFT_Pos         25           /**< \brief (GMAC_ISR) PDelay Response Frame Transmitted */
#define GMAC_ISR_PDRSFT             (_U_(0x1) << GMAC_ISR_PDRSFT_Pos)
#define GMAC_ISR_SRI_Pos            26           /**< \brief (GMAC_ISR) TSU Seconds Register Increment */
#define GMAC_ISR_SRI                (_U_(0x1) << GMAC_ISR_SRI_Pos)
#define GMAC_ISR_WOL_Pos            28           /**< \brief (GMAC_ISR) Wake On LAN */
#define GMAC_ISR_WOL                (_U_(0x1) << GMAC_ISR_WOL_Pos)
#define GMAC_ISR_TSUCMP_Pos         29           /**< \brief (GMAC_ISR) Tsu timer comparison */
#define GMAC_ISR_TSUCMP             (_U_(0x1) << GMAC_ISR_TSUCMP_Pos)
#define GMAC_ISR_MASK               _U_(0x37FC7CFF) /**< \brief (GMAC_ISR) MASK Register */

/* -------- GMAC_IER : (GMAC Offset: 0x028) ( /W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MFS:1;            /*!< bit:      0  Management Frame Sent              */
    uint32_t RCOMP:1;          /*!< bit:      1  Receive Complete                   */
    uint32_t RXUBR:1;          /*!< bit:      2  RX Used Bit Read                   */
    uint32_t TXUBR:1;          /*!< bit:      3  TX Used Bit Read                   */
    uint32_t TUR:1;            /*!< bit:      4  Transmit Underrun                  */
    uint32_t RLEX:1;           /*!< bit:      5  Retry Limit Exceeded or Late Collision */
    uint32_t TFC:1;            /*!< bit:      6  Transmit Frame Corruption Due to AHB Error */
    uint32_t TCOMP:1;          /*!< bit:      7  Transmit Complete                  */
    uint32_t :2;               /*!< bit:  8.. 9  Reserved                           */
    uint32_t ROVR:1;           /*!< bit:     10  Receive Overrun                    */
    uint32_t HRESP:1;          /*!< bit:     11  HRESP Not OK                       */
    uint32_t PFNZ:1;           /*!< bit:     12  Pause Frame with Non-zero Pause Quantum Received */
    uint32_t PTZ:1;            /*!< bit:     13  Pause Time Zero                    */
    uint32_t PFTR:1;           /*!< bit:     14  Pause Frame Transmitted            */
    uint32_t EXINT:1;          /*!< bit:     15  External Interrupt                 */
    uint32_t :2;               /*!< bit: 16..17  Reserved                           */
    uint32_t DRQFR:1;          /*!< bit:     18  PTP Delay Request Frame Received   */
    uint32_t SFR:1;            /*!< bit:     19  PTP Sync Frame Received            */
    uint32_t DRQFT:1;          /*!< bit:     20  PTP Delay Request Frame Transmitted */
    uint32_t SFT:1;            /*!< bit:     21  PTP Sync Frame Transmitted         */
    uint32_t PDRQFR:1;         /*!< bit:     22  PDelay Request Frame Received      */
    uint32_t PDRSFR:1;         /*!< bit:     23  PDelay Response Frame Received     */
    uint32_t PDRQFT:1;         /*!< bit:     24  PDelay Request Frame Transmitted   */
    uint32_t PDRSFT:1;         /*!< bit:     25  PDelay Response Frame Transmitted  */
    uint32_t SRI:1;            /*!< bit:     26  TSU Seconds Register Increment     */
    uint32_t :1;               /*!< bit:     27  Reserved                           */
    uint32_t WOL:1;            /*!< bit:     28  Wake On LAN                        */
    uint32_t TSUCMP:1;         /*!< bit:     29  Tsu timer comparison               */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_IER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IER_OFFSET             0x028        /**< \brief (GMAC_IER offset) Interrupt Enable Register */

#define GMAC_IER_MFS_Pos            0            /**< \brief (GMAC_IER) Management Frame Sent */
#define GMAC_IER_MFS                (_U_(0x1) << GMAC_IER_MFS_Pos)
#define GMAC_IER_RCOMP_Pos          1            /**< \brief (GMAC_IER) Receive Complete */
#define GMAC_IER_RCOMP              (_U_(0x1) << GMAC_IER_RCOMP_Pos)
#define GMAC_IER_RXUBR_Pos          2            /**< \brief (GMAC_IER) RX Used Bit Read */
#define GMAC_IER_RXUBR              (_U_(0x1) << GMAC_IER_RXUBR_Pos)
#define GMAC_IER_TXUBR_Pos          3            /**< \brief (GMAC_IER) TX Used Bit Read */
#define GMAC_IER_TXUBR              (_U_(0x1) << GMAC_IER_TXUBR_Pos)
#define GMAC_IER_TUR_Pos            4            /**< \brief (GMAC_IER) Transmit Underrun */
#define GMAC_IER_TUR                (_U_(0x1) << GMAC_IER_TUR_Pos)
#define GMAC_IER_RLEX_Pos           5            /**< \brief (GMAC_IER) Retry Limit Exceeded or Late Collision */
#define GMAC_IER_RLEX               (_U_(0x1) << GMAC_IER_RLEX_Pos)
#define GMAC_IER_TFC_Pos            6            /**< \brief (GMAC_IER) Transmit Frame Corruption Due to AHB Error */
#define GMAC_IER_TFC                (_U_(0x1) << GMAC_IER_TFC_Pos)
#define GMAC_IER_TCOMP_Pos          7            /**< \brief (GMAC_IER) Transmit Complete */
#define GMAC_IER_TCOMP              (_U_(0x1) << GMAC_IER_TCOMP_Pos)
#define GMAC_IER_ROVR_Pos           10           /**< \brief (GMAC_IER) Receive Overrun */
#define GMAC_IER_ROVR               (_U_(0x1) << GMAC_IER_ROVR_Pos)
#define GMAC_IER_HRESP_Pos          11           /**< \brief (GMAC_IER) HRESP Not OK */
#define GMAC_IER_HRESP              (_U_(0x1) << GMAC_IER_HRESP_Pos)
#define GMAC_IER_PFNZ_Pos           12           /**< \brief (GMAC_IER) Pause Frame with Non-zero Pause Quantum Received */
#define GMAC_IER_PFNZ               (_U_(0x1) << GMAC_IER_PFNZ_Pos)
#define GMAC_IER_PTZ_Pos            13           /**< \brief (GMAC_IER) Pause Time Zero */
#define GMAC_IER_PTZ                (_U_(0x1) << GMAC_IER_PTZ_Pos)
#define GMAC_IER_PFTR_Pos           14           /**< \brief (GMAC_IER) Pause Frame Transmitted */
#define GMAC_IER_PFTR               (_U_(0x1) << GMAC_IER_PFTR_Pos)
#define GMAC_IER_EXINT_Pos          15           /**< \brief (GMAC_IER) External Interrupt */
#define GMAC_IER_EXINT              (_U_(0x1) << GMAC_IER_EXINT_Pos)
#define GMAC_IER_DRQFR_Pos          18           /**< \brief (GMAC_IER) PTP Delay Request Frame Received */
#define GMAC_IER_DRQFR              (_U_(0x1) << GMAC_IER_DRQFR_Pos)
#define GMAC_IER_SFR_Pos            19           /**< \brief (GMAC_IER) PTP Sync Frame Received */
#define GMAC_IER_SFR                (_U_(0x1) << GMAC_IER_SFR_Pos)
#define GMAC_IER_DRQFT_Pos          20           /**< \brief (GMAC_IER) PTP Delay Request Frame Transmitted */
#define GMAC_IER_DRQFT              (_U_(0x1) << GMAC_IER_DRQFT_Pos)
#define GMAC_IER_SFT_Pos            21           /**< \brief (GMAC_IER) PTP Sync Frame Transmitted */
#define GMAC_IER_SFT                (_U_(0x1) << GMAC_IER_SFT_Pos)
#define GMAC_IER_PDRQFR_Pos         22           /**< \brief (GMAC_IER) PDelay Request Frame Received */
#define GMAC_IER_PDRQFR             (_U_(0x1) << GMAC_IER_PDRQFR_Pos)
#define GMAC_IER_PDRSFR_Pos         23           /**< \brief (GMAC_IER) PDelay Response Frame Received */
#define GMAC_IER_PDRSFR             (_U_(0x1) << GMAC_IER_PDRSFR_Pos)
#define GMAC_IER_PDRQFT_Pos         24           /**< \brief (GMAC_IER) PDelay Request Frame Transmitted */
#define GMAC_IER_PDRQFT             (_U_(0x1) << GMAC_IER_PDRQFT_Pos)
#define GMAC_IER_PDRSFT_Pos         25           /**< \brief (GMAC_IER) PDelay Response Frame Transmitted */
#define GMAC_IER_PDRSFT             (_U_(0x1) << GMAC_IER_PDRSFT_Pos)
#define GMAC_IER_SRI_Pos            26           /**< \brief (GMAC_IER) TSU Seconds Register Increment */
#define GMAC_IER_SRI                (_U_(0x1) << GMAC_IER_SRI_Pos)
#define GMAC_IER_WOL_Pos            28           /**< \brief (GMAC_IER) Wake On LAN */
#define GMAC_IER_WOL                (_U_(0x1) << GMAC_IER_WOL_Pos)
#define GMAC_IER_TSUCMP_Pos         29           /**< \brief (GMAC_IER) Tsu timer comparison */
#define GMAC_IER_TSUCMP             (_U_(0x1) << GMAC_IER_TSUCMP_Pos)
#define GMAC_IER_MASK               _U_(0x37FCFCFF) /**< \brief (GMAC_IER) MASK Register */

/* -------- GMAC_IDR : (GMAC Offset: 0x02C) ( /W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MFS:1;            /*!< bit:      0  Management Frame Sent              */
    uint32_t RCOMP:1;          /*!< bit:      1  Receive Complete                   */
    uint32_t RXUBR:1;          /*!< bit:      2  RX Used Bit Read                   */
    uint32_t TXUBR:1;          /*!< bit:      3  TX Used Bit Read                   */
    uint32_t TUR:1;            /*!< bit:      4  Transmit Underrun                  */
    uint32_t RLEX:1;           /*!< bit:      5  Retry Limit Exceeded or Late Collision */
    uint32_t TFC:1;            /*!< bit:      6  Transmit Frame Corruption Due to AHB Error */
    uint32_t TCOMP:1;          /*!< bit:      7  Transmit Complete                  */
    uint32_t :2;               /*!< bit:  8.. 9  Reserved                           */
    uint32_t ROVR:1;           /*!< bit:     10  Receive Overrun                    */
    uint32_t HRESP:1;          /*!< bit:     11  HRESP Not OK                       */
    uint32_t PFNZ:1;           /*!< bit:     12  Pause Frame with Non-zero Pause Quantum Received */
    uint32_t PTZ:1;            /*!< bit:     13  Pause Time Zero                    */
    uint32_t PFTR:1;           /*!< bit:     14  Pause Frame Transmitted            */
    uint32_t EXINT:1;          /*!< bit:     15  External Interrupt                 */
    uint32_t :2;               /*!< bit: 16..17  Reserved                           */
    uint32_t DRQFR:1;          /*!< bit:     18  PTP Delay Request Frame Received   */
    uint32_t SFR:1;            /*!< bit:     19  PTP Sync Frame Received            */
    uint32_t DRQFT:1;          /*!< bit:     20  PTP Delay Request Frame Transmitted */
    uint32_t SFT:1;            /*!< bit:     21  PTP Sync Frame Transmitted         */
    uint32_t PDRQFR:1;         /*!< bit:     22  PDelay Request Frame Received      */
    uint32_t PDRSFR:1;         /*!< bit:     23  PDelay Response Frame Received     */
    uint32_t PDRQFT:1;         /*!< bit:     24  PDelay Request Frame Transmitted   */
    uint32_t PDRSFT:1;         /*!< bit:     25  PDelay Response Frame Transmitted  */
    uint32_t SRI:1;            /*!< bit:     26  TSU Seconds Register Increment     */
    uint32_t :1;               /*!< bit:     27  Reserved                           */
    uint32_t WOL:1;            /*!< bit:     28  Wake On LAN                        */
    uint32_t TSUCMP:1;         /*!< bit:     29  Tsu timer comparison               */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_IDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IDR_OFFSET             0x02C        /**< \brief (GMAC_IDR offset) Interrupt Disable Register */

#define GMAC_IDR_MFS_Pos            0            /**< \brief (GMAC_IDR) Management Frame Sent */
#define GMAC_IDR_MFS                (_U_(0x1) << GMAC_IDR_MFS_Pos)
#define GMAC_IDR_RCOMP_Pos          1            /**< \brief (GMAC_IDR) Receive Complete */
#define GMAC_IDR_RCOMP              (_U_(0x1) << GMAC_IDR_RCOMP_Pos)
#define GMAC_IDR_RXUBR_Pos          2            /**< \brief (GMAC_IDR) RX Used Bit Read */
#define GMAC_IDR_RXUBR              (_U_(0x1) << GMAC_IDR_RXUBR_Pos)
#define GMAC_IDR_TXUBR_Pos          3            /**< \brief (GMAC_IDR) TX Used Bit Read */
#define GMAC_IDR_TXUBR              (_U_(0x1) << GMAC_IDR_TXUBR_Pos)
#define GMAC_IDR_TUR_Pos            4            /**< \brief (GMAC_IDR) Transmit Underrun */
#define GMAC_IDR_TUR                (_U_(0x1) << GMAC_IDR_TUR_Pos)
#define GMAC_IDR_RLEX_Pos           5            /**< \brief (GMAC_IDR) Retry Limit Exceeded or Late Collision */
#define GMAC_IDR_RLEX               (_U_(0x1) << GMAC_IDR_RLEX_Pos)
#define GMAC_IDR_TFC_Pos            6            /**< \brief (GMAC_IDR) Transmit Frame Corruption Due to AHB Error */
#define GMAC_IDR_TFC                (_U_(0x1) << GMAC_IDR_TFC_Pos)
#define GMAC_IDR_TCOMP_Pos          7            /**< \brief (GMAC_IDR) Transmit Complete */
#define GMAC_IDR_TCOMP              (_U_(0x1) << GMAC_IDR_TCOMP_Pos)
#define GMAC_IDR_ROVR_Pos           10           /**< \brief (GMAC_IDR) Receive Overrun */
#define GMAC_IDR_ROVR               (_U_(0x1) << GMAC_IDR_ROVR_Pos)
#define GMAC_IDR_HRESP_Pos          11           /**< \brief (GMAC_IDR) HRESP Not OK */
#define GMAC_IDR_HRESP              (_U_(0x1) << GMAC_IDR_HRESP_Pos)
#define GMAC_IDR_PFNZ_Pos           12           /**< \brief (GMAC_IDR) Pause Frame with Non-zero Pause Quantum Received */
#define GMAC_IDR_PFNZ               (_U_(0x1) << GMAC_IDR_PFNZ_Pos)
#define GMAC_IDR_PTZ_Pos            13           /**< \brief (GMAC_IDR) Pause Time Zero */
#define GMAC_IDR_PTZ                (_U_(0x1) << GMAC_IDR_PTZ_Pos)
#define GMAC_IDR_PFTR_Pos           14           /**< \brief (GMAC_IDR) Pause Frame Transmitted */
#define GMAC_IDR_PFTR               (_U_(0x1) << GMAC_IDR_PFTR_Pos)
#define GMAC_IDR_EXINT_Pos          15           /**< \brief (GMAC_IDR) External Interrupt */
#define GMAC_IDR_EXINT              (_U_(0x1) << GMAC_IDR_EXINT_Pos)
#define GMAC_IDR_DRQFR_Pos          18           /**< \brief (GMAC_IDR) PTP Delay Request Frame Received */
#define GMAC_IDR_DRQFR              (_U_(0x1) << GMAC_IDR_DRQFR_Pos)
#define GMAC_IDR_SFR_Pos            19           /**< \brief (GMAC_IDR) PTP Sync Frame Received */
#define GMAC_IDR_SFR                (_U_(0x1) << GMAC_IDR_SFR_Pos)
#define GMAC_IDR_DRQFT_Pos          20           /**< \brief (GMAC_IDR) PTP Delay Request Frame Transmitted */
#define GMAC_IDR_DRQFT              (_U_(0x1) << GMAC_IDR_DRQFT_Pos)
#define GMAC_IDR_SFT_Pos            21           /**< \brief (GMAC_IDR) PTP Sync Frame Transmitted */
#define GMAC_IDR_SFT                (_U_(0x1) << GMAC_IDR_SFT_Pos)
#define GMAC_IDR_PDRQFR_Pos         22           /**< \brief (GMAC_IDR) PDelay Request Frame Received */
#define GMAC_IDR_PDRQFR             (_U_(0x1) << GMAC_IDR_PDRQFR_Pos)
#define GMAC_IDR_PDRSFR_Pos         23           /**< \brief (GMAC_IDR) PDelay Response Frame Received */
#define GMAC_IDR_PDRSFR             (_U_(0x1) << GMAC_IDR_PDRSFR_Pos)
#define GMAC_IDR_PDRQFT_Pos         24           /**< \brief (GMAC_IDR) PDelay Request Frame Transmitted */
#define GMAC_IDR_PDRQFT             (_U_(0x1) << GMAC_IDR_PDRQFT_Pos)
#define GMAC_IDR_PDRSFT_Pos         25           /**< \brief (GMAC_IDR) PDelay Response Frame Transmitted */
#define GMAC_IDR_PDRSFT             (_U_(0x1) << GMAC_IDR_PDRSFT_Pos)
#define GMAC_IDR_SRI_Pos            26           /**< \brief (GMAC_IDR) TSU Seconds Register Increment */
#define GMAC_IDR_SRI                (_U_(0x1) << GMAC_IDR_SRI_Pos)
#define GMAC_IDR_WOL_Pos            28           /**< \brief (GMAC_IDR) Wake On LAN */
#define GMAC_IDR_WOL                (_U_(0x1) << GMAC_IDR_WOL_Pos)
#define GMAC_IDR_TSUCMP_Pos         29           /**< \brief (GMAC_IDR) Tsu timer comparison */
#define GMAC_IDR_TSUCMP             (_U_(0x1) << GMAC_IDR_TSUCMP_Pos)
#define GMAC_IDR_MASK               _U_(0x37FCFCFF) /**< \brief (GMAC_IDR) MASK Register */

/* -------- GMAC_IMR : (GMAC Offset: 0x030) (R/  32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MFS:1;            /*!< bit:      0  Management Frame Sent              */
    uint32_t RCOMP:1;          /*!< bit:      1  Receive Complete                   */
    uint32_t RXUBR:1;          /*!< bit:      2  RX Used Bit Read                   */
    uint32_t TXUBR:1;          /*!< bit:      3  TX Used Bit Read                   */
    uint32_t TUR:1;            /*!< bit:      4  Transmit Underrun                  */
    uint32_t RLEX:1;           /*!< bit:      5  Retry Limit Exceeded               */
    uint32_t TFC:1;            /*!< bit:      6  Transmit Frame Corruption Due to AHB Error */
    uint32_t TCOMP:1;          /*!< bit:      7  Transmit Complete                  */
    uint32_t :2;               /*!< bit:  8.. 9  Reserved                           */
    uint32_t ROVR:1;           /*!< bit:     10  Receive Overrun                    */
    uint32_t HRESP:1;          /*!< bit:     11  HRESP Not OK                       */
    uint32_t PFNZ:1;           /*!< bit:     12  Pause Frame with Non-zero Pause Quantum Received */
    uint32_t PTZ:1;            /*!< bit:     13  Pause Time Zero                    */
    uint32_t PFTR:1;           /*!< bit:     14  Pause Frame Transmitted            */
    uint32_t EXINT:1;          /*!< bit:     15  External Interrupt                 */
    uint32_t :2;               /*!< bit: 16..17  Reserved                           */
    uint32_t DRQFR:1;          /*!< bit:     18  PTP Delay Request Frame Received   */
    uint32_t SFR:1;            /*!< bit:     19  PTP Sync Frame Received            */
    uint32_t DRQFT:1;          /*!< bit:     20  PTP Delay Request Frame Transmitted */
    uint32_t SFT:1;            /*!< bit:     21  PTP Sync Frame Transmitted         */
    uint32_t PDRQFR:1;         /*!< bit:     22  PDelay Request Frame Received      */
    uint32_t PDRSFR:1;         /*!< bit:     23  PDelay Response Frame Received     */
    uint32_t PDRQFT:1;         /*!< bit:     24  PDelay Request Frame Transmitted   */
    uint32_t PDRSFT:1;         /*!< bit:     25  PDelay Response Frame Transmitted  */
    uint32_t SRI:1;            /*!< bit:     26  TSU Seconds Register Increment     */
    uint32_t :1;               /*!< bit:     27  Reserved                           */
    uint32_t WOL:1;            /*!< bit:     28  Wake On Lan                        */
    uint32_t TSUCMP:1;         /*!< bit:     29  Tsu timer comparison               */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_IMR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IMR_OFFSET             0x030        /**< \brief (GMAC_IMR offset) Interrupt Mask Register */
#define GMAC_IMR_RESETVALUE         _U_(0x3FFFFFFF) /**< \brief (GMAC_IMR reset_value) Interrupt Mask Register */

#define GMAC_IMR_MFS_Pos            0            /**< \brief (GMAC_IMR) Management Frame Sent */
#define GMAC_IMR_MFS                (_U_(0x1) << GMAC_IMR_MFS_Pos)
#define GMAC_IMR_RCOMP_Pos          1            /**< \brief (GMAC_IMR) Receive Complete */
#define GMAC_IMR_RCOMP              (_U_(0x1) << GMAC_IMR_RCOMP_Pos)
#define GMAC_IMR_RXUBR_Pos          2            /**< \brief (GMAC_IMR) RX Used Bit Read */
#define GMAC_IMR_RXUBR              (_U_(0x1) << GMAC_IMR_RXUBR_Pos)
#define GMAC_IMR_TXUBR_Pos          3            /**< \brief (GMAC_IMR) TX Used Bit Read */
#define GMAC_IMR_TXUBR              (_U_(0x1) << GMAC_IMR_TXUBR_Pos)
#define GMAC_IMR_TUR_Pos            4            /**< \brief (GMAC_IMR) Transmit Underrun */
#define GMAC_IMR_TUR                (_U_(0x1) << GMAC_IMR_TUR_Pos)
#define GMAC_IMR_RLEX_Pos           5            /**< \brief (GMAC_IMR) Retry Limit Exceeded */
#define GMAC_IMR_RLEX               (_U_(0x1) << GMAC_IMR_RLEX_Pos)
#define GMAC_IMR_TFC_Pos            6            /**< \brief (GMAC_IMR) Transmit Frame Corruption Due to AHB Error */
#define GMAC_IMR_TFC                (_U_(0x1) << GMAC_IMR_TFC_Pos)
#define GMAC_IMR_TCOMP_Pos          7            /**< \brief (GMAC_IMR) Transmit Complete */
#define GMAC_IMR_TCOMP              (_U_(0x1) << GMAC_IMR_TCOMP_Pos)
#define GMAC_IMR_ROVR_Pos           10           /**< \brief (GMAC_IMR) Receive Overrun */
#define GMAC_IMR_ROVR               (_U_(0x1) << GMAC_IMR_ROVR_Pos)
#define GMAC_IMR_HRESP_Pos          11           /**< \brief (GMAC_IMR) HRESP Not OK */
#define GMAC_IMR_HRESP              (_U_(0x1) << GMAC_IMR_HRESP_Pos)
#define GMAC_IMR_PFNZ_Pos           12           /**< \brief (GMAC_IMR) Pause Frame with Non-zero Pause Quantum Received */
#define GMAC_IMR_PFNZ               (_U_(0x1) << GMAC_IMR_PFNZ_Pos)
#define GMAC_IMR_PTZ_Pos            13           /**< \brief (GMAC_IMR) Pause Time Zero */
#define GMAC_IMR_PTZ                (_U_(0x1) << GMAC_IMR_PTZ_Pos)
#define GMAC_IMR_PFTR_Pos           14           /**< \brief (GMAC_IMR) Pause Frame Transmitted */
#define GMAC_IMR_PFTR               (_U_(0x1) << GMAC_IMR_PFTR_Pos)
#define GMAC_IMR_EXINT_Pos          15           /**< \brief (GMAC_IMR) External Interrupt */
#define GMAC_IMR_EXINT              (_U_(0x1) << GMAC_IMR_EXINT_Pos)
#define GMAC_IMR_DRQFR_Pos          18           /**< \brief (GMAC_IMR) PTP Delay Request Frame Received */
#define GMAC_IMR_DRQFR              (_U_(0x1) << GMAC_IMR_DRQFR_Pos)
#define GMAC_IMR_SFR_Pos            19           /**< \brief (GMAC_IMR) PTP Sync Frame Received */
#define GMAC_IMR_SFR                (_U_(0x1) << GMAC_IMR_SFR_Pos)
#define GMAC_IMR_DRQFT_Pos          20           /**< \brief (GMAC_IMR) PTP Delay Request Frame Transmitted */
#define GMAC_IMR_DRQFT              (_U_(0x1) << GMAC_IMR_DRQFT_Pos)
#define GMAC_IMR_SFT_Pos            21           /**< \brief (GMAC_IMR) PTP Sync Frame Transmitted */
#define GMAC_IMR_SFT                (_U_(0x1) << GMAC_IMR_SFT_Pos)
#define GMAC_IMR_PDRQFR_Pos         22           /**< \brief (GMAC_IMR) PDelay Request Frame Received */
#define GMAC_IMR_PDRQFR             (_U_(0x1) << GMAC_IMR_PDRQFR_Pos)
#define GMAC_IMR_PDRSFR_Pos         23           /**< \brief (GMAC_IMR) PDelay Response Frame Received */
#define GMAC_IMR_PDRSFR             (_U_(0x1) << GMAC_IMR_PDRSFR_Pos)
#define GMAC_IMR_PDRQFT_Pos         24           /**< \brief (GMAC_IMR) PDelay Request Frame Transmitted */
#define GMAC_IMR_PDRQFT             (_U_(0x1) << GMAC_IMR_PDRQFT_Pos)
#define GMAC_IMR_PDRSFT_Pos         25           /**< \brief (GMAC_IMR) PDelay Response Frame Transmitted */
#define GMAC_IMR_PDRSFT             (_U_(0x1) << GMAC_IMR_PDRSFT_Pos)
#define GMAC_IMR_SRI_Pos            26           /**< \brief (GMAC_IMR) TSU Seconds Register Increment */
#define GMAC_IMR_SRI                (_U_(0x1) << GMAC_IMR_SRI_Pos)
#define GMAC_IMR_WOL_Pos            28           /**< \brief (GMAC_IMR) Wake On Lan */
#define GMAC_IMR_WOL                (_U_(0x1) << GMAC_IMR_WOL_Pos)
#define GMAC_IMR_TSUCMP_Pos         29           /**< \brief (GMAC_IMR) Tsu timer comparison */
#define GMAC_IMR_TSUCMP             (_U_(0x1) << GMAC_IMR_TSUCMP_Pos)
#define GMAC_IMR_MASK               _U_(0x37FCFCFF) /**< \brief (GMAC_IMR) MASK Register */

/* -------- GMAC_MAN : (GMAC Offset: 0x034) (R/W 32) PHY Maintenance Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DATA:16;          /*!< bit:  0..15  PHY Data                           */
    uint32_t WTN:2;            /*!< bit: 16..17  Write Ten                          */
    uint32_t REGA:5;           /*!< bit: 18..22  Register Address                   */
    uint32_t PHYA:5;           /*!< bit: 23..27  PHY Address                        */
    uint32_t OP:2;             /*!< bit: 28..29  Operation                          */
    uint32_t CLTTO:1;          /*!< bit:     30  Clause 22 Operation                */
    uint32_t WZO:1;            /*!< bit:     31  Write ZERO                         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_MAN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_MAN_OFFSET             0x034        /**< \brief (GMAC_MAN offset) PHY Maintenance Register */
#define GMAC_MAN_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_MAN reset_value) PHY Maintenance Register */

#define GMAC_MAN_DATA_Pos           0            /**< \brief (GMAC_MAN) PHY Data */
#define GMAC_MAN_DATA_Msk           (_U_(0xFFFF) << GMAC_MAN_DATA_Pos)
#define GMAC_MAN_DATA(value)        (GMAC_MAN_DATA_Msk & ((value) << GMAC_MAN_DATA_Pos))
#define GMAC_MAN_WTN_Pos            16           /**< \brief (GMAC_MAN) Write Ten */
#define GMAC_MAN_WTN_Msk            (_U_(0x3) << GMAC_MAN_WTN_Pos)
#define GMAC_MAN_WTN(value)         (GMAC_MAN_WTN_Msk & ((value) << GMAC_MAN_WTN_Pos))
#define GMAC_MAN_REGA_Pos           18           /**< \brief (GMAC_MAN) Register Address */
#define GMAC_MAN_REGA_Msk           (_U_(0x1F) << GMAC_MAN_REGA_Pos)
#define GMAC_MAN_REGA(value)        (GMAC_MAN_REGA_Msk & ((value) << GMAC_MAN_REGA_Pos))
#define GMAC_MAN_PHYA_Pos           23           /**< \brief (GMAC_MAN) PHY Address */
#define GMAC_MAN_PHYA_Msk           (_U_(0x1F) << GMAC_MAN_PHYA_Pos)
#define GMAC_MAN_PHYA(value)        (GMAC_MAN_PHYA_Msk & ((value) << GMAC_MAN_PHYA_Pos))
#define GMAC_MAN_OP_Pos             28           /**< \brief (GMAC_MAN) Operation */
#define GMAC_MAN_OP_Msk             (_U_(0x3) << GMAC_MAN_OP_Pos)
#define GMAC_MAN_OP(value)          (GMAC_MAN_OP_Msk & ((value) << GMAC_MAN_OP_Pos))
#define GMAC_MAN_CLTTO_Pos          30           /**< \brief (GMAC_MAN) Clause 22 Operation */
#define GMAC_MAN_CLTTO              (_U_(0x1) << GMAC_MAN_CLTTO_Pos)
#define GMAC_MAN_WZO_Pos            31           /**< \brief (GMAC_MAN) Write ZERO */
#define GMAC_MAN_WZO                (_U_(0x1) << GMAC_MAN_WZO_Pos)
#define GMAC_MAN_MASK               _U_(0xFFFFFFFF) /**< \brief (GMAC_MAN) MASK Register */

/* -------- GMAC_RPQ : (GMAC Offset: 0x038) (R/  32) Received Pause Quantum Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RPQ:16;           /*!< bit:  0..15  Received Pause Quantum             */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RPQ_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RPQ_OFFSET             0x038        /**< \brief (GMAC_RPQ offset) Received Pause Quantum Register */
#define GMAC_RPQ_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_RPQ reset_value) Received Pause Quantum Register */

#define GMAC_RPQ_RPQ_Pos            0            /**< \brief (GMAC_RPQ) Received Pause Quantum */
#define GMAC_RPQ_RPQ_Msk            (_U_(0xFFFF) << GMAC_RPQ_RPQ_Pos)
#define GMAC_RPQ_RPQ(value)         (GMAC_RPQ_RPQ_Msk & ((value) << GMAC_RPQ_RPQ_Pos))
#define GMAC_RPQ_MASK               _U_(0x0000FFFF) /**< \brief (GMAC_RPQ) MASK Register */

/* -------- GMAC_TPQ : (GMAC Offset: 0x03C) (R/W 32) Transmit Pause Quantum Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TPQ:16;           /*!< bit:  0..15  Transmit Pause Quantum             */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TPQ_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TPQ_OFFSET             0x03C        /**< \brief (GMAC_TPQ offset) Transmit Pause Quantum Register */
#define GMAC_TPQ_RESETVALUE         _U_(0x0000FFFF) /**< \brief (GMAC_TPQ reset_value) Transmit Pause Quantum Register */

#define GMAC_TPQ_TPQ_Pos            0            /**< \brief (GMAC_TPQ) Transmit Pause Quantum */
#define GMAC_TPQ_TPQ_Msk            (_U_(0xFFFF) << GMAC_TPQ_TPQ_Pos)
#define GMAC_TPQ_TPQ(value)         (GMAC_TPQ_TPQ_Msk & ((value) << GMAC_TPQ_TPQ_Pos))
#define GMAC_TPQ_MASK               _U_(0x0000FFFF) /**< \brief (GMAC_TPQ) MASK Register */

/* -------- GMAC_TPSF : (GMAC Offset: 0x040) (R/W 32) TX partial store and forward Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TPB1ADR:10;       /*!< bit:  0.. 9  TX packet buffer address           */
    uint32_t :21;              /*!< bit: 10..30  Reserved                           */
    uint32_t ENTXP:1;          /*!< bit:     31  Enable TX partial store and forward operation */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TPSF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TPSF_OFFSET            0x040        /**< \brief (GMAC_TPSF offset) TX partial store and forward Register */
#define GMAC_TPSF_RESETVALUE        _U_(0x000003FF) /**< \brief (GMAC_TPSF reset_value) TX partial store and forward Register */

#define GMAC_TPSF_TPB1ADR_Pos       0            /**< \brief (GMAC_TPSF) TX packet buffer address */
#define GMAC_TPSF_TPB1ADR_Msk       (_U_(0x3FF) << GMAC_TPSF_TPB1ADR_Pos)
#define GMAC_TPSF_TPB1ADR(value)    (GMAC_TPSF_TPB1ADR_Msk & ((value) << GMAC_TPSF_TPB1ADR_Pos))
#define GMAC_TPSF_ENTXP_Pos         31           /**< \brief (GMAC_TPSF) Enable TX partial store and forward operation */
#define GMAC_TPSF_ENTXP             (_U_(0x1) << GMAC_TPSF_ENTXP_Pos)
#define GMAC_TPSF_MASK              _U_(0x800003FF) /**< \brief (GMAC_TPSF) MASK Register */

/* -------- GMAC_RPSF : (GMAC Offset: 0x044) (R/W 32) RX partial store and forward Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RPB1ADR:10;       /*!< bit:  0.. 9  RX packet buffer address           */
    uint32_t :21;              /*!< bit: 10..30  Reserved                           */
    uint32_t ENRXP:1;          /*!< bit:     31  Enable RX partial store and forward operation */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RPSF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RPSF_OFFSET            0x044        /**< \brief (GMAC_RPSF offset) RX partial store and forward Register */
#define GMAC_RPSF_RESETVALUE        _U_(0x000003FF) /**< \brief (GMAC_RPSF reset_value) RX partial store and forward Register */

#define GMAC_RPSF_RPB1ADR_Pos       0            /**< \brief (GMAC_RPSF) RX packet buffer address */
#define GMAC_RPSF_RPB1ADR_Msk       (_U_(0x3FF) << GMAC_RPSF_RPB1ADR_Pos)
#define GMAC_RPSF_RPB1ADR(value)    (GMAC_RPSF_RPB1ADR_Msk & ((value) << GMAC_RPSF_RPB1ADR_Pos))
#define GMAC_RPSF_ENRXP_Pos         31           /**< \brief (GMAC_RPSF) Enable RX partial store and forward operation */
#define GMAC_RPSF_ENRXP             (_U_(0x1) << GMAC_RPSF_ENRXP_Pos)
#define GMAC_RPSF_MASK              _U_(0x800003FF) /**< \brief (GMAC_RPSF) MASK Register */

/* -------- GMAC_RJFML : (GMAC Offset: 0x048) (R/W 32) RX Jumbo Frame Max Length Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FML:14;           /*!< bit:  0..13  Frame Max Length                   */
    uint32_t :18;              /*!< bit: 14..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RJFML_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RJFML_OFFSET           0x048        /**< \brief (GMAC_RJFML offset) RX Jumbo Frame Max Length Register */
#define GMAC_RJFML_RESETVALUE       _U_(0x00003FFF) /**< \brief (GMAC_RJFML reset_value) RX Jumbo Frame Max Length Register */

#define GMAC_RJFML_FML_Pos          0            /**< \brief (GMAC_RJFML) Frame Max Length */
#define GMAC_RJFML_FML_Msk          (_U_(0x3FFF) << GMAC_RJFML_FML_Pos)
#define GMAC_RJFML_FML(value)       (GMAC_RJFML_FML_Msk & ((value) << GMAC_RJFML_FML_Pos))
#define GMAC_RJFML_MASK             _U_(0x00003FFF) /**< \brief (GMAC_RJFML) MASK Register */

/* -------- GMAC_HRB : (GMAC Offset: 0x080) (R/W 32) Hash Register Bottom [31:0] -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  Hash Address                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_HRB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_HRB_OFFSET             0x080        /**< \brief (GMAC_HRB offset) Hash Register Bottom [31:0] */
#define GMAC_HRB_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_HRB reset_value) Hash Register Bottom [31:0] */

#define GMAC_HRB_ADDR_Pos           0            /**< \brief (GMAC_HRB) Hash Address */
#define GMAC_HRB_ADDR_Msk           (_U_(0xFFFFFFFF) << GMAC_HRB_ADDR_Pos)
#define GMAC_HRB_ADDR(value)        (GMAC_HRB_ADDR_Msk & ((value) << GMAC_HRB_ADDR_Pos))
#define GMAC_HRB_MASK               _U_(0xFFFFFFFF) /**< \brief (GMAC_HRB) MASK Register */

/* -------- GMAC_HRT : (GMAC Offset: 0x084) (R/W 32) Hash Register Top [63:32] -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  Hash Address                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_HRT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_HRT_OFFSET             0x084        /**< \brief (GMAC_HRT offset) Hash Register Top [63:32] */
#define GMAC_HRT_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_HRT reset_value) Hash Register Top [63:32] */

#define GMAC_HRT_ADDR_Pos           0            /**< \brief (GMAC_HRT) Hash Address */
#define GMAC_HRT_ADDR_Msk           (_U_(0xFFFFFFFF) << GMAC_HRT_ADDR_Pos)
#define GMAC_HRT_ADDR(value)        (GMAC_HRT_ADDR_Msk & ((value) << GMAC_HRT_ADDR_Pos))
#define GMAC_HRT_MASK               _U_(0xFFFFFFFF) /**< \brief (GMAC_HRT) MASK Register */

/* -------- GMAC_SAB : (GMAC Offset: 0x088) (R/W 32) SA Specific Address Bottom [31:0] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  Specific Address 1                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SAB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SAB_OFFSET             0x088        /**< \brief (GMAC_SAB offset) Specific Address Bottom [31:0] Register */
#define GMAC_SAB_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_SAB reset_value) Specific Address Bottom [31:0] Register */

#define GMAC_SAB_ADDR_Pos           0            /**< \brief (GMAC_SAB) Specific Address 1 */
#define GMAC_SAB_ADDR_Msk           (_U_(0xFFFFFFFF) << GMAC_SAB_ADDR_Pos)
#define GMAC_SAB_ADDR(value)        (GMAC_SAB_ADDR_Msk & ((value) << GMAC_SAB_ADDR_Pos))
#define GMAC_SAB_MASK               _U_(0xFFFFFFFF) /**< \brief (GMAC_SAB) MASK Register */

/* -------- GMAC_SAT : (GMAC Offset: 0x08C) (R/W 32) SA Specific Address Top [47:32] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:16;          /*!< bit:  0..15  Specific Address 1                 */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SAT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SAT_OFFSET             0x08C        /**< \brief (GMAC_SAT offset) Specific Address Top [47:32] Register */
#define GMAC_SAT_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_SAT reset_value) Specific Address Top [47:32] Register */

#define GMAC_SAT_ADDR_Pos           0            /**< \brief (GMAC_SAT) Specific Address 1 */
#define GMAC_SAT_ADDR_Msk           (_U_(0xFFFF) << GMAC_SAT_ADDR_Pos)
#define GMAC_SAT_ADDR(value)        (GMAC_SAT_ADDR_Msk & ((value) << GMAC_SAT_ADDR_Pos))
#define GMAC_SAT_MASK               _U_(0x0000FFFF) /**< \brief (GMAC_SAT) MASK Register */

/* -------- GMAC_TIDM : (GMAC Offset: 0x0A8) (R/W 32) Type ID Match Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TID:16;           /*!< bit:  0..15  Type ID Match 1                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TIDM_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TIDM_OFFSET            0x0A8        /**< \brief (GMAC_TIDM offset) Type ID Match Register */
#define GMAC_TIDM_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_TIDM reset_value) Type ID Match Register */

#define GMAC_TIDM_TID_Pos           0            /**< \brief (GMAC_TIDM) Type ID Match 1 */
#define GMAC_TIDM_TID_Msk           (_U_(0xFFFF) << GMAC_TIDM_TID_Pos)
#define GMAC_TIDM_TID(value)        (GMAC_TIDM_TID_Msk & ((value) << GMAC_TIDM_TID_Pos))
#define GMAC_TIDM_MASK              _U_(0x0000FFFF) /**< \brief (GMAC_TIDM) MASK Register */

/* -------- GMAC_WOL : (GMAC Offset: 0x0B8) (R/W 32) Wake on LAN -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t IP:16;            /*!< bit:  0..15  IP address                         */
    uint32_t MAG:1;            /*!< bit:     16  Event enable                       */
    uint32_t ARP:1;            /*!< bit:     17  LAN ARP req                        */
    uint32_t SA1:1;            /*!< bit:     18  WOL specific address reg 1         */
    uint32_t MTI:1;            /*!< bit:     19  WOL LAN multicast                  */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_WOL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_WOL_OFFSET             0x0B8        /**< \brief (GMAC_WOL offset) Wake on LAN */
#define GMAC_WOL_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_WOL reset_value) Wake on LAN */

#define GMAC_WOL_IP_Pos             0            /**< \brief (GMAC_WOL) IP address */
#define GMAC_WOL_IP_Msk             (_U_(0xFFFF) << GMAC_WOL_IP_Pos)
#define GMAC_WOL_IP(value)          (GMAC_WOL_IP_Msk & ((value) << GMAC_WOL_IP_Pos))
#define GMAC_WOL_MAG_Pos            16           /**< \brief (GMAC_WOL) Event enable */
#define GMAC_WOL_MAG                (_U_(0x1) << GMAC_WOL_MAG_Pos)
#define GMAC_WOL_ARP_Pos            17           /**< \brief (GMAC_WOL) LAN ARP req */
#define GMAC_WOL_ARP                (_U_(0x1) << GMAC_WOL_ARP_Pos)
#define GMAC_WOL_SA1_Pos            18           /**< \brief (GMAC_WOL) WOL specific address reg 1 */
#define GMAC_WOL_SA1                (_U_(0x1) << GMAC_WOL_SA1_Pos)
#define GMAC_WOL_MTI_Pos            19           /**< \brief (GMAC_WOL) WOL LAN multicast */
#define GMAC_WOL_MTI                (_U_(0x1) << GMAC_WOL_MTI_Pos)
#define GMAC_WOL_MASK               _U_(0x000FFFFF) /**< \brief (GMAC_WOL) MASK Register */

/* -------- GMAC_IPGS : (GMAC Offset: 0x0BC) (R/W 32) IPG Stretch Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FL:16;            /*!< bit:  0..15  Frame Length                       */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_IPGS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IPGS_OFFSET            0x0BC        /**< \brief (GMAC_IPGS offset) IPG Stretch Register */
#define GMAC_IPGS_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_IPGS reset_value) IPG Stretch Register */

#define GMAC_IPGS_FL_Pos            0            /**< \brief (GMAC_IPGS) Frame Length */
#define GMAC_IPGS_FL_Msk            (_U_(0xFFFF) << GMAC_IPGS_FL_Pos)
#define GMAC_IPGS_FL(value)         (GMAC_IPGS_FL_Msk & ((value) << GMAC_IPGS_FL_Pos))
#define GMAC_IPGS_MASK              _U_(0x0000FFFF) /**< \brief (GMAC_IPGS) MASK Register */

/* -------- GMAC_SVLAN : (GMAC Offset: 0x0C0) (R/W 32) Stacked VLAN Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t VLAN_TYPE:16;     /*!< bit:  0..15  User Defined VLAN_TYPE Field       */
    uint32_t :15;              /*!< bit: 16..30  Reserved                           */
    uint32_t ESVLAN:1;         /*!< bit:     31  Enable Stacked VLAN Processing Mode */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SVLAN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SVLAN_OFFSET           0x0C0        /**< \brief (GMAC_SVLAN offset) Stacked VLAN Register */
#define GMAC_SVLAN_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_SVLAN reset_value) Stacked VLAN Register */

#define GMAC_SVLAN_VLAN_TYPE_Pos    0            /**< \brief (GMAC_SVLAN) User Defined VLAN_TYPE Field */
#define GMAC_SVLAN_VLAN_TYPE_Msk    (_U_(0xFFFF) << GMAC_SVLAN_VLAN_TYPE_Pos)
#define GMAC_SVLAN_VLAN_TYPE(value) (GMAC_SVLAN_VLAN_TYPE_Msk & ((value) << GMAC_SVLAN_VLAN_TYPE_Pos))
#define GMAC_SVLAN_ESVLAN_Pos       31           /**< \brief (GMAC_SVLAN) Enable Stacked VLAN Processing Mode */
#define GMAC_SVLAN_ESVLAN           (_U_(0x1) << GMAC_SVLAN_ESVLAN_Pos)
#define GMAC_SVLAN_MASK             _U_(0x8000FFFF) /**< \brief (GMAC_SVLAN) MASK Register */

/* -------- GMAC_TPFCP : (GMAC Offset: 0x0C4) (R/W 32) Transmit PFC Pause Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PEV:8;            /*!< bit:  0.. 7  Priority Enable Vector             */
    uint32_t PQ:8;             /*!< bit:  8..15  Pause Quantum                      */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TPFCP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TPFCP_OFFSET           0x0C4        /**< \brief (GMAC_TPFCP offset) Transmit PFC Pause Register */
#define GMAC_TPFCP_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_TPFCP reset_value) Transmit PFC Pause Register */

#define GMAC_TPFCP_PEV_Pos          0            /**< \brief (GMAC_TPFCP) Priority Enable Vector */
#define GMAC_TPFCP_PEV_Msk          (_U_(0xFF) << GMAC_TPFCP_PEV_Pos)
#define GMAC_TPFCP_PEV(value)       (GMAC_TPFCP_PEV_Msk & ((value) << GMAC_TPFCP_PEV_Pos))
#define GMAC_TPFCP_PQ_Pos           8            /**< \brief (GMAC_TPFCP) Pause Quantum */
#define GMAC_TPFCP_PQ_Msk           (_U_(0xFF) << GMAC_TPFCP_PQ_Pos)
#define GMAC_TPFCP_PQ(value)        (GMAC_TPFCP_PQ_Msk & ((value) << GMAC_TPFCP_PQ_Pos))
#define GMAC_TPFCP_MASK             _U_(0x0000FFFF) /**< \brief (GMAC_TPFCP) MASK Register */

/* -------- GMAC_SAMB1 : (GMAC Offset: 0x0C8) (R/W 32) Specific Address 1 Mask Bottom [31:0] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  Specific Address 1 Mask            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SAMB1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SAMB1_OFFSET           0x0C8        /**< \brief (GMAC_SAMB1 offset) Specific Address 1 Mask Bottom [31:0] Register */
#define GMAC_SAMB1_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_SAMB1 reset_value) Specific Address 1 Mask Bottom [31:0] Register */

#define GMAC_SAMB1_ADDR_Pos         0            /**< \brief (GMAC_SAMB1) Specific Address 1 Mask */
#define GMAC_SAMB1_ADDR_Msk         (_U_(0xFFFFFFFF) << GMAC_SAMB1_ADDR_Pos)
#define GMAC_SAMB1_ADDR(value)      (GMAC_SAMB1_ADDR_Msk & ((value) << GMAC_SAMB1_ADDR_Pos))
#define GMAC_SAMB1_MASK             _U_(0xFFFFFFFF) /**< \brief (GMAC_SAMB1) MASK Register */

/* -------- GMAC_SAMT1 : (GMAC Offset: 0x0CC) (R/W 32) Specific Address 1 Mask Top [47:32] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:16;          /*!< bit:  0..15  Specific Address 1 Mask            */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SAMT1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SAMT1_OFFSET           0x0CC        /**< \brief (GMAC_SAMT1 offset) Specific Address 1 Mask Top [47:32] Register */
#define GMAC_SAMT1_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_SAMT1 reset_value) Specific Address 1 Mask Top [47:32] Register */

#define GMAC_SAMT1_ADDR_Pos         0            /**< \brief (GMAC_SAMT1) Specific Address 1 Mask */
#define GMAC_SAMT1_ADDR_Msk         (_U_(0xFFFF) << GMAC_SAMT1_ADDR_Pos)
#define GMAC_SAMT1_ADDR(value)      (GMAC_SAMT1_ADDR_Msk & ((value) << GMAC_SAMT1_ADDR_Pos))
#define GMAC_SAMT1_MASK             _U_(0x0000FFFF) /**< \brief (GMAC_SAMT1) MASK Register */

/* -------- GMAC_NSC : (GMAC Offset: 0x0DC) (R/W 32) Tsu timer comparison nanoseconds Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NANOSEC:21;       /*!< bit:  0..20  1588 Timer Nanosecond comparison value */
    uint32_t :11;              /*!< bit: 21..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_NSC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_NSC_OFFSET             0x0DC        /**< \brief (GMAC_NSC offset) Tsu timer comparison nanoseconds Register */
#define GMAC_NSC_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_NSC reset_value) Tsu timer comparison nanoseconds Register */

#define GMAC_NSC_NANOSEC_Pos        0            /**< \brief (GMAC_NSC) 1588 Timer Nanosecond comparison value */
#define GMAC_NSC_NANOSEC_Msk        (_U_(0x1FFFFF) << GMAC_NSC_NANOSEC_Pos)
#define GMAC_NSC_NANOSEC(value)     (GMAC_NSC_NANOSEC_Msk & ((value) << GMAC_NSC_NANOSEC_Pos))
#define GMAC_NSC_MASK               _U_(0x001FFFFF) /**< \brief (GMAC_NSC) MASK Register */

/* -------- GMAC_SCL : (GMAC Offset: 0x0E0) (R/W 32) Tsu timer second comparison Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SEC:32;           /*!< bit:  0..31  1588 Timer Second comparison value */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SCL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SCL_OFFSET             0x0E0        /**< \brief (GMAC_SCL offset) Tsu timer second comparison Register */
#define GMAC_SCL_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_SCL reset_value) Tsu timer second comparison Register */

#define GMAC_SCL_SEC_Pos            0            /**< \brief (GMAC_SCL) 1588 Timer Second comparison value */
#define GMAC_SCL_SEC_Msk            (_U_(0xFFFFFFFF) << GMAC_SCL_SEC_Pos)
#define GMAC_SCL_SEC(value)         (GMAC_SCL_SEC_Msk & ((value) << GMAC_SCL_SEC_Pos))
#define GMAC_SCL_MASK               _U_(0xFFFFFFFF) /**< \brief (GMAC_SCL) MASK Register */

/* -------- GMAC_SCH : (GMAC Offset: 0x0E4) (R/W 32) Tsu timer second comparison Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SEC:16;           /*!< bit:  0..15  1588 Timer Second comparison value */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SCH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SCH_OFFSET             0x0E4        /**< \brief (GMAC_SCH offset) Tsu timer second comparison Register */
#define GMAC_SCH_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_SCH reset_value) Tsu timer second comparison Register */

#define GMAC_SCH_SEC_Pos            0            /**< \brief (GMAC_SCH) 1588 Timer Second comparison value */
#define GMAC_SCH_SEC_Msk            (_U_(0xFFFF) << GMAC_SCH_SEC_Pos)
#define GMAC_SCH_SEC(value)         (GMAC_SCH_SEC_Msk & ((value) << GMAC_SCH_SEC_Pos))
#define GMAC_SCH_MASK               _U_(0x0000FFFF) /**< \brief (GMAC_SCH) MASK Register */

/* -------- GMAC_EFTSH : (GMAC Offset: 0x0E8) (R/  32) PTP Event Frame Transmitted Seconds High Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:16;           /*!< bit:  0..15  Register Update                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFTSH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFTSH_OFFSET           0x0E8        /**< \brief (GMAC_EFTSH offset) PTP Event Frame Transmitted Seconds High Register */
#define GMAC_EFTSH_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_EFTSH reset_value) PTP Event Frame Transmitted Seconds High Register */

#define GMAC_EFTSH_RUD_Pos          0            /**< \brief (GMAC_EFTSH) Register Update */
#define GMAC_EFTSH_RUD_Msk          (_U_(0xFFFF) << GMAC_EFTSH_RUD_Pos)
#define GMAC_EFTSH_RUD(value)       (GMAC_EFTSH_RUD_Msk & ((value) << GMAC_EFTSH_RUD_Pos))
#define GMAC_EFTSH_MASK             _U_(0x0000FFFF) /**< \brief (GMAC_EFTSH) MASK Register */

/* -------- GMAC_EFRSH : (GMAC Offset: 0x0EC) (R/  32) PTP Event Frame Received Seconds High Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:16;           /*!< bit:  0..15  Register Update                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFRSH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFRSH_OFFSET           0x0EC        /**< \brief (GMAC_EFRSH offset) PTP Event Frame Received Seconds High Register */
#define GMAC_EFRSH_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_EFRSH reset_value) PTP Event Frame Received Seconds High Register */

#define GMAC_EFRSH_RUD_Pos          0            /**< \brief (GMAC_EFRSH) Register Update */
#define GMAC_EFRSH_RUD_Msk          (_U_(0xFFFF) << GMAC_EFRSH_RUD_Pos)
#define GMAC_EFRSH_RUD(value)       (GMAC_EFRSH_RUD_Msk & ((value) << GMAC_EFRSH_RUD_Pos))
#define GMAC_EFRSH_MASK             _U_(0x0000FFFF) /**< \brief (GMAC_EFRSH) MASK Register */

/* -------- GMAC_PEFTSH : (GMAC Offset: 0x0F0) (R/  32) PTP Peer Event Frame Transmitted Seconds High Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:16;           /*!< bit:  0..15  Register Update                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFTSH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFTSH_OFFSET          0x0F0        /**< \brief (GMAC_PEFTSH offset) PTP Peer Event Frame Transmitted Seconds High Register */
#define GMAC_PEFTSH_RESETVALUE      _U_(0x00000000) /**< \brief (GMAC_PEFTSH reset_value) PTP Peer Event Frame Transmitted Seconds High Register */

#define GMAC_PEFTSH_RUD_Pos         0            /**< \brief (GMAC_PEFTSH) Register Update */
#define GMAC_PEFTSH_RUD_Msk         (_U_(0xFFFF) << GMAC_PEFTSH_RUD_Pos)
#define GMAC_PEFTSH_RUD(value)      (GMAC_PEFTSH_RUD_Msk & ((value) << GMAC_PEFTSH_RUD_Pos))
#define GMAC_PEFTSH_MASK            _U_(0x0000FFFF) /**< \brief (GMAC_PEFTSH) MASK Register */

/* -------- GMAC_PEFRSH : (GMAC Offset: 0x0F4) (R/  32) PTP Peer Event Frame Received Seconds High Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:16;           /*!< bit:  0..15  Register Update                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFRSH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFRSH_OFFSET          0x0F4        /**< \brief (GMAC_PEFRSH offset) PTP Peer Event Frame Received Seconds High Register */
#define GMAC_PEFRSH_RESETVALUE      _U_(0x00000000) /**< \brief (GMAC_PEFRSH reset_value) PTP Peer Event Frame Received Seconds High Register */

#define GMAC_PEFRSH_RUD_Pos         0            /**< \brief (GMAC_PEFRSH) Register Update */
#define GMAC_PEFRSH_RUD_Msk         (_U_(0xFFFF) << GMAC_PEFRSH_RUD_Pos)
#define GMAC_PEFRSH_RUD(value)      (GMAC_PEFRSH_RUD_Msk & ((value) << GMAC_PEFRSH_RUD_Pos))
#define GMAC_PEFRSH_MASK            _U_(0x0000FFFF) /**< \brief (GMAC_PEFRSH) MASK Register */

/* -------- GMAC_OTLO : (GMAC Offset: 0x100) (R/  32) Octets Transmitted [31:0] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TXO:32;           /*!< bit:  0..31  Transmitted Octets                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_OTLO_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_OTLO_OFFSET            0x100        /**< \brief (GMAC_OTLO offset) Octets Transmitted [31:0] Register */
#define GMAC_OTLO_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_OTLO reset_value) Octets Transmitted [31:0] Register */

#define GMAC_OTLO_TXO_Pos           0            /**< \brief (GMAC_OTLO) Transmitted Octets */
#define GMAC_OTLO_TXO_Msk           (_U_(0xFFFFFFFF) << GMAC_OTLO_TXO_Pos)
#define GMAC_OTLO_TXO(value)        (GMAC_OTLO_TXO_Msk & ((value) << GMAC_OTLO_TXO_Pos))
#define GMAC_OTLO_MASK              _U_(0xFFFFFFFF) /**< \brief (GMAC_OTLO) MASK Register */

/* -------- GMAC_OTHI : (GMAC Offset: 0x104) (R/  32) Octets Transmitted [47:32] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TXO:16;           /*!< bit:  0..15  Transmitted Octets                 */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_OTHI_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_OTHI_OFFSET            0x104        /**< \brief (GMAC_OTHI offset) Octets Transmitted [47:32] Register */
#define GMAC_OTHI_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_OTHI reset_value) Octets Transmitted [47:32] Register */

#define GMAC_OTHI_TXO_Pos           0            /**< \brief (GMAC_OTHI) Transmitted Octets */
#define GMAC_OTHI_TXO_Msk           (_U_(0xFFFF) << GMAC_OTHI_TXO_Pos)
#define GMAC_OTHI_TXO(value)        (GMAC_OTHI_TXO_Msk & ((value) << GMAC_OTHI_TXO_Pos))
#define GMAC_OTHI_MASK              _U_(0x0000FFFF) /**< \brief (GMAC_OTHI) MASK Register */

/* -------- GMAC_FT : (GMAC Offset: 0x108) (R/  32) Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FTX:32;           /*!< bit:  0..31  Frames Transmitted without Error   */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_FT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_FT_OFFSET              0x108        /**< \brief (GMAC_FT offset) Frames Transmitted Register */
#define GMAC_FT_RESETVALUE          _U_(0x00000000) /**< \brief (GMAC_FT reset_value) Frames Transmitted Register */

#define GMAC_FT_FTX_Pos             0            /**< \brief (GMAC_FT) Frames Transmitted without Error */
#define GMAC_FT_FTX_Msk             (_U_(0xFFFFFFFF) << GMAC_FT_FTX_Pos)
#define GMAC_FT_FTX(value)          (GMAC_FT_FTX_Msk & ((value) << GMAC_FT_FTX_Pos))
#define GMAC_FT_MASK                _U_(0xFFFFFFFF) /**< \brief (GMAC_FT) MASK Register */

/* -------- GMAC_BCFT : (GMAC Offset: 0x10C) (R/  32) Broadcast Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BFTX:32;          /*!< bit:  0..31  Broadcast Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_BCFT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_BCFT_OFFSET            0x10C        /**< \brief (GMAC_BCFT offset) Broadcast Frames Transmitted Register */
#define GMAC_BCFT_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_BCFT reset_value) Broadcast Frames Transmitted Register */

#define GMAC_BCFT_BFTX_Pos          0            /**< \brief (GMAC_BCFT) Broadcast Frames Transmitted without Error */
#define GMAC_BCFT_BFTX_Msk          (_U_(0xFFFFFFFF) << GMAC_BCFT_BFTX_Pos)
#define GMAC_BCFT_BFTX(value)       (GMAC_BCFT_BFTX_Msk & ((value) << GMAC_BCFT_BFTX_Pos))
#define GMAC_BCFT_MASK              _U_(0xFFFFFFFF) /**< \brief (GMAC_BCFT) MASK Register */

/* -------- GMAC_MFT : (GMAC Offset: 0x110) (R/  32) Multicast Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MFTX:32;          /*!< bit:  0..31  Multicast Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_MFT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_MFT_OFFSET             0x110        /**< \brief (GMAC_MFT offset) Multicast Frames Transmitted Register */
#define GMAC_MFT_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_MFT reset_value) Multicast Frames Transmitted Register */

#define GMAC_MFT_MFTX_Pos           0            /**< \brief (GMAC_MFT) Multicast Frames Transmitted without Error */
#define GMAC_MFT_MFTX_Msk           (_U_(0xFFFFFFFF) << GMAC_MFT_MFTX_Pos)
#define GMAC_MFT_MFTX(value)        (GMAC_MFT_MFTX_Msk & ((value) << GMAC_MFT_MFTX_Pos))
#define GMAC_MFT_MASK               _U_(0xFFFFFFFF) /**< \brief (GMAC_MFT) MASK Register */

/* -------- GMAC_PFT : (GMAC Offset: 0x114) (R/  32) Pause Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PFTX:16;          /*!< bit:  0..15  Pause Frames Transmitted Register  */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PFT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PFT_OFFSET             0x114        /**< \brief (GMAC_PFT offset) Pause Frames Transmitted Register */
#define GMAC_PFT_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_PFT reset_value) Pause Frames Transmitted Register */

#define GMAC_PFT_PFTX_Pos           0            /**< \brief (GMAC_PFT) Pause Frames Transmitted Register */
#define GMAC_PFT_PFTX_Msk           (_U_(0xFFFF) << GMAC_PFT_PFTX_Pos)
#define GMAC_PFT_PFTX(value)        (GMAC_PFT_PFTX_Msk & ((value) << GMAC_PFT_PFTX_Pos))
#define GMAC_PFT_MASK               _U_(0x0000FFFF) /**< \brief (GMAC_PFT) MASK Register */

/* -------- GMAC_BFT64 : (GMAC Offset: 0x118) (R/  32) 64 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  64 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_BFT64_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_BFT64_OFFSET           0x118        /**< \brief (GMAC_BFT64 offset) 64 Byte Frames Transmitted Register */
#define GMAC_BFT64_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_BFT64 reset_value) 64 Byte Frames Transmitted Register */

#define GMAC_BFT64_NFTX_Pos         0            /**< \brief (GMAC_BFT64) 64 Byte Frames Transmitted without Error */
#define GMAC_BFT64_NFTX_Msk         (_U_(0xFFFFFFFF) << GMAC_BFT64_NFTX_Pos)
#define GMAC_BFT64_NFTX(value)      (GMAC_BFT64_NFTX_Msk & ((value) << GMAC_BFT64_NFTX_Pos))
#define GMAC_BFT64_MASK             _U_(0xFFFFFFFF) /**< \brief (GMAC_BFT64) MASK Register */

/* -------- GMAC_TBFT127 : (GMAC Offset: 0x11C) (R/  32) 65 to 127 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  65 to 127 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFT127_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFT127_OFFSET         0x11C        /**< \brief (GMAC_TBFT127 offset) 65 to 127 Byte Frames Transmitted Register */
#define GMAC_TBFT127_RESETVALUE     _U_(0x00000000) /**< \brief (GMAC_TBFT127 reset_value) 65 to 127 Byte Frames Transmitted Register */

#define GMAC_TBFT127_NFTX_Pos       0            /**< \brief (GMAC_TBFT127) 65 to 127 Byte Frames Transmitted without Error */
#define GMAC_TBFT127_NFTX_Msk       (_U_(0xFFFFFFFF) << GMAC_TBFT127_NFTX_Pos)
#define GMAC_TBFT127_NFTX(value)    (GMAC_TBFT127_NFTX_Msk & ((value) << GMAC_TBFT127_NFTX_Pos))
#define GMAC_TBFT127_MASK           _U_(0xFFFFFFFF) /**< \brief (GMAC_TBFT127) MASK Register */

/* -------- GMAC_TBFT255 : (GMAC Offset: 0x120) (R/  32) 128 to 255 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  128 to 255 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFT255_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFT255_OFFSET         0x120        /**< \brief (GMAC_TBFT255 offset) 128 to 255 Byte Frames Transmitted Register */
#define GMAC_TBFT255_RESETVALUE     _U_(0x00000000) /**< \brief (GMAC_TBFT255 reset_value) 128 to 255 Byte Frames Transmitted Register */

#define GMAC_TBFT255_NFTX_Pos       0            /**< \brief (GMAC_TBFT255) 128 to 255 Byte Frames Transmitted without Error */
#define GMAC_TBFT255_NFTX_Msk       (_U_(0xFFFFFFFF) << GMAC_TBFT255_NFTX_Pos)
#define GMAC_TBFT255_NFTX(value)    (GMAC_TBFT255_NFTX_Msk & ((value) << GMAC_TBFT255_NFTX_Pos))
#define GMAC_TBFT255_MASK           _U_(0xFFFFFFFF) /**< \brief (GMAC_TBFT255) MASK Register */

/* -------- GMAC_TBFT511 : (GMAC Offset: 0x124) (R/  32) 256 to 511 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  256 to 511 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFT511_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFT511_OFFSET         0x124        /**< \brief (GMAC_TBFT511 offset) 256 to 511 Byte Frames Transmitted Register */
#define GMAC_TBFT511_RESETVALUE     _U_(0x00000000) /**< \brief (GMAC_TBFT511 reset_value) 256 to 511 Byte Frames Transmitted Register */

#define GMAC_TBFT511_NFTX_Pos       0            /**< \brief (GMAC_TBFT511) 256 to 511 Byte Frames Transmitted without Error */
#define GMAC_TBFT511_NFTX_Msk       (_U_(0xFFFFFFFF) << GMAC_TBFT511_NFTX_Pos)
#define GMAC_TBFT511_NFTX(value)    (GMAC_TBFT511_NFTX_Msk & ((value) << GMAC_TBFT511_NFTX_Pos))
#define GMAC_TBFT511_MASK           _U_(0xFFFFFFFF) /**< \brief (GMAC_TBFT511) MASK Register */

/* -------- GMAC_TBFT1023 : (GMAC Offset: 0x128) (R/  32) 512 to 1023 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  512 to 1023 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFT1023_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFT1023_OFFSET        0x128        /**< \brief (GMAC_TBFT1023 offset) 512 to 1023 Byte Frames Transmitted Register */
#define GMAC_TBFT1023_RESETVALUE    _U_(0x00000000) /**< \brief (GMAC_TBFT1023 reset_value) 512 to 1023 Byte Frames Transmitted Register */

#define GMAC_TBFT1023_NFTX_Pos      0            /**< \brief (GMAC_TBFT1023) 512 to 1023 Byte Frames Transmitted without Error */
#define GMAC_TBFT1023_NFTX_Msk      (_U_(0xFFFFFFFF) << GMAC_TBFT1023_NFTX_Pos)
#define GMAC_TBFT1023_NFTX(value)   (GMAC_TBFT1023_NFTX_Msk & ((value) << GMAC_TBFT1023_NFTX_Pos))
#define GMAC_TBFT1023_MASK          _U_(0xFFFFFFFF) /**< \brief (GMAC_TBFT1023) MASK Register */

/* -------- GMAC_TBFT1518 : (GMAC Offset: 0x12C) (R/  32) 1024 to 1518 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  1024 to 1518 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFT1518_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFT1518_OFFSET        0x12C        /**< \brief (GMAC_TBFT1518 offset) 1024 to 1518 Byte Frames Transmitted Register */
#define GMAC_TBFT1518_RESETVALUE    _U_(0x00000000) /**< \brief (GMAC_TBFT1518 reset_value) 1024 to 1518 Byte Frames Transmitted Register */

#define GMAC_TBFT1518_NFTX_Pos      0            /**< \brief (GMAC_TBFT1518) 1024 to 1518 Byte Frames Transmitted without Error */
#define GMAC_TBFT1518_NFTX_Msk      (_U_(0xFFFFFFFF) << GMAC_TBFT1518_NFTX_Pos)
#define GMAC_TBFT1518_NFTX(value)   (GMAC_TBFT1518_NFTX_Msk & ((value) << GMAC_TBFT1518_NFTX_Pos))
#define GMAC_TBFT1518_MASK          _U_(0xFFFFFFFF) /**< \brief (GMAC_TBFT1518) MASK Register */

/* -------- GMAC_GTBFT1518 : (GMAC Offset: 0x130) (R/  32) Greater Than 1518 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  Greater than 1518 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_GTBFT1518_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_GTBFT1518_OFFSET       0x130        /**< \brief (GMAC_GTBFT1518 offset) Greater Than 1518 Byte Frames Transmitted Register */
#define GMAC_GTBFT1518_RESETVALUE   _U_(0x00000000) /**< \brief (GMAC_GTBFT1518 reset_value) Greater Than 1518 Byte Frames Transmitted Register */

#define GMAC_GTBFT1518_NFTX_Pos     0            /**< \brief (GMAC_GTBFT1518) Greater than 1518 Byte Frames Transmitted without Error */
#define GMAC_GTBFT1518_NFTX_Msk     (_U_(0xFFFFFFFF) << GMAC_GTBFT1518_NFTX_Pos)
#define GMAC_GTBFT1518_NFTX(value)  (GMAC_GTBFT1518_NFTX_Msk & ((value) << GMAC_GTBFT1518_NFTX_Pos))
#define GMAC_GTBFT1518_MASK         _U_(0xFFFFFFFF) /**< \brief (GMAC_GTBFT1518) MASK Register */

/* -------- GMAC_TUR : (GMAC Offset: 0x134) (R/  32) Transmit Underruns Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TXUNR:10;         /*!< bit:  0.. 9  Transmit Underruns                 */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TUR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TUR_OFFSET             0x134        /**< \brief (GMAC_TUR offset) Transmit Underruns Register */
#define GMAC_TUR_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_TUR reset_value) Transmit Underruns Register */

#define GMAC_TUR_TXUNR_Pos          0            /**< \brief (GMAC_TUR) Transmit Underruns */
#define GMAC_TUR_TXUNR_Msk          (_U_(0x3FF) << GMAC_TUR_TXUNR_Pos)
#define GMAC_TUR_TXUNR(value)       (GMAC_TUR_TXUNR_Msk & ((value) << GMAC_TUR_TXUNR_Pos))
#define GMAC_TUR_MASK               _U_(0x000003FF) /**< \brief (GMAC_TUR) MASK Register */

/* -------- GMAC_SCF : (GMAC Offset: 0x138) (R/  32) Single Collision Frames Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SCOL:18;          /*!< bit:  0..17  Single Collision                   */
    uint32_t :14;              /*!< bit: 18..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SCF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_SCF_OFFSET             0x138        /**< \brief (GMAC_SCF offset) Single Collision Frames Register */
#define GMAC_SCF_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_SCF reset_value) Single Collision Frames Register */

#define GMAC_SCF_SCOL_Pos           0            /**< \brief (GMAC_SCF) Single Collision */
#define GMAC_SCF_SCOL_Msk           (_U_(0x3FFFF) << GMAC_SCF_SCOL_Pos)
#define GMAC_SCF_SCOL(value)        (GMAC_SCF_SCOL_Msk & ((value) << GMAC_SCF_SCOL_Pos))
#define GMAC_SCF_MASK               _U_(0x0003FFFF) /**< \brief (GMAC_SCF) MASK Register */

/* -------- GMAC_MCF : (GMAC Offset: 0x13C) (R/  32) Multiple Collision Frames Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MCOL:18;          /*!< bit:  0..17  Multiple Collision                 */
    uint32_t :14;              /*!< bit: 18..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_MCF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_MCF_OFFSET             0x13C        /**< \brief (GMAC_MCF offset) Multiple Collision Frames Register */
#define GMAC_MCF_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_MCF reset_value) Multiple Collision Frames Register */

#define GMAC_MCF_MCOL_Pos           0            /**< \brief (GMAC_MCF) Multiple Collision */
#define GMAC_MCF_MCOL_Msk           (_U_(0x3FFFF) << GMAC_MCF_MCOL_Pos)
#define GMAC_MCF_MCOL(value)        (GMAC_MCF_MCOL_Msk & ((value) << GMAC_MCF_MCOL_Pos))
#define GMAC_MCF_MASK               _U_(0x0003FFFF) /**< \brief (GMAC_MCF) MASK Register */

/* -------- GMAC_EC : (GMAC Offset: 0x140) (R/  32) Excessive Collisions Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t XCOL:10;          /*!< bit:  0.. 9  Excessive Collisions               */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EC_OFFSET              0x140        /**< \brief (GMAC_EC offset) Excessive Collisions Register */
#define GMAC_EC_RESETVALUE          _U_(0x00000000) /**< \brief (GMAC_EC reset_value) Excessive Collisions Register */

#define GMAC_EC_XCOL_Pos            0            /**< \brief (GMAC_EC) Excessive Collisions */
#define GMAC_EC_XCOL_Msk            (_U_(0x3FF) << GMAC_EC_XCOL_Pos)
#define GMAC_EC_XCOL(value)         (GMAC_EC_XCOL_Msk & ((value) << GMAC_EC_XCOL_Pos))
#define GMAC_EC_MASK                _U_(0x000003FF) /**< \brief (GMAC_EC) MASK Register */

/* -------- GMAC_LC : (GMAC Offset: 0x144) (R/  32) Late Collisions Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LCOL:10;          /*!< bit:  0.. 9  Late Collisions                    */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_LC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_LC_OFFSET              0x144        /**< \brief (GMAC_LC offset) Late Collisions Register */
#define GMAC_LC_RESETVALUE          _U_(0x00000000) /**< \brief (GMAC_LC reset_value) Late Collisions Register */

#define GMAC_LC_LCOL_Pos            0            /**< \brief (GMAC_LC) Late Collisions */
#define GMAC_LC_LCOL_Msk            (_U_(0x3FF) << GMAC_LC_LCOL_Pos)
#define GMAC_LC_LCOL(value)         (GMAC_LC_LCOL_Msk & ((value) << GMAC_LC_LCOL_Pos))
#define GMAC_LC_MASK                _U_(0x000003FF) /**< \brief (GMAC_LC) MASK Register */

/* -------- GMAC_DTF : (GMAC Offset: 0x148) (R/  32) Deferred Transmission Frames Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DEFT:18;          /*!< bit:  0..17  Deferred Transmission              */
    uint32_t :14;              /*!< bit: 18..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_DTF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_DTF_OFFSET             0x148        /**< \brief (GMAC_DTF offset) Deferred Transmission Frames Register */
#define GMAC_DTF_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_DTF reset_value) Deferred Transmission Frames Register */

#define GMAC_DTF_DEFT_Pos           0            /**< \brief (GMAC_DTF) Deferred Transmission */
#define GMAC_DTF_DEFT_Msk           (_U_(0x3FFFF) << GMAC_DTF_DEFT_Pos)
#define GMAC_DTF_DEFT(value)        (GMAC_DTF_DEFT_Msk & ((value) << GMAC_DTF_DEFT_Pos))
#define GMAC_DTF_MASK               _U_(0x0003FFFF) /**< \brief (GMAC_DTF) MASK Register */

/* -------- GMAC_CSE : (GMAC Offset: 0x14C) (R/  32) Carrier Sense Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CSR:10;           /*!< bit:  0.. 9  Carrier Sense Error                */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_CSE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_CSE_OFFSET             0x14C        /**< \brief (GMAC_CSE offset) Carrier Sense Errors Register */
#define GMAC_CSE_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_CSE reset_value) Carrier Sense Errors Register */

#define GMAC_CSE_CSR_Pos            0            /**< \brief (GMAC_CSE) Carrier Sense Error */
#define GMAC_CSE_CSR_Msk            (_U_(0x3FF) << GMAC_CSE_CSR_Pos)
#define GMAC_CSE_CSR(value)         (GMAC_CSE_CSR_Msk & ((value) << GMAC_CSE_CSR_Pos))
#define GMAC_CSE_MASK               _U_(0x000003FF) /**< \brief (GMAC_CSE) MASK Register */

/* -------- GMAC_ORLO : (GMAC Offset: 0x150) (R/  32) Octets Received [31:0] Received -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXO:32;           /*!< bit:  0..31  Received Octets                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_ORLO_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ORLO_OFFSET            0x150        /**< \brief (GMAC_ORLO offset) Octets Received [31:0] Received */
#define GMAC_ORLO_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_ORLO reset_value) Octets Received [31:0] Received */

#define GMAC_ORLO_RXO_Pos           0            /**< \brief (GMAC_ORLO) Received Octets */
#define GMAC_ORLO_RXO_Msk           (_U_(0xFFFFFFFF) << GMAC_ORLO_RXO_Pos)
#define GMAC_ORLO_RXO(value)        (GMAC_ORLO_RXO_Msk & ((value) << GMAC_ORLO_RXO_Pos))
#define GMAC_ORLO_MASK              _U_(0xFFFFFFFF) /**< \brief (GMAC_ORLO) MASK Register */

/* -------- GMAC_ORHI : (GMAC Offset: 0x154) (R/  32) Octets Received [47:32] Received -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXO:16;           /*!< bit:  0..15  Received Octets                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_ORHI_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ORHI_OFFSET            0x154        /**< \brief (GMAC_ORHI offset) Octets Received [47:32] Received */
#define GMAC_ORHI_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_ORHI reset_value) Octets Received [47:32] Received */

#define GMAC_ORHI_RXO_Pos           0            /**< \brief (GMAC_ORHI) Received Octets */
#define GMAC_ORHI_RXO_Msk           (_U_(0xFFFF) << GMAC_ORHI_RXO_Pos)
#define GMAC_ORHI_RXO(value)        (GMAC_ORHI_RXO_Msk & ((value) << GMAC_ORHI_RXO_Pos))
#define GMAC_ORHI_MASK              _U_(0x0000FFFF) /**< \brief (GMAC_ORHI) MASK Register */

/* -------- GMAC_FR : (GMAC Offset: 0x158) (R/  32) Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FRX:32;           /*!< bit:  0..31  Frames Received without Error      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_FR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_FR_OFFSET              0x158        /**< \brief (GMAC_FR offset) Frames Received Register */
#define GMAC_FR_RESETVALUE          _U_(0x00000000) /**< \brief (GMAC_FR reset_value) Frames Received Register */

#define GMAC_FR_FRX_Pos             0            /**< \brief (GMAC_FR) Frames Received without Error */
#define GMAC_FR_FRX_Msk             (_U_(0xFFFFFFFF) << GMAC_FR_FRX_Pos)
#define GMAC_FR_FRX(value)          (GMAC_FR_FRX_Msk & ((value) << GMAC_FR_FRX_Pos))
#define GMAC_FR_MASK                _U_(0xFFFFFFFF) /**< \brief (GMAC_FR) MASK Register */

/* -------- GMAC_BCFR : (GMAC Offset: 0x15C) (R/  32) Broadcast Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BFRX:32;          /*!< bit:  0..31  Broadcast Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_BCFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_BCFR_OFFSET            0x15C        /**< \brief (GMAC_BCFR offset) Broadcast Frames Received Register */
#define GMAC_BCFR_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_BCFR reset_value) Broadcast Frames Received Register */

#define GMAC_BCFR_BFRX_Pos          0            /**< \brief (GMAC_BCFR) Broadcast Frames Received without Error */
#define GMAC_BCFR_BFRX_Msk          (_U_(0xFFFFFFFF) << GMAC_BCFR_BFRX_Pos)
#define GMAC_BCFR_BFRX(value)       (GMAC_BCFR_BFRX_Msk & ((value) << GMAC_BCFR_BFRX_Pos))
#define GMAC_BCFR_MASK              _U_(0xFFFFFFFF) /**< \brief (GMAC_BCFR) MASK Register */

/* -------- GMAC_MFR : (GMAC Offset: 0x160) (R/  32) Multicast Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MFRX:32;          /*!< bit:  0..31  Multicast Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_MFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_MFR_OFFSET             0x160        /**< \brief (GMAC_MFR offset) Multicast Frames Received Register */
#define GMAC_MFR_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_MFR reset_value) Multicast Frames Received Register */

#define GMAC_MFR_MFRX_Pos           0            /**< \brief (GMAC_MFR) Multicast Frames Received without Error */
#define GMAC_MFR_MFRX_Msk           (_U_(0xFFFFFFFF) << GMAC_MFR_MFRX_Pos)
#define GMAC_MFR_MFRX(value)        (GMAC_MFR_MFRX_Msk & ((value) << GMAC_MFR_MFRX_Pos))
#define GMAC_MFR_MASK               _U_(0xFFFFFFFF) /**< \brief (GMAC_MFR) MASK Register */

/* -------- GMAC_PFR : (GMAC Offset: 0x164) (R/  32) Pause Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PFRX:16;          /*!< bit:  0..15  Pause Frames Received Register     */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PFR_OFFSET             0x164        /**< \brief (GMAC_PFR offset) Pause Frames Received Register */
#define GMAC_PFR_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_PFR reset_value) Pause Frames Received Register */

#define GMAC_PFR_PFRX_Pos           0            /**< \brief (GMAC_PFR) Pause Frames Received Register */
#define GMAC_PFR_PFRX_Msk           (_U_(0xFFFF) << GMAC_PFR_PFRX_Pos)
#define GMAC_PFR_PFRX(value)        (GMAC_PFR_PFRX_Msk & ((value) << GMAC_PFR_PFRX_Pos))
#define GMAC_PFR_MASK               _U_(0x0000FFFF) /**< \brief (GMAC_PFR) MASK Register */

/* -------- GMAC_BFR64 : (GMAC Offset: 0x168) (R/  32) 64 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  64 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_BFR64_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_BFR64_OFFSET           0x168        /**< \brief (GMAC_BFR64 offset) 64 Byte Frames Received Register */
#define GMAC_BFR64_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_BFR64 reset_value) 64 Byte Frames Received Register */

#define GMAC_BFR64_NFRX_Pos         0            /**< \brief (GMAC_BFR64) 64 Byte Frames Received without Error */
#define GMAC_BFR64_NFRX_Msk         (_U_(0xFFFFFFFF) << GMAC_BFR64_NFRX_Pos)
#define GMAC_BFR64_NFRX(value)      (GMAC_BFR64_NFRX_Msk & ((value) << GMAC_BFR64_NFRX_Pos))
#define GMAC_BFR64_MASK             _U_(0xFFFFFFFF) /**< \brief (GMAC_BFR64) MASK Register */

/* -------- GMAC_TBFR127 : (GMAC Offset: 0x16C) (R/  32) 65 to 127 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  65 to 127 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFR127_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFR127_OFFSET         0x16C        /**< \brief (GMAC_TBFR127 offset) 65 to 127 Byte Frames Received Register */
#define GMAC_TBFR127_RESETVALUE     _U_(0x00000000) /**< \brief (GMAC_TBFR127 reset_value) 65 to 127 Byte Frames Received Register */

#define GMAC_TBFR127_NFRX_Pos       0            /**< \brief (GMAC_TBFR127) 65 to 127 Byte Frames Received without Error */
#define GMAC_TBFR127_NFRX_Msk       (_U_(0xFFFFFFFF) << GMAC_TBFR127_NFRX_Pos)
#define GMAC_TBFR127_NFRX(value)    (GMAC_TBFR127_NFRX_Msk & ((value) << GMAC_TBFR127_NFRX_Pos))
#define GMAC_TBFR127_MASK           _U_(0xFFFFFFFF) /**< \brief (GMAC_TBFR127) MASK Register */

/* -------- GMAC_TBFR255 : (GMAC Offset: 0x170) (R/  32) 128 to 255 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  128 to 255 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFR255_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFR255_OFFSET         0x170        /**< \brief (GMAC_TBFR255 offset) 128 to 255 Byte Frames Received Register */
#define GMAC_TBFR255_RESETVALUE     _U_(0x00000000) /**< \brief (GMAC_TBFR255 reset_value) 128 to 255 Byte Frames Received Register */

#define GMAC_TBFR255_NFRX_Pos       0            /**< \brief (GMAC_TBFR255) 128 to 255 Byte Frames Received without Error */
#define GMAC_TBFR255_NFRX_Msk       (_U_(0xFFFFFFFF) << GMAC_TBFR255_NFRX_Pos)
#define GMAC_TBFR255_NFRX(value)    (GMAC_TBFR255_NFRX_Msk & ((value) << GMAC_TBFR255_NFRX_Pos))
#define GMAC_TBFR255_MASK           _U_(0xFFFFFFFF) /**< \brief (GMAC_TBFR255) MASK Register */

/* -------- GMAC_TBFR511 : (GMAC Offset: 0x174) (R/  32) 256 to 511Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  256 to 511 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFR511_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFR511_OFFSET         0x174        /**< \brief (GMAC_TBFR511 offset) 256 to 511Byte Frames Received Register */
#define GMAC_TBFR511_RESETVALUE     _U_(0x00000000) /**< \brief (GMAC_TBFR511 reset_value) 256 to 511Byte Frames Received Register */

#define GMAC_TBFR511_NFRX_Pos       0            /**< \brief (GMAC_TBFR511) 256 to 511 Byte Frames Received without Error */
#define GMAC_TBFR511_NFRX_Msk       (_U_(0xFFFFFFFF) << GMAC_TBFR511_NFRX_Pos)
#define GMAC_TBFR511_NFRX(value)    (GMAC_TBFR511_NFRX_Msk & ((value) << GMAC_TBFR511_NFRX_Pos))
#define GMAC_TBFR511_MASK           _U_(0xFFFFFFFF) /**< \brief (GMAC_TBFR511) MASK Register */

/* -------- GMAC_TBFR1023 : (GMAC Offset: 0x178) (R/  32) 512 to 1023 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  512 to 1023 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFR1023_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFR1023_OFFSET        0x178        /**< \brief (GMAC_TBFR1023 offset) 512 to 1023 Byte Frames Received Register */
#define GMAC_TBFR1023_RESETVALUE    _U_(0x00000000) /**< \brief (GMAC_TBFR1023 reset_value) 512 to 1023 Byte Frames Received Register */

#define GMAC_TBFR1023_NFRX_Pos      0            /**< \brief (GMAC_TBFR1023) 512 to 1023 Byte Frames Received without Error */
#define GMAC_TBFR1023_NFRX_Msk      (_U_(0xFFFFFFFF) << GMAC_TBFR1023_NFRX_Pos)
#define GMAC_TBFR1023_NFRX(value)   (GMAC_TBFR1023_NFRX_Msk & ((value) << GMAC_TBFR1023_NFRX_Pos))
#define GMAC_TBFR1023_MASK          _U_(0xFFFFFFFF) /**< \brief (GMAC_TBFR1023) MASK Register */

/* -------- GMAC_TBFR1518 : (GMAC Offset: 0x17C) (R/  32) 1024 to 1518 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  1024 to 1518 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFR1518_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TBFR1518_OFFSET        0x17C        /**< \brief (GMAC_TBFR1518 offset) 1024 to 1518 Byte Frames Received Register */
#define GMAC_TBFR1518_RESETVALUE    _U_(0x00000000) /**< \brief (GMAC_TBFR1518 reset_value) 1024 to 1518 Byte Frames Received Register */

#define GMAC_TBFR1518_NFRX_Pos      0            /**< \brief (GMAC_TBFR1518) 1024 to 1518 Byte Frames Received without Error */
#define GMAC_TBFR1518_NFRX_Msk      (_U_(0xFFFFFFFF) << GMAC_TBFR1518_NFRX_Pos)
#define GMAC_TBFR1518_NFRX(value)   (GMAC_TBFR1518_NFRX_Msk & ((value) << GMAC_TBFR1518_NFRX_Pos))
#define GMAC_TBFR1518_MASK          _U_(0xFFFFFFFF) /**< \brief (GMAC_TBFR1518) MASK Register */

/* -------- GMAC_TMXBFR : (GMAC Offset: 0x180) (R/  32) 1519 to Maximum Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  1519 to Maximum Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TMXBFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TMXBFR_OFFSET          0x180        /**< \brief (GMAC_TMXBFR offset) 1519 to Maximum Byte Frames Received Register */
#define GMAC_TMXBFR_RESETVALUE      _U_(0x00000000) /**< \brief (GMAC_TMXBFR reset_value) 1519 to Maximum Byte Frames Received Register */

#define GMAC_TMXBFR_NFRX_Pos        0            /**< \brief (GMAC_TMXBFR) 1519 to Maximum Byte Frames Received without Error */
#define GMAC_TMXBFR_NFRX_Msk        (_U_(0xFFFFFFFF) << GMAC_TMXBFR_NFRX_Pos)
#define GMAC_TMXBFR_NFRX(value)     (GMAC_TMXBFR_NFRX_Msk & ((value) << GMAC_TMXBFR_NFRX_Pos))
#define GMAC_TMXBFR_MASK            _U_(0xFFFFFFFF) /**< \brief (GMAC_TMXBFR) MASK Register */

/* -------- GMAC_UFR : (GMAC Offset: 0x184) (R/  32) Undersize Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t UFRX:10;          /*!< bit:  0.. 9  Undersize Frames Received          */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_UFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_UFR_OFFSET             0x184        /**< \brief (GMAC_UFR offset) Undersize Frames Received Register */
#define GMAC_UFR_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_UFR reset_value) Undersize Frames Received Register */

#define GMAC_UFR_UFRX_Pos           0            /**< \brief (GMAC_UFR) Undersize Frames Received */
#define GMAC_UFR_UFRX_Msk           (_U_(0x3FF) << GMAC_UFR_UFRX_Pos)
#define GMAC_UFR_UFRX(value)        (GMAC_UFR_UFRX_Msk & ((value) << GMAC_UFR_UFRX_Pos))
#define GMAC_UFR_MASK               _U_(0x000003FF) /**< \brief (GMAC_UFR) MASK Register */

/* -------- GMAC_OFR : (GMAC Offset: 0x188) (R/  32) Oversize Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t OFRX:10;          /*!< bit:  0.. 9  Oversized Frames Received          */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_OFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_OFR_OFFSET             0x188        /**< \brief (GMAC_OFR offset) Oversize Frames Received Register */
#define GMAC_OFR_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_OFR reset_value) Oversize Frames Received Register */

#define GMAC_OFR_OFRX_Pos           0            /**< \brief (GMAC_OFR) Oversized Frames Received */
#define GMAC_OFR_OFRX_Msk           (_U_(0x3FF) << GMAC_OFR_OFRX_Pos)
#define GMAC_OFR_OFRX(value)        (GMAC_OFR_OFRX_Msk & ((value) << GMAC_OFR_OFRX_Pos))
#define GMAC_OFR_MASK               _U_(0x000003FF) /**< \brief (GMAC_OFR) MASK Register */

/* -------- GMAC_JR : (GMAC Offset: 0x18C) (R/  32) Jabbers Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t JRX:10;           /*!< bit:  0.. 9  Jabbers Received                   */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_JR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_JR_OFFSET              0x18C        /**< \brief (GMAC_JR offset) Jabbers Received Register */
#define GMAC_JR_RESETVALUE          _U_(0x00000000) /**< \brief (GMAC_JR reset_value) Jabbers Received Register */

#define GMAC_JR_JRX_Pos             0            /**< \brief (GMAC_JR) Jabbers Received */
#define GMAC_JR_JRX_Msk             (_U_(0x3FF) << GMAC_JR_JRX_Pos)
#define GMAC_JR_JRX(value)          (GMAC_JR_JRX_Msk & ((value) << GMAC_JR_JRX_Pos))
#define GMAC_JR_MASK                _U_(0x000003FF) /**< \brief (GMAC_JR) MASK Register */

/* -------- GMAC_FCSE : (GMAC Offset: 0x190) (R/  32) Frame Check Sequence Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FCKR:10;          /*!< bit:  0.. 9  Frame Check Sequence Errors        */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_FCSE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_FCSE_OFFSET            0x190        /**< \brief (GMAC_FCSE offset) Frame Check Sequence Errors Register */
#define GMAC_FCSE_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_FCSE reset_value) Frame Check Sequence Errors Register */

#define GMAC_FCSE_FCKR_Pos          0            /**< \brief (GMAC_FCSE) Frame Check Sequence Errors */
#define GMAC_FCSE_FCKR_Msk          (_U_(0x3FF) << GMAC_FCSE_FCKR_Pos)
#define GMAC_FCSE_FCKR(value)       (GMAC_FCSE_FCKR_Msk & ((value) << GMAC_FCSE_FCKR_Pos))
#define GMAC_FCSE_MASK              _U_(0x000003FF) /**< \brief (GMAC_FCSE) MASK Register */

/* -------- GMAC_LFFE : (GMAC Offset: 0x194) (R/  32) Length Field Frame Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LFER:10;          /*!< bit:  0.. 9  Length Field Frame Errors          */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_LFFE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_LFFE_OFFSET            0x194        /**< \brief (GMAC_LFFE offset) Length Field Frame Errors Register */
#define GMAC_LFFE_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_LFFE reset_value) Length Field Frame Errors Register */

#define GMAC_LFFE_LFER_Pos          0            /**< \brief (GMAC_LFFE) Length Field Frame Errors */
#define GMAC_LFFE_LFER_Msk          (_U_(0x3FF) << GMAC_LFFE_LFER_Pos)
#define GMAC_LFFE_LFER(value)       (GMAC_LFFE_LFER_Msk & ((value) << GMAC_LFFE_LFER_Pos))
#define GMAC_LFFE_MASK              _U_(0x000003FF) /**< \brief (GMAC_LFFE) MASK Register */

/* -------- GMAC_RSE : (GMAC Offset: 0x198) (R/  32) Receive Symbol Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXSE:10;          /*!< bit:  0.. 9  Receive Symbol Errors              */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RSE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RSE_OFFSET             0x198        /**< \brief (GMAC_RSE offset) Receive Symbol Errors Register */
#define GMAC_RSE_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_RSE reset_value) Receive Symbol Errors Register */

#define GMAC_RSE_RXSE_Pos           0            /**< \brief (GMAC_RSE) Receive Symbol Errors */
#define GMAC_RSE_RXSE_Msk           (_U_(0x3FF) << GMAC_RSE_RXSE_Pos)
#define GMAC_RSE_RXSE(value)        (GMAC_RSE_RXSE_Msk & ((value) << GMAC_RSE_RXSE_Pos))
#define GMAC_RSE_MASK               _U_(0x000003FF) /**< \brief (GMAC_RSE) MASK Register */

/* -------- GMAC_AE : (GMAC Offset: 0x19C) (R/  32) Alignment Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t AER:10;           /*!< bit:  0.. 9  Alignment Errors                   */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_AE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_AE_OFFSET              0x19C        /**< \brief (GMAC_AE offset) Alignment Errors Register */
#define GMAC_AE_RESETVALUE          _U_(0x00000000) /**< \brief (GMAC_AE reset_value) Alignment Errors Register */

#define GMAC_AE_AER_Pos             0            /**< \brief (GMAC_AE) Alignment Errors */
#define GMAC_AE_AER_Msk             (_U_(0x3FF) << GMAC_AE_AER_Pos)
#define GMAC_AE_AER(value)          (GMAC_AE_AER_Msk & ((value) << GMAC_AE_AER_Pos))
#define GMAC_AE_MASK                _U_(0x000003FF) /**< \brief (GMAC_AE) MASK Register */

/* -------- GMAC_RRE : (GMAC Offset: 0x1A0) (R/  32) Receive Resource Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXRER:18;         /*!< bit:  0..17  Receive Resource Errors            */
    uint32_t :14;              /*!< bit: 18..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RRE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RRE_OFFSET             0x1A0        /**< \brief (GMAC_RRE offset) Receive Resource Errors Register */
#define GMAC_RRE_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_RRE reset_value) Receive Resource Errors Register */

#define GMAC_RRE_RXRER_Pos          0            /**< \brief (GMAC_RRE) Receive Resource Errors */
#define GMAC_RRE_RXRER_Msk          (_U_(0x3FFFF) << GMAC_RRE_RXRER_Pos)
#define GMAC_RRE_RXRER(value)       (GMAC_RRE_RXRER_Msk & ((value) << GMAC_RRE_RXRER_Pos))
#define GMAC_RRE_MASK               _U_(0x0003FFFF) /**< \brief (GMAC_RRE) MASK Register */

/* -------- GMAC_ROE : (GMAC Offset: 0x1A4) (R/  32) Receive Overrun Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXOVR:10;         /*!< bit:  0.. 9  Receive Overruns                   */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_ROE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_ROE_OFFSET             0x1A4        /**< \brief (GMAC_ROE offset) Receive Overrun Register */
#define GMAC_ROE_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_ROE reset_value) Receive Overrun Register */

#define GMAC_ROE_RXOVR_Pos          0            /**< \brief (GMAC_ROE) Receive Overruns */
#define GMAC_ROE_RXOVR_Msk          (_U_(0x3FF) << GMAC_ROE_RXOVR_Pos)
#define GMAC_ROE_RXOVR(value)       (GMAC_ROE_RXOVR_Msk & ((value) << GMAC_ROE_RXOVR_Pos))
#define GMAC_ROE_MASK               _U_(0x000003FF) /**< \brief (GMAC_ROE) MASK Register */

/* -------- GMAC_IHCE : (GMAC Offset: 0x1A8) (R/  32) IP Header Checksum Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t HCKER:8;          /*!< bit:  0.. 7  IP Header Checksum Errors          */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_IHCE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_IHCE_OFFSET            0x1A8        /**< \brief (GMAC_IHCE offset) IP Header Checksum Errors Register */
#define GMAC_IHCE_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_IHCE reset_value) IP Header Checksum Errors Register */

#define GMAC_IHCE_HCKER_Pos         0            /**< \brief (GMAC_IHCE) IP Header Checksum Errors */
#define GMAC_IHCE_HCKER_Msk         (_U_(0xFF) << GMAC_IHCE_HCKER_Pos)
#define GMAC_IHCE_HCKER(value)      (GMAC_IHCE_HCKER_Msk & ((value) << GMAC_IHCE_HCKER_Pos))
#define GMAC_IHCE_MASK              _U_(0x000000FF) /**< \brief (GMAC_IHCE) MASK Register */

/* -------- GMAC_TCE : (GMAC Offset: 0x1AC) (R/  32) TCP Checksum Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TCKER:8;          /*!< bit:  0.. 7  TCP Checksum Errors                */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TCE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TCE_OFFSET             0x1AC        /**< \brief (GMAC_TCE offset) TCP Checksum Errors Register */
#define GMAC_TCE_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_TCE reset_value) TCP Checksum Errors Register */

#define GMAC_TCE_TCKER_Pos          0            /**< \brief (GMAC_TCE) TCP Checksum Errors */
#define GMAC_TCE_TCKER_Msk          (_U_(0xFF) << GMAC_TCE_TCKER_Pos)
#define GMAC_TCE_TCKER(value)       (GMAC_TCE_TCKER_Msk & ((value) << GMAC_TCE_TCKER_Pos))
#define GMAC_TCE_MASK               _U_(0x000000FF) /**< \brief (GMAC_TCE) MASK Register */

/* -------- GMAC_UCE : (GMAC Offset: 0x1B0) (R/  32) UDP Checksum Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t UCKER:8;          /*!< bit:  0.. 7  UDP Checksum Errors                */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_UCE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_UCE_OFFSET             0x1B0        /**< \brief (GMAC_UCE offset) UDP Checksum Errors Register */
#define GMAC_UCE_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_UCE reset_value) UDP Checksum Errors Register */

#define GMAC_UCE_UCKER_Pos          0            /**< \brief (GMAC_UCE) UDP Checksum Errors */
#define GMAC_UCE_UCKER_Msk          (_U_(0xFF) << GMAC_UCE_UCKER_Pos)
#define GMAC_UCE_UCKER(value)       (GMAC_UCE_UCKER_Msk & ((value) << GMAC_UCE_UCKER_Pos))
#define GMAC_UCE_MASK               _U_(0x000000FF) /**< \brief (GMAC_UCE) MASK Register */

/* -------- GMAC_TISUBN : (GMAC Offset: 0x1BC) (R/W 32) 1588 Timer Increment [15:0] Sub-Nanoseconds Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LSBTIR:16;        /*!< bit:  0..15  Lower Significant Bits of Timer Increment */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TISUBN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TISUBN_OFFSET          0x1BC        /**< \brief (GMAC_TISUBN offset) 1588 Timer Increment [15:0] Sub-Nanoseconds Register */
#define GMAC_TISUBN_RESETVALUE      _U_(0x00000000) /**< \brief (GMAC_TISUBN reset_value) 1588 Timer Increment [15:0] Sub-Nanoseconds Register */

#define GMAC_TISUBN_LSBTIR_Pos      0            /**< \brief (GMAC_TISUBN) Lower Significant Bits of Timer Increment */
#define GMAC_TISUBN_LSBTIR_Msk      (_U_(0xFFFF) << GMAC_TISUBN_LSBTIR_Pos)
#define GMAC_TISUBN_LSBTIR(value)   (GMAC_TISUBN_LSBTIR_Msk & ((value) << GMAC_TISUBN_LSBTIR_Pos))
#define GMAC_TISUBN_MASK            _U_(0x0000FFFF) /**< \brief (GMAC_TISUBN) MASK Register */

/* -------- GMAC_TSH : (GMAC Offset: 0x1C0) (R/W 32) 1588 Timer Seconds High [15:0] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TCS:16;           /*!< bit:  0..15  Timer Count in Seconds             */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TSH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TSH_OFFSET             0x1C0        /**< \brief (GMAC_TSH offset) 1588 Timer Seconds High [15:0] Register */
#define GMAC_TSH_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_TSH reset_value) 1588 Timer Seconds High [15:0] Register */

#define GMAC_TSH_TCS_Pos            0            /**< \brief (GMAC_TSH) Timer Count in Seconds */
#define GMAC_TSH_TCS_Msk            (_U_(0xFFFF) << GMAC_TSH_TCS_Pos)
#define GMAC_TSH_TCS(value)         (GMAC_TSH_TCS_Msk & ((value) << GMAC_TSH_TCS_Pos))
#define GMAC_TSH_MASK               _U_(0x0000FFFF) /**< \brief (GMAC_TSH) MASK Register */

/* -------- GMAC_TSSSL : (GMAC Offset: 0x1C8) (R/W 32) 1588 Timer Sync Strobe Seconds [31:0] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t VTS:32;           /*!< bit:  0..31  Value of Timer Seconds Register Capture */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TSSSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TSSSL_OFFSET           0x1C8        /**< \brief (GMAC_TSSSL offset) 1588 Timer Sync Strobe Seconds [31:0] Register */
#define GMAC_TSSSL_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_TSSSL reset_value) 1588 Timer Sync Strobe Seconds [31:0] Register */

#define GMAC_TSSSL_VTS_Pos          0            /**< \brief (GMAC_TSSSL) Value of Timer Seconds Register Capture */
#define GMAC_TSSSL_VTS_Msk          (_U_(0xFFFFFFFF) << GMAC_TSSSL_VTS_Pos)
#define GMAC_TSSSL_VTS(value)       (GMAC_TSSSL_VTS_Msk & ((value) << GMAC_TSSSL_VTS_Pos))
#define GMAC_TSSSL_MASK             _U_(0xFFFFFFFF) /**< \brief (GMAC_TSSSL) MASK Register */

/* -------- GMAC_TSSN : (GMAC Offset: 0x1CC) (R/W 32) 1588 Timer Sync Strobe Nanoseconds Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t VTN:30;           /*!< bit:  0..29  Value Timer Nanoseconds Register Capture */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TSSN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TSSN_OFFSET            0x1CC        /**< \brief (GMAC_TSSN offset) 1588 Timer Sync Strobe Nanoseconds Register */
#define GMAC_TSSN_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_TSSN reset_value) 1588 Timer Sync Strobe Nanoseconds Register */

#define GMAC_TSSN_VTN_Pos           0            /**< \brief (GMAC_TSSN) Value Timer Nanoseconds Register Capture */
#define GMAC_TSSN_VTN_Msk           (_U_(0x3FFFFFFF) << GMAC_TSSN_VTN_Pos)
#define GMAC_TSSN_VTN(value)        (GMAC_TSSN_VTN_Msk & ((value) << GMAC_TSSN_VTN_Pos))
#define GMAC_TSSN_MASK              _U_(0x3FFFFFFF) /**< \brief (GMAC_TSSN) MASK Register */

/* -------- GMAC_TSL : (GMAC Offset: 0x1D0) (R/W 32) 1588 Timer Seconds [31:0] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TCS:32;           /*!< bit:  0..31  Timer Count in Seconds             */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TSL_OFFSET             0x1D0        /**< \brief (GMAC_TSL offset) 1588 Timer Seconds [31:0] Register */
#define GMAC_TSL_RESETVALUE         _U_(0x00000000) /**< \brief (GMAC_TSL reset_value) 1588 Timer Seconds [31:0] Register */

#define GMAC_TSL_TCS_Pos            0            /**< \brief (GMAC_TSL) Timer Count in Seconds */
#define GMAC_TSL_TCS_Msk            (_U_(0xFFFFFFFF) << GMAC_TSL_TCS_Pos)
#define GMAC_TSL_TCS(value)         (GMAC_TSL_TCS_Msk & ((value) << GMAC_TSL_TCS_Pos))
#define GMAC_TSL_MASK               _U_(0xFFFFFFFF) /**< \brief (GMAC_TSL) MASK Register */

/* -------- GMAC_TN : (GMAC Offset: 0x1D4) (R/W 32) 1588 Timer Nanoseconds Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TNS:30;           /*!< bit:  0..29  Timer Count in Nanoseconds         */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TN_OFFSET              0x1D4        /**< \brief (GMAC_TN offset) 1588 Timer Nanoseconds Register */
#define GMAC_TN_RESETVALUE          _U_(0x00000000) /**< \brief (GMAC_TN reset_value) 1588 Timer Nanoseconds Register */

#define GMAC_TN_TNS_Pos             0            /**< \brief (GMAC_TN) Timer Count in Nanoseconds */
#define GMAC_TN_TNS_Msk             (_U_(0x3FFFFFFF) << GMAC_TN_TNS_Pos)
#define GMAC_TN_TNS(value)          (GMAC_TN_TNS_Msk & ((value) << GMAC_TN_TNS_Pos))
#define GMAC_TN_MASK                _U_(0x3FFFFFFF) /**< \brief (GMAC_TN) MASK Register */

/* -------- GMAC_TA : (GMAC Offset: 0x1D8) ( /W 32) 1588 Timer Adjust Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ITDT:30;          /*!< bit:  0..29  Increment/Decrement                */
    uint32_t :1;               /*!< bit:     30  Reserved                           */
    uint32_t ADJ:1;            /*!< bit:     31  Adjust 1588 Timer                  */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TA_OFFSET              0x1D8        /**< \brief (GMAC_TA offset) 1588 Timer Adjust Register */
#define GMAC_TA_RESETVALUE          _U_(0x00000000) /**< \brief (GMAC_TA reset_value) 1588 Timer Adjust Register */

#define GMAC_TA_ITDT_Pos            0            /**< \brief (GMAC_TA) Increment/Decrement */
#define GMAC_TA_ITDT_Msk            (_U_(0x3FFFFFFF) << GMAC_TA_ITDT_Pos)
#define GMAC_TA_ITDT(value)         (GMAC_TA_ITDT_Msk & ((value) << GMAC_TA_ITDT_Pos))
#define GMAC_TA_ADJ_Pos             31           /**< \brief (GMAC_TA) Adjust 1588 Timer */
#define GMAC_TA_ADJ                 (_U_(0x1) << GMAC_TA_ADJ_Pos)
#define GMAC_TA_MASK                _U_(0xBFFFFFFF) /**< \brief (GMAC_TA) MASK Register */

/* -------- GMAC_TI : (GMAC Offset: 0x1DC) (R/W 32) 1588 Timer Increment Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CNS:8;            /*!< bit:  0.. 7  Count Nanoseconds                  */
    uint32_t ACNS:8;           /*!< bit:  8..15  Alternative Count Nanoseconds      */
    uint32_t NIT:8;            /*!< bit: 16..23  Number of Increments               */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TI_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TI_OFFSET              0x1DC        /**< \brief (GMAC_TI offset) 1588 Timer Increment Register */
#define GMAC_TI_RESETVALUE          _U_(0x00000000) /**< \brief (GMAC_TI reset_value) 1588 Timer Increment Register */

#define GMAC_TI_CNS_Pos             0            /**< \brief (GMAC_TI) Count Nanoseconds */
#define GMAC_TI_CNS_Msk             (_U_(0xFF) << GMAC_TI_CNS_Pos)
#define GMAC_TI_CNS(value)          (GMAC_TI_CNS_Msk & ((value) << GMAC_TI_CNS_Pos))
#define GMAC_TI_ACNS_Pos            8            /**< \brief (GMAC_TI) Alternative Count Nanoseconds */
#define GMAC_TI_ACNS_Msk            (_U_(0xFF) << GMAC_TI_ACNS_Pos)
#define GMAC_TI_ACNS(value)         (GMAC_TI_ACNS_Msk & ((value) << GMAC_TI_ACNS_Pos))
#define GMAC_TI_NIT_Pos             16           /**< \brief (GMAC_TI) Number of Increments */
#define GMAC_TI_NIT_Msk             (_U_(0xFF) << GMAC_TI_NIT_Pos)
#define GMAC_TI_NIT(value)          (GMAC_TI_NIT_Msk & ((value) << GMAC_TI_NIT_Pos))
#define GMAC_TI_MASK                _U_(0x00FFFFFF) /**< \brief (GMAC_TI) MASK Register */

/* -------- GMAC_EFTSL : (GMAC Offset: 0x1E0) (R/  32) PTP Event Frame Transmitted Seconds Low Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:32;           /*!< bit:  0..31  Register Update                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFTSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFTSL_OFFSET           0x1E0        /**< \brief (GMAC_EFTSL offset) PTP Event Frame Transmitted Seconds Low Register */
#define GMAC_EFTSL_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_EFTSL reset_value) PTP Event Frame Transmitted Seconds Low Register */

#define GMAC_EFTSL_RUD_Pos          0            /**< \brief (GMAC_EFTSL) Register Update */
#define GMAC_EFTSL_RUD_Msk          (_U_(0xFFFFFFFF) << GMAC_EFTSL_RUD_Pos)
#define GMAC_EFTSL_RUD(value)       (GMAC_EFTSL_RUD_Msk & ((value) << GMAC_EFTSL_RUD_Pos))
#define GMAC_EFTSL_MASK             _U_(0xFFFFFFFF) /**< \brief (GMAC_EFTSL) MASK Register */

/* -------- GMAC_EFTN : (GMAC Offset: 0x1E4) (R/  32) PTP Event Frame Transmitted Nanoseconds -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:30;           /*!< bit:  0..29  Register Update                    */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFTN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFTN_OFFSET            0x1E4        /**< \brief (GMAC_EFTN offset) PTP Event Frame Transmitted Nanoseconds */
#define GMAC_EFTN_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_EFTN reset_value) PTP Event Frame Transmitted Nanoseconds */

#define GMAC_EFTN_RUD_Pos           0            /**< \brief (GMAC_EFTN) Register Update */
#define GMAC_EFTN_RUD_Msk           (_U_(0x3FFFFFFF) << GMAC_EFTN_RUD_Pos)
#define GMAC_EFTN_RUD(value)        (GMAC_EFTN_RUD_Msk & ((value) << GMAC_EFTN_RUD_Pos))
#define GMAC_EFTN_MASK              _U_(0x3FFFFFFF) /**< \brief (GMAC_EFTN) MASK Register */

/* -------- GMAC_EFRSL : (GMAC Offset: 0x1E8) (R/  32) PTP Event Frame Received Seconds Low Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:32;           /*!< bit:  0..31  Register Update                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFRSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFRSL_OFFSET           0x1E8        /**< \brief (GMAC_EFRSL offset) PTP Event Frame Received Seconds Low Register */
#define GMAC_EFRSL_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_EFRSL reset_value) PTP Event Frame Received Seconds Low Register */

#define GMAC_EFRSL_RUD_Pos          0            /**< \brief (GMAC_EFRSL) Register Update */
#define GMAC_EFRSL_RUD_Msk          (_U_(0xFFFFFFFF) << GMAC_EFRSL_RUD_Pos)
#define GMAC_EFRSL_RUD(value)       (GMAC_EFRSL_RUD_Msk & ((value) << GMAC_EFRSL_RUD_Pos))
#define GMAC_EFRSL_MASK             _U_(0xFFFFFFFF) /**< \brief (GMAC_EFRSL) MASK Register */

/* -------- GMAC_EFRN : (GMAC Offset: 0x1EC) (R/  32) PTP Event Frame Received Nanoseconds -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:30;           /*!< bit:  0..29  Register Update                    */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFRN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_EFRN_OFFSET            0x1EC        /**< \brief (GMAC_EFRN offset) PTP Event Frame Received Nanoseconds */
#define GMAC_EFRN_RESETVALUE        _U_(0x00000000) /**< \brief (GMAC_EFRN reset_value) PTP Event Frame Received Nanoseconds */

#define GMAC_EFRN_RUD_Pos           0            /**< \brief (GMAC_EFRN) Register Update */
#define GMAC_EFRN_RUD_Msk           (_U_(0x3FFFFFFF) << GMAC_EFRN_RUD_Pos)
#define GMAC_EFRN_RUD(value)        (GMAC_EFRN_RUD_Msk & ((value) << GMAC_EFRN_RUD_Pos))
#define GMAC_EFRN_MASK              _U_(0x3FFFFFFF) /**< \brief (GMAC_EFRN) MASK Register */

/* -------- GMAC_PEFTSL : (GMAC Offset: 0x1F0) (R/  32) PTP Peer Event Frame Transmitted Seconds Low Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:32;           /*!< bit:  0..31  Register Update                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFTSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFTSL_OFFSET          0x1F0        /**< \brief (GMAC_PEFTSL offset) PTP Peer Event Frame Transmitted Seconds Low Register */
#define GMAC_PEFTSL_RESETVALUE      _U_(0x00000000) /**< \brief (GMAC_PEFTSL reset_value) PTP Peer Event Frame Transmitted Seconds Low Register */

#define GMAC_PEFTSL_RUD_Pos         0            /**< \brief (GMAC_PEFTSL) Register Update */
#define GMAC_PEFTSL_RUD_Msk         (_U_(0xFFFFFFFF) << GMAC_PEFTSL_RUD_Pos)
#define GMAC_PEFTSL_RUD(value)      (GMAC_PEFTSL_RUD_Msk & ((value) << GMAC_PEFTSL_RUD_Pos))
#define GMAC_PEFTSL_MASK            _U_(0xFFFFFFFF) /**< \brief (GMAC_PEFTSL) MASK Register */

/* -------- GMAC_PEFTN : (GMAC Offset: 0x1F4) (R/  32) PTP Peer Event Frame Transmitted Nanoseconds -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:30;           /*!< bit:  0..29  Register Update                    */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFTN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFTN_OFFSET           0x1F4        /**< \brief (GMAC_PEFTN offset) PTP Peer Event Frame Transmitted Nanoseconds */
#define GMAC_PEFTN_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_PEFTN reset_value) PTP Peer Event Frame Transmitted Nanoseconds */

#define GMAC_PEFTN_RUD_Pos          0            /**< \brief (GMAC_PEFTN) Register Update */
#define GMAC_PEFTN_RUD_Msk          (_U_(0x3FFFFFFF) << GMAC_PEFTN_RUD_Pos)
#define GMAC_PEFTN_RUD(value)       (GMAC_PEFTN_RUD_Msk & ((value) << GMAC_PEFTN_RUD_Pos))
#define GMAC_PEFTN_MASK             _U_(0x3FFFFFFF) /**< \brief (GMAC_PEFTN) MASK Register */

/* -------- GMAC_PEFRSL : (GMAC Offset: 0x1F8) (R/  32) PTP Peer Event Frame Received Seconds Low Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:32;           /*!< bit:  0..31  Register Update                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFRSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFRSL_OFFSET          0x1F8        /**< \brief (GMAC_PEFRSL offset) PTP Peer Event Frame Received Seconds Low Register */
#define GMAC_PEFRSL_RESETVALUE      _U_(0x00000000) /**< \brief (GMAC_PEFRSL reset_value) PTP Peer Event Frame Received Seconds Low Register */

#define GMAC_PEFRSL_RUD_Pos         0            /**< \brief (GMAC_PEFRSL) Register Update */
#define GMAC_PEFRSL_RUD_Msk         (_U_(0xFFFFFFFF) << GMAC_PEFRSL_RUD_Pos)
#define GMAC_PEFRSL_RUD(value)      (GMAC_PEFRSL_RUD_Msk & ((value) << GMAC_PEFRSL_RUD_Pos))
#define GMAC_PEFRSL_MASK            _U_(0xFFFFFFFF) /**< \brief (GMAC_PEFRSL) MASK Register */

/* -------- GMAC_PEFRN : (GMAC Offset: 0x1FC) (R/  32) PTP Peer Event Frame Received Nanoseconds -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:30;           /*!< bit:  0..29  Register Update                    */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFRN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_PEFRN_OFFSET           0x1FC        /**< \brief (GMAC_PEFRN offset) PTP Peer Event Frame Received Nanoseconds */
#define GMAC_PEFRN_RESETVALUE       _U_(0x00000000) /**< \brief (GMAC_PEFRN reset_value) PTP Peer Event Frame Received Nanoseconds */

#define GMAC_PEFRN_RUD_Pos          0            /**< \brief (GMAC_PEFRN) Register Update */
#define GMAC_PEFRN_RUD_Msk          (_U_(0x3FFFFFFF) << GMAC_PEFRN_RUD_Pos)
#define GMAC_PEFRN_RUD(value)       (GMAC_PEFRN_RUD_Msk & ((value) << GMAC_PEFRN_RUD_Pos))
#define GMAC_PEFRN_MASK             _U_(0x3FFFFFFF) /**< \brief (GMAC_PEFRN) MASK Register */

/* -------- GMAC_RLPITR : (GMAC Offset: 0x270) (R/  32) Receive LPI transition Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RLPITR:16;        /*!< bit:  0..15  Count number of times transition from rx normal idle to low power idle */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RLPITR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RLPITR_OFFSET          0x270        /**< \brief (GMAC_RLPITR offset) Receive LPI transition Register */
#define GMAC_RLPITR_RESETVALUE      _U_(0x00000000) /**< \brief (GMAC_RLPITR reset_value) Receive LPI transition Register */

#define GMAC_RLPITR_RLPITR_Pos      0            /**< \brief (GMAC_RLPITR) Count number of times transition from rx normal idle to low power idle */
#define GMAC_RLPITR_RLPITR_Msk      (_U_(0xFFFF) << GMAC_RLPITR_RLPITR_Pos)
#define GMAC_RLPITR_RLPITR(value)   (GMAC_RLPITR_RLPITR_Msk & ((value) << GMAC_RLPITR_RLPITR_Pos))
#define GMAC_RLPITR_MASK            _U_(0x0000FFFF) /**< \brief (GMAC_RLPITR) MASK Register */

/* -------- GMAC_RLPITI : (GMAC Offset: 0x274) (R/  32) Receive LPI Time Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RLPITI:24;        /*!< bit:  0..23  Increment once over 16 ahb clock when LPI indication bit 20 is set in rx mode */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RLPITI_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_RLPITI_OFFSET          0x274        /**< \brief (GMAC_RLPITI offset) Receive LPI Time Register */
#define GMAC_RLPITI_RESETVALUE      _U_(0x00000000) /**< \brief (GMAC_RLPITI reset_value) Receive LPI Time Register */

#define GMAC_RLPITI_RLPITI_Pos      0            /**< \brief (GMAC_RLPITI) Increment once over 16 ahb clock when LPI indication bit 20 is set in rx mode */
#define GMAC_RLPITI_RLPITI_Msk      (_U_(0xFFFFFF) << GMAC_RLPITI_RLPITI_Pos)
#define GMAC_RLPITI_RLPITI(value)   (GMAC_RLPITI_RLPITI_Msk & ((value) << GMAC_RLPITI_RLPITI_Pos))
#define GMAC_RLPITI_MASK            _U_(0x00FFFFFF) /**< \brief (GMAC_RLPITI) MASK Register */

/* -------- GMAC_TLPITR : (GMAC Offset: 0x278) (R/  32) Receive LPI transition Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TLPITR:16;        /*!< bit:  0..15  Count number of times enable LPI tx bit 20 goes from low to high */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TLPITR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TLPITR_OFFSET          0x278        /**< \brief (GMAC_TLPITR offset) Receive LPI transition Register */
#define GMAC_TLPITR_RESETVALUE      _U_(0x00000000) /**< \brief (GMAC_TLPITR reset_value) Receive LPI transition Register */

#define GMAC_TLPITR_TLPITR_Pos      0            /**< \brief (GMAC_TLPITR) Count number of times enable LPI tx bit 20 goes from low to high */
#define GMAC_TLPITR_TLPITR_Msk      (_U_(0xFFFF) << GMAC_TLPITR_TLPITR_Pos)
#define GMAC_TLPITR_TLPITR(value)   (GMAC_TLPITR_TLPITR_Msk & ((value) << GMAC_TLPITR_TLPITR_Pos))
#define GMAC_TLPITR_MASK            _U_(0x0000FFFF) /**< \brief (GMAC_TLPITR) MASK Register */

/* -------- GMAC_TLPITI : (GMAC Offset: 0x27C) (R/  32) Receive LPI Time Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TLPITI:24;        /*!< bit:  0..23  Increment once over 16 ahb clock when LPI indication bit 20 is set in tx mode */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TLPITI_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define GMAC_TLPITI_OFFSET          0x27C        /**< \brief (GMAC_TLPITI offset) Receive LPI Time Register */
#define GMAC_TLPITI_RESETVALUE      _U_(0x00000000) /**< \brief (GMAC_TLPITI reset_value) Receive LPI Time Register */

#define GMAC_TLPITI_TLPITI_Pos      0            /**< \brief (GMAC_TLPITI) Increment once over 16 ahb clock when LPI indication bit 20 is set in tx mode */
#define GMAC_TLPITI_TLPITI_Msk      (_U_(0xFFFFFF) << GMAC_TLPITI_TLPITI_Pos)
#define GMAC_TLPITI_TLPITI(value)   (GMAC_TLPITI_TLPITI_Msk & ((value) << GMAC_TLPITI_TLPITI_Pos))
#define GMAC_TLPITI_MASK            _U_(0x00FFFFFF) /**< \brief (GMAC_TLPITI) MASK Register */

/** \brief GmacSa hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO GMAC_SAB_Type             SAB;         /**< \brief Offset: 0x000 (R/W 32) Specific Address Bottom [31:0] Register */
  __IO GMAC_SAT_Type             SAT;         /**< \brief Offset: 0x004 (R/W 32) Specific Address Top [47:32] Register */
} GmacSa;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief GMAC hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO GMAC_NCR_Type             NCR;         /**< \brief Offset: 0x000 (R/W 32) Network Control Register */
  __IO GMAC_NCFGR_Type           NCFGR;       /**< \brief Offset: 0x004 (R/W 32) Network Configuration Register */
  __I  GMAC_NSR_Type             NSR;         /**< \brief Offset: 0x008 (R/  32) Network Status Register */
  __IO GMAC_UR_Type              UR;          /**< \brief Offset: 0x00C (R/W 32) User Register */
  __IO GMAC_DCFGR_Type           DCFGR;       /**< \brief Offset: 0x010 (R/W 32) DMA Configuration Register */
  __IO GMAC_TSR_Type             TSR;         /**< \brief Offset: 0x014 (R/W 32) Transmit Status Register */
  __IO GMAC_RBQB_Type            RBQB;        /**< \brief Offset: 0x018 (R/W 32) Receive Buffer Queue Base Address */
  __IO GMAC_TBQB_Type            TBQB;        /**< \brief Offset: 0x01C (R/W 32) Transmit Buffer Queue Base Address */
  __IO GMAC_RSR_Type             RSR;         /**< \brief Offset: 0x020 (R/W 32) Receive Status Register */
  __IO GMAC_ISR_Type             ISR;         /**< \brief Offset: 0x024 (R/W 32) Interrupt Status Register */
  __O  GMAC_IER_Type             IER;         /**< \brief Offset: 0x028 ( /W 32) Interrupt Enable Register */
  __O  GMAC_IDR_Type             IDR;         /**< \brief Offset: 0x02C ( /W 32) Interrupt Disable Register */
  __I  GMAC_IMR_Type             IMR;         /**< \brief Offset: 0x030 (R/  32) Interrupt Mask Register */
  __IO GMAC_MAN_Type             MAN;         /**< \brief Offset: 0x034 (R/W 32) PHY Maintenance Register */
  __I  GMAC_RPQ_Type             RPQ;         /**< \brief Offset: 0x038 (R/  32) Received Pause Quantum Register */
  __IO GMAC_TPQ_Type             TPQ;         /**< \brief Offset: 0x03C (R/W 32) Transmit Pause Quantum Register */
  __IO GMAC_TPSF_Type            TPSF;        /**< \brief Offset: 0x040 (R/W 32) TX partial store and forward Register */
  __IO GMAC_RPSF_Type            RPSF;        /**< \brief Offset: 0x044 (R/W 32) RX partial store and forward Register */
  __IO GMAC_RJFML_Type           RJFML;       /**< \brief Offset: 0x048 (R/W 32) RX Jumbo Frame Max Length Register */
       RoReg8                    Reserved1[0x34];
  __IO GMAC_HRB_Type             HRB;         /**< \brief Offset: 0x080 (R/W 32) Hash Register Bottom [31:0] */
  __IO GMAC_HRT_Type             HRT;         /**< \brief Offset: 0x084 (R/W 32) Hash Register Top [63:32] */
       GmacSa                    Sa[4];       /**< \brief Offset: 0x088 GmacSa groups */
  __IO GMAC_TIDM_Type            TIDM[4];     /**< \brief Offset: 0x0A8 (R/W 32) Type ID Match Register */
  __IO GMAC_WOL_Type             WOL;         /**< \brief Offset: 0x0B8 (R/W 32) Wake on LAN */
  __IO GMAC_IPGS_Type            IPGS;        /**< \brief Offset: 0x0BC (R/W 32) IPG Stretch Register */
  __IO GMAC_SVLAN_Type           SVLAN;       /**< \brief Offset: 0x0C0 (R/W 32) Stacked VLAN Register */
  __IO GMAC_TPFCP_Type           TPFCP;       /**< \brief Offset: 0x0C4 (R/W 32) Transmit PFC Pause Register */
  __IO GMAC_SAMB1_Type           SAMB1;       /**< \brief Offset: 0x0C8 (R/W 32) Specific Address 1 Mask Bottom [31:0] Register */
  __IO GMAC_SAMT1_Type           SAMT1;       /**< \brief Offset: 0x0CC (R/W 32) Specific Address 1 Mask Top [47:32] Register */
       RoReg8                    Reserved2[0xC];
  __IO GMAC_NSC_Type             NSC;         /**< \brief Offset: 0x0DC (R/W 32) Tsu timer comparison nanoseconds Register */
  __IO GMAC_SCL_Type             SCL;         /**< \brief Offset: 0x0E0 (R/W 32) Tsu timer second comparison Register */
  __IO GMAC_SCH_Type             SCH;         /**< \brief Offset: 0x0E4 (R/W 32) Tsu timer second comparison Register */
  __I  GMAC_EFTSH_Type           EFTSH;       /**< \brief Offset: 0x0E8 (R/  32) PTP Event Frame Transmitted Seconds High Register */
  __I  GMAC_EFRSH_Type           EFRSH;       /**< \brief Offset: 0x0EC (R/  32) PTP Event Frame Received Seconds High Register */
  __I  GMAC_PEFTSH_Type          PEFTSH;      /**< \brief Offset: 0x0F0 (R/  32) PTP Peer Event Frame Transmitted Seconds High Register */
  __I  GMAC_PEFRSH_Type          PEFRSH;      /**< \brief Offset: 0x0F4 (R/  32) PTP Peer Event Frame Received Seconds High Register */
       RoReg8                    Reserved3[0x8];
  __I  GMAC_OTLO_Type            OTLO;        /**< \brief Offset: 0x100 (R/  32) Octets Transmitted [31:0] Register */
  __I  GMAC_OTHI_Type            OTHI;        /**< \brief Offset: 0x104 (R/  32) Octets Transmitted [47:32] Register */
  __I  GMAC_FT_Type              FT;          /**< \brief Offset: 0x108 (R/  32) Frames Transmitted Register */
  __I  GMAC_BCFT_Type            BCFT;        /**< \brief Offset: 0x10C (R/  32) Broadcast Frames Transmitted Register */
  __I  GMAC_MFT_Type             MFT;         /**< \brief Offset: 0x110 (R/  32) Multicast Frames Transmitted Register */
  __I  GMAC_PFT_Type             PFT;         /**< \brief Offset: 0x114 (R/  32) Pause Frames Transmitted Register */
  __I  GMAC_BFT64_Type           BFT64;       /**< \brief Offset: 0x118 (R/  32) 64 Byte Frames Transmitted Register */
  __I  GMAC_TBFT127_Type         TBFT127;     /**< \brief Offset: 0x11C (R/  32) 65 to 127 Byte Frames Transmitted Register */
  __I  GMAC_TBFT255_Type         TBFT255;     /**< \brief Offset: 0x120 (R/  32) 128 to 255 Byte Frames Transmitted Register */
  __I  GMAC_TBFT511_Type         TBFT511;     /**< \brief Offset: 0x124 (R/  32) 256 to 511 Byte Frames Transmitted Register */
  __I  GMAC_TBFT1023_Type        TBFT1023;    /**< \brief Offset: 0x128 (R/  32) 512 to 1023 Byte Frames Transmitted Register */
  __I  GMAC_TBFT1518_Type        TBFT1518;    /**< \brief Offset: 0x12C (R/  32) 1024 to 1518 Byte Frames Transmitted Register */
  __I  GMAC_GTBFT1518_Type       GTBFT1518;   /**< \brief Offset: 0x130 (R/  32) Greater Than 1518 Byte Frames Transmitted Register */
  __I  GMAC_TUR_Type             TUR;         /**< \brief Offset: 0x134 (R/  32) Transmit Underruns Register */
  __I  GMAC_SCF_Type             SCF;         /**< \brief Offset: 0x138 (R/  32) Single Collision Frames Register */
  __I  GMAC_MCF_Type             MCF;         /**< \brief Offset: 0x13C (R/  32) Multiple Collision Frames Register */
  __I  GMAC_EC_Type              EC;          /**< \brief Offset: 0x140 (R/  32) Excessive Collisions Register */
  __I  GMAC_LC_Type              LC;          /**< \brief Offset: 0x144 (R/  32) Late Collisions Register */
  __I  GMAC_DTF_Type             DTF;         /**< \brief Offset: 0x148 (R/  32) Deferred Transmission Frames Register */
  __I  GMAC_CSE_Type             CSE;         /**< \brief Offset: 0x14C (R/  32) Carrier Sense Errors Register */
  __I  GMAC_ORLO_Type            ORLO;        /**< \brief Offset: 0x150 (R/  32) Octets Received [31:0] Received */
  __I  GMAC_ORHI_Type            ORHI;        /**< \brief Offset: 0x154 (R/  32) Octets Received [47:32] Received */
  __I  GMAC_FR_Type              FR;          /**< \brief Offset: 0x158 (R/  32) Frames Received Register */
  __I  GMAC_BCFR_Type            BCFR;        /**< \brief Offset: 0x15C (R/  32) Broadcast Frames Received Register */
  __I  GMAC_MFR_Type             MFR;         /**< \brief Offset: 0x160 (R/  32) Multicast Frames Received Register */
  __I  GMAC_PFR_Type             PFR;         /**< \brief Offset: 0x164 (R/  32) Pause Frames Received Register */
  __I  GMAC_BFR64_Type           BFR64;       /**< \brief Offset: 0x168 (R/  32) 64 Byte Frames Received Register */
  __I  GMAC_TBFR127_Type         TBFR127;     /**< \brief Offset: 0x16C (R/  32) 65 to 127 Byte Frames Received Register */
  __I  GMAC_TBFR255_Type         TBFR255;     /**< \brief Offset: 0x170 (R/  32) 128 to 255 Byte Frames Received Register */
  __I  GMAC_TBFR511_Type         TBFR511;     /**< \brief Offset: 0x174 (R/  32) 256 to 511Byte Frames Received Register */
  __I  GMAC_TBFR1023_Type        TBFR1023;    /**< \brief Offset: 0x178 (R/  32) 512 to 1023 Byte Frames Received Register */
  __I  GMAC_TBFR1518_Type        TBFR1518;    /**< \brief Offset: 0x17C (R/  32) 1024 to 1518 Byte Frames Received Register */
  __I  GMAC_TMXBFR_Type          TMXBFR;      /**< \brief Offset: 0x180 (R/  32) 1519 to Maximum Byte Frames Received Register */
  __I  GMAC_UFR_Type             UFR;         /**< \brief Offset: 0x184 (R/  32) Undersize Frames Received Register */
  __I  GMAC_OFR_Type             OFR;         /**< \brief Offset: 0x188 (R/  32) Oversize Frames Received Register */
  __I  GMAC_JR_Type              JR;          /**< \brief Offset: 0x18C (R/  32) Jabbers Received Register */
  __I  GMAC_FCSE_Type            FCSE;        /**< \brief Offset: 0x190 (R/  32) Frame Check Sequence Errors Register */
  __I  GMAC_LFFE_Type            LFFE;        /**< \brief Offset: 0x194 (R/  32) Length Field Frame Errors Register */
  __I  GMAC_RSE_Type             RSE;         /**< \brief Offset: 0x198 (R/  32) Receive Symbol Errors Register */
  __I  GMAC_AE_Type              AE;          /**< \brief Offset: 0x19C (R/  32) Alignment Errors Register */
  __I  GMAC_RRE_Type             RRE;         /**< \brief Offset: 0x1A0 (R/  32) Receive Resource Errors Register */
  __I  GMAC_ROE_Type             ROE;         /**< \brief Offset: 0x1A4 (R/  32) Receive Overrun Register */
  __I  GMAC_IHCE_Type            IHCE;        /**< \brief Offset: 0x1A8 (R/  32) IP Header Checksum Errors Register */
  __I  GMAC_TCE_Type             TCE;         /**< \brief Offset: 0x1AC (R/  32) TCP Checksum Errors Register */
  __I  GMAC_UCE_Type             UCE;         /**< \brief Offset: 0x1B0 (R/  32) UDP Checksum Errors Register */
       RoReg8                    Reserved4[0x8];
  __IO GMAC_TISUBN_Type          TISUBN;      /**< \brief Offset: 0x1BC (R/W 32) 1588 Timer Increment [15:0] Sub-Nanoseconds Register */
  __IO GMAC_TSH_Type             TSH;         /**< \brief Offset: 0x1C0 (R/W 32) 1588 Timer Seconds High [15:0] Register */
       RoReg8                    Reserved5[0x4];
  __IO GMAC_TSSSL_Type           TSSSL;       /**< \brief Offset: 0x1C8 (R/W 32) 1588 Timer Sync Strobe Seconds [31:0] Register */
  __IO GMAC_TSSN_Type            TSSN;        /**< \brief Offset: 0x1CC (R/W 32) 1588 Timer Sync Strobe Nanoseconds Register */
  __IO GMAC_TSL_Type             TSL;         /**< \brief Offset: 0x1D0 (R/W 32) 1588 Timer Seconds [31:0] Register */
  __IO GMAC_TN_Type              TN;          /**< \brief Offset: 0x1D4 (R/W 32) 1588 Timer Nanoseconds Register */
  __O  GMAC_TA_Type              TA;          /**< \brief Offset: 0x1D8 ( /W 32) 1588 Timer Adjust Register */
  __IO GMAC_TI_Type              TI;          /**< \brief Offset: 0x1DC (R/W 32) 1588 Timer Increment Register */
  __I  GMAC_EFTSL_Type           EFTSL;       /**< \brief Offset: 0x1E0 (R/  32) PTP Event Frame Transmitted Seconds Low Register */
  __I  GMAC_EFTN_Type            EFTN;        /**< \brief Offset: 0x1E4 (R/  32) PTP Event Frame Transmitted Nanoseconds */
  __I  GMAC_EFRSL_Type           EFRSL;       /**< \brief Offset: 0x1E8 (R/  32) PTP Event Frame Received Seconds Low Register */
  __I  GMAC_EFRN_Type            EFRN;        /**< \brief Offset: 0x1EC (R/  32) PTP Event Frame Received Nanoseconds */
  __I  GMAC_PEFTSL_Type          PEFTSL;      /**< \brief Offset: 0x1F0 (R/  32) PTP Peer Event Frame Transmitted Seconds Low Register */
  __I  GMAC_PEFTN_Type           PEFTN;       /**< \brief Offset: 0x1F4 (R/  32) PTP Peer Event Frame Transmitted Nanoseconds */
  __I  GMAC_PEFRSL_Type          PEFRSL;      /**< \brief Offset: 0x1F8 (R/  32) PTP Peer Event Frame Received Seconds Low Register */
  __I  GMAC_PEFRN_Type           PEFRN;       /**< \brief Offset: 0x1FC (R/  32) PTP Peer Event Frame Received Nanoseconds */
       RoReg8                    Reserved6[0x70];
  __I  GMAC_RLPITR_Type          RLPITR;      /**< \brief Offset: 0x270 (R/  32) Receive LPI transition Register */
  __I  GMAC_RLPITI_Type          RLPITI;      /**< \brief Offset: 0x274 (R/  32) Receive LPI Time Register */
  __I  GMAC_TLPITR_Type          TLPITR;      /**< \brief Offset: 0x278 (R/  32) Receive LPI transition Register */
  __I  GMAC_TLPITI_Type          TLPITI;      /**< \brief Offset: 0x27C (R/  32) Receive LPI Time Register */
} Gmac;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/*@}*/

#endif /* _SAME54_GMAC_COMPONENT_ */
