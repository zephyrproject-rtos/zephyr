/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_GMAC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_GMAC_COMPONENT_FIXUP_H_

/* -------- GMAC_SAB : (GMAC Offset: 0x00) (R/W 32) Specific Address Bottom [31:0] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  Specific Address 1                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SAB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_SAT : (GMAC Offset: 0x04) (R/W 32) Specific Address Top [47:32] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:16;          /*!< bit:  0..15  Specific Address 1                 */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SAT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_NCR : (GMAC Offset: 0x00) (R/W 32) Network Control Register -------- */
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

/* -------- GMAC_NCFGR : (GMAC Offset: 0x04) (R/W 32) Network Configuration Register -------- */
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
/* -------- GMAC_NSR : (GMAC Offset: 0x08) ( R/ 32) Network Status Register -------- */
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

/* -------- GMAC_UR : (GMAC Offset: 0x0C) (R/W 32) User Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MII:1;            /*!< bit:      0  MII Mode                           */
    uint32_t :31;              /*!< bit:  1..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_UR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_DCFGR : (GMAC Offset: 0x10) (R/W 32) DMA Configuration Register -------- */
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

/* -------- GMAC_TSR : (GMAC Offset: 0x14) (R/W 32) Transmit Status Register -------- */
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

/* -------- GMAC_RBQB : (GMAC Offset: 0x18) (R/W 32) Receive Buffer Queue Base Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint32_t ADDR:30;          /*!< bit:  2..31  Receive Buffer Queue Base Address  */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RBQB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TBQB : (GMAC Offset: 0x1C) (R/W 32) Transmit Buffer Queue Base Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint32_t ADDR:30;          /*!< bit:  2..31  Transmit Buffer Queue Base Address */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBQB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_RSR : (GMAC Offset: 0x20) (R/W 32) Receive Status Register -------- */
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

/* -------- GMAC_ISR : (GMAC Offset: 0x24) (R/W 32) Interrupt Status Register -------- */
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
/* -------- GMAC_IER : (GMAC Offset: 0x28) ( /W 32) Interrupt Enable Register -------- */
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
/* -------- GMAC_IDR : (GMAC Offset: 0x2C) ( /W 32) Interrupt Disable Register -------- */
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

/* -------- GMAC_IMR : (GMAC Offset: 0x30) ( R/ 32) Interrupt Mask Register -------- */
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

/* -------- GMAC_MAN : (GMAC Offset: 0x34) (R/W 32) PHY Maintenance Register -------- */
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

/* -------- GMAC_RPQ : (GMAC Offset: 0x38) ( R/ 32) Received Pause Quantum Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RPQ:16;           /*!< bit:  0..15  Received Pause Quantum             */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RPQ_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TPQ : (GMAC Offset: 0x3C) (R/W 32) Transmit Pause Quantum Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TPQ:16;           /*!< bit:  0..15  Transmit Pause Quantum             */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TPQ_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TPSF : (GMAC Offset: 0x40) (R/W 32) TX partial store and forward Register -------- */
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

/* -------- GMAC_RPSF : (GMAC Offset: 0x44) (R/W 32) RX partial store and forward Register -------- */
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

/* -------- GMAC_RJFML : (GMAC Offset: 0x48) (R/W 32) RX Jumbo Frame Max Length Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FML:14;           /*!< bit:  0..13  Frame Max Length                   */
    uint32_t :18;              /*!< bit: 14..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RJFML_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_HRB : (GMAC Offset: 0x80) (R/W 32) Hash Register Bottom [31:0] -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  Hash Address                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_HRB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_HRT : (GMAC Offset: 0x84) (R/W 32) Hash Register Top [63:32] -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  Hash Address                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_HRT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TIDM : (GMAC Offset: 0xA8) (R/W 32) Type ID Match n Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TID:16;           /*!< bit:  0..15  Type ID Match 1                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TIDM_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_WOL : (GMAC Offset: 0xB8) (R/W 32) Wake on LAN -------- */
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

/* -------- GMAC_IPGS : (GMAC Offset: 0xBC) (R/W 32) IPG Stretch Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FL:16;            /*!< bit:  0..15  Frame Length                       */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_IPGS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_SVLAN : (GMAC Offset: 0xC0) (R/W 32) Stacked VLAN Register -------- */
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

/* -------- GMAC_TPFCP : (GMAC Offset: 0xC4) (R/W 32) Transmit PFC Pause Register -------- */
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

/* -------- GMAC_SAMB1 : (GMAC Offset: 0xC8) (R/W 32) Specific Address 1 Mask Bottom [31:0] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  Specific Address 1 Mask            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SAMB1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_SAMT1 : (GMAC Offset: 0xCC) (R/W 32) Specific Address 1 Mask Top [47:32] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:16;          /*!< bit:  0..15  Specific Address 1 Mask            */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SAMT1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_NSC : (GMAC Offset: 0xDC) (R/W 32) Tsu timer comparison nanoseconds Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NANOSEC:21;       /*!< bit:  0..20  1588 Timer Nanosecond comparison value */
    uint32_t :11;              /*!< bit: 21..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_NSC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_SCL : (GMAC Offset: 0xE0) (R/W 32) Tsu timer second comparison Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SEC:32;           /*!< bit:  0..31  1588 Timer Second comparison value */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SCL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_SCH : (GMAC Offset: 0xE4) (R/W 32) Tsu timer second comparison Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SEC:16;           /*!< bit:  0..15  1588 Timer Second comparison value */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SCH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_EFTSH : (GMAC Offset: 0xE8) ( R/ 32) PTP Event Frame Transmitted Seconds High Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:16;           /*!< bit:  0..15  Register Update                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFTSH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_EFRSH : (GMAC Offset: 0xEC) ( R/ 32) PTP Event Frame Received Seconds High Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:16;           /*!< bit:  0..15  Register Update                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFRSH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_PEFTSH : (GMAC Offset: 0xF0) ( R/ 32) PTP Peer Event Frame Transmitted Seconds High Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:16;           /*!< bit:  0..15  Register Update                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFTSH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_PEFRSH : (GMAC Offset: 0xF4) ( R/ 32) PTP Peer Event Frame Received Seconds High Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:16;           /*!< bit:  0..15  Register Update                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFRSH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_OTLO : (GMAC Offset: 0x100) ( R/ 32) Octets Transmitted [31:0] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TXO:32;           /*!< bit:  0..31  Transmitted Octets                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_OTLO_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_OTHI : (GMAC Offset: 0x104) ( R/ 32) Octets Transmitted [47:32] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TXO:16;           /*!< bit:  0..15  Transmitted Octets                 */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_OTHI_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_FT : (GMAC Offset: 0x108) ( R/ 32) Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FTX:32;           /*!< bit:  0..31  Frames Transmitted without Error   */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_FT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_BCFT : (GMAC Offset: 0x10C) ( R/ 32) Broadcast Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BFTX:32;          /*!< bit:  0..31  Broadcast Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_BCFT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_MFT : (GMAC Offset: 0x110) ( R/ 32) Multicast Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MFTX:32;          /*!< bit:  0..31  Multicast Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_MFT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_PFT : (GMAC Offset: 0x114) ( R/ 32) Pause Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PFTX:16;          /*!< bit:  0..15  Pause Frames Transmitted Register  */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PFT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_BFT64 : (GMAC Offset: 0x118) ( R/ 32) 64 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  64 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_BFT64_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TBFT127 : (GMAC Offset: 0x11C) ( R/ 32) 65 to 127 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  65 to 127 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFT127_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TBFT255 : (GMAC Offset: 0x120) ( R/ 32) 128 to 255 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  128 to 255 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFT255_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TBFT511 : (GMAC Offset: 0x124) ( R/ 32) 256 to 511 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  256 to 511 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFT511_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TBFT1023 : (GMAC Offset: 0x128) ( R/ 32) 512 to 1023 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  512 to 1023 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFT1023_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TBFT1518 : (GMAC Offset: 0x12C) ( R/ 32) 1024 to 1518 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  1024 to 1518 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFT1518_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_GTBFT1518 : (GMAC Offset: 0x130) ( R/ 32) Greater Than 1518 Byte Frames Transmitted Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFTX:32;          /*!< bit:  0..31  Greater than 1518 Byte Frames Transmitted without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_GTBFT1518_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TUR : (GMAC Offset: 0x134) ( R/ 32) Transmit Underruns Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TXUNR:10;         /*!< bit:  0.. 9  Transmit Underruns                 */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TUR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_SCF : (GMAC Offset: 0x138) ( R/ 32) Single Collision Frames Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SCOL:18;          /*!< bit:  0..17  Single Collision                   */
    uint32_t :14;              /*!< bit: 18..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_SCF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_MCF : (GMAC Offset: 0x13C) ( R/ 32) Multiple Collision Frames Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MCOL:18;          /*!< bit:  0..17  Multiple Collision                 */
    uint32_t :14;              /*!< bit: 18..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_MCF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_EC : (GMAC Offset: 0x140) ( R/ 32) Excessive Collisions Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t XCOL:10;          /*!< bit:  0.. 9  Excessive Collisions               */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_LC : (GMAC Offset: 0x144) ( R/ 32) Late Collisions Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LCOL:10;          /*!< bit:  0.. 9  Late Collisions                    */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_LC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_DTF : (GMAC Offset: 0x148) ( R/ 32) Deferred Transmission Frames Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DEFT:18;          /*!< bit:  0..17  Deferred Transmission              */
    uint32_t :14;              /*!< bit: 18..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_DTF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_CSE : (GMAC Offset: 0x14C) ( R/ 32) Carrier Sense Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CSR:10;           /*!< bit:  0.. 9  Carrier Sense Error                */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_CSE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_ORLO : (GMAC Offset: 0x150) ( R/ 32) Octets Received [31:0] Received -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXO:32;           /*!< bit:  0..31  Received Octets                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_ORLO_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_ORHI : (GMAC Offset: 0x154) ( R/ 32) Octets Received [47:32] Received -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXO:16;           /*!< bit:  0..15  Received Octets                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_ORHI_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_FR : (GMAC Offset: 0x158) ( R/ 32) Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FRX:32;           /*!< bit:  0..31  Frames Received without Error      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_FR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_BCFR : (GMAC Offset: 0x15C) ( R/ 32) Broadcast Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BFRX:32;          /*!< bit:  0..31  Broadcast Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_BCFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_MFR : (GMAC Offset: 0x160) ( R/ 32) Multicast Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MFRX:32;          /*!< bit:  0..31  Multicast Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_MFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_PFR : (GMAC Offset: 0x164) ( R/ 32) Pause Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PFRX:16;          /*!< bit:  0..15  Pause Frames Received Register     */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_BFR64 : (GMAC Offset: 0x168) ( R/ 32) 64 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  64 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_BFR64_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TBFR127 : (GMAC Offset: 0x16C) ( R/ 32) 65 to 127 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  65 to 127 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFR127_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TBFR255 : (GMAC Offset: 0x170) ( R/ 32) 128 to 255 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  128 to 255 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFR255_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TBFR511 : (GMAC Offset: 0x174) ( R/ 32) 256 to 511Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  256 to 511 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFR511_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TBFR1023 : (GMAC Offset: 0x178) ( R/ 32) 512 to 1023 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  512 to 1023 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFR1023_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TBFR1518 : (GMAC Offset: 0x17C) ( R/ 32) 1024 to 1518 Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  1024 to 1518 Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TBFR1518_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TMXBFR : (GMAC Offset: 0x180) ( R/ 32) 1519 to Maximum Byte Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NFRX:32;          /*!< bit:  0..31  1519 to Maximum Byte Frames Received without Error */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TMXBFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_UFR : (GMAC Offset: 0x184) ( R/ 32) Undersize Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t UFRX:10;          /*!< bit:  0.. 9  Undersize Frames Received          */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_UFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_OFR : (GMAC Offset: 0x188) ( R/ 32) Oversize Frames Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t OFRX:10;          /*!< bit:  0.. 9  Oversized Frames Received          */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_OFR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_JR : (GMAC Offset: 0x18C) ( R/ 32) Jabbers Received Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t JRX:10;           /*!< bit:  0.. 9  Jabbers Received                   */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_JR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_FCSE : (GMAC Offset: 0x190) ( R/ 32) Frame Check Sequence Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FCKR:10;          /*!< bit:  0.. 9  Frame Check Sequence Errors        */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_FCSE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_LFFE : (GMAC Offset: 0x194) ( R/ 32) Length Field Frame Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LFER:10;          /*!< bit:  0.. 9  Length Field Frame Errors          */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_LFFE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_RSE : (GMAC Offset: 0x198) ( R/ 32) Receive Symbol Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXSE:10;          /*!< bit:  0.. 9  Receive Symbol Errors              */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RSE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_AE : (GMAC Offset: 0x19C) ( R/ 32) Alignment Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t AER:10;           /*!< bit:  0.. 9  Alignment Errors                   */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_AE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_RRE : (GMAC Offset: 0x1A0) ( R/ 32) Receive Resource Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXRER:18;         /*!< bit:  0..17  Receive Resource Errors            */
    uint32_t :14;              /*!< bit: 18..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RRE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_ROE : (GMAC Offset: 0x1A4) ( R/ 32) Receive Overrun Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXOVR:10;         /*!< bit:  0.. 9  Receive Overruns                   */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_ROE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_IHCE : (GMAC Offset: 0x1A8) ( R/ 32) IP Header Checksum Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t HCKER:8;          /*!< bit:  0.. 7  IP Header Checksum Errors          */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_IHCE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TCE : (GMAC Offset: 0x1AC) ( R/ 32) TCP Checksum Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TCKER:8;          /*!< bit:  0.. 7  TCP Checksum Errors                */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TCE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_UCE : (GMAC Offset: 0x1B0) ( R/ 32) UDP Checksum Errors Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t UCKER:8;          /*!< bit:  0.. 7  UDP Checksum Errors                */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_UCE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

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

/* -------- GMAC_TSSSL : (GMAC Offset: 0x1C8) (R/W 32) 1588 Timer Sync Strobe Seconds [31:0] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t VTS:32;           /*!< bit:  0..31  Value of Timer Seconds Register Capture */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TSSSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

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

/* -------- GMAC_TSL : (GMAC Offset: 0x1D0) (R/W 32) 1588 Timer Seconds [31:0] Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TCS:32;           /*!< bit:  0..31  Timer Count in Seconds             */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

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

/* -------- GMAC_EFTSL : (GMAC Offset: 0x1E0) ( R/ 32) PTP Event Frame Transmitted Seconds Low Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:32;           /*!< bit:  0..31  Register Update                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFTSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_EFTN : (GMAC Offset: 0x1E4) ( R/ 32) PTP Event Frame Transmitted Nanoseconds -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:30;           /*!< bit:  0..29  Register Update                    */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFTN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_EFRSL : (GMAC Offset: 0x1E8) ( R/ 32) PTP Event Frame Received Seconds Low Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:32;           /*!< bit:  0..31  Register Update                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFRSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_EFRN : (GMAC Offset: 0x1EC) ( R/ 32) PTP Event Frame Received Nanoseconds -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:30;           /*!< bit:  0..29  Register Update                    */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_EFRN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_PEFTSL : (GMAC Offset: 0x1F0) ( R/ 32) PTP Peer Event Frame Transmitted Seconds Low Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:32;           /*!< bit:  0..31  Register Update                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFTSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_PEFTN : (GMAC Offset: 0x1F4) ( R/ 32) PTP Peer Event Frame Transmitted Nanoseconds -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:30;           /*!< bit:  0..29  Register Update                    */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFTN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_PEFRSL : (GMAC Offset: 0x1F8) ( R/ 32) PTP Peer Event Frame Received Seconds Low Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:32;           /*!< bit:  0..31  Register Update                    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFRSL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_PEFRN : (GMAC Offset: 0x1FC) ( R/ 32) PTP Peer Event Frame Received Nanoseconds -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RUD:30;           /*!< bit:  0..29  Register Update                    */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_PEFRN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_RLPITR : (GMAC Offset: 0x270) ( R/ 32) Receive LPI transition Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RLPITR:16;        /*!< bit:  0..15  Count number of times transition from rx normal idle to low power idle */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RLPITR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_RLPITI : (GMAC Offset: 0x274) ( R/ 32) Receive LPI Time Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RLPITI:24;        /*!< bit:  0..23  Increment once over 16 ahb clock when LPI indication bit 20 is set in rx mode */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_RLPITI_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TLPITR : (GMAC Offset: 0x278) ( R/ 32) Receive LPI transition Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TLPITR:16;        /*!< bit:  0..15  Count number of times enable LPI tx bit 20 goes from low to high */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TLPITR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- GMAC_TLPITI : (GMAC Offset: 0x27C) ( R/ 32) Receive LPI Time Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TLPITI:24;        /*!< bit:  0..23  Increment once over 16 ahb clock when LPI indication bit 20 is set in tx mode */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} GMAC_TLPITI_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

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

#endif /* _MICROCHIP_PIC32CXSG_GMAC_COMPONENT_FIXUP_H_ */
