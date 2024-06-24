/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_USB_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_USB_COMPONENT_FIXUP_H_

/* -------- USB_DEVICE_ADDR : (USB Offset: 0x00) (R/W 32) DEVICE_DESC_BANK Endpoint Bank, Adress of Data Buffer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  Adress of data buffer              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} USB_DEVICE_ADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_PCKSIZE : (USB Offset: 0x04) (R/W 32) DEVICE_DESC_BANK Endpoint Bank, Packet Size -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BYTE_COUNT:14;    /*!< bit:  0..13  Byte Count                         */
    uint32_t MULTI_PACKET_SIZE:14; /*!< bit: 14..27  Multi Packet In or Out size        */
    uint32_t SIZE:3;           /*!< bit: 28..30  Enpoint size                       */
    uint32_t AUTO_ZLP:1;       /*!< bit:     31  Automatic Zero Length Packet       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} USB_DEVICE_PCKSIZE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_EXTREG : (USB Offset: 0x08) (R/W 16) DEVICE_DESC_BANK Endpoint Bank, Extended -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SUBPID:4;         /*!< bit:  0.. 3  SUBPID field send with extended token */
    uint16_t VARIABLE:11;      /*!< bit:  4..14  Variable field send with extended token */
    uint16_t :1;               /*!< bit:     15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_DEVICE_EXTREG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_STATUS_BK : (USB Offset: 0x0A) (R/W 8) DEVICE_DESC_BANK Enpoint Bank, Status of Bank -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CRCERR:1;         /*!< bit:      0  CRC Error Status                   */
    uint8_t  ERRORFLOW:1;      /*!< bit:      1  Error Flow Status                  */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_DEVICE_STATUS_BK_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_ADDR : (USB Offset: 0x00) (R/W 32) HOST_DESC_BANK Host Bank, Adress of Data Buffer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  Adress of data buffer              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} USB_HOST_ADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_PCKSIZE : (USB Offset: 0x04) (R/W 32) HOST_DESC_BANK Host Bank, Packet Size -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t BYTE_COUNT:14;    /*!< bit:  0..13  Byte Count                         */
    uint32_t MULTI_PACKET_SIZE:14; /*!< bit: 14..27  Multi Packet In or Out size        */
    uint32_t SIZE:3;           /*!< bit: 28..30  Pipe size                          */
    uint32_t AUTO_ZLP:1;       /*!< bit:     31  Automatic Zero Length Packet       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} USB_HOST_PCKSIZE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_EXTREG : (USB Offset: 0x08) (R/W 16) HOST_DESC_BANK Host Bank, Extended -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SUBPID:4;         /*!< bit:  0.. 3  SUBPID field send with extended token */
    uint16_t VARIABLE:11;      /*!< bit:  4..14  Variable field send with extended token */
    uint16_t :1;               /*!< bit:     15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_HOST_EXTREG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_STATUS_BK : (USB Offset: 0x0A) (R/W 8) HOST_DESC_BANK Host Bank, Status of Bank -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CRCERR:1;         /*!< bit:      0  CRC Error Status                   */
    uint8_t  ERRORFLOW:1;      /*!< bit:      1  Error Flow Status                  */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_STATUS_BK_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_CTRL_PIPE : (USB Offset: 0x0C) (R/W 16) HOST_DESC_BANK Host Bank, Host Control Pipe -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t PDADDR:7;         /*!< bit:  0.. 6  Pipe Device Adress                 */
    uint16_t :1;               /*!< bit:      7  Reserved                           */
    uint16_t PEPNUM:4;         /*!< bit:  8..11  Pipe Endpoint Number               */
    uint16_t PERMAX:4;         /*!< bit: 12..15  Pipe Error Max Number              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_HOST_CTRL_PIPE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_STATUS_PIPE : (USB Offset: 0x0E) (R/W 16) HOST_DESC_BANK Host Bank, Host Status Pipe -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t DTGLER:1;         /*!< bit:      0  Data Toggle Error                  */
    uint16_t DAPIDER:1;        /*!< bit:      1  Data PID Error                     */
    uint16_t PIDER:1;          /*!< bit:      2  PID Error                          */
    uint16_t TOUTER:1;         /*!< bit:      3  Time Out Error                     */
    uint16_t CRC16ER:1;        /*!< bit:      4  CRC16 Error                        */
    uint16_t ERCNT:3;          /*!< bit:  5.. 7  Pipe Error Count                   */
    uint16_t :8;               /*!< bit:  8..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_HOST_STATUS_PIPE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_EPCFG : (USB Offset: 0x00) (R/W 8) DEVICE_ENDPOINT End Point Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  EPTYPE0:3;        /*!< bit:  0.. 2  End Point Type0                    */
    uint8_t  :1;               /*!< bit:      3  Reserved                           */
    uint8_t  EPTYPE1:3;        /*!< bit:  4.. 6  End Point Type1                    */
    uint8_t  NYETDIS:1;        /*!< bit:      7  NYET Token Disable                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_DEVICE_EPCFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_EPSTATUSCLR : (USB Offset: 0x04) ( /W 8) DEVICE_ENDPOINT End Point Pipe Status Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DTGLOUT:1;        /*!< bit:      0  Data Toggle OUT Clear              */
    uint8_t  DTGLIN:1;         /*!< bit:      1  Data Toggle IN Clear               */
    uint8_t  CURBK:1;          /*!< bit:      2  Current Bank Clear                 */
    uint8_t  :1;               /*!< bit:      3  Reserved                           */
    uint8_t  STALLRQ0:1;       /*!< bit:      4  Stall 0 Request Clear              */
    uint8_t  STALLRQ1:1;       /*!< bit:      5  Stall 1 Request Clear              */
    uint8_t  BK0RDY:1;         /*!< bit:      6  Bank 0 Ready Clear                 */
    uint8_t  BK1RDY:1;         /*!< bit:      7  Bank 1 Ready Clear                 */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  :4;               /*!< bit:  0.. 3  Reserved                           */
    uint8_t  STALLRQ:2;        /*!< bit:  4.. 5  Stall x Request Clear              */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_DEVICE_EPSTATUSCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_EPSTATUSSET : (USB Offset: 0x05) ( /W 8) DEVICE_ENDPOINT End Point Pipe Status Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DTGLOUT:1;        /*!< bit:      0  Data Toggle OUT Set                */
    uint8_t  DTGLIN:1;         /*!< bit:      1  Data Toggle IN Set                 */
    uint8_t  CURBK:1;          /*!< bit:      2  Current Bank Set                   */
    uint8_t  :1;               /*!< bit:      3  Reserved                           */
    uint8_t  STALLRQ0:1;       /*!< bit:      4  Stall 0 Request Set                */
    uint8_t  STALLRQ1:1;       /*!< bit:      5  Stall 1 Request Set                */
    uint8_t  BK0RDY:1;         /*!< bit:      6  Bank 0 Ready Set                   */
    uint8_t  BK1RDY:1;         /*!< bit:      7  Bank 1 Ready Set                   */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  :4;               /*!< bit:  0.. 3  Reserved                           */
    uint8_t  STALLRQ:2;        /*!< bit:  4.. 5  Stall x Request Set                */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_DEVICE_EPSTATUSSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_EPSTATUS : (USB Offset: 0x06) ( R/ 8) DEVICE_ENDPOINT End Point Pipe Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DTGLOUT:1;        /*!< bit:      0  Data Toggle Out                    */
    uint8_t  DTGLIN:1;         /*!< bit:      1  Data Toggle In                     */
    uint8_t  CURBK:1;          /*!< bit:      2  Current Bank                       */
    uint8_t  :1;               /*!< bit:      3  Reserved                           */
    uint8_t  STALLRQ0:1;       /*!< bit:      4  Stall 0 Request                    */
    uint8_t  STALLRQ1:1;       /*!< bit:      5  Stall 1 Request                    */
    uint8_t  BK0RDY:1;         /*!< bit:      6  Bank 0 ready                       */
    uint8_t  BK1RDY:1;         /*!< bit:      7  Bank 1 ready                       */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  :4;               /*!< bit:  0.. 3  Reserved                           */
    uint8_t  STALLRQ:2;        /*!< bit:  4.. 5  Stall x Request                    */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_DEVICE_EPSTATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_EPINTFLAG : (USB Offset: 0x07) (R/W 8) DEVICE_ENDPOINT End Point Interrupt Flag -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  TRCPT0:1;         /*!< bit:      0  Transfer Complete 0                */
    __I uint8_t  TRCPT1:1;         /*!< bit:      1  Transfer Complete 1                */
    __I uint8_t  TRFAIL0:1;        /*!< bit:      2  Error Flow 0                       */
    __I uint8_t  TRFAIL1:1;        /*!< bit:      3  Error Flow 1                       */
    __I uint8_t  RXSTP:1;          /*!< bit:      4  Received Setup                     */
    __I uint8_t  STALL0:1;         /*!< bit:      5  Stall 0 In/out                     */
    __I uint8_t  STALL1:1;         /*!< bit:      6  Stall 1 In/out                     */
    __I uint8_t  :1;               /*!< bit:      7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    __I uint8_t  TRCPT:2;          /*!< bit:  0.. 1  Transfer Complete x                */
    __I uint8_t  TRFAIL:2;         /*!< bit:  2.. 3  Error Flow x                       */
    __I uint8_t  :1;               /*!< bit:      4  Reserved                           */
    __I uint8_t  STALL:2;          /*!< bit:  5.. 6  Stall x In/out                     */
    __I uint8_t  :1;               /*!< bit:      7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_DEVICE_EPINTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_EPINTENCLR : (USB Offset: 0x08) (R/W 8) DEVICE_ENDPOINT End Point Interrupt Clear Flag -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  TRCPT0:1;         /*!< bit:      0  Transfer Complete 0 Interrupt Disable */
    uint8_t  TRCPT1:1;         /*!< bit:      1  Transfer Complete 1 Interrupt Disable */
    uint8_t  TRFAIL0:1;        /*!< bit:      2  Error Flow 0 Interrupt Disable     */
    uint8_t  TRFAIL1:1;        /*!< bit:      3  Error Flow 1 Interrupt Disable     */
    uint8_t  RXSTP:1;          /*!< bit:      4  Received Setup Interrupt Disable   */
    uint8_t  STALL0:1;         /*!< bit:      5  Stall 0 In/Out Interrupt Disable   */
    uint8_t  STALL1:1;         /*!< bit:      6  Stall 1 In/Out Interrupt Disable   */
    uint8_t  :1;               /*!< bit:      7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  TRCPT:2;          /*!< bit:  0.. 1  Transfer Complete x Interrupt Disable */
    uint8_t  TRFAIL:2;         /*!< bit:  2.. 3  Error Flow x Interrupt Disable     */
    uint8_t  :1;               /*!< bit:      4  Reserved                           */
    uint8_t  STALL:2;          /*!< bit:  5.. 6  Stall x In/Out Interrupt Disable   */
    uint8_t  :1;               /*!< bit:      7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_DEVICE_EPINTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_EPINTENSET : (USB Offset: 0x09) (R/W 8) DEVICE_ENDPOINT End Point Interrupt Set Flag -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  TRCPT0:1;         /*!< bit:      0  Transfer Complete 0 Interrupt Enable */
    uint8_t  TRCPT1:1;         /*!< bit:      1  Transfer Complete 1 Interrupt Enable */
    uint8_t  TRFAIL0:1;        /*!< bit:      2  Error Flow 0 Interrupt Enable      */
    uint8_t  TRFAIL1:1;        /*!< bit:      3  Error Flow 1 Interrupt Enable      */
    uint8_t  RXSTP:1;          /*!< bit:      4  Received Setup Interrupt Enable    */
    uint8_t  STALL0:1;         /*!< bit:      5  Stall 0 In/out Interrupt enable    */
    uint8_t  STALL1:1;         /*!< bit:      6  Stall 1 In/out Interrupt enable    */
    uint8_t  :1;               /*!< bit:      7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  TRCPT:2;          /*!< bit:  0.. 1  Transfer Complete x Interrupt Enable */
    uint8_t  TRFAIL:2;         /*!< bit:  2.. 3  Error Flow x Interrupt Enable      */
    uint8_t  :1;               /*!< bit:      4  Reserved                           */
    uint8_t  STALL:2;          /*!< bit:  5.. 6  Stall x In/out Interrupt enable    */
    uint8_t  :1;               /*!< bit:      7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_DEVICE_EPINTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_PCFG : (USB Offset: 0x00) (R/W 8) HOST_PIPE End Point Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  PTOKEN:2;         /*!< bit:  0.. 1  Pipe Token                         */
    uint8_t  BK:1;             /*!< bit:      2  Pipe Bank                          */
    uint8_t  PTYPE:3;          /*!< bit:  3.. 5  Pipe Type                          */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_PCFG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_BINTERVAL : (USB Offset: 0x03) (R/W 8) HOST_PIPE Bus Access Period of Pipe -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  BITINTERVAL:8;    /*!< bit:  0.. 7  Bit Interval                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_BINTERVAL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_PSTATUSCLR : (USB Offset: 0x04) ( /W 8) HOST_PIPE End Point Pipe Status Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DTGL:1;           /*!< bit:      0  Data Toggle clear                  */
    uint8_t  :1;               /*!< bit:      1  Reserved                           */
    uint8_t  CURBK:1;          /*!< bit:      2  Curren Bank clear                  */
    uint8_t  :1;               /*!< bit:      3  Reserved                           */
    uint8_t  PFREEZE:1;        /*!< bit:      4  Pipe Freeze Clear                  */
    uint8_t  :1;               /*!< bit:      5  Reserved                           */
    uint8_t  BK0RDY:1;         /*!< bit:      6  Bank 0 Ready Clear                 */
    uint8_t  BK1RDY:1;         /*!< bit:      7  Bank 1 Ready Clear                 */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_PSTATUSCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_PSTATUSSET : (USB Offset: 0x05) ( /W 8) HOST_PIPE End Point Pipe Status Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DTGL:1;           /*!< bit:      0  Data Toggle Set                    */
    uint8_t  :1;               /*!< bit:      1  Reserved                           */
    uint8_t  CURBK:1;          /*!< bit:      2  Current Bank Set                   */
    uint8_t  :1;               /*!< bit:      3  Reserved                           */
    uint8_t  PFREEZE:1;        /*!< bit:      4  Pipe Freeze Set                    */
    uint8_t  :1;               /*!< bit:      5  Reserved                           */
    uint8_t  BK0RDY:1;         /*!< bit:      6  Bank 0 Ready Set                   */
    uint8_t  BK1RDY:1;         /*!< bit:      7  Bank 1 Ready Set                   */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_PSTATUSSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- USB_HOST_PSTATUS : (USB Offset: 0x06) ( R/ 8) HOST_PIPE End Point Pipe Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DTGL:1;           /*!< bit:      0  Data Toggle                        */
    uint8_t  :1;               /*!< bit:      1  Reserved                           */
    uint8_t  CURBK:1;          /*!< bit:      2  Current Bank                       */
    uint8_t  :1;               /*!< bit:      3  Reserved                           */
    uint8_t  PFREEZE:1;        /*!< bit:      4  Pipe Freeze                        */
    uint8_t  :1;               /*!< bit:      5  Reserved                           */
    uint8_t  BK0RDY:1;         /*!< bit:      6  Bank 0 ready                       */
    uint8_t  BK1RDY:1;         /*!< bit:      7  Bank 1 ready                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_PSTATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_PINTFLAG : (USB Offset: 0x07) (R/W 8) HOST_PIPE Pipe Interrupt Flag -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  TRCPT0:1;         /*!< bit:      0  Transfer Complete 0 Interrupt Flag */
    __I uint8_t  TRCPT1:1;         /*!< bit:      1  Transfer Complete 1 Interrupt Flag */
    __I uint8_t  TRFAIL:1;         /*!< bit:      2  Error Flow Interrupt Flag          */
    __I uint8_t  PERR:1;           /*!< bit:      3  Pipe Error Interrupt Flag          */
    __I uint8_t  TXSTP:1;          /*!< bit:      4  Transmit  Setup Interrupt Flag     */
    __I uint8_t  STALL:1;          /*!< bit:      5  Stall Interrupt Flag               */
    __I uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    __I uint8_t  TRCPT:2;          /*!< bit:  0.. 1  Transfer Complete x Interrupt Flag */
    __I uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_PINTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_PINTENCLR : (USB Offset: 0x08) (R/W 8) HOST_PIPE Pipe Interrupt Flag Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  TRCPT0:1;         /*!< bit:      0  Transfer Complete 0 Disable        */
    uint8_t  TRCPT1:1;         /*!< bit:      1  Transfer Complete 1 Disable        */
    uint8_t  TRFAIL:1;         /*!< bit:      2  Error Flow Interrupt Disable       */
    uint8_t  PERR:1;           /*!< bit:      3  Pipe Error Interrupt Disable       */
    uint8_t  TXSTP:1;          /*!< bit:      4  Transmit Setup Interrupt Disable   */
    uint8_t  STALL:1;          /*!< bit:      5  Stall Inetrrupt Disable            */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  TRCPT:2;          /*!< bit:  0.. 1  Transfer Complete x Disable        */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_PINTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_PINTENSET : (USB Offset: 0x09) (R/W 8) HOST_PIPE Pipe Interrupt Flag Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  TRCPT0:1;         /*!< bit:      0  Transfer Complete 0 Interrupt Enable */
    uint8_t  TRCPT1:1;         /*!< bit:      1  Transfer Complete 1 Interrupt Enable */
    uint8_t  TRFAIL:1;         /*!< bit:      2  Error Flow Interrupt Enable        */
    uint8_t  PERR:1;           /*!< bit:      3  Pipe Error Interrupt Enable        */
    uint8_t  TXSTP:1;          /*!< bit:      4  Transmit  Setup Interrupt Enable   */
    uint8_t  STALL:1;          /*!< bit:      5  Stall Interrupt Enable             */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint8_t  TRCPT:2;          /*!< bit:  0.. 1  Transfer Complete x Interrupt Enable */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_PINTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_CTRLA : (USB Offset: 0x00) (R/W 8) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint8_t  ENABLE:1;         /*!< bit:      1  Enable                             */
    uint8_t  RUNSTDBY:1;       /*!< bit:      2  Run in Standby Mode                */
    uint8_t  :4;               /*!< bit:  3.. 6  Reserved                           */
    uint8_t  MODE:1;           /*!< bit:      7  Operating Mode                     */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_SYNCBUSY : (USB Offset: 0x02) ( R/ 8) Synchronization Busy -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SWRST:1;          /*!< bit:      0  Software Reset Synchronization Busy */
    uint8_t  ENABLE:1;         /*!< bit:      1  Enable Synchronization Busy        */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_QOSCTRL : (USB Offset: 0x03) (R/W 8) USB Quality Of Service -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  CQOS:2;           /*!< bit:  0.. 1  Configuration Quality of Service   */
    uint8_t  DQOS:2;           /*!< bit:  2.. 3  Data Quality of Service            */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_QOSCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_CTRLB : (USB Offset: 0x08) (R/W 16) DEVICE Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t DETACH:1;         /*!< bit:      0  Detach                             */
    uint16_t UPRSM:1;          /*!< bit:      1  Upstream Resume                    */
    uint16_t SPDCONF:2;        /*!< bit:  2.. 3  Speed Configuration                */
    uint16_t NREPLY:1;         /*!< bit:      4  No Reply                           */
    uint16_t TSTJ:1;           /*!< bit:      5  Test mode J                        */
    uint16_t TSTK:1;           /*!< bit:      6  Test mode K                        */
    uint16_t TSTPCKT:1;        /*!< bit:      7  Test packet mode                   */
    uint16_t OPMODE2:1;        /*!< bit:      8  Specific Operational Mode          */
    uint16_t GNAK:1;           /*!< bit:      9  Global NAK                         */
    uint16_t LPMHDSK:2;        /*!< bit: 10..11  Link Power Management Handshake    */
    uint16_t :4;               /*!< bit: 12..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_DEVICE_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_CTRLB : (USB Offset: 0x08) (R/W 16) HOST Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t :1;               /*!< bit:      0  Reserved                           */
    uint16_t RESUME:1;         /*!< bit:      1  Send USB Resume                    */
    uint16_t SPDCONF:2;        /*!< bit:  2.. 3  Speed Configuration for Host       */
    uint16_t AUTORESUME:1;     /*!< bit:      4  Auto Resume Enable                 */
    uint16_t TSTJ:1;           /*!< bit:      5  Test mode J                        */
    uint16_t TSTK:1;           /*!< bit:      6  Test mode K                        */
    uint16_t :1;               /*!< bit:      7  Reserved                           */
    uint16_t SOFE:1;           /*!< bit:      8  Start of Frame Generation Enable   */
    uint16_t BUSRESET:1;       /*!< bit:      9  Send USB Reset                     */
    uint16_t VBUSOK:1;         /*!< bit:     10  VBUS is OK                         */
    uint16_t L1RESUME:1;       /*!< bit:     11  Send L1 Resume                     */
    uint16_t :4;               /*!< bit: 12..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_HOST_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_DADD : (USB Offset: 0x0A) (R/W 8) DEVICE Device Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DADD:7;           /*!< bit:  0.. 6  Device Address                     */
    uint8_t  ADDEN:1;          /*!< bit:      7  Device Address Enable              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_DEVICE_DADD_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_HSOFC : (USB Offset: 0x0A) (R/W 8) HOST Host Start Of Frame Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  FLENC:4;          /*!< bit:  0.. 3  Frame Length Control               */
    uint8_t  :3;               /*!< bit:  4.. 6  Reserved                           */
    uint8_t  FLENCE:1;         /*!< bit:      7  Frame Length Control Enable        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_HSOFC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_STATUS : (USB Offset: 0x0C) ( R/ 8) DEVICE Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  :2;               /*!< bit:  0.. 1  Reserved                           */
    uint8_t  SPEED:2;          /*!< bit:  2.. 3  Speed Status                       */
    uint8_t  :2;               /*!< bit:  4.. 5  Reserved                           */
    uint8_t  LINESTATE:2;      /*!< bit:  6.. 7  USB Line State Status              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_DEVICE_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_STATUS : (USB Offset: 0x0C) (R/W 8) HOST Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  :2;               /*!< bit:  0.. 1  Reserved                           */
    uint8_t  SPEED:2;          /*!< bit:  2.. 3  Speed Status                       */
    uint8_t  :2;               /*!< bit:  4.. 5  Reserved                           */
    uint8_t  LINESTATE:2;      /*!< bit:  6.. 7  USB Line State Status              */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_FSMSTATUS : (USB Offset: 0x0D) ( R/ 8) Finite State Machine Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  FSMSTATE:7;       /*!< bit:  0.. 6  Fine State Machine Status          */
    uint8_t  :1;               /*!< bit:      7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_FSMSTATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_FNUM : (USB Offset: 0x10) ( R/ 16) DEVICE Device Frame Number -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t MFNUM:3;          /*!< bit:  0.. 2  Micro Frame Number                 */
    uint16_t FNUM:11;          /*!< bit:  3..13  Frame Number                       */
    uint16_t :1;               /*!< bit:     14  Reserved                           */
    uint16_t FNCERR:1;         /*!< bit:     15  Frame Number CRC Error             */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_DEVICE_FNUM_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_FNUM : (USB Offset: 0x10) (R/W 16) HOST Host Frame Number -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t MFNUM:3;          /*!< bit:  0.. 2  Micro Frame Number                 */
    uint16_t FNUM:11;          /*!< bit:  3..13  Frame Number                       */
    uint16_t :2;               /*!< bit: 14..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_HOST_FNUM_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_FLENHIGH : (USB Offset: 0x12) ( R/ 8) HOST Host Frame Length -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  FLENHIGH:8;       /*!< bit:  0.. 7  Frame Length                       */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} USB_HOST_FLENHIGH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_INTENCLR : (USB Offset: 0x14) (R/W 16) DEVICE Device Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SUSPEND:1;        /*!< bit:      0  Suspend Interrupt Enable           */
    uint16_t MSOF:1;           /*!< bit:      1  Micro Start of Frame Interrupt Enable in High Speed Mode */
    uint16_t SOF:1;            /*!< bit:      2  Start Of Frame Interrupt Enable    */
    uint16_t EORST:1;          /*!< bit:      3  End of Reset Interrupt Enable      */
    uint16_t WAKEUP:1;         /*!< bit:      4  Wake Up Interrupt Enable           */
    uint16_t EORSM:1;          /*!< bit:      5  End Of Resume Interrupt Enable     */
    uint16_t UPRSM:1;          /*!< bit:      6  Upstream Resume Interrupt Enable   */
    uint16_t RAMACER:1;        /*!< bit:      7  Ram Access Interrupt Enable        */
    uint16_t LPMNYET:1;        /*!< bit:      8  Link Power Management Not Yet Interrupt Enable */
    uint16_t LPMSUSP:1;        /*!< bit:      9  Link Power Management Suspend Interrupt Enable */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_DEVICE_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_INTENCLR : (USB Offset: 0x14) (R/W 16) HOST Host Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint16_t HSOF:1;           /*!< bit:      2  Host Start Of Frame Interrupt Disable */
    uint16_t RST:1;            /*!< bit:      3  BUS Reset Interrupt Disable        */
    uint16_t WAKEUP:1;         /*!< bit:      4  Wake Up Interrupt Disable          */
    uint16_t DNRSM:1;          /*!< bit:      5  DownStream to Device Interrupt Disable */
    uint16_t UPRSM:1;          /*!< bit:      6  Upstream Resume from Device Interrupt Disable */
    uint16_t RAMACER:1;        /*!< bit:      7  Ram Access Interrupt Disable       */
    uint16_t DCONN:1;          /*!< bit:      8  Device Connection Interrupt Disable */
    uint16_t DDISC:1;          /*!< bit:      9  Device Disconnection Interrupt Disable */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_HOST_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_INTENSET : (USB Offset: 0x18) (R/W 16) DEVICE Device Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SUSPEND:1;        /*!< bit:      0  Suspend Interrupt Enable           */
    uint16_t MSOF:1;           /*!< bit:      1  Micro Start of Frame Interrupt Enable in High Speed Mode */
    uint16_t SOF:1;            /*!< bit:      2  Start Of Frame Interrupt Enable    */
    uint16_t EORST:1;          /*!< bit:      3  End of Reset Interrupt Enable      */
    uint16_t WAKEUP:1;         /*!< bit:      4  Wake Up Interrupt Enable           */
    uint16_t EORSM:1;          /*!< bit:      5  End Of Resume Interrupt Enable     */
    uint16_t UPRSM:1;          /*!< bit:      6  Upstream Resume Interrupt Enable   */
    uint16_t RAMACER:1;        /*!< bit:      7  Ram Access Interrupt Enable        */
    uint16_t LPMNYET:1;        /*!< bit:      8  Link Power Management Not Yet Interrupt Enable */
    uint16_t LPMSUSP:1;        /*!< bit:      9  Link Power Management Suspend Interrupt Enable */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_DEVICE_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_INTENSET : (USB Offset: 0x18) (R/W 16) HOST Host Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t :2;               /*!< bit:  0.. 1  Reserved                           */
    uint16_t HSOF:1;           /*!< bit:      2  Host Start Of Frame Interrupt Enable */
    uint16_t RST:1;            /*!< bit:      3  Bus Reset Interrupt Enable         */
    uint16_t WAKEUP:1;         /*!< bit:      4  Wake Up Interrupt Enable           */
    uint16_t DNRSM:1;          /*!< bit:      5  DownStream to the Device Interrupt Enable */
    uint16_t UPRSM:1;          /*!< bit:      6  Upstream Resume fromthe device Interrupt Enable */
    uint16_t RAMACER:1;        /*!< bit:      7  Ram Access Interrupt Enable        */
    uint16_t DCONN:1;          /*!< bit:      8  Link Power Management Interrupt Enable */
    uint16_t DDISC:1;          /*!< bit:      9  Device Disconnection Interrupt Enable */
    uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_HOST_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_INTFLAG : (USB Offset: 0x1C) (R/W 16) DEVICE Device Interrupt Flag -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint16_t SUSPEND:1;        /*!< bit:      0  Suspend                            */
    __I uint16_t MSOF:1;           /*!< bit:      1  Micro Start of Frame in High Speed Mode */
    __I uint16_t SOF:1;            /*!< bit:      2  Start Of Frame                     */
    __I uint16_t EORST:1;          /*!< bit:      3  End of Reset                       */
    __I uint16_t WAKEUP:1;         /*!< bit:      4  Wake Up                            */
    __I uint16_t EORSM:1;          /*!< bit:      5  End Of Resume                      */
    __I uint16_t UPRSM:1;          /*!< bit:      6  Upstream Resume                    */
    __I uint16_t RAMACER:1;        /*!< bit:      7  Ram Access                         */
    __I uint16_t LPMNYET:1;        /*!< bit:      8  Link Power Management Not Yet      */
    __I uint16_t LPMSUSP:1;        /*!< bit:      9  Link Power Management Suspend      */
    __I uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_DEVICE_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_INTFLAG : (USB Offset: 0x1C) (R/W 16) HOST Host Interrupt Flag -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint16_t :2;               /*!< bit:  0.. 1  Reserved                           */
    __I uint16_t HSOF:1;           /*!< bit:      2  Host Start Of Frame                */
    __I uint16_t RST:1;            /*!< bit:      3  Bus Reset                          */
    __I uint16_t WAKEUP:1;         /*!< bit:      4  Wake Up                            */
    __I uint16_t DNRSM:1;          /*!< bit:      5  Downstream                         */
    __I uint16_t UPRSM:1;          /*!< bit:      6  Upstream Resume from the Device    */
    __I uint16_t RAMACER:1;        /*!< bit:      7  Ram Access                         */
    __I uint16_t DCONN:1;          /*!< bit:      8  Device Connection                  */
    __I uint16_t DDISC:1;          /*!< bit:      9  Device Disconnection               */
    __I uint16_t :6;               /*!< bit: 10..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_HOST_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DEVICE_EPINTSMRY : (USB Offset: 0x20) ( R/ 16) DEVICE End Point Interrupt Summary -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t EPINT0:1;         /*!< bit:      0  End Point 0 Interrupt              */
    uint16_t EPINT1:1;         /*!< bit:      1  End Point 1 Interrupt              */
    uint16_t EPINT2:1;         /*!< bit:      2  End Point 2 Interrupt              */
    uint16_t EPINT3:1;         /*!< bit:      3  End Point 3 Interrupt              */
    uint16_t EPINT4:1;         /*!< bit:      4  End Point 4 Interrupt              */
    uint16_t EPINT5:1;         /*!< bit:      5  End Point 5 Interrupt              */
    uint16_t EPINT6:1;         /*!< bit:      6  End Point 6 Interrupt              */
    uint16_t EPINT7:1;         /*!< bit:      7  End Point 7 Interrupt              */
    uint16_t :8;               /*!< bit:  8..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t EPINT:8;          /*!< bit:  0.. 7  End Point x Interrupt              */
    uint16_t :8;               /*!< bit:  8..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_DEVICE_EPINTSMRY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_HOST_PINTSMRY : (USB Offset: 0x20) ( R/ 16) HOST Pipe Interrupt Summary -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t EPINT0:1;         /*!< bit:      0  Pipe 0 Interrupt                   */
    uint16_t EPINT1:1;         /*!< bit:      1  Pipe 1 Interrupt                   */
    uint16_t EPINT2:1;         /*!< bit:      2  Pipe 2 Interrupt                   */
    uint16_t EPINT3:1;         /*!< bit:      3  Pipe 3 Interrupt                   */
    uint16_t EPINT4:1;         /*!< bit:      4  Pipe 4 Interrupt                   */
    uint16_t EPINT5:1;         /*!< bit:      5  Pipe 5 Interrupt                   */
    uint16_t EPINT6:1;         /*!< bit:      6  Pipe 6 Interrupt                   */
    uint16_t EPINT7:1;         /*!< bit:      7  Pipe 7 Interrupt                   */
    uint16_t :8;               /*!< bit:  8..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t EPINT:8;          /*!< bit:  0.. 7  Pipe x Interrupt                   */
    uint16_t :8;               /*!< bit:  8..15  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_HOST_PINTSMRY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_DESCADD : (USB Offset: 0x24) (R/W 32) Descriptor Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DESCADD:32;       /*!< bit:  0..31  Descriptor Address Value           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} USB_DESCADD_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- USB_PADCAL : (USB Offset: 0x28) (R/W 16) USB PAD Calibration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t TRANSP:5;         /*!< bit:  0.. 4  USB Pad Transp calibration         */
    uint16_t :1;               /*!< bit:      5  Reserved                           */
    uint16_t TRANSN:5;         /*!< bit:  6..10  USB Pad Transn calibration         */
    uint16_t :1;               /*!< bit:     11  Reserved                           */
    uint16_t TRIM:3;           /*!< bit: 12..14  USB Pad Trim calibration           */
    uint16_t :1;               /*!< bit:     15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} USB_PADCAL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief UsbDeviceDescBank SRAM registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO USB_DEVICE_ADDR_Type      ADDR;        /**< \brief Offset: 0x000 (R/W 32) DEVICE_DESC_BANK Endpoint Bank, Adress of Data Buffer */
  __IO USB_DEVICE_PCKSIZE_Type   PCKSIZE;     /**< \brief Offset: 0x004 (R/W 32) DEVICE_DESC_BANK Endpoint Bank, Packet Size */
  __IO USB_DEVICE_EXTREG_Type    EXTREG;      /**< \brief Offset: 0x008 (R/W 16) DEVICE_DESC_BANK Endpoint Bank, Extended */
  __IO USB_DEVICE_STATUS_BK_Type STATUS_BK;   /**< \brief Offset: 0x00A (R/W  8) DEVICE_DESC_BANK Enpoint Bank, Status of Bank */
       RoReg8                    Reserved1[0x5];
} UsbDeviceDescBank;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief UsbHostDescBank SRAM registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO USB_HOST_ADDR_Type        ADDR;        /**< \brief Offset: 0x000 (R/W 32) HOST_DESC_BANK Host Bank, Adress of Data Buffer */
  __IO USB_HOST_PCKSIZE_Type     PCKSIZE;     /**< \brief Offset: 0x004 (R/W 32) HOST_DESC_BANK Host Bank, Packet Size */
  __IO USB_HOST_EXTREG_Type      EXTREG;      /**< \brief Offset: 0x008 (R/W 16) HOST_DESC_BANK Host Bank, Extended */
  __IO USB_HOST_STATUS_BK_Type   STATUS_BK;   /**< \brief Offset: 0x00A (R/W  8) HOST_DESC_BANK Host Bank, Status of Bank */
       RoReg8                    Reserved1[0x1];
  __IO USB_HOST_CTRL_PIPE_Type   CTRL_PIPE;   /**< \brief Offset: 0x00C (R/W 16) HOST_DESC_BANK Host Bank, Host Control Pipe */
  __IO USB_HOST_STATUS_PIPE_Type STATUS_PIPE; /**< \brief Offset: 0x00E (R/W 16) HOST_DESC_BANK Host Bank, Host Status Pipe */
} UsbHostDescBank;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief UsbDeviceEndpoint hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO USB_DEVICE_EPCFG_Type     EPCFG;       /**< \brief Offset: 0x000 (R/W  8) DEVICE_ENDPOINT End Point Configuration */
       RoReg8                    Reserved1[0x3];
  __O  USB_DEVICE_EPSTATUSCLR_Type EPSTATUSCLR; /**< \brief Offset: 0x004 ( /W  8) DEVICE_ENDPOINT End Point Pipe Status Clear */
  __O  USB_DEVICE_EPSTATUSSET_Type EPSTATUSSET; /**< \brief Offset: 0x005 ( /W  8) DEVICE_ENDPOINT End Point Pipe Status Set */
  __I  USB_DEVICE_EPSTATUS_Type  EPSTATUS;    /**< \brief Offset: 0x006 (R/   8) DEVICE_ENDPOINT End Point Pipe Status */
  __IO USB_DEVICE_EPINTFLAG_Type EPINTFLAG;   /**< \brief Offset: 0x007 (R/W  8) DEVICE_ENDPOINT End Point Interrupt Flag */
  __IO USB_DEVICE_EPINTENCLR_Type EPINTENCLR;  /**< \brief Offset: 0x008 (R/W  8) DEVICE_ENDPOINT End Point Interrupt Clear Flag */
  __IO USB_DEVICE_EPINTENSET_Type EPINTENSET;  /**< \brief Offset: 0x009 (R/W  8) DEVICE_ENDPOINT End Point Interrupt Set Flag */
       RoReg8                    Reserved2[0x16];
} UsbDeviceEndpoint;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief UsbHostPipe hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO USB_HOST_PCFG_Type        PCFG;        /**< \brief Offset: 0x000 (R/W  8) HOST_PIPE End Point Configuration */
       RoReg8                    Reserved1[0x2];
  __IO USB_HOST_BINTERVAL_Type   BINTERVAL;   /**< \brief Offset: 0x003 (R/W  8) HOST_PIPE Bus Access Period of Pipe */
  __O  USB_HOST_PSTATUSCLR_Type  PSTATUSCLR;  /**< \brief Offset: 0x004 ( /W  8) HOST_PIPE End Point Pipe Status Clear */
  __O  USB_HOST_PSTATUSSET_Type  PSTATUSSET;  /**< \brief Offset: 0x005 ( /W  8) HOST_PIPE End Point Pipe Status Set */
  __I  USB_HOST_PSTATUS_Type     PSTATUS;     /**< \brief Offset: 0x006 (R/   8) HOST_PIPE End Point Pipe Status */
  __IO USB_HOST_PINTFLAG_Type    PINTFLAG;    /**< \brief Offset: 0x007 (R/W  8) HOST_PIPE Pipe Interrupt Flag */
  __IO USB_HOST_PINTENCLR_Type   PINTENCLR;   /**< \brief Offset: 0x008 (R/W  8) HOST_PIPE Pipe Interrupt Flag Clear */
  __IO USB_HOST_PINTENSET_Type   PINTENSET;   /**< \brief Offset: 0x009 (R/W  8) HOST_PIPE Pipe Interrupt Flag Set */
       RoReg8                    Reserved2[0x16];
} UsbHostPipe;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief USB_DEVICE APB hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* USB is Device */
  __IO USB_CTRLA_Type            CTRLA;       /**< \brief Offset: 0x000 (R/W  8) Control A */
       RoReg8                    Reserved1[0x1];
  __I  USB_SYNCBUSY_Type         SYNCBUSY;    /**< \brief Offset: 0x002 (R/   8) Synchronization Busy */
  __IO USB_QOSCTRL_Type          QOSCTRL;     /**< \brief Offset: 0x003 (R/W  8) USB Quality Of Service */
       RoReg8                    Reserved2[0x4];
  __IO USB_DEVICE_CTRLB_Type     CTRLB;       /**< \brief Offset: 0x008 (R/W 16) DEVICE Control B */
  __IO USB_DEVICE_DADD_Type      DADD;        /**< \brief Offset: 0x00A (R/W  8) DEVICE Device Address */
       RoReg8                    Reserved3[0x1];
  __I  USB_DEVICE_STATUS_Type    STATUS;      /**< \brief Offset: 0x00C (R/   8) DEVICE Status */
  __I  USB_FSMSTATUS_Type        FSMSTATUS;   /**< \brief Offset: 0x00D (R/   8) Finite State Machine Status */
       RoReg8                    Reserved4[0x2];
  __I  USB_DEVICE_FNUM_Type      FNUM;        /**< \brief Offset: 0x010 (R/  16) DEVICE Device Frame Number */
       RoReg8                    Reserved5[0x2];
  __IO USB_DEVICE_INTENCLR_Type  INTENCLR;    /**< \brief Offset: 0x014 (R/W 16) DEVICE Device Interrupt Enable Clear */
       RoReg8                    Reserved6[0x2];
  __IO USB_DEVICE_INTENSET_Type  INTENSET;    /**< \brief Offset: 0x018 (R/W 16) DEVICE Device Interrupt Enable Set */
       RoReg8                    Reserved7[0x2];
  __IO USB_DEVICE_INTFLAG_Type   INTFLAG;     /**< \brief Offset: 0x01C (R/W 16) DEVICE Device Interrupt Flag */
       RoReg8                    Reserved8[0x2];
  __I  USB_DEVICE_EPINTSMRY_Type EPINTSMRY;   /**< \brief Offset: 0x020 (R/  16) DEVICE End Point Interrupt Summary */
       RoReg8                    Reserved9[0x2];
  __IO USB_DESCADD_Type          DESCADD;     /**< \brief Offset: 0x024 (R/W 32) Descriptor Address */
  __IO USB_PADCAL_Type           PADCAL;      /**< \brief Offset: 0x028 (R/W 16) USB PAD Calibration */
       RoReg8                    Reserved10[0xD6];
       UsbDeviceEndpoint         DeviceEndpoint[8]; /**< \brief Offset: 0x100 UsbDeviceEndpoint groups [EPT_NUM] */
} UsbDevice;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief USB_HOST hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* USB is Host */
  __IO USB_CTRLA_Type            CTRLA;       /**< \brief Offset: 0x000 (R/W  8) Control A */
       RoReg8                    Reserved1[0x1];
  __I  USB_SYNCBUSY_Type         SYNCBUSY;    /**< \brief Offset: 0x002 (R/   8) Synchronization Busy */
  __IO USB_QOSCTRL_Type          QOSCTRL;     /**< \brief Offset: 0x003 (R/W  8) USB Quality Of Service */
       RoReg8                    Reserved2[0x4];
  __IO USB_HOST_CTRLB_Type       CTRLB;       /**< \brief Offset: 0x008 (R/W 16) HOST Control B */
  __IO USB_HOST_HSOFC_Type       HSOFC;       /**< \brief Offset: 0x00A (R/W  8) HOST Host Start Of Frame Control */
       RoReg8                    Reserved3[0x1];
  __IO USB_HOST_STATUS_Type      STATUS;      /**< \brief Offset: 0x00C (R/W  8) HOST Status */
  __I  USB_FSMSTATUS_Type        FSMSTATUS;   /**< \brief Offset: 0x00D (R/   8) Finite State Machine Status */
       RoReg8                    Reserved4[0x2];
  __IO USB_HOST_FNUM_Type        FNUM;        /**< \brief Offset: 0x010 (R/W 16) HOST Host Frame Number */
  __I  USB_HOST_FLENHIGH_Type    FLENHIGH;    /**< \brief Offset: 0x012 (R/   8) HOST Host Frame Length */
       RoReg8                    Reserved5[0x1];
  __IO USB_HOST_INTENCLR_Type    INTENCLR;    /**< \brief Offset: 0x014 (R/W 16) HOST Host Interrupt Enable Clear */
       RoReg8                    Reserved6[0x2];
  __IO USB_HOST_INTENSET_Type    INTENSET;    /**< \brief Offset: 0x018 (R/W 16) HOST Host Interrupt Enable Set */
       RoReg8                    Reserved7[0x2];
  __IO USB_HOST_INTFLAG_Type     INTFLAG;     /**< \brief Offset: 0x01C (R/W 16) HOST Host Interrupt Flag */
       RoReg8                    Reserved8[0x2];
  __I  USB_HOST_PINTSMRY_Type    PINTSMRY;    /**< \brief Offset: 0x020 (R/  16) HOST Pipe Interrupt Summary */
       RoReg8                    Reserved9[0x2];
  __IO USB_DESCADD_Type          DESCADD;     /**< \brief Offset: 0x024 (R/W 32) Descriptor Address */
  __IO USB_PADCAL_Type           PADCAL;      /**< \brief Offset: 0x028 (R/W 16) USB PAD Calibration */
       RoReg8                    Reserved10[0xD6];
       UsbHostPipe               HostPipe[8]; /**< \brief Offset: 0x100 UsbHostPipe groups [PIPE_NUM*HOST_IMPLEMENTED] */
} UsbHost;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief USB_DEVICE Descriptor SRAM registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* USB is Device */
       UsbDeviceDescBank         DeviceDescBank[2]; /**< \brief Offset: 0x000 UsbDeviceDescBank groups */
} UsbDeviceDescriptor;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief USB_HOST Descriptor SRAM registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct { /* USB is Host */
       UsbHostDescBank           HostDescBank[2]; /**< \brief Offset: 0x000 UsbHostDescBank groups [2*HOST_IMPLEMENTED] */
} UsbHostDescriptor;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#define SECTION_USB_DESCRIPTOR

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
       UsbDevice                 DEVICE;      /**< \brief Offset: 0x000 USB is Device */
       UsbHost                   HOST;        /**< \brief Offset: 0x000 USB is Host */
} Usb;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_USB_COMPONENT_FIXUP_H_ */
