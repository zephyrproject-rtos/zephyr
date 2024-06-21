/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_DMAC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_DMAC_COMPONENT_FIXUP_H_

/* -------- DMAC_BTCTRL : (DMAC Offset: 0x00) (R/W 16) Block Transfer Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t VALID:1;          /*!< bit:      0  Descriptor Valid                   */
    uint16_t EVOSEL:2;         /*!< bit:  1.. 2  Block Event Output Selection       */
    uint16_t BLOCKACT:2;       /*!< bit:  3.. 4  Block Action                       */
    uint16_t :3;               /*!< bit:  5.. 7  Reserved                           */
    uint16_t BEATSIZE:2;       /*!< bit:  8.. 9  Beat Size                          */
    uint16_t SRCINC:1;         /*!< bit:     10  Source Address Increment Enable    */
    uint16_t DSTINC:1;         /*!< bit:     11  Destination Address Increment Enable */
    uint16_t STEPSEL:1;        /*!< bit:     12  Step Selection                     */
    uint16_t STEPSIZE:3;       /*!< bit: 13..15  Address Increment Step Size        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} DMAC_BTCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_BTCNT : (DMAC Offset: 0x02) (R/W 16) Block Transfer Count -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t BTCNT:16;         /*!< bit:  0..15  Block Transfer Count               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} DMAC_BTCNT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_SRCADDR : (DMAC Offset: 0x04) (R/W 32) Block Transfer Source Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SRCADDR:32;       /*!< bit:  0..31  Transfer Source Address            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_SRCADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_DSTADDR : (DMAC Offset: 0x08) (R/W 32) Block Transfer Destination Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct { // CRC mode
    uint32_t CHKINIT:32;       /*!< bit:  0..31  CRC Checksum Initial Value         */
  } CRC;                       /*!< Structure used for CRC                          */
  struct {
    uint32_t DSTADDR:32;       /*!< bit:  0..31  Transfer Destination Address       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_DSTADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_DESCADDR : (DMAC Offset: 0x0C) (R/W 32) Next Descriptor Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DESCADDR:32;      /*!< bit:  0..31  Next Descriptor Address            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_DESCADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CHCTRLA : (DMAC Offset: 0x00) (R/W 32) Channel n Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Channel Software Reset             */
    uint32_t ENABLE:1;         /*!< bit:      1  Channel Enable                     */
    uint32_t :4;               /*!< bit:  2.. 5  Reserved                           */
    uint32_t RUNSTDBY:1;       /*!< bit:      6  Channel Run in Standby             */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t TRIGSRC:7;        /*!< bit:  8..14  Trigger Source                     */
    uint32_t :5;               /*!< bit: 15..19  Reserved                           */
    uint32_t TRIGACT:2;        /*!< bit: 20..21  Trigger Action                     */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t BURSTLEN:4;       /*!< bit: 24..27  Burst Length                       */
    uint32_t THRESHOLD:2;      /*!< bit: 28..29  FIFO Threshold                     */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_CHCTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CHCTRLB : (DMAC Offset: 0x04) (R/W 8) Channel n Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CMD:2;            /*!< bit:  0.. 1  Software Command                   */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DMAC_CHCTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CHPRILVL : (DMAC Offset: 0x05) (R/W 8) Channel n Priority Level -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  PRILVL:2;         /*!< bit:  0.. 1  Channel Priority Level             */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DMAC_CHPRILVL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CHEVCTRL : (DMAC Offset: 0x06) (R/W 8) Channel n Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  EVACT:3;          /*!< bit:  0.. 2  Channel Event Input Action         */
    uint8_t  :1;               /*!< bit:      3  Reserved                           */
    uint8_t  EVOMODE:2;        /*!< bit:  4.. 5  Channel Event Output Mode          */
    uint8_t  EVIE:1;           /*!< bit:      6  Channel Event Input Enable         */
    uint8_t  EVOE:1;           /*!< bit:      7  Channel Event Output Enable        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DMAC_CHEVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CHINTENCLR : (DMAC Offset: 0x0C) (R/W 8) Channel n Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  TERR:1;           /*!< bit:      0  Channel Transfer Error Interrupt Enable */
    uint8_t  TCMPL:1;          /*!< bit:      1  Channel Transfer Complete Interrupt Enable */
    uint8_t  SUSP:1;           /*!< bit:      2  Channel Suspend Interrupt Enable   */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DMAC_CHINTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CHINTENSET : (DMAC Offset: 0x0D) (R/W 8) Channel n Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  TERR:1;           /*!< bit:      0  Channel Transfer Error Interrupt Enable */
    uint8_t  TCMPL:1;          /*!< bit:      1  Channel Transfer Complete Interrupt Enable */
    uint8_t  SUSP:1;           /*!< bit:      2  Channel Suspend Interrupt Enable   */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DMAC_CHINTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CHINTFLAG : (DMAC Offset: 0x0E) (R/W 8) Channel n Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  TERR:1;           /*!< bit:      0  Channel Transfer Error             */
    __I uint8_t  TCMPL:1;          /*!< bit:      1  Channel Transfer Complete          */
    __I uint8_t  SUSP:1;           /*!< bit:      2  Channel Suspend                    */
    __I uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DMAC_CHINTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CHSTATUS : (DMAC Offset: 0x0F) (R/W 8) Channel n Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  PEND:1;           /*!< bit:      0  Channel Pending                    */
    uint8_t  BUSY:1;           /*!< bit:      1  Channel Busy                       */
    uint8_t  FERR:1;           /*!< bit:      2  Channel Fetch Error                */
    uint8_t  CRCERR:1;         /*!< bit:      3  Channel CRC Error                  */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DMAC_CHSTATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CTRL : (DMAC Offset: 0x00) (R/W 16) Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint16_t DMAENABLE:1;      /*!< bit:      1  DMA Enable                         */
    uint16_t :6;               /*!< bit:  2.. 7  Reserved                           */
    uint16_t LVLEN0:1;         /*!< bit:      8  Priority Level 0 Enable            */
    uint16_t LVLEN1:1;         /*!< bit:      9  Priority Level 1 Enable            */
    uint16_t LVLEN2:1;         /*!< bit:     10  Priority Level 2 Enable            */
    uint16_t LVLEN3:1;         /*!< bit:     11  Priority Level 3 Enable            */
    uint16_t :4;               /*!< bit: 12..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t :8;               /*!< bit:  0.. 7  Reserved                           */
    uint16_t LVLEN:4;          /*!< bit:  8..11  Priority Level x Enable            */
    uint16_t :4;               /*!< bit: 12..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} DMAC_CTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CRCCTRL : (DMAC Offset: 0x02) (R/W 16) CRC Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t CRCBEATSIZE:2;    /*!< bit:  0.. 1  CRC Beat Size                      */
    uint16_t CRCPOLY:2;        /*!< bit:  2.. 3  CRC Polynomial Type                */
    uint16_t :4;               /*!< bit:  4.. 7  Reserved                           */
    uint16_t CRCSRC:6;         /*!< bit:  8..13  CRC Input Source                   */
    uint16_t CRCMODE:2;        /*!< bit: 14..15  CRC Operating Mode                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} DMAC_CRCCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CRCDATAIN : (DMAC Offset: 0x04) (R/W 32) CRC Data Input -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CRCDATAIN:32;     /*!< bit:  0..31  CRC Data Input                     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_CRCDATAIN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CRCCHKSUM : (DMAC Offset: 0x08) (R/W 32) CRC Checksum -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CRCCHKSUM:32;     /*!< bit:  0..31  CRC Checksum                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_CRCCHKSUM_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_CRCSTATUS : (DMAC Offset: 0x0C) (R/W 8) CRC Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CRCBUSY:1;        /*!< bit:      0  CRC Module Busy                    */
    uint8_t  CRCZERO:1;        /*!< bit:      1  CRC Zero                           */
    uint8_t  CRCERR:1;         /*!< bit:      2  CRC Error                          */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DMAC_CRCSTATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_DBGCTRL : (DMAC Offset: 0x0D) (R/W 8) Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DBGRUN:1;         /*!< bit:      0  Debug Run                          */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} DMAC_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_SWTRIGCTRL : (DMAC Offset: 0x10) (R/W 32) Software Trigger Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWTRIG0:1;        /*!< bit:      0  Channel 0 Software Trigger         */
    uint32_t SWTRIG1:1;        /*!< bit:      1  Channel 1 Software Trigger         */
    uint32_t SWTRIG2:1;        /*!< bit:      2  Channel 2 Software Trigger         */
    uint32_t SWTRIG3:1;        /*!< bit:      3  Channel 3 Software Trigger         */
    uint32_t SWTRIG4:1;        /*!< bit:      4  Channel 4 Software Trigger         */
    uint32_t SWTRIG5:1;        /*!< bit:      5  Channel 5 Software Trigger         */
    uint32_t SWTRIG6:1;        /*!< bit:      6  Channel 6 Software Trigger         */
    uint32_t SWTRIG7:1;        /*!< bit:      7  Channel 7 Software Trigger         */
    uint32_t SWTRIG8:1;        /*!< bit:      8  Channel 8 Software Trigger         */
    uint32_t SWTRIG9:1;        /*!< bit:      9  Channel 9 Software Trigger         */
    uint32_t SWTRIG10:1;       /*!< bit:     10  Channel 10 Software Trigger        */
    uint32_t SWTRIG11:1;       /*!< bit:     11  Channel 11 Software Trigger        */
    uint32_t SWTRIG12:1;       /*!< bit:     12  Channel 12 Software Trigger        */
    uint32_t SWTRIG13:1;       /*!< bit:     13  Channel 13 Software Trigger        */
    uint32_t SWTRIG14:1;       /*!< bit:     14  Channel 14 Software Trigger        */
    uint32_t SWTRIG15:1;       /*!< bit:     15  Channel 15 Software Trigger        */
    uint32_t SWTRIG16:1;       /*!< bit:     16  Channel 16 Software Trigger        */
    uint32_t SWTRIG17:1;       /*!< bit:     17  Channel 17 Software Trigger        */
    uint32_t SWTRIG18:1;       /*!< bit:     18  Channel 18 Software Trigger        */
    uint32_t SWTRIG19:1;       /*!< bit:     19  Channel 19 Software Trigger        */
    uint32_t SWTRIG20:1;       /*!< bit:     20  Channel 20 Software Trigger        */
    uint32_t SWTRIG21:1;       /*!< bit:     21  Channel 21 Software Trigger        */
    uint32_t SWTRIG22:1;       /*!< bit:     22  Channel 22 Software Trigger        */
    uint32_t SWTRIG23:1;       /*!< bit:     23  Channel 23 Software Trigger        */
    uint32_t SWTRIG24:1;       /*!< bit:     24  Channel 24 Software Trigger        */
    uint32_t SWTRIG25:1;       /*!< bit:     25  Channel 25 Software Trigger        */
    uint32_t SWTRIG26:1;       /*!< bit:     26  Channel 26 Software Trigger        */
    uint32_t SWTRIG27:1;       /*!< bit:     27  Channel 27 Software Trigger        */
    uint32_t SWTRIG28:1;       /*!< bit:     28  Channel 28 Software Trigger        */
    uint32_t SWTRIG29:1;       /*!< bit:     29  Channel 29 Software Trigger        */
    uint32_t SWTRIG30:1;       /*!< bit:     30  Channel 30 Software Trigger        */
    uint32_t SWTRIG31:1;       /*!< bit:     31  Channel 31 Software Trigger        */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t SWTRIG:32;        /*!< bit:  0..31  Channel x Software Trigger         */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_SWTRIGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_PRICTRL0 : (DMAC Offset: 0x14) (R/W 32) Priority Control 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LVLPRI0:5;        /*!< bit:  0.. 4  Level 0 Channel Priority Number    */
    uint32_t QOS0:2;           /*!< bit:  5.. 6  Level 0 Quality of Service         */
    uint32_t RRLVLEN0:1;       /*!< bit:      7  Level 0 Round-Robin Scheduling Enable */
    uint32_t LVLPRI1:5;        /*!< bit:  8..12  Level 1 Channel Priority Number    */
    uint32_t QOS1:2;           /*!< bit: 13..14  Level 1 Quality of Service         */
    uint32_t RRLVLEN1:1;       /*!< bit:     15  Level 1 Round-Robin Scheduling Enable */
    uint32_t LVLPRI2:5;        /*!< bit: 16..20  Level 2 Channel Priority Number    */
    uint32_t QOS2:2;           /*!< bit: 21..22  Level 2 Quality of Service         */
    uint32_t RRLVLEN2:1;       /*!< bit:     23  Level 2 Round-Robin Scheduling Enable */
    uint32_t LVLPRI3:5;        /*!< bit: 24..28  Level 3 Channel Priority Number    */
    uint32_t QOS3:2;           /*!< bit: 29..30  Level 3 Quality of Service         */
    uint32_t RRLVLEN3:1;       /*!< bit:     31  Level 3 Round-Robin Scheduling Enable */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_PRICTRL0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_INTPEND : (DMAC Offset: 0x20) (R/W 16) Interrupt Pending -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t ID:5;             /*!< bit:  0.. 4  Channel ID                         */
    uint16_t :3;               /*!< bit:  5.. 7  Reserved                           */
    uint16_t TERR:1;           /*!< bit:      8  Transfer Error                     */
    uint16_t TCMPL:1;          /*!< bit:      9  Transfer Complete                  */
    uint16_t SUSP:1;           /*!< bit:     10  Channel Suspend                    */
    uint16_t :1;               /*!< bit:     11  Reserved                           */
    uint16_t CRCERR:1;         /*!< bit:     12  CRC Error                          */
    uint16_t FERR:1;           /*!< bit:     13  Fetch Error                        */
    uint16_t BUSY:1;           /*!< bit:     14  Busy                               */
    uint16_t PEND:1;           /*!< bit:     15  Pending                            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} DMAC_INTPEND_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_INTSTATUS : (DMAC Offset: 0x24) ( R/ 32) Interrupt Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CHINT0:1;         /*!< bit:      0  Channel 0 Pending Interrupt        */
    uint32_t CHINT1:1;         /*!< bit:      1  Channel 1 Pending Interrupt        */
    uint32_t CHINT2:1;         /*!< bit:      2  Channel 2 Pending Interrupt        */
    uint32_t CHINT3:1;         /*!< bit:      3  Channel 3 Pending Interrupt        */
    uint32_t CHINT4:1;         /*!< bit:      4  Channel 4 Pending Interrupt        */
    uint32_t CHINT5:1;         /*!< bit:      5  Channel 5 Pending Interrupt        */
    uint32_t CHINT6:1;         /*!< bit:      6  Channel 6 Pending Interrupt        */
    uint32_t CHINT7:1;         /*!< bit:      7  Channel 7 Pending Interrupt        */
    uint32_t CHINT8:1;         /*!< bit:      8  Channel 8 Pending Interrupt        */
    uint32_t CHINT9:1;         /*!< bit:      9  Channel 9 Pending Interrupt        */
    uint32_t CHINT10:1;        /*!< bit:     10  Channel 10 Pending Interrupt       */
    uint32_t CHINT11:1;        /*!< bit:     11  Channel 11 Pending Interrupt       */
    uint32_t CHINT12:1;        /*!< bit:     12  Channel 12 Pending Interrupt       */
    uint32_t CHINT13:1;        /*!< bit:     13  Channel 13 Pending Interrupt       */
    uint32_t CHINT14:1;        /*!< bit:     14  Channel 14 Pending Interrupt       */
    uint32_t CHINT15:1;        /*!< bit:     15  Channel 15 Pending Interrupt       */
    uint32_t CHINT16:1;        /*!< bit:     16  Channel 16 Pending Interrupt       */
    uint32_t CHINT17:1;        /*!< bit:     17  Channel 17 Pending Interrupt       */
    uint32_t CHINT18:1;        /*!< bit:     18  Channel 18 Pending Interrupt       */
    uint32_t CHINT19:1;        /*!< bit:     19  Channel 19 Pending Interrupt       */
    uint32_t CHINT20:1;        /*!< bit:     20  Channel 20 Pending Interrupt       */
    uint32_t CHINT21:1;        /*!< bit:     21  Channel 21 Pending Interrupt       */
    uint32_t CHINT22:1;        /*!< bit:     22  Channel 22 Pending Interrupt       */
    uint32_t CHINT23:1;        /*!< bit:     23  Channel 23 Pending Interrupt       */
    uint32_t CHINT24:1;        /*!< bit:     24  Channel 24 Pending Interrupt       */
    uint32_t CHINT25:1;        /*!< bit:     25  Channel 25 Pending Interrupt       */
    uint32_t CHINT26:1;        /*!< bit:     26  Channel 26 Pending Interrupt       */
    uint32_t CHINT27:1;        /*!< bit:     27  Channel 27 Pending Interrupt       */
    uint32_t CHINT28:1;        /*!< bit:     28  Channel 28 Pending Interrupt       */
    uint32_t CHINT29:1;        /*!< bit:     29  Channel 29 Pending Interrupt       */
    uint32_t CHINT30:1;        /*!< bit:     30  Channel 30 Pending Interrupt       */
    uint32_t CHINT31:1;        /*!< bit:     31  Channel 31 Pending Interrupt       */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t CHINT:32;         /*!< bit:  0..31  Channel x Pending Interrupt        */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_INTSTATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_BUSYCH : (DMAC Offset: 0x28) ( R/ 32) Busy Channels -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BUSYCH0:1;        /*!< bit:      0  Busy Channel 0                     */
    uint32_t BUSYCH1:1;        /*!< bit:      1  Busy Channel 1                     */
    uint32_t BUSYCH2:1;        /*!< bit:      2  Busy Channel 2                     */
    uint32_t BUSYCH3:1;        /*!< bit:      3  Busy Channel 3                     */
    uint32_t BUSYCH4:1;        /*!< bit:      4  Busy Channel 4                     */
    uint32_t BUSYCH5:1;        /*!< bit:      5  Busy Channel 5                     */
    uint32_t BUSYCH6:1;        /*!< bit:      6  Busy Channel 6                     */
    uint32_t BUSYCH7:1;        /*!< bit:      7  Busy Channel 7                     */
    uint32_t BUSYCH8:1;        /*!< bit:      8  Busy Channel 8                     */
    uint32_t BUSYCH9:1;        /*!< bit:      9  Busy Channel 9                     */
    uint32_t BUSYCH10:1;       /*!< bit:     10  Busy Channel 10                    */
    uint32_t BUSYCH11:1;       /*!< bit:     11  Busy Channel 11                    */
    uint32_t BUSYCH12:1;       /*!< bit:     12  Busy Channel 12                    */
    uint32_t BUSYCH13:1;       /*!< bit:     13  Busy Channel 13                    */
    uint32_t BUSYCH14:1;       /*!< bit:     14  Busy Channel 14                    */
    uint32_t BUSYCH15:1;       /*!< bit:     15  Busy Channel 15                    */
    uint32_t BUSYCH16:1;       /*!< bit:     16  Busy Channel 16                    */
    uint32_t BUSYCH17:1;       /*!< bit:     17  Busy Channel 17                    */
    uint32_t BUSYCH18:1;       /*!< bit:     18  Busy Channel 18                    */
    uint32_t BUSYCH19:1;       /*!< bit:     19  Busy Channel 19                    */
    uint32_t BUSYCH20:1;       /*!< bit:     20  Busy Channel 20                    */
    uint32_t BUSYCH21:1;       /*!< bit:     21  Busy Channel 21                    */
    uint32_t BUSYCH22:1;       /*!< bit:     22  Busy Channel 22                    */
    uint32_t BUSYCH23:1;       /*!< bit:     23  Busy Channel 23                    */
    uint32_t BUSYCH24:1;       /*!< bit:     24  Busy Channel 24                    */
    uint32_t BUSYCH25:1;       /*!< bit:     25  Busy Channel 25                    */
    uint32_t BUSYCH26:1;       /*!< bit:     26  Busy Channel 26                    */
    uint32_t BUSYCH27:1;       /*!< bit:     27  Busy Channel 27                    */
    uint32_t BUSYCH28:1;       /*!< bit:     28  Busy Channel 28                    */
    uint32_t BUSYCH29:1;       /*!< bit:     29  Busy Channel 29                    */
    uint32_t BUSYCH30:1;       /*!< bit:     30  Busy Channel 30                    */
    uint32_t BUSYCH31:1;       /*!< bit:     31  Busy Channel 31                    */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t BUSYCH:32;        /*!< bit:  0..31  Busy Channel x                     */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_BUSYCH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_PENDCH : (DMAC Offset: 0x2C) ( R/ 32) Pending Channels -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t PENDCH0:1;        /*!< bit:      0  Pending Channel 0                  */
    uint32_t PENDCH1:1;        /*!< bit:      1  Pending Channel 1                  */
    uint32_t PENDCH2:1;        /*!< bit:      2  Pending Channel 2                  */
    uint32_t PENDCH3:1;        /*!< bit:      3  Pending Channel 3                  */
    uint32_t PENDCH4:1;        /*!< bit:      4  Pending Channel 4                  */
    uint32_t PENDCH5:1;        /*!< bit:      5  Pending Channel 5                  */
    uint32_t PENDCH6:1;        /*!< bit:      6  Pending Channel 6                  */
    uint32_t PENDCH7:1;        /*!< bit:      7  Pending Channel 7                  */
    uint32_t PENDCH8:1;        /*!< bit:      8  Pending Channel 8                  */
    uint32_t PENDCH9:1;        /*!< bit:      9  Pending Channel 9                  */
    uint32_t PENDCH10:1;       /*!< bit:     10  Pending Channel 10                 */
    uint32_t PENDCH11:1;       /*!< bit:     11  Pending Channel 11                 */
    uint32_t PENDCH12:1;       /*!< bit:     12  Pending Channel 12                 */
    uint32_t PENDCH13:1;       /*!< bit:     13  Pending Channel 13                 */
    uint32_t PENDCH14:1;       /*!< bit:     14  Pending Channel 14                 */
    uint32_t PENDCH15:1;       /*!< bit:     15  Pending Channel 15                 */
    uint32_t PENDCH16:1;       /*!< bit:     16  Pending Channel 16                 */
    uint32_t PENDCH17:1;       /*!< bit:     17  Pending Channel 17                 */
    uint32_t PENDCH18:1;       /*!< bit:     18  Pending Channel 18                 */
    uint32_t PENDCH19:1;       /*!< bit:     19  Pending Channel 19                 */
    uint32_t PENDCH20:1;       /*!< bit:     20  Pending Channel 20                 */
    uint32_t PENDCH21:1;       /*!< bit:     21  Pending Channel 21                 */
    uint32_t PENDCH22:1;       /*!< bit:     22  Pending Channel 22                 */
    uint32_t PENDCH23:1;       /*!< bit:     23  Pending Channel 23                 */
    uint32_t PENDCH24:1;       /*!< bit:     24  Pending Channel 24                 */
    uint32_t PENDCH25:1;       /*!< bit:     25  Pending Channel 25                 */
    uint32_t PENDCH26:1;       /*!< bit:     26  Pending Channel 26                 */
    uint32_t PENDCH27:1;       /*!< bit:     27  Pending Channel 27                 */
    uint32_t PENDCH28:1;       /*!< bit:     28  Pending Channel 28                 */
    uint32_t PENDCH29:1;       /*!< bit:     29  Pending Channel 29                 */
    uint32_t PENDCH30:1;       /*!< bit:     30  Pending Channel 30                 */
    uint32_t PENDCH31:1;       /*!< bit:     31  Pending Channel 31                 */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t PENDCH:32;        /*!< bit:  0..31  Pending Channel x                  */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_PENDCH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_ACTIVE : (DMAC Offset: 0x30) ( R/ 32) Active Channel and Levels -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LVLEX0:1;         /*!< bit:      0  Level 0 Channel Trigger Request Executing */
    uint32_t LVLEX1:1;         /*!< bit:      1  Level 1 Channel Trigger Request Executing */
    uint32_t LVLEX2:1;         /*!< bit:      2  Level 2 Channel Trigger Request Executing */
    uint32_t LVLEX3:1;         /*!< bit:      3  Level 3 Channel Trigger Request Executing */
    uint32_t :4;               /*!< bit:  4.. 7  Reserved                           */
    uint32_t ID:5;             /*!< bit:  8..12  Active Channel ID                  */
    uint32_t :2;               /*!< bit: 13..14  Reserved                           */
    uint32_t ABUSY:1;          /*!< bit:     15  Active Channel Busy                */
    uint32_t BTCNT:16;         /*!< bit: 16..31  Active Channel Block Transfer Count */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t LVLEX:4;          /*!< bit:  0.. 3  Level x Channel Trigger Request Executing */
    uint32_t :28;              /*!< bit:  4..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_ACTIVE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_BASEADDR : (DMAC Offset: 0x34) (R/W 32) Descriptor Memory Section Base Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BASEADDR:32;      /*!< bit:  0..31  Descriptor Memory Base Address     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_BASEADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- DMAC_WRBADDR : (DMAC Offset: 0x38) (R/W 32) Write-Back Memory Section Base Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t WRBADDR:32;       /*!< bit:  0..31  Write-Back Memory Base Address     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} DMAC_WRBADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief DMAC Descriptor SRAM registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO DMAC_BTCTRL_Type          BTCTRL;      /**< \brief Offset: 0x00 (R/W 16) Block Transfer Control */
  __IO DMAC_BTCNT_Type           BTCNT;       /**< \brief Offset: 0x02 (R/W 16) Block Transfer Count */
  __IO DMAC_SRCADDR_Type         SRCADDR;     /**< \brief Offset: 0x04 (R/W 32) Block Transfer Source Address */
  __IO DMAC_DSTADDR_Type         DSTADDR;     /**< \brief Offset: 0x08 (R/W 32) Block Transfer Destination Address */
  __IO DMAC_DESCADDR_Type        DESCADDR;    /**< \brief Offset: 0x0C (R/W 32) Next Descriptor Address */
} DmacDescriptor
#ifdef __GNUC__
  __attribute__ ((aligned (8)))
#endif
;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief DmacChannel hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO DMAC_CHCTRLA_Type         CHCTRLA;     /**< \brief Offset: 0x00 (R/W 32) Channel n Control A */
  __IO DMAC_CHCTRLB_Type         CHCTRLB;     /**< \brief Offset: 0x04 (R/W  8) Channel n Control B */
  __IO DMAC_CHPRILVL_Type        CHPRILVL;    /**< \brief Offset: 0x05 (R/W  8) Channel n Priority Level */
  __IO DMAC_CHEVCTRL_Type        CHEVCTRL;    /**< \brief Offset: 0x06 (R/W  8) Channel n Event Control */
       RoReg8                    Reserved1[0x5];
  __IO DMAC_CHINTENCLR_Type      CHINTENCLR;  /**< \brief Offset: 0x0C (R/W  8) Channel n Interrupt Enable Clear */
  __IO DMAC_CHINTENSET_Type      CHINTENSET;  /**< \brief Offset: 0x0D (R/W  8) Channel n Interrupt Enable Set */
  __IO DMAC_CHINTFLAG_Type       CHINTFLAG;   /**< \brief Offset: 0x0E (R/W  8) Channel n Interrupt Flag Status and Clear */
  __IO DMAC_CHSTATUS_Type        CHSTATUS;    /**< \brief Offset: 0x0F (R/W  8) Channel n Status */
} DmacChannel;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief DMAC APB hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO DMAC_CTRL_Type            CTRL;        /**< \brief Offset: 0x00 (R/W 16) Control */
  __IO DMAC_CRCCTRL_Type         CRCCTRL;     /**< \brief Offset: 0x02 (R/W 16) CRC Control */
  __IO DMAC_CRCDATAIN_Type       CRCDATAIN;   /**< \brief Offset: 0x04 (R/W 32) CRC Data Input */
  __IO DMAC_CRCCHKSUM_Type       CRCCHKSUM;   /**< \brief Offset: 0x08 (R/W 32) CRC Checksum */
  __IO DMAC_CRCSTATUS_Type       CRCSTATUS;   /**< \brief Offset: 0x0C (R/W  8) CRC Status */
  __IO DMAC_DBGCTRL_Type         DBGCTRL;     /**< \brief Offset: 0x0D (R/W  8) Debug Control */
       RoReg8                    Reserved1[0x2];
  __IO DMAC_SWTRIGCTRL_Type      SWTRIGCTRL;  /**< \brief Offset: 0x10 (R/W 32) Software Trigger Control */
  __IO DMAC_PRICTRL0_Type        PRICTRL0;    /**< \brief Offset: 0x14 (R/W 32) Priority Control 0 */
       RoReg8                    Reserved2[0x8];
  __IO DMAC_INTPEND_Type         INTPEND;     /**< \brief Offset: 0x20 (R/W 16) Interrupt Pending */
       RoReg8                    Reserved3[0x2];
  __I  DMAC_INTSTATUS_Type       INTSTATUS;   /**< \brief Offset: 0x24 (R/  32) Interrupt Status */
  __I  DMAC_BUSYCH_Type          BUSYCH;      /**< \brief Offset: 0x28 (R/  32) Busy Channels */
  __I  DMAC_PENDCH_Type          PENDCH;      /**< \brief Offset: 0x2C (R/  32) Pending Channels */
  __I  DMAC_ACTIVE_Type          ACTIVE;      /**< \brief Offset: 0x30 (R/  32) Active Channel and Levels */
  __IO DMAC_BASEADDR_Type        BASEADDR;    /**< \brief Offset: 0x34 (R/W 32) Descriptor Memory Section Base Address */
  __IO DMAC_WRBADDR_Type         WRBADDR;     /**< \brief Offset: 0x38 (R/W 32) Write-Back Memory Section Base Address */
       RoReg8                    Reserved4[0x4];
       DmacChannel               Channel[DMAC_CHANNEL_NUMBER]; /**< \brief Offset: 0x40 DmacChannel groups [CH_NUM] */
} Dmac;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_DMAC_COMPONENT_FIXUP_H_ */
