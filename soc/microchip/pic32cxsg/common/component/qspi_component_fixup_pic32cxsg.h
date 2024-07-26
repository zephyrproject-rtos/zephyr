/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_QSPI_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_QSPI_COMPONENT_FIXUP_H_

/* -------- QSPI_CTRLA : (QSPI Offset: 0x00) (R/W 32) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t :22;              /*!< bit:  2..23  Reserved                           */
    uint32_t LASTXFER:1;       /*!< bit:     24  Last Transfer                      */
    uint32_t :7;               /*!< bit: 25..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- QSPI_CTRLB : (QSPI Offset: 0x04) (R/W 32) Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t MODE:1;           /*!< bit:      0  Serial Memory Mode                 */
    uint32_t LOOPEN:1;         /*!< bit:      1  Local Loopback Enable              */
    uint32_t WDRBT:1;          /*!< bit:      2  Wait Data Read Before Transfer     */
    uint32_t SMEMREG:1;        /*!< bit:      3  Serial Memory reg                  */
    uint32_t CSMODE:2;         /*!< bit:  4.. 5  Chip Select Mode                   */
    uint32_t :2;               /*!< bit:  6.. 7  Reserved                           */
    uint32_t DATALEN:4;        /*!< bit:  8..11  Data Length                        */
    uint32_t :4;               /*!< bit: 12..15  Reserved                           */
    uint32_t DLYBCT:8;         /*!< bit: 16..23  Delay Between Consecutive Transfers */
    uint32_t DLYCS:8;          /*!< bit: 24..31  Minimum Inactive CS Delay          */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- QSPI_BAUD : (QSPI Offset: 0x08) (R/W 32) Baud Rate -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t CPOL:1;           /*!< bit:      0  Clock Polarity                     */
    uint32_t CPHA:1;           /*!< bit:      1  Clock Phase                        */
    uint32_t :6;               /*!< bit:  2.. 7  Reserved                           */
    uint32_t BAUD:8;           /*!< bit:  8..15  Serial Clock Baud Rate             */
    uint32_t DLYBS:8;          /*!< bit: 16..23  Delay Before SCK                   */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_BAUD_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- QSPI_RXDATA : (QSPI Offset: 0x0C) ( R/ 32) Receive Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DATA:16;          /*!< bit:  0..15  Receive Data                       */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_RXDATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- QSPI_TXDATA : (QSPI Offset: 0x10) ( /W 32) Transmit Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DATA:16;          /*!< bit:  0..15  Transmit Data                      */
    uint32_t :16;              /*!< bit: 16..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_TXDATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- QSPI_INTENCLR : (QSPI Offset: 0x14) (R/W 32) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXC:1;            /*!< bit:      0  Receive Data Register Full Interrupt Disable */
    uint32_t DRE:1;            /*!< bit:      1  Transmit Data Register Empty Interrupt Disable */
    uint32_t TXC:1;            /*!< bit:      2  Transmission Complete Interrupt Disable */
    uint32_t ERROR:1;          /*!< bit:      3  Overrun Error Interrupt Disable    */
    uint32_t :4;               /*!< bit:  4.. 7  Reserved                           */
    uint32_t CSRISE:1;         /*!< bit:      8  Chip Select Rise Interrupt Disable */
    uint32_t :1;               /*!< bit:      9  Reserved                           */
    uint32_t INSTREND:1;       /*!< bit:     10  Instruction End Interrupt Disable  */
    uint32_t :21;              /*!< bit: 11..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- QSPI_INTENSET : (QSPI Offset: 0x18) (R/W 32) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t RXC:1;            /*!< bit:      0  Receive Data Register Full Interrupt Enable */
    uint32_t DRE:1;            /*!< bit:      1  Transmit Data Register Empty Interrupt Enable */
    uint32_t TXC:1;            /*!< bit:      2  Transmission Complete Interrupt Enable */
    uint32_t ERROR:1;          /*!< bit:      3  Overrun Error Interrupt Enable     */
    uint32_t :4;               /*!< bit:  4.. 7  Reserved                           */
    uint32_t CSRISE:1;         /*!< bit:      8  Chip Select Rise Interrupt Enable  */
    uint32_t :1;               /*!< bit:      9  Reserved                           */
    uint32_t INSTREND:1;       /*!< bit:     10  Instruction End Interrupt Enable   */
    uint32_t :21;              /*!< bit: 11..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- QSPI_INTFLAG : (QSPI Offset: 0x1C) (R/W 32) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint32_t RXC:1;            /*!< bit:      0  Receive Data Register Full         */
    __I uint32_t DRE:1;            /*!< bit:      1  Transmit Data Register Empty       */
    __I uint32_t TXC:1;            /*!< bit:      2  Transmission Complete              */
    __I uint32_t ERROR:1;          /*!< bit:      3  Overrun Error                      */
    __I uint32_t :4;               /*!< bit:  4.. 7  Reserved                           */
    __I uint32_t CSRISE:1;         /*!< bit:      8  Chip Select Rise                   */
    __I uint32_t :1;               /*!< bit:      9  Reserved                           */
    __I uint32_t INSTREND:1;       /*!< bit:     10  Instruction End                    */
    __I uint32_t :21;              /*!< bit: 11..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- QSPI_STATUS : (QSPI Offset: 0x20) ( R/ 32) Status Register -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t :1;               /*!< bit:      0  Reserved                           */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t :7;               /*!< bit:  2.. 8  Reserved                           */
    uint32_t CSSTATUS:1;       /*!< bit:      9  Chip Select                        */
    uint32_t :22;              /*!< bit: 10..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- QSPI_INSTRADDR : (QSPI Offset: 0x30) (R/W 32) Instruction Address -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ADDR:32;          /*!< bit:  0..31  Instruction Address                */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_INSTRADDR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- QSPI_INSTRCTRL : (QSPI Offset: 0x34) (R/W 32) Instruction Code -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t INSTR:8;          /*!< bit:  0.. 7  Instruction Code                   */
    uint32_t :8;               /*!< bit:  8..15  Reserved                           */
    uint32_t OPTCODE:8;        /*!< bit: 16..23  Option Code                        */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_INSTRCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- QSPI_INSTRFRAME : (QSPI Offset: 0x38) (R/W 32) Instruction Frame -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t WIDTH:3;          /*!< bit:  0.. 2  Instruction Code, Address, Option Code and Data Width */
    uint32_t :1;               /*!< bit:      3  Reserved                           */
    uint32_t INSTREN:1;        /*!< bit:      4  Instruction Enable                 */
    uint32_t ADDREN:1;         /*!< bit:      5  Address Enable                     */
    uint32_t OPTCODEEN:1;      /*!< bit:      6  Option Enable                      */
    uint32_t DATAEN:1;         /*!< bit:      7  Data Enable                        */
    uint32_t OPTCODELEN:2;     /*!< bit:  8.. 9  Option Code Length                 */
    uint32_t ADDRLEN:1;        /*!< bit:     10  Address Length                     */
    uint32_t :1;               /*!< bit:     11  Reserved                           */
    uint32_t TFRTYPE:2;        /*!< bit: 12..13  Data Transfer Type                 */
    uint32_t CRMODE:1;         /*!< bit:     14  Continuous Read Mode               */
    uint32_t DDREN:1;          /*!< bit:     15  Double Data Rate Enable            */
    uint32_t DUMMYLEN:5;       /*!< bit: 16..20  Dummy Cycles Length                */
    uint32_t :11;              /*!< bit: 21..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_INSTRFRAME_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- QSPI_SCRAMBCTRL : (QSPI Offset: 0x40) (R/W 32) Scrambling Mode -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t ENABLE:1;         /*!< bit:      0  Scrambling/Unscrambling Enable     */
    uint32_t RANDOMDIS:1;      /*!< bit:      1  Scrambling/Unscrambling Random Value Disable */
    uint32_t :30;              /*!< bit:  2..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_SCRAMBCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- QSPI_SCRAMBKEY : (QSPI Offset: 0x44) ( /W 32) Scrambling Key -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t KEY:32;           /*!< bit:  0..31  Scrambling User Key                */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} QSPI_SCRAMBKEY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief QSPI APB hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO QSPI_CTRLA_Type           CTRLA;       /**< \brief Offset: 0x00 (R/W 32) Control A */
  __IO QSPI_CTRLB_Type           CTRLB;       /**< \brief Offset: 0x04 (R/W 32) Control B */
  __IO QSPI_BAUD_Type            BAUD;        /**< \brief Offset: 0x08 (R/W 32) Baud Rate */
  __I  QSPI_RXDATA_Type          RXDATA;      /**< \brief Offset: 0x0C (R/  32) Receive Data */
  __O  QSPI_TXDATA_Type          TXDATA;      /**< \brief Offset: 0x10 ( /W 32) Transmit Data */
  __IO QSPI_INTENCLR_Type        INTENCLR;    /**< \brief Offset: 0x14 (R/W 32) Interrupt Enable Clear */
  __IO QSPI_INTENSET_Type        INTENSET;    /**< \brief Offset: 0x18 (R/W 32) Interrupt Enable Set */
  __IO QSPI_INTFLAG_Type         INTFLAG;     /**< \brief Offset: 0x1C (R/W 32) Interrupt Flag Status and Clear */
  __I  QSPI_STATUS_Type          STATUS;      /**< \brief Offset: 0x20 (R/  32) Status Register */
       RoReg8                    Reserved1[0xC];
  __IO QSPI_INSTRADDR_Type       INSTRADDR;   /**< \brief Offset: 0x30 (R/W 32) Instruction Address */
  __IO QSPI_INSTRCTRL_Type       INSTRCTRL;   /**< \brief Offset: 0x34 (R/W 32) Instruction Code */
  __IO QSPI_INSTRFRAME_Type      INSTRFRAME;  /**< \brief Offset: 0x38 (R/W 32) Instruction Frame */
       RoReg8                    Reserved2[0x4];
  __IO QSPI_SCRAMBCTRL_Type      SCRAMBCTRL;  /**< \brief Offset: 0x40 (R/W 32) Scrambling Mode */
  __O  QSPI_SCRAMBKEY_Type       SCRAMBKEY;   /**< \brief Offset: 0x44 ( /W 32) Scrambling Key */
} Qspi;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_QSPI_COMPONENT_FIXUP_H_ */
