/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_CAN_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_CAN_COMPONENT_FIXUP_H_

/* -------- CAN_RXBE_0 : (CAN Offset: 0x00) (R/W 32) Rx Buffer Element 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ID:29;            /*!< bit:  0..28  Identifier                         */
    uint32_t RTR:1;            /*!< bit:     29  Remote Transmission Request        */
    uint32_t XTD:1;            /*!< bit:     30  Extended Identifier                */
    uint32_t ESI:1;            /*!< bit:     31  Error State Indicator              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXBE_0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXBE_1 : (CAN Offset: 0x04) (R/W 32) Rx Buffer Element 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXTS:16;          /*!< bit:  0..15  Rx Timestamp                       */
    uint32_t DLC:4;            /*!< bit: 16..19  Data Length Code                   */
    uint32_t BRS:1;            /*!< bit:     20  Bit Rate Search                    */
    uint32_t FDF:1;            /*!< bit:     21  FD Format                          */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t FIDX:7;           /*!< bit: 24..30  Filter Index                       */
    uint32_t ANMF:1;           /*!< bit:     31  Accepted Non-matching Frame        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXBE_1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXBE_DATA : (CAN Offset: 0x08) (R/W 32) Rx Buffer Element Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DB0:8;            /*!< bit:  0.. 7  Data Byte 0                        */
    uint32_t DB1:8;            /*!< bit:  8..15  Data Byte 1                        */
    uint32_t DB2:8;            /*!< bit: 16..23  Data Byte 2                        */
    uint32_t DB3:8;            /*!< bit: 24..31  Data Byte 3                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXBE_DATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXF0E_0 : (CAN Offset: 0x00) (R/W 32) Rx FIFO 0 Element 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ID:29;            /*!< bit:  0..28  Identifier                         */
    uint32_t RTR:1;            /*!< bit:     29  Remote Transmission Request        */
    uint32_t XTD:1;            /*!< bit:     30  Extended Identifier                */
    uint32_t ESI:1;            /*!< bit:     31  Error State Indicator              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF0E_0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXF0E_1 : (CAN Offset: 0x04) (R/W 32) Rx FIFO 0 Element 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXTS:16;          /*!< bit:  0..15  Rx Timestamp                       */
    uint32_t DLC:4;            /*!< bit: 16..19  Data Length Code                   */
    uint32_t BRS:1;            /*!< bit:     20  Bit Rate Search                    */
    uint32_t FDF:1;            /*!< bit:     21  FD Format                          */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t FIDX:7;           /*!< bit: 24..30  Filter Index                       */
    uint32_t ANMF:1;           /*!< bit:     31  Accepted Non-matching Frame        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF0E_1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXF0E_DATA : (CAN Offset: 0x08) (R/W 32) Rx FIFO 0 Element Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DB0:8;            /*!< bit:  0.. 7  Data Byte 0                        */
    uint32_t DB1:8;            /*!< bit:  8..15  Data Byte 1                        */
    uint32_t DB2:8;            /*!< bit: 16..23  Data Byte 2                        */
    uint32_t DB3:8;            /*!< bit: 24..31  Data Byte 3                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF0E_DATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXF1E_0 : (CAN Offset: 0x00) (R/W 32) Rx FIFO 1 Element 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ID:29;            /*!< bit:  0..28  Identifier                         */
    uint32_t RTR:1;            /*!< bit:     29  Remote Transmission Request        */
    uint32_t XTD:1;            /*!< bit:     30  Extended Identifier                */
    uint32_t ESI:1;            /*!< bit:     31  Error State Indicator              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF1E_0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXF1E_1 : (CAN Offset: 0x04) (R/W 32) Rx FIFO 1 Element 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXTS:16;          /*!< bit:  0..15  Rx Timestamp                       */
    uint32_t DLC:4;            /*!< bit: 16..19  Data Length Code                   */
    uint32_t BRS:1;            /*!< bit:     20  Bit Rate Search                    */
    uint32_t FDF:1;            /*!< bit:     21  FD Format                          */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t FIDX:7;           /*!< bit: 24..30  Filter Index                       */
    uint32_t ANMF:1;           /*!< bit:     31  Accepted Non-matching Frame        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF1E_1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXF1E_DATA : (CAN Offset: 0x08) (R/W 32) Rx FIFO 1 Element Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DB0:8;            /*!< bit:  0.. 7  Data Byte 0                        */
    uint32_t DB1:8;            /*!< bit:  8..15  Data Byte 1                        */
    uint32_t DB2:8;            /*!< bit: 16..23  Data Byte 2                        */
    uint32_t DB3:8;            /*!< bit: 24..31  Data Byte 3                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF1E_DATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXBE_0 : (CAN Offset: 0x00) (R/W 32) Tx Buffer Element 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ID:29;            /*!< bit:  0..28  Identifier                         */
    uint32_t RTR:1;            /*!< bit:     29  Remote Transmission Request        */
    uint32_t XTD:1;            /*!< bit:     30  Extended Identifier                */
    uint32_t ESI:1;            /*!< bit:     31  Error State Indicator              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBE_0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXBE_1 : (CAN Offset: 0x04) (R/W 32) Tx Buffer Element 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :16;              /*!< bit:  0..15  Reserved                           */
    uint32_t DLC:4;            /*!< bit: 16..19  Identifier                         */
    uint32_t BRS:1;            /*!< bit:     20  Bit Rate Search                    */
    uint32_t FDF:1;            /*!< bit:     21  FD Format                          */
    uint32_t :1;               /*!< bit:     22  Reserved                           */
    uint32_t EFC:1;            /*!< bit:     23  Event FIFO Control                 */
    uint32_t MM:8;             /*!< bit: 24..31  Message Marker                     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBE_1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXBE_DATA : (CAN Offset: 0x08) (R/W 32) Tx Buffer Element Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DB0:8;            /*!< bit:  0.. 7  Data Byte 0                        */
    uint32_t DB1:8;            /*!< bit:  8..15  Data Byte 1                        */
    uint32_t DB2:8;            /*!< bit: 16..23  Data Byte 2                        */
    uint32_t DB3:8;            /*!< bit: 24..31  Data Byte 3                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBE_DATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXEFE_0 : (CAN Offset: 0x00) (R/W 32) Tx Event FIFO Element 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ID:29;            /*!< bit:  0..28  Identifier                         */
    uint32_t RTR:1;            /*!< bit:     29  Remote Transmission Request        */
    uint32_t XTD:1;            /*!< bit:     30  Extended Indentifier               */
    uint32_t ESI:1;            /*!< bit:     31  Error State Indicator              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXEFE_0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXEFE_1 : (CAN Offset: 0x04) (R/W 32) Tx Event FIFO Element 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TXTS:16;          /*!< bit:  0..15  Tx Timestamp                       */
    uint32_t DLC:4;            /*!< bit: 16..19  Data Length Code                   */
    uint32_t BRS:1;            /*!< bit:     20  Bit Rate Search                    */
    uint32_t FDF:1;            /*!< bit:     21  FD Format                          */
    uint32_t ET:2;             /*!< bit: 22..23  Event Type                         */
    uint32_t MM:8;             /*!< bit: 24..31  Message Marker                     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXEFE_1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_SIDFE_0 : (CAN Offset: 0x00) (R/W 32) Standard Message ID Filter Element 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SFID2:11;         /*!< bit:  0..10  Standard Filter ID 2               */
    uint32_t :5;               /*!< bit: 11..15  Reserved                           */
    uint32_t SFID1:11;         /*!< bit: 16..26  Standard Filter ID 1               */
    uint32_t SFEC:3;           /*!< bit: 27..29  Standard Filter Element Configuration */
    uint32_t SFT:2;            /*!< bit: 30..31  Standard Filter Type               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_SIDFE_0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_XIDFE_0 : (CAN Offset: 0x00) (R/W 32) Extended Message ID Filter Element 0 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EFID1:29;         /*!< bit:  0..28  Extended Filter ID 1               */
    uint32_t EFEC:3;           /*!< bit: 29..31  Extended Filter Element Configuration */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_XIDFE_0_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_XIDFE_1 : (CAN Offset: 0x04) (R/W 32) Extended Message ID Filter Element 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EFID2:29;         /*!< bit:  0..28  Extended Filter ID 2               */
    uint32_t :1;               /*!< bit:     29  Reserved                           */
    uint32_t EFT:2;            /*!< bit: 30..31  Extended Filter Type               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_XIDFE_1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_CREL : (CAN Offset: 0x00) ( R/ 32) Core Release -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :20;              /*!< bit:  0..19  Reserved                           */
    uint32_t SUBSTEP:4;        /*!< bit: 20..23  Sub-step of Core Release           */
    uint32_t STEP:4;           /*!< bit: 24..27  Step of Core Release               */
    uint32_t REL:4;            /*!< bit: 28..31  Core Release                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_CREL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_ENDN : (CAN Offset: 0x04) ( R/ 32) Endian -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ETV:32;           /*!< bit:  0..31  Endianness Test Value              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_ENDN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CAN_MRCFG : (CAN Offset: 0x08) (R/W 32) Message RAM Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t QOS:2;            /*!< bit:  0.. 1  Quality of Service                 */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_MRCFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_DBTP : (CAN Offset: 0x0C) (R/W 32) Fast Bit Timing and Prescaler -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DSJW:4;           /*!< bit:  0.. 3  Data (Re)Synchronization Jump Width */
    uint32_t DTSEG2:4;         /*!< bit:  4.. 7  Data time segment after sample point */
    uint32_t DTSEG1:5;         /*!< bit:  8..12  Data time segment before sample point */
    uint32_t :3;               /*!< bit: 13..15  Reserved                           */
    uint32_t DBRP:5;           /*!< bit: 16..20  Data Baud Rate Prescaler           */
    uint32_t :2;               /*!< bit: 21..22  Reserved                           */
    uint32_t TDC:1;            /*!< bit:     23  Tranceiver Delay Compensation      */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_DBTP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CAN_TEST : (CAN Offset: 0x10) (R/W 32) Test -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :4;               /*!< bit:  0.. 3  Reserved                           */
    uint32_t LBCK:1;           /*!< bit:      4  Loop Back Mode                     */
    uint32_t TX:2;             /*!< bit:  5.. 6  Control of Transmit Pin            */
    uint32_t RX:1;             /*!< bit:      7  Receive Pin                        */
    uint32_t :24;              /*!< bit:  8..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TEST_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RWD : (CAN Offset: 0x14) (R/W 32) RAM Watchdog -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t WDC:8;            /*!< bit:  0.. 7  Watchdog Configuration             */
    uint32_t WDV:8;            /*!< bit:  8..15  Watchdog Value                     */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RWD_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_CCCR : (CAN Offset: 0x18) (R/W 32) CC Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t INIT:1;           /*!< bit:      0  Initialization                     */
    uint32_t CCE:1;            /*!< bit:      1  Configuration Change Enable        */
    uint32_t ASM:1;            /*!< bit:      2  ASM Restricted Operation Mode      */
    uint32_t CSA:1;            /*!< bit:      3  Clock Stop Acknowledge             */
    uint32_t CSR:1;            /*!< bit:      4  Clock Stop Request                 */
    uint32_t MON:1;            /*!< bit:      5  Bus Monitoring Mode                */
    uint32_t DAR:1;            /*!< bit:      6  Disable Automatic Retransmission   */
    uint32_t TEST:1;           /*!< bit:      7  Test Mode Enable                   */
    uint32_t FDOE:1;           /*!< bit:      8  FD Operation Enable                */
    uint32_t BRSE:1;           /*!< bit:      9  Bit Rate Switch Enable             */
    uint32_t :2;               /*!< bit: 10..11  Reserved                           */
    uint32_t PXHD:1;           /*!< bit:     12  Protocol Exception Handling Disable */
    uint32_t EFBI:1;           /*!< bit:     13  Edge Filtering during Bus Integration */
    uint32_t TXP:1;            /*!< bit:     14  Transmit Pause                     */
    uint32_t NISO:1;           /*!< bit:     15  Non ISO Operation                  */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_CCCR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CAN_NBTP : (CAN Offset: 0x1C) (R/W 32) Nominal Bit Timing and Prescaler -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NTSEG2:7;         /*!< bit:  0.. 6  Nominal Time segment after sample point */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t NTSEG1:8;         /*!< bit:  8..15  Nominal Time segment before sample point */
    uint32_t NBRP:9;           /*!< bit: 16..24  Nominal Baud Rate Prescaler        */
    uint32_t NSJW:7;           /*!< bit: 25..31  Nominal (Re)Synchronization Jump Width */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_NBTP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TSCC : (CAN Offset: 0x20) (R/W 32) Timestamp Counter Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TSS:2;            /*!< bit:  0.. 1  Timestamp Select                   */
    uint32_t :14;              /*!< bit:  2..15  Reserved                           */
    uint32_t TCP:4;            /*!< bit: 16..19  Timestamp Counter Prescaler        */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TSCC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TSCV : (CAN Offset: 0x24) ( R/ 32) Timestamp Counter Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TSC:16;           /*!< bit:  0..15  Timestamp Counter                  */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TSCV_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TOCC : (CAN Offset: 0x28) (R/W 32) Timeout Counter Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ETOC:1;           /*!< bit:      0  Enable Timeout Counter             */
    uint32_t TOS:2;            /*!< bit:  1.. 2  Timeout Select                     */
    uint32_t :13;              /*!< bit:  3..15  Reserved                           */
    uint32_t TOP:16;           /*!< bit: 16..31  Timeout Period                     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TOCC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TOCV : (CAN Offset: 0x2C) (R/W 32) Timeout Counter Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TOC:16;           /*!< bit:  0..15  Timeout Counter                    */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TOCV_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CAN_ECR : (CAN Offset: 0x40) ( R/ 32) Error Counter -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TEC:8;            /*!< bit:  0.. 7  Transmit Error Counter             */
    uint32_t REC:7;            /*!< bit:  8..14  Receive Error Counter              */
    uint32_t RP:1;             /*!< bit:     15  Receive Error Passive              */
    uint32_t CEL:8;            /*!< bit: 16..23  CAN Error Logging                  */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_ECR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CAN_PSR : (CAN Offset: 0x44) ( R/ 32) Protocol Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t LEC:3;            /*!< bit:  0.. 2  Last Error Code                    */
    uint32_t ACT:2;            /*!< bit:  3.. 4  Activity                           */
    uint32_t EP:1;             /*!< bit:      5  Error Passive                      */
    uint32_t EW:1;             /*!< bit:      6  Warning Status                     */
    uint32_t BO:1;             /*!< bit:      7  Bus_Off Status                     */
    uint32_t DLEC:3;           /*!< bit:  8..10  Data Phase Last Error Code         */
    uint32_t RESI:1;           /*!< bit:     11  ESI flag of last received CAN FD Message */
    uint32_t RBRS:1;           /*!< bit:     12  BRS flag of last received CAN FD Message */
    uint32_t RFDF:1;           /*!< bit:     13  Received a CAN FD Message          */
    uint32_t PXE:1;            /*!< bit:     14  Protocol Exception Event           */
    uint32_t :1;               /*!< bit:     15  Reserved                           */
    uint32_t TDCV:7;           /*!< bit: 16..22  Transmitter Delay Compensation Value */
    uint32_t :9;               /*!< bit: 23..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_PSR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CAN_TDCR : (CAN Offset: 0x48) (R/W 32) Extended ID Filter Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TDCF:7;           /*!< bit:  0.. 6  Transmitter Delay Compensation Filter Length */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t TDCO:7;           /*!< bit:  8..14  Transmitter Delay Compensation Offset */
    uint32_t :17;              /*!< bit: 15..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TDCR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CAN_IR : (CAN Offset: 0x50) (R/W 32) Interrupt -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RF0N:1;           /*!< bit:      0  Rx FIFO 0 New Message              */
    uint32_t RF0W:1;           /*!< bit:      1  Rx FIFO 0 Watermark Reached        */
    uint32_t RF0F:1;           /*!< bit:      2  Rx FIFO 0 Full                     */
    uint32_t RF0L:1;           /*!< bit:      3  Rx FIFO 0 Message Lost             */
    uint32_t RF1N:1;           /*!< bit:      4  Rx FIFO 1 New Message              */
    uint32_t RF1W:1;           /*!< bit:      5  Rx FIFO 1 Watermark Reached        */
    uint32_t RF1F:1;           /*!< bit:      6  Rx FIFO 1 FIFO Full                */
    uint32_t RF1L:1;           /*!< bit:      7  Rx FIFO 1 Message Lost             */
    uint32_t HPM:1;            /*!< bit:      8  High Priority Message              */
    uint32_t TC:1;             /*!< bit:      9  Timestamp Completed                */
    uint32_t TCF:1;            /*!< bit:     10  Transmission Cancellation Finished */
    uint32_t TFE:1;            /*!< bit:     11  Tx FIFO Empty                      */
    uint32_t TEFN:1;           /*!< bit:     12  Tx Event FIFO New Entry            */
    uint32_t TEFW:1;           /*!< bit:     13  Tx Event FIFO Watermark Reached    */
    uint32_t TEFF:1;           /*!< bit:     14  Tx Event FIFO Full                 */
    uint32_t TEFL:1;           /*!< bit:     15  Tx Event FIFO Element Lost         */
    uint32_t TSW:1;            /*!< bit:     16  Timestamp Wraparound               */
    uint32_t MRAF:1;           /*!< bit:     17  Message RAM Access Failure         */
    uint32_t TOO:1;            /*!< bit:     18  Timeout Occurred                   */
    uint32_t DRX:1;            /*!< bit:     19  Message stored to Dedicated Rx Buffer */
    uint32_t BEC:1;            /*!< bit:     20  Bit Error Corrected                */
    uint32_t BEU:1;            /*!< bit:     21  Bit Error Uncorrected              */
    uint32_t ELO:1;            /*!< bit:     22  Error Logging Overflow             */
    uint32_t EP:1;             /*!< bit:     23  Error Passive                      */
    uint32_t EW:1;             /*!< bit:     24  Warning Status                     */
    uint32_t BO:1;             /*!< bit:     25  Bus_Off Status                     */
    uint32_t WDI:1;            /*!< bit:     26  Watchdog Interrupt                 */
    uint32_t PEA:1;            /*!< bit:     27  Protocol Error in Arbitration Phase */
    uint32_t PED:1;            /*!< bit:     28  Protocol Error in Data Phase       */
    uint32_t ARA:1;            /*!< bit:     29  Access to Reserved Address         */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_IR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_IE : (CAN Offset: 0x54) (R/W 32) Interrupt Enable -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RF0NE:1;          /*!< bit:      0  Rx FIFO 0 New Message Interrupt Enable */
    uint32_t RF0WE:1;          /*!< bit:      1  Rx FIFO 0 Watermark Reached Interrupt Enable */
    uint32_t RF0FE:1;          /*!< bit:      2  Rx FIFO 0 Full Interrupt Enable    */
    uint32_t RF0LE:1;          /*!< bit:      3  Rx FIFO 0 Message Lost Interrupt Enable */
    uint32_t RF1NE:1;          /*!< bit:      4  Rx FIFO 1 New Message Interrupt Enable */
    uint32_t RF1WE:1;          /*!< bit:      5  Rx FIFO 1 Watermark Reached Interrupt Enable */
    uint32_t RF1FE:1;          /*!< bit:      6  Rx FIFO 1 FIFO Full Interrupt Enable */
    uint32_t RF1LE:1;          /*!< bit:      7  Rx FIFO 1 Message Lost Interrupt Enable */
    uint32_t HPME:1;           /*!< bit:      8  High Priority Message Interrupt Enable */
    uint32_t TCE:1;            /*!< bit:      9  Timestamp Completed Interrupt Enable */
    uint32_t TCFE:1;           /*!< bit:     10  Transmission Cancellation Finished Interrupt Enable */
    uint32_t TFEE:1;           /*!< bit:     11  Tx FIFO Empty Interrupt Enable     */
    uint32_t TEFNE:1;          /*!< bit:     12  Tx Event FIFO New Entry Interrupt Enable */
    uint32_t TEFWE:1;          /*!< bit:     13  Tx Event FIFO Watermark Reached Interrupt Enable */
    uint32_t TEFFE:1;          /*!< bit:     14  Tx Event FIFO Full Interrupt Enable */
    uint32_t TEFLE:1;          /*!< bit:     15  Tx Event FIFO Element Lost Interrupt Enable */
    uint32_t TSWE:1;           /*!< bit:     16  Timestamp Wraparound Interrupt Enable */
    uint32_t MRAFE:1;          /*!< bit:     17  Message RAM Access Failure Interrupt Enable */
    uint32_t TOOE:1;           /*!< bit:     18  Timeout Occurred Interrupt Enable  */
    uint32_t DRXE:1;           /*!< bit:     19  Message stored to Dedicated Rx Buffer Interrupt Enable */
    uint32_t BECE:1;           /*!< bit:     20  Bit Error Corrected Interrupt Enable */
    uint32_t BEUE:1;           /*!< bit:     21  Bit Error Uncorrected Interrupt Enable */
    uint32_t ELOE:1;           /*!< bit:     22  Error Logging Overflow Interrupt Enable */
    uint32_t EPE:1;            /*!< bit:     23  Error Passive Interrupt Enable     */
    uint32_t EWE:1;            /*!< bit:     24  Warning Status Interrupt Enable    */
    uint32_t BOE:1;            /*!< bit:     25  Bus_Off Status Interrupt Enable    */
    uint32_t WDIE:1;           /*!< bit:     26  Watchdog Interrupt Interrupt Enable */
    uint32_t PEAE:1;           /*!< bit:     27  Protocol Error in Arbitration Phase Enable */
    uint32_t PEDE:1;           /*!< bit:     28  Protocol Error in Data Phase Enable */
    uint32_t ARAE:1;           /*!< bit:     29  Access to Reserved Address Enable  */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_IE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CAN_ILS : (CAN Offset: 0x58) (R/W 32) Interrupt Line Select -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RF0NL:1;          /*!< bit:      0  Rx FIFO 0 New Message Interrupt Line */
    uint32_t RF0WL:1;          /*!< bit:      1  Rx FIFO 0 Watermark Reached Interrupt Line */
    uint32_t RF0FL:1;          /*!< bit:      2  Rx FIFO 0 Full Interrupt Line      */
    uint32_t RF0LL:1;          /*!< bit:      3  Rx FIFO 0 Message Lost Interrupt Line */
    uint32_t RF1NL:1;          /*!< bit:      4  Rx FIFO 1 New Message Interrupt Line */
    uint32_t RF1WL:1;          /*!< bit:      5  Rx FIFO 1 Watermark Reached Interrupt Line */
    uint32_t RF1FL:1;          /*!< bit:      6  Rx FIFO 1 FIFO Full Interrupt Line */
    uint32_t RF1LL:1;          /*!< bit:      7  Rx FIFO 1 Message Lost Interrupt Line */
    uint32_t HPML:1;           /*!< bit:      8  High Priority Message Interrupt Line */
    uint32_t TCL:1;            /*!< bit:      9  Timestamp Completed Interrupt Line */
    uint32_t TCFL:1;           /*!< bit:     10  Transmission Cancellation Finished Interrupt Line */
    uint32_t TFEL:1;           /*!< bit:     11  Tx FIFO Empty Interrupt Line       */
    uint32_t TEFNL:1;          /*!< bit:     12  Tx Event FIFO New Entry Interrupt Line */
    uint32_t TEFWL:1;          /*!< bit:     13  Tx Event FIFO Watermark Reached Interrupt Line */
    uint32_t TEFFL:1;          /*!< bit:     14  Tx Event FIFO Full Interrupt Line  */
    uint32_t TEFLL:1;          /*!< bit:     15  Tx Event FIFO Element Lost Interrupt Line */
    uint32_t TSWL:1;           /*!< bit:     16  Timestamp Wraparound Interrupt Line */
    uint32_t MRAFL:1;          /*!< bit:     17  Message RAM Access Failure Interrupt Line */
    uint32_t TOOL:1;           /*!< bit:     18  Timeout Occurred Interrupt Line    */
    uint32_t DRXL:1;           /*!< bit:     19  Message stored to Dedicated Rx Buffer Interrupt Line */
    uint32_t BECL:1;           /*!< bit:     20  Bit Error Corrected Interrupt Line */
    uint32_t BEUL:1;           /*!< bit:     21  Bit Error Uncorrected Interrupt Line */
    uint32_t ELOL:1;           /*!< bit:     22  Error Logging Overflow Interrupt Line */
    uint32_t EPL:1;            /*!< bit:     23  Error Passive Interrupt Line       */
    uint32_t EWL:1;            /*!< bit:     24  Warning Status Interrupt Line      */
    uint32_t BOL:1;            /*!< bit:     25  Bus_Off Status Interrupt Line      */
    uint32_t WDIL:1;           /*!< bit:     26  Watchdog Interrupt Interrupt Line  */
    uint32_t PEAL:1;           /*!< bit:     27  Protocol Error in Arbitration Phase Line */
    uint32_t PEDL:1;           /*!< bit:     28  Protocol Error in Data Phase Line  */
    uint32_t ARAL:1;           /*!< bit:     29  Access to Reserved Address Line    */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_ILS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_ILE : (CAN Offset: 0x5C) (R/W 32) Interrupt Line Enable -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EINT0:1;          /*!< bit:      0  Enable Interrupt Line 0            */
    uint32_t EINT1:1;          /*!< bit:      1  Enable Interrupt Line 1            */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_ILE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_GFC : (CAN Offset: 0x80) (R/W 32) Global Filter Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RRFE:1;           /*!< bit:      0  Reject Remote Frames Extended      */
    uint32_t RRFS:1;           /*!< bit:      1  Reject Remote Frames Standard      */
    uint32_t ANFE:2;           /*!< bit:  2.. 3  Accept Non-matching Frames Extended */
    uint32_t ANFS:2;           /*!< bit:  4.. 5  Accept Non-matching Frames Standard */
    uint32_t :26;              /*!< bit:  6..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_GFC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_SIDFC : (CAN Offset: 0x84) (R/W 32) Standard ID Filter Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FLSSA:16;         /*!< bit:  0..15  Filter List Standard Start Address */
    uint32_t LSS:8;            /*!< bit: 16..23  List Size Standard                 */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_SIDFC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_XIDFC : (CAN Offset: 0x88) (R/W 32) Extended ID Filter Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t FLESA:16;         /*!< bit:  0..15  Filter List Extended Start Address */
    uint32_t LSE:7;            /*!< bit: 16..22  List Size Extended                 */
    uint32_t :9;               /*!< bit: 23..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_XIDFC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_XIDAM : (CAN Offset: 0x90) (R/W 32) Extended ID AND Mask -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EIDM:29;          /*!< bit:  0..28  Extended ID Mask                   */
    uint32_t :3;               /*!< bit: 29..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_XIDAM_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_HPMS : (CAN Offset: 0x94) ( R/ 32) High Priority Message Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BIDX:6;           /*!< bit:  0.. 5  Buffer Index                       */
    uint32_t MSI:2;            /*!< bit:  6.. 7  Message Storage Indicator          */
    uint32_t FIDX:7;           /*!< bit:  8..14  Filter Index                       */
    uint32_t FLST:1;           /*!< bit:     15  Filter List                        */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_HPMS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_NDAT1 : (CAN Offset: 0x98) (R/W 32) New Data 1 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ND0:1;            /*!< bit:      0  New Data 0                         */
    uint32_t ND1:1;            /*!< bit:      1  New Data 1                         */
    uint32_t ND2:1;            /*!< bit:      2  New Data 2                         */
    uint32_t ND3:1;            /*!< bit:      3  New Data 3                         */
    uint32_t ND4:1;            /*!< bit:      4  New Data 4                         */
    uint32_t ND5:1;            /*!< bit:      5  New Data 5                         */
    uint32_t ND6:1;            /*!< bit:      6  New Data 6                         */
    uint32_t ND7:1;            /*!< bit:      7  New Data 7                         */
    uint32_t ND8:1;            /*!< bit:      8  New Data 8                         */
    uint32_t ND9:1;            /*!< bit:      9  New Data 9                         */
    uint32_t ND10:1;           /*!< bit:     10  New Data 10                        */
    uint32_t ND11:1;           /*!< bit:     11  New Data 11                        */
    uint32_t ND12:1;           /*!< bit:     12  New Data 12                        */
    uint32_t ND13:1;           /*!< bit:     13  New Data 13                        */
    uint32_t ND14:1;           /*!< bit:     14  New Data 14                        */
    uint32_t ND15:1;           /*!< bit:     15  New Data 15                        */
    uint32_t ND16:1;           /*!< bit:     16  New Data 16                        */
    uint32_t ND17:1;           /*!< bit:     17  New Data 17                        */
    uint32_t ND18:1;           /*!< bit:     18  New Data 18                        */
    uint32_t ND19:1;           /*!< bit:     19  New Data 19                        */
    uint32_t ND20:1;           /*!< bit:     20  New Data 20                        */
    uint32_t ND21:1;           /*!< bit:     21  New Data 21                        */
    uint32_t ND22:1;           /*!< bit:     22  New Data 22                        */
    uint32_t ND23:1;           /*!< bit:     23  New Data 23                        */
    uint32_t ND24:1;           /*!< bit:     24  New Data 24                        */
    uint32_t ND25:1;           /*!< bit:     25  New Data 25                        */
    uint32_t ND26:1;           /*!< bit:     26  New Data 26                        */
    uint32_t ND27:1;           /*!< bit:     27  New Data 27                        */
    uint32_t ND28:1;           /*!< bit:     28  New Data 28                        */
    uint32_t ND29:1;           /*!< bit:     29  New Data 29                        */
    uint32_t ND30:1;           /*!< bit:     30  New Data 30                        */
    uint32_t ND31:1;           /*!< bit:     31  New Data 31                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_NDAT1_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_NDAT2 : (CAN Offset: 0x9C) (R/W 32) New Data 2 -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ND32:1;           /*!< bit:      0  New Data 32                        */
    uint32_t ND33:1;           /*!< bit:      1  New Data 33                        */
    uint32_t ND34:1;           /*!< bit:      2  New Data 34                        */
    uint32_t ND35:1;           /*!< bit:      3  New Data 35                        */
    uint32_t ND36:1;           /*!< bit:      4  New Data 36                        */
    uint32_t ND37:1;           /*!< bit:      5  New Data 37                        */
    uint32_t ND38:1;           /*!< bit:      6  New Data 38                        */
    uint32_t ND39:1;           /*!< bit:      7  New Data 39                        */
    uint32_t ND40:1;           /*!< bit:      8  New Data 40                        */
    uint32_t ND41:1;           /*!< bit:      9  New Data 41                        */
    uint32_t ND42:1;           /*!< bit:     10  New Data 42                        */
    uint32_t ND43:1;           /*!< bit:     11  New Data 43                        */
    uint32_t ND44:1;           /*!< bit:     12  New Data 44                        */
    uint32_t ND45:1;           /*!< bit:     13  New Data 45                        */
    uint32_t ND46:1;           /*!< bit:     14  New Data 46                        */
    uint32_t ND47:1;           /*!< bit:     15  New Data 47                        */
    uint32_t ND48:1;           /*!< bit:     16  New Data 48                        */
    uint32_t ND49:1;           /*!< bit:     17  New Data 49                        */
    uint32_t ND50:1;           /*!< bit:     18  New Data 50                        */
    uint32_t ND51:1;           /*!< bit:     19  New Data 51                        */
    uint32_t ND52:1;           /*!< bit:     20  New Data 52                        */
    uint32_t ND53:1;           /*!< bit:     21  New Data 53                        */
    uint32_t ND54:1;           /*!< bit:     22  New Data 54                        */
    uint32_t ND55:1;           /*!< bit:     23  New Data 55                        */
    uint32_t ND56:1;           /*!< bit:     24  New Data 56                        */
    uint32_t ND57:1;           /*!< bit:     25  New Data 57                        */
    uint32_t ND58:1;           /*!< bit:     26  New Data 58                        */
    uint32_t ND59:1;           /*!< bit:     27  New Data 59                        */
    uint32_t ND60:1;           /*!< bit:     28  New Data 60                        */
    uint32_t ND61:1;           /*!< bit:     29  New Data 61                        */
    uint32_t ND62:1;           /*!< bit:     30  New Data 62                        */
    uint32_t ND63:1;           /*!< bit:     31  New Data 63                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_NDAT2_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXF0C : (CAN Offset: 0xA0) (R/W 32) Rx FIFO 0 Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t F0SA:16;          /*!< bit:  0..15  Rx FIFO 0 Start Address            */
    uint32_t F0S:7;            /*!< bit: 16..22  Rx FIFO 0 Size                     */
    uint32_t :1;               /*!< bit:     23  Reserved                           */
    uint32_t F0WM:7;           /*!< bit: 24..30  Rx FIFO 0 Watermark                */
    uint32_t F0OM:1;           /*!< bit:     31  FIFO 0 Operation Mode              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF0C_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CAN_RXF0S : (CAN Offset: 0xA4) ( R/ 32) Rx FIFO 0 Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t F0FL:7;           /*!< bit:  0.. 6  Rx FIFO 0 Fill Level               */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t F0GI:6;           /*!< bit:  8..13  Rx FIFO 0 Get Index                */
    uint32_t :2;               /*!< bit: 14..15  Reserved                           */
    uint32_t F0PI:6;           /*!< bit: 16..21  Rx FIFO 0 Put Index                */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t F0F:1;            /*!< bit:     24  Rx FIFO 0 Full                     */
    uint32_t RF0L:1;           /*!< bit:     25  Rx FIFO 0 Message Lost             */
    uint32_t :6;               /*!< bit: 26..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF0S_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXF0A : (CAN Offset: 0xA8) (R/W 32) Rx FIFO 0 Acknowledge -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t F0AI:6;           /*!< bit:  0.. 5  Rx FIFO 0 Acknowledge Index        */
    uint32_t :26;              /*!< bit:  6..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF0A_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXBC : (CAN Offset: 0xAC) (R/W 32) Rx Buffer Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RBSA:16;          /*!< bit:  0..15  Rx Buffer Start Address            */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXBC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXF1C : (CAN Offset: 0xB0) (R/W 32) Rx FIFO 1 Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t F1SA:16;          /*!< bit:  0..15  Rx FIFO 1 Start Address            */
    uint32_t F1S:7;            /*!< bit: 16..22  Rx FIFO 1 Size                     */
    uint32_t :1;               /*!< bit:     23  Reserved                           */
    uint32_t F1WM:7;           /*!< bit: 24..30  Rx FIFO 1 Watermark                */
    uint32_t F1OM:1;           /*!< bit:     31  FIFO 1 Operation Mode              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF1C_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXF1S : (CAN Offset: 0xB4) ( R/ 32) Rx FIFO 1 Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t F1FL:7;           /*!< bit:  0.. 6  Rx FIFO 1 Fill Level               */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t F1GI:6;           /*!< bit:  8..13  Rx FIFO 1 Get Index                */
    uint32_t :2;               /*!< bit: 14..15  Reserved                           */
    uint32_t F1PI:6;           /*!< bit: 16..21  Rx FIFO 1 Put Index                */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t F1F:1;            /*!< bit:     24  Rx FIFO 1 Full                     */
    uint32_t RF1L:1;           /*!< bit:     25  Rx FIFO 1 Message Lost             */
    uint32_t :4;               /*!< bit: 26..29  Reserved                           */
    uint32_t DMS:2;            /*!< bit: 30..31  Debug Message Status               */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF1S_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_RXF1A : (CAN Offset: 0xB8) (R/W 32) Rx FIFO 1 Acknowledge -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t F1AI:6;           /*!< bit:  0.. 5  Rx FIFO 1 Acknowledge Index        */
    uint32_t :26;              /*!< bit:  6..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXF1A_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- CAN_RXESC : (CAN Offset: 0xBC) (R/W 32) Rx Buffer / FIFO Element Size Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t F0DS:3;           /*!< bit:  0.. 2  Rx FIFO 0 Data Field Size          */
    uint32_t :1;               /*!< bit:      3  Reserved                           */
    uint32_t F1DS:3;           /*!< bit:  4.. 6  Rx FIFO 1 Data Field Size          */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t RBDS:3;           /*!< bit:  8..10  Rx Buffer Data Field Size          */
    uint32_t :21;              /*!< bit: 11..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_RXESC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXBC : (CAN Offset: 0xC0) (R/W 32) Tx Buffer Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TBSA:16;          /*!< bit:  0..15  Tx Buffers Start Address           */
    uint32_t NDTB:6;           /*!< bit: 16..21  Number of Dedicated Transmit Buffers */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t TFQS:6;           /*!< bit: 24..29  Transmit FIFO/Queue Size           */
    uint32_t TFQM:1;           /*!< bit:     30  Tx FIFO/Queue Mode                 */
    uint32_t :1;               /*!< bit:     31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXFQS : (CAN Offset: 0xC4) ( R/ 32) Tx FIFO / Queue Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TFFL:6;           /*!< bit:  0.. 5  Tx FIFO Free Level                 */
    uint32_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint32_t TFGI:5;           /*!< bit:  8..12  Tx FIFO Get Index                  */
    uint32_t :3;               /*!< bit: 13..15  Reserved                           */
    uint32_t TFQPI:5;          /*!< bit: 16..20  Tx FIFO/Queue Put Index            */
    uint32_t TFQF:1;           /*!< bit:     21  Tx FIFO/Queue Full                 */
    uint32_t :10;              /*!< bit: 22..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXFQS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXESC : (CAN Offset: 0xC8) (R/W 32) Tx Buffer Element Size Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TBDS:3;           /*!< bit:  0.. 2  Tx Buffer Data Field Size          */
    uint32_t :29;              /*!< bit:  3..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXESC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXBRP : (CAN Offset: 0xCC) ( R/ 32) Tx Buffer Request Pending -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TRP0:1;           /*!< bit:      0  Transmission Request Pending 0     */
    uint32_t TRP1:1;           /*!< bit:      1  Transmission Request Pending 1     */
    uint32_t TRP2:1;           /*!< bit:      2  Transmission Request Pending 2     */
    uint32_t TRP3:1;           /*!< bit:      3  Transmission Request Pending 3     */
    uint32_t TRP4:1;           /*!< bit:      4  Transmission Request Pending 4     */
    uint32_t TRP5:1;           /*!< bit:      5  Transmission Request Pending 5     */
    uint32_t TRP6:1;           /*!< bit:      6  Transmission Request Pending 6     */
    uint32_t TRP7:1;           /*!< bit:      7  Transmission Request Pending 7     */
    uint32_t TRP8:1;           /*!< bit:      8  Transmission Request Pending 8     */
    uint32_t TRP9:1;           /*!< bit:      9  Transmission Request Pending 9     */
    uint32_t TRP10:1;          /*!< bit:     10  Transmission Request Pending 10    */
    uint32_t TRP11:1;          /*!< bit:     11  Transmission Request Pending 11    */
    uint32_t TRP12:1;          /*!< bit:     12  Transmission Request Pending 12    */
    uint32_t TRP13:1;          /*!< bit:     13  Transmission Request Pending 13    */
    uint32_t TRP14:1;          /*!< bit:     14  Transmission Request Pending 14    */
    uint32_t TRP15:1;          /*!< bit:     15  Transmission Request Pending 15    */
    uint32_t TRP16:1;          /*!< bit:     16  Transmission Request Pending 16    */
    uint32_t TRP17:1;          /*!< bit:     17  Transmission Request Pending 17    */
    uint32_t TRP18:1;          /*!< bit:     18  Transmission Request Pending 18    */
    uint32_t TRP19:1;          /*!< bit:     19  Transmission Request Pending 19    */
    uint32_t TRP20:1;          /*!< bit:     20  Transmission Request Pending 20    */
    uint32_t TRP21:1;          /*!< bit:     21  Transmission Request Pending 21    */
    uint32_t TRP22:1;          /*!< bit:     22  Transmission Request Pending 22    */
    uint32_t TRP23:1;          /*!< bit:     23  Transmission Request Pending 23    */
    uint32_t TRP24:1;          /*!< bit:     24  Transmission Request Pending 24    */
    uint32_t TRP25:1;          /*!< bit:     25  Transmission Request Pending 25    */
    uint32_t TRP26:1;          /*!< bit:     26  Transmission Request Pending 26    */
    uint32_t TRP27:1;          /*!< bit:     27  Transmission Request Pending 27    */
    uint32_t TRP28:1;          /*!< bit:     28  Transmission Request Pending 28    */
    uint32_t TRP29:1;          /*!< bit:     29  Transmission Request Pending 29    */
    uint32_t TRP30:1;          /*!< bit:     30  Transmission Request Pending 30    */
    uint32_t TRP31:1;          /*!< bit:     31  Transmission Request Pending 31    */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBRP_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXBAR : (CAN Offset: 0xD0) (R/W 32) Tx Buffer Add Request -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t AR0:1;            /*!< bit:      0  Add Request 0                      */
    uint32_t AR1:1;            /*!< bit:      1  Add Request 1                      */
    uint32_t AR2:1;            /*!< bit:      2  Add Request 2                      */
    uint32_t AR3:1;            /*!< bit:      3  Add Request 3                      */
    uint32_t AR4:1;            /*!< bit:      4  Add Request 4                      */
    uint32_t AR5:1;            /*!< bit:      5  Add Request 5                      */
    uint32_t AR6:1;            /*!< bit:      6  Add Request 6                      */
    uint32_t AR7:1;            /*!< bit:      7  Add Request 7                      */
    uint32_t AR8:1;            /*!< bit:      8  Add Request 8                      */
    uint32_t AR9:1;            /*!< bit:      9  Add Request 9                      */
    uint32_t AR10:1;           /*!< bit:     10  Add Request 10                     */
    uint32_t AR11:1;           /*!< bit:     11  Add Request 11                     */
    uint32_t AR12:1;           /*!< bit:     12  Add Request 12                     */
    uint32_t AR13:1;           /*!< bit:     13  Add Request 13                     */
    uint32_t AR14:1;           /*!< bit:     14  Add Request 14                     */
    uint32_t AR15:1;           /*!< bit:     15  Add Request 15                     */
    uint32_t AR16:1;           /*!< bit:     16  Add Request 16                     */
    uint32_t AR17:1;           /*!< bit:     17  Add Request 17                     */
    uint32_t AR18:1;           /*!< bit:     18  Add Request 18                     */
    uint32_t AR19:1;           /*!< bit:     19  Add Request 19                     */
    uint32_t AR20:1;           /*!< bit:     20  Add Request 20                     */
    uint32_t AR21:1;           /*!< bit:     21  Add Request 21                     */
    uint32_t AR22:1;           /*!< bit:     22  Add Request 22                     */
    uint32_t AR23:1;           /*!< bit:     23  Add Request 23                     */
    uint32_t AR24:1;           /*!< bit:     24  Add Request 24                     */
    uint32_t AR25:1;           /*!< bit:     25  Add Request 25                     */
    uint32_t AR26:1;           /*!< bit:     26  Add Request 26                     */
    uint32_t AR27:1;           /*!< bit:     27  Add Request 27                     */
    uint32_t AR28:1;           /*!< bit:     28  Add Request 28                     */
    uint32_t AR29:1;           /*!< bit:     29  Add Request 29                     */
    uint32_t AR30:1;           /*!< bit:     30  Add Request 30                     */
    uint32_t AR31:1;           /*!< bit:     31  Add Request 31                     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBAR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXBCR : (CAN Offset: 0xD4) (R/W 32) Tx Buffer Cancellation Request -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CR0:1;            /*!< bit:      0  Cancellation Request 0             */
    uint32_t CR1:1;            /*!< bit:      1  Cancellation Request 1             */
    uint32_t CR2:1;            /*!< bit:      2  Cancellation Request 2             */
    uint32_t CR3:1;            /*!< bit:      3  Cancellation Request 3             */
    uint32_t CR4:1;            /*!< bit:      4  Cancellation Request 4             */
    uint32_t CR5:1;            /*!< bit:      5  Cancellation Request 5             */
    uint32_t CR6:1;            /*!< bit:      6  Cancellation Request 6             */
    uint32_t CR7:1;            /*!< bit:      7  Cancellation Request 7             */
    uint32_t CR8:1;            /*!< bit:      8  Cancellation Request 8             */
    uint32_t CR9:1;            /*!< bit:      9  Cancellation Request 9             */
    uint32_t CR10:1;           /*!< bit:     10  Cancellation Request 10            */
    uint32_t CR11:1;           /*!< bit:     11  Cancellation Request 11            */
    uint32_t CR12:1;           /*!< bit:     12  Cancellation Request 12            */
    uint32_t CR13:1;           /*!< bit:     13  Cancellation Request 13            */
    uint32_t CR14:1;           /*!< bit:     14  Cancellation Request 14            */
    uint32_t CR15:1;           /*!< bit:     15  Cancellation Request 15            */
    uint32_t CR16:1;           /*!< bit:     16  Cancellation Request 16            */
    uint32_t CR17:1;           /*!< bit:     17  Cancellation Request 17            */
    uint32_t CR18:1;           /*!< bit:     18  Cancellation Request 18            */
    uint32_t CR19:1;           /*!< bit:     19  Cancellation Request 19            */
    uint32_t CR20:1;           /*!< bit:     20  Cancellation Request 20            */
    uint32_t CR21:1;           /*!< bit:     21  Cancellation Request 21            */
    uint32_t CR22:1;           /*!< bit:     22  Cancellation Request 22            */
    uint32_t CR23:1;           /*!< bit:     23  Cancellation Request 23            */
    uint32_t CR24:1;           /*!< bit:     24  Cancellation Request 24            */
    uint32_t CR25:1;           /*!< bit:     25  Cancellation Request 25            */
    uint32_t CR26:1;           /*!< bit:     26  Cancellation Request 26            */
    uint32_t CR27:1;           /*!< bit:     27  Cancellation Request 27            */
    uint32_t CR28:1;           /*!< bit:     28  Cancellation Request 28            */
    uint32_t CR29:1;           /*!< bit:     29  Cancellation Request 29            */
    uint32_t CR30:1;           /*!< bit:     30  Cancellation Request 30            */
    uint32_t CR31:1;           /*!< bit:     31  Cancellation Request 31            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBCR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXBTO : (CAN Offset: 0xD8) ( R/ 32) Tx Buffer Transmission Occurred -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TO0:1;            /*!< bit:      0  Transmission Occurred 0            */
    uint32_t TO1:1;            /*!< bit:      1  Transmission Occurred 1            */
    uint32_t TO2:1;            /*!< bit:      2  Transmission Occurred 2            */
    uint32_t TO3:1;            /*!< bit:      3  Transmission Occurred 3            */
    uint32_t TO4:1;            /*!< bit:      4  Transmission Occurred 4            */
    uint32_t TO5:1;            /*!< bit:      5  Transmission Occurred 5            */
    uint32_t TO6:1;            /*!< bit:      6  Transmission Occurred 6            */
    uint32_t TO7:1;            /*!< bit:      7  Transmission Occurred 7            */
    uint32_t TO8:1;            /*!< bit:      8  Transmission Occurred 8            */
    uint32_t TO9:1;            /*!< bit:      9  Transmission Occurred 9            */
    uint32_t TO10:1;           /*!< bit:     10  Transmission Occurred 10           */
    uint32_t TO11:1;           /*!< bit:     11  Transmission Occurred 11           */
    uint32_t TO12:1;           /*!< bit:     12  Transmission Occurred 12           */
    uint32_t TO13:1;           /*!< bit:     13  Transmission Occurred 13           */
    uint32_t TO14:1;           /*!< bit:     14  Transmission Occurred 14           */
    uint32_t TO15:1;           /*!< bit:     15  Transmission Occurred 15           */
    uint32_t TO16:1;           /*!< bit:     16  Transmission Occurred 16           */
    uint32_t TO17:1;           /*!< bit:     17  Transmission Occurred 17           */
    uint32_t TO18:1;           /*!< bit:     18  Transmission Occurred 18           */
    uint32_t TO19:1;           /*!< bit:     19  Transmission Occurred 19           */
    uint32_t TO20:1;           /*!< bit:     20  Transmission Occurred 20           */
    uint32_t TO21:1;           /*!< bit:     21  Transmission Occurred 21           */
    uint32_t TO22:1;           /*!< bit:     22  Transmission Occurred 22           */
    uint32_t TO23:1;           /*!< bit:     23  Transmission Occurred 23           */
    uint32_t TO24:1;           /*!< bit:     24  Transmission Occurred 24           */
    uint32_t TO25:1;           /*!< bit:     25  Transmission Occurred 25           */
    uint32_t TO26:1;           /*!< bit:     26  Transmission Occurred 26           */
    uint32_t TO27:1;           /*!< bit:     27  Transmission Occurred 27           */
    uint32_t TO28:1;           /*!< bit:     28  Transmission Occurred 28           */
    uint32_t TO29:1;           /*!< bit:     29  Transmission Occurred 29           */
    uint32_t TO30:1;           /*!< bit:     30  Transmission Occurred 30           */
    uint32_t TO31:1;           /*!< bit:     31  Transmission Occurred 31           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBTO_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXBCF : (CAN Offset: 0xDC) ( R/ 32) Tx Buffer Cancellation Finished -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CF0:1;            /*!< bit:      0  Tx Buffer Cancellation Finished 0  */
    uint32_t CF1:1;            /*!< bit:      1  Tx Buffer Cancellation Finished 1  */
    uint32_t CF2:1;            /*!< bit:      2  Tx Buffer Cancellation Finished 2  */
    uint32_t CF3:1;            /*!< bit:      3  Tx Buffer Cancellation Finished 3  */
    uint32_t CF4:1;            /*!< bit:      4  Tx Buffer Cancellation Finished 4  */
    uint32_t CF5:1;            /*!< bit:      5  Tx Buffer Cancellation Finished 5  */
    uint32_t CF6:1;            /*!< bit:      6  Tx Buffer Cancellation Finished 6  */
    uint32_t CF7:1;            /*!< bit:      7  Tx Buffer Cancellation Finished 7  */
    uint32_t CF8:1;            /*!< bit:      8  Tx Buffer Cancellation Finished 8  */
    uint32_t CF9:1;            /*!< bit:      9  Tx Buffer Cancellation Finished 9  */
    uint32_t CF10:1;           /*!< bit:     10  Tx Buffer Cancellation Finished 10 */
    uint32_t CF11:1;           /*!< bit:     11  Tx Buffer Cancellation Finished 11 */
    uint32_t CF12:1;           /*!< bit:     12  Tx Buffer Cancellation Finished 12 */
    uint32_t CF13:1;           /*!< bit:     13  Tx Buffer Cancellation Finished 13 */
    uint32_t CF14:1;           /*!< bit:     14  Tx Buffer Cancellation Finished 14 */
    uint32_t CF15:1;           /*!< bit:     15  Tx Buffer Cancellation Finished 15 */
    uint32_t CF16:1;           /*!< bit:     16  Tx Buffer Cancellation Finished 16 */
    uint32_t CF17:1;           /*!< bit:     17  Tx Buffer Cancellation Finished 17 */
    uint32_t CF18:1;           /*!< bit:     18  Tx Buffer Cancellation Finished 18 */
    uint32_t CF19:1;           /*!< bit:     19  Tx Buffer Cancellation Finished 19 */
    uint32_t CF20:1;           /*!< bit:     20  Tx Buffer Cancellation Finished 20 */
    uint32_t CF21:1;           /*!< bit:     21  Tx Buffer Cancellation Finished 21 */
    uint32_t CF22:1;           /*!< bit:     22  Tx Buffer Cancellation Finished 22 */
    uint32_t CF23:1;           /*!< bit:     23  Tx Buffer Cancellation Finished 23 */
    uint32_t CF24:1;           /*!< bit:     24  Tx Buffer Cancellation Finished 24 */
    uint32_t CF25:1;           /*!< bit:     25  Tx Buffer Cancellation Finished 25 */
    uint32_t CF26:1;           /*!< bit:     26  Tx Buffer Cancellation Finished 26 */
    uint32_t CF27:1;           /*!< bit:     27  Tx Buffer Cancellation Finished 27 */
    uint32_t CF28:1;           /*!< bit:     28  Tx Buffer Cancellation Finished 28 */
    uint32_t CF29:1;           /*!< bit:     29  Tx Buffer Cancellation Finished 29 */
    uint32_t CF30:1;           /*!< bit:     30  Tx Buffer Cancellation Finished 30 */
    uint32_t CF31:1;           /*!< bit:     31  Tx Buffer Cancellation Finished 31 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBCF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXBTIE : (CAN Offset: 0xE0) (R/W 32) Tx Buffer Transmission Interrupt Enable -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t TIE0:1;           /*!< bit:      0  Transmission Interrupt Enable 0    */
    uint32_t TIE1:1;           /*!< bit:      1  Transmission Interrupt Enable 1    */
    uint32_t TIE2:1;           /*!< bit:      2  Transmission Interrupt Enable 2    */
    uint32_t TIE3:1;           /*!< bit:      3  Transmission Interrupt Enable 3    */
    uint32_t TIE4:1;           /*!< bit:      4  Transmission Interrupt Enable 4    */
    uint32_t TIE5:1;           /*!< bit:      5  Transmission Interrupt Enable 5    */
    uint32_t TIE6:1;           /*!< bit:      6  Transmission Interrupt Enable 6    */
    uint32_t TIE7:1;           /*!< bit:      7  Transmission Interrupt Enable 7    */
    uint32_t TIE8:1;           /*!< bit:      8  Transmission Interrupt Enable 8    */
    uint32_t TIE9:1;           /*!< bit:      9  Transmission Interrupt Enable 9    */
    uint32_t TIE10:1;          /*!< bit:     10  Transmission Interrupt Enable 10   */
    uint32_t TIE11:1;          /*!< bit:     11  Transmission Interrupt Enable 11   */
    uint32_t TIE12:1;          /*!< bit:     12  Transmission Interrupt Enable 12   */
    uint32_t TIE13:1;          /*!< bit:     13  Transmission Interrupt Enable 13   */
    uint32_t TIE14:1;          /*!< bit:     14  Transmission Interrupt Enable 14   */
    uint32_t TIE15:1;          /*!< bit:     15  Transmission Interrupt Enable 15   */
    uint32_t TIE16:1;          /*!< bit:     16  Transmission Interrupt Enable 16   */
    uint32_t TIE17:1;          /*!< bit:     17  Transmission Interrupt Enable 17   */
    uint32_t TIE18:1;          /*!< bit:     18  Transmission Interrupt Enable 18   */
    uint32_t TIE19:1;          /*!< bit:     19  Transmission Interrupt Enable 19   */
    uint32_t TIE20:1;          /*!< bit:     20  Transmission Interrupt Enable 20   */
    uint32_t TIE21:1;          /*!< bit:     21  Transmission Interrupt Enable 21   */
    uint32_t TIE22:1;          /*!< bit:     22  Transmission Interrupt Enable 22   */
    uint32_t TIE23:1;          /*!< bit:     23  Transmission Interrupt Enable 23   */
    uint32_t TIE24:1;          /*!< bit:     24  Transmission Interrupt Enable 24   */
    uint32_t TIE25:1;          /*!< bit:     25  Transmission Interrupt Enable 25   */
    uint32_t TIE26:1;          /*!< bit:     26  Transmission Interrupt Enable 26   */
    uint32_t TIE27:1;          /*!< bit:     27  Transmission Interrupt Enable 27   */
    uint32_t TIE28:1;          /*!< bit:     28  Transmission Interrupt Enable 28   */
    uint32_t TIE29:1;          /*!< bit:     29  Transmission Interrupt Enable 29   */
    uint32_t TIE30:1;          /*!< bit:     30  Transmission Interrupt Enable 30   */
    uint32_t TIE31:1;          /*!< bit:     31  Transmission Interrupt Enable 31   */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBTIE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXBCIE : (CAN Offset: 0xE4) (R/W 32) Tx Buffer Cancellation Finished Interrupt Enable -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CFIE0:1;          /*!< bit:      0  Cancellation Finished Interrupt Enable 0 */
    uint32_t CFIE1:1;          /*!< bit:      1  Cancellation Finished Interrupt Enable 1 */
    uint32_t CFIE2:1;          /*!< bit:      2  Cancellation Finished Interrupt Enable 2 */
    uint32_t CFIE3:1;          /*!< bit:      3  Cancellation Finished Interrupt Enable 3 */
    uint32_t CFIE4:1;          /*!< bit:      4  Cancellation Finished Interrupt Enable 4 */
    uint32_t CFIE5:1;          /*!< bit:      5  Cancellation Finished Interrupt Enable 5 */
    uint32_t CFIE6:1;          /*!< bit:      6  Cancellation Finished Interrupt Enable 6 */
    uint32_t CFIE7:1;          /*!< bit:      7  Cancellation Finished Interrupt Enable 7 */
    uint32_t CFIE8:1;          /*!< bit:      8  Cancellation Finished Interrupt Enable 8 */
    uint32_t CFIE9:1;          /*!< bit:      9  Cancellation Finished Interrupt Enable 9 */
    uint32_t CFIE10:1;         /*!< bit:     10  Cancellation Finished Interrupt Enable 10 */
    uint32_t CFIE11:1;         /*!< bit:     11  Cancellation Finished Interrupt Enable 11 */
    uint32_t CFIE12:1;         /*!< bit:     12  Cancellation Finished Interrupt Enable 12 */
    uint32_t CFIE13:1;         /*!< bit:     13  Cancellation Finished Interrupt Enable 13 */
    uint32_t CFIE14:1;         /*!< bit:     14  Cancellation Finished Interrupt Enable 14 */
    uint32_t CFIE15:1;         /*!< bit:     15  Cancellation Finished Interrupt Enable 15 */
    uint32_t CFIE16:1;         /*!< bit:     16  Cancellation Finished Interrupt Enable 16 */
    uint32_t CFIE17:1;         /*!< bit:     17  Cancellation Finished Interrupt Enable 17 */
    uint32_t CFIE18:1;         /*!< bit:     18  Cancellation Finished Interrupt Enable 18 */
    uint32_t CFIE19:1;         /*!< bit:     19  Cancellation Finished Interrupt Enable 19 */
    uint32_t CFIE20:1;         /*!< bit:     20  Cancellation Finished Interrupt Enable 20 */
    uint32_t CFIE21:1;         /*!< bit:     21  Cancellation Finished Interrupt Enable 21 */
    uint32_t CFIE22:1;         /*!< bit:     22  Cancellation Finished Interrupt Enable 22 */
    uint32_t CFIE23:1;         /*!< bit:     23  Cancellation Finished Interrupt Enable 23 */
    uint32_t CFIE24:1;         /*!< bit:     24  Cancellation Finished Interrupt Enable 24 */
    uint32_t CFIE25:1;         /*!< bit:     25  Cancellation Finished Interrupt Enable 25 */
    uint32_t CFIE26:1;         /*!< bit:     26  Cancellation Finished Interrupt Enable 26 */
    uint32_t CFIE27:1;         /*!< bit:     27  Cancellation Finished Interrupt Enable 27 */
    uint32_t CFIE28:1;         /*!< bit:     28  Cancellation Finished Interrupt Enable 28 */
    uint32_t CFIE29:1;         /*!< bit:     29  Cancellation Finished Interrupt Enable 29 */
    uint32_t CFIE30:1;         /*!< bit:     30  Cancellation Finished Interrupt Enable 30 */
    uint32_t CFIE31:1;         /*!< bit:     31  Cancellation Finished Interrupt Enable 31 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXBCIE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXEFC : (CAN Offset: 0xF0) (R/W 32) Tx Event FIFO Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EFSA:16;          /*!< bit:  0..15  Event FIFO Start Address           */
    uint32_t EFS:6;            /*!< bit: 16..21  Event FIFO Size                    */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t EFWM:6;           /*!< bit: 24..29  Event FIFO Watermark               */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXEFC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXEFS : (CAN Offset: 0xF4) ( R/ 32) Tx Event FIFO Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EFFL:6;           /*!< bit:  0.. 5  Event FIFO Fill Level              */
    uint32_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint32_t EFGI:5;           /*!< bit:  8..12  Event FIFO Get Index               */
    uint32_t :3;               /*!< bit: 13..15  Reserved                           */
    uint32_t EFPI:5;           /*!< bit: 16..20  Event FIFO Put Index               */
    uint32_t :3;               /*!< bit: 21..23  Reserved                           */
    uint32_t EFF:1;            /*!< bit:     24  Event FIFO Full                    */
    uint32_t TEFL:1;           /*!< bit:     25  Tx Event FIFO Element Lost         */
    uint32_t :6;               /*!< bit: 26..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXEFS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- CAN_TXEFA : (CAN Offset: 0xF8) (R/W 32) Tx Event FIFO Acknowledge -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EFAI:5;           /*!< bit:  0.. 4  Event FIFO Acknowledge Index       */
    uint32_t :27;              /*!< bit:  5..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} CAN_TXEFA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief CAN Mram_rxbe hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO CAN_RXBE_0_Type           RXBE_0;      /**< \brief Offset: 0x00 (R/W 32) Rx Buffer Element 0 */
  __IO CAN_RXBE_1_Type           RXBE_1;      /**< \brief Offset: 0x04 (R/W 32) Rx Buffer Element 1 */
  __IO CAN_RXBE_DATA_Type        RXBE_DATA[16]; /**< \brief Offset: 0x08 (R/W 32) Rx Buffer Element Data */
} CanMramRxbe
#ifdef __GNUC__
  __attribute__ ((aligned (4)))
#endif
;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief CAN Mram_rxf0e hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO CAN_RXF0E_0_Type          RXF0E_0;     /**< \brief Offset: 0x00 (R/W 32) Rx FIFO 0 Element 0 */
  __IO CAN_RXF0E_1_Type          RXF0E_1;     /**< \brief Offset: 0x04 (R/W 32) Rx FIFO 0 Element 1 */
  __IO CAN_RXF0E_DATA_Type       RXF0E_DATA[16]; /**< \brief Offset: 0x08 (R/W 32) Rx FIFO 0 Element Data */
} CanMramRxf0e
#ifdef __GNUC__
  __attribute__ ((aligned (4)))
#endif
;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief CAN Mram_rxf1e hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO CAN_RXF1E_0_Type          RXF1E_0;     /**< \brief Offset: 0x00 (R/W 32) Rx FIFO 1 Element 0 */
  __IO CAN_RXF1E_1_Type          RXF1E_1;     /**< \brief Offset: 0x04 (R/W 32) Rx FIFO 1 Element 1 */
  __IO CAN_RXF1E_DATA_Type       RXF1E_DATA[16]; /**< \brief Offset: 0x08 (R/W 32) Rx FIFO 1 Element Data */
} CanMramRxf1e
#ifdef __GNUC__
  __attribute__ ((aligned (4)))
#endif
;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief CAN Mram_sidfe hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO CAN_SIDFE_0_Type          SIDFE_0;     /**< \brief Offset: 0x00 (R/W 32) Standard Message ID Filter Element */
} CanMramSidfe
#ifdef __GNUC__
  __attribute__ ((aligned (4)))
#endif
;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief CAN Mram_txbe hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO CAN_TXBE_0_Type           TXBE_0;      /**< \brief Offset: 0x00 (R/W 32) Tx Buffer Element 0 */
  __IO CAN_TXBE_1_Type           TXBE_1;      /**< \brief Offset: 0x04 (R/W 32) Tx Buffer Element 1 */
  __IO CAN_TXBE_DATA_Type        TXBE_DATA[16]; /**< \brief Offset: 0x08 (R/W 32) Tx Buffer Element Data */
} CanMramTxbe
#ifdef __GNUC__
  __attribute__ ((aligned (4)))
#endif
;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief CAN Mram_txefe hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO CAN_TXEFE_0_Type          TXEFE_0;     /**< \brief Offset: 0x00 (R/W 32) Tx Event FIFO Element 0 */
  __IO CAN_TXEFE_1_Type          TXEFE_1;     /**< \brief Offset: 0x04 (R/W 32) Tx Event FIFO Element 1 */
} CanMramTxefe
#ifdef __GNUC__
  __attribute__ ((aligned (4)))
#endif
;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief CAN Mram_xifde hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO CAN_XIDFE_0_Type          XIDFE_0;     /**< \brief Offset: 0x00 (R/W 32) Extended Message ID Filter Element 0 */
  __IO CAN_XIDFE_1_Type          XIDFE_1;     /**< \brief Offset: 0x04 (R/W 32) Extended Message ID Filter Element 1 */
} CanMramXifde
#ifdef __GNUC__
  __attribute__ ((aligned (4)))
#endif
;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief CAN APB hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __I  CAN_CREL_Type             CREL;        /**< \brief Offset: 0x00 (R/  32) Core Release */
  __I  CAN_ENDN_Type             ENDN;        /**< \brief Offset: 0x04 (R/  32) Endian */
  __IO CAN_MRCFG_Type            MRCFG;       /**< \brief Offset: 0x08 (R/W 32) Message RAM Configuration */
  __IO CAN_DBTP_Type             DBTP;        /**< \brief Offset: 0x0C (R/W 32) Fast Bit Timing and Prescaler */
  __IO CAN_TEST_Type             TEST;        /**< \brief Offset: 0x10 (R/W 32) Test */
  __IO CAN_RWD_Type              RWD;         /**< \brief Offset: 0x14 (R/W 32) RAM Watchdog */
  __IO CAN_CCCR_Type             CCCR;        /**< \brief Offset: 0x18 (R/W 32) CC Control */
  __IO CAN_NBTP_Type             NBTP;        /**< \brief Offset: 0x1C (R/W 32) Nominal Bit Timing and Prescaler */
  __IO CAN_TSCC_Type             TSCC;        /**< \brief Offset: 0x20 (R/W 32) Timestamp Counter Configuration */
  __I  CAN_TSCV_Type             TSCV;        /**< \brief Offset: 0x24 (R/  32) Timestamp Counter Value */
  __IO CAN_TOCC_Type             TOCC;        /**< \brief Offset: 0x28 (R/W 32) Timeout Counter Configuration */
  __IO CAN_TOCV_Type             TOCV;        /**< \brief Offset: 0x2C (R/W 32) Timeout Counter Value */
       RoReg8                    Reserved1[0x10];
  __I  CAN_ECR_Type              ECR;         /**< \brief Offset: 0x40 (R/  32) Error Counter */
  __I  CAN_PSR_Type              PSR;         /**< \brief Offset: 0x44 (R/  32) Protocol Status */
  __IO CAN_TDCR_Type             TDCR;        /**< \brief Offset: 0x48 (R/W 32) Extended ID Filter Configuration */
       RoReg8                    Reserved2[0x4];
  __IO CAN_IR_Type               IR;          /**< \brief Offset: 0x50 (R/W 32) Interrupt */
  __IO CAN_IE_Type               IE;          /**< \brief Offset: 0x54 (R/W 32) Interrupt Enable */
  __IO CAN_ILS_Type              ILS;         /**< \brief Offset: 0x58 (R/W 32) Interrupt Line Select */
  __IO CAN_ILE_Type              ILE;         /**< \brief Offset: 0x5C (R/W 32) Interrupt Line Enable */
       RoReg8                    Reserved3[0x20];
  __IO CAN_GFC_Type              GFC;         /**< \brief Offset: 0x80 (R/W 32) Global Filter Configuration */
  __IO CAN_SIDFC_Type            SIDFC;       /**< \brief Offset: 0x84 (R/W 32) Standard ID Filter Configuration */
  __IO CAN_XIDFC_Type            XIDFC;       /**< \brief Offset: 0x88 (R/W 32) Extended ID Filter Configuration */
       RoReg8                    Reserved4[0x4];
  __IO CAN_XIDAM_Type            XIDAM;       /**< \brief Offset: 0x90 (R/W 32) Extended ID AND Mask */
  __I  CAN_HPMS_Type             HPMS;        /**< \brief Offset: 0x94 (R/  32) High Priority Message Status */
  __IO CAN_NDAT1_Type            NDAT1;       /**< \brief Offset: 0x98 (R/W 32) New Data 1 */
  __IO CAN_NDAT2_Type            NDAT2;       /**< \brief Offset: 0x9C (R/W 32) New Data 2 */
  __IO CAN_RXF0C_Type            RXF0C;       /**< \brief Offset: 0xA0 (R/W 32) Rx FIFO 0 Configuration */
  __I  CAN_RXF0S_Type            RXF0S;       /**< \brief Offset: 0xA4 (R/  32) Rx FIFO 0 Status */
  __IO CAN_RXF0A_Type            RXF0A;       /**< \brief Offset: 0xA8 (R/W 32) Rx FIFO 0 Acknowledge */
  __IO CAN_RXBC_Type             RXBC;        /**< \brief Offset: 0xAC (R/W 32) Rx Buffer Configuration */
  __IO CAN_RXF1C_Type            RXF1C;       /**< \brief Offset: 0xB0 (R/W 32) Rx FIFO 1 Configuration */
  __I  CAN_RXF1S_Type            RXF1S;       /**< \brief Offset: 0xB4 (R/  32) Rx FIFO 1 Status */
  __IO CAN_RXF1A_Type            RXF1A;       /**< \brief Offset: 0xB8 (R/W 32) Rx FIFO 1 Acknowledge */
  __IO CAN_RXESC_Type            RXESC;       /**< \brief Offset: 0xBC (R/W 32) Rx Buffer / FIFO Element Size Configuration */
  __IO CAN_TXBC_Type             TXBC;        /**< \brief Offset: 0xC0 (R/W 32) Tx Buffer Configuration */
  __I  CAN_TXFQS_Type            TXFQS;       /**< \brief Offset: 0xC4 (R/  32) Tx FIFO / Queue Status */
  __IO CAN_TXESC_Type            TXESC;       /**< \brief Offset: 0xC8 (R/W 32) Tx Buffer Element Size Configuration */
  __I  CAN_TXBRP_Type            TXBRP;       /**< \brief Offset: 0xCC (R/  32) Tx Buffer Request Pending */
  __IO CAN_TXBAR_Type            TXBAR;       /**< \brief Offset: 0xD0 (R/W 32) Tx Buffer Add Request */
  __IO CAN_TXBCR_Type            TXBCR;       /**< \brief Offset: 0xD4 (R/W 32) Tx Buffer Cancellation Request */
  __I  CAN_TXBTO_Type            TXBTO;       /**< \brief Offset: 0xD8 (R/  32) Tx Buffer Transmission Occurred */
  __I  CAN_TXBCF_Type            TXBCF;       /**< \brief Offset: 0xDC (R/  32) Tx Buffer Cancellation Finished */
  __IO CAN_TXBTIE_Type           TXBTIE;      /**< \brief Offset: 0xE0 (R/W 32) Tx Buffer Transmission Interrupt Enable */
  __IO CAN_TXBCIE_Type           TXBCIE;      /**< \brief Offset: 0xE4 (R/W 32) Tx Buffer Cancellation Finished Interrupt Enable */
       RoReg8                    Reserved5[0x8];
  __IO CAN_TXEFC_Type            TXEFC;       /**< \brief Offset: 0xF0 (R/W 32) Tx Event FIFO Configuration */
  __I  CAN_TXEFS_Type            TXEFS;       /**< \brief Offset: 0xF4 (R/  32) Tx Event FIFO Status */
  __IO CAN_TXEFA_Type            TXEFA;       /**< \brief Offset: 0xF8 (R/W 32) Tx Event FIFO Acknowledge */
} Can;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SECTION_CAN_MRAM_RXBE
#define SECTION_CAN_MRAM_RXF0E
#define SECTION_CAN_MRAM_RXF1E
#define SECTION_CAN_MRAM_SIDFE
#define SECTION_CAN_MRAM_TXBE
#define SECTION_CAN_MRAM_TXEFE
#define SECTION_CAN_MRAM_XIFDE

#endif /* _MICROCHIP_PIC32CXSG_CAN_COMPONENT_FIXUP_H_ */
